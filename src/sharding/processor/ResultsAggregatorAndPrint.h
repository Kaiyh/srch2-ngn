#ifndef __SHARDING_PROCESSOR_RESULTS_AGGREGATOR_AND_PRINT_H_
#define __SHARDING_PROCESSOR_RESULTS_AGGREGATOR_AND_PRINT_H_

#include "sharding/configuration/ConfigManager.h"
//#include "sharding/processor/ProcessorUtil.h"

namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {

struct ResultsAggregatorAndPrintMetadata{

};

template <class Request, class Response>
class ResultAggregatorAndPrint {
public:


	/*
	 * This function is always called by RoutingManager as the first call back function
	 */
	virtual void preProcessing(ResultsAggregatorAndPrintMetadata metadata){};
	/*
	 * This function is called by RoutingManager if a timeout happens, The call to
	 * this function must be between preProcessing(...) and callBack()
	 */
	virtual void timeoutProcessing(ShardId * shardInfo,
			Request * sentRequest, ResultsAggregatorAndPrintMetadata metadata){};

	/*
	 * The callBack function used by routing manager
	 */
	virtual void callBack(Response * responseObject){};
	virtual void callBack(vector<Response *> responseObjects){};

	/*
	 * The last call back function called by RoutingManager in all cases.
	 * Example of call back call order for search :
	 * 1. preProcessing()
	 * 2. timeoutProcessing() [only if some shard times out]
	 * 3. aggregateSearchResults()
	 * 4. finalize()
	 */
	virtual void finalize(ResultsAggregatorAndPrintMetadata metadata){};


	virtual ~ResultAggregatorAndPrint(){};

};

}
}


#endif // __SHARDING_PROCESSOR_RESULTS_AGGREGATOR_AND_PRINT_H_
