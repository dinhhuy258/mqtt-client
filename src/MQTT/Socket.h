#ifndef _SOCKET_H_
#define _SOCKET_H_
#define SUCCESS false
#define FAIL true
#if defined(WIN32) || defined(WIN64)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#define INVALID_SOCKET -1
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif
#include <functional>

class Socket
{
	public:
		Socket() = default;
		virtual ~Socket() = default;
		virtual bool Initialize() = 0;
		virtual void Connect(std::string host, uint32_t port, std::function<void(bool)> connectedCallback) = 0;
		virtual void WriteData(uint8_t *data, std::size_t dataLength, std::function<void(bool, std::size_t)> sentCallback) = 0;
		virtual void ReadData(uint8_t *buffer, std::size_t bytes, std::function<void(bool, std::size_t)> receivedCallback) = 0;
		virtual void Close();
	protected:
		bool SetSocketBlockingEnabled(bool blocking);
		int sockfd;
};

#endif