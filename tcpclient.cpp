#include "tcpclient.h"
#include "log.h"
#include "pb/netmessage.pb.h"
#define MAXLISTSIZE	20

namespace UVNET
{
	/**************************************** TcpClientCtx *******************************************************/
	/**
	**	分配TcpClientCtx
	**	@parenet_server: TCPClient
	**	@return:TcpClientCtx 
	*/
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

	/**
	**	释放TcpClientCtx
	**	@ctx: 待释放的ctx
	*/
	void TcpClientCtx::Release(TcpClientCtx* ctx)
	{
		delete ctx->packet;
		free(ctx->read_buf.base);	
		free(ctx);
	}

	/**************************************** ClientWriteParam ***************************************************/
	/**
	**	分配ClientWriteParam
	**	@return: ClientWriteParam
	*/
	ClientWriteParam* ClientWriteParam::Alloc()
	{
		ClientWriteParam* param = (ClientWriteParam *)malloc(sizeof(ClientWriteParam));
		param->buf.base = (char *)malloc(BUFFER_SIZE);
		param->buf.len = BUFFER_SIZE;
		param->buf_true_len = BUFFER_SIZE;
		return param;
	}

	/**
	**	释放ClientWriteParam
	**	@param: 待释放的param
	*/
	void ClientWriteParam::Release(ClientWriteParam* param)
	{
		free(param->buf.base);
		free(param);
	}

	/**************************************** TCPClient **********************************************************/
	TCPClient::TCPClient(unsigned char pack_head, unsigned char pack_tail)
		: _packet_head(pack_head), _packet_tail(pack_tail)
		, _recv_cb(nullptr), _recv_userdata(nullptr), _close_cb(nullptr), _close_userdata(nullptr)
		, _connect_cb(nullptr), _connect_userdata(nullptr)
		, _connect_status(CONNECT_DIS), _write_circularbuf(BUFFER_SIZE)
		, _is_closed(true), _is_user_closed(false), _is_connecting(false)
	{
		_ctx = TcpClientCtx::Alloc(this);
		int iret = uv_loop_init(&_loop);
		if(iret)
		{
			_err_msg = GetUVError(iret);
			LOG_ERROR("%s|init loop failed|%s", __FUNCTION__, _err_msg.c_str());
		}
		iret = uv_mutex_init(&_mutex_writebuf);
		if(iret)
		{
			_err_msg = GetUVError(iret);
			LOG_ERROR("%s|init writebuf mutex failed|%s", __FUNCTION__, _err_msg.c_str());
		}
		iret = uv_mutex_init(&_mutex_params);
		if(iret)
		{
			_err_msg = GetUVError(iret);
			LOG_ERROR("%s|init params mutex failed|%s", __FUNCTION__, _err_msg.c_str());
		}
		_connect_req.data = this;
	}

	TCPClient::~TCPClient()
	{
		Close();
		uv_thread_join(&_connect_thread_handle);
		TcpClientCtx::Release(_ctx);
		uv_loop_close(&_loop);
		uv_mutex_destroy(&_mutex_writebuf);
		uv_mutex_destroy(&_mutex_params);
		for(auto it = _avail_params.begin(); it != _avail_params.end(); ++it)
		{
			ClientWriteParam::Release(*it);
		}
		_avail_params.clear();

		for(std::map<int, Protocol*>::iterator it = _protocols.begin(); it != _protocols.end(); ++it)
		{
			delete it->second;
		}
		_protocols.clear();

		LOG_TRACE("tcp client closed");
	}

	/**
	**	初始化函数
	**	@return: true, 成功; false, 失败
	*/
	bool TCPClient::_init()
	{
		if(!_is_closed) return true;
		int iret = uv_async_init(&_loop, &_async_handle, AsyncCB);
		if(iret)
		{
			_err_msg = GetUVError(iret);
			LOG_ERROR("%s|init async close handle failed|%s", __FUNCTION__, _err_msg.c_str());
			return false;
		}
		_async_handle.data = this;

		iret = uv_tcp_init(&_loop, &_ctx->tcp_handle);
		if(iret)
		{
			_err_msg = GetUVError(iret);
			LOG_ERROR("%s|init tcp handle failed|%s", __FUNCTION__, _err_msg.c_str());
			return false;
		}
		_ctx->tcp_handle.data = _ctx;
		_ctx->parent_server = this;
		_ctx->packet->SetPacketCB(GetPacket, _ctx);
		_ctx->packet->Start(_packet_head, _packet_tail);

		iret = uv_timer_init(&_loop, &_connect_timer);
		if(iret)
		{
			_err_msg = GetUVError(iret);
			LOG_ERROR("%s|init connect timer failed|%s", __FUNCTION__, _err_msg.c_str());
			return false;
		}
		_connect_timer.data = this;
		LOG_TRACE("client init");
		_is_closed = false;
		return true;
	}

