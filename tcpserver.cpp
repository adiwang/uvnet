#include "tcpserver.h"
#include "log.h"
#include "net_base.h"
#include "pb/netmessage.pb.h"

#include <cassert>
#include <arpa/inet.h>
#define MAXLISTSIZE 20

namespace UVNET
{

/*********************************************** TCPServer *****************************************************/
TCPServer::TCPServer(unsigned char pack_head, unsigned char pack_tail)
	: _pack_head(pack_head), _pack_tail(pack_tail)
	, _new_conn_cb(nullptr), _new_conn_userdata(nullptr), _close_cb(nullptr), _close_userdata(nullptr)
	, _is_closed(true), _is_user_closed(false)
	, _start_status(START_DIS)
{
	int iret = uv_loop_init(&_loop);
	if(iret)
	{
		_err_msg = GetUVError(iret);
		LOG_ERROR("%s|init loop failed|%s", __FUNCTION__, _err_msg.c_str());
	}
	iret = uv_mutex_init(&_mutex_sessions);
	if(iret)
	{
		_err_msg = GetUVError(iret);
		LOG_ERROR("%s|init sessions mutex failed|%s", __FUNCTION__, _err_msg.c_str());
	}
	iret = uv_mutex_init(&_mutex_ctxs);
	if(iret)
	{
		_err_msg = GetUVError(iret);
		LOG_ERROR("%s|init session ctx mutex failed|%s", __FUNCTION__, _err_msg.c_str());
	}
	iret = uv_mutex_init(&_mutex_params);
	if(iret)
	{
		_err_msg = GetUVError(iret);
		LOG_ERROR("%s|init write param mutex failed|%s", __FUNCTION__, _err_msg.c_str());
	}
}

TCPServer::~TCPServer()
{
	Close();
	uv_thread_join(&_start_thread_handle);
	uv_mutex_destroy(&_mutex_sessions);
	uv_mutex_destroy(&_mutex_ctxs);
	uv_mutex_destroy(&_mutex_params);
	uv_loop_close(&_loop);
	for(std::list<SessionCtx*>::iterator it = _avail_ctxs.begin(); it != _avail_ctxs.end(); ++it)
	{
		SessionCtx::Release(*it);
	}
	_avail_ctxs.clear();
	
	for(std::list<WriteParam*>::iterator it = _avail_params.begin(); it != _avail_params.end(); ++it)
	{
		WriteParam::Release(*it);
	}
	_avail_params.clear();

	for(std::map<int, Protocol*>::iterator it = _protocols.begin(); it != _protocols.end(); ++it)
	{
		delete it->second;
	}
	_protocols.clear();

	LOG_TRACE("tcp server exit");
}

/**
**	初始化函数
**	@return: true, 成功; false, 失败
*/
bool TCPServer::_init()
{
	if(!_is_closed)	return true;
	int iret = uv_async_init(&_loop, &_async_close_handle, AsyncCloseCB);
	if(iret)
	{
		_err_msg = GetUVError(iret);
		LOG_ERROR("%s|init async close handle failed|%s", __FUNCTION__, _err_msg.c_str());
		return false;
	}
	_async_close_handle.data = this;

	iret = uv_tcp_init(&_loop, &_tcp_handle);
	if(iret)
	{
		_err_msg = GetUVError(iret);
		LOG_ERROR("%s|init tcp handle failed|%s", __FUNCTION__, _err_msg.c_str());
		return false;
	}
	_tcp_handle.data = this;

	iret = uv_tcp_nodelay(&_tcp_handle, 1);
	if(iret)
	{
		_err_msg = GetUVError(iret);
		LOG_ERROR("%s|set tcp handle nodelay failed|%s", __FUNCTION__, _err_msg.c_str());
		return false;
	}
	_is_closed = false;
	return true;
}

/**
**	真正的关闭函数
**	关闭同server连接的每个session并关闭ioloop中的每个handle
*/
void TCPServer::_close()
{
	if(_is_closed) return;
	uv_mutex_lock(&_mutex_sessions);
	for(std::map<int, Session*>::iterator it = _sessions.begin(); it != _sessions.end(); ++it)
	{
		Session* session = it->second;
		session->Close();
	}
	uv_mutex_unlock(&_mutex_sessions);
	uv_walk(&_loop, CloseWalkCB, this);
	LOG_TRACE("close server");
}

/**
**	启动事件循环
**	@status: 运行模式
**	@return: true,成功; false, 失败
*/
bool TCPServer::_run(int status)
{
	int iret = uv_run(&_loop, (uv_run_mode)status);
	if(iret)
	{
		_err_msg = GetUVError(iret);
		LOG_ERROR("%s|run loop failed|%s", __FUNCTION__, _err_msg.c_str());
		return false;
	}
	LOG_TRACE("server runing");
	return true;
}

/**
**	绑定ip和端口
**	@ip: ip地址
**	@port: 端口
**  @return: true, 绑定成功; false, 绑定失败
*/
bool TCPServer::_bind(const char *ip, int port)
{
	struct sockaddr_in bind_addr;
	int iret = uv_ip4_addr(ip, port, &bind_addr);
	if(iret)
	{
		_err_msg = GetUVError(iret);
		LOG_ERROR("%s|ip or port error|%s|%s|%d", __FUNCTION__, _err_msg.c_str(), ip, port);
		return false;
	}
	iret = uv_tcp_bind(&_tcp_handle, (const struct sockaddr*)&bind_addr, 0);
	if(iret)
	{
		_err_msg = GetUVError(iret);
		LOG_ERROR("%s|bind failed|%s|%s|%d", __FUNCTION__, _err_msg.c_str(), ip, port);
		return false;
	}
	LOG_TRACE("server bind ip=%s, port=%d", ip, port);
	return true;
}

/**
**	监听函数
**	@backlog: 最大队列连接数
**  @return: true, 监听成功; false, 监听失败
*/
bool TCPServer::_listen(int backlog)
{
	int iret = uv_listen((uv_stream_t*)&_tcp_handle, backlog, OnConnection);
	if(iret)
	{
		_err_msg = GetUVError(iret);
		LOG_ERROR("%s|listen failed|%s", __FUNCTION__, _err_msg.c_str());
		return false;
	}
	LOG_TRACE("server start listening");
	return true;
}

/**
**	启动server
**	@ip: ip
**	@port: 端口
**  @return: true, 启动成功; false, 启动失败
*/
bool TCPServer::Start(const char* ip, int port)
{
	_server_ip = ip;
	_server_port = port;
	// 先调用_close确保server是关闭的
	_close();
	if(!_init()) return false;
	if(!_bind(ip, port)) return false;
	if(!_listen()) return false;
	// 单独创建一个启动线程用来启动事件循环
	int iret = uv_thread_create(&_start_thread_handle, _start_thread, this);
	if(iret)
	{
		_err_msg = GetUVError(iret);
		LOG_ERROR("%s|create start thread failed|%s", __FUNCTION__, _err_msg.c_str());
		return false;
	}
	// 等待启动是否成功，如果启动成功会在启动线程中将_start_status设为START_FINISH
	int retry_count = 0;
	while(_start_status == START_DIS)
	{
		ThreadSleep(100);
		if(++retry_count > 100)
		{
			_start_status = START_TIMEOUT;
			break;
		}
	}
	return _start_status == START_FINISH;
}

/**
**	关闭server
*/
void TCPServer::Close()
{
	if(_is_closed)	return;
	_is_user_closed = true;
	uv_async_send(&_async_close_handle);
}

/**
**	开启或关闭 Nagle 算法，在服务启动后方可调用
**  @enable: 1是启动，0是禁止
**  @return: true, 设置成功; false, 设置失败
*/
bool TCPServer::SetNoDelay(bool enable)
{
	int iret = uv_tcp_nodelay(&_tcp_handle, enable ? 1 : 0);
	if(iret)
	{
		_err_msg = GetUVError(iret);
		LOG_ERROR("%s|set tcp no delay failed|%s", __FUNCTION__, _err_msg.c_str());
		return false;
	}
	return true;
}

/**
**	开启或关闭 KeepAlive，在服务启动后方可调用
**  @enable: 1是启动，0是禁止
**  @delay: 延时多少秒发送KeepAlive，当enable为0时忽略此值
**  @return: true, 设置成功; false, 设置失败
*/
bool TCPServer::SetKeepAlive(int enable, unsigned int delay)
{
	int iret = uv_tcp_keepalive(&_tcp_handle, enable, delay);
	if(iret)
	{
		_err_msg = GetUVError(iret);
		LOG_ERROR("%s|set tcp no delay failed|%s", __FUNCTION__, _err_msg.c_str());
		return false;
	}
	return true;
}

/**
**	设置server的处理协议
**  @proto: 协议处理实例
*/
/*
void TCPServer::SetProtocol(TCPServerProtocolProcess* proto)
{
	_protocol = proto;
}
*/

/**
**	添加server的处理协议
**	@proto_id: 协议id
**  @proto: 协议处理实例
*/
void TCPServer::AddProtocol(int proto_id, Protocol* proto)
{
	if(proto_id > 0 && proto)
	{
		_protocols.insert(std::make_pair(proto_id, proto));	
	}
}

/**
**	删除协议
**	@proto_id: 协议id
*/
void TCPServer::RemoveProtocol(int protocol_id)
{
	if(protocol_id > 0)
	{
		std::map<int, Protocol*>::iterator it = _protocols.find(protocol_id);
		if(it != _protocols.end())
		{
			delete it->second;
			_protocols.erase(it);
		}
	}	
}

/**
**	查找指定协议
**	@proto_id: 协议id
**	@return: 返回找到的协议, NULL为没找到
*/
Protocol* TCPServer::GetProtocol(int proto_id)
{
	if(proto_id <= 0) return NULL;
	std::map<int, Protocol*>::iterator it = _protocols.find(proto_id);
	if(it != _protocols.end())
	{
		return it->second;
	}
	return NULL;
}

/**
**	线程函数
**  @arg: TCPServer类型指针
*/
void TCPServer::_start_thread(void *arg)
{
	TCPServer *instance = (TCPServer *)arg;
	instance->_start_status = START_FINISH;
	instance->_run();
	// 当运行到下面时说明已经close
	instance->_is_closed = true;
	instance->_is_user_closed = false;
	LOG_TRACE("server had closed");
	if(instance->_close_cb)
	{
		instance->_close_cb(-1, instance->_close_userdata);
	}
}

/**
**	设置连接回调函数
**  @cb: 连接回调函数
**  @userdata: 连接回调参数
*/
void TCPServer::SetNewConnectCB(NewConnectCB cb, void *userdata)
{
	_new_conn_cb = cb;
	_new_conn_userdata = userdata;
}

/**
**	设置关闭回调函数
**  @cb: 关闭回调函数
**  @userdata: 关闭回调参数
*/
void TCPServer::SetCloseCB(TcpCloseCB cb, void *userdata)
{
	_close_cb = cb;
	_close_userdata = userdata;
}

/**
**	设置指定session的接收数据回调函数
**	@sid: session id
**  @cb: 接收数据回调函数
**  @userdata: 接收数据回调参数
*/
void TCPServer::SetRecvCB(int sid, ServerRecvCB cb, void *userdata)
{
	uv_mutex_lock(&_mutex_sessions);
	std::map<int, Session*>::iterator it = _sessions.find(sid);
	if(it != _sessions.end())
	{
		it->second->SetRecvCB(cb, userdata);
	}
	uv_mutex_unlock(&_mutex_sessions);
}

/**
**	生成session id
**  @return: session id
*/
int TCPServer::GenerateSessionID()
{
	static int s_id = 0;
	return ++s_id;
}

/**
**	服务器关闭回调函数
**	@handle: data域中存放着TCPServer*
*/
void TCPServer::AfterServerClose(uv_handle_t* handle)
{
}

/**
**	删除TCP连接
**	@handle: data域中存放着SessionCtx*
*/
void TCPServer::DeleteTcpHandle(uv_handle_t* handle)
{
	SessionCtx* ctx = (SessionCtx *)handle->data;
	SessionCtx::Release(ctx);
}

/**
**	回收SessionCtx
**	@handle: data域中存放着SessionCtx*
*/
void TCPServer::RecycleSessionCtx(uv_handle_t* handle)
{
	SessionCtx* ctx = (SessionCtx *)handle->data;
	assert(ctx);
	TCPServer* server = (TCPServer *)ctx->parent_server;
	server->_recycle_one_ctx(ctx);
}

/**
**	接收连接回调函数, 在_listen中调用uv_listen设置
**	@server: data域中存放着TCPServer*
**  @status: 状态异常码
*/
void TCPServer::OnConnection(uv_stream_t* server, int status)
{
	TCPServer* server_instance = (TCPServer *)server->data;
	assert(server_instance);
	if(status)
	{
		server_instance->_err_msg = GetUVError(status);
		LOG_ERROR("%s|on connection status abnormal|%s|%d", __FUNCTION__, server_instance->_err_msg.c_str(), status);
		return;
	}
	
	SessionCtx* ctx = server_instance->_fetch_one_ctx(server_instance);	
	int iret = uv_tcp_init(&server_instance->_loop, &ctx->tcp_handle);
	if(iret)
	{
		server_instance->_recycle_one_ctx(ctx);
		server_instance->_err_msg = GetUVError(iret);
		LOG_ERROR("%s|on connection init new tcp handle failed|%s", __FUNCTION__, server_instance->_err_msg.c_str());
		return;
	}
	ctx->tcp_handle.data = ctx;

	int sid = server_instance->GenerateSessionID();
	ctx->sid = sid;
	iret = uv_accept((uv_stream_t *)server, (uv_stream_t *)&ctx->tcp_handle);
	if(iret)
	{
		server_instance->_recycle_one_ctx(ctx);
		server_instance->_err_msg = GetUVError(iret);
		LOG_ERROR("%s|accept failed|%s", __FUNCTION__, server_instance->_err_msg.c_str());
		return;
	}

	// get ip
	struct sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);
	uv_tcp_getsockname(&ctx->tcp_handle, (struct sockaddr *)&client_addr, &client_addr_len);
	inet_ntop(AF_INET, &client_addr.sin_addr, ctx->client_ip, sizeof(ctx->client_ip));

