#ifndef _OPENSPY_H
#define _OPENSPY_H
#ifdef _WIN32
#include <Windows.h>
#include <WinSock.h>
#include "Threads/Win32/WinThread.h"
typedef int socklen_t;
#define snprintf sprintf_s
#define strcasecmp _stricmp
#define strncasecmp _strnicmp

int gettimeofday(struct timeval *tp, struct timezone *tzp);
#else
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/times.h>
#include "Threads/PThreads/PThread.h"

#endif
#include <stdlib.h>
#include <string>
#include <vector>
#include <stdint.h>
#include <memory.h>
#include <hiredis/hiredis.h>
namespace OS {

	///////////////////////
	/// XXX: put in os/geo/region.h
	typedef struct {
		const char *countrycode;
		const char *countryname;
		int region;
	} countryRegion;
	///////////////////
	//// XXX: put in os/game.h
	typedef struct {
		int gameid;
		int queryport;
		char gamename[32];
		char description[64];
		char secretkey[7];
		char disabled_services; //0= ok, 1 = temp, 2 = perm
	} GameData;
	GameData GetGameByName(const char *from_gamename);
	GameData GetGameByID(int gameid);
	enum ERedisDB {
		ERedisDB_QR,
		ERedisDB_SBGroups,
		ERedisDB_Game,
	};

	typedef struct {
		uint32_t ip;
		uint16_t port;
	} Address;
	
	void Init();
	std::vector<std::string> KeyStringToMap(std::string input);
	std::string strip_quotes(std::string s);

	//thread
	CThread *CreateThread(ThreadEntry *entry, void *param, bool auto_start);
}



#endif //_OPENSPY_H
