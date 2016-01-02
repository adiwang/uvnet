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
}	// end of namespace UVNET
