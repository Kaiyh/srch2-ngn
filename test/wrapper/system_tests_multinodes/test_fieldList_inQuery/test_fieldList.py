#this test is used for checking field list parameter specified in the query.
#using: python ./test_fieldList_inQuery/test_fieldList.py $SRCH2_ENGINE ./test_fieldList_inQuery/queriesAndResults.txt
#We check it by specifying the field list in the query and comparing the returned response with the expected result. The expected result contain only those field which we mention in the query.

import sys, urllib2, json, time, subprocess, os, commands, signal

sys.path.insert(0, 'srch2lib')
import test_lib

#Function of checking the results
def checkResult(query, responseJson,resultValue):
#    for key, value in responseJson:
#        print key, value
    isPass=1
    if  len(responseJson) == len(resultValue):
        for i in range(0, len(resultValue)):
            resultValue[i] = resultValue[i].rstrip('\n')
            expectedRecordJson=json.loads(resultValue[i])
            if (responseJson[i]['record']['id'] !=  expectedRecordJson['id']):
                isPass=0
                print query+' test failed'
                print 'query results||given results'
                print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
                for i in range(0, len(responseJson)):
                    print str(responseJson[i]['record']) +'||'+str(resultValue[i])
                break
    else:
        isPass=0
        print query+' test failed'
        print 'query results||given results'
        print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
        maxLen = max(len(responseJson),len(resultValue))
        for i in range(0, maxLen):
            if i >= len(resultValue):
             print responseJson[i]['record']+'||'
            elif i >= len(responseJson):
             print '  '+'||'+resultValue[i]
            else:
             print responseJson[i]['record']+'||'+resultValue[i]

    if isPass == 1:
        print  query+' test pass'
        return 0
    return 1


#prepare the query based on the valid syntax
def prepareQuery(queryKeywords):
    query = ''
    #################  prepare main query part
    query = query + 'q='
    # local parameters
    query = query + '%7BdefaultPrefixComplete=COMPLETE%7D'
    # keywords section
    for i in range(0, len(queryKeywords)):
        if i == (len(queryKeywords)-1):
            query=query+queryKeywords[i]+'*' # last keyword prefix
        else:
            query=query+queryKeywords[i]+'%20AND%20'
    
    ################# fuzzy parameter
    query = query + '&fuzzy=false&fl=name,category'
    
#    print 'Query : ' + query
    ##################################
    return query
    
def testFieldList(queriesAndResultsPath, args):
    #Start the engine server

    serverHandle = test_lib.startServer(args)
    if serverHandle == None:
        return -1

    #Load initial data
    dataFile = './test_fieldList_inQuery/data.json'
    test_lib.loadIntialData(dataFile)

    #construct the query
    failCount = 0
    f_in = open(queriesAndResultsPath, 'r')
    for line in f_in:
        #get the query keyword and results
        print line 
        value=line.split('||')
        queryValue=value[0]
        resultValue=(value[1])
        #construct the query
        query = prepareQuery([queryValue])
        response_json = test_lib.searchRequest(query)

        #check the result
        failCount += checkResult(query, response_json['results'], [resultValue] )

    test_lib.killServer(serverHandle)
    print '=============================='
    return failCount

if __name__ == '__main__':      
    #Path of the query file
    #each line like "trust||01c90b4effb2353742080000" ---- query||record_ids(results)
    binary_path = sys.argv[1]
    queriesAndResultsPath = sys.argv[2]
    exitCode = testFieldList(queriesAndResultsPath, [ binary_path, "./test_fieldList_inQuery/conf.xml","./test_fieldList_inQuery/conf-A.xml","./test_fieldList_inQuery/conf-B.xml"])
    time.sleep(5)
    exitCode = testFieldList(queriesAndResultsPath, [ binary_path, "./test_fieldList_inQuery/conf1.xml","./test_fieldList_inQuery/conf1-A.xml","./test_fieldList_inQuery/conf1-B.xml"])
    time.sleep(5)
    exitCode = testFieldList(queriesAndResultsPath, [ binary_path, "./test_fieldList_inQuery/conf2.xml","./test_fieldList_inQuery/conf2-A.xml","./test_fieldList_inQuery/conf2-B.xml"])
    time.sleep(5)
    os._exit(exitCode)
