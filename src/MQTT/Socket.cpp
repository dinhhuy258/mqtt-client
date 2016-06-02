#include "Socket.h"

void Socket::Close()
{
#if defined(WIN32) || defined(WIN64)
	closesocket(sockfd);
#else
	close(sockfd);
#endif
}

bool Socket::SetSocketBlockingEnabled(bool blocking)
{
#if defined(WIN32) || defined(WIN64)
	unsigned long flags = blocking ? 0 : 1;
	return (ioctlsocket(sockfd, FIONBIO, &flags) == 0) ? true : false;
#else
	int flags = fcntl(sockfd, F_GETFL, 0);
	if (flags < 0)
	{
		return false;
	}
	flags = blocking ? (flags&~O_NONBLOCK) : (flags | O_NONBLOCK);
	return (fcntl(sockfd, F_SETFL, flags) == 0) ? true : false;
#endif
}