#include "exceptions.hpp"
#include <nan.h>

using namespace v8;
using namespace apache::geode::client;

namespace node_gemfire {

Local<Value> v8Error(const apache::geode::client::Exception & exception) {
  NanEscapableScope();

  Local<Object> error(NanError(exception.getMessage())->ToObject());
  error->Set(NanNew("name"), NanNew(exception.getName()));

  return NanEscapeScope(error);
}

Local<Value> v8Error(const UserFunctionExecutionExceptionPtr & exceptionPtr) {
  NanEscapableScope();

  Local<Object> error(NanError(exceptionPtr->getMessage()->asChar())->ToObject());
  error->Set(NanNew("name"), NanNew(exceptionPtr->getName()->asChar()));

  return NanEscapeScope(error);
}

void ThrowGemfireException(const apache::geode::client::Exception & e) {
  NanThrowError(v8Error(e));
}

}  // namespace node_gemfire
