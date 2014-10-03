/*
 * DataConnectorThread.cpp
 *
 *  Created on: Jun 24, 2014
 *      Author: Chen Liu at SRCH2
 */

#include <dlfcn.h>
#include "DataConnectorThread.h"
#include <iostream>
#include <string>
#include <stdlib.h>
#include <cstdlib>
#include "DataConnector.h"

//Called by the pthread_create, create the database connector
void * spawnConnector(void *arg) {
    ConnectorThreadArguments * connThreadArg = (ConnectorThreadArguments *) arg;
    DataConnectorThread::bootStrapConnector(connThreadArg);

    delete connThreadArg->server;
    delete connThreadArg;

    return NULL;
}

//The main function run by the thread, get connector and start listener.
void DataConnectorThread::bootStrapConnector(ConnectorThreadArguments * connThreadArg) {
    void * pdlHandle = NULL;

    //Get the pointer of the shared library
    std::string libName =  connThreadArg->sharedLibraryFullPath;

    pdlHandle = dlopen(libName.c_str(), RTLD_LAZY); //Open the shared library.

    if (!pdlHandle) {
        Logger::error("Fail to load shared library %s due to %s",
                libName.c_str(), dlerror());
        return;   //Exit from the current thread if can not open the shared library
    }

    /*
     * Suppress specific warnings on gcc/g++.
     * See: http://www.mr-edd.co.uk/blog/supressing_gcc_warnings
     * The warnings g++ spat out is to do with an invalid cast from
     * a pointer-to-object to a pointer-to-function before gcc 4.
     */
#ifdef __GNUC__
    __extension__
#endif
    //Get the function "create" in the shared library.
    create_t* create_dataConnector = (create_t*) dlsym(pdlHandle, "create");

    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        Logger::error(
                "Cannot load symbol \"create\" in shared library due to: %s",
                dlsym_error);
        dlclose(pdlHandle);
        return;   //Exit from the current thread if can not open the shared library
    }

    /*
     * Suppress specific warnings on gcc/g++.
     * See: http://www.mr-edd.co.uk/blog/supressing_gcc_warnings
     * The warnings g++ spat out is to do with an invalid cast from
     * a pointer-to-object to a pointer-to-function before gcc 4.
     */
#ifdef __GNUC__
    __extension__
#endif
    destroy_t* destroy_dataConnector = (destroy_t*) dlsym(pdlHandle, "destroy");

    dlsym_error = dlerror();
    if (dlsym_error) {
        Logger::error(
                "Cannot load symbol \"destroy\" in shared library due to: %s",
                dlsym_error);
        dlclose(pdlHandle);
        return;   //Exit from the current thread if can not open the shared library
    }

    //Call the "create" function in the shared library.
    DataConnector * connector = create_dataConnector();

    if (connector->init(connThreadArg->server) == 0) {
        if (!connThreadArg->createNewIndexFlag) {
            Logger::debug("Create Indices from empty");
            if (connector->createNewIndexes() != 0) {
                //Exit the database connector if create new indexes failed.
                Logger::error("Create Indices Failed.");
                destroy_dataConnector(connector);
                dlclose(pdlHandle);
                return;
            }
        }
        connector->runListener();
    }

    //After the listener;
    destroy_dataConnector(connector);
    dlclose(pdlHandle);
}

//Create thread if interface built successfully.
void DataConnectorThread::getDataConnectorThread(void * server) {
    pthread_t tid;
    ConnectorThreadArguments * dbArg = new ConnectorThreadArguments();
    ServerInterfaceInternal * internal = new ServerInterfaceInternal(server);

    //If the source is not database, do not create the thread.
    if (!internal->isDatabase()) {
        delete dbArg;
        delete internal;
    } else {
        dbArg->server = internal;
        dbArg->createNewIndexFlag = checkIndexExistence(server);

        srch2::httpwrapper::Srch2Server* srch2Server =
                (srch2::httpwrapper::Srch2Server*) server;
        dbArg->sharedLibraryFullPath =
                srch2Server->getCoreInfo()->getSrch2Home() + "/"
                        + srch2Server->getCoreInfo()->getDatabaseSharedLibraryPath()
                        + "/"
                        + srch2Server->getCoreInfo()->getDatabaseSharedLibraryName();
/*
 * Currently only support MAC_OS, LINUX_OS, ANDROID.
 * The engine will load the shared library with corresponding suffix
 *  based on the platform.
 */
#ifdef MAC_OS
    if(dbArg->sharedLibraryFullPath.find(".dylib")==std::string::npos){
        dbArg->sharedLibraryFullPath.append(".dylib");
    }
#else
    if(dbArg->sharedLibraryFullPath.find(".so")==std::string::npos){
        dbArg->sharedLibraryFullPath.append(".so");
    }
#endif

        int res = pthread_create(&tid, NULL, spawnConnector, (void *) dbArg);
    }
}


bool DataConnectorThread::checkIndexExistence(void * server) {
    srch2::httpwrapper::Srch2Server * srch2Server =
            (srch2::httpwrapper::Srch2Server*) server;

    const string &directoryName = srch2Server->getCoreInfo()->getIndexPath();
    if (!checkDirExistence(
            (directoryName + "/" + IndexConfig::analyzerFileName).c_str()))
        return false;
    if (!checkDirExistence(
            (directoryName + "/" + IndexConfig::trieFileName).c_str()))
        return false;
    if (!checkDirExistence(
            (directoryName + "/" + IndexConfig::forwardIndexFileName).c_str()))
        return false;
    if (!checkDirExistence(
            (directoryName + "/" + IndexConfig::schemaFileName).c_str()))
        return false;
    if (srch2Server->getCoreInfo()->getIndexType()
            == srch2::instantsearch::DefaultIndex) {
        // Check existence of the inverted index file for basic keyword search ("A1")
        if (!checkDirExistence(
                (directoryName + "/" + IndexConfig::invertedIndexFileName).c_str()))
            return false;
    } else {
        // Check existence of the quadtree index file for geo keyword search ("M1")
        if (!checkDirExistence(
                (directoryName + "/" + IndexConfig::quadTreeFileName).c_str()))
            return false;
    }
    return true;
}