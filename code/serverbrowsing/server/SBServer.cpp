#include "SBPeer.h"
#include "SBServer.h"
#include "SBDriver.h"
#include <OS/OpenSpy.h>
SBServer::SBServer() : INetServer() {
	uv_loop_set_data(uv_default_loop(), this);
}
SBServer::~SBServer() {
}
void SBServer::tick() {
	std::vector<INetDriver *>::iterator it = m_net_drivers.begin();
	while (it != m_net_drivers.end()) {
		INetDriver *driver = *it;
		driver->think();
		it++;
	}
	NetworkTick();
}
void SBServer::OnNewServer(MM::Server server) {
	std::vector<INetDriver *>::iterator it = m_net_drivers.begin();
	while (it != m_net_drivers.end()) {
		SB::Driver *driver = (SB::Driver *)*it;
		driver->AddNewServer(server);
		it++;
	}
}
void SBServer::OnUpdateServer(MM::Server server) {
	std::vector<INetDriver *>::iterator it = m_net_drivers.begin();
	while (it != m_net_drivers.end()) {
		SB::Driver *driver = (SB::Driver *)*it;
		driver->AddUpdateServer(server);
		it++;
	}
}
void SBServer::OnDeleteServer(MM::Server server) {
	std::vector<INetDriver *>::iterator it = m_net_drivers.begin();
	while (it != m_net_drivers.end()) {
		SB::Driver *driver = (SB::Driver *)*it;
		driver->AddDeleteServer(server);
		it++;
	}
}