	ctx->packet->SetPacketCB(GetPacket, ctx);
	ctx->packet->Start(server_instance->_pack_head, server_instance->_pack_tail);
	iret = uv_read_start((uv_stream_t *)&ctx->tcp_handle, AllocBufferForRecv, OnRecv);
	if(iret)
	{
		uv_close((uv_handle_t *)&ctx->tcp_handle, TCPServer::RecycleSessionCtx);
		server_instance->_err_msg = GetUVError(iret);
		LOG_ERROR("%s|start read failed|%s", __FUNCTION__, server_instance->_err_msg.c_str());
		return;
	}

	Session *session = new Session(ctx, &server_instance->_loop);
	session->SetCloseCB(TCPServer::SessionClosed, server_instance);
	
	uv_mutex_lock(&server_instance->_mutex_sessions);
	server_instance->_sessions.insert(std::make_pair(sid, session));
	uv_mutex_unlock(&server_instance->_mutex_sessions);

	if(server_instance->_new_conn_cb)
	{
		server_instance->_new_conn_cb(sid, server_instance->_new_conn_userdata);
	}

	LOG_TRACE("new connect client|%d|%s", sid, ctx->client_ip);
}

/**
**	回收一个SessionCtx
**	@ctx: 待回收的ctx
*/
void TCPServer::_recycle_one_ctx(SessionCtx* ctx)
{
	// 如果空闲列表超限，则释放掉，否则加到空闲列表中
	uv_mutex_lock(&_mutex_ctxs);
	if(_avail_ctxs.size() > MAXLISTSIZE)
	{
		SessionCtx::Release(ctx);
	}
	else
	{
		_avail_ctxs.push_back(ctx);
	}
	uv_mutex_unlock(&_mutex_ctxs);
}

