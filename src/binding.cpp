#define NODE_GEMFIRE_VERSION "0.1.19"

#include <v8.h>
#include <nan.h>
#include <geode/CacheFactory.hpp>
#include "dependencies.hpp"
#include "cache.hpp"
#include "region.hpp"
#include "select_results.hpp"

using namespace v8;
using namespace apache::geode::client;

namespace node_gemfire {

NAN_METHOD(Connected) {
  Nan::HandleScope scope;
  DistributedSystemPtr distributedSystemPtr = DistributedSystem::getInstance();
  info.GetReturnValue().Set(Nan::New(distributedSystemPtr->isConnected()));
}

NAN_METHOD(Initialize) {
  Nan::HandleScope scope;

  Local<Object> gemfire = Nan::New<Object>();

  Nan::DefineOwnProperty(gemfire, Nan::New("version").ToLocalChecked(),
      Nan::New(NODE_GEMFIRE_VERSION).ToLocalChecked(),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  Nan::DefineOwnProperty(gemfire, Nan::New("gemfireVersion").ToLocalChecked(),
      Nan::New(std::string(apache::geode::client::CacheFactory::getVersion())).ToLocalChecked(),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  Nan::DefineOwnProperty(gemfire, Nan::New("connected").ToLocalChecked(),
      Nan::New<FunctionTemplate>(Connected)->GetFunction(),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  node_gemfire::Cache::Init(gemfire);
  node_gemfire::Region::Init(gemfire);
  node_gemfire::SelectResults::Init(gemfire);

  dependencies.Reset(v8::Isolate::GetCurrent(),info[0]->ToObject());

  info.GetReturnValue().Set(gemfire);

}

}  // namespace node_gemfire

static void Initialize(Local<Object> exports) {
  Local<FunctionTemplate> initializeTemplate(Nan::New<FunctionTemplate>(node_gemfire::Initialize));
  exports->Set(Nan::New("initialize").ToLocalChecked(), initializeTemplate->GetFunction());
}

NODE_MODULE(gemfire, Initialize)
