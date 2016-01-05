# uvnet
tcp server warpper use libuv

1. 安装libuv
   (1) ./autogen.sh
   (2) ./configure --prefix=/usr/local
   (3) make
   (4) make install

2. 编译
   由于libuv安装到了/usr/local下，因此uv.h在/usr/local/include, lib在/usr/local/lib
   编译的时候需要-I/usr/local/include -L/usr/local/lib

3. 链接
   (1) export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
   (2) -lpthread -luv -lssl -lcrypto

