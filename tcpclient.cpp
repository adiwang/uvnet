#include "tcpclient.h"
#define MAXLISTSIZE	20
namespace UVNET
{
	/**************************************** TcpClientCtx *******************************************************/
	TcpClientCtx* TcpClientCtx::Alloc(void* parent_server)
	{
		TcpClientCtx* ctx = (TcpClientCtx *)malloc(sizeof(TcpClientCtx));
		ctx->packet = new PacketSync;
		ctx->read_buf.base = (char *)malloc(BUFFER_SIZE);
		ctx->read_buf.len = BUFFER_SIZE;
		ctx->write_req.data = ctx;
		ctx->parent_server = parent_server;
		return ctx;
	}

	void TcpClientCtx::Release(TcpClientCtx* ctx)
	{
		delete ctx->packet;
		free(ctx->read_buf.base);	
		free(ctx);
	}

	/**************************************** ClientWriteParam ***************************************************/
	ClientWriteParam* ClientWriteParam::Alloc()
	{
		ClientWriteParam* param = (ClientWriteParam *)malloc(sizeof(ClientWriteParam));
		param->buf.base = (char *)malloc(BUFFER_SIZE);
		param->buf.len = BUFFER_SIZE;
		param->buf_true_len = BUFFER_SIZE;
		return param;
	}

	void ClientWriteParam::Release(ClientWriteParam* param)
	{
		free(param->buf.base);
		free(param);
	}

	/**************************************** TCPClient **********************************************************/
	TCPClient::TCPClient(unsigned char pack_head, unsigned char pack_tail)
		: _pack_head(pack_head), _pack_tail(pack_tail)
		, _recv_cb(nullptr), _recv_userdata(nullptr), _close_cb(nullptr), _close_userdata(nullptr)
		, _connect_cb(nullptr), _connect_userdata(nullptr)
		, _connect_status(CONNECT_DIS), _write_circularbuf(BUFFER_SIZE)
		, _is_closed(true), _is_user_closed(false), _is_connecting(false)
	{
		ctx = TcpClientCtx::Alloc(this);
		int iret = uv_loop_init(&_loop);
		if(iret)
		{
			_err_msg = GetUVError(iret);
			log_error("%s|init loop failed|%s", __FUNCTION__, _err_msg.c_str());
		}
		iret = uv_mutex_init(&_mutex_writebuf);
		if(iret)
		{
			_err_msg = GetUVError(iret);
			log_error("%s|init writebuf mutex failed|%s", __FUNCTION__, _err_msg.c_str());
		}
		iret = uv_mutex_init(&_mutex_params);
		if(iret)
		{
			_err_msg = GetUVError(iret);
			log_error("%s|init params mutex failed|%s", __FUNCTION__, _err_msg.c_str());
		}
		_connect_req.data = this;
	}

	TCPClient::~TCPClient()
	{
		Close();
		uv_thread_join(&_connect_thread_handle);
		TcpClientCtx::Release(_ctx);
		uv_mutex_destroy(&_mutex_writebuf);
		uv_mutex_destroy(&_mutex_params);
		for(auto it = _avail_params.begin(); it != _avail_params.end(); ++it)
		{
			ClientWriteParam::Release(*it);
		}
		_avail_params.clear();
		log_info("tcp client closed");
	}

	bool TCPClient::_init()
	{
		if(!_is_closed) return true;
		int iret = uv_async_init(&_loop, &_async_handle, AsyncCB);
		if(iret)
		{
			_err_msg = GetUVError(iret);
			log_error("%s|init async close handle failed|%s", __FUNCTION__, _err_msg.c_str());
			return false;
		}
		_async_handle.data = this;

		iret = uv_tcp_init(&_loop, &_ctx->tcp_handle);
		if(iret)
		{
			_err_msg = GetUVError(iret);
			log_error("%s|init tcp handle failed|%s", __FUNCTION__, _err_msg.c_str());
			return false;
		}
		_ctx->tcp_handle.data = ctx;
		_ctx->parent_server = this;
		_ctx->packet->SetPacketCB(GetPacket, _tcp_handle);
		_ctx->packet->Start(_pack_head, _pack_tail);

		iret = uv_timer_init(&_loop, &_connect_timer);
		if(iret)
		{
			_err_msg = GetUVError(iret);
			log_error("%s|init connect timer failed|%s", __FUNCTION__, _err_msg.c_str());
			return false;
		}
		_connect_timer.data = this;
		log_info("client init");
		_is_closed = false;
		return true;
	}

