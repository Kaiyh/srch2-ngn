#!/usr/bin/env python

# File: test_srch2_top_k_pagination.py
# Author: contact@srch2.com
# Retrieves JSON response using
# Srch2-base-search-url for a given query.
# 
# 1) Test order. For example:
# JSON A:
# 1 - a - 0.9 
# 2 - b - 0.8
# 3 - c - 0.8
# 4 - d - 0.8
# 5 - e - 0.5
#
# JSON B:
# 1 - a - 0.9 
# 2 - c - 0.8
# 3 - d - 0.8
# 4 - b - 0.8
# 5 - e - 0.5
#
# JSON C:
# 1 - a - 0.9 
# 2 - b - 0.8
# 3 - c - 0.8
# 4 - d - 0.8
# 5 - f - 0.5
# 6 - e - 0.5
# 7 - g - 0.3
# 
# JSON A, B, C are all valid
#
import sys, urllib2, json, time, subprocess, os, commands, signal

sys.path.insert(0, 'srch2lib')
import test_lib

def resultsScoreToRecordMap(json_response):
     results = json_response["results"]
     resultsScoreToRecordMap = {}
     for item in results:
         try:
             #print item["score"], item["record"]["_id"]
             resultsScoreToRecordMap[item["score"]].add(str(item["record"]["id"]))
         except KeyError:
             resultsScoreToRecordMap[item["score"]] = set()
             resultsScoreToRecordMap[item["score"]].add(str(item["record"]["id"]))
                                                             
     return resultsScoreToRecordMap
 
def verify(jsonA, topk_A, jsonB, topk_B):
    resultsA = resultsScoreToRecordMap(jsonA)
    resultsB = resultsScoreToRecordMap(jsonB)

    counter = 0
    items_A = sorted(resultsA.iteritems())
    items_A.reverse()
    for key,value in items_A: # iter from top score to least score items
        if (resultsB[key] != value):
            if ((counter + len(value)) == int(topk_A) ) and (resultsB[key].issuperset(value)): # corner case where top k element has same score as top k+1 element
                continue
            else:
                return False
        counter = counter + len(value)
    return True
   
if __name__ == "__main__":
    #Start the engine server
    args = [ sys.argv[1], './top_k/conf-1.xml', './top_k/conf-2.xml', './top_k/conf-3.xml']

    serverHandle = test_lib.startServer(args)
    if serverHandle == None:
        os._exit(-1)

    #Load initial data
    dataFile = './top_k/data.json'
    test_lib.loadIntialData(dataFile)

    query = "obam"
    topk_A = str(10)
    topk_B = str(20)
    if len(sys.argv) == 5:
        query = sys.argv[2]
        if (sys.argv[3] < sys.argv[4]):
            topk_A = str(sys.argv[3])
            topk_B = str(sys.argv[4])
            exitCode = 0
        else:
            topk_B = str(sys.argv[3])
            topk_A = str(sys.argv[4])

        query_A = "fuzzy=1&start=0&q=%7BdefaultPrefixComplete=COMPLETE%7D"+query+ "&rows="+topk_A
        query_B = "fuzzy=1&start=0&q=%7BdefaultPrefixComplete=COMPLETE%7D"+query+ "&rows="+topk_B
        jsonTop10 = test_lib.searchRequest(query_A)
        jsonTop20 = test_lib.searchRequest(query_B)
        if ( verify(jsonTop10, topk_A,  jsonTop20, topk_B) ):
            print "Test passed."
            exitCode = 0
        else:
            print "Test failed."
            exitCode = 1
    else:
        print "Usage:"
        print "python test_srch2_top_k.py test+obam 10 20"
        print "python test_srch2_top_k.py ${query} ${topk_A} ${topk_B}"
        exitCode = 1


    print "======================================="
    test_lib.killServer(serverHandle)
    os._exit(exitCode)