	/**
	**	真正的关闭函数
	*/
	void TCPClient::_close()
	{
		if(_is_closed)	return;
		_stop_reconnect();
		uv_walk(&_loop, CloseWalkCB, this);
		LOG_TRACE("tcp client closed");
	}

	/**
	**	启动事件循环
	**	@status: 运行模式
	*/
	bool TCPClient::_run(int status)
	{
		int iret = uv_run(&_loop, (uv_run_mode)status);	
		if(iret)
		{
			_err_msg = GetUVError(iret);
			LOG_ERROR("%s|run loop failed|%s", __FUNCTION__, _err_msg.c_str());
			return false;
		}
		// 运行到此处意味着已关闭
		_is_closed = true;
		LOG_TRACE("client had closed");
		if(_close_cb) _close_cb(-1, _close_userdata);
		return true;
	}

	/**
	**	开启或关闭 Nagle 算法，在服务启动后方可调用
	**  @enable: 1是启动，0是禁止
	**  @return: true, 设置成功; false, 设置失败
	*/
	bool TCPClient::SetNoDelay(bool enable)
	{
		int iret = uv_tcp_nodelay(&_ctx->tcp_handle, enable ? 1 : 0);
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
	bool TCPClient::SetKeepAlive(int enable, unsigned int delay)
	{
		int iret = uv_tcp_keepalive(&_ctx->tcp_handle, enable, delay);
		if(iret)
		{
			_err_msg = GetUVError(iret);
			LOG_ERROR("%s|set tcp keepalive failed|%s", __FUNCTION__, _err_msg.c_str());
			return false;
		}
		return true;
	}

	/**
	**	添加server的处理协议
	**	@proto_id: 协议id
	**  @proto: 协议处理实例
	*/
	void TCPClient::AddProtocol(int proto_id, Protocol* proto)
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
	void TCPClient::RemoveProtocol(int protocol_id)
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
	Protocol* TCPClient::GetProtocol(int proto_id)
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
	**	连接目标地址
	**  @ip: 目标ip
	**  @port: 目标端口
	**  @return: true, 连接成功; false, 连接失败
	*/
	bool TCPClient::Connect(const char* ip, int port)
	{
		// 先保证已经关闭
		_close();
		if(!_init())	return false;
		_connect_ip = ip;
		_connect_port = port;
		struct sockaddr_in bind_addr;
		int iret = uv_ip4_addr(ip, port, &bind_addr);
		if(iret)
		{
			_err_msg = GetUVError(iret);
			LOG_ERROR("%s|ip or port invalid|%s|%s|%d", __FUNCTION__, _err_msg.c_str(), ip, port);
			return false;
		}
		
		iret = uv_tcp_connect(&_connect_req, &_ctx->tcp_handle, (const sockaddr*)&bind_addr, OnConnect);
		if(iret)
		{
			_err_msg = GetUVError(iret);
			LOG_ERROR("%s|connect failed|%s|%s|%d", __FUNCTION__, _err_msg.c_str(), ip, port);
			return false;
		}

		// 单独开启一个连接线程启动事件循环，任何libuv相关操作都在此线程中完成，因此TCPClient可以多线程任意调用
		iret = uv_thread_create(&_connect_thread_handle, ConnectThread, this);
		if(iret)
		{
			_err_msg = GetUVError(iret);
			LOG_ERROR("%s|create thread failed|%s", __FUNCTION__, _err_msg.c_str());
			return false;
		}
		LOG_TRACE("client start connect |%s|%d", ip, port);
		// 等待一定时间
		int wait_cont = 0;
		while(_connect_status == CONNECT_DIS)
		{
			ThreadSleep(100);
			if(++wait_cont > 100)
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
	
	/**
	**	连接线程函数
	**  @arg: TCPClient
	*/
	void TCPClient::ConnectThread(void* arg)
	{
		TCPClient *pClient = (TCPClient *)arg;
		pClient->_run();
	}

	/**
	**	连接回调函数, 在Connect中调用uv_tcp_connect设置
	**  @req: uv_connect_t, handle->data中存放着TcpClientCtx
	**  @status: 状态，非0代表出错
	*/
	void TCPClient::OnConnect(uv_connect_t* req, int status)
	{
		TcpClientCtx* ctx = (TcpClientCtx *)req->handle->data;
		TCPClient * pClient = (TCPClient *)ctx->parent_server;
		if(status)
		{
			pClient->_connect_status = CONNECT_ERROR;
			pClient->_err_msg = GetUVError(status);
			LOG_ERROR("%s|on connect error|%s|%s|%d", __FUNCTION__, pClient->_err_msg.c_str(), pClient->_connect_ip.c_str(), pClient->_connect_port);
			// 如果正在重连，则将重试时间变为2倍
			if(pClient->_is_connecting)
			{
				uv_timer_stop(&pClient->_connect_timer);	
				pClient->_repeat_time *= 2;
				uv_timer_start(&pClient->_connect_timer, TCPClient::ReconnectTimer, pClient->_repeat_time, pClient->_repeat_time);
			}
			return;
		}

		// 准备接收数据, 注册接收数据回调函数
		int iret = uv_read_start(req->handle, AllocBufferForRecv, OnRecv);
		if(iret)
		{
			pClient->_err_msg = GetUVError(status);
			LOG_ERROR("%s|uv_read_start failed|%s", __FUNCTION__, pClient->_err_msg.c_str());
			pClient->_connect_status = CONNECT_ERROR;
		}
		else
		{
			// 读取数据成功
			pClient->_connect_status = CONNECT_FINISH;
			LOG_TRACE("client connect success");
		}
		// 如果这是重连的连接成功
		if(pClient->_is_connecting)
		{
			pClient->_stop_reconnect();
			LOG_TRACE("client reconnect success");
		}

		if(pClient->_connect_cb) pClient->_connect_cb(NET_EVENT_TYPE_CONNECT, pClient->_connect_userdata);
	}

	/**
	**	发送数据
	**  @data: 待发送数据
	**  @len: 待发送数据的长度
	**	@return: 向circular buffer写入的数据长度
	*/
	int TCPClient::Send(const char * data, size_t len)
	{
		if(!data || len <= 0)
		{
			LOG_TRACE("send data is null or less than 0");
			return 0;
		}
		// 向异步句柄发送信号，由注册的AsyncCB负责调用_send进行真实的数据发送
		uv_async_send(&_async_handle);
		size_t iret = 0;
		while(!_is_user_closed)
		{
			// 将数据发往circular buffer
			// TODO: 这里进行了数据拷贝，是否可以考虑省掉这次数据拷贝，从而提升效率?
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
		// 向异步句柄发送信号，由注册的AsyncCB负责调用_send进行真实的数据发送
		uv_async_send(&_async_handle);
		return iret;
	}

	/**
	**	设置接收数据回调函数
	**  @cb: 回调函数
	**  @userdata: 回调参数
	*/
	void TCPClient::SetRecvCB(ClientRecvCB cb, void* userdata)
	{
		_recv_cb = cb;
		_recv_userdata = userdata;
	}

	/**
	**	设置关闭连接回调函数
	**  @cb: 回调函数
	**  @userdata: 回调参数
	*/
	void TCPClient::SetCloseCB(TcpCloseCB cb, void* userdata)
	{
		_close_cb = cb;
		_close_userdata = userdata;
	}

	/**
	**	设置连接回调函数
	**  @cb: 回调函数
	**  @userdata: 回调参数
	*/
	void TCPClient::SetConnectCB(ConnectCB cb, void* userdata)
	{
		_connect_cb = cb;
		_connect_userdata = userdata;
	}

	/**
	**	接收数据时分配缓冲区回调函数, 在OnConnect中调用uv_read_start设置
	**  @handle: data中存放TcpClientCtx
	**  @suggested_size: 建议大小
	**	@buf:	分配的缓冲区
	*/
	void TCPClient::AllocBufferForRecv(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
	{
		TcpClientCtx* ctx = (TcpClientCtx *)handle->data;
		assert(ctx);
		*buf = ctx->read_buf;
	}

	/**
	**	接收数据回调函数, 在OnConnect中调用uv_read_start设置
	**  @handle: data中存放TcpClientCtx
	**  @nread: 接收到的数据
	**	@buf:	数据的缓冲区
	*/
	void TCPClient::OnRecv(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
	{
		TcpClientCtx* ctx = (TcpClientCtx *)handle->data;
		assert(ctx);
		TCPClient *pClient = (TCPClient *)ctx->parent_server;
		if(nread < 0)
		{
			// 断开连接事件触发，调用连接回调
			if(pClient->_connect_cb)	pClient->_connect_cb(NET_EVENT_TYPE_DISCONNECT, pClient->_connect_userdata);
			// 尝试重连
			if(!pClient->_start_reconnect())	return;
			if(nread == UV_EOF)
			{
				LOG_TRACE("server closed(EOF)");
			}
			else if(nread == UV_ECONNRESET)
			{
				LOG_TRACE("server closed(RESET)");
			}
			else
			{
				LOG_TRACE("server closed(%s)", GetUVError(nread).c_str());
			}
			uv_close((uv_handle_t *)handle, OnClientClose);
			return;
		}	// end of if nread < 0
		// 调用_send发送circular buf中的数据
		pClient->_send();
		// 调用packet对收到的数据进行解包
		if(nread > 0) pClient->_ctx->packet->recvdata((const unsigned char *)buf->base, nread);
	}

	/**
	**	发送数据回调函数
	**  @req: data中存放TCPClient
	**	@status:	状态，0为成功
	*/
	void TCPClient::OnSend(uv_write_t* req, int status)
	{
		TCPClient* pClient = (TCPClient *)req->data;
		if(status < 0)
		{
			pClient->_recycle_one_param((ClientWriteParam *)req);
			LOG_ERROR("send error|%s", GetUVError(status).c_str());
			return;
		}
		// 调用_send进行真正的数据发送
		pClient->_send(req);
	}

	/**
	**	回收WriteParam
	**  @param: 待回收的param
	*/
	void TCPClient::_recycle_one_param(ClientWriteParam* param)
	{
			uv_mutex_lock(&_mutex_params);
			if(_avail_params.size() > MAXLISTSIZE)
			{
				ClientWriteParam::Release(param);
			}
			else
			{
				_avail_params.push_back(param);
			}
			uv_mutex_unlock(&_mutex_params);
	}

	/**
	**	获取WriteParam
	**  @return: 分配的param
	*/
	ClientWriteParam* TCPClient::_fetch_one_param()
	{
			ClientWriteParam* param = NULL;
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
			return param;
	}

	/**
	**	关闭ioloop中的handle, 由uv_walk设置
	**  @handle: 待关闭的句柄
	**	@arg: TCPClient
	*/
	void TCPClient::CloseWalkCB(uv_handle_t* handle, void* arg)
	{
		TCPClient* pClient = (TCPClient *)arg;
		if(!uv_is_closing(handle)) uv_close(handle, OnClientClose);
	}

	/**
	**	客户端handle关闭回调函数, 由uv_close设置
	**  @handle: 待关闭的句柄
	*/
	void TCPClient::OnClientClose(uv_handle_t* handle)
	{
		TCPClient* pClient = (TCPClient *)handle->data;
		if(handle == (uv_handle_t *)&pClient->_ctx->tcp_handle && pClient->_is_connecting)
		{
			// 是tcp连接句柄 且 正在重连中
			int iret = uv_timer_start(&pClient->_connect_timer, TCPClient::ReconnectTimer, pClient->_repeat_time, pClient->_repeat_time);
			if(iret)
			{
				uv_close((uv_handle_t *)&pClient->_connect_timer, TCPClient::OnClientClose);
				LOG_ERROR("%s|read failed|%s", __FUNCTION__, GetUVError(iret).c_str());
				return;
			}
		}
	}

	/**
	**	获取一个完整的数据帧的回调函数, 由uv_read_start设置
	**  @packet_head: 封包头，在net_base.h中定义
	**	@packet_data: 真实的数据
	**	@userdata: TCPClientCtx
	*/
	void TCPClient::GetPacket(const NetPacket& packet_head, const char* packet_data, void* userdata)
	{
		assert(userdata);
		TcpClientCtx* ctx = (TcpClientCtx *)userdata;
		TCPClient* pClient = (TCPClient *)ctx->parent_server;
		// 收到完整数据封包后调用recv_cb回调函数
		// TODO: 考虑此处是否向server一样采用处理协议的方式?
		if(pClient->_recv_cb)	pClient->_recv_cb(packet_head, packet_data, pClient->_recv_userdata);

		unsigned int proto_id = 0;
		const char* proto_data = NULL;
		int data_size = 0;
		if(packet_head.type == 1)
		{
			// 采用protobuf解析协议
			CProto proto;
			proto.ParseFromArray(packet_data, packet_head.datalen);
			proto_id = proto.id();
			proto_data = proto.body().c_str();
			data_size = proto.body().size();
		}
		else if(packet_head.type == 2)
		{
			// 手动解析协议
			// 前4个字节是协议id，后面是序列化后的协议内容
			if(!CharToInt32(packet_data, proto_id)) return;
			proto_data = &packet_data[4];
			data_size = packet_head.datalen - 4;
		}
		else
		{
			// 无效协议
		}
		if(proto_id > 0)
		{
			Protocol* proto_handle = NULL;
			proto_handle = pClient->GetProtocol(proto_id);
			if(proto_handle)
			{
				// 调用协议来解析数据包并返回相应的response
				const std::string& send_data = proto_handle->Process(proto_data, data_size);
				pClient->Send(send_data.c_str(), send_data.size());
			}
		}
	}

	/**
	**	异步句柄回调函数
	**  @handle: 异步句柄
	*/
	void TCPClient::AsyncCB(uv_async_t* handle)
	{
		TCPClient* pClient = (TCPClient *)handle->data;
		if(pClient->_is_user_closed)
		{
			// 如果是用户请求关闭
			pClient->_close();
			return;
		}
		pClient->_send();
	}

	/**
	**	真正的发送数据的函数
	**  @req: ClientWriteParam
	*/
	void TCPClient::_send(uv_write_t* req)
	{
		ClientWriteParam* param = (ClientWriteParam *)req;
		if(param)
		{
			_recycle_one_param(param);
		}
		while(true)
		{
			uv_mutex_lock(&_mutex_writebuf);
			if(_write_circularbuf.empty())
			{
				uv_mutex_unlock(&_mutex_writebuf);
				break;
			}

			param = _fetch_one_param();
			// 从circular buf中读取要发送的数据
			param->buf.len = _write_circularbuf.read(param->buf.base, param->buf_true_len);
			uv_mutex_unlock(&_mutex_writebuf);

			// 发送数据
			int iret = uv_write((uv_write_t *)&param->write_req, (uv_stream_t *)&_ctx->tcp_handle, &param->buf, 1, OnSend);
			if(iret)
			{
				_recycle_one_param(param);
				LOG_ERROR("client send data error|%s", GetUVError(iret).c_str());
			}
		}	// end of while
	}

	/**
	**	关闭函数
	*/
	void TCPClient::Close()
	{
		if(_is_closed) return;
		_is_user_closed = true;
		uv_async_send(&_async_handle);
	}

	/**
	**	尝试重连，目前只在recv出错时重连
	*/
	bool TCPClient::_start_reconnect()
	{
		_is_connecting = true;
		_ctx->tcp_handle.data = this;
		_repeat_time = 1e3;
		return true;
	}

	/**
	**	停止重连，连接成功或者关闭时会调用
	*/
	void TCPClient::_stop_reconnect()
	{
		_is_connecting = false;
		_ctx->tcp_handle.data = _ctx;
		_repeat_time = 1e3;
		uv_timer_stop(&_connect_timer);
	}

	/**
	**	timer定时重连函数
	**	@handle: data中存放的是TCPClient
	*/
	void TCPClient::ReconnectTimer(uv_timer_t* handle)
	{
		TCPClient* pClient = (TCPClient *)handle->data;
		if(!pClient->_is_connecting) return;
		LOG_TRACE("start reconnect...");
		do
		{
			int iret = uv_tcp_init(&pClient->_loop, &pClient->_ctx->tcp_handle);
			if(iret)
			{
				LOG_ERROR("%s|init tcp failed|%s", __FUNCTION__, GetUVError(iret).c_str());
				break;
			}
			pClient->_ctx->tcp_handle.data = pClient->_ctx;
			pClient->_ctx->parent_server = pClient;
			struct sockaddr_in bind_addr;
			iret = uv_ip4_addr(pClient->_connect_ip.c_str(), pClient->_connect_port, &bind_addr);
			if(iret)
			{
				LOG_ERROR("%s|ip or port invalid|%s|%s|%d", __FUNCTION__, GetUVError(iret).c_str(), pClient->_connect_ip.c_str(), pClient->_connect_port);
				uv_close((uv_handle_t *)&pClient->_ctx->tcp_handle, NULL);
				break;
			}
			iret = uv_tcp_connect(&pClient->_connect_req, &pClient->_ctx->tcp_handle, (const struct sockaddr*)&bind_addr, OnConnect);
			if(iret)
			{
				LOG_ERROR("%s|reconnect failed|%s|%s|%d", __FUNCTION__, GetUVError(iret).c_str(), pClient->_connect_ip.c_str(), pClient->_connect_port);
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
