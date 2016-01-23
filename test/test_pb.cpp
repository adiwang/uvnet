#include <iostream>
#include "../pb/netmessage.pb.h"

class Protocol
{
public:
	Protocol() {}
	virtual ~Protocol() {}
	virtual void Process(const char* buf, int length) = 0;
};

class CProtocol1 : public Protocol
{
public:
	CProtocol1() {}
	virtual ~CProtocol1() {}
	static CProtocol1* GetInstance() { static CProtocol1 instance; return &instance; }
	virtual void Process(const char *buf, int length)
	{
		CP1 cp1;
		cp1.ParseFromArray(buf, length);
		std::cout << "CP1: a = " << cp1.a() << ", b = " << cp1.b() << ", c = " << cp1.c() << std::endl;
	}
};

class CProtocol2 : public Protocol
{
public:
	CProtocol2() {}
	virtual ~CProtocol2() {}
	static CProtocol2* GetInstance() { static CProtocol2 instance; return &instance; }
	virtual void Process(const char *buf, int length)
	{
		CP2 cp2;
		cp2.ParseFromArray(buf, length);
		std::cout << "CP2: a = " << cp2.a() << ", b = " << cp2.b() << ", c = " << cp2.c() << std::endl;
	}
};

void HandleProtocol(const char* buf, int length)
{
	CProto proto;
	proto.ParseFromArray(buf, length);
	if(proto.id() == 1)
	{
		CProtocol1::GetInstance()->Process(proto.body().c_str(), proto.body().size());
	}
	else if(proto.id() == 2)
	{
		CProtocol2::GetInstance()->Process(proto.body().c_str(), proto.body().size());
	}
	else
	{
		std::cout << "unknown protocol" << std::endl;
	}
}

int main(int argc, char **argv)
{
	CProto cproto;
	std::string data;
	
	CP1 cp1;
	cp1.set_a(32);
	cp1.set_b(64);
	cp1.set_c("this ia a string");

	cproto.set_id(1);
	cproto.set_body(cp1.SerializeAsString());
	cproto.SerializeToString(&data);
	HandleProtocol(data.c_str(), data.size());

	CP2 cp2;
	cp2.set_a("this is cp2 a string");
	cp2.set_b("this is cp2 b string");
	cp2.set_c(64);

	cproto.set_id(2);
	cproto.set_body(cp2.SerializeAsString());
	cproto.SerializeToString(&data);
	HandleProtocol(data.c_str(), data.size());

	return 0;
}
