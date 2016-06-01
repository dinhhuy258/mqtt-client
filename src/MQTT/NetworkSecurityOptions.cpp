#include "NetworkSecurityOptions.h"

std::string NetworkSecurityOptions::certificateAuthority = std::string();
std::string NetworkSecurityOptions::clientCertificate = std::string();
std::string NetworkSecurityOptions::clientPrivateKey = std::string();
std::string NetworkSecurityOptions::clientPrivateKeyPassword = std::string();
bool NetworkSecurityOptions::enableServerCertificate = true;