#include "TCPSocket.h"
#include <thread>
#include "Utils.h"

void TCPSocket::Connect(std::string host, uint32_t port, std::function<void(bool)> connectedCallback)
{
	std::thread([&, host, port, connectedCallback]
	{
		//Create socket
		struct sockaddr_in serverAddress;
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd == INVALID_SOCKET)
		{
			LOGI("Create socket fail");
			if (connectedCallback)
			{
				connectedCallback(FAIL);
			}
			return;
		}
		char opt = 1;
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0)
		{
			LOGI("Set socket options error");
			if (connectedCallback)
			{
				connectedCallback(FAIL);
			}
			return;
		}
		memset(&serverAddress, 0, sizeof(serverAddress));
		serverAddress.sin_family = AF_INET;
		if (inet_pton(AF_INET, host.c_str(), &serverAddress.sin_addr.S_un.S_addr) <= 0)
		{
			LOGI("Invalid address");
			if (connectedCallback)
			{
				connectedCallback(FAIL);
			}
			return;
		}
		serverAddress.sin_port = htons(port);
		//Connect to server
		if (connect(sockfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
		{
			LOGI("Failed to connect to server");
			if (connectedCallback)
			{
				connectedCallback(FAIL);
			}
			return;
		}
		//Set socket nonblocking
		if (!SetSocketBlockingEnabled(false))
		{
			LOGI("Set socket blocking error");
			if (connectedCallback)
			{
				connectedCallback(FAIL);
			}
			return;
		}
		LOGI("Connected to server");
		if (connectedCallback)
		{
			connectedCallback(SUCCESS);
		}
		return;
	}).detach();
}

void TCPSocket::WriteData(uint8_t *data, std::size_t dataLength, std::function<void(bool, std::size_t)> sentCallback)
{
	if (dataLength == 0)
	{
		return;
	}
	std::thread([&, data, dataLength, sentCallback]
	{
		fd_set writefds;
		std::size_t total = 0;
		std::size_t bytesLeft = dataLength;
		std::size_t bytesTransferred;
		int activity;
		while (total < dataLength)
		{
			FD_ZERO(&writefds);
			FD_SET(sockfd, &writefds);
			activity = select(sockfd + 1, nullptr, &writefds, nullptr, nullptr);
			if ((activity < 0) && (errno != EINTR/*A signal was caught*/))
			{
				LOGI("Select error");
				if (sentCallback)
				{
					sentCallback(FAIL, 0);
				}
				return;
			}
			if (FD_ISSET(sockfd, &writefds))
			{
				bytesTransferred = send(sockfd, (char*)data + total, bytesLeft, 0);
				if (bytesTransferred <= 0)
				{
					if (sentCallback)
					{
						sentCallback(FAIL, total);
					}
					break;
				}
				total += bytesTransferred;
				bytesLeft -= bytesTransferred;
			}
		}
		if (sentCallback)
		{
			sentCallback(SUCCESS, total);
		}
	}).detach();
}

void TCPSocket::ReadData(uint8_t *buffer, std::size_t bytes, std::function<void(bool, std::size_t)> receivedCallback)
{
	if (bytes == 0)
	{
		return;
	}
	std::thread([&, buffer, bytes, receivedCallback]
	{
		fd_set readfds;
		std::size_t total = 0;
		std::size_t bytesLeft = bytes;
		std::size_t bytesTransferred;
		int activity;
		while (total < bytes)
		{
			FD_ZERO(&readfds);
			FD_SET(sockfd, &readfds);
			activity = select(sockfd + 1, &readfds, nullptr, nullptr, nullptr);
			if ((activity < 0) && (errno != EINTR/*A signal was caught*/))
			{
				LOGI("Select error");
				if (receivedCallback)
				{
					receivedCallback(FAIL, 0);
				}
				return;
			}
			if (FD_ISSET(sockfd, &readfds))
			{
				bytesTransferred = recv(sockfd, (char*)buffer + total, bytesLeft, 0);
				if (bytesTransferred <= 0)
				{
					LOGI("Read data fail");
					if (receivedCallback)
					{
						receivedCallback(FAIL, 0);
					}
					return;
				}
				total += bytesTransferred;
				bytesLeft -= bytesTransferred;
			}
		}
		if (receivedCallback)
		{
			receivedCallback(SUCCESS, bytes);
		}
	}).detach();
}

