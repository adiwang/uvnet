#include <iostream>
#include <string>
#include "../tcpserver.h"
#include "../net_base.h"
#include "../protocol.h"
#include "../packet_sync.h"
#include "../log.h"
#include "../pb/netmessage.pb.h"
#include <cstdio>
#include "uv.h"

class EchoProtocol: public Protocol
{
public:
	EchoProtocol(){}
	virtual ~EchoProtocol(){}
	virtual const std::string& Process(const char * buf, int length){
		EchoProto ep;
		ep.ParseFromArray(buf, length);	
		printf("recv string: %s\n", ep.data().c_str());

		std::string data;
		CProto cproto;
		cproto.set_id(1);
		cproto.set_body(ep.SerializeAsString());
		cproto.SerializeToString(&data);

		NetPacket tmppack;
		tmppack.header = 0x01;
		tmppack.tail = 0x02;
		tmppack.type = 1;
		tmppack.datalen = data.size();
		response = PacketData(tmppack, data.c_str());
		return response;
	}
private:
	std::string response;
};

using namespace std;
using namespace UVNET;
bool is_eist = false;
int call_time = 0;

UVNET::TCPServer server(0x01,0x02);

void CloseCB(int clientid, void* userdata)
{
    fprintf(stdout,"cliend %d close\n",clientid);
    TCPServer *theclass = (TCPServer *)userdata;
    //is_eist = true;
}

void NewConnect(int clientid, void* userdata)
{
    fprintf(stdout,"new connect:%d\n",clientid);
    server.SetRecvCB(clientid,NULL,NULL);
}

int main(int argc, char** argv)
{
	EchoProtocol protocol;
    // DeclareDumpFile();
    TCPServer::StartLog(LL_DEBUG, "tcpserver", "./log");
    server.SetNewConnectCB(NewConnect,&server);
    server.AddProtocol(1, &protocol);
    if(!server.Start("0.0.0.0",12345)) {
        fprintf(stdout,"Start Server error:%s\n",server.GetLastErrMsg());
    }
	server.SetKeepAlive(1,60);//enable Keepalive, 60s
    fprintf(stdout,"server return on main.\n");
    while(!is_eist) {
        ThreadSleep(10);
    }
    return 0;
}
