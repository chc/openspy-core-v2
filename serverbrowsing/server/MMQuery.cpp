#include "main.h"

#include "server/SBDriver.h"
#include "MMQuery.h"

#include <OS/socketlib/socketlib.h>

#include <OS/legacy/helpers.h>

#include <OS/Thread.h>

#include <sstream>
#include <algorithm>

#include <serverbrowsing/filter/filter.h>

namespace MM {


	const char *sb_mm_channel = "serverbrowsing.servers";
	MMQueryTask *MMQueryTask::m_task_singleton = NULL;
	void MMQueryTask::onRedisMessage(Redis::Connection *c, Redis::Response reply, void *privdata) {
		MMQueryTask *task = (MMQueryTask *)privdata;
		Redis::Value v = reply.values.front();

	    Server *server = NULL;

	    char msg_type[16], server_key[64];
	    if (v.type == Redis::REDIS_RESPONSE_TYPE_ARRAY) {
	    	if(v.arr_value.values.size() == 3 && v.arr_value.values[2].first == Redis::REDIS_RESPONSE_TYPE_STRING) {
	    		if(strcmp(v.arr_value.values[2].second.value._str.c_str(),sb_mm_channel) == 0) {
					char *temp_str = strdup(v.arr_value.values[2].second.value._str.c_str());
	    			find_param(0, temp_str,(char *)&msg_type, sizeof(msg_type));
	    			find_param(1, temp_str, (char *)&server_key, sizeof(server_key));
					free((void *)temp_str);

	    			server = task->GetServerByKey(server_key, task->mp_redis_async_retrival_connection, strcmp(msg_type,"del") == 0);
	    			if(!server) return;

	    			std::vector<SB::Driver *>::iterator it = task->m_drivers.begin();
	    			while(it != task->m_drivers.end()) {
	    				SB::Driver *driver = *it;
			   			if(strcmp(msg_type,"del") == 0) {
		    				driver->SendDeleteServer(server);
		    			} else if(strcmp(msg_type,"new") == 0) {
		    				driver->SendNewServer(server);
		    			} else if(strcmp(msg_type,"update") == 0) {
		    				driver->SendUpdateServer(server);
		    			}
	    				it++;
	    			}
	    		}
	    	}
	    }
	}

	void *MMQueryTask::setup_redis_async(OS::CThread *thread) {
		MMQueryTask *task = (MMQueryTask *)thread->getParams();

		struct timeval t;
		t.tv_usec = 0;
		t.tv_sec = task->m_redis_timeout;
		task->mp_redis_async_connection = Redis::Connect(OS_REDIS_ADDR, t);

		Redis::LoopingCommand(task->mp_redis_async_connection, task->m_redis_timeout, MMQueryTask::onRedisMessage, thread->getParams(), "SUBSCRIBE %s", sb_mm_channel);
		return NULL;
	}
	void MMQueryTask::AddDriver(SB::Driver *driver) {
		m_drivers.push_back(driver);
	}
	void MMQueryTask::RemoveDriver(SB::Driver *driver) {

	}
	MMQueryTask *MMQueryTask::getQueryTask() {
		if(!MMQueryTask::m_task_singleton) {
			MMQueryTask::m_task_singleton = new MMQueryTask();
		}
		return MMQueryTask::m_task_singleton;
	}
	MMQueryTask::MMQueryTask() {
		m_redis_timeout = 5;

		struct timeval t;
		t.tv_usec = 0;
		t.tv_sec = m_redis_timeout;

		mp_redis_connection = Redis::Connect(OS_REDIS_ADDR, t);
		mp_redis_async_retrival_connection = Redis::Connect(OS_REDIS_ADDR, t);

		mp_async_thread = OS::CreateThread(setup_redis_async, this, true);

		mp_mutex = OS::CreateMutex();
		mp_thread = OS::CreateThread(MMQueryTask::TaskThread, this, true);

	}

