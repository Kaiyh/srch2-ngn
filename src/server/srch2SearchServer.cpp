// $Id: chat.cpp 1217 2011-06-15 20:59:17Z srch2.vijay $
/*
 * This file is part of the Mongoose project, http://code.google.com/p/mongoose
 * It implements an online chat server. For more details,
 * see the documentation on the project web site.
 * To test the application,
 *  1. type "make" in the directory where this file lives
 *  2. point your browser to http://127.0.0.1:8081
 *
 * NOTE(lsm): this file follows Google style, not BSD style as the rest of
 * Mongoose code.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <string>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#if defined( __MACH__) || defined(ANDROID)
// This header is used only in mac osx related code
#include <arpa/inet.h>
#endif
#include "HTTPRequestHandler.h"
#include "license/LicenseVerifier.h"
#include "util/Logger.h"
#include "util/Version.h"
#include <event2/http.h>
#include <event2/thread.h>
#include "event2/event.h"
#include <signal.h>

#include <sys/types.h>
#include <map>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include "util/FileOps.h"
#include "analyzer/AnalyzerContainers.cpp"
#include "WrapperConstants.h"

#include "transport/TransportManager.h"
#include "processor/DistributedProcessorExternal.h"
#include "configuration/ConfigManager.h"
#include "synchronization/SynchronizerManager.h"
#include "sharding/sharding/ShardManager.h"
#include "sharding/sharding/metadata_manager/MetadataInitializer.h"
#include "sharding/sharding/metadata_manager/ResourceMetadataManager.h"
#include "discovery/DiscoveryManager.h"
#include "migration/MigrationManager.h"
#include "Srch2Server.h"

#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>


namespace po = boost::program_options;
namespace srch2is = srch2::instantsearch;
namespace srch2http = srch2::httpwrapper;

using srch2http::HTTPRequestHandler;
using srch2http::ConfigManager;
using namespace srch2::util;

using std::string;

#define MAX_USER_LEN  20
#define MAX_MESSAGE_LEN  100
#define MAX_MESSAGES 5
#define MAX_SESSIONS 2
#define SESSION_TTL 120


pthread_t *threadsToHandleExternalRequests = NULL;
pthread_t *threadsToHandleInternalRequests = NULL;
unsigned int MAX_THREADS = 0;
unsigned int MAX_INTERNAL_THREADS = 0;
srch2http::TransportManager *transportManager;
// These are global variables that store host and port information for srch2 engine
unsigned short globalDefaultPort;
const char *globalHostName;

// map from port numbers (shared among cores) to socket file descriptors
// IETF RFC 6335 specifies port number range is 0 - 65535: http://tools.ietf.org/html/rfc6335#page-11
typedef std::map<unsigned short /*portNumber*/, int /*fd*/> PortSocketMap_t;

/* Convert an amount of bytes into a human readable string in the form
 * of 100B, 2G, 100M, 4K, and so forth.
 * Thanks Redis */
void bytesToHuman(char *s, unsigned long long n) {
	double d;

	if (n < 1024) {
		/* Bytes */
		sprintf(s, "%lluB", n);
		return;
	} else if (n < (1024 * 1024)) {
		d = (double) n / (1024);
		sprintf(s, "%.2fK", d);
	} else if (n < (1024LL * 1024 * 1024)) {
		d = (double) n / (1024 * 1024);
		sprintf(s, "%.2fM", d);
	} else if (n < (1024LL * 1024 * 1024 * 1024)) {
		d = (double) n / (1024LL * 1024 * 1024);
		sprintf(s, "%.2fG", d);
	}
}

std::string getCurrentVersion() {
	return Version::getCurrentVersion();
}

/**
 * NotFound event handler.
 * @param req evhttp request object
 * @param arg optional argument
 */
static void cb_notfound(evhttp_request *req, void *arg)
{
	evhttp_add_header(req->output_headers, "Content-Type",
			"application/json; charset=UTF-8");
	try {
		evhttp_send_reply(req, HTTP_NOTFOUND, "Not found", NULL);
	} catch (exception& e) {
		// exception caught
		Logger::error(e.what());
		srch2http::HTTPRequestHandler::handleException(req);
	}
}

/*
 * Helper function to parse HTTP port from it's input headers.  Assumes a little knowledge of http_request
 * internals in libevent.
 *
 * Typical usage:
 * void cb_search(evhttp_request *req, void *arg)
 * {
 *     int portNumber = getLibeventHttpRequestPort(req);
 * }
 *
 * where cb_search() is a callback invoked by libevent.
 * If "req" is from "localhost:8082/core1/search", then this function will extract "8082" from "req".
 */
static unsigned short int getLibeventHttpRequestPort(struct evhttp_request *req)
{
	const char *host = NULL;
	const char *p;
	unsigned short int port = 0; // IETF RFC 6335 specifies port number range is 0 - 65535: http://tools.ietf.org/html/rfc6335#page-11


	host = evhttp_find_header(req->input_headers, "Host");
	/* The Host: header may include a port. Look for it here, else return -1 as an error. */
	if (host) {
		p = strchr(host,  ':');
		if (p != NULL) {
			p++; // skip past colon
			port = 0;
			while (isdigit(*p)) {
				unsigned int newValue = (10 * port) + (*p++ - '0'); // check for overflow (must be int)
				if (newValue <= USHRT_MAX) {
					port = newValue;
				} else {
					port = 0;
					break;
				}
			}
			if (*p != '\000') {
				Logger::error("Did not reach end of Host input header");
				port = 0;
			}
		}
	}

	return port;
}

