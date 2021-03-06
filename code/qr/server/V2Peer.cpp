#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include "QRServer.h"


#include "QRPeer.h"
#include "QRDriver.h"
#include "V2Peer.h"

namespace QR {
	V2Peer::V2Peer(Driver *driver, INetIOSocket *sd) : Peer(driver,sd,2) {
		m_recv_instance_key = false;
		m_resend_count = 0;

		m_sent_challenge = false;
		m_server_pushed = false;
		memset(&m_instance_key, 0, sizeof(m_instance_key));
		memset(&m_challenge, 0, sizeof(m_challenge));

		m_server_info.m_address = sd->address;

		gettimeofday(&m_last_ping, NULL);
		gettimeofday(&m_last_recv, NULL);
		gettimeofday(&m_last_msg_resend, NULL);

	}
	V2Peer::~V2Peer() {
	}

	void V2Peer::SendPacket(OS::Buffer &buffer) {
		NetIOCommResp resp = GetDriver()->getNetIOInterface()->datagramSend(m_sd, buffer);
		if (resp.disconnect_flag || resp.error_flag) {
			Delete();
		}
	}
	void V2Peer::handle_packet(INetIODatagram packet) {
		uint8_t type = packet.buffer.ReadByte();

		uint8_t instance_key[REQUEST_KEY_LEN];
		packet.buffer.ReadBuffer(&instance_key, REQUEST_KEY_LEN);

	
		if (!m_recv_instance_key && type != PACKET_AVAILABLE) {
			memcpy(&m_instance_key, &instance_key, REQUEST_KEY_LEN);
			m_recv_instance_key = true;
		}

		if(m_recv_instance_key) {
			if(memcmp((uint8_t *)&instance_key, (uint8_t *)&m_instance_key, sizeof(instance_key)) != 0) {
				OS::LogText(OS::ELogLevel_Info, "[%s] Instance key mismatch/possible spoofed packet, keys: %d %d", getAddress().ToString().c_str(), *(uint32_t *)&m_instance_key, *(uint32_t *)&instance_key);
				return;
			}
		}


		gettimeofday(&m_last_recv, NULL);

		switch(type) {
			case PACKET_AVAILABLE:
				handle_available(packet.buffer);
				break;
			case PACKET_HEARTBEAT:
				handle_heartbeat(packet.buffer);
			break;
			case PACKET_CHALLENGE:
				handle_challenge(packet.buffer);
			break;
			case PACKET_KEEPALIVE:
				handle_keepalive(packet.buffer);
			break;
			case PACKET_CLIENT_MESSAGE_ACK:
				handle_client_message_ack(packet.buffer);
			break;
		}
	}
	void V2Peer::handle_challenge(OS::Buffer &buffer) {
		char challenge_resp[90] = { 0 };

		if(m_server_info.m_game.secretkey[0] == 0) {
			send_error(true, "Unknown game");
			return;
		}
		gsseckey((unsigned char *)&challenge_resp, (unsigned char *)&m_challenge, (const unsigned char *)m_server_info.m_game.secretkey.c_str(), 0);
		if(strcmp(buffer.ReadNTS().c_str(),challenge_resp) == 0) { //matching challenge
			OS::LogText(OS::ELogLevel_Info, "[%s] Server pushed, gamename: %s", getAddress().ToString().c_str(), m_server_info.m_game.gamename.c_str());
			if(m_sent_challenge && !m_server_pushed) {
				TaskScheduler<MM::MMPushRequest, TaskThreadData> *scheduler = ((QR::Server *)(GetDriver()->getServer()))->getScheduler();
				MM::MMPushRequest req;
				req.peer = this;
				req.server = m_dirty_server_info;
				m_server_info = m_dirty_server_info;
				req.peer->IncRef();
				req.type = MM::EMMPushRequestType_PushServer;
				m_server_pushed = true;
				scheduler->AddRequest(req.type, req);
			}
			m_sent_challenge = true;
		}
		else {
			OS::LogText(OS::ELogLevel_Info, "[%s] Incorrect challenge for gamename: %s", getAddress().ToString().c_str(), m_server_info.m_game.gamename.c_str());
		}
	}
	void V2Peer::handle_keepalive(OS::Buffer &buffer) {
		OS::Buffer send_buffer;
		struct timeval current_time;
		send_buffer.WriteByte(QR_MAGIC_1);
		send_buffer.WriteByte(QR_MAGIC_2);
		send_buffer.WriteByte(PACKET_KEEPALIVE);
		send_buffer.WriteBuffer((uint8_t *)&m_instance_key, sizeof(m_instance_key));
		send_buffer.WriteInt(current_time.tv_sec);
		SendPacket(send_buffer);
	}
	void V2Peer::handle_heartbeat(OS::Buffer &buffer) {
		unsigned int i = 0;

		MM::ServerInfo server_info, old_server_info = m_server_info;
		server_info.m_game = m_server_info.m_game;
		server_info.m_address = m_server_info.m_address;
		server_info.id = m_server_info.id;
		server_info.groupid = m_server_info.groupid;

		std::string key, value;

		std::stringstream ss;

		while(true) {

			if(i%2 == 0) {
				key = buffer.ReadNTS();
				if (key.length() == 0) break;
			} else {
				value = buffer.ReadNTS();
				ss << "(" << key << "," << value << ") ";
			}

			if(value.length() > 0) {
				if(server_info.m_keys.find(key) == server_info.m_keys.end()) {
					server_info.m_keys[key] = value;
				}
				value = std::string();
			}
			i++;
		}

		OS::LogText(OS::ELogLevel_Info, "[%s] HB Keys: %s", getAddress().ToString().c_str(), ss.str().c_str());
		ss.str("");


		uint16_t num_values = 0;

		while((num_values = htons(buffer.ReadShort()))) {
			std::vector<std::string> nameValueList;
			if(buffer.readRemaining() <= 3) {
				break;
			}
			uint32_t num_keys = 0;
			std::string x;
			while(buffer.readRemaining() && (x = buffer.ReadNTS()).length() > 0) {
				nameValueList.push_back(x);
				num_keys++;
			}
			unsigned int player=0/*,num_keys_t = num_keys*/,num_values_t = num_values*num_keys;
			i = 0;

			while(num_values_t--) {
				std::string name = nameValueList.at(i);

				x = buffer.ReadNTS();

				if(isTeamString(name.c_str())) {
					if(server_info.m_team_keys[name].size() <= player) {
						server_info.m_team_keys[name].push_back(x);
					}
					else {
						server_info.m_team_keys[name][player] = x;
					}
					ss << "T(" << player << ") (" << name.c_str() << "," << x << ") ";
				} else {
					if(server_info.m_player_keys[name].size() <= player) {
						server_info.m_player_keys[name].push_back(x);
					} else {
						server_info.m_player_keys[name][player] = x;
					}
					ss << "P(" << player << ") (" << name.c_str() << "," << x << " ) ";
				}
				i++;
				if(i >= num_keys) {
					player++;
					i = 0;
				}
			}
		}


		OS::LogText(OS::ELogLevel_Info, "[%s] HB Keys: %s", getAddress().ToString().c_str(), ss.str().c_str());
		ss.str("");

		m_dirty_server_info = server_info;

		//register gamename
		MM::MMPushRequest req;
		TaskScheduler<MM::MMPushRequest, TaskThreadData> *scheduler = ((QR::Server *)(GetDriver()->getServer()))->getScheduler();
		req.peer = this;
		if (server_info.m_game.secretkey[0] != 0) {
			if (m_server_pushed) {
				if (server_info.m_keys.find("statechanged") != server_info.m_keys.end() && atoi(server_info.m_keys["statechanged"].c_str()) == 2) {
					Delete();
					return;
				}
				struct timeval current_time;
				gettimeofday(&current_time, NULL);
				if (current_time.tv_sec - m_last_heartbeat.tv_sec > HB_THROTTLE_TIME) {
					m_server_info_dirty = false;
					m_server_info = server_info;
					gettimeofday(&m_last_heartbeat, NULL);
					req.server = m_server_info;
					req.old_server = old_server_info;
					req.peer->IncRef();
					req.type = MM::EMMPushRequestType_UpdateServer;
					scheduler->AddRequest(req.type, req);
				} else {
					m_server_info_dirty = true;
				}
			}
			else {
				OnGetGameInfo(server_info.m_game, 1);
			}
		}
		else if(!m_sent_game_query){
			m_sent_game_query = true;
			req.peer->IncRef();
			req.state = 1;
			req.gamename = server_info.m_keys["gamename"];
			req.type = MM::EMMPushRequestType_GetGameInfoByGameName;
			scheduler->AddRequest(req.type, req);
		}
	}
	void V2Peer::handle_available(OS::Buffer &buffer) {
		TaskScheduler<MM::MMPushRequest, TaskThreadData> *scheduler = ((QR::Server *)(GetDriver()->getServer()))->getScheduler();
		MM::MMPushRequest req;
		req.peer = this;
		req.peer->IncRef();
		req.state = 2;

		req.gamename = buffer.ReadNTS();

		OS::LogText(OS::ELogLevel_Info, "[%s] Got available request: %s", getAddress().ToString().c_str(), req.gamename.c_str());
		req.type = MM::EMMPushRequestType_GetGameInfoByGameName;
		scheduler->AddRequest(req.type, req);
	}
	void V2Peer::OnGetGameInfo(OS::GameData game_info, int state) {
		if (state == 1) {
			m_server_info.m_game = game_info;
			m_dirty_server_info.m_game = game_info;
			if (m_server_info.m_game.secretkey[0] == 0) {
				send_error(true, "Game not found");
				return;
			}

			if (m_server_info.m_keys.find("statechanged") != m_server_info.m_keys.end() && atoi(m_server_info.m_keys["statechanged"].c_str()) == 2) {
				Delete();
				return;
			}

			if (!m_sent_challenge) {
				send_challenge();
			}

			//TODO: check if changed and only push changes
			if (m_server_pushed) {
				struct timeval current_time;
				gettimeofday(&current_time, NULL);
				if (current_time.tv_sec - m_last_heartbeat.tv_sec > HB_THROTTLE_TIME) {
					m_server_info_dirty = false;
					gettimeofday(&m_last_heartbeat, NULL);
					TaskScheduler<MM::MMPushRequest, TaskThreadData> *scheduler = ((QR::Server *)(GetDriver()->getServer()))->getScheduler();
					MM::MMPushRequest req;
					req.peer = this;
					m_dirty_server_info = m_server_info;
					req.server = m_server_info;
					req.peer->IncRef();
					req.type = MM::EMMPushRequestType_UpdateServer_NoDiff;
					scheduler->AddRequest(req.type, req);
				}
			}
		}
		else if(state == 2) {
			if (game_info.secretkey[0] == 0) {
				game_info.disabled_services = OS::QR2_GAME_AVAILABLE;
			}
			OS::Buffer buffer;
			buffer.WriteByte(QR_MAGIC_1);
			buffer.WriteByte(QR_MAGIC_2);

			buffer.WriteByte(PACKET_AVAILABLE);
			buffer.WriteInt(htonl(game_info.disabled_services));
			SendPacket(buffer);
			Delete();
		}
	}
	void V2Peer::handle_client_message_ack(OS::Buffer &buffer) {
		uint32_t key = buffer.ReadInt();
		std::map<uint32_t, OS::Buffer>::iterator it = m_client_message_queue.find(key);
		bool key_found = false;
		if(it != m_client_message_queue.end()) {
			m_client_message_queue.erase(key);
			key_found = true;
			m_resend_count = 0;
		}
		OS::LogText(OS::ELogLevel_Info, "[%s] Client Message ACK, key: %d, found: %d", getAddress().ToString().c_str(), key, key_found);
	}
	void V2Peer::send_error(bool die, const char *fmt, ...) {

		//XXX: make all these support const vars
		char vsbuff[256];
		va_list args;
		va_start(args, fmt);
		int len = vsnprintf(vsbuff, sizeof(vsbuff), fmt, args);
		vsbuff[len] = 0;
		va_end(args);

		OS::Buffer buffer;
		buffer.WriteByte(QR_MAGIC_1);
		buffer.WriteByte(QR_MAGIC_2);

		buffer.WriteByte(PACKET_ADDERROR);
		buffer.WriteBuffer((uint8_t *)&m_instance_key, sizeof(m_instance_key));
		buffer.WriteByte((char)die);
		buffer.WriteNTS(vsbuff);

		SendPacket(buffer);
		OS::LogText(OS::ELogLevel_Info, "[%s] Error:", getAddress().ToString().c_str(), vsbuff);
		if(die) {
			Delete();
		}
	}
	void V2Peer::send_ping() {
		//check for timeout
		struct timeval current_time;


		gettimeofday(&current_time, NULL);
		if(current_time.tv_sec - m_last_ping.tv_sec > QR2_PING_TIME) {
			OS::Buffer buffer;

			gettimeofday(&m_last_ping, NULL);

			buffer.WriteByte(QR_MAGIC_1);
			buffer.WriteByte(QR_MAGIC_2);

			buffer.WriteByte(PACKET_KEEPALIVE);
			buffer.WriteBuffer((uint8_t *)&m_instance_key, sizeof(m_instance_key));
			buffer.WriteInt(m_last_ping.tv_sec);
			SendPacket(buffer);
		}

	}
	void V2Peer::think(bool listener_waiting) {
		send_ping();
		SubmitDirtyServer();
		ResendMessages();

		//check for timeout
		struct timeval current_time;
		gettimeofday(&current_time, NULL);
		if(current_time.tv_sec - m_last_recv.tv_sec > QR2_PING_TIME) {
			Delete(true);			
		}
	}

