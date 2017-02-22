#include <OS/OpenSpy.h>
#include <OS/Net/NetDriver.h>
#include <OS/Net/NetServer.h>
#include <OS/legacy/buffreader.h>
#include <OS/legacy/buffwriter.h>
#include <OS/socketlib/socketlib.h>
#include <OS/legacy/enctypex_decoder.h>
#include <OS/legacy/gsmsalg.h>
#include <OS/legacy/enctype_shared.h>
#include <OS/legacy/helpers.h>


#include <sstream>
#include "ChatDriver.h"
#include "ChatServer.h"
#include "ChatPeer.h"
#include "IRCPeer.h"
#include "ChatBackend.h"


#include <sstream>
#include <vector>
#include <algorithm>
#include <iomanip>

#include <OS/legacy/buffreader.h>
#include <OS/legacy/buffwriter.h>


namespace Chat {
		void IRCPeer::OnNickCmd_InUseLookup(const struct Chat::_ChatQueryRequest request, const struct Chat::_ChatQueryResponse response, Peer *peer,void *extra) {
			Chat::Driver *driver = (Chat::Driver *)extra;
			IRCPeer *irc_peer = (IRCPeer *)peer;


			if(!driver->HasPeer(peer)) {
				return;
			}

			irc_peer->mp_mutex->lock();

			if(response.client_info.name.size() > 0) {
				irc_peer->send_numeric(433, "Nickname is already in use");
			} else {
				irc_peer->m_client_info.name = request.query_name;
				ChatBackendTask::getQueryTask()->flagPushTask();
				ChatBackendTask::SubmitClientInfo(OnNickCmd_SubmitClientInfo, peer, driver);
			}
			irc_peer->mp_mutex->unlock();
		}
		void IRCPeer::OnNickCmd_SubmitClientInfo(const struct Chat::_ChatQueryRequest request, const struct Chat::_ChatQueryResponse response, Peer *peer,void *extra) {
			Chat::Driver *driver = (Chat::Driver *)extra;
			IRCPeer *irc_peer = (IRCPeer *)peer;


			if(!driver->HasPeer(peer)) {
				return;
			}

			irc_peer->mp_mutex->lock();
			irc_peer->m_client_info = response.client_info;

			irc_peer->send_client_init();

			std::ostringstream s;
			s << "Your client id is " << irc_peer->m_client_info.client_id;
			irc_peer->send_numeric(401, s.str());

			irc_peer->mp_mutex->unlock();
		}
		EIRCCommandHandlerRet IRCPeer::handle_nick(std::vector<std::string> params, std::string full_params) {
			std::string nick;
			if(params.size() >= 1) {
				nick = params[1];
			} else {
				return EIRCCommandHandlerRet_NotEnoughParams;
			}
			if(nick.size() && nick[0] == ':') {
				nick = nick.substr(1, nick.length()-1);
			}
			nick = OS::strip_whitespace(nick);

			ChatBackendTask::SubmitGetClientInfoByName(nick, OnNickCmd_InUseLookup, (Peer *)this, mp_driver);

			return EIRCCommandHandlerRet_NoError;
		}
		EIRCCommandHandlerRet IRCPeer::handle_user(std::vector<std::string> params, std::string full_params) {
			if(params.size() < 5) {
				return EIRCCommandHandlerRet_NotEnoughParams;
			}
			m_client_info.user = params[1];
			m_client_info.realname = params[4];
			m_client_info.realname = m_client_info.realname.substr(1, m_client_info.realname.length()-1);

			if(m_client_info.name.size())
				send_client_init();

			return EIRCCommandHandlerRet_NoError;
		}
		EIRCCommandHandlerRet IRCPeer::handle_ping(std::vector<std::string> params, std::string full_params) {
			std::ostringstream s;
			s << "PONG :" << ((ChatServer*)mp_driver->getServer())->getName() << std::endl;
			SendPacket((const uint8_t*)s.str().c_str(),s.str().length());
			return EIRCCommandHandlerRet_NoError;	
		}
		EIRCCommandHandlerRet IRCPeer::handle_pong(std::vector<std::string> params, std::string full_params) {
			return EIRCCommandHandlerRet_NoError;
		}
		void IRCPeer::OnPrivMsg_Lookup(const struct Chat::_ChatQueryRequest request, const struct Chat::_ChatQueryResponse response, Peer *peer,void *extra) {
			IRCMessageCallbackData *cb_data = (IRCMessageCallbackData *)extra;
			Chat::Driver *driver = (Chat::Driver *)cb_data->driver;
			IRCPeer *irc_peer = (IRCPeer *)peer;

			if(!driver->HasPeer(peer)) {
				delete cb_data;
				return;
			}
			if(response.error != EChatBackendResponseError_NoError) {
				irc_peer->send_callback_error(request, response);
				delete cb_data;
				return;
			}
			std::ostringstream s;
			if(response.client_info.client_id != 0) {
				ChatBackendTask::getQueryTask()->flagPushTask();
				ChatBackendTask::SubmitClientMessage(response.client_info.client_id, cb_data->message, cb_data->message_type, NULL, peer, driver);
			} else if(response.channel_info.channel_id != 0) {
				ChatBackendTask::getQueryTask()->flagPushTask();
				ChatBackendTask::SubmitChannelMessage(response.channel_info.channel_id, cb_data->message, cb_data->message_type, NULL, peer, driver);
			}
			delete cb_data;
		}
		EIRCCommandHandlerRet IRCPeer::handle_privmsg(std::vector<std::string> params, std::string full_params) {
			std::string target;
			EChatMessageType type;
			if(params.size() > 2) {
				target = params[1];
			} else {
				return EIRCCommandHandlerRet_NotEnoughParams;
			}
			const char *str = full_params.c_str();
			const char *beg = strchr(str, ':');
			if(beg) {
				beg++;
			} else {
				return EIRCCommandHandlerRet_NotEnoughParams;
			}

			IRCMessageCallbackData *cb_data = new IRCMessageCallbackData;

			if(strcasecmp(params[0].c_str(),"PRIVMSG") == 0) {
				type = EChatMessageType_Msg;
			} else if(strcasecmp(params[0].c_str(),"NOTICE") == 0) {
				type = EChatMessageType_Notice;
			} else if(strcasecmp(params[0].c_str(),"UTM") == 0) {
				type = EChatMessageType_UTM;
			} else if(strcasecmp(params[0].c_str(),"ATM") == 0) {
				type = EChatMessageType_ATM;
			}
			cb_data->message = beg;
			cb_data->driver = mp_driver;
			cb_data->message_type = type;

			if(is_channel_name(target)) {
				ChatBackendTask::SubmitFindChannel(OnPrivMsg_Lookup, (Peer *)this, cb_data, target);
			} else {
				ChatBackendTask::SubmitGetClientInfoByName(target, OnPrivMsg_Lookup, (Peer *)this, cb_data);
				
			}			
			return EIRCCommandHandlerRet_NoError;
		}
		void IRCPeer::OnWhoisCmd_UserLookup(const struct Chat::_ChatQueryRequest request, const struct Chat::_ChatQueryResponse response, Peer *peer,void *extra) {
			Chat::Driver *driver = (Chat::Driver *)extra;
			IRCPeer *irc_peer = (IRCPeer *)peer;

			if(!driver->HasPeer(peer)) {
				return;
			}
			std::ostringstream s;
			if(response.error != EChatBackendResponseError_NoError) {
				irc_peer->send_callback_error(request, response);
				return;
			}
			if(response.client_info.name.size() > 0) {
				s.str("");
				s << response.client_info.name << " " << response.client_info.user  << " " << response.client_info.hostname << " * :" << response.client_info.realname << std::endl;
				irc_peer->send_numeric(311, s.str(), true);
			} else {
				//
				irc_peer->send_nonick_channel_error(request.query_name);
			}

			s.str("");
			s << request.query_name << " :End of WHOIS list";
			irc_peer->send_numeric(318, s.str(), true);
		}
		EIRCCommandHandlerRet IRCPeer::handle_whois(std::vector<std::string> params, std::string full_params) {
			if(params.size() < 2) {
				return EIRCCommandHandlerRet_NotEnoughParams;
			}
			ChatBackendTask::SubmitGetClientInfoByName(params[1], OnWhoisCmd_UserLookup, (Peer *)this, mp_driver);
			return EIRCCommandHandlerRet_NoError;		
		}
		void IRCPeer::OnUserhostCmd_UserLookup(const struct Chat::_ChatQueryRequest request, const struct Chat::_ChatQueryResponse response, Peer *peer,void *extra) {
			Chat::Driver *driver = (Chat::Driver *)extra;
			IRCPeer *irc_peer = (IRCPeer *)peer;

			if(!driver->HasPeer(peer)) {
				return;
			}
			if(response.error != EChatBackendResponseError_NoError) {
				irc_peer->send_callback_error(request, response);
				return;
			}
			if(response.client_info.name.size() > 0) {
				std::ostringstream s;
				s << response.client_info.name << "=+" << response.client_info.user << "@" << response.client_info.hostname << std::endl;
				irc_peer->send_numeric(302, s.str());
			} else {
				//
				irc_peer->send_nonick_channel_error(request.query_name);
			}

		}
		EIRCCommandHandlerRet IRCPeer::handle_userhost(std::vector<std::string> params, std::string full_params) {
			if(params.size() < 2) {
				return EIRCCommandHandlerRet_NotEnoughParams;
			}
			std::string search = m_client_info.name;
			if(params.size() > 1) {
				search = params[1];
			}

			ChatBackendTask::SubmitGetClientInfoByName(search, OnUserhostCmd_UserLookup, (Peer *)this, mp_driver);
			return EIRCCommandHandlerRet_NoError;
		}

