# OpenSpy Core (v2)

This is a full rewrite of the old openspy. Each service runs as a seperate process, communicating over HTTP, AMQP, and also using redis as a data store.

[See here](./DESIGN.md) for some architecture info

## TODO / Things to improve
* Remove HTTP calls, replacing with AMQP listeners


## Building
A [docker file](Dockerfile) has been created to convey the required build and runtime environment. This has not been used for development, but can be helpful for testing and just getting the project running.

```sh
git clone https://github.com/chc/openspy-core-v2.git
cd openspy-core-v2
docker build -t os-core .
docker-compose up
```

## Running
If you refer to the "openspy-web-backend" project, this will have everything you need to get openspy running.
From the perspective of this application, the requirements are redis, rabbitmq, and then the openspy-web-backend.

Depending on the services you will be running, you can get away without running openspy-web-backend, but it is recommended. Redis and rabbitmq are required.

### Configuration
A single openspy.xml file is used for each service. 
[See here for reference.](docker-support/openspy.xml)
Environment variables and literal values are supported.
If for whatever reason you want to seperate this file just change the working directly of the application.
Within this file you will need to configure the AMQP (rabbitmq) connection details, redis connection details, and the HTTP API web service details.

HA Proxy headers are also supported, but due to UDP servers it is not fully usable throughout the application. UDP & TCP load balancers have been used in the past, but UDP load balancer support can be spotty around providers. In the case of UDP, the NAT layer must be transparent, which is typically not the case. Due to this, this feature is no longer used, but remains in the code.

## Services
### QR (Query & Reporting)
This service handles both the old V1 QR protocol (ASCII based), and the V2 protocol (binary based).
GameSpy implemented this on the same port, so version is determined by the presence of a '\' as the first byte.

This service is used to push new servers to be queried by server browsing.

### Server Browsing
This service also has 2 versions.

This service is used to query active servers.

The legacy V1 protocol (which includes enctype 0 and 2 support) listens on port 28900. Enctype 1 is not supported, by as far as I know, that was only ever used in "GameSpy 3D".

The V2 protocol, also known as enctypex listens on port 28910. Search filtering is supported, as well as event pushing and group listings. Map loop, and player search is not supported, and no game has been seen requiring this.

### NatNeg
This service is used so that port forwarding is typically not required when hosting games. As with all gamespy services, only IPV4 is supported. Due to IPV4 exhaustion, and ISPs implementing CGN (Carrier Grade NAT), support can be spotty, such as over 4G networks, or in smaller countries like Poland where ISPs are implementing this. In these situations you may be able to port forward, or need to contact your ISP.

For full support, this service requires 3 seperate IPs to listen on

Additionally, the "NAT Neg Helper" project is required for this service. It functions as an AMQP listener and assists in the logic for NAT type resolving.

### GP (GameSpy Presence)
This service is used for things like buddy lists, buddy messaging, and some account management. It listens on port 29900.

### Search
This service is used for account lookups, and some account registration. It listens on port 29901.

### GStats
This service is used for profile specific data storage, and game session snapshot submission for later stats processing (such as leaderboards). It listens on port 29920.

### FESL
This service is actually not part of GameSpy, but instead is an EA service which formally integrated with GameSpy (using PreAuth). Currently only BF2142 is supported. In the past Red Alert 3 was tested and worked, but it was not tested in great detail. The listening port and SSL type differs depending on the game.

### Peerchat
This service is used for many things, such as game lobby chat, in some games the room chat, and even in some games it is used in place of, or in combination with the Server Browsing protocol. Its a custom IRC server listening on the default port of 6667.

### UTMaster
This service is a master server for UT2003 and UT2004. Currently a work in progress, but fairly functional.