/**
**	获取一个SessionCtx
**	@return: 获取的ctx
*/
SessionCtx* TCPServer::_fetch_one_ctx(TCPServer* server)
{
	// 如果空闲列表为空，则分配一个，否则取空闲列表的第一个
	SessionCtx* ctx = NULL;
	uv_mutex_lock(&_mutex_ctxs);
	if(_avail_ctxs.empty())
	{
		ctx = SessionCtx::Alloc(server);
	}
	else
	{
		ctx = _avail_ctxs.front();
		_avail_ctxs.pop_front();
		ctx->parent_session = NULL;
	}
	uv_mutex_unlock(&_mutex_ctxs);
	return ctx;
}

/**
**	获取一个WriteParam
**	@return: 获取的param
*/
WriteParam* TCPServer::_fetch_one_param()
{
	WriteParam* param = NULL;
	uv_mutex_lock(&_mutex_params);
	if(_avail_params.empty())
	{
		param = WriteParam::Alloc();
	}
	else
	{
		param = _avail_params.front();
		_avail_params.pop_front();
	}
	uv_mutex_unlock(&_mutex_params);
	return param;
}

/**
**	回收一个WriteParam
**	@ctx: 待回收的param
*/
void TCPServer::_recycle_one_param(WriteParam* param)
{
	uv_mutex_lock(&_mutex_params);
	if(_avail_params.size() > MAXLISTSIZE)
	{
		WriteParam::Release(param);
	}
	else
	{
		_avail_params.push_back(param);
	}
	uv_mutex_unlock(&_mutex_params);
}

