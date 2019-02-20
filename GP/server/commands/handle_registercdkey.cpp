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
	void Peer::m_update_cdkey_callback(TaskShared::WebErrorDetails error_details, std::vector<OS::Profile> results, std::map<int, OS::User> result_users, void *extra, INetPeer *peer) {
		GP::Peer *gppeer = (GP::Peer *)peer;
		switch (error_details.response_code) {
			case TaskShared::WebErrorCode_CdKeyAlreadyTaken:
				gppeer->send_error(GPShared::GP_REGISTERCDKEY_ALREADY_TAKEN);
				break;
			case TaskShared::WebErrorCode_BadCdKey:
				gppeer->send_error(GPShared::GP_REGISTERCDKEY_BAD_KEY);
				break;
		}
		if (error_details.response_code != TaskShared::WebErrorCode_Success) {
			return;
		}
		std::ostringstream s;
		s << "\\rc\\1";

		gppeer->SendPacket((const uint8_t *)s.str().c_str(), s.str().length());
	}
	void Peer::handle_registercdkey(OS::KVReader data_parser) {
        std::string cdkey;
		int id = data_parser.GetValueInt("id");
        if(m_profile.cdkey.length() > 0) {
            send_error(GPShared::GP_REGISTERCDKEY_ALREADY_SET);
            return;
        }
        if(data_parser.HasKey("cdkey")) {
            cdkey = data_parser.GetValue("cdkey");
        }

		TaskShared::ProfileRequest request;
		request.profile_search_details = m_profile;
        request.profile_search_details.cdkey = cdkey;
		request.extra = (void *)id;
		request.peer = this;
		request.peer->IncRef();
		request.type = TaskShared::EProfileSearch_UpdateProfile;
		request.callback = Peer::m_update_cdkey_callback;
		TaskScheduler<TaskShared::ProfileRequest, TaskThreadData> *scheduler = ((GP::Server *)(GetDriver()->getServer()))->GetProfileTask();
		scheduler->AddRequest(request.type, request);
	}
}