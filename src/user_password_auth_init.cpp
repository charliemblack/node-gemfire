#include "UserPasswordAuthInit.hpp"
#include <geode/Properties.hpp>
#include <geode/ExceptionTypes.hpp>

#define SECURITY_USERNAME "security-username"
#define SECURITY_PASSWORD "security-password"

namespace node_gemfire {

extern "C" {
LIBEXP AuthInitialize* createUserPasswordAuthInitInstance() {
  return new UserPasswordAuthInit();
}
}

PropertiesPtr UserPasswordAuthInit::getCredentials(PropertiesPtr& securityprops, const char* server) {
  CacheablePtr userName;
  if (securityprops == NULLPTR || (userName = securityprops->find(SECURITY_USERNAME)) == NULLPTR) {
    throw AuthenticationFailedException("UserPasswordAuthInit: user name " "property [" SECURITY_USERNAME "] not set.");
  }
  PropertiesPtr credentials = Properties::create();
  credentials->insert(SECURITY_USERNAME, userName->toString()->asChar());
  CacheablePtr passwd = securityprops->find(SECURITY_PASSWORD);
  if (passwd == NULLPTR) {
    passwd = CacheableString::create("");
  }
  credentials->insert(SECURITY_PASSWORD, passwd->toString()->asChar());
  return credentials;
}
}  // namespace node_gemfire