/**
**	客户端断开连接回调函数
**  @sid: session id
**	@userdata: TCPServer
*/
void TCPServer::SessionClosed(int sid, void *userdata)
{
	TCPServer *server = (TCPServer *)userdata;
	uv_mutex_lock(&server->_mutex_sessions);
	std::map<int, Session*>::iterator it = server->_sessions.find(sid);
	if(it != server->_sessions.end())
	{
		if(server->_close_cb) server->_close_cb(sid, server->_close_userdata);
		server->_recycle_one_ctx(it->second->GetCtx());

		LOG_TRACE("session closed|%d|%s", sid, it->second->GetCtx()->client_ip);
		delete it->second;
		server->_sessions.erase(it);
	}
	uv_mutex_unlock(&server->_mutex_sessions);
}

/**
**	服务端异步关闭回调函数
**  @handle: data中存放TCPServer
*/
void TCPServer::AsyncCloseCB(uv_async_t* handle)
{
	TCPServer *server = (TCPServer *)handle->data;
	if(server->_is_user_closed) server->_close();
}

/**
**	walk遍历关闭回调，关闭每一个handle
**  @handle: 待关闭的handle
**  @arg: TCPServer
*/
void TCPServer::CloseWalkCB(uv_handle_t* handle, void *arg)
{
	TCPServer *server = (TCPServer *)arg;
	if(!uv_is_closing(handle)) uv_close(handle, AfterServerClose);
}