static const struct portMap_t {
	srch2http::PortType_t portType;
	const char *operationName;
} portTypeOperationMap[] = {
		{ srch2http::SearchPort, "search" },
		{ srch2http::SuggestPort, "suggest" },
		{ srch2http::InfoPort, "info" },
		{ srch2http::DocsPort, "docs" },
		{ srch2http::UpdatePort, "update" },
		{ srch2http::SavePort, "save" },
		{ srch2http::ExportPort, "export" },
		{ srch2http::ResetLoggerPort, "resetlogger" },
		{ srch2http::CommitPort, "commit" },
		{ srch2http::MergePort, "merge" },
		{ srch2http::EndOfPortType, NULL },
};

struct ExternalOperationArguments{
	srch2::httpwrapper::DPExternalRequestHandler * dpExternal;
	unsigned coreId;
};

static bool checkOperationPermission(evhttp_request *req, 
		unsigned coreId, srch2http::PortType_t portType) {
/*	if (portType >= srch2http::EndOfPortType) {
		Logger::error("Illegal port type: %d", static_cast<int> (portType));
		cb_notfound(req, NULL);
		return false;
	}

	const srch2http::CoreInfo_t *const coreInfo = &coreShardInfo->core;
	unsigned short configuredPort = coreInfo->getPort(portType);
	if(!configuredPort) configuredPort = globalDefaultPort;
	unsigned short arrivalPort = getLibeventHttpRequestPort(req);

	if(!arrivalPort) {
		Logger::warn("Unable to ascertain arrival port from request headers.");
		return false;
	}

	if(!configuredPort) {
		//error
		//TODO: make sure cm return defaultPort on portType which are not set
		// this operation not set to a specific port so only accepted
		// on the default port
	}

	// compare arrival port to configuration file port
	if(configuredPort != arrivalPort) {
		string coreName = coreInfo->getName();
		Logger::warn("/%s request for %s core arriving on port %d"
				" denied (port %d will permit)",
				portTypeOperationMap[portType].operationName, coreName.c_str(),
				arrivalPort, configuredPort);

		cb_notfound(req, NULL);
		return false;
	}*/
	return true;
}

/**
 * 'search' callback function
 * @param req evhttp request object
 * @param arg optional argument
 */
static void cb_search(evhttp_request *req, void *arg) {
	ExternalOperationArguments * dpExternalAndCoreId = (ExternalOperationArguments * )arg;

	evhttp_add_header(req->output_headers, "Content-Type",
			"application/json; charset=UTF-8");

	if(!checkOperationPermission(req, dpExternalAndCoreId->coreId, srch2http::SearchPort))
		return;

	try {
		dpExternalAndCoreId->dpExternal->externalSearchCommand(req, dpExternalAndCoreId->coreId);
	} catch (exception& e) {
		// exception caught
		Logger::error(e.what());
		srch2http::HTTPRequestHandler::handleException(req);
	}
}

/**
 * 'suggest' callback function
 * @param req evhttp request object
 * @param arg optional argument
 */
static void cb_suggest(evhttp_request *req, void *arg) {
	ExternalOperationArguments * dpExternalAndCoreId = (ExternalOperationArguments * )arg;

	evhttp_add_header(req->output_headers, "Content-Type",
			"application/json; charset=UTF-8");

	if(!checkOperationPermission(req, dpExternalAndCoreId->coreId, srch2http::SuggestPort))
		return;

	try {
//		dpExternalAndCoreId->dpExternal->externalSuggestCommand(req, dpExternalAndCoreId->coreId);
	} catch (exception& e) {
		// exception caught
		Logger::error(e.what());
		srch2http::HTTPRequestHandler::handleException(req);
	}
}

/**
 * 'info' callback function
 * @param req evhttp request object
 * @param arg optional argument
 */
static void cb_info(evhttp_request *req, void *arg) {
	ExternalOperationArguments * dpExternalAndCoreId = (ExternalOperationArguments * )arg;

	evhttp_add_header(req->output_headers, "Content-Type",
			"application/json; charset=UTF-8");

	if(!checkOperationPermission(req, dpExternalAndCoreId->coreId, srch2http::InfoPort))
		return;

	try {
		dpExternalAndCoreId->dpExternal->externalGetInfoCommand(req, dpExternalAndCoreId->coreId);
	} catch (exception& e) {
		// exception caught
		Logger::error(e.what());
		srch2http::HTTPRequestHandler::handleException(req);
	}
}

/**
 * 'write/v1/' callback function
 * @param req evhttp request object
 * @param arg optional argument
 */
static void cb_write(evhttp_request *req, void *arg) {
	ExternalOperationArguments * dpExternalAndCoreId = (ExternalOperationArguments * )arg;

	evhttp_add_header(req->output_headers, "Content-Type",
			"application/json; charset=UTF-8");

	if(!checkOperationPermission(req, dpExternalAndCoreId->coreId, srch2http::DocsPort))
		return;

	try {
	    if(req->type == EVHTTP_REQ_PUT){
            dpExternalAndCoreId->dpExternal->externalInsertCommand(req, dpExternalAndCoreId->coreId);
	    }else if(req->type == EVHTTP_REQ_DELETE){
	    	Logger::console("Delete request came ...");
	    	dpExternalAndCoreId->dpExternal->externalDeleteCommand(req, dpExternalAndCoreId->coreId);
	    }
	} catch (exception& e) {
		// exception caught
		Logger::error(e.what());
		srch2http::HTTPRequestHandler::handleException(req);
	}
}

