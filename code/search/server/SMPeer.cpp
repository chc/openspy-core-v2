#include <OS/OpenSpy.h>
#include <OS/Buffer.h>

#include <OS/gamespy/gamespy.h>
#include <OS/gamespy/gsmsalg.h>

#include <sstream>

#include <OS/Task/TaskScheduler.h>
#include <OS/SharedTasks/tasks.h>
#include <OS/GPShared.h>

#include "SMPeer.h"
#include "SMDriver.h"
#include "SMServer.h"

namespace SM {
	const char *Peer::mp_hidden_str = "[hidden]";
	Peer::Peer(Driver *driver, INetIOSocket *sd) : INetPeer(driver, sd) {
		m_delete_flag = false;
		m_timeout_flag = false;
		mp_mutex = OS::CreateMutex();
		gettimeofday(&m_last_ping, NULL);
		gettimeofday(&m_last_recv, NULL);
		
	}
	void Peer::OnConnectionReady() {
		OS::LogText(OS::ELogLevel_Info, "[%s] New connection", getAddress().ToString().c_str());
	}
	Peer::~Peer() {
		OS::LogText(OS::ELogLevel_Info, "[%s] Connection closed, timeout: %d", getAddress().ToString().c_str(), m_timeout_flag);
		delete mp_mutex;
	}
	void Peer::think(bool packet_waiting) {
		NetIOCommResp io_resp;
		if (m_delete_flag) return;

		if (packet_waiting) {
			OS::Buffer recv_buffer;
			io_resp = this->GetDriver()->getNetIOInterface()->streamRecv(m_sd, recv_buffer);

			int len = io_resp.comm_len;

			if (len <= 0) {
				goto end;
			}

			/*
			This scans the incoming packets for \\final\\ and splits based on that,


			as well as handles incomplete packets -- due to TCP preserving data order, this is possible, and cannot be used on UDP protocols
			*/
			std::string recv_buf = m_kv_accumulator;
			m_kv_accumulator.clear();
			recv_buf.append((const char *)recv_buffer.GetHead(), len);

			size_t final_pos = 0, last_pos = 0;

			do {
				final_pos = recv_buf.find("\\final\\", last_pos);
				if (final_pos == std::string::npos) break;

				std::string partial_string = recv_buf.substr(last_pos, final_pos - last_pos);
				handle_packet(partial_string);

				last_pos = final_pos + 7; // 7 = strlen of \\final
			} while (final_pos != std::string::npos);


			//check for extra data that didn't have the final string -- incase of incomplete data
			if (last_pos < (size_t)len) {
				std::string remaining_str = recv_buf.substr(last_pos);
				m_kv_accumulator.append(remaining_str);
			}

			if (m_kv_accumulator.length() > MAX_UNPROCESSED_DATA) {
				Delete();
				return;
			}
		}

	end:
		//send_ping();

		//check for timeout
		struct timeval current_time;
		gettimeofday(&current_time, NULL);
		if (current_time.tv_sec - m_last_recv.tv_sec > SM_PING_TIME * 2) {
			Delete(true);
		} else if ((io_resp.disconnect_flag || io_resp.error_flag) && packet_waiting) {
			Delete();
		}
	}
	void Peer::handle_packet(std::string data) {
		OS::LogText(OS::ELogLevel_Debug, "[%s] Recv: %s\n", getAddress().ToString().c_str(), data.c_str());

		OS::KVReader data_parser = OS::KVReader(data);
		std::string command = data_parser.GetKeyByIdx(0);
		if(!command.compare("search")) {
			handle_search(data_parser);
		} else if(!command.compare("others")) {
			handle_others(data_parser);
		} else if(!command.compare("otherslist")) {
			handle_otherslist(data_parser);
		} else if(!command.compare("valid")) {
			handle_valid(data_parser);
		} else if(!command.compare("nicks")) {
			handle_nicks(data_parser);
		} else if(!command.compare("pmatch")) {

		} else if(!command.compare("check")) {
			handle_check(data_parser);
		} else if(!command.compare("newuser")) {
			handle_newuser(data_parser);
		} else if( !command.compare("searchunique")) {
			handle_searchunique(data_parser);
		} else if(!command.compare("profilelist")) {
			
		} else if(!command.compare("uniquesearch")) {
			handle_uniquesearch(data_parser);
		}
		
		gettimeofday(&m_last_recv, NULL);
	}

	void Peer::send_error(GPShared::GPErrorCode code, std::string addon_data) {
		GPShared::GPErrorData error_data = GPShared::getErrorDataByCode(code);
		if (error_data.msg == NULL) {
			Delete();
			return;
		}
		std::ostringstream ss;
		ss << "\\error\\";
		ss << "\\err\\" << error_data.error;
		if (error_data.die) {
			ss << "\\fatal\\";
		}
		ss << "\\errmsg\\" << error_data.msg;
		if (addon_data.length())
			ss << addon_data;
		
		SendPacket(ss.str().c_str());
		if (error_data.die) {
			Delete();
		}
	}
	void Peer::SendPacket(std::string string, bool attach_final) {
		OS::Buffer buffer;
		//buffer.Write
		OS::LogText(OS::ELogLevel_Debug, "[%s] Send: %s\n", getAddress().ToString().c_str(), string.c_str());
		buffer.WriteBuffer((void *)string.c_str(), string.length());
		if (attach_final) {
			buffer.WriteBuffer((void *)"\\final\\", 7);
		}
		NetIOCommResp io_resp;
		io_resp = this->GetDriver()->getNetIOInterface()->streamSend(m_sd, buffer);
		if (io_resp.disconnect_flag || io_resp.error_flag) {
			Delete();
		}
	}
	void Peer::Delete(bool timeout) {
		m_timeout_flag = timeout;
		m_delete_flag = true;
	}
}