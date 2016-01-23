#ifndef _TCPSERVER_H
#define _TCPSERVER_H
#include <string>
#include <map>
#include <list>
#include <vector>
#include "uv.h"
#include "log.h"
//#include "tcpserverprotocolprocess.h"
#include "protocol.h"
#include "net_base.h"
#include "packet_sync.h"

#ifndef BUFFER_SIZE
#define BUFFER_SIZE (1024 * 10)
#endif
namespace UVNET
{
class Session;
struct SessionCtx
{
	uv_tcp_t tcp_handle;		// tcp handle
	PacketSync* packet;			// packet
	uv_buf_t read_buf;
	int sid;					// session id
	char client_ip[20];			// ip 这样不是一个好习惯，即便记录也应该是记录整数值的ip TODO: 需要修改
	void *parent_server;		// tcp server
	void *parent_session;		// session

	static SessionCtx* Alloc(void *parent_server);
	static void Release(SessionCtx* ctx);
};

struct	WriteParam
{
	uv_write_t write_req;
	uv_buf_t buf;
	int buf_true_len;

	static WriteParam* Alloc();
	static void Release(WriteParam* param);
};

class TCPServer
{
public:
	TCPServer(unsigned char pack_head, unsigned char pack_tail);
	virtual ~TCPServer();
	// Start/Stop the log
	static bool StartLog(LogLevel log_level, const char* module_name, const char* log_dir);
	// static void StopLog();

	void SetNewConnectCB(NewConnectCB cb, void *userdata);
	void SetRecvCB(int sid, ServerRecvCB cb, void *userdata);
	void SetCloseCB(TcpCloseCB cb, void *userdata);
	// void SetProtocol(TCPServerProtocolProcess* proto);

	bool Start(const char* ip, int port);
	void Close();
	bool IsClosed() { return _is_closed; }
	bool SetNoDelay(bool enable);
	bool SetKeepAlive(int enable, unsigned int delay);
	const char* GetLastErrMsg() const { return _err_msg.c_str(); }
	void AddProtocol(int proto_id, Protocol* proto);
	void RemoveProtocol(int proto_id);
	Protocol* GetProtocol(int proto_id);
	bool _send(const std::string& data, SessionCtx* ctx);


protected:
	static int GenerateSessionID();
	
	static void AfterServerClose(uv_handle_t* handle);
	static void DeleteTcpHandle(uv_handle_t* handle);
	static void RecycleSessionCtx(uv_handle_t* handle);
	static void OnConnection(uv_stream_t* server, int status);
	static void SessionClosed(int sid, void *userdata);
	static void AsyncCloseCB(uv_async_t* handle);
	static void CloseWalkCB(uv_handle_t* handle, void *arg);

private:
	bool _init();
	void _close();
	bool _run(int status = UV_RUN_DEFAULT);
	bool _bind(const char* ip, int port);
	bool _listen(int backlog = SOMAXCONN);
	//bool _send(const std::string& data, SessionCtx* ctx);
	bool _broadcast(const std::string& data, std::vector<int> exclude_ids);
	static void _start_thread(void *arg);
	SessionCtx* _fetch_one_ctx(TCPServer* server);
	void _recycle_one_ctx(SessionCtx* ctx);
	WriteParam* _fetch_one_param();
	void _recycle_one_param(WriteParam* param);

private:
	enum 
	{
		START_TIMEOUT,
		START_FINISH,
		START_ERROR,
		START_DIS,
	};
	
	uv_loop_t	_loop;														
	uv_tcp_t	_tcp_handle;												
	uv_async_t	_async_close_handle;			
	bool		_is_closed;					
	bool		_is_user_closed;

	std::map<int/*session id*/, Session* /*session*/> _sessions;
	uv_mutex_t	_mutex_sessions;

	// TCPServerProtocolProcess*	_protocol;
	std::map<int/*protocol id*/, Protocol*> _protocols;
	uv_thread_t	_start_thread_handle;
	int			_start_status;
	std::string	_err_msg;

	NewConnectCB	_new_conn_cb;
	void*		_new_conn_userdata;

	TcpCloseCB		_close_cb;
	void*		_close_userdata;

	std::string	_server_ip;
	int			_server_port;

	unsigned char	_pack_head;
	unsigned char	_pack_tail;

	std::list<SessionCtx*>	_avail_ctxs;
	uv_mutex_t	_mutex_ctxs;

	std::list<WriteParam*>	_avail_params;
	uv_mutex_t	_mutex_params;

public:
	friend void AllocBufferForRecv(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
	friend void OnRecv(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf);
	friend void OnSend(uv_write_t *req, int status);
	friend void GetPacket(const NetPacket& packethead, const char *packetdata, void *userdata);
};

// TODO: 其实这个Session类比较累赘，可以考虑干掉，只用SessionCtx即可
class Session
{
public:
	Session(SessionCtx* ctx, uv_loop_t* loop);
	virtual ~Session();

	void SetRecvCB(ServerRecvCB cb, void* userdata);
	void SetCloseCB(TcpCloseCB cb, void* userdata);
	SessionCtx* GetCtx() const;
	void Close();
	const char* GetLastErrMsg()const { return _err_msg.c_str(); }

private:
	bool _init();
	static void _session_close(uv_handle_t* handle);

private:
	uv_loop_t*	_loop;
	SessionCtx* _ctx;
	bool		_is_closed;
	std::string _err_msg;

	ServerRecvCB _recv_cb;
	void*		_recv_userdata;

	TcpCloseCB	_close_cb;
	void*		_close_userdata;

public:
	friend void AllocBufferForRecv(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
	friend void OnRecv(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf);
	friend void OnSend(uv_write_t *req, int status);
	friend void GetPacket(const NetPacket& packethead, const char *packetdata, void *userdata);
};

// Global Function
void AllocBufferForRecv(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
void OnRecv(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf);
void OnSend(uv_write_t* req, int status);
void GetPacket(const NetPacket& packethead, const char* packetdata, void* userdata);

}	// end of namespace UVNET

#endif