	MMQueryTask::~MMQueryTask() {
		delete mp_thread;
		delete mp_async_thread;
		delete mp_mutex;

		Redis::Disconnect(mp_redis_async_retrival_connection);
		Redis::Disconnect(mp_redis_connection);

	}

	void MMQueryTask::Shutdown() {
		MMQueryTask *query_task = getQueryTask();


		delete query_task;
	}

	//////////////////////////////////////////////////
	/// Async MM Query code
	void MMQueryTask::AppendServerEntry(std::string entry_name, ServerListQuery *ret, bool all_keys, bool include_deleted, Redis::Connection *redis_ctx, const sServerListReq *req) {
		Redis::Response reply;
		int cursor = 0;
		int idx = 0;
		uint8_t last_type = Redis::REDIS_RESPONSE_TYPE_NULL;
		std::string key;
		std::ostringstream s;

		Redis::Value v, arr;

		std::map<std::string, std::string> all_cust_keys; //used for filtering

		if(!redis_ctx) {
			redis_ctx = mp_redis_connection;
		}

		/*
		XXX: add redis error checks, cleanup on error, etc
		*/


		//freeReplyObject(redisCommand(redis_ctx, "SELECT %d", OS::ERedisDB_QR));
		Redis::Command(redis_ctx, 0, "SELECT %d", OS::ERedisDB_QR);

		std::vector<std::string>::iterator it = ret->requested_fields.begin();

		/*
		reply = (redisReply *)redisCommand(redis_ctx, "HGET %s deleted", entry_name.c_str());
		if(reply) {
			if(reply->type != REDIS_REPLY_NIL && !include_deleted) {
				freeReplyObject(reply);
				return;
			}
			freeReplyObject(reply);
			reply = (redisReply *)NULL;
		} else {
			return;
		}
		*/


		Server *server = new MM::Server();

		reply = Redis::Command(redis_ctx, 0, "HGET %s gameid", entry_name.c_str());

		if (reply.values.size() == 0 || reply.values.front().type == Redis::REDIS_RESPONSE_TYPE_ERROR)
			goto error_cleanup;

		v = reply.values.front();
		if(v.type == Redis::REDIS_RESPONSE_TYPE_STRING) {
			std::string gameid_reply = OS::strip_quotes(v.value._str).c_str();
			int gameid = atoi(gameid_reply.c_str());
			server->game = OS::GetGameByID(gameid, redis_ctx);
			Redis::Command(redis_ctx, 0, "SELECT %d", OS::ERedisDB_QR);
		}

		server->key = entry_name;


		reply = Redis::Command(redis_ctx, 0, "HGET %s id", entry_name.c_str());
		if (reply.values.size() == 0 || reply.values.front().type == Redis::REDIS_RESPONSE_TYPE_ERROR)
			goto error_cleanup;

		v = reply.values.front();

		if (v.type == Redis::REDIS_RESPONSE_TYPE_STRING)
			server->id = atoi(OS::strip_quotes(v.value._str).c_str());

		reply = Redis::Command(redis_ctx, 0, "HGET %s wan_port", entry_name.c_str());

		if (reply.values.size() == 0 || reply.values.front().type == Redis::REDIS_RESPONSE_TYPE_ERROR)
			goto error_cleanup;

		v = reply.values.front();

		if(v.type== Redis::REDIS_RESPONSE_TYPE_STRING)
			server->wan_address.port = atoi(OS::strip_quotes(v.value._str).c_str());

		reply = Redis::Command(redis_ctx, 0, "HGET %s wan_ip", entry_name.c_str());

		if (reply.values.size() == 0 || reply.values.front().type == Redis::REDIS_RESPONSE_TYPE_ERROR)
			goto error_cleanup;

		v = reply.values.front();

		if(v.type == Redis::REDIS_RESPONSE_TYPE_STRING)
			server->wan_address.ip = Socket::htonl(Socket::inet_addr(OS::strip_quotes(v.value._str).c_str()));


		if(all_keys) {
			reply = Redis::Command(redis_ctx, 0, "HSCAN %scustkeys %d MATCH *", entry_name.c_str(), cursor);
			if (reply.values.size() == 0 || reply.values.front().type == Redis::REDIS_RESPONSE_TYPE_ERROR)
				goto error_cleanup;

			v = reply.values[0].arr_value.values[0].second;
			arr = reply.values[0].arr_value.values[1].second;

			if (arr.type == Redis::REDIS_RESPONSE_TYPE_ARRAY) {
				if(v.type == Redis::REDIS_RESPONSE_TYPE_STRING) {
					cursor = atoi(v.value._str.c_str());
				} else if(v.type == Redis::REDIS_RESPONSE_TYPE_INTEGER) {
					cursor = v.arr_value.values[0].second.value._int;
				}
				
				if(arr.arr_value.values.size() <= 1) {
					return;
				}
				for(int i=0;i<arr.arr_value.values.size();i+=2) {

					if(arr.arr_value.values[1].first != Redis::REDIS_RESPONSE_TYPE_STRING)
						continue;

					std::string key = arr.arr_value.values[i].second.value._str;
					if (arr.arr_value.values[i+1].first == Redis::REDIS_RESPONSE_TYPE_STRING) {
						server->kvFields[key] = OS::strip_quotes(arr.arr_value.values[i + 1].second.value._str);
					}
					else if(arr.arr_value.values[i + 1].first == Redis::REDIS_RESPONSE_TYPE_INTEGER) {
						server->kvFields[key] = arr.arr_value.values[i + 1].second.value._int;
					}

					if(std::find(ret->captured_basic_fields.begin(), ret->captured_basic_fields.end(), arr.arr_value.values[i + 1].second.value._str) == ret->captured_basic_fields.end()) {
						ret->captured_basic_fields.push_back(arr.arr_value.values[i].second.value._str);
					}
				}
			}

			do {
				cursor = 0;
				s << entry_name << "custkeys_player_" << idx;
				key = s.str();

				reply = Redis::Command(redis_ctx, 0, "EXISTS %s", key.c_str());
				if (reply.values.size() == 0 || reply.values.front().type == Redis::REDIS_RESPONSE_TYPE_ERROR)
				        break;

				v = reply.values.front();
				if(v.type == Redis::REDIS_RESPONSE_TYPE_INTEGER) {
				        if(!v.value._int)
							break;
				} else if(v.type == Redis::REDIS_RESPONSE_TYPE_STRING) {
					if(!atoi(v.value._str.c_str()))
						break;
				}


				reply = Redis::Command(redis_ctx, 0, "HSCAN %s %d MATCH *", key.c_str(), cursor);
				if (reply.values.size() == 0 || reply.values.front().type == Redis::REDIS_RESPONSE_TYPE_ERROR)
					break;

				v = reply.values.front();
				if (v.type == Redis::REDIS_RESPONSE_TYPE_ARRAY) {
					if(v.arr_value.values.size() <= 0)  {
						break;
					}
					arr = reply.values[1];
					for (int i = 0; i<arr.arr_value.values.size(); i += 2) {

						if (arr.arr_value.values[1].first != Redis::REDIS_RESPONSE_TYPE_STRING)
							continue;

						std::string key = arr.arr_value.values[i].second.value._str;
						if (arr.arr_value.values[i + 1].first == Redis::REDIS_RESPONSE_TYPE_STRING) {
							server->kvPlayers[idx][key] = OS::strip_quotes(arr.arr_value.values[i + 1].second.value._str);
						}
						else if (arr.arr_value.values[i + 1].first == Redis::REDIS_RESPONSE_TYPE_INTEGER) {
							server->kvPlayers[idx][key] = arr.arr_value.values[i + 1].second.value._int;
						}

						if(std::find(ret->captured_player_fields.begin(), ret->captured_player_fields.end(), arr.arr_value.values[i].second.value._str) == ret->captured_player_fields.end()) {
							ret->captured_player_fields.push_back(arr.arr_value.values[i].second.value._str);
						}
					}
				}
				s.str("");
				idx++;
			} while(last_type != Redis::REDIS_RESPONSE_TYPE_NULL);
			s.str("");

			idx = 0;

			do {
				s << entry_name << "custkeys_team_" << idx;
				key = s.str();

				reply = Redis::Command(redis_ctx, 0, "EXISTS %s", key.c_str());
				if (reply.values.size() == 0 || reply.values.front().type == Redis::REDIS_RESPONSE_TYPE_ERROR)
					break;

				v = reply.values.front();
				if (v.type == Redis::REDIS_RESPONSE_TYPE_INTEGER) {
					if (!v.value._int)
						break;
				}
				else if (v.type == Redis::REDIS_RESPONSE_TYPE_STRING) {
					if (!atoi(v.value._str.c_str()))
						break;
				}

				cursor = 0;
				do {
					reply = Redis::Command(redis_ctx, 0, "HSCAN %s %d MATCH *", key.c_str(), cursor);
					if (reply.values.size() < 2|| reply.values.front().type == Redis::REDIS_RESPONSE_TYPE_ERROR)
						break;

					v = reply.values[0];
					arr = reply.values[1];

					if (v.type == Redis::REDIS_RESPONSE_TYPE_ARRAY) {
						if (v.arr_value.values.size() <= 0) {
							break;
						}
						for (int i = 0; i<arr.arr_value.values.size(); i += 2) {

							if (arr.arr_value.values[1].first != Redis::REDIS_RESPONSE_TYPE_STRING)
								continue;

							std::string key = arr.arr_value.values[i].second.value._str;
							if (arr.arr_value.values[i + 1].first == Redis::REDIS_RESPONSE_TYPE_STRING) {
								server->kvTeams[idx][key] = OS::strip_quotes(arr.arr_value.values[i + 1].second.value._str);
							}
							else if (arr.arr_value.values[i + 1].first == Redis::REDIS_RESPONSE_TYPE_INTEGER) {
								server->kvTeams[idx][key] = arr.arr_value.values[i + 1].second.value._int;
							}

							if(std::find(ret->captured_team_fields.begin(), ret->captured_team_fields.end(), arr.arr_value.values[i].second.value._str) == ret->captured_team_fields.end()) {
								ret->captured_team_fields.push_back(arr.arr_value.values[i].second.value._str);
							}
						}
					}
					s.str("");
				} while(cursor != 0);
				idx++;
			} while(true);

			idx = 0;
			all_cust_keys = server->kvFields;
		} else {
			do {
				reply = Redis::Command(redis_ctx, 0, "HSCAN %scustkeys %d MATCH *", entry_name.c_str(), cursor);
				if (reply.values.size() == 0 || reply.values.front().type == Redis::REDIS_RESPONSE_TYPE_ERROR)
                        goto error_cleanup;

				v = reply.values[0];
				arr = reply.values[1];

				if (arr.type == Redis::REDIS_RESPONSE_TYPE_ARRAY) {
					if (v.arr_value.values.size() <= 0) {
						break;
					}
					if (v.type == Redis::REDIS_RESPONSE_TYPE_INTEGER) {
						if (!v.value._int)
							break;
					}
					else if (v.type == Redis::REDIS_RESPONSE_TYPE_STRING) {
						if (!atoi(v.value._str.c_str()))
							break;
					}
					if (arr.arr_value.values.size() <= 0) {
						break;
					}
					for (int i = 0; i<arr.arr_value.values.size(); i += 2) {
						if (arr.arr_value.values[1].first != Redis::REDIS_RESPONSE_TYPE_STRING)
							continue;

						std::string key = arr.arr_value.values[i].second.value._str;
						if (arr.arr_value.values[i + 1].first == Redis::REDIS_RESPONSE_TYPE_STRING) {
                                server->kvFields[key] = OS::strip_quotes(arr.arr_value.values[i + 1].second.value._str);
	                    }
						else if (arr.arr_value.values[i + 1].first == Redis::REDIS_RESPONSE_TYPE_INTEGER) {
	                            server->kvFields[key] = arr.arr_value.values[i + 1].second.value._int;
        	            }

	                    if(std::find(ret->captured_basic_fields.begin(), ret->captured_basic_fields.end(), arr.arr_value.values[i].second.value._str) == ret->captured_basic_fields.end()) {
	                            ret->captured_basic_fields.push_back(arr.arr_value.values[i].second.value._str);
	                    }
					}
				}
			} while(cursor != 0);


			all_cust_keys = server->kvFields;
			server->kvFields.clear(); //remove all keys
			std::map<std::string, std::string>::iterator it = all_cust_keys.begin();
			while(it != all_cust_keys.end()) {
				std::pair<std::string, std::string> p = *it;

				//add only keys which were requested
				if(std::find(ret->requested_fields.begin(), ret->requested_fields.end(), p.first) != ret->requested_fields.end()) {
					server->kvFields[p.first] = p.second;
				}
				it++;
			}
		}

		if(!req || filterMatches(req->filter.c_str(), all_cust_keys)) {
			ret->list.push_back(server);
		}
		return;

		error_cleanup:
			delete server;

	}
	bool MMQueryTask::FindAppend_PlayerKVFields(Server *server, std::string entry_name, std::string key, int index, Redis::Connection *redis_ctx)
	 {
		Redis::Response reply;
		Redis::Value v;
		reply = Redis::Command(redis_ctx, 0, "HGET %s %s", entry_name.c_str(), key.c_str());
		if (reply.values.size() == 0 || reply.values.front().type == Redis::REDIS_RESPONSE_TYPE_ERROR)
			return false;

		v = reply.values.front();
		if (v.type == Redis::REDIS_RESPONSE_TYPE_STRING) {
			server->kvPlayers[index][key] = OS::strip_quotes(v.value._str);
		}
		if (v.type == Redis::REDIS_RESPONSE_TYPE_INTEGER) {
			server->kvPlayers[index][key] = v.value._int;
		}

		return true;

	}
	bool MMQueryTask::FindAppend_TeamKVFields(Server *server, std::string entry_name, std::string key, int index, Redis::Connection *redis_ctx) {
		Redis::Response reply;
		Redis::Value v;
		reply = Redis::Command(redis_ctx, 0, "HGET %s %s", entry_name.c_str(), key.c_str());
		if (reply.values.size() == 0 || reply.values.front().type == Redis::REDIS_RESPONSE_TYPE_ERROR)
			return false;

		v = reply.values.front();
		if (v.type == Redis::REDIS_RESPONSE_TYPE_STRING) {
			server->kvTeams[index][key] = OS::strip_quotes(v.value._str);
		}
		if (v.type == Redis::REDIS_RESPONSE_TYPE_INTEGER) {
			server->kvTeams[index][key] = v.value._int;
		}

		return true;

	}
	bool MMQueryTask::FindAppend_ServKVFields(Server *server, std::string entry_name, std::string key, Redis::Connection *redis_ctx) {
		Redis::Response reply;
		Redis::Value v;
		reply = Redis::Command(redis_ctx, 0, "HGET %s %s", entry_name.c_str(), key.c_str());
		if (reply.values.size() == 0 || reply.values.front().type == Redis::REDIS_RESPONSE_TYPE_ERROR)
			return false;

		v = reply.values.front();
		if (v.type == Redis::REDIS_RESPONSE_TYPE_STRING) {
			server->kvFields[key] = OS::strip_quotes(v.value._str);
		}
		if (v.type == Redis::REDIS_RESPONSE_TYPE_INTEGER) {
			server->kvFields[key] = v.value._int;
		}

		return true;

	}
	void MMQueryTask::AppendGroupEntry(const char *entry_name, ServerListQuery *ret, Redis::Connection *redis_ctx, bool all_keys) {
		Redis::Response reply;
		Redis::Value v, arr;
		int cursor = 0;

		/*
		XXX: add redis error checks, cleanup on error, etc
		*/

		std::vector<std::string>::iterator it = ret->requested_fields.begin();
		Server *server = new MM::Server();

		Redis::Command(redis_ctx, 0, "SELECT %d", OS::ERedisDB_SBGroups);

		reply = Redis::Command(redis_ctx, 0, "HGET %s gameid", entry_name);
		if (reply.values.size() == 0 || reply.values.front().type == Redis::REDIS_RESPONSE_TYPE_ERROR)
			goto error_cleanup;

		v = reply.values.front();

		server->game = OS::GetGameByID(atoi(OS::strip_quotes(v.value._str).c_str()));
		
		Redis::Command(redis_ctx, 0, "SELECT %d", OS::ERedisDB_SBGroups); //change context back to SB db id

		reply = Redis::Command(redis_ctx, 0, "HGET %s groupid", entry_name);
		if (reply.values.size() == 0 || reply.values.front().type == Redis::REDIS_RESPONSE_TYPE_ERROR)
			goto error_cleanup;

		server->wan_address.ip = atoi(OS::strip_quotes(v.value._str).c_str()); //for V2
		server->kvFields["groupid"] = OS::strip_quotes(v.value._str).c_str(); //for V1
		
		FindAppend_ServKVFields(server, entry_name, "maxwaiting", redis_ctx);
		FindAppend_ServKVFields(server, entry_name, "hostname", redis_ctx);
		FindAppend_ServKVFields(server, entry_name, "password", redis_ctx);
		FindAppend_ServKVFields(server, entry_name, "numwaiting", redis_ctx);
		FindAppend_ServKVFields(server, entry_name, "numservers", redis_ctx);

		if(all_keys) {
			/*
			reply = (redisReply *)redisCommand(redis_ctx, "HKEYS %scustkeys", entry_name);
			if (!reply)
				goto error_cleanup;
			if (reply->type == REDIS_REPLY_ARRAY) {
				for (unsigned int j = 0; j < reply->elements; j++) {
					std::string search_key = entry_name;
					search_key += "custkeys";
					FindAppend_ServKVFields(server, search_key, reply->element[j]->str, redis_ctx);
					if(std::find(ret->captured_basic_fields.begin(), ret->captured_basic_fields.end(), reply->element[j]->str) == ret->captured_basic_fields.end()) {
						ret->captured_basic_fields.push_back(reply->element[j]->str);
					}
				}
			}*/



			do {
				reply = Redis::Command(redis_ctx, 0, "HSCAN %scustkeys %d match *", entry_name);
				if (reply.values.size() < 1 || reply.values.front().type == Redis::REDIS_RESPONSE_TYPE_ERROR)
					goto error_cleanup;

					v = reply.values[0].arr_value.values[0].second;
					arr = reply.values[0].arr_value.values[1].second;
					if (arr.type == Redis::REDIS_RESPONSE_TYPE_ARRAY) {
						if(v.type == Redis::REDIS_RESPONSE_TYPE_STRING) {
					 		cursor = atoi(v.value._str.c_str());
					 	} else if (v.type == Redis::REDIS_RESPONSE_TYPE_INTEGER) {
					 		cursor = v.value._int;
					 	}

						if(arr.arr_value.values.size() <= 0) {
							break;
						}

						for(int i=0;i<arr.arr_value.values.size();i++) {
							std::string search_key = entry_name;
							search_key += "custkeys";
							FindAppend_ServKVFields(server, search_key, arr.arr_value.values[i].second.value._str, redis_ctx);
							if(std::find(ret->captured_basic_fields.begin(), ret->captured_basic_fields.end(), arr.arr_value.values[i].second.value._str) == ret->captured_basic_fields.end()) {
								ret->captured_basic_fields.push_back(arr.arr_value.values[i].second.value._str);
							}
						}
					}
			} while(cursor != 0);

		} else {

			while (it != ret->requested_fields.end()) {
				std::string field = *it;
				std::string entry = entry_name;
				entry += "custkeys";
				FindAppend_ServKVFields(server, entry, field, redis_ctx);
				it++;
			}
	}

		ret->list.push_back(server);
		return;

	error_cleanup:
		delete server;

	}
	ServerListQuery MMQueryTask::GetServers(const sServerListReq *req) {
		ServerListQuery ret;

		Redis::Response reply;
		Redis::Value v, arr;

		ret.requested_fields = req->field_list;

		Redis::Command(mp_redis_connection, 0, "SELECT %d", OS::ERedisDB_QR);
		
		int cursor = 0;
		do {
			reply = Redis::Command(mp_redis_connection, 0, "SCAN %d MATCH %s:*:", cursor, req->m_for_game.gamename);
			if (Redis::CheckError(reply))
				goto error_cleanup;

			v = reply.values[0].arr_value.values[0].second;
			arr = reply.values[0].arr_value.values[1].second;

		 	if(v.type == Redis::REDIS_RESPONSE_TYPE_STRING) {
		 		cursor = atoi(v.value._str.c_str());
		 	} else if (v.type == Redis::REDIS_RESPONSE_TYPE_INTEGER) {
		 		cursor = v.value._int;
		 	}

			if(arr.arr_value.values.size() <= 0) {
				break;
			}

			for(int i=0;i<arr.arr_value.values.size();i++) {
				AppendServerEntry(arr.arr_value.values[i].second.value._str, &ret, req->all_keys, false, mp_redis_connection, req);
			}
		} while(cursor != 0);

		error_cleanup:
			return ret;
	}
	ServerListQuery MMQueryTask::GetGroups(const sServerListReq *req) {
		ServerListQuery ret;
		Redis::Response reply;
		Redis::Value v;
		Redis::ArrayValue arr;

		ret.requested_fields = req->field_list;

		Redis::Command(mp_redis_connection, 0, "SELECT %d", OS::ERedisDB_SBGroups);

		int cursor = 0;
		do {
			reply = Redis::Command(mp_redis_connection, 0, "SCAN %d MATCH %s:*:", cursor, req->m_for_game.gamename);
			if (Redis::CheckError(reply))
				goto error_cleanup;

			v = reply.values[0].arr_value.values[0].second;
			arr = reply.values[0].arr_value.values[1].second.arr_value;

			if (v.type == Redis::REDIS_RESPONSE_TYPE_STRING) {
				cursor = atoi(v.value._str.c_str());
			}
			else if (v.type == Redis::REDIS_RESPONSE_TYPE_INTEGER) {
				cursor = v.value._int;
			}

			for (int i = 0; i<arr.values.size(); i++) {
				AppendGroupEntry(arr.values[i].second.value._str.c_str(), &ret, mp_redis_connection, req->all_keys);
			}
		} while(cursor != 0);

		error_cleanup:
			return ret;
	}
	Server *MMQueryTask::GetServerByKey(std::string key, Redis::Connection *redis_ctx, bool include_deleted) {
		if(redis_ctx == NULL) {
			redis_ctx = mp_redis_connection;
		}
		ServerListQuery ret;
		AppendServerEntry(key, &ret, true, include_deleted, redis_ctx, NULL);
		if(ret.list.size() < 1)
			return NULL;

		return ret.list[0];
	}
	Server *MMQueryTask::GetServerByIP(OS::Address address, OS::GameData game, Redis::Connection *redis_ctx) {
		Server *server = NULL;
		if(redis_ctx == NULL) {
			redis_ctx = mp_redis_connection;
		}
		std::ostringstream s;
		Redis::Response reply;
		Redis::Value v;

		struct sockaddr_in addr;
		addr.sin_port = Socket::htons(address.port);
		addr.sin_addr.s_addr = Socket::htonl(address.ip);
		const char *ipinput = Socket::inet_ntoa(addr.sin_addr);

		Redis::Command(mp_redis_connection, 0, "SELECT %d", OS::ERedisDB_QR);

		s << "GET IPMAP_" << game.gamename << "_" << ipinput << "-" << address.port;
		std::string cmd = s.str();
		reply = Redis::Command(mp_redis_connection, 0, cmd.c_str());
		if (reply.values.size() < 1 || reply.values.front().type == Redis::REDIS_RESPONSE_TYPE_ERROR)
			goto error_cleanup;

		v = reply.values.front();

		if(v.type == Redis::REDIS_RESPONSE_TYPE_STRING) {
			server = GetServerByKey(v.value._str, redis_ctx);
		}



		return server;

		error_cleanup:
			if (server)
				delete server;
			return NULL;
	}
	void MMQueryTask::FreeServerListQuery(struct MM::ServerListQuery *query) {
		std::vector<Server *>::iterator it = query->list.begin();
		while(it != query->list.end()) {
			Server *server = *it;
			delete server;
			it++;
		}
	}

