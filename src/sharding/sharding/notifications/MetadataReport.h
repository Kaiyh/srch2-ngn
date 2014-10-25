#ifndef __SHARDING_SHARDING_METADATA_REPORT_H__
#define __SHARDING_SHARDING_METADATA_REPORT_H__

#include "Notification.h"
#include "../metadata_manager/Cluster_Writeview.h"
#include "core/util/SerializationHelper.h"
#include "sharding/transport/Message.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

// This notification is used to read the metadata of a node
class MetadataReport: public ShardingNotification{
public:
	MetadataReport(Cluster_Writeview * writeview){
		this->writeview = writeview;
	}
	MetadataReport(){};

	bool resolveNotification(SP(ShardingNotification) _notif);

	ShardingMessageType messageType() const;
	void * serializeBody(void * buffer) const;
	unsigned getNumberOfBytesBody() const;
	void * deserializeBody(void * buffer) ;
    Cluster_Writeview * getWriteview() const;

    bool operator==(const MetadataReport & report);

private:
    // must not be deleted in this class.
    Cluster_Writeview * writeview;

public:
    ///////////// Sub classes :
    class REQUEST : public ShardingNotification{
    public:
        ShardingMessageType messageType() const;
		bool resolveNotification(SP(ShardingNotification) _notif);
    };
};

}
}


#endif // __SHARDING_SHARDING_METADATA_REPORT_H__