		EIRCCommandHandlerRet IRCPeer::handle_setkey(std::vector<std::string> params, std::string full_params) {
			std::string target, identifier, set_data_str;
			if(params.size() < 5) {
				return EIRCCommandHandlerRet_NotEnoughParams;
			}

			const char *str = full_params.c_str();
			const char *beg = strchr(str, ':');
			if(beg) {
				beg++;
			} else {
				return EIRCCommandHandlerRet_NotEnoughParams;
			}

			ChatBackendTask::SubmitSetClientKeys(NULL, this, mp_driver, m_client_info.client_id, OS::KeyStringToMap(beg));
			//SubmitSetChannelKeys

			return EIRCCommandHandlerRet_NoError;
		}
		/*
			-> s GETKEY Krad 0 000 :\gameid
			<- :s 700 CHC Krad 0 :\0
		*/
		void IRCPeer::OnGetKey_UserLookup(const struct Chat::_ChatQueryRequest request, const struct Chat::_ChatQueryResponse response, Peer *peer,void *extra) {
			GetCKeyData *cb_data = (GetCKeyData *)extra;
			IRCPeer *irc_peer = (IRCPeer *)peer;
			Chat::Driver *driver = (Chat::Driver *)cb_data->driver;

			if(!driver->HasPeer(peer)) {
				delete cb_data->search_data;
				delete cb_data->target_user;
				free((void *)cb_data);
				return;
			}
			std::ostringstream s;
			std::ostringstream result_oss;
			if(response.error != EChatBackendResponseError_NoError) {
				irc_peer->send_callback_error(request, response);
				delete cb_data->search_data;
				delete cb_data->target_user;
				free((void *)cb_data);
				return;
			}

			std::map<std::string, std::string>::const_iterator it;
			char *search_data_cpy = strdup(cb_data->search_data->c_str());
			char key_name[256];
			int i = 0;
			while(find_param(i++, search_data_cpy, key_name, sizeof(key_name))) {
					it = response.client_info.custom_keys.find(key_name);
					result_oss << "\\";
					//TODO - special attributes: gameid, user, nick, realname, profileid, userid
					if(it != response.client_info.custom_keys.end()) {
						result_oss << (*it).second;
					}
			}
			s << irc_peer->m_client_info.name  << " " << response.client_info.name << " " << cb_data->response_identifier << " :" << result_oss.str();
			irc_peer->send_numeric(700, s.str(), true);

			free((void *)search_data_cpy);
			delete cb_data->search_data;
			delete cb_data->target_user;
			free((void *)cb_data);
		}
		EIRCCommandHandlerRet IRCPeer::handle_getkey(std::vector<std::string> params, std::string full_params) {
			std::string target, identifier, set_data_str;
			GetCKeyData *cb_data;
			if(params.size() < 5) {
				return EIRCCommandHandlerRet_NotEnoughParams;
			}

			const char *str = full_params.c_str();
			const char *beg = strchr(str, ':');
			if(beg) {
				beg++;
			} else {
				return EIRCCommandHandlerRet_NotEnoughParams;
			}
			cb_data = (GetCKeyData *)malloc(sizeof(GetCKeyData));
			cb_data->driver = mp_driver;
			cb_data->search_data = new std::string(OS::strip_whitespace(beg));
			cb_data->target_user = new std::string(params[1]);
			cb_data->response_identifier = atoi(params[3].c_str());

			ChatBackendTask::SubmitGetClientInfoByName(params[1], OnGetKey_UserLookup, (Peer *)this, cb_data);

			return EIRCCommandHandlerRet_NoError;
		}
}