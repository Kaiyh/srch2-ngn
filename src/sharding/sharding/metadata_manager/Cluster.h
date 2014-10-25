#ifndef __SHARDING_CONFIGURATION_CLUSTER_H__
#define __SHARDING_CONFIGURATION_CLUSTER_H__

#include "Shard.h"
#include "Node.h"
#include "Partition.h"
#include "../../configuration/CoreInfo.h"
#include "../../configuration/ShardingConstants.h"

#include "wrapper/WrapperConstants.h"
#include "core/util/Assert.h"
#include "core/util/mypthread.h"
#include "core/util/Logger.h"

#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <string>
#include <vector>
#include <map>

using namespace std;
using namespace srch2::util;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {

class ClusterResourceMetadata_Readview{
public:
	ClusterResourceMetadata_Readview(unsigned versionId, string clusterName, vector<CoreInfo_t *> cores);
	ClusterResourceMetadata_Readview(const ClusterResourceMetadata_Readview & copy);
	~ClusterResourceMetadata_Readview();

	const CorePartitionContianer * getPartitioner(unsigned coreId) const;
	const LocalShardContainer * getLocalShardContainer(unsigned coreId) const;
	void getAllCores(vector<const CoreInfo_t *> & cores) const;
	const CoreInfo_t * getCore(unsigned coreId) const;
	const CoreInfo_t * getCoreByName(const string & coreName) const;
	void getAllNodes(vector<Node> & nodes) const;
	void getAllNodeIds(vector<NodeId> & nodeIds) const;
	unsigned getTotalNumberOfNodes() const;
	Node getNode(NodeId nodeId) const;
	unsigned getCurrentNodeId() const;
	string getClusterName() const;
	unsigned getVersionId() const;

	void print() const;

	void setCurrentNodeId(NodeId nodeId);
	CorePartitionContianer * getCorePartitionContianer(unsigned coreId);
	const CorePartitionContianer * getCorePartitionContianer(unsigned coreId) const;
	LocalShardContainer * getLocalShardContainer(unsigned coreId);
	void addNode(const Node & node);

private:

	unsigned versionId;

	string clusterName;

	// coreId => core *
	map<unsigned, CoreInfo_t *> allCores;
	// coreId => core partitions
	map<unsigned, CorePartitionContianer *> corePartitioners;
	// coreId => local core shards
	map<unsigned, LocalShardContainer *> localShardContainers;


	// nodeId => node
	map<NodeId, Node> allNodes;

	// this node id
	NodeId currentNodeId;
};




}
}

#endif // __SHARDING_CONFIGURATION_CLUSTER_H__