	void V2Peer::send_challenge() {
		OS::Buffer buffer;

		memset(&m_challenge, 0, sizeof(m_challenge));
		OS::gen_random((char *)&m_challenge,sizeof(m_challenge)-1);

		uint8_t *backend_flags = (uint8_t *)&m_challenge[6];

		//force this part of the challenge to match sscanf pattern %02x, by setting the most significant bit		
		char hex_chars[] = "0123456789abcdef";
		if(m_server_info.m_game.backendflags & QR2_OPTION_USE_QUERY_CHALLENGE) {
			*backend_flags = hex_chars[(rand() % 8) + 8];
		} else if(!(m_server_info.m_game.backendflags & QR2_OPTION_USE_QUERY_CHALLENGE)) {
			*backend_flags = hex_chars[rand() % 8];
		}
		*(backend_flags+1) = hex_chars[rand() % (sizeof(hex_chars)-1)];

		buffer.WriteByte(QR_MAGIC_1);
		buffer.WriteByte(QR_MAGIC_2);

		buffer.WriteByte(PACKET_CHALLENGE);
		buffer.WriteBuffer((void *)&m_instance_key, sizeof(m_instance_key));
		buffer.WriteNTS(m_challenge);

		m_sent_challenge = true;

		SendPacket(buffer);
	}
	void V2Peer::SendClientMessage(void *data, size_t data_len) {
		OS::Buffer buffer(data_len);
		uint32_t key = rand() % 100000 + 1;

		buffer.WriteBuffer(data, data_len);
		SendClientMessage(buffer, key);
	}
	void V2Peer::SendClientMessage(OS::Buffer buffer, uint32_t key, bool no_insert) {
		OS::Buffer send_buff;
		send_buff.WriteByte(QR_MAGIC_1);
		send_buff.WriteByte(QR_MAGIC_2);

		send_buff.WriteByte(PACKET_CLIENT_MESSAGE);
		send_buff.WriteBuffer((void *)&m_instance_key, sizeof(m_instance_key));


		send_buff.WriteInt(key);
		send_buff.WriteBuffer(buffer.GetHead(), buffer.bytesWritten());

		mp_mutex->lock();
		if(!no_insert) {
			m_client_message_queue[key] = buffer;
		}
		gettimeofday(&m_last_msg_resend, NULL); //blocks resending of recent messages
		mp_mutex->unlock();

		OS::LogText(OS::ELogLevel_Info, "[%s] Recv client message: key: %d - len: %d, resend: %d", getAddress().ToString().c_str(), key, buffer.readRemaining(), no_insert);
		SendPacket(send_buff);
	}
	void V2Peer::OnRegisteredServer(int pk_id) {
		OS::Buffer buffer;
		m_server_info.id = pk_id;

		buffer.WriteByte(QR_MAGIC_1);
		buffer.WriteByte(QR_MAGIC_2);

		buffer.WriteByte(PACKET_CLIENT_REGISTERED);
		buffer.WriteBuffer((void *)&m_instance_key, sizeof(m_instance_key));
		SendPacket(buffer);		
	}
	void V2Peer::ResendMessages() {
		struct timeval current_time;
		gettimeofday(&current_time, NULL);

		if(current_time.tv_sec - m_last_msg_resend.tv_sec > QR2_RESEND_MSG_TIME) {
			gettimeofday(&m_last_msg_resend, NULL);
			if (m_client_message_queue.size() > 0) {
				if (m_resend_count > QR2_MAX_RESEND_COUNT) {
					Delete();
					return;
				}
				m_resend_count++;
			}
			std::map<uint32_t, OS::Buffer>::iterator it =  m_client_message_queue.begin();
			while(it != m_client_message_queue.end()) {
				std::pair<uint32_t, OS::Buffer> p = *it;
				SendClientMessage(p.second, p.first, true);
				it++;
			}
		}
		
	}
}