static void cb_update(evhttp_request *req, void *arg) {
	ExternalOperationArguments * dpExternalAndCoreId = (ExternalOperationArguments * )arg;
	evhttp_add_header(req->output_headers, "Content-Type",
			"application/json; charset=UTF-8");

	if(!checkOperationPermission(req, dpExternalAndCoreId->coreId, srch2http::UpdatePort))
		return;

	try {
		dpExternalAndCoreId->dpExternal->externalUpdateCommand(req, dpExternalAndCoreId->coreId);
	} catch (exception& e) {
		// exception caught
		Logger::error(e.what());
		srch2http::HTTPRequestHandler::handleException(req);
	}

}

static void cb_save(evhttp_request *req, void *arg) {
	ExternalOperationArguments * dpExternalAndCoreId = (ExternalOperationArguments * )arg;
	evhttp_add_header(req->output_headers, "Content-Type",
			"application/json; charset=UTF-8");

	if(!checkOperationPermission(req, dpExternalAndCoreId->coreId, srch2http::SavePort))
		return;

	try {
		dpExternalAndCoreId->dpExternal->externalSerializeIndexCommand(req, dpExternalAndCoreId->coreId);
	} catch (exception& e) {
		// exception caught
		Logger::error(e.what());
		srch2http::HTTPRequestHandler::handleException(req);
	}
}

static void cb_export(evhttp_request *req, void *arg) {
	ExternalOperationArguments * dpExternalAndCoreId = (ExternalOperationArguments * )arg;

	evhttp_add_header(req->output_headers, "Content-Type",
			"application/json; charset=UTF-8");

	if(!checkOperationPermission(req, dpExternalAndCoreId->coreId, srch2http::ExportPort))
		return;

	try{
         dpExternalAndCoreId->dpExternal->externalSerializeRecordsCommand(req, dpExternalAndCoreId->coreId);
	} catch(exception& e){
        // exception caught
        Logger::error(e.what());
        srch2http::HTTPRequestHandler::handleException(req);
	}
}

/**
 *  'reset logger file' callback function
 *  @param req evhttp request object
 *  @param arg optional argument
 */
static void cb_resetLogger(evhttp_request *req, void *arg) {
	ExternalOperationArguments * dpExternalAndCoreId = (ExternalOperationArguments * )arg;

	evhttp_add_header(req->output_headers, "Content-Type",
			"application/json; charset=UTF-8");

	if(!checkOperationPermission(req, dpExternalAndCoreId->coreId, srch2http::ResetLoggerPort))
		return;

	try {
		dpExternalAndCoreId->dpExternal->externalResetLogCommand(req, dpExternalAndCoreId->coreId);
	} catch (exception& e) {
		// exception caught
		Logger::error(e.what());
		srch2http::HTTPRequestHandler::handleException(req);
	}
}

/**
 *  'comit' callback function
 *  @param req evhttp request object
 *  @param arg optional argument
 */
static void cb_commit(evhttp_request *req, void *arg) {
	ExternalOperationArguments * dpExternalAndCoreId = (ExternalOperationArguments * )arg;

	evhttp_add_header(req->output_headers, "Content-Type",
			"application/json; charset=UTF-8");

	if(!checkOperationPermission(req, dpExternalAndCoreId->coreId, srch2http::CommitPort))
		return;

	try {
		dpExternalAndCoreId->dpExternal->externalCommitCommand(req, dpExternalAndCoreId->coreId);
	} catch (exception& e) {
		// exception caught
		Logger::error(e.what());
		srch2http::HTTPRequestHandler::handleException(req);
	}
}


/**
 *  'comit' callback function
 *  @param req evhttp request object
 *  @param arg optional argument
 */
static void cb_merge(evhttp_request *req, void *arg) {
	ExternalOperationArguments * dpExternalAndCoreId = (ExternalOperationArguments * )arg;

	evhttp_add_header(req->output_headers, "Content-Type",
			"application/json; charset=UTF-8");

	if(!checkOperationPermission(req, dpExternalAndCoreId->coreId, srch2http::MergePort))
		return;

	try {
		dpExternalAndCoreId->dpExternal->externalMergeCommand(req, dpExternalAndCoreId->coreId);
	} catch (exception& e) {
		// exception caught
		Logger::error(e.what());
		srch2http::HTTPRequestHandler::handleException(req);
	}
}

/**
 * Busy 409 event handler.
 * @param req evhttp request object
 * @param arg optional argument
 */
static void cb_busy_indexing(evhttp_request *req, void *arg) {
	evhttp_add_header(req->output_headers, "Content-Type",
			"application/json; charset=UTF-8");
	try {
		evhttp_send_reply(req, 409, "Indexer busy", NULL);
	} catch (exception& e) {
		// exception caught
		Logger::error(e.what());
		srch2http::HTTPRequestHandler::handleException(req);
	}
}

