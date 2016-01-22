/***************************************
* @file     tcpserverprotocolprocess.h
* @brief    TCP Server protocol pure base class,need to subclass.
* @details  
* @author   phata, wqvbjhc@gmail.com
* @date     2014-7-31
****************************************/
#ifndef _PROTOCOL_H
#define _PROTOCOL_H
#include <string>

class Protocol
{
public:
    Protocol(){}
    virtual ~Protocol(){}

    //parse the recv packet, and make the response packet return. see test_tcpserver.cpp for example
	//packet     : the recv packet
	//buf        : the packet data
	//std::string: the response packet. no response can't return empty string.
    virtual const std::string& Process(const char* buf, int length) = 0;
};

#endif//_PROTOCOL_H
