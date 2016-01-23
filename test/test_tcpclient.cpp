#include <iostream>
#include <string>
#include "../tcpclient.h"
#include "../pb/netmessage.pb.h"
using namespace std;
using namespace UVNET;

std::string serverip;
int serverport;
int call_time = 0;
bool is_exist = false;

class EchoProtocol: public Protocol
{
public:
	EchoProtocol(){}
	virtual ~EchoProtocol(){}
	virtual const std::string& Process(const char * buf, int length){
		EchoProto ep;
		ep.ParseFromString(std::string(buf, length));	
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

void CloseCB(int clientid, void* userdata)
{
    fprintf(stdout, "cliend %d close\n", clientid);
    TCPClient* client = (TCPClient*)userdata;
    client->Close();
}

void ReadCB(const NetPacket& packet, const char* buf, void* userdata)
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
    //client.SetRecvCB(ReadCB, &client);
    client.SetCloseCB(CloseCB, &client);
    EchoProtocol protocol;
    client.AddProtocol(1, &protocol);
    if(!client.Connect(serverip.c_str(), serverport))
    {
            fprintf(stdout, "connect error:%s\n", client.GetLastErrMsg());
    }
    else
    {
            fprintf(stdout, "client(%p) connect succeed.\n", &client);
    }
    
    EchoProto ep;
    ep.set_data("client call 1");
    std::string data;
    CProto cproto;
    cproto.set_id(1);
    cproto.set_body(ep.SerializeAsString());
    cproto.SerializeToString(&data);

    NetPacket packet;
    packet.header = 0x01;
    packet.tail = 0x02;
    packet.type = 1;
    packet.datalen = data.size();
    std::string str = PacketData(packet, data.c_str());
    if (client.Send(&str[0], str.length()) <= 0) 
    {
    	fprintf(stdout, "(%p)send error.%s\n", &client, client.GetLastErrMsg());
    }
    else
    {
	fprintf(stdout, "send succeed\n");
    }
	
    while (!is_exist) {
        ThreadSleep(10);
    }
    return 0;
}
