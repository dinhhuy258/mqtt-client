#include "SSLSocket.h"
#include <thread>
#include "Utils.h"
#include "NetworkSecurityOptions.h"

SSLSocket::SSLSocket() :sslContext(nullptr), ssl(nullptr)
{
}

SSLSocket::~SSLSocket()
{
	if (ssl)
	{
		SSL_free(ssl);
	}

	if (sslContext)
	{
		SSL_CTX_free(sslContext);
	}
}

static int PemPasswordCallback(char *buf, int size, int rwflag, void *password)
{
	strncpy_s(buf, size, (char *)(password), size);
	buf[size - 1] = '\0';
	return(static_cast<int>(strlen(buf)));
}

bool SSLSocket::Initialize()
{
	//Initializing OpenSSL
	SSL_load_error_strings();
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	//Initializing SSL context and SSL method in this case I use SSLv23_client_method (Negotiate highest available SSL / TLS version)
	sslContext = SSL_CTX_new(SSLv23_client_method()); 
	if (sslContext == nullptr)
	{
		LOGI("SSL_CTX_new error");
		LOGI("%s\n", ERR_error_string(ERR_get_error(), NULL));
		return false;
	}
	//Configure SSL_CTX
	//Load the certificateAuthority (trustStore)
	if (NetworkSecurityOptions::certificateAuthority.empty() != SSL_SUCCESS)
	{
		if (!SSL_CTX_load_verify_locations(sslContext, NetworkSecurityOptions::certificateAuthority.c_str(), nullptr))
		{
			LOGI("SSL_CTX_load_verify_locations error");
			LOGI("%s\n", ERR_error_string(ERR_get_error(), NULL));
			SSL_CTX_free(sslContext);
			sslContext = nullptr;
			return false;
		}
	}
	else if (SSL_CTX_set_default_verify_paths(sslContext) != SSL_SUCCESS)
	{
		LOGI("SSL_CTX_set_default_verify_paths error");
		LOGI("%s\n", ERR_error_string(ERR_get_error(), NULL));
		SSL_CTX_free(sslContext);
		sslContext = nullptr;
		return false;
	}
	//Load the client's certificate (keyStore)
	if (!NetworkSecurityOptions::clientCertificate.empty())
	{
		if (SSL_CTX_use_certificate_file(sslContext, NetworkSecurityOptions::clientCertificate.c_str(), SSL_FILETYPE_PEM) != SSL_SUCCESS)
		{
			LOGI("SSL_CTX_use_certificate_chain_file error");
			LOGI("%s\n", ERR_error_string(ERR_get_error(), NULL));
			SSL_CTX_free(sslContext);
			sslContext = nullptr;
			return false;
		}
		std::string clientPrivateKey;
		if (NetworkSecurityOptions::clientPrivateKey.empty())
		{
			clientPrivateKey = NetworkSecurityOptions::clientCertificate; //The client's private key is in client certificate
		}
		else
		{
			clientPrivateKey = NetworkSecurityOptions::clientPrivateKey; //Load client's private key if it is not included in client's certificate (keyStore)
		}
		//Load private key password if it's exist
		//Note you should call these function before calling SSL_CTX_use_PrivateKey_file 
		if (!NetworkSecurityOptions::clientPrivateKeyPassword.empty())
		{
			SSL_CTX_set_default_passwd_cb(sslContext, PemPasswordCallback);
			SSL_CTX_set_default_passwd_cb_userdata(sslContext, (void*)NetworkSecurityOptions::clientPrivateKeyPassword.c_str());
		}
		if (SSL_CTX_use_PrivateKey_file(sslContext, clientPrivateKey.c_str(), SSL_FILETYPE_PEM) != SSL_SUCCESS)
		{
			LOGI("SSL_CTX_use_PrivateKey_file error");
			LOGI("%s\n", ERR_error_string(ERR_get_error(), NULL));
			SSL_CTX_free(sslContext);
			sslContext = nullptr;
			return false;
		}
	}
	// Set list of cipher
	if (SSL_CTX_set_cipher_list(sslContext, "DEFAULT") != SSL_SUCCESS)
	{
		LOGI("SSL_CTX_set_cipher_list error");
		LOGI("%s\n", ERR_error_string(ERR_get_error(), NULL));
		SSL_CTX_free(sslContext);
		sslContext = nullptr;
		return false;
	}
	SSL_CTX_set_mode(sslContext, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER | SSL_MODE_ENABLE_PARTIAL_WRITE);
	if (NetworkSecurityOptions::enableServerCertificate)
	{
		SSL_CTX_set_verify(sslContext, SSL_VERIFY_PEER, nullptr);
		SSL_CTX_set_verify_depth(sslContext, 1);
	}
	ssl = SSL_new(sslContext);
	if (ssl == nullptr)
	{
		LOGI("SSL_new error");
		LOGI("%s\n", ERR_error_string(ERR_get_error(), NULL));
		SSL_CTX_free(sslContext);
		sslContext = nullptr;
		return false;
	}
	return true;
}

