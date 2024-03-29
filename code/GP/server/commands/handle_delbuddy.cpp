#include <OS/OpenSpy.h>

#include <OS/Buffer.h>
#include <OS/KVReader.h>
#include <sstream>
#include <algorithm>

#include <OS/gamespy/gamespy.h>
#include <tasks/tasks.h>

#include <GP/server/GPPeer.h>
#include <GP/server/GPDriver.h>
#include <GP/server/GPServer.h>

namespace GP {
	void Peer::handle_delbuddy(OS::KVReader data_parser) {
		if (data_parser.HasKey("delprofileid")) {
			int delprofileid = data_parser.GetValueInt("delprofileid");
			if (m_buddies.find(delprofileid) != m_buddies.end()) {
				GPBackendRedisRequest req;
				req.type = EGPRedisRequestType_DelBuddy;
				req.peer = (GP::Peer *)this;
				req.peer->IncRef();
				req.ToFromData.from_profileid = m_profile.id;
				req.ToFromData.to_profileid = delprofileid;
				m_buddies.erase(delprofileid);
				AddGPTaskRequest(req);
			}
			else {
				send_error(GPShared::GP_DELBUDDY_NOT_BUDDY);
			}
		}
		else {
			send_error(GPShared::GP_PARSE);
			return;
		}
	}

}