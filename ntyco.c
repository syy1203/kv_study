#include "nty_coroutine.h"
#include "kvstore.h"
#include <arpa/inet.h>

typedef int (*msg_handler)(char *msg, int length, char *response);
static msg_handler kvs_handler;
void server_reader(void *arg)
{
	int fd = *(int *)arg;
	int ret = 0;

	// 为每个连接维护一个缓冲区
	char buffer[4096] = {0};
	int buffer_len = 0;

	while (1)
	{
		// 读取数据到缓冲区
		ret = recv(fd, buffer + buffer_len, sizeof(buffer) - buffer_len, 0);
	
		if (ret <= 0)
		{
			close(fd);
			break;
		}

		buffer_len += ret;

		// 处理完整的数据包
		while (buffer_len >= TCP_HEADER_SIZE)
		{
			tcp_header_t header;

			// 解析协议头
			if (parse_tcp_header(buffer, &header) < 0)
			{
				printf("协议头解析失败\n");
				close(fd);
				return;
			}

			// 计算完整数据包长度
			int total_len = TCP_HEADER_SIZE + header.data_len;

			// 检查数据是否完整
			if (buffer_len < total_len)
			{
				break; // 数据不完整，等待更多数据
			}

			// 处理完整的数据包
			char response[2048] = {0};
			int resp_len = kvs_tcp_protocol(buffer, total_len, response);

			if (resp_len > 0)
			{
				// 构造响应头
				char send_buf[4096] = {0};
				uint32_t net_val;
				net_val = htonl(header.magic);    memcpy(send_buf + 0,  &net_val, 4);
				net_val = htonl(header.version);  memcpy(send_buf + 4,  &net_val, 4);
				net_val = htonl(header.cmd_type); memcpy(send_buf + 8,  &net_val, 4);
				net_val = htonl(resp_len);     memcpy(send_buf + 12, &net_val, 4);
				net_val = htonl(header.seq_id);   memcpy(send_buf + 16, &net_val, 4);
				net_val = htonl(header.flags);    memcpy(send_buf + 20, &net_val, 4);

				memcpy(send_buf + TCP_HEADER_SIZE, response, resp_len);
				int send_len = TCP_HEADER_SIZE + resp_len;

				ret = send(fd, send_buf, send_len, 0);
				if (ret == -1) {
					perror("send");
				}
			}

			// 移动缓冲区，移除已处理的数据包
			memmove(buffer, buffer + total_len, buffer_len - total_len);
			buffer_len -= total_len;
		}
	}
}



void server(void *arg)
{

	unsigned short port = *(unsigned short *)arg;

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return;

	struct sockaddr_in local, remote;
	local.sin_family = AF_INET;
	local.sin_port = htons(port);
	local.sin_addr.s_addr = INADDR_ANY;
	bind(fd, (struct sockaddr *)&local, sizeof(struct sockaddr_in));

	listen(fd, 20);
	printf("listen port : %d\n", port);

	while (1)
	{
		socklen_t len = sizeof(struct sockaddr_in);
		int cli_fd = accept(fd, (struct sockaddr *)&remote, &len);

		nty_coroutine *read_co;
		nty_coroutine_create(&read_co, server_reader, &cli_fd);
		// int *pfd = malloc(sizeof(int));
		// *pfd = cli_fd;
		// nty_coroutine_create(&read_co, server_reader, pfd);
	}
}

int ntyco_start(unsigned short port, msg_handler handler)
{

	// int port = atoi(argv[1]);
	kvs_handler = handler;

	nty_coroutine *co = NULL;
	nty_coroutine_create(&co, server, &port);

	nty_schedule_run();
}
