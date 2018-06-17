#ifndef __USER_PASSWORD_AUTH_INIT.HPP__
#define __USER_PASSWORD_AUTH_INIT.HPP__
#include <geode/AuthInitialize.hpp>


namespace node_gemfire {

class UserPasswordAuthInit : public AuthInitialize {
 public:
  UserPasswordAuthInit() {}
  ~UserPasswordAuthInit() {}

  PropertiesPtr getCredentials(PropertiesPtr& securityprops, const char* server);
  void close() { return; }
}
}  // namespace node_gemfire
#endif  //__USER_PASSWORD_AUTH_INIT.HPP__