void printVersion() {
	std::cout << "SRCH2 server version:" << getCurrentVersion() << std::endl;
}


/*
 * TODO : needs comments
 */
int bindSocket(const char * hostname, unsigned short port) {
	int r;
	int nfd;
	nfd = socket(AF_INET, SOCK_STREAM, 0);
	if (nfd < 0)
		return -1;

	int one = 1;
	r = setsockopt(nfd, SOL_SOCKET, SO_REUSEADDR, (char *) &one, sizeof(int));
	if(r < 0){
		cerr << "Set socket option failed." << endl;  // TODO : change it to Logger
		return 0;
	}

	// ignore a SIGPIPE signal (http://stackoverflow.com/questions/108183/how-to-prevent-sigpipes-or-handle-them-properly)
	signal(SIGPIPE, SIG_IGN);

	struct sockaddr_in addr;
	struct hostent * host = gethostbyname(hostname);
	if (host == NULL)
		return -1;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	bcopy((char *) host->h_addr,
			(char *)&addr.sin_addr.s_addr,
			host->h_length);
	addr.sin_port = htons(port);

	r = bind(nfd, (struct sockaddr*) &addr, sizeof(addr));
	if (r < 0)
		return -1;
	r = listen(nfd, 10240);
	if (r < 0)
		return -1;

	int flags;
	if ((flags = fcntl(nfd, F_GETFL, 0)) < 0
			|| fcntl(nfd, F_SETFL, flags | O_NONBLOCK) < 0)
		return -1;

	return nfd;
}

// entry point for each thread
void* dispatch(void *arg) {
	// ask libevent to loop on events
	event_base_dispatch((struct event_base*) arg);
	return NULL;
}

void* dispatchInternalEvent(void *arg) {
	while(true) {
		event_base_dispatch((struct event_base*) arg);
		sleep(1);
	}
	return NULL;
}

void parseProgramArguments(int argc, char** argv,
		po::options_description& description,
		po::variables_map& vm_command_line_args) {
	description.add_options()("config-file",po::value<string>(), "Path to the config file")
            		("help", "Prints help message")
            		("version", "Prints version number of the engine");
	try {
		po::store(po::parse_command_line(argc, argv, description),
				vm_command_line_args);
		po::notify(vm_command_line_args);
	} catch (exception &ex) {
		cout << "error while parsing the arguments : " << endl << ex.what()
                		<< endl;
		cout << "Usage: <SRCH2_HOME>/bin/srch2-engine" << endl;
		cout << description << endl;
		exit(-1);
	}
}


#ifdef __MACH__
/*
 *  dummy request handler for make_http_request function below.
 */
void dummyRequestHandler(struct evhttp_request *req, void *state) {
	if (req == NULL) {
		Logger::debug("timed out!\n");
	} else if (req->response_code == 0) {
		Logger::debug("connection refused!\n");
	} else if (req->response_code != 200) {
		Logger::debug("error: %u %s\n", req->response_code, req->response_code_line);
	} else {
		Logger::debug("success: %u %s\n", req->response_code, req->response_code_line);
	}
}
/*
 *  Creates a http request for /info Rest API of srch2 engine.
 */
void makeHttpRequest(){
	struct evhttp_connection *conn;
	struct evhttp_request *req;
	/*
	 * evhttp_connection_new does not perform dns lookup and expects numeric ip address
	 * Hence we should do explicit coversion before calling evhttp_connection_new
	 */
	char hostIpAddr[20];
	memset(hostIpAddr, 0, sizeof(hostIpAddr));
	struct hostent * host = gethostbyname(globalHostName);
	if (host == NULL) {
		// nothing much can be done..let us try 0.0.0.0
		strncpy(hostIpAddr, "0.0.0.0", 7);
	} else {
		// convert binary ip address to human readable ip address.
		struct in_addr **addr_list = (struct in_addr **) host->h_addr_list;
		strcpy(hostIpAddr, inet_ntoa(*addr_list[0]));
	}
	conn = evhttp_connection_new( hostIpAddr, globalDefaultPort);
	evhttp_connection_set_timeout(conn, 1);
	req = evhttp_request_new(dummyRequestHandler, (void*)NULL);
	evhttp_make_request(conn, req, EVHTTP_REQ_GET, "/info");
}
#endif

/**
 * Kill the server.  This function can be called from another thread to kill the server
 */

static void killServer(int signal) {
    Logger::console("Stopping server.");
    for (int i = 0; i < MAX_THREADS; i++) {
#ifndef ANDROID
    	// Android thread implementation does not have pthread_cancel()
    	// use pthread_kill instead.
    	pthread_cancel(threadsToHandleExternalRequests[i]);
#endif
    }
    for (int i = 0; i < MAX_INTERNAL_THREADS; i++) {
#ifndef ANDROID
    	pthread_cancel(threadsToHandleInternalRequests[i]);
#endif
    }
    for(srch2http::ConnectionMap::iterator conn =
        transportManager->getConnectionMap().begin();
        conn != transportManager->getConnectionMap().end(); ++conn) {
      close(conn->second.fd);
    }
#ifndef ANDROID
    pthread_cancel(transportManager->getListeningThread());
#endif

#ifdef ANDROID
    exit(0);
#endif

#ifdef __MACH__
	/*
	 *  In MacOS, pthread_cancel could not cancel a thread when the thread is executing kevent syscall
	 *  which is a blocking call. In other words, our engine threads are not cancelled while they
	 *  are waiting for http requests. So we send a dummy http request to our own engine from within
	 *  the engine. This request allows the threads to come out of blocking syscall and get killed.
	 */
	makeHttpRequest();
#endif
}

