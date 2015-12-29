#include "tcpserver.h"

#include <cassert>
#include <arpa/inet.h>
#define MAXLISTSIZE 20

namespace UVNET
{

TCPServer::TCPServer(unsigned char pack_head, unsigned char pack_tail)
	: _pack_head(pack_head), _pack_tail(pack_tail)
	, _new_conn_cb(nullptr), _new_conn_userdata(nullptr), _close_cb(nullptr), _close_userdata(nullptr)
	, _is_closed(true), _is_user_closed(false)
	, _start_status(START_DIS)/*, _protocol(NULL)*/
{
	int iret = uv_loop_init(&_loop);
	if(iret)
	{
		_err_msg = GetUVError(iret);
		log_error("%s|init loop failed|%s", __FUNCTION__, _err_msg.c_str());
	}
	iret = uv_mutex_init(&_mutex_sessions);
	if(iret)
	{
		_err_msg = GetUVError(iret);
		log_error("%s|init sessions mutex failed|%s", __FUNCTION__, _err_msg.c_str());
	}
	iret = uv_mutex_init(&_mutex_ctxs);
	if(iret)
	{
		_err_msg = GetUVError(iret);
		log_error("%s|init session ctx mutex failed|%s", __FUNCTION__, _err_msg.c_str());
	}
	iret = uv_mutex_init(&_mutex_params);
	if(iret)
	{
		_err_msg = GetUVError(iret);
		log_error("%s|init write param mutex failed|%s", __FUNCTION__, _err_msg.c_str());
	}
}

TCPServer::~TcpServer()
{
	Close();
	uv_thread_join(&_start_thread_handle);
	uv_mutex_destroy(&mutex_sessions);
	uv_mutex_destroy(&_mutex_ctxs);
	uv_mutex_destroy(&_mutex_params);
	uv_loop_close(&_loop);
	for(auto it = _avail_ctxs.begin(); it != _avail_ctxs.end(); ++it)
	{
		SessionCtx::Release(*it);
	}
	_avail_ctxs.clear();
	
	for(auto it = _avail_params.begin(); it != _avail_params.end(); ++it)
	{
		WriteParam::Release(*it);
	}
	_avail_params.clear();
	log_info("tcp server exit");
}

bool TCPServer::_init()
{
	if(!_is_closed)	return true;
	int iret = uv_async_init(&_loop, &_async_close_handle, AsyncCloseCB);
	if(iret)
	{
		_err_msg = GetUVError(iret);
		log_error("%s|init async close handle failed|%s", __FUNCTION__, _err_msg.c_str());
		return false;
	}
	_async_close_handle.data = this;

	iret = uv_tcp_init(&loop, &_tcp_handle);
	if(iret)
	{
		_err_msg = GetUVError(iret);
		log_error("%s|init tcp handle failed|%s", __FUNCTION__, _err_msg.c_str());
		return false;
	}
	_tcp_handle.data = this;

	iret = uv_tcp_nodelay(&_tcp_handle, 1);
	if(iret)
	{
		_err_msg = GetUVError(iret);
		log_error("%s|set tcp handle nodelay failed|%s", __FUNCTION__, _err_msg.c_str());
		return false;
	}
	_is_closed = false;
	return true;
}

void TCPServer::_close()
{
	if(_is_closed) return;
	uv_mutex_lock(&_mutex_sessions);
	for(auto it = _sessions.begin(); it != _sessions.end*(); ++it)
	{
		auto session = it->second;
		session->Close();
	}
	uv_mutex_unlock(&_mutex_sessions);
	uv_walk(&_loop, CloseWalkCB, this);
	log_info("close server");
}

bool TCPServer::_run(int status)
{
	int iret = uv_run(&_loop, (uv_run_mode)status);
	if(iret)
	{
		_err_msg = GetUVError(iret);
		log_error("%s|run loop failed|%s", __FUNCTION__, _err_msg.c_str());
		return false;
	}
	log_info("server runing");
	return true;
}

bool TCPServer::_bind(const char *ip, int port)
{
	struct sockaddr_in bind_addr;
	int iret = uv_ip4_addr(ip, port, &bind_addr);
	if(iret)
	{
		_err_msg = GetUVError(iret);
		log_error("%s|ip or port error|%s|%s|%d", __FUNCTION__, _err_msg.c_str(), ip, port);
		return false;
	}
	iret = uv_tcp_bind(&_tcp_handle, (const struct sockaddr*)&bind_addr, 0);
	if(iret)
	{
		_err_msg = GetUVError(iret);
		log_error("%s|bind failed|%s|%s|%d", __FUNCTION__, _err_msg.c_str(), ip, port);
		return false;
	}
	log_info("server bind ip=%s, port=%d", ip, port);
	return true;
}

bool TCPServer::_listen(int backlog)
{
	int iret = uv_listen((uv_stream_t*)&_tcp_handle, backlog, OnConnection);
	if(iret)
	{
		_err_msg = GetUVError(iret);
		log_error("%s|listen failed|%s", __FUNCTION__, _err_msg.c_str());
		return false;
	}
	log_info("server start listening");
	return true;
}

bool TCPServer::Start(const char* ip, int port)
{
	_server_ip = ip;
	_server_port = port;
	_close();
	if(!_init()) return false;
	if(!_bind(ip, port)) return false;
	if(!_listen()) return false;
	int iret = uv_thread_create(&_start_thread_handle, _start_thread, this);
	if(iret)
	{
		_err_msg = GetUVError(iret);
		log_error("%s|create start thread failed|%s", __FUNCTION__, _err_msg.c_str());
		return false;
	}
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

void TCPServer::Close()
{
	if(_is_closed)	return;
	_is_user_closed = true;
	uv_async_send(&_async_close_handle);
}

bool TCPServer::SetNoDelay(bool enable)
{
	int iret = uv_tcp_nodelay(&_tcp_handle, enable ? 1 : 0);
	if(iret)
	{
		_err_msg = GetUVError(iret);
		log_error("%s|set tcp no delay failed|%s", __FUNCTION__, _err_msg.c_str());
		return false;
	}
	return true;
}

bool TCPServer::SetKeepAlive(int enable, unsigned int delay)
{
	int iret = uv_tcp_keepalive(&_tcp_handle, enable, delay);
	if(iret)
	{
		_err_msg = GetUVError(iret);
		log_error("%s|set tcp no delay failed|%s", __FUNCTION__, _err_msg.c_str());
		return false;
	}
	return true;
}

void TCPServer::_start_thread(void *arg)
{
	TCPServer *instance = (TCPServer *)arg;
	instance->_start_status = START_FINISH;
	instance->run();
	// the server is close when come here
	instance->_is_closed = true;
	instance->_is_user_closed = false;
	log_info("server had closed");
	if(instance->_close_cb)
	{
		instance->_close_cb(-1, instance->_close_userdata);
	}
}

void TCPServer::SetNewConnectCB(NewConnectCB cb, void *userdata)
{
	_new_conn_cb = cb;
	_new_conn_userdata = userdata;
}

void TCPServer::SetCloseCB(TcpCloseCB cb, void *userdata)
{
	_close_cb = cb;
	_close_userdata = userdata;
}

void TCPServer::SetRecvCB(int sid, ServerRecvCB cb, void *userdata)
{
	uv_mutex_lock(&_mutex_sessions);
	auto it = _sessions.find(sid);
	if(it != _sessions.end())
	{
		it->second->SetRecvCB(cb, userdata);
	}
	uv_mutex_unlock(&_mutex_sessions);
}

int TCPServer::GenerateSessionID()
{
	static int s_id = 0;
	return ++s_id;
}

void TCPServer::AfterServerClose(uv_handle_t* handle)
{
}

void TCPServer::DeleteTcpHandle(uv_handle_t* handle)
{
	SessionCtx* ctx = (SessionCtx *)handle->data;
	SessionCtx::Release(ctx);
}

void TCPServer::RecycleSessionCtx(uv_handle_t* handle)
{
	SessionCtx* ctx = (SessionCtx *)handle->data;
	assert(ctx);
	TCPServer* server = (TCPServer *)ctx->parent_server;
	server->_recycle_one_ctx(ctx);
}

void TCPServer::OnConnection(uv_stream_t* server, int status)
{
	TCPServer* server_instance = (TCPServer *)server->data;
	assert(server_instance);
	if(status)
	{
		server_instance->_err_msg = GetUVError(status);
		log_error("%s|on connection status abnormal|%s|%d", __FUNCTION__, _err_msg.c_str(), status);
		return;
	}
	
	SessionCtx* ctx = server_instance->_fetch_one_ctx();	
	int iret = uv_tcp_init(&server->instance->_loop, &ctx->tcp_handle);
	if(iret)
	{
		server_instance->_recycle_one_ctx(ctx);
		server_instance->_err_msg = GetUVError(iret);
		log_error("%s|on connection init new tcp handle failed|%s", __FUNCTION__, _err_msg.c_str());
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
		log_error("%s|accept failed|%s", __FUNCTION__, _err_msg.c_str());
		return;
	}
	ctx->packet->SetPacketCB(GetPacket, ctx);
	ctx->packet->Start(server_instance->_pack_head, server_instance->_pack_tail);
	iret = uv_read_start((uv_stream_t *)&ctx->tcp_handle, AllocBufferForRecv, OnRecv);
	if(iret)
	{
		uv_close((uv_handle_t *)&ctx->tcp_handle, TCPServer::RecycleSessionCtx);
		server_instance->_err_msg = GetUVError(iret);
		log_error("%s|start read failed|%s", __FUNCTION__, _err_msg.c_str());
		return;
	}

	Session *session = new Session(ctx, server_instance->_pack_head, server_instance->_pack_tail, &server_instance->_loop);
	session->SetCloseCB(TCPServer::SessionClosed, server_instance);
	
	uv_mutex_lock(&server_instance->_mutex_sessions);
	server_instance->_sessions.insert(std::make_pair(sid, session));
	uv_mutex_unlock(&server_instance->_mutex_sessions);

	if(server_instance->_new_conn_cb)
	{
		server_instance->_new_conn_cb(sid, server_instance->_new_conn_userdata);
	}
	// get ip
	struct sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);
	char client_ip[20] = {0};
	uv_tcp_getsockname(ctx->tcp_handle, (struct sockaddr *)&client_addr, &client_addr_len);
	inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));

	log_info("new connect client|%d|%s", sid, client_ip);
}

