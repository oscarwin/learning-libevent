#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <strings.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 100

int main(int argc, char const *argv[])
{
	int iSockFd;
	int n;
	struct sockaddr_in stServAddr;
	char sendBuf[BUFSIZE];
	char recvBuf[BUFSIZE];

	bzero(&stServAddr, sizeof(stServAddr));
	bzero(sendBuf, BUFSIZE);
	bzero(recvBuf, BUFSIZE);

	if ((iSockFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket error");
		return 1;
	}

	stServAddr.sin_family = AF_INET;
	stServAddr.sin_port = htons(9999);
	inet_pton(AF_INET, "127.0.0.1", (void*)&stServAddr.sin_addr);
	if (connect(iSockFd, (struct sockaddr*)&stServAddr, sizeof(stServAddr)) < 0)
	{
		perror("connet error");
		return 1;
	}

    // 从服务读取
    if ((n = read(iSockFd, recvBuf, BUFSIZE)) < 0)
    {
        perror("write error");
        return 1;
    }

    // 输出到标准输出
    if (write(STDOUT_FILENO, recvBuf, n) < 0)
    {
        perror("write error");
        return 1;
    }

	close(iSockFd);

	return 0;
}