	void TCPClient::_close()
	{
		if(_is_closed)	return;
		_stop_connect();
		uv_walk(&_loop, CloseWalkCB, this);
		log_info("tcp client closed");
	}

	bool TCPClient::_run(int status)
	{
		int iret = uv_run(&_loop, (uv_run_mode)status);	
		if(iret)
		{
			_err_msg = GetUVError(iret);
			log_error("%s|run loop failed|%s", __FUNCTION__, _err_msg.c_str());
			return false;
		}
		// codes come here should be close
		_is_closed = true;
		log_info("client had closed");
		if(_close_cb) _close_cb(-1, _close_userdata);
		return true;
	}

	bool TCPClient::SetNoDelay(bool enable)
	{
		int iret = uv_tcp_nodelay(&ctx->tcp_handle, enable ? 1 : 0);
		if(iret)
		{
			_err_msg = GetUVError(iret);
			log_error("%s|set tcp no delay failed|%s", __FUNCTION__, _err_msg.c_str());
			return false;
		}
		return true;
	}

	bool TCPClient::SetKeepAlive(int enable, unsigned int delay)
	{
		int iret = uv_tcp_keepalive(&_ctx->tcp_handle, enable, delay);
		if(iret)
		{
			_err_msg = GetUVError(iret);
			log_error("%s|set tcp keepalive failed|%s", __FUNCTION__, _err_msg.c_str());
			return false;
		}
		return true;
	}

	bool TCPClient::Connect(const char* ip, int port)
	{
		_close();
		if(!_init())	return false;
		_connect_ip = ip;
		_connect_port = port;
		struct sockaddr_in bind_addr;
		int iret = uv_ip4_addr(ip, port, &bind_addr);
		if(iret)
		{
			_err_msg = GetUVError(iret);
			log_error("%s|ip or port invalid|%s|%s|%d", __FUNCTION__, _err_msg.c_str(), ip, port);
			return false;
		}
		iret = uv_thread_create(&_connect_thread_handle, ConnectThread, this);
		if(iret)
		{
			_err_msg = GetUVError(iret);
			log_error("%s|connect failed|%s|%s|%d", __FUNCTION__, _err_msg.c_str(), ip, port);
			return false;
		}
		log_info("client start connect |%s|%d", ip, port);
		int retry_cont = 0;
		while(_connect_status == CONNECT_DIS)
		{
			ThreadSleep(100);
			if(++retry_count > 100)
			{
				_connect_status = CONNECT_TIMEOUT;
				break;
			}
		}
		if(CONNECT_FINISH != _connect_status)
		{
			_err_msg = "connect time out";
			return false;
		}
		return true;
	}
	
	void TCPClient::ConnectThread(void* arg)
	{
		TCPClient *pClient = (TCPClient *)arg;
		pClient->run();
	}

	void TCPClient::OnConnect(uv_connect_t* handle, int status)
	{
		TcpClientCtx* ctx = (TcpClientCtx *)handle->handle->data;
		TCPClient * pClient = (TCPClient *)ctx->parent_server;
		if(status)
		{
			pClient->_connect_status = CONNECT_ERROR;
			pClient->_err_msg = GetUVError(status);
			log_error("%s|connect error|%s|%s|%d", __FUNCTION__, pClient->_err_msg.c_str(), pClient->_connect_ip.c_str(), pClient->_connect_port);
			if(pClient->_is_connecting)
			{
				uv_timer_stop(&pClient->_connect_timer);	
				pClient->_repeat_time *= 2;
				uv_timer_start(&pClient->_connect_timer, TCPClient::ReconnectTimer, pClient->_repeat_time, pClient->_repeat_time);
			}
			return;
		}

		int iret = uv_read_start(handle->handle, AllocBufferForRecv, OnRecv);
		if(iret)
		{
			pClient->_err_msg = GetUVError(status);
			log_error("%s|read failed|%s", __FUNCTION__, pClient->_err_msg.c_str());
			pClient->_connect_status = CONNECT_ERROR;
		}
		else
		{
			pClient->_connect_status = CONNECT_FINISH;
			log_info("client connect success");
		}
		if(pClient->_is_connecting)
		{
			pClient->_stop_reconnect();
			if(pClient->_connect_cb) pClient->_connect_cb(NET_EVENT_TYPE_RECONNECT, pClient->_connect_userdata);
		}
	}

