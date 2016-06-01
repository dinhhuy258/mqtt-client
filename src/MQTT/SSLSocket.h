#ifndef _SSL_SOCKET_H_
#define _SSL_SOCKET_H_
#define SSL_SUCCESS 1
#if defined(WIN32) || defined(WIN64)
// Avoid redefine when include openssl and define timeval fpr openssls
#include <winsock2.h>
#endif
#if defined(WIN32)
#pragma comment (lib, "libeay32.lib")
#pragma comment (lib, "ssleay32.lib")
#elif defined(WIN64)
#pragma comment (lib, "libeay64.lib")
#pragma comment (lib, "ssleay64.lib")
#endif
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "Socket.h"

class SSLSocket : public Socket
{
	public:
		SSLSocket();
		~SSLSocket();
		bool Initialize() override;
		void Connect(std::string host, uint32_t port, std::function<void(bool)> connectedCallback) override;
		void WriteData(uint8_t *data, std::size_t dataLength, std::function<void(bool, std::size_t)> sentCallback) override;
		void ReadData(uint8_t *buffer, std::size_t bytes, std::function<void(bool, std::size_t)> receivedCallback) override;
		void Close() override;
private:
		SSL_CTX *sslContext;
		SSL *ssl;
};

#endif //_SSL_SOCKET_H_