/**
**	真正的发送数据的函数
**  @data: 待发送的数据
**  @ctx: 连接session的上下文
**	@return: true, 发送成功; false, 发送失败
*/
bool TCPServer::_send(const std::string& data, SessionCtx* ctx)
{
	if(data.empty())
	{
		LOG_TRACE("send data is empty|%d", ctx->sid);
		return true;
	}
	// 分配一个WriteParam，如果buf空间小于数据长度则重新分配buf
	WriteParam* param = NULL;
	param = _fetch_one_param();
	if(param->buf_true_len < data.length())
	{
		param->buf.base = (char *)realloc(param->buf.base, data.length());
		param->buf_true_len = data.length();
	}
	memcpy(param->buf.base, data.data(), data.length());
	param->buf.len = data.length();
	param->write_req.data = ctx;
	int iret = uv_write((uv_write_t *)&param->write_req, (uv_stream_t *)&ctx->tcp_handle, &param->buf, 1, OnSend);
	if(iret)
	{
		_recycle_one_param(param);
		_err_msg = GetUVError(iret);
		LOG_ERROR("%s|send data failed|%s|%d", __FUNCTION__, _err_msg.c_str(), ctx->sid);
		return false;
	}
	return true;
}

/**
**	调用Clog的init
**  @log_level: 日志级别
**  @module_name: 模块名
**	@log_dir: 日志目录
**  @return: true, 成功; false, 失败
*/
bool TCPServer::StartLog(LogLevel log_level, const char* module_name, const char* log_dir)
{
	return log_init(log_level, module_name, log_dir);
}

