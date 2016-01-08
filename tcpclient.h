#ifndef _TCPCLIENT_H
#define _TCPCLIENT_H
#include <string>
#include <list>
#include <uv.h>
#include "packet_sync.h"
#include "pod_circularbuffer.h"

#ifndef BUFFER_SIZE
#define BUFFER_SIZE (1024 * 10)
#endif

namespace UVNET
{
struct TcpClientCtx
{
	uv_tcp_t tcp_handle;
	uv_write_t	write_req;
	PacketSync* packet;
	uv_buf_t	read_buf;
	int sid;
	void* parent_server;

	static TcpClientCtx* Alloc(void* parent_server);
	static void Release(TcpClientCtx* ctx);
};

struct ClientWriteParam
{
	uv_write_t write_req;
	uv_buf_t buf;
	int buf_true_len;

	static ClientWriteParam* Alloc();
	static void Release(ClientWriteParam* param);
};

class TCPClient
{
public:
	TCPClient(unsigned char pack_head, unsigned char pack_tail);
	virtual ~TCPClient();

public:
	void SetRecvCB(ClientRecvCB cb, void* userdata);
	void SetCloseCB(TcpCloseCB cb, void* userdata);
	void SetConnectCB(ConnectCB cb, void* userdata);
	bool Connect(const char* ip, int port);
	int Send(const char* data, size_t len);
	void Close();
	bool IsClosed() { return _is_closed; }
	bool SetNoDelay(bool enable);
	bool SetKeepAlive(int enable, unsigned int delay);
	const char* GetLastErrMsg() const { return _err_msg.c_str(); }


protected:
	static void ConnectThread(void* arg);
	static void OnConnect(uv_connect_t* handle, int status);
	static void OnRecv(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf);
	static void OnSend(uv_write_t* req, int status);
	static void AllocBufferForRecv(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
	static void OnClientClose(uv_handle_t* handle);
	static void AsyncCB(uv_async_t* handle);
	static void CloseWalkCB(uv_handle_t* handle, void* arg);
	static void GetPacket(const NetPacket& packet_head, const unsigned char* data, void* userdata);
	static void ReconnectTimer(uv_timer_t* handle);

private:
	bool _init();
	void _close();
	bool _run(int status = UV_RUN_DEFAULT);
	void _send(uv_write_t* req = NULL);
	bool _start_reconnect();
	void _stop_reconnect();
	ClientWriteParam* _fetch_one_param();
	void _recycle_one_param(ClientWriteParam* param);


private:
	enum
	{
		CONNECT_TIMEOUT,
		CONNECT_FINISH,
		CONNECT_ERROR,
		CONNECT_DIS,
	};
	
	TcpClientCtx*		_ctx;
	uv_async_t		_async_handle;
	uv_loop_t		_loop;
	bool			_is_closed;
	bool			_is_user_closed;
	uv_thread_t		_connect_thread_handle;
	uv_connect_t		_connect_req;
	int			_connect_status;
	
	PodCircularBuffer<char> _write_circularbuf;
	uv_mutex_t _mutex_writebuf;

	std::list<ClientWriteParam *> _avail_params;	// available write param list
	uv_mutex_t _mutex_params;

	ClientRecvCB	_recv_cb;
	void*			_recv_userdata;

	TcpCloseCB		_close_cb;
	void*			_close_userdata;

	ConnectCB		_connect_cb;
	void*			_connect_userdata;

	uv_timer_t		_connect_timer;
	bool			_is_connecting;
	int64_t			_repeat_time;

	std::string		_connect_ip;
	int			_connect_port;
	std::string		_err_msg;

	unsigned char	_packet_head;
	unsigned char	_packet_tail;
};

}	// end of namespace UVNET


#endif	// _TCPCLIENT_H