	int TCPClient::_send(const char * data, size_t len)
	{
		if(!data || len <= 0)
		{
			log_info("send data is null or less than 0");
			return 0;
		}
		uv_async_send(&_async_handle);
		size_t iret = 0;
		while(!_is_user_closed)
		{
			uv_mutex_lock(&_mutex_writebuf);
			iret += _write_circularbuf.write(data + iret, len - iret);
			uv_mutex_unlock(&_mutex_writebuf);
			if(iret < len)
			{
				ThreadSleep(100);
				continue;
			}
			break;
		}
		uv_async_send(&_async_handle);
		return iret;
	}

	void TCPClient::SetRecvCB(ClientRecvCB cb, void* userdata)
	{
		_recv_cb = cb;
		_recv_userdata = userdata;
	}

	void TCPClient::SetCloseCB(TcpCloseCB cb, void* userdata)
	{
		_close_cb = cb;
		_close_userdata = userdata;
	}

	void TCPClient::SetConnectCB(ConnectCB cb, void* userdata)
	{
		_connect_cb = cb;
		_connect_userdata = userdata;
	}

	void TCPClient::AllocBufferForRecv(uv_handle_t* handle, ssize_t suggested_size, uv_buf_t* buf)
	{
		TcpClientCtx* ctx = (TcpClientCtx *)handle->data;
		assert(ctx);
		*buf = ctx->read_buf;
	}

