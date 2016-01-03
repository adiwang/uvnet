#include <cstdio>
#include <iostream>
#include <stdint.h>
#include <assert.h>
#include "../pod_circularbuffer.h"

struct strdata{
	int idata;
	float fdata;
	double ddata;
	char cdata[7];
};

int main(int argc, char **argv)
{
	{
		PodCircularBuffer<char> objbuf(1000);
		char tmpwchar[10]= {'1','2'};
		char tmprchar[15];
		assert(10 == objbuf.write(tmpwchar,sizeof(tmpwchar)));
		objbuf.read(tmprchar,2);
		assert(tmprchar[0] == '1');
		assert(tmprchar[1] == '2');
		int count = 0;
		while(++count < 10) {
			for (int i=0; i< 11; ++i) {
				int iret = objbuf.write(tmpwchar,sizeof(tmpwchar));
				printf("write %d, size %d, capacity %d\n",iret,objbuf.size(),objbuf.capacity());
			}
			for (int i=0; i< 7; ++i) {
				int iret = objbuf.read(tmprchar,sizeof(tmprchar));
				printf("read %d, size %d, capacity %d\n",iret,objbuf.size(),objbuf.capacity());
			}
		}
	}
	{
#define  ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))
		printf("\n\n\nstart opt struct\n");
		PodCircularBuffer<strdata> objbuf(1000);
		strdata tmpwchar[10];
		strdata tmprchar[15];
		assert(10 == objbuf.write(tmpwchar,ARRAYSIZE(tmpwchar)));
		objbuf.read(tmprchar,2);
		int count = 0;
		while(++count < 10) {
			for (int i=0; i< 11; ++i) {
				int iret = objbuf.write(tmpwchar,ARRAYSIZE(tmpwchar));
				printf("write %d, size %d, capacity %d\n",iret,objbuf.size(),objbuf.capacity());
			}
			for (int i=0; i< 7; ++i) {
				int iret = objbuf.read(tmprchar,ARRAYSIZE(tmprchar));
				printf("read %d, size %d, capacity %d\n",iret,objbuf.size(),objbuf.capacity());
			}
		}
	}
	return 0;
}
