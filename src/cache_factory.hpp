#ifndef __CACHE_FACTORY_HPP__
#define __CACHE_FACTORY_HPP__

#include <v8.h>
#include <nan.h>
#include <node.h>
#include <geode/CacheFactory.hpp>

namespace node_gemfire {

class CacheFactory : public Nan::ObjectWrap {
 public:
  // Called from binding.cpp to initialize the system
  static void Init(v8::Local<v8::Object> exports);

  apache::geode::client::CacheFactoryPtr cacheFactoryPtr;

 protected:
  explicit CacheFactory(apache::geode::client::CacheFactoryPtr cacheFactoryPtr) :
    cacheFactoryPtr(cacheFactoryPtr),
    callback(NULL) {}

  virtual ~CacheFactory() {
  }
  static NAN_METHOD(New);
  static NAN_METHOD(AddLocator);
  static NAN_METHOD(AddServer);
  static NAN_METHOD(Create);
  static NAN_METHOD(Set);
  static NAN_METHOD(SetFreeConnectionTimeout);
  static NAN_METHOD(SetIdleTimeout);
  static NAN_METHOD(SetLoadConditioningInterval);
  static NAN_METHOD(SetMaxConnections);
  static NAN_METHOD(SetMinConnections);
  static NAN_METHOD(SetPingInterval);
  static NAN_METHOD(SetPRSingleHopEnabled);
  static NAN_METHOD(SetReadTimeout);
  static NAN_METHOD(SetRetryAttempts);

  
  Nan::Callback * callback;
 private:
  
  static inline Nan::Persistent<v8::Function> & constructor() {
    static Nan::Persistent<v8::Function> my_constructor;
    return my_constructor;
  }
};
}  // namespace node_gemfire

#endif
