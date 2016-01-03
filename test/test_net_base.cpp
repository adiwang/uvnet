#include <cstdio>
#include <cstring>
#include "../net_base.h"

int main(int argc, char **argv)
{
	printf("islittleendian %d\n",IsLittleendian());
	printf("IsSystem32 %d\n",IsSystem32());
	uint32_t intnum = 0x1234567A;
	unsigned char charnum[4];
	Int32ToChar(intnum,&charnum[0]);
	printf("Int32ToChar-int=0x%x, %d, char=%x,%x,%x,%x\n",intnum,intnum,charnum[0],charnum[1],charnum[2],charnum[3]);
	CharToInt32(&charnum[0],intnum);
	printf("CharToInt32-int=0x%x, %d, char=%x,%x,%x,%x\n",intnum,intnum,charnum[0],charnum[1],charnum[2],charnum[3]);

	uint64_t int64num = 0x123456789ABCDEF0;
	unsigned char char8num[8];
	Int64ToChar(int64num,char8num);
	printf("Int64ToChar-int=0x%I64x, %I64d, char=%x,%x,%x,%x,%x,%x,%x,%x\n",int64num,int64num,char8num[0],char8num[1],char8num[2],
		char8num[3],char8num[4],char8num[5],char8num[6],char8num[7]);
	CharToInt64(char8num,int64num);
	printf("CharToInt64-int=0x%I64x, %I64d, char=%x,%x,%x,%x,%x,%x,%x,%x\n",int64num,int64num,char8num[0],char8num[1],char8num[2],
		char8num[3],char8num[4],char8num[5],char8num[6],char8num[7]);

	printf("sizeof NetPackage=%d\n",sizeof(NetPacket));
	unsigned char packagechar[NET_PACKAGE_HEADLEN];
	NetPacket package;
	package.type = intnum;
	package.reserve = intnum + 1;
	package.datalen = int64num;
	memset(packagechar,0,NET_PACKAGE_HEADLEN);
	NetPacketToChar(package,packagechar);
	printf("NetPackageToChar -- package data (type=%d,reserve=%d,datalen=%d), char=",package.type,package.reserve,package.datalen);
	for (int i=0; i<NET_PACKAGE_HEADLEN; ++i) {
		printf("%x,",packagechar[i]);
	}
	printf("\n");
	memset(&package,0,NET_PACKAGE_HEADLEN);
	CharToNetPacket(packagechar,package);
	printf("CharToNetPackage -- package data (type=%d,reserve=%d,datalen=%d), char=",package.type,package.reserve,package.datalen);
	for (int i=0; i<NET_PACKAGE_HEADLEN; ++i) {
		printf("%x,",packagechar[i]);
	}
	printf("\n");
	memset(packagechar,0,NET_PACKAGE_HEADLEN);
	NetPacketToChar(package,packagechar);
	printf("NetPackageToChar -- package data (type=%d,reserve=%d,datalen=%d), char=",package.type,package.reserve,package.datalen);
	for (int i=0; i<NET_PACKAGE_HEADLEN; ++i) {
		printf("%x,",packagechar[i]);
	}
	printf("\n");
	return 0;
}
