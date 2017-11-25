# WsServer - WebSocket message server

## Features
* Multi-threaded (boost thread pool)
* Undelivered messages queue (with TTL in future)
* Multiple recipients in one message
* Transparent admin user (use sender=0)
* ws/wss protocols (text, binary, continuation (fragmented)) 
* Support fragmented frame buffer
* Benchmark (almost stress-test) included
* Custom payload via json
* User-independent
* Payload size limit
* REST Api server
	* list active users with simple statistics
	* sending message
* Event notifier. Send message copy to your server. Supports couple auth methods: **basic**, **header-based**, **bearer**
	* Postbacks: set url on where notifier will send messages payload
	
### Todo features
* Lock-free queues
* Horizontal scaling (custom cluster or using another solution)
* Persistence for queued messages
* Multi-connection support (multiple connections for single user, example: connected with few devices)
* Runtime switch using **ws**/**wss** protocol (now only at compile time: `-DUSE_SSL=ON`)
* Event notifier targets:
	* Redis pub/sub
	* SQL (PostgreSQL, MySQL)
	* MongoDB
	* Maybe: Cloud DBs (like Firebase CDB)
	* Maybe: some message queue like RabbitMQ
	* Maybe: unix socket, tcp or udp, or all of this
* Maybe: Process forks instead of threads

 

## Requirements
* CMake 3.7+
* GCC/CLang: -stdlib=c++14
* Boost (system thread coroutine context random, GCC < 4.9 - regex)
* Boost.Asio
* OpenSSL
* ToolBox++ (https://github.com/edwardstock/toolboxpp) 
 
 
## Cloning (with submodules)
```bash
git clone --recursive=100 git@github.com/edwardstock/wsserver.git
```
 
## Build
TBD

## Configuring
TBD