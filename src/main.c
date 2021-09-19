#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "constants.h"
#include "uv.h"
#include "messages.h"
#include "blake3.h"
#include "pow.h"
#include "worker.h"

uv_loop_t *loop;

typedef struct mining_template_t {
	job_t *job;
	uint64_t chain_task_count; // increase this by one everytime the template for the chain is updated
} mining_template_t;

mining_template_t* mining_templates[chain_nums] = {};
uint64_t mining_counts[chain_nums];
uint64_t task_counts[chain_nums];
bool mining_templates_initialized = false;

uv_stream_t *tcp;
uint8_t write_buffers[parallel_mining_works][2048 * 1024];

void update_templates(job_t *job)
{
	mining_template_t *mining_template = (mining_template_t *)malloc(sizeof(mining_template_t));
	ssize_t chain_index = job->from_group * group_nums + job->to_group;
	mining_template->job = job;
	mining_template->chain_task_count = mining_templates[chain_index] ? mining_templates[chain_index]->chain_task_count + 1 : 0;
	mining_templates[chain_index] = mining_template;
}

void on_write_end(uv_write_t *req, int status)
{
	if (status == -1) {
		fprintf(stderr, "error on_write_end");
		exit(1);
	}
	printf("Sent new block\n");
}

void submit_new_block(uint32_t worker_id, job_t *job, uint8_t *nonce)
{
	uint8_t *write_pos = write_buffers[worker_id];
	ssize_t block_size = 24 + job->header_blob.len + job->txs_blob.len;
	ssize_t message_size = 1 + 4 + block_size;

	printf("message: %ld\n", message_size);
	write_size(&write_pos, message_size);
	write_byte(&write_pos, 0); // message type
	write_size(&write_pos, block_size);
	write_bytes(&write_pos, nonce, 24);
	write_blob(&write_pos, &job->header_blob);
	write_blob(&write_pos, &job->txs_blob);

	uv_buf_t buf = uv_buf_init((char *)write_buffers[worker_id], message_size + 4);
	print_hex((uint8_t *)buf.base, buf.len);

	uv_write_t *write_req = (uv_write_t *)malloc(sizeof(uv_write_t));
	uint32_t buf_count = 1;

	printf("Sending new block\n");
	uv_write(write_req, tcp, &buf, buf_count, on_write_end);
}

void mine_(mining_worker_t *worker, mining_work_t *work)
{
	worker->hash_count++;
	update_nonce(worker);

	blake3_hasher *hasher = &worker->hasher;
	blake3_hasher_init(hasher);
	blake3_hasher_update(hasher, worker->nonce, 24);
	blob_t *header = &work->job->header_blob;
	blake3_hasher_update(hasher, header->blob, header->len);
	blake3_hasher_finalize(hasher, worker->hash, BLAKE3_OUT_LEN);
	blake3_hasher_init(hasher);
	blake3_hasher_update(hasher, worker->hash, BLAKE3_OUT_LEN);
	blake3_hasher_finalize(hasher, worker->hash, BLAKE3_OUT_LEN);

	if (check_hash(worker->hash, &work->job->target, work->job->from_group, work->job->to_group)) {
		printf("found: %s\n", bytes_to_hex(worker->hash, 32));
		printf("with noce: %s\n", bytes_to_hex(worker->nonce, 24));
		printf("with target: %s\n", bytes_to_hex(work->job->target.blob, work->job->target.len));
		printf("with groups: %d %d\n", work->job->from_group, work->job->to_group);
		worker->found_good_hash = true;
		return;
	} else if (worker->hash_count == mining_steps) {
		return;
	} else {
		mine_(worker, work);
	}
}

void mine(uv_work_t *req)
{
	mining_work_t *work = (mining_work_t *)req->data;
	// printf("start mine: %d %d\n", work->job->from_group, work->job->to_group);
	mining_worker_t *worker = &mining_workers[work->worker_id];
	reset_worker(worker);
	mine_(worker, work);
}

void continue_mine(uint32_t worker_id);

void after_mine(uv_work_t *req, int status)
{
	mining_work_t *work = (mining_work_t *)req->data;
	mining_worker_t *worker = &mining_workers[work->worker_id];
	// printf("after mine: %d %d %d\n", work->job->from_group, work->job->to_group, worker->hash_count);

	if (worker->found_good_hash) {
		submit_new_block(work->worker_id, work->job, worker->nonce);
	}

	uint32_t chain_index = work->job->from_group * group_nums + work->job->to_group;
	mining_counts[chain_index] += worker->hash_count - mining_steps;
	continue_mine(work->worker_id);
}

void mine_on_chain(uint32_t worker_id, uint32_t to_mine_index)
{
	mining_work_t *work = &mining_works[worker_id];
	work->job = mining_templates[to_mine_index]->job;
	work->worker_id = worker_id;
	req[worker_id].data = (void *)work;
	mining_counts[to_mine_index] += mining_steps;
	uv_queue_work(loop, &req[worker_id], mine, after_mine);
}

void continue_mine(uint32_t worker_id)
{
	uint32_t to_mine_index = 0;
	uint64_t least_hash_count = mining_counts[0];
	for (int i = 1; i < chain_nums; i ++) {
		uint64_t i_hash_count = mining_counts[i];
		if (i_hash_count < least_hash_count) {
			to_mine_index = i;
			least_hash_count = i_hash_count;
		}
	}

	mine_on_chain(worker_id, to_mine_index);
}

void start_mining()
{
	assert(mining_templates_initialized == true);

	for (uint32_t i = 0; i < parallel_mining_works; i++) {
		mine_on_chain(i, i % chain_nums);
	}
}

void start_mining_if_needed()
{
	if (!mining_templates_initialized) {
		bool all_initialized = true;
		for (int i = 0; i < chain_nums; i++) {
			if (mining_templates[i] == NULL) {
				all_initialized = false;
				break;
			}
		}
		if (all_initialized) {
			mining_templates_initialized = true;
			start_mining();
		}
	}
}

void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    buf->base = (char *)malloc(suggested_size);
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
		exit(1);
	}

	server_message_t *message = decode_server_message((uint8_t *)buf->base, nread);
	printf("message type: %d\n", message->kind);
	switch (message->kind)
	{
	case JOBS:
		for (int i = 0; i < message->jobs->len; i ++) {
			update_templates(message->jobs->jobs[i]);
		}
		start_mining_if_needed();
		break;

	case SUBMIT_RESULT:
		printf("submitted: %d -> %d: %d \n", message->submit_result->from_group, message->submit_result->to_group, message->submit_result->status);
		break;
	}

	free_server_message_except_jobs(message);
	// uv_close((uv_handle_t *) server, free_close_cb);
}

void on_connect(uv_connect_t *req, int status)
{
	if (status == -1) {
		fprintf(stderr, "connection error");
		return;
	}
	printf("the server is connected %d %p\n", status, req);

	tcp = req->handle;
	uv_read_start(req->handle, alloc_buffer, on_read);
}

int main()
{
	loop = uv_default_loop();

	uv_tcp_t* socket = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	uv_tcp_init(loop, socket);

	uv_connect_t* connect = (uv_connect_t*)malloc(sizeof(uv_connect_t));

	struct sockaddr_in dest;
	uv_ip4_addr("127.0.0.1", 10973, &dest);

	uv_tcp_connect(connect, socket, (const struct sockaddr*)&dest, on_connect);
	uv_run(loop, UV_RUN_DEFAULT);

	return (0);
}
