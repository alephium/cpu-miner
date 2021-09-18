#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "uv.h"
#include "messages.h"

void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
	printf("buffer size: %ld\n", suggested_size);
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}

static void free_close_cb(uv_handle_t *handle)
{
    free(handle);
}

void on_read(uv_stream_t *server, ssize_t nread, const uv_buf_t *buf)
{
	if (nread == -1) {
		fprintf(stderr, "error on_read");
		return;
	}

	server_message_t *message = decode_server_message((uint8_t *)buf->base, nread);
	printf("message type: %d\n", message->kind);
	free_server_message(message);
	// uv_close((uv_handle_t *) server, free_close_cb);
}

void on_connect(uv_connect_t *req, int status)
{
	if (status == -1) {
		fprintf(stderr, "connection error");
		return;
	}
	printf("the server is connected %d %p\n", status, req);

	uv_read_start(req->handle, alloc_buffer, on_read);
}

int main()
{
	uv_loop_t* loop = uv_default_loop();

	uv_tcp_t* socket = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	uv_tcp_init(loop, socket);

	uv_connect_t* connect = (uv_connect_t*)malloc(sizeof(uv_connect_t));

	struct sockaddr_in dest;
	uv_ip4_addr("127.0.0.1", 10973, &dest);

	uv_tcp_connect(connect, socket, (const struct sockaddr*)&dest, on_connect);
	uv_run(loop, UV_RUN_DEFAULT);

	return (0);
}
