﻿#include <iostream>
#include <string>
#include "../tcpserver.h"
#include "../net_base.h"
#include <cstdio>
#include "uv.h"

class TestTCPProtocol: public TCPServerProtocolProcess
{
public:
	TestTCPProtocol(){}
	virtual ~TestTCPProtocol(){}
	virtual const std::string& ParsePacket(const NetPacket& packet, const unsigned char* buf){
		static char senddata[256];
		sprintf(senddata,"****recv datalen %d",packet.datalen);
		fprintf(stdout,"%s\n",senddata);

		NetPacket tmppack = packet;
		tmppack.datalen = (std::min)(strlen(senddata),sizeof(senddata)-1);
		response = PacketData(tmppack,(const unsigned char*)senddata);
		return response;
	}
private:
	std::string response;
};

using namespace std;
using namespace uv;
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
	TestTCPProtocol protocol;
    // DeclareDumpFile();
    TCPServer::StartLog(1, "tcpserver", "log/");
    server.SetNewConnectCB(NewConnect,&server);
	server.SetPortocol(&protocol);
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