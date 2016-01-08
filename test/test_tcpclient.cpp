#include <iostream>
#include <string>
#include "../tcpclient.h"
using namespace std;
using namespace UVNET;

std::string serverip;
int serverport;
int call_time = 0;
bool is_exist = false;

void CloseCB(int clientid, void* userdata)
{
    fprintf(stdout, "cliend %d close\n", clientid);
    TCPClient* client = (TCPClient*)userdata;
    client->Close();
}

void ReadCB(const NetPacket& packet, const unsigned char* buf, void* userdata)
{
    fprintf(stdout,"call time %d\n",++call_time);
    if (call_time > 5000) {
        return;
    }
    char senddata[256] = {0};
    TCPClient* client = (TCPClient*)userdata;
    sprintf(senddata, "****recv server data(%p,%d)\n", client, packet.datalen);
    for(int i =0; i < packet.datalen; i++)
	    printf("%x ", buf[i]);
    printf("\n");
    fprintf(stdout, "%s\n", senddata);
    /*
    NetPacket tmppack = packet;
    tmppack.datalen = (std::min)(strlen(senddata), sizeof(senddata) - 1);
    std::string retstr = PacketData(tmppack, (const unsigned char*)senddata);
    if (client->Send(&retstr[0], retstr.length()) <= 0) {
        fprintf(stdout, "(%p)send error.%s\n", client, client->GetLastErrMsg());
    }
    */
}

int main(int argc, char** argv)
{
    if (argc != 3) {
        fprintf(stdout, "usage: %s server_ip_address server_port\neg.%s 192.168.1.1 50\n", argv[0], argv[0]);
        return 0;
    }
    serverip = argv[1];
    serverport = std::stoi(argv[2]);

    TCPClient client(0x01, 0x02);
    client.SetRecvCB(ReadCB, &client);
    client.SetCloseCB(CloseCB, &client);
    if(!client.Connect(serverip.c_str(), serverport))
    {
            fprintf(stdout, "connect error:%s\n", client.GetLastErrMsg());
    }
    else
    {
            fprintf(stdout, "client(%p) connect succeed.\n", &client);
    }
    char senddata[256];
    memset(senddata, 0, sizeof(senddata));
    sprintf(senddata, "client(%p) call 1", &client);
    NetPacket packet;
    packet.header = 0x01;
    packet.tail = 0x02;
    packet.datalen = (std::min)(strlen(senddata), sizeof(senddata) - 1);
    std::string str = PacketData(packet, (const unsigned char*)senddata);
    if (client.Send(&str[0], str.length()) <= 0) 
    {
    	fprintf(stdout, "(%p)send error.%s\n", &client, client.GetLastErrMsg());
    }
    else
    {
	fprintf(stdout, "send succeed:%s\n", senddata);
    }
	
    while (!is_exist) {
        ThreadSleep(10);
    }
    return 0;
}