	void MMQueryTask::PerformServersQuery(MMQueryRequest request) {
		request.callback(request, GetServers(&request.req), request.extra);
	}
	void MMQueryTask::PerformGroupsQuery(MMQueryRequest request) {
		request.callback(request, GetGroups(&request.req), request.extra);
	}
	void MMQueryTask::PerformGetServerByKey(MMQueryRequest request) {
		ServerListQuery ret;
		AppendServerEntry(request.key, &ret, true, false, NULL, NULL);
		request.callback(request, ret, request.extra);
		FreeServerListQuery(&ret);
	}
	void MMQueryTask::PerformGetServerByIP(MMQueryRequest request) {
		ServerListQuery ret;
		Server *serv = GetServerByIP(request.address, request.SubmitData.game, NULL);
		if(serv)
			ret.list.push_back(serv);
		request.callback(request, ret, request.extra);
		FreeServerListQuery(&ret);
	}
	void MMQueryTask::PerformSubmitData(MMQueryRequest request) {
		Redis::Command(mp_redis_connection, 0, "PUBLISH %s \\send_msg\\%s\\%s\\%d\\%s\\%d\\%s\n",
			sb_mm_channel,request.SubmitData.game.gamename,Socket::inet_ntoa(request.SubmitData.from.sin_addr),
			request.SubmitData.from.sin_port,Socket::inet_ntoa(request.SubmitData.to.sin_addr),
			request.SubmitData.to.sin_port,request.SubmitData.base64.c_str());
	}
	void *MMQueryTask::TaskThread(OS::CThread *thread) {
		MMQueryTask *task = (MMQueryTask *)thread->getParams();
		for(;;) {
			if(task->m_request_list.size() > 0) {
				std::vector<MMQueryRequest>::iterator it = task->m_request_list.begin();
				task->mp_mutex->lock();
				while(it != task->m_request_list.end()) {
					MMQueryRequest task_params = *it;
					switch(task_params.type) {
						case EMMQueryRequestType_GetServers:
							task->PerformServersQuery(task_params);
							break;
						case EMMQueryRequestType_GetGroups:
							task->PerformGroupsQuery(task_params);
							break;
						case EMMQueryRequestType_GetServerByKey:
							task->PerformGetServerByKey(task_params);
							break;
						case EMMQueryRequestType_GetServerByIP:
							task->PerformGetServerByIP(task_params);
							break;
						case EMMQueryRequestType_SubmitData:
							task->PerformSubmitData(task_params);
							break;
					}
					it = task->m_request_list.erase(it);
					continue;
				}
				task->mp_mutex->unlock();
			}
			OS::Sleep(TASK_SLEEP_TIME);
		}
		return NULL;
	}
}
