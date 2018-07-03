#include "cache_factory.hpp"
#include "cache.hpp"
#include <v8.h>
#include <node.h>
#include <nan.h>
#include <geode/Cache.hpp>
#include <geode/CacheFactory.hpp>
#include <geode/Region.hpp>
#include <string>
#include <sstream>
#include "exceptions.hpp"
#include "conversions.hpp"
#include "region.hpp"
#include "gemfire_worker.hpp"
#include "dependencies.hpp"
#include "functions.hpp"
#include "region_shortcuts.hpp"

using namespace v8;

namespace node_gemfire {


NAN_MODULE_INIT(CacheFactory::Init) {
  Nan::HandleScope scope;
  v8::Local<v8::FunctionTemplate> constructorTemplate = Nan::New<v8::FunctionTemplate>(New);
  constructorTemplate->SetClassName(Nan::New("CacheFactory").ToLocalChecked());
  
  constructorTemplate->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(constructorTemplate, "addLocator", CacheFactory::AddLocator);
  Nan::SetPrototypeMethod(constructorTemplate, "addServer", CacheFactory::AddServer);
  Nan::SetPrototypeMethod(constructorTemplate, "create", CacheFactory::Create);
  Nan::SetPrototypeMethod(constructorTemplate, "set", CacheFactory::Set);
  Nan::SetPrototypeMethod(constructorTemplate, "setFreeConnectionTimeout", CacheFactory::SetFreeConnectionTimeout);
  Nan::SetPrototypeMethod(constructorTemplate, "setIdleTimeout", CacheFactory::SetIdleTimeout);
  Nan::SetPrototypeMethod(constructorTemplate, "setLoadConditioningInterval", CacheFactory::SetLoadConditioningInterval);
  Nan::SetPrototypeMethod(constructorTemplate, "setMaxConnections", CacheFactory::SetMaxConnections);
  Nan::SetPrototypeMethod(constructorTemplate, "setMinConnections", CacheFactory::SetMinConnections);
  Nan::SetPrototypeMethod(constructorTemplate, "setPdxIgnoreUnreadFields", CacheFactory::SetPdxIgnoreUnreadFields);
  Nan::SetPrototypeMethod(constructorTemplate, "setPingInterval", CacheFactory::SetPingInterval);
  Nan::SetPrototypeMethod(constructorTemplate, "setPRSingleHopEnabled", CacheFactory::SetPRSingleHopEnabled);
  Nan::SetPrototypeMethod(constructorTemplate, "setReadTimeout", CacheFactory::SetReadTimeout);
  Nan::SetPrototypeMethod(constructorTemplate, "setRetryAttempts", CacheFactory::SetRetryAttempts);
  Nan::SetPrototypeMethod(constructorTemplate, "setServerGroup", CacheFactory::SetServerGroup);
  Nan::SetPrototypeMethod(constructorTemplate, "setSocketBufferSize", CacheFactory::SetSocketBufferSize);
  Nan::SetPrototypeMethod(constructorTemplate, "setStatisticInterval", CacheFactory::SetStatisticInterval);
  Nan::SetPrototypeMethod(constructorTemplate, "setSubscriptionAckInterval", CacheFactory::SetSubscriptionAckInterval);
  Nan::SetPrototypeMethod(constructorTemplate, "setSubscriptionEnabled", CacheFactory::SetSubscriptionEnabled);
  Nan::SetPrototypeMethod(constructorTemplate, "setSubscriptionMessageTrackingTimeout", CacheFactory::SetSubscriptionMessageTrackingTimeout);
  Nan::SetPrototypeMethod(constructorTemplate, "setSubscriptionRedundancy", CacheFactory::SetSubscriptionRedundancy);
  Nan::SetPrototypeMethod(constructorTemplate, "setThreadLocalConnections", CacheFactory::SetThreadLocalConnections);
  Nan::SetPrototypeMethod(constructorTemplate, "setUpdateLocatorListInterval", CacheFactory::SetUpdateLocatorListInterval);

  constructor().Reset(Nan::GetFunction(constructorTemplate).ToLocalChecked());

  Nan::Set(target, Nan::New("CacheFactory").ToLocalChecked(), Nan::GetFunction(constructorTemplate).ToLocalChecked());
}

NAN_METHOD(CacheFactory::New) {
  Nan::HandleScope scope;

  if(info.Length() != 2){
    Nan::ThrowError("You must call the CacheFactory with a properties file location and a callback.");
    info.GetReturnValue().Set(Nan::Undefined());
    return;
  }
  apache::geode::client::CacheFactoryPtr cacheFactoryPtr;
  if (info[0]->IsString()) {
    apache::geode::client::PropertiesPtr gemfireProperties = apache::geode::client::Properties::create();
    gemfireProperties->load(*Nan::Utf8String(info[0]));
    cacheFactoryPtr = apache::geode::client::CacheFactory::createCacheFactory(gemfireProperties);
  } else {
    cacheFactoryPtr = apache::geode::client::CacheFactory::createCacheFactory();
  }
  CacheFactory * cacheFactory = new CacheFactory(cacheFactoryPtr);
  cacheFactory->callback = new Nan::Callback(Local<Function>::Cast(info[1]));
  cacheFactory->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(CacheFactory::AddLocator) {
  Nan::HandleScope scope;
  int argsLength = info.Length();
  if(argsLength != 2 && info[0]->IsString() && info[1]->IsNumber()){
    Nan::ThrowError("You must pass the address and the port of the locator.");
    return;
  }
  CacheFactory * cacheFactory = Nan::ObjectWrap::Unwrap<CacheFactory>(info.This());
  cacheFactory->cacheFactoryPtr->addLocator(*Nan::Utf8String(info[0]), Nan::To<int>(info[1]).FromJust());
  info.GetReturnValue().Set(info.This());
}
NAN_METHOD(CacheFactory::AddServer) {
  Nan::HandleScope scope;
  int argsLength = info.Length();
  if(argsLength != 2 && info[0]->IsString() && info[1]->IsNumber()){
    Nan::ThrowError("You must pass the address and the port of the server.");
    return;
  }
  CacheFactory * cacheFactory = Nan::ObjectWrap::Unwrap<CacheFactory>(info.This());
  cacheFactory->cacheFactoryPtr->addServer(*Nan::Utf8String(info[0]), Nan::To<int>(info[1]).FromJust());
  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(CacheFactory::Set) {
  Nan::HandleScope scope;
  int argsLength = info.Length();
  if(argsLength != 2 && info[0]->IsString() && info[1]->IsString()){
    Nan::ThrowError("You must pass the property name and the property value.");
    return;
  }
  CacheFactory * cacheFactory = Nan::ObjectWrap::Unwrap<CacheFactory>(info.This());
  cacheFactory->cacheFactoryPtr->set(*Nan::Utf8String(info[0]), *Nan::Utf8String(info[1]));
  info.GetReturnValue().Set(info.This());
}


NAN_METHOD(CacheFactory::SetFreeConnectionTimeout) {
  Nan::HandleScope scope;
  int argsLength = info.Length();
  if(argsLength != 1 && info[0]->IsNumber()){
    Nan::ThrowError("You must pass an integer value.");
    return;
  }
  CacheFactory * cacheFactory = Nan::ObjectWrap::Unwrap<CacheFactory>(info.This());
  cacheFactory->cacheFactoryPtr->setFreeConnectionTimeout(Nan::To<int>(info[1]).FromJust());
  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(CacheFactory::SetIdleTimeout) {
  Nan::HandleScope scope;
  int argsLength = info.Length();
  if(argsLength != 1 && info[0]->IsNumber()){
    Nan::ThrowError("You must pass an integer value.");
    return;
  }
  CacheFactory * cacheFactory = Nan::ObjectWrap::Unwrap<CacheFactory>(info.This());
  cacheFactory->cacheFactoryPtr->setIdleTimeout(Nan::To<int>(info[1]).FromJust());
  info.GetReturnValue().Set(info.This());
}


NAN_METHOD(CacheFactory::SetLoadConditioningInterval) {
  Nan::HandleScope scope;
  int argsLength = info.Length();
  if(argsLength != 1 && info[0]->IsNumber()){
    Nan::ThrowError("You must pass an integer value.");
    return;
  }
  CacheFactory * cacheFactory = Nan::ObjectWrap::Unwrap<CacheFactory>(info.This());
  cacheFactory->cacheFactoryPtr->setLoadConditioningInterval(Nan::To<int>(info[1]).FromJust());
  info.GetReturnValue().Set(info.This());
}
NAN_METHOD(CacheFactory::SetMaxConnections) {
  Nan::HandleScope scope;
  int argsLength = info.Length();
  if(argsLength != 1 && info[0]->IsNumber()){
    Nan::ThrowError("You must pass an integer value.");
    return;
  }
  CacheFactory * cacheFactory = Nan::ObjectWrap::Unwrap<CacheFactory>(info.This());
  cacheFactory->cacheFactoryPtr->setMaxConnections(Nan::To<int>(info[1]).FromJust());
  info.GetReturnValue().Set(info.This());
}
NAN_METHOD(CacheFactory::SetMinConnections) {
  Nan::HandleScope scope;
  int argsLength = info.Length();
  if(argsLength != 1 && info[0]->IsNumber()){
    Nan::ThrowError("You must pass an integer value.");
    return;
  }
  CacheFactory * cacheFactory = Nan::ObjectWrap::Unwrap<CacheFactory>(info.This());
  cacheFactory->cacheFactoryPtr->setMinConnections(Nan::To<int>(info[1]).FromJust());
  info.GetReturnValue().Set(info.This());
}
NAN_METHOD(CacheFactory::SetPdxIgnoreUnreadFields) {
  Nan::HandleScope scope;
  int argsLength = info.Length();
  if(argsLength != 1 && info[0]->IsBoolean()){
    Nan::ThrowError("You must pass an bool value.");
    return;
  }
  CacheFactory * cacheFactory = Nan::ObjectWrap::Unwrap<CacheFactory>(info.This());
  cacheFactory->cacheFactoryPtr->setPdxIgnoreUnreadFields(Nan::To<bool>(info[1]).FromJust());
  info.GetReturnValue().Set(info.This());
}
NAN_METHOD(CacheFactory::SetPingInterval) {
  Nan::HandleScope scope;
  int argsLength = info.Length();
  if(argsLength != 1 && info[0]->IsNumber()){
    Nan::ThrowError("You must pass an long value.");
    return;
  }
  CacheFactory * cacheFactory = Nan::ObjectWrap::Unwrap<CacheFactory>(info.This());
  cacheFactory->cacheFactoryPtr->setPingInterval(Nan::To<long>(info[1]).FromJust());
  info.GetReturnValue().Set(info.This());
}
NAN_METHOD(CacheFactory::SetPRSingleHopEnabled) {
  Nan::HandleScope scope;
  int argsLength = info.Length();
  if(argsLength != 1 && info[0]->IsBoolean()){
    Nan::ThrowError("You must pass an bool value.");
    return;
  }
  CacheFactory * cacheFactory = Nan::ObjectWrap::Unwrap<CacheFactory>(info.This());
  cacheFactory->cacheFactoryPtr->setPRSingleHopEnabled(Nan::To<bool>(info[1]).FromJust());
  info.GetReturnValue().Set(info.This());
}
NAN_METHOD(CacheFactory::SetReadTimeout) {
  Nan::HandleScope scope;
  int argsLength = info.Length();
  if(argsLength != 1 && info[0]->IsNumber()){
    Nan::ThrowError("You must pass an integer value.");
    return;
  }
  CacheFactory * cacheFactory = Nan::ObjectWrap::Unwrap<CacheFactory>(info.This());
  cacheFactory->cacheFactoryPtr->setReadTimeout(Nan::To<int>(info[1]).FromJust());
  info.GetReturnValue().Set(info.This());
}
NAN_METHOD(CacheFactory::SetRetryAttempts) {
  Nan::HandleScope scope;
  int argsLength = info.Length();
  if(argsLength != 1 && info[0]->IsNumber()){
    Nan::ThrowError("You must pass an integer value.");
    return;
  }
  CacheFactory * cacheFactory = Nan::ObjectWrap::Unwrap<CacheFactory>(info.This());
  cacheFactory->cacheFactoryPtr->setRetryAttempts(Nan::To<int>(info[1]).FromJust());
  info.GetReturnValue().Set(info.This());
}
NAN_METHOD(CacheFactory::SetServerGroup) {
  Nan::HandleScope scope;
  int argsLength = info.Length();
  if(argsLength != 1 && info[0]->IsString()){
    Nan::ThrowError("You must pass the server group name as a string.");
    return;
  }
  CacheFactory * cacheFactory = Nan::ObjectWrap::Unwrap<CacheFactory>(info.This());
  cacheFactory->cacheFactoryPtr->setServerGroup(*Nan::Utf8String(info[0]));
  info.GetReturnValue().Set(info.This());
}
NAN_METHOD(CacheFactory::SetSocketBufferSize) {
  Nan::HandleScope scope;
  int argsLength = info.Length();
  if(argsLength != 1 && info[0]->IsNumber()){
    Nan::ThrowError("You must pass an integer value.");
    return;
  }
  CacheFactory * cacheFactory = Nan::ObjectWrap::Unwrap<CacheFactory>(info.This());
  cacheFactory->cacheFactoryPtr->setSocketBufferSize(Nan::To<int>(info[1]).FromJust());
  info.GetReturnValue().Set(info.This());
}
NAN_METHOD(CacheFactory::SetStatisticInterval) {
  Nan::HandleScope scope;
  int argsLength = info.Length();
  if(argsLength != 1 && info[0]->IsNumber()){
    Nan::ThrowError("You must pass an integer value.");
    return;
  }
  CacheFactory * cacheFactory = Nan::ObjectWrap::Unwrap<CacheFactory>(info.This());
  cacheFactory->cacheFactoryPtr->setStatisticInterval(Nan::To<int>(info[1]).FromJust());
  info.GetReturnValue().Set(info.This());
}
NAN_METHOD(CacheFactory::SetSubscriptionAckInterval) {
  Nan::HandleScope scope;
  int argsLength = info.Length();
  if(argsLength != 1 && info[0]->IsNumber()){
    Nan::ThrowError("You must pass an integer value.");
    return;
  }
  CacheFactory * cacheFactory = Nan::ObjectWrap::Unwrap<CacheFactory>(info.This());
  cacheFactory->cacheFactoryPtr->setSubscriptionAckInterval(Nan::To<int>(info[1]).FromJust());
  info.GetReturnValue().Set(info.This());
}
NAN_METHOD(CacheFactory::SetSubscriptionEnabled) {
  Nan::HandleScope scope;
  int argsLength = info.Length();
  if(argsLength != 1 && info[0]->IsBoolean()){
    Nan::ThrowError("You must pass an bool value.");
    return;
  }
  CacheFactory * cacheFactory = Nan::ObjectWrap::Unwrap<CacheFactory>(info.This());
  cacheFactory->cacheFactoryPtr->setSubscriptionEnabled(Nan::To<bool>(info[1]).FromJust());
  info.GetReturnValue().Set(info.This());
}
NAN_METHOD(CacheFactory::SetSubscriptionMessageTrackingTimeout) {
  Nan::HandleScope scope;
  int argsLength = info.Length();
  if(argsLength != 1 && info[0]->IsNumber()){
    Nan::ThrowError("You must pass an integer value.");
    return;
  }
  CacheFactory * cacheFactory = Nan::ObjectWrap::Unwrap<CacheFactory>(info.This());
  cacheFactory->cacheFactoryPtr->setSubscriptionMessageTrackingTimeout(Nan::To<int>(info[1]).FromJust());
  info.GetReturnValue().Set(info.This());
}
NAN_METHOD(CacheFactory::SetSubscriptionRedundancy) {
  Nan::HandleScope scope;
  int argsLength = info.Length();
  if(argsLength != 1 && info[0]->IsNumber()){
    Nan::ThrowError("You must pass an integer value.");
    return;
  }
  CacheFactory * cacheFactory = Nan::ObjectWrap::Unwrap<CacheFactory>(info.This());
  cacheFactory->cacheFactoryPtr->setSubscriptionRedundancy(Nan::To<int>(info[1]).FromJust());
  info.GetReturnValue().Set(info.This());
}
NAN_METHOD(CacheFactory::SetThreadLocalConnections) {
  Nan::HandleScope scope;
  int argsLength = info.Length();
  if(argsLength != 1 && info[0]->IsBoolean()){
    Nan::ThrowError("You must pass an bool value.");
    return;
  }
  CacheFactory * cacheFactory = Nan::ObjectWrap::Unwrap<CacheFactory>(info.This());
  cacheFactory->cacheFactoryPtr->setThreadLocalConnections(Nan::To<bool>(info[1]).FromJust());
  info.GetReturnValue().Set(info.This());
}
NAN_METHOD(CacheFactory::SetUpdateLocatorListInterval) {
  Nan::HandleScope scope;
  int argsLength = info.Length();
  if(argsLength != 1 && info[0]->IsNumber()){
    Nan::ThrowError("You must pass an long value.");
    return;
  }
  CacheFactory * cacheFactory = Nan::ObjectWrap::Unwrap<CacheFactory>(info.This());
  cacheFactory->cacheFactoryPtr->setUpdateLocatorListInterval(Nan::To<long>(info[1]).FromJust());
  info.GetReturnValue().Set(info.This());
}
NAN_METHOD(CacheFactory::Create) {
  Nan::HandleScope scope;
  CacheFactory * cacheFactory = Nan::ObjectWrap::Unwrap<CacheFactory>(info.This());
  apache::geode::client::CacheFactoryPtr cacheFactoryPtr = cacheFactory->cacheFactoryPtr;

  // Force the PDX Read Serialized to true no matter what
  cacheFactoryPtr->setPdxReadSerialized(true);

  v8::Local<v8::Object> cache = Cache::NewInstance(cacheFactoryPtr->create());
  
  if(cacheFactory->callback != NULL){
    Local<Value> argv[1] = { cache };
    Nan::Call(*(cacheFactory->callback), 1, argv);
  }
  info.GetReturnValue().Set(cache);
}
}  // namespace node_gemfire
