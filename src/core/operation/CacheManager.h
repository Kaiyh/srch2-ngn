
#ifndef __CACHEMANAGER_H__
#define __CACHEMANAGER_H__

#include <instantsearch/GlobalCache.h>
#include "operation/ActiveNode.h"
#include "query/QueryResultsInternal.h"
#include "CacheBase.h"
#include "PhysicalPlanRecordItemFactory.h"

#include <boost/shared_ptr.hpp>

using namespace std;

namespace srch2
{
namespace instantsearch
{

class PhysicalOperatorCacheObject;

/*
 * This cache module is used by physical plan operators
 * to set/get partially executed physical plans.
 */
class PhysicalOperatorsCache {
    public:
    PhysicalOperatorsCache(unsigned long byteSizeOfCache = 134217728){
        this->cacheContainer = new CacheContainer<PhysicalOperatorCacheObject>(byteSizeOfCache);
    }
    bool getPhysicalOperatorsInfo(string & key,  boost::shared_ptr<PhysicalOperatorCacheObject> & in);
    void setPhysicalOperatosInfo(string & key , boost::shared_ptr<PhysicalOperatorCacheObject> object);
    int clear();
    ~PhysicalOperatorsCache(){
        delete this->cacheContainer;
    }
private:
    CacheContainer<PhysicalOperatorCacheObject> * cacheContainer;
};


/*
 * This cache module is used to set/get prefixActiveNodeSet objects to incrementally
 * compute new ones.
 */
class ActiveNodesCache {
public:
    ActiveNodesCache(unsigned long byteSizeOfCache = 134217728){
        this->cacheContainer = new CacheContainer<PrefixActiveNodeSet>(byteSizeOfCache);
    }
    int findLongestPrefixActiveNodes(Term *term, boost::shared_ptr<PrefixActiveNodeSet> &in);
    int setPrefixActiveNodeSet(boost::shared_ptr<PrefixActiveNodeSet> &prefixActiveNodeSet);
    int clear();
    ~ActiveNodesCache(){
        delete cacheContainer;
    }
private:
    CacheContainer<PrefixActiveNodeSet> * cacheContainer;

};

/*
 * We have one cache module which is used if a query is repeated exactly the same.
 * This cache module is used for this purpose. Cache entries are the result records of a query.
 * these result record will be used if a query is repeated exactly the same.
 */
class QueryResultsCacheEntry{
public:
    QueryResultsCacheEntry(){
        factory = new QueryResultFactoryInternal();
    }
    ~QueryResultsCacheEntry(){
        delete factory ;
    }
    std::vector<QueryResult *> sortedFinalResults;
    bool resultsApproximated;
    long int estimatedNumberOfResults;
    std::map<std::string , std::pair< FacetType , std::vector<std::pair<std::string, float> > > > facetResults;
    QueryResultFactoryInternal * factory;
    void copyToQueryResultsInternal(QueryResultsInternal * destination){
        destination->resultsApproximated = resultsApproximated;
        destination->estimatedNumberOfResults = estimatedNumberOfResults;
        for(std::vector<QueryResult *>::iterator queryResult = sortedFinalResults.begin(); queryResult != sortedFinalResults.end() ; ++ queryResult){
            destination->sortedFinalResults.push_back(destination->getReultsFactory()->impl->createQueryResult(*(*queryResult)));
        }
        destination->facetResults = facetResults;
    }
    void copyFromQueryResultsInternal(QueryResultsInternal * destination){
        resultsApproximated = destination->resultsApproximated;
        estimatedNumberOfResults = destination->estimatedNumberOfResults;
        for(std::vector<QueryResult *>::iterator queryResult = destination->sortedFinalResults.begin();
                queryResult != destination->sortedFinalResults.end() ; ++ queryResult){
            sortedFinalResults.push_back(factory->createQueryResult(*(*queryResult)));
        }
        facetResults = destination->facetResults;
    }

    unsigned getNumberOfBytes() {
        unsigned result = sizeof(QueryResultsCacheEntry);   // This adds up size of all elements in this class
        for(std::map<std::string, std::pair< FacetType , std::vector<std::pair<std::string, float> > > >::iterator attr =
                facetResults.begin(); attr != facetResults.end(); ++attr){
            result += sizeof(attr->first) + attr->first.capacity();   // String Type
            result += sizeof(attr->second.first);                     // Enum Type
            result += sizeof(attr->second.second);                    // Vector Type
            result += attr->second.second.capacity() * sizeof(std::pair<std::string, float>);
            for(std::vector<std::pair<std::string, float> >::iterator f = attr->second.second.begin() ;
                    f != attr->second.second.end() ; ++f){
                result += f->first.capacity();
            }
            // We estimate the overhead of STL ordered map to be 32 bytes per node.
            result += 32;
        }
        result += sortedFinalResults.capacity() * sizeof(QueryResult *);
        for(std::vector<QueryResult *>::iterator q = sortedFinalResults.begin();
                q != sortedFinalResults.end() ; ++q){
            result += (*q)->getNumberOfBytes();
        }
        return result;
    }
};

class QueryResultsCache {
public:
    QueryResultsCache(unsigned long byteSizeOfCache = 134217728){
        this->cacheContainer = new CacheContainer<QueryResultsCacheEntry>(byteSizeOfCache);
    }

    bool getQueryResults(string & key,  boost::shared_ptr<QueryResultsCacheEntry> & in);
    void setQueryResults(string & key , boost::shared_ptr<QueryResultsCacheEntry> object);
    int clear();
    ~QueryResultsCache(){
        delete this->cacheContainer;
    }
private:
    CacheContainer<QueryResultsCacheEntry> * cacheContainer;
};


/*
 * This class is the cache manager. The CacheManager is the holder of different kinds of Cache, e.g.
 * ActiveNodesCache.
 */
class CacheManager : public GlobalCache
{
public:
    CacheManager(unsigned long byteSizeOfCache = 134217728){
        aCache = new ActiveNodesCache(byteSizeOfCache * 3.0/9); // we don't allocate cache budget equally
        qCache = new QueryResultsCache(byteSizeOfCache * 2.0/9);
        pCache = new PhysicalOperatorsCache(byteSizeOfCache * 4.0/9);
        physicalPlanRecordItemFactory = new PhysicalPlanRecordItemFactory();
    }
    virtual ~CacheManager(){
        delete aCache;
        delete qCache;
        delete pCache;
        delete physicalPlanRecordItemFactory;
    }

    int clear();
    ActiveNodesCache * getActiveNodesCache();
    QueryResultsCache * getQueryResultsCache();
    PhysicalOperatorsCache * getPhysicalOperatorsCache();
    PhysicalPlanRecordItemFactory * getPhysicalPlanRecordItemFactory();


private:
    ActiveNodesCache * aCache;

    QueryResultsCache * qCache;

    PhysicalOperatorsCache * pCache;

    PhysicalPlanRecordItemFactory * physicalPlanRecordItemFactory;

};



}
}

#endif /* __CACHEMANAGER_H__ */