	void TCPClient::OnRecv(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
	{
		TcpClientCtx* ctx = (TcpClientCtx *)handle->data;
		assert(ctx);
		TCPClient *pClient = ctx->parent_server;
		if(nread < 0)
		{
			if(pClient->_connect_cb)	pClient->_connect_cb(NET_EVENT_TYPE_DISCONNECT, pClient->_connect_userdata);
			if(!pClient->StartReconnect())	return;
			if(nread == UV_EOF)
			{
				log_info("server closed(EOF)");
			}
			else if(nread == UV_ECONNRESET)
			{
				log_info("server closed(RESET)");
			}
			else
			{
				log_info("server closed(%s)", GetUVError(nread));
			}
			uv_close((uv_handle_t *)handle, OnClientClose);
			return;
		}	// end of if nread < 0
		pClient->_send();
		if(nread > 0) pClient->packet->recvdata((const unsigned char *)buf->base, nread);
	}

	void TCPClient::OnSend(uv_write_t* req, int status)
	{
		TCPClient* pClient = (TCPClient *)req->data;
		if(status < 0)
		{
			uv_mutex_lock(&mutex_params);
			if(pClient->_avail_params.size() > MAXLISTSIZE)
			{
				ClientWriteParam::Release((ClientWriteParam *)req);
			}
			else
			{
				pClient->_avail_params.push_back((ClientWriteParam *)req);
			}
			uv_mutex_unlock(&mutex_params);
			log_error("send error|%s", GetUVError(status));
			return;
		}
		pClient->_send(req);
	}

	void TCPClient::CloseWalkCB(uv_handle_t* handle, void* arg)
	{
		TCPClient* pClient = (TCPClient *)arg;
		if(!uv_is_closing(handle)) uv_close(handle, OnClientClose);
	}

	void TCPClient::OnClientClose(uv_handle_t* handle)
	{
		TCPClient* pClient = (TCPClient *)handle->data;
		if(handle == (uv_handle_t *)&pClient->_ctx->tcp_handle && pClient->_is_connecting)
		{
			int iret = uv_timer_start(&pClient->_connect_timer, TCPClient::ReconnectTimer, pClient->_repeat_time, pClient->_repeat_time);
			if(iret)
			{
				uv_close((uv_handle_t *)&pClient->_connect_timer, TCPClient::OnClientClose)
				log_error("%s|read failed|%s", __FUNCTION__, GetUVError(iret));
				return;
			}
		}
	}

	void TCPClient::GetPacket(const NetPacket& packet_head, const unsigned char* packet_data, void* userdata)
	{
		assert(userdata);
		TcpClientCtx* ctx = (TcpClientCtx *)userdata;
		TCPClient* pClient = ctx->parent_server;
		if(pClient->_recv_cb)	pClient->_recv_cb(packet_head, packet_data, pClient->_recv_userdata);
	}

	void TCPClient::AsyncCB(uv_async_t* handle)
	{
		TCPClient* pClient = (TCPClient *)handle->data;
		if(pClient->_is_user_closed)
		{
			pClient->_close();
			return;
		}
		pClient->_send();
	}

	void TCPClient::_send(uv_write_t* req)
	{
		ClientWriteParam* param = (ClientWriteParam *)req;
		if(param)
		{
			uv_mutex_lock(&_mutex_params);
			if(_avail_params.size() > MAXLISTSIZE)
			{
				ClientWriteParam::Release();
			}
			else
			{
				_avail_params.push_back(param);
			}
			uv_mutex_unlock(&_mutex_params);
		}
		while(true)
		{
			uv_mutex_lock(&_mutex_writebuf);
			if(_write_circularbuf.empty())
			{
				uv_mutex_unlock(&_mutex_writebuf);
				break;
			}

			uv_mutex_lock(&_mutex_params);
			if(_avail_params.empty())
			{
				param = ClientWriteParam::Alloc();
				param->write_req.data = this;
			}
			else
			{
				param = _avail_params.front();
				_avail_params.pop_front();
			}
			uv_mutex_unlock(&_mutex_params);
			
			param->buf.len = _write_circularbuf.read(param->buf.base, param->buf_true_len);
			uv_mutex_unlock(&_mutex_writebuf);

			int iret = uv_write((uv_write_t *)&param->write_req, (uv_stream_t *)&ctx->tcp_handle, &param->buf, 1, OnSend);
			if(iret)
			{
				uv_mutex_lock(&_mutex_params);
				if(_avail_params.size() > MAXLISTSIZE)
				{
					ClientWriteParam::Release();
				}
				else
				{
					_avail_params.push_back(param);
				}
				uv_mutex_unlock(&_mutex_params);
				log_error("client send data error|%s", GetUVError(iret));
			}
		}	// end of while
	}

	void TCPClient::Close()
	{
		if(_is_closed) return;
		_is_user_closed = true;
		uv_async_send(&_async_handle);
	}

	void TCPClient::_start_reconnect()
	{
		_is_connecting = true;
		_ctx->tcp_handle.data = this;
		_repeat_time = 1e3;
		return true;
	}

	void TCPClient::_stop_reconnect()
	{
		_is_connecting = false;
		_ctx->tcp_handle.data = _ctx;
		_repeat_time = 1e3;
		uv_timer_stop(&_connect_timer);
	}

	void TCPClient::ReconnectTimer(uv_timer_t* handle)
	{
		TCPClient* pClient = (TCPClient *)handle->data;
		if(!pClient->_is_connecting) return;
		log_info("start reconnect...");
		do
		{
			int iret = uv_tcp_init(&pClient->_loop, &pClient->_ctx->tcp_handle);
			if(iret)
			{
				log_error("%s|init tcp failed|%s", __FUNCTION__, GetUVError(iret));
				break;
			}
			pClient->_ctx->tcp_handle.data = pClient->_ctx;
			pClient->_ctx->parent_server = pClient;
			struct sockaddr_in bind_addr;
			iret = uv_ip4_addr(pClient->_connect_ip.c_str(), pClient->_connect_port.c_str(), &bind_addr);
			if(iret)
			{
				_err_msg = GetUVError(iret);
				log_error("%s|ip or port invalid|%s|%s|%d", __FUNCTION__, _err_msg.c_str(), ip, port);
				uv_close((uv_handle_t *)&pClient->_ctx->tcp_handle, NULL);
				break;
			}
			struct sockaddr* pAddr;
			pAddr = (struct sockaddr*)&bind_addr;
			iret = uv_tcp_connect(&pClient->_connect_req, &pClient->_ctx->tcp_handle, (const sockaddr*)pAddr, OnConnect);
			if(iret)
			{
				_err_msg = GetUVError(iret);
				log_error("%s|reconnect failed|%s|%s|%d", __FUNCTION__, _err_msg.c_str(), ip, port);
				uv_close((uv_handle_t *)&pClient->_ctx->tcp_handle, NULL);
				break;
			}
			return;
		}while(0);
		// reconnect failed
		uv_timer_stop(handle);
		pClient->_repeat_time *= 2;
		uv_timer_start(handle, TCPClient::ReconnectTimer, pClient->_repeat_time, pClient->_repeat_time);
	}
}	// end of namespace UVNET
