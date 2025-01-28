#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>


int main()
{

	int lfd = socket(AF_INET, SOCK_STREAM, 0);//���������׽���
	if (lfd == -1)
	{
		perror("socket");
		exit(0);
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(9999);

	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	//��socket()����ֵ�ͱ��ص�IP�˿ڰ󶨵�һ��
	int ret = bind(lfd, (struct sockaddr*)&addr, sizeof(addr));
	if (ret == -1)
	{
		perror("bind");
		exit(0);
	}

	//���ü���
	ret = listen(lfd, 128);
	if (ret == -1)
	{
		perror("listen");
		exit(0);
	}

	//�����ȴ������ܿͻ�����
	struct sockaddr_in cliaddr;
	unsigned int clilen = sizeof(cliaddr);
	int cfd = accept(lfd, (struct sockaddr*)&cliaddr, &clilen);
	if (cfd == -1)
	{
		perror("accept");
		exit(0);
	}

	char ip[24] = { 0 };
	printf("�ͻ��˵�IP��ַ: %s, �˿�: %d\n",
		inet_ntop(AF_INET, &cliaddr.sin_addr.s_addr, ip, sizeof(ip)),
		ntohs(cliaddr.sin_port));

	while (1)
	{
		char buf[1024];
		memset(buf, 0, sizeof(buf));
		int len = read(cfd, buf, sizeof(buf));
		if (len > 0)
		{
			printf("�ͻ���say:%s\n", buf);
			write(cfd, buf, len);
		}
		else if (len == 0)
		{
			printf("�ͻ��˶Ͽ�������...\n");
			break;
		}
		else
		{
			perror("read");
			break;
		}
	}
	close(cfd);
	close(lfd);

	return 0;
}