/**
**	广播
**  @data: 要发送的数据
**  @exclude_ids: 要排除的session id列表
**  @return: true, 成功; false, 失败
*/
bool TCPServer::_broadcast(const std::string& data, std::vector<int> exclude_ids)
{
	if(data.empty())
	{
		LOG_TRACE("broadcast data is empty");
		return true;
	}
	
	uv_mutex_lock(&_mutex_sessions);
	Session *session = NULL;
	WriteParam *param = NULL;
	if(exclude_ids.empty())
	{
		for(std::map<int, Session*>::iterator it = _sessions.begin(); it != _sessions.end(); ++it)
		{
			session = it->second;
			_send(data, session->GetCtx());
		}
	}
	else
	{
		for(std::map<int, Session*>::iterator it = _sessions.begin(); it != _sessions.end(); ++it)
		{
			std::vector<int>::iterator find_it = std::find(exclude_ids.begin(), exclude_ids.end(), it->first);
			if(find_it != exclude_ids.end())
			{
				exclude_ids.erase(find_it);
				continue;
			}
			session = it->second;
			_send(data, session->GetCtx());
		}
	}
	uv_mutex_unlock(&_mutex_sessions);
	return true;
}

/***************************************** Session *******************************************************/
Session::Session(SessionCtx* ctx, uv_loop_t* loop)
	: _ctx(ctx), _loop(loop), _is_closed(true), _recv_cb(nullptr), _recv_userdata(nullptr), _close_cb(nullptr), _close_userdata(nullptr)
{
	_init();
}

Session::~Session()
{
	Close();
	while(!_is_closed)
	{
		ThreadSleep(10);
	}
}

/**
**	session的初始化函数
**	@return: true, 成功; false, 失败
*/
bool Session::_init()
{
	if(!_is_closed)	return true;
	_ctx->parent_session = this;
	_is_closed = false;
	return true;
}

/**
**	session的真正关闭函数
**	在Close的uv_close中设置
**	@handle: data中存放着Session*
*/
void Session::_session_close(uv_handle_t* handle)
{
	Session* session = (Session *)handle->data;
	assert(session);
	if(handle == (uv_handle_t *)&session->GetCtx()->tcp_handle)
	{
		session->_is_closed = true;
		LOG_TRACE("session closed|%d", session->GetCtx()->sid);
		if(session->_close_cb)	session->_close_cb(session->GetCtx()->sid, session->_close_userdata);
	}
}

/**
**	设置接收数据的回调函数
**	@cb: 回调函数
**	@userdata: 参数
*/
void Session::SetRecvCB(ServerRecvCB cb, void* userdata)
{
	_recv_cb = cb;
	_recv_userdata = userdata;
}

/**
**	设置关闭连接的回调函数
**	@cb: 回调函数
**	@userdata: 参数
*/
void Session::SetCloseCB(TcpCloseCB cb, void* userdata)
{
	_close_cb = cb;
	_close_userdata = userdata;
}

/**
**	获取sesscion上下文
**	@return: SessionCtx
*/
SessionCtx* Session::GetCtx() const
{
	return _ctx;
}

/**
**	session的关闭函数
*/
void Session::Close()
{
	if(_is_closed)	return;
	_ctx->tcp_handle.data = this;
	uv_close((uv_handle_t *)&_ctx->tcp_handle, _session_close);
}

/*********************************************** SessionCtx *****************************************************/

/**
**	分配SessionCtx
**	@parent_server: 所属TCPServer
**	@return: SessionCtx
*/
SessionCtx* SessionCtx::Alloc(void *parent_server)
{
	SessionCtx* ctx = (SessionCtx *)malloc(sizeof(SessionCtx));
	ctx->packet = new PacketSync;
	ctx->read_buf.base = (char *)malloc(BUFFER_SIZE);
	ctx->read_buf.len = BUFFER_SIZE;
	memset(ctx->client_ip, 0, sizeof(ctx->client_ip));
	ctx->parent_server = parent_server;
	ctx->parent_session = NULL;
	return ctx;
}

/**
**	释放SessionCtx
**	@ctx: 待释放的SessionCtx
*/
void SessionCtx::Release(SessionCtx* ctx)
{
	delete ctx->packet;
	free(ctx->read_buf.base);
	free(ctx);
}

/*********************************************** WriteParam *****************************************************/

/**
**	分配WriteParam
**	@return: WriteParam
*/
WriteParam* WriteParam::Alloc()
{
	WriteParam* param = (WriteParam *)malloc(sizeof(WriteParam));
	param->buf.base = (char *)malloc(BUFFER_SIZE);
	param->buf.len = BUFFER_SIZE;
	param->buf_true_len = BUFFER_SIZE;
	return param;
}

/**
**	释放WriteParam
**	@ctx: 待释放的param
*/
void WriteParam::Release(WriteParam* param)
{
	free(param->buf.base);
	free(param);
}

/************************************************ Global Function ***********************************************/

/**
**	uv_read的分配回调函数
**	@handle: data存放SessionCtx
**	@suggested_size: 建议大小
**	@buf: 分配缓冲区
*/
void AllocBufferForRecv(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
	SessionCtx* ctx = (SessionCtx *)handle->data;
	assert(ctx);
	*buf = ctx->read_buf;
}

/**
**	接收数据回调函数，同样是uv_read中设置的
**	@handle: data中存放的是SessionCtx
**  @nread: 接收到的数据大小
**	@buf: 接收数据的缓冲区
*/
void OnRecv(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
{
	SessionCtx* ctx = (SessionCtx *)handle->data;
	assert(ctx);
	if(nread < 0)
	{
		if(nread == UV_EOF)
		{
			LOG_ERROR("client recv eof|%d", ctx->sid);
		}
		else if(nread == UV_ECONNRESET)
		{
			LOG_ERROR("client recv conn reset|%d", ctx->sid);
		}
		else
		{
			LOG_ERROR("client recv error|%d|%s", ctx->sid, GetUVError(nread).c_str());
		}
		Session* session = (Session *)ctx->parent_session;
		session->Close();
		return;
	}
	else if (nread == 0)
	{
		// everything ok
	}
	else
	{
		// 正常接收，调用解包函数来解包
		ctx->packet->recvdata((const unsigned char *)buf->base, nread);
	}
}

/**
**	发送数据回调函数，_send()函数中调用uv_write设置的
**	@req: data中存放的是SessionCtx
**  @status: 发送的状态
*/
void OnSend(uv_write_t* req, int status)
{
	SessionCtx* ctx = (SessionCtx *)req->data;
	TCPServer* server = (TCPServer *)ctx->parent_server;
	// 完成发送后回收WriteParam
	server->_recycle_one_param((WriteParam *)req);
	if(status < 0)
	{
		LOG_ERROR("send data error|%d", ctx->sid);
	}
}

/**
**	当接收到一个完整的数据帧时的回调函数
**	@packethead: 包头，定义在net_base中
**  @packetdata: 真实的数据
**	@userdata:	SessionCtx
*/
void GetPacket(const NetPacket& packethead, const char* packetdata, void* userdata)
{
	assert(userdata);
	SessionCtx* ctx = (SessionCtx *)userdata;
	TCPServer* server = (TCPServer*)ctx->parent_server;
	
	unsigned int proto_id = 0;
	const char* proto_data = NULL;
	int data_size = 0;
	if(packethead.type == 1)
	{
		// 采用protobuf解析协议
		CProto proto;
		// proto.ParseFromArray(packetdata, packethead.datalen);
		proto.ParseFromString(std::string(packetdata, packethead.datalen));
		proto_id = proto.id();
		proto_data = proto.body().c_str();
		data_size = proto.body().size();
	}
	else if(packethead.type == 2)
	{
		// 手动解析协议
		// 前4个字节是协议id，后面是序列化后的协议内容
		if(!CharToInt32(packetdata, proto_id)) return;
		proto_data = &packetdata[4];
		data_size = packethead.datalen - 4;
	}
	else
	{
		// 无效协议
	}
	if(proto_id > 0)
	{
		Protocol* proto_handle = NULL;
		proto_handle = server->GetProtocol(proto_id);
		if(proto_handle)
		{
			// 调用协议来解析数据包并返回相应的response
			const std::string& send_data = proto_handle->Process(proto_data, data_size);
			server->_send(send_data, ctx);
		}
	}
}

}	// end of namespace UVNET