static int getHttpServerMetadata(ConfigManager *config, PortSocketMap_t *globalPortSocketMap) {
	// TODO : we should use default port if no port is provided. or just get rid of default port and move it to node
	// as a <defaultPort>
	std::set<short> ports;

	// global host name
	globalHostName = config->getHTTPServerListeningHostname().c_str();

	// add the default port
	globalDefaultPort = atoi(config->getHTTPServerDefaultListeningPort().c_str());
	if (globalDefaultPort > 0) {
		ports.insert(globalDefaultPort);
	}

	// loop over operations and extract all port numbers of current node to use
	unsigned short port;
	const map<srch2http::PortType_t, unsigned short int> & allPorts = config->getHTTPServerListeningPorts();
	for (srch2http::PortType_t portType = (srch2http::PortType_t) 0;
			portType < srch2http::EndOfPortType; portType = srch2http::incrementPortType(portType)) {

		port = allPorts.at(portType);
		if(port > 0){
			ports.insert(port);
		}
	}

	// bind once each port defined for use by this core
	for(std::set<short>::const_iterator port = ports.begin();
			port != ports.end() ; ++port) {
		int socketFd = bindSocket(globalHostName, *port);
		if(socketFd < 0) {
			perror("socket bind error");
			return 255;
		}
		(*globalPortSocketMap)[*port] = socketFd;
	}
	return 0;
}

/*
 * This function just creates pairs of event_base and http_server objects
 * in the maximum number of threads.
 */
static int createHTTPServersAndAccompanyingThreads(int MAX_THREADS,
		vector<struct event_base *> *evBases, vector<struct evhttp *> *evServers) {
	// for each thread, we bind an evbase and http_server object
	// TODO : do we need to deallocate these objects anywhere ?

	threadsToHandleExternalRequests = new pthread_t[MAX_THREADS];
	for (int i = 0; i < MAX_THREADS; i++) {
		struct event_base *evbase = event_base_new();
		if(!evbase) {
			perror("event_base_new");
			return 255;
		}
		evBases->push_back(evbase);

		struct evhttp *http_server = evhttp_new(evbase);
		if(!http_server) {
			perror("evhttp_new");
			return 255;
		}
		evServers->push_back(http_server);
	}
	return 0;
}


/*
 * This function just creates pairs of event_base and threads.
 */
static int createInternalReqEventBasesAndAccompanyingThreads(int MAX_INTERNAL_THREADS,
		vector<struct event_base *> *evBases) {
	// for each thread, we bind an evbase and http_server object
	// TODO : do we need to deallocate these objects anywhere ?
	threadsToHandleInternalRequests = new pthread_t[MAX_INTERNAL_THREADS];
	for (int i = 0; i < MAX_INTERNAL_THREADS; i++) {
		struct event_base *evbase = event_base_new();
		if(!evbase) {
			perror("event_base_new");
			return 255;
		}
		evBases->push_back(evbase);
	}
	return 0;
}

// helper array - loop instead of repetitous code
static const struct UserRequestAttributes_t {
	const char *path;
	srch2http::PortType_t portType;
	void (*callback)(struct evhttp_request *, void *);
} userRequestAttributesList[] = {
		{ "/search", srch2http::SearchPort, cb_search },
		{ "/suggest", srch2http::SuggestPort, cb_suggest },
		{ "/info", srch2http::InfoPort, cb_info },
		{ "/docs", srch2http::DocsPort, cb_write },
		{ "/update", srch2http::UpdatePort, cb_update },
		{ "/save", srch2http::SavePort, cb_save },
		{ "/export", srch2http::ExportPort, cb_export },
		{ "/resetLogger", srch2http::ResetLoggerPort, cb_resetLogger },
		{ NULL, srch2http::EndOfPortType, NULL }
};
typedef unsigned CoreId;//TODO is it needed ? if not let's delete it.

/*
 * This function sets the callback functions to be used for different types of
 * user request URLs.
 * For URLs w/o core name we use the default core (only if default core is actually defined in config manager)
 * For URLs with core names we use the specified core object.
 */
