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

}	// end of namespace UVNET
