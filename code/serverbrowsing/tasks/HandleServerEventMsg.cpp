#include <tasks/tasks.h>
#include <server/SBServer.h>
#include <server/SBDriver.h>
namespace MM {

	//this event is handled by the "qr-service" project, which handles resending and other QR related logic
    bool Handle_ServerEventMsg(TaskThreadData *thread_data, std::string message) {
		SBServer *server = (SBServer *)uv_loop_get_data(uv_default_loop());
		if(!server) return false;
		
		std::vector<std::string> vec = OS::KeyStringToVector(message);
		if (vec.size() < 2) return false;
		std::string msg_type = vec.at(0);
		std::string server_key = vec.at(1);

		MM::Server *gameserver = GetServerByKey(thread_data, server_key, msg_type.compare("del") == 0);

		if(!gameserver) {
			return false;
		}
		
		if(msg_type.compare("del") == 0) {
			server->OnDeleteServer(*gameserver);
		} else if(msg_type.compare("new") == 0) {
			server->OnNewServer(*gameserver);
		} else if(msg_type.compare("update") == 0) {
			server->OnUpdateServer(*gameserver);
		}
		delete gameserver;
        return true;
    }
}