void TCPServer::_recycle_one_ctx(SessionCtx* ctx)
{
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

SessionCtx* TCPServer::_fetch_one_ctx()
{
	SessionCtx* ctx = NULL;
	uv_mutex_lock(&_mutex_ctxs);
	if(_avail_ctxs.empty())
	{
		ctx = SessionCtx::Alloc(server_instance);
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

void TCPServer::SessionClosed(int sid, void *userdata)
{
	TCPServer *server = (TCPServer *)userdata;
	uv_mutex_lock(&server->mutex_sessions);
	auto it = server->_sessions.find(sid);
	if(it != server->_sessions.end())
	{
		if(server->_close_cb) server->_close_cb(sid, server->_close_userdata);
		server->_recycle_one_ctx(it->second->GetCtx());

		// get ip
		struct sockaddr_in client_addr;
		int client_addr_len = sizeof(client_addr);
		char client_ip[20] = {0};
		uv_tcp_getsockname(it->second->GetCtx()->tcp_handle, (struct sockaddr *)&client_addr, &client_addr_len);
		inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));

		log_info("session closed|%d|%s", sid, client_ip);
		delete it->second;
		server->_sessions.erase(it);
	}
	uv_mutex_unlock(&server->mutex_sessions);
}

void TCPServer::AsyncCloseCB(uv_async_t* handle)
{
	TCPServer *server = (TCPServer *)handle->data;
	if(server->_is_user_closed) server->_close();
}

void TCPServer::CloseWalkCB(uv_handle_t* handle, void *arg)
{
	TCPServer *server = (TCPServer *)arg;
	if(!uv_is_closing(handle)) uv_close(handle, AfterServerClose);
}

}	// end of namespace UVNET