int setCallBacksonHTTPServer(ConfigManager *const config,
		evhttp *const http_server,
		boost::shared_ptr<const srch2::httpwrapper::ClusterResourceMetadata_Readview> & clusterReadview,
		srch2::httpwrapper::DPExternalRequestHandler * dpExternal) {

	// setup default core callbacks for queries without a core name
	// only if default core is available.
	if(config->getDefaultCoreSetFlag() == true) {
		// prepare arguments to pass to external commands for default core
		ExternalOperationArguments * defaultArgs = new ExternalOperationArguments();
		defaultArgs->dpExternal = dpExternal;
		defaultArgs->coreId = clusterReadview->getCoreByName(config->getDefaultCoreName())->getCoreId();
		// iterate on all operations and map the path (w/o core info)
		// to a callback function.
		// we pass DPExternalCoreHandle as the argument of callbacks
		const map<srch2http::PortType_t, unsigned short int> & allPorts = config->getHTTPServerListeningPorts();
		for (int j = 0; userRequestAttributesList[j].path != NULL; j++) {


			evhttp_set_cb(http_server, userRequestAttributesList[j].path,
					userRequestAttributesList[j].callback, defaultArgs);

			// just for print

			unsigned short port = allPorts.at(userRequestAttributesList[j].portType);
			if (port < 1) port = globalDefaultPort;
			Logger::debug("Routing port %d route %s to default core %s",
					port, userRequestAttributesList[j].path, config->getDefaultCoreName().c_str());
		}
	}



	evhttp_set_gencb(http_server, cb_notfound, NULL);

	// for every core, for every OTHER port that core uses, do accept
	// NOTE : CoreInfoMap is a typedef of std::map<const string, CoreInfo_t *>
	std::vector<const srch2http::CoreInfo_t * > allCores;
	clusterReadview->getAllCores(allCores);
	for(unsigned i = 0 ; i < allCores.size(); ++i) {
		string coreName = allCores.at(i)->getName();
		unsigned coreId = allCores.at(i)->getCoreId();

		// prepare external command argument
		ExternalOperationArguments * args = new ExternalOperationArguments();
		args->dpExternal = dpExternal;
		args->coreId = coreId;

		for(unsigned int j = 0; userRequestAttributesList[j].path != NULL; j++) {
			string path = string("/") + coreName + string(userRequestAttributesList[j].path);
			evhttp_set_cb(http_server, path.c_str(),
					userRequestAttributesList[j].callback, args);

			// just for print
			const map<srch2http::PortType_t, unsigned short int> & allPorts = config->getHTTPServerListeningPorts();
			unsigned short port = allPorts.at(userRequestAttributesList[j].portType);
			if(port < 1){
				port = globalDefaultPort;
			}
			Logger::debug("Adding port %d route %s to core %s",
					port, path.c_str(), coreName.c_str());
		}
	}

	return 0;
}

int startListeningToRequest(evhttp *const http_server, 
		PortSocketMap_t& globalPortSocketMap) {
	/* 4). accept bound socket */
	for(PortSocketMap_t::iterator iterator = globalPortSocketMap.begin();
			iterator != globalPortSocketMap.end(); iterator++) {
		Logger::console("Port %d added to HTTP listener for external requests.", iterator->first);
		if(evhttp_accept_socket(http_server, iterator->second) != 0) {
			perror("evhttp_accept_socket");
			return 255;
		}
		//  Logger::debug("Socket accept by thread %d on port %d", i, iterator->first);
	}
	return 0;
}

