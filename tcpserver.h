#ifndef TCPSERVER_H
#define TCPSERVER_H
#include <string>
#include <map>
#include <list>
#include "uv.h"

namespace UVNET
{
class Session;
typedef struct _session_ctx
{
	uv_tcp_t tcp_handle;		// tcp handle
	PacketSync* packet;			// packet
	uv_buf_t read_buf;
	int sid;					// session id
	void *parent_server;		// tcp server
	void *parent_session;		// session

	static _session_ctx* Alloc(void *parent_server);
	static void Release(_session_ctx* ctx);
}SessionCtx;

typedef struct	_write_param
{
	uv_write_t write_req;
	uv_buf_t buf;
	int buf_true_len;

	static _write_param* Alloc();
	static void Release(_write_param* param);
}WriteParam;

class TCPServer
{
public:
	TCPServer(unsigned char pack_head, unsigned char pack_tail);
	virtual ~TCPServer();
	// Start/Stop the log
	static StartLog(const char * log_path = nullptr);
	static void StopLog();

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

protected:
	virtual int GenerateSessionID() const;
	
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
	bool _send(const std::string& data, SessionCtx* ctx);
	bool _broadcast(const std::string& data, std::vector<int> exclude_ids);
	static _start_thread(void *arg);
	SessionCtx* _fetch_one_ctx();
	void _recycle_one_ctx(SessionCtx* ctx);

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
	friend static void AllocBufferForRecv(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
	friend static void OnRecv(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf);
	friend static void OnSend(uv_write_t *req, int status);
	friend static void GetPacket(const NetPacket& packethead, const unsigned char *packetdata, void *userdata);
};

}	// end of namespace UVNET

#endif