#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<pthread.h>
#include"ThreadPool.h"

void working(void* arg);
void acceptConn(void* arg);
struct SockInfo
{
	struct sockaddr_in addr;
	int fd;
};
typedef struct  PoolInfo
{
	ThreadPool* p;
	int fd;
}pi;
//struct SockInfo infos[512];

int main(void)
{
	//创建通信套接字
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
	{
		perror("fd");
		return -1;
	}

	struct sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(9999);
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);

	//绑定到本地端口
	int ret = bind(fd, (struct sockaddr*)&saddr, sizeof(saddr));
	if (ret == -1)
	{
		perror("bind");
		return -1;
	}

	//设置监听
	ret = listen(fd, 128);
	if (ret == -1)
	{
		perror("listen");
		return -1;
	}

	/*int max = sizeof(infos) / sizeof(infos[0]);
	for (int i = 0; i < max; i++)
	{
		bzero(&infos[i], sizeof(infos[i]));
		infos[i].fd = -1;
	}*/
	ThreadPool* pool = ThreadPoolcreate(3, 8, 100);
	pi* p = (struct pi*)malloc(sizeof(struct pi*));
	p->p = pool;
	p->fd = fd;
	threadPoolAdd(pool, acceptConn,p);

	pthread_exit(NULL);
	return 0;
}
	


void acceptConn(void*arg)
{
	pi* pool = (pi*)arg;
	unsigned int len = sizeof(struct sockaddr_in);
	while (1)
	{
		struct SockInfo* pinfo;
		pinfo = (struct SockInfo*)malloc(sizeof(struct SockInfo*));
		pinfo->fd = accept(pool->fd, (struct sockaddr*)&pinfo->addr, &len);
		if (pinfo->fd == -1)
		{
			perror("accept");
			break;
		}
		threadPoolAdd(pool->p, working, pinfo);
	}
	close(pool->fd);
}

void working(void* arg)
{
	struct SockInfo* pinfo = (struct SockInfo*)arg;
	char ip[32];
	printf("客户端的IP地址: %s, 端口: %d\n", inet_ntop(AF_INET, &pinfo->addr.sin_addr.s_addr, ip, sizeof(ip)), ntohl(pinfo->addr.sin_port));

	while (1)
	{
		char buff[1024];
		memset(buff, 0, sizeof(buff));
		int len = recv(pinfo->fd, buff, sizeof(buff), 0);
		if (len > 0)
		{
			printf("client says %s\n", buff);
			send(pinfo->fd, buff, len, 0);
		}
		else if (len == 0)
		{
			printf("客户端已经断开连接.../n");
			break;
		}
		else
		{
			perror("recv");
			break;
		}
	}
	//关闭连接
	close(pinfo->fd);
};
