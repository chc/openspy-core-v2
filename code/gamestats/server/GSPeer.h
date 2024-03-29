#ifndef _GPPEER_H
#define _GPPEER_H
#include "../main.h"

#include <OS/Net/NetPeer.h>
#include <OS/Ref.h>

#include <OS/User.h>
#include <OS/Profile.h>

#include <OS/Buffer.h>

#include <OS/GPShared.h>

#include <server/tasks/tasks.h>

#include <OS/KVReader.h>

#define MAX_UNPROCESSED_DATA 500000

namespace GS {
	typedef struct {
		int profileid;
		int operation_id;
		uint32_t wait_index;
	} GPPersistRequestData;


	//probably should be moved into seperate lib
	class WaitBufferCtx {
		public:
			std::atomic<uint32_t> wait_index;
			uint32_t top_index;
			std::map<int, OS::Buffer> buffer_map;
	} ;

	class Driver;

	class Peer : public INetPeer {
	public:
		Peer(Driver *driver, uv_tcp_t *sd);
		~Peer();
		
		void think();
		void handle_packet(std::string packet_string);
		void Delete(bool timeout = false);
		void on_stream_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
		void AddRequest(PersistBackendRequest req);

		int GetProfileID();

		bool ShouldDelete() { return m_delete_flag; };
		bool IsTimeout() { return m_timeout_flag; }

		void send_ping();

		void send_login_challenge(int type);
		void SendPacket(std::string str, bool attach_final = true);
		void SendPacket(OS::Buffer &buffer, bool attach_final = true);

		OS::GameData GetGame() { return m_game; };

		void OnConnectionReady();
	private:
		//packet handlers
		static void newGameCreateCallback(bool success, PersistBackendResponse response_data, GS::Peer *peer, void* extra);
		void handle_newgame(OS::KVReader data_parser);

		static void updateGameCreateCallback(bool success, PersistBackendResponse response_data, GS::Peer *peer, void* extra);
		void handle_updgame(OS::KVReader data_parser);

		static void onGetGameDataCallback(bool success, PersistBackendResponse response_data, GS::Peer *peer, void* extra);
		void handle_authp(OS::KVReader data_parser);
		void handle_auth(OS::KVReader data_parser);
		void handle_getpid(OS::KVReader data_parser);

		static void getPersistDataCallback(bool success, PersistBackendResponse response_data, GS::Peer *peer, void* extra);
		void handle_getpd(OS::KVReader data_parser);

		static void setPersistDataCallback(bool success, PersistBackendResponse response_data, GS::Peer *peer, void* extra);
		void handle_setpd(OS::KVReader data_parser);

		//login
		void perform_cdkey_auth(std::string cdkey, std::string response, std::string nick, int operation_id);
		void perform_preauth_auth(std::string auth_token, const char *response, int operation_id);
		void perform_pid_auth(int profileid, const char *response, int operation_id);
		static void m_nick_email_auth_cb(bool success, OS::User user, OS::Profile profile, TaskShared::AuthData auth_data, void *extra, INetPeer *peer);
		static void m_getpid_cb(bool success, OS::User user, OS::Profile profile, TaskShared::AuthData auth_data, void *extra, INetPeer *peer);


		void SendOrWaitBuffer(uint32_t index, WaitBufferCtx &wait_ctx, OS::Buffer buffer);
		WaitBufferCtx m_getpd_wait_ctx; //getpd must respond in order of request, as "lid" value is not always used
		int m_get_request_index;
		WaitBufferCtx m_setpd_wait_ctx; //setpd must respond in order of request, as "lid" value is not always used
		int m_set_request_index;

		int m_getpid_request_index;
		WaitBufferCtx m_getpid_wait_ctx; //getpid must respond in order of request, as "lid" value is not always used


		//incase updgame calls are sent prior to the retrieval of the backend identify, save calls by client provided sesskey
		#define MAX_SESSKEY_WAIT 10
		std::map<int, std::vector<OS::KVReader> > m_updgame_sesskey_wait_list;
		int m_updgame_increfs;
		int m_last_authp_operation_id;
		std::vector<int> m_authenticated_profileids;


		void send_error(GPShared::GPErrorCode code);
		void gamespy3dxor(char *data, int len);
		int gs_chresp_num(const char *challenge);
		bool IsResponseValid(const char *response);

		OS::GameData m_game;
		
		uv_timespec64_t m_last_recv, m_last_ping;

		char m_challenge[CHALLENGE_LEN + 1];
		int m_session_key;
		int m_xor_index;


		//these are modified by other threads
		std::string m_backend_session_key;
		OS::User m_user;
		OS::Profile m_profile;

		uint16_t m_game_port;

		std::map<int, std::string> m_game_session_backend_identifier_map; //map used for lookup of backend identifier, seperate from client generated identifier, used for multiple game sessions at once
		std::string m_response;

		std::string m_kv_accumulator;
	};
}
#endif //_GPPEER_H