void SSLSocket::Connect(std::string host, uint32_t port, std::function<void(bool)> connectedCallback)
{
	std::thread([&, host, port, connectedCallback]
	{
		//Setup TCP socket
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
		
		//Setup socket ssl
		if (SSL_set_fd(ssl, sockfd) != SSL_SUCCESS)
		{
			LOGI("SSL_set_fd error");
			LOGI("%s\n", ERR_error_string(ERR_get_error(), NULL));;
			if (connectedCallback)
			{
				connectedCallback(FAIL);
			}
			return;
		}
		if (SSL_connect(ssl) != SSL_SUCCESS)
		{
			LOGI("SSL_connect error");
			LOGI("%s\n", ERR_error_string(ERR_get_error(), NULL));
			if (connectedCallback)
			{
				connectedCallback(FAIL);
			}
			return;
		}       
		LOGI("SSL connection using %s\n", SSL_get_cipher(ssl));
		LOGI("Connected to server");
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
		if (connectedCallback)
		{
			connectedCallback(SUCCESS);
		}
		return;
	}).detach();
}

void SSLSocket::WriteData(uint8_t *data, std::size_t dataLength, std::function<void(bool, std::size_t)> sentCallback)
{
	if (dataLength == 0)
	{
		return;
	}
	std::thread([&, data, dataLength, sentCallback]
	{
		fd_set writefds;
		fd_set readfds;
		bool writeBlockedOnRead = false;
		std::size_t total = 0;
		std::size_t bytesLeft = dataLength;
		std::size_t bytesTransferred;
		int activity;
		while (total < dataLength)
		{
			FD_ZERO(&writefds);
			FD_ZERO(&readfds);
			FD_SET(sockfd, &writefds);
			FD_SET(sockfd, &readfds);
			activity = select(sockfd + 1, &readfds, &writefds, nullptr, nullptr);
			if ((activity < 0) && (errno != EINTR/*A signal was caught*/))
			{
				LOGI("Select error");
				if (sentCallback)
				{
					sentCallback(FAIL, 0);
				}
				return;
			}
			if ((FD_ISSET(sockfd, &writefds)) || (writeBlockedOnRead && FD_ISSET(sockfd, &readfds)))
			{
				writeBlockedOnRead = false;
				bytesTransferred = SSL_write(ssl, (char*)data + total, bytesLeft);
				switch (SSL_get_error(ssl, bytesTransferred))
				{
				case SSL_ERROR_NONE:
					total += bytesTransferred;
					bytesLeft -= bytesTransferred;
					break;
				case SSL_ERROR_WANT_WRITE:
					break;
				case SSL_ERROR_WANT_READ:
					writeBlockedOnRead = true;
					break;
				default:
					LOGI("SSL_write error");
					LOGI("%s\n", ERR_error_string(ERR_get_error(), NULL));
					if (sentCallback)
					{
						sentCallback(FAIL, total);
					}
					break;
				}
			}
		}
		if (sentCallback)
		{
			sentCallback(SUCCESS, total);
		}
	}).detach();
}

void SSLSocket::ReadData(uint8_t *buffer, std::size_t bytes, std::function<void(bool, std::size_t)> receivedCallback)
{
	if (bytes == 0)
	{
		return;
	}
	std::thread([&, buffer, bytes, receivedCallback]
	{
		bool readBlockedOnWrite = false;
		bool readBlocked = false;
		std::size_t total = 0;
		std::size_t bytesLeft = bytes;
		std::size_t bytesTransferred;
		fd_set readfds;
		fd_set writefds;
	read_retry:
		do 
		{
			readBlockedOnWrite = false;
			readBlocked = false;
			bytesTransferred = SSL_read(ssl, buffer + total, bytesLeft);
			switch (SSL_get_error(ssl, bytesTransferred)) {
			case SSL_ERROR_NONE:
				total += bytesTransferred;
				bytesLeft -= bytesTransferred;
				if (total == bytes)
				{
					if (receivedCallback)
					{
						receivedCallback(SUCCESS, total);
					}
					return;
				}
				break;
			case SSL_ERROR_ZERO_RETURN:
				if (receivedCallback)
				{
					receivedCallback(FAIL, total);
				}
				return;
				break;
			case SSL_ERROR_WANT_READ:
				readBlocked = true;
				break;
			case SSL_ERROR_WANT_WRITE:
				readBlockedOnWrite = true;
				break;
			default:
				LOGI("SSL_read error");
				LOGI("%s\n", ERR_error_string(ERR_get_error(), NULL));
				if (receivedCallback)
				{
					receivedCallback(FAIL, total);
				}
				return;
				break;
			}
		} 
		while (SSL_pending(ssl) && !readBlocked);
		while (true) 
		{
			FD_ZERO(&readfds);
			FD_ZERO(&writefds);
			FD_SET(sockfd, &readfds);
			FD_SET(sockfd, &writefds);
			int activity = select(sockfd + 1, &readfds, &writefds, nullptr, nullptr);
			if ((activity < 0) && (errno != EINTR/*A signal was caught*/))
			{
				LOGI("Select error");
				if (receivedCallback)
				{
					receivedCallback(FAIL, 0);
				}
				return;
			}
			else
			{
				if ((FD_ISSET(sockfd, &readfds)) || (readBlockedOnWrite && FD_ISSET(sockfd, &writefds)))
				{
					goto read_retry;
				}
			}
		}	
	}).detach();
}

void SSLSocket::Close()
{
	SSL_shutdown(ssl);
	Socket::Close();
}