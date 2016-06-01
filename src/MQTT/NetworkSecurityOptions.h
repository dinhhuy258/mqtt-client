#ifndef _NETWORK_SECURITY_OPTIONS_H_
#define _NETWORK_SECURITY_OPTIONS_H_
#include <string>

class NetworkSecurityOptions
{
	public:
		NetworkSecurityOptions() = delete;
		~NetworkSecurityOptions() = delete;
	public:
		//The file in PEM format containing the public digital certificates trusted by the client
		static std::string certificateAuthority;
		//The file in PEM format containing the public certificate chain of the client. It may also include the client's private key
		static std::string clientCertificate;
		//If private key not included in the clientCertificate, this setting points to the file in PEM format containing the client's private key
		static std::string clientPrivateKey;
		//The password to load the client's private key if encrypted
		static std::string clientPrivateKeyPassword;
		//Enable verification of the server certificate 
		static bool enableServerCertificate;
};

#endif //_NETWORK_SECURITY_OPTIONS_H_