int main(int argc, char** argv) {
	if (argc > 1) {
		if (strcmp(argv[1], "--version") == 0) {
			printVersion();
			return 0;
		}
	}
	// Parse command line arguments
	po::options_description description("Valid Arguments");
	po::variables_map vm_command_line_args;
	parseProgramArguments(argc, argv, description, vm_command_line_args);

	if (vm_command_line_args.count("help")) {
		cout << "Usage: <SRCH2_HOME>/bin/srch2-engine" << endl;
		cout << description << endl;
		return 0;
	}

	std::string srch2_config_file = "";
	if (vm_command_line_args.count("config-file")) {
		srch2_config_file = vm_command_line_args["config-file"].as<string>();
		int status = ::access(srch2_config_file.c_str(), F_OK);
		if (status != 0) {
			std::cout << "config file = '" << srch2_config_file
					<< "' not found or could not be read" << std::endl;
			return -1;
		}
	} else {
		cout << "Usage: <SRCH2_HOME>/bin/srch2-engine" << endl;
		cout << description << endl;
		exit(-1);
	}

	ConfigManager *serverConf = new ConfigManager(srch2_config_file);

	// cluster metadata is populated and commited in this function for the first time,
	// from now, MM is responsible for doing this.
	srch2http::ResourceMetadataManager * metadataManager = new srch2http::ResourceMetadataManager();
	srch2http::ShardManager * shardManager = srch2http::ShardManager::createShardManager(serverConf, metadataManager);

	serverConf->loadConfigFile(metadataManager);

	LicenseVerifier::testFile(serverConf->getLicenseKeyFileName());
	string logDir = getFilePath(serverConf->getHTTPServerAccessLogFile());
	// If the path does't exist, try to create it.
	if (!logDir.empty() && !checkDirExistence(logDir.c_str())) {
		if (createDir(logDir.c_str()) == -1) {
			exit(1);
		}
	}
	FILE *logFile = fopen(serverConf->getHTTPServerAccessLogFile().c_str(),
			"a");
	if (logFile == NULL) {
		Logger::setOutputFile(stdout);
		Logger::error("Open Log file %s failed.",
				serverConf->getHTTPServerAccessLogFile().c_str());
	} else {
		Logger::setOutputFile(logFile);
	}
	Logger::setLogLevel(serverConf->getHTTPServerLogLevel());

	//TODO to remove
	srch2::util::Logger::setLogLevel(srch2::util::Logger::SRCH2_LOG_DEBUG);


	// create shard manager
	// cores information is populated in CM but nodes map is not populated yet.
	// writeview is prepared in good shape by having 0 for every place that we need currentNodeId.
	// This is feasible because at this point we must not have any information in writeview about any other nodes
	// because the information coming from disk is not usable.
	srch2http::MetadataInitializer nodeInitializer(serverConf, metadataManager);
	nodeInitializer.initializeNode();

	// TM pointer must be set later when node information is provided to SHM.


	// all libevent base objects (one per thread)
	vector<struct event_base *> evBasesForExternalRequests;
	vector<struct event_base *> evBasesForInternalRequests;
	vector<struct evhttp *> evServersForExternalRequests;
	// map of all ports across all cores to shared socket file descriptors
	PortSocketMap_t globalPortSocketMap;

	int start = getHttpServerMetadata(serverConf, &globalPortSocketMap);

	if (start != 0) {
		Logger::close();
		return start; // startup failed
	}

	MAX_THREADS = serverConf->getNumberOfThreads();
	MAX_INTERNAL_THREADS = 3; // TODO : read from config file.

    evthread_use_pthreads();
	// create threads for external requests
	createHTTPServersAndAccompanyingThreads(MAX_THREADS, &evBasesForExternalRequests, &evServersForExternalRequests);

	// create threads for internal requests
	//TODO set the number of threads for internal messaging
	createInternalReqEventBasesAndAccompanyingThreads(MAX_INTERNAL_THREADS, &evBasesForInternalRequests);

	// create Transport Module
	srch2http::TransportConfig transportConfig;
	transportConfig.interfaceAddress = serverConf->getTransport().getIpAddress();
	transportConfig.internalCommunicationPort = serverConf->getTransport().getPort();

	transportManager = new srch2http::TransportManager(evBasesForInternalRequests, transportConfig);
	shardManager->attachToTransportManager(transportManager);
	// since internal threads are not dispatched yet, it's safe to set the callback handler of shard manager.

	// start threads for internal messages.
	// Note: eventbases are not assigned any event yet.
	for(int j=0; j < evBasesForInternalRequests.size(); ++j){
		if (pthread_create(&threadsToHandleInternalRequests[j], NULL, dispatchInternalEvent, evBasesForInternalRequests[j]) != 0){
			return 255;
		}
	}
	/*
	 *  Start Synchronization manager thread. It performs following operation.
	 *  1. Start discovery thread to find existing cluster and join it. If there is no cluster
	 *     then this node becomes master.
	 *  2. Once the node joins the cluster, SM will use TM to setup a connection with master, so
	 *     that the connection can be used for inter-node communications.
	 *  3. SM get cluster info from master and setup connection with all the node in master.
	 */

	srch2http::SyncManager  *syncManager = new srch2http::SyncManager(*serverConf, *transportManager);
	// Discovery of SyncManager adds {currentNodeId, list of nodes} to the writeview.
	// we must start the ShardManager thread here before SM even has a chance to call it to
	// not miss any notifications from SM.
	syncManager->startDiscovery();
	// at this point, node information is also available. So we can safely notify shard manager
	// that you have all the initial node information and can start distributed initialization.

	// Set ShardManager pointer (used for callbacks) to syncManager before it starts working.
	shardManager->start();

	pthread_t *synchronizerThread = new pthread_t;
	pthread_create(synchronizerThread, NULL, srch2http::bootSynchronizer, (void *)syncManager);

	// create DP Internal
	srch2http::DPInternalRequestHandler * dpInternal =
			new srch2http::DPInternalRequestHandler(serverConf);

	// TEST CODE FOR MIGRATION MANAGER
//	serverConf->getClusterReadView(clusterReadview);
//	if (clusterReadview->getCurrentNode()->isMaster()) {
//		cout << "waiting for new node " << endl;
//		while(1) {
//			//boost::shared_ptr<const srch2::httpwrapper::Cluster> clusterReadview;
//			serverConf->getClusterReadView(clusterReadview);
//			if (clusterReadview->getTotalNumberOfNodes() == 2) {
//				cout << "new Node found ..start migration ..." << endl;
//				std::vector<const srch2http::Node *> nodes;
//				clusterReadview->getAllNodes(nodes);
//
//				unsigned currNodeId = clusterReadview->getCurrentNode()->getId();
//				std::vector< const srch2http::CoreShardContainer * >  coreShardContainers;
//				clusterReadview->getNodeShardInformation(currNodeId, coreShardContainers);
//
//				if (coreShardContainers.size() > 0) {
//					vector<srch2http::Shard *> * shardPtr = ((srch2http::CoreShardContainer *)coreShardContainers[0])->getPrimaryShards();
//					if (shardPtr != NULL) {
//						if (shardPtr->size() > 0) {
//							unsigned destinationNodeId =  nodes[1]->getId();
//							if (currNodeId == destinationNodeId) {
//								destinationNodeId =  nodes[0]->getId();
//							}
//
							//std::ostringstream outputBuffer(std::ios::out|std::ios::binary);
//							namespace boostio = boost::iostreams;
//							typedef std::vector<char> buffer_type;
//							buffer_type buffer;
//							boostio::stream<boostio::back_insert_device<buffer_type> > output_stream(buffer);
//
//							cout << "trying to serialize the shard ...." << endl;
//
//
//
//							shardPtr->at(0)->getSrch2Server()->serialize(output_stream);
//
//							//outputBuffer.flush();
//							output_stream.seekp(0, ios::end);
//							unsigned shardSize = output_stream.tellp();
//							cout << "ostream size = " << shardSize << endl;
//							output_stream.seekp(0, ios::beg);
//
//							//std::istringstream inputStream(std::ios::in | std::ios::binary);
//							//inputStream.rdbuf()->pubsetbuf((char *)outputBuffer.str().c_str() , shardSize);
//							//inputStream.str(outputBuffer.str());
//
//							srch2http::Srch2Server * tempSS = new srch2http::Srch2Server(
//									shardPtr->at(0)->getSrch2Server()->getCoreInfo() , shardPtr->at(0)->getShardId(), 1);
//
//							string directoryPath = serverConf->createShardDir(serverConf->getClusterWriteView()->getClusterName(),
//									serverConf->getClusterWriteView()->getCurrentNode()->getName(),
//									shardPtr->at(0)->getSrch2Server()->getCoreInfo()->getName() + "_1", shardPtr->at(0)->getShardId());
//
//							cout << "Saving shard to : "  << directoryPath << endl;
//
//							cout << "buffer size " << buffer.size() << endl;
//							boostio::basic_array_source<char> source(&buffer[0],buffer.size());
//							boostio::stream<boostio::basic_array_source <char> > input_stream(source);
//
//							tempSS->bootStrapIndexerFromByteStream(input_stream, directoryPath);
//							cout << "DONE!! " << endl;

//							boost::shared_ptr<srch2http::Srch2Server> shard = shardPtr->at(0)->getSrch2Server();
//							migrationManager->migrateShard(shardPtr->at(0)->getShardId(), shard
//									, destinationNodeId);
//
//						}
//					} else {
//						exit(-1);
//					}
//				} else {
//					exit(-1);
//				}
//				break;
//			}
//			sleep(2);
//		}
//	}
	/// TEMP CODE END

	// create DP external
	srch2http::DPExternalRequestHandler *dpExternal =
			new srch2http::DPExternalRequestHandler(*serverConf, *transportManager, *dpInternal);


	// get read view to use for system startup
	boost::shared_ptr<const srch2::httpwrapper::ClusterResourceMetadata_Readview> clusterReadview;
	srch2::httpwrapper::ShardManager::getReadview(clusterReadview);

	// bound http_server and evbase and core objects together
	for(int j=0; j < evServersForExternalRequests.size(); ++j) {
		setCallBacksonHTTPServer(serverConf, evServersForExternalRequests[j], clusterReadview, dpExternal);
		startListeningToRequest(evServersForExternalRequests[j], globalPortSocketMap);

		if (pthread_create(&threadsToHandleExternalRequests[j], NULL, dispatch, evBasesForExternalRequests[j]) != 0){
			return 255;
		}
	}

	clusterReadview.reset();
	/* Set signal handlers */
	sigset_t sigset;
	sigemptyset(&sigset);

	// handle signal of Ctrl-C interruption
	sigaddset(&sigset, SIGINT);
	// handle signal of terminate(kill)
	sigaddset(&sigset, SIGTERM);
	struct sigaction siginfo;
	// add the handler to be killServer, sa_sigaction and sa_handler are union type, so we don't need to assign sa_sigaction to be NULL
	siginfo.sa_handler = killServer;
	siginfo.sa_mask = sigset;
	// If a blocked call to one of the following interfaces is interrupted by a signal handler,
	// then the call will be automatically restarted after the signal handler returns if the SA_RESTART flag was used;
	// otherwise the call will fail with the error EINTR, check the detail at http://man7.org/linux/man-pages/man7/signal.7.html
	siginfo.sa_flags = SA_RESTART;
	sigaction(SIGINT, &siginfo, NULL);
	sigaction(SIGTERM, &siginfo, NULL);

	for (int i = 0; i < MAX_THREADS; i++) {
		pthread_join(threadsToHandleExternalRequests[i], NULL);
		Logger::console("Thread = <%u> stopped", threadsToHandleExternalRequests[i]);
	}

	for (int i = 0; i < MAX_INTERNAL_THREADS; i++) {
		pthread_join(threadsToHandleInternalRequests[i], NULL);
		Logger::console("Thread = <%u> stopped", threadsToHandleInternalRequests[i]);
	}

	pthread_join(transportManager->getListeningThread(), NULL);
	Logger::console("Thread = <%u> stopped", transportManager->getListeningThread());
#ifndef ANDROID
	pthread_cancel(*synchronizerThread);
#else
	pthread_kill(*synchronizerThread, SIGKILL);
#endif
	pthread_join(*synchronizerThread, NULL);
	Logger::console("synch thread stopped.");

	delete[] threadsToHandleExternalRequests;
	delete[] threadsToHandleInternalRequests;

	for (unsigned int i = 0; i < MAX_THREADS; i++) {
		event_base_free(evBasesForExternalRequests[i]);
	}
	for (unsigned int i = 0; i < MAX_INTERNAL_THREADS; i++) {
		event_base_free(evBasesForInternalRequests[i]);
	}

	// use global port map to close each file descriptor just once
	for (PortSocketMap_t::iterator portSocket = globalPortSocketMap.begin(); portSocket != globalPortSocketMap.end(); portSocket++) {
		shutdown(portSocket->second, SHUT_RD);
	}

	AnalyzerContainer::freeAll();
	delete serverConf;

	Logger::console("Server stopped successfully");
	Logger::close();
	return EXIT_SUCCESS;
}
