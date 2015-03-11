#using python fuzzy_A1.py queriesAndResults.txt

import sys, urllib2, json, time, subprocess, os, commands,signal

sys.path.insert(0, 'srch2lib')
import test_lib

#the function of checking the results
def checkResult(query, responseJson,resultValue):
    isPass=1
    if  len(responseJson) == len(resultValue):
         for i in range(0, len(resultValue)):
                #print response_json['results'][i]['record']['id']
            if (resultValue.count(responseJson[i]['record']['id']) != 1):
                isPass=0
                print query+' test failed'
                print 'query results||given results'
                print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
                for i in range(0, len(responseJson)):
                    print responseJson[i]['record']['id']+'||'+resultValue[i]
                break
    else:
        isPass=0
        print query+' test failed'
        print 'query results||given results'
        print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
        maxLen = max(len(responseJson),len(resultValue))
        for i in range(0, maxLen):
            if i >= len(resultValue):
                 print responseJson[i]['record']['id']+'||'
            elif i >= len(responseJson):
                 print '  '+'||'+resultValue[i]
            else:
                 print responseJson[i]['record']['id']+'||'+resultValue[i]

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


    ############################ fq parameter
    query = query + '&fq=model%3ABMW' # 'model:BMW'
    query = query + '%20OR%20' # ' OR '
    query = query + 'likes%3A%5B44%20TO%20*%5D' # 'likes:[44 TO *]'
    query = query + '%20OR%20' # ' OR '
    query = query + 'boolexp%24price%3E88%20and%20price%3C96%24' # CMPLX$price>88 and price<96$
    #print 'Query : ' + query
    ##################################
    return query

def testFilterQuery(queriesAndResultsPath, binary_path):
    # Start the engine server
    args = [ binary_path, './filter_query/conf-1.xml', './filter_query/conf-2.xml', './filter_query/conf-3.xml' ]

    serverHandle = test_lib.startServer(args)
    if serverHandle == None:
        return -1

    #Load initial data
    dataFile = './filter_query/data.json'
    test_lib.loadIntialData(dataFile)

    #construct the query

    failCount = 0
    f_in = open(queriesAndResultsPath, 'r')
    for line in f_in:
        #get the query keyword and results
        value=line.split('||')
        queryValue=value[0].split()
        resultValue=(value[1]).split()
        #construct the query
        query = prepareQuery(queryValue)
        response_json = test_lib.searchRequest(query)
      
        #check the result
        failCount += checkResult(query, response_json['results'], resultValue )
    

    test_lib.killServer(serverHandle)
    print '=============================='
    return failCount

if __name__ == '__main__':    
   #Path of the query file
   #each line like "trust||01c90b4effb2353742080000" ---- query||record_ids(results)
   binary_path = sys.argv[1]
   queriesAndResultsPath = sys.argv[2]
   exitCode = testFilterQuery(queriesAndResultsPath, binary_path)
   os._exit(exitCode)

