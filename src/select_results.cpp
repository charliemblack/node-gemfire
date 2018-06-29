#include <geode/SelectResultsIterator.hpp>
#include <sstream>
#include "conversions.hpp"
#include "select_results.hpp"

using namespace v8;
using namespace apache::geode::client;

namespace node_gemfire {

NAN_MODULE_INIT(SelectResults::Init){
  Nan::HandleScope scope;

  Local<FunctionTemplate> constructorTemplate = Nan::New<FunctionTemplate>();

  constructorTemplate->SetClassName(Nan::New("SelectResults").ToLocalChecked());
  constructorTemplate->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(constructorTemplate, "toArray", SelectResults::ToArray);
  Nan::SetPrototypeMethod(constructorTemplate, "each", SelectResults::Each);
  Nan::SetPrototypeMethod(constructorTemplate, "inspect", SelectResults::Inspect);

  constructor().Reset(Nan::GetFunction(constructorTemplate).ToLocalChecked());

  Nan::Set(target, Nan::New("SelectResults").ToLocalChecked(), Nan::GetFunction(constructorTemplate).ToLocalChecked());
}

Local<Object> SelectResults::NewInstance(const SelectResultsPtr & selectResultsPtr) {
  Nan::EscapableHandleScope scope;
 const unsigned int argc = 0;
  #ifdef __linux__
  #Local<Value> argv[argc] = {};
  #endif

  #ifdef __linux__
  #Local<Object> instance(Nan::New(SelectResults::constructor())->NewInstance(argc, argv));
  #else
  #Local<Object> instance(Nan::New(SelectResults::constructor())->NewInstance(argc, NULL));
  #endif
  SelectResults * selectResults = new SelectResults(selectResultsPtr);
  selectResults->Wrap(instance);

  return scope.Escape(instance);
}

NAN_METHOD(SelectResults::ToArray) {
  Nan::HandleScope scope;

  SelectResults * selectResults = Nan::ObjectWrap::Unwrap<SelectResults>(info.Holder());
  SelectResultsPtr selectResultsPtr(selectResults->selectResultsPtr);

  unsigned int length = selectResultsPtr->size();

  Local<Array> array(Nan::New<Array>(length));
  for (unsigned int i = 0; i < length; i++) {
    array->Set(i, v8Value((*selectResultsPtr)[i]));
  }
  info.GetReturnValue().Set(array);
}

NAN_METHOD(SelectResults::Each) {
  Nan::HandleScope scope;

  if (info.Length() == 0 || !info[0]->IsFunction()) {
    Nan::ThrowError("You must pass a callback to each()");
    info.GetReturnValue().Set(Nan::Undefined());
    return;
  }

  SelectResults * selectResults = Nan::ObjectWrap::Unwrap<SelectResults>(info.Holder());
  SelectResultsPtr selectResultsPtr(selectResults->selectResultsPtr);

  SelectResultsIterator iterator(selectResultsPtr->getIterator());
  Nan::Callback callback(Local<Function>::Cast(info[0]));

  while (iterator.hasNext()) {
    const unsigned int argc = 1;
    Local<Value> argv[argc] = { v8Value(iterator.next()) };
    callback(1, argv);
  }
  info.GetReturnValue().Set(info.Holder());
}

NAN_METHOD(SelectResults::Inspect) {
  Nan::HandleScope scope;

  SelectResults * selectResults =  Nan::ObjectWrap::Unwrap<SelectResults>(info.Holder());
  SelectResultsPtr selectResultsPtr(selectResults->selectResultsPtr);

  std::stringstream inspectStream;
  inspectStream << "[SelectResults size=" << selectResultsPtr->size() << "]";

  info.GetReturnValue().Set(Nan::New(inspectStream.str()).ToLocalChecked());
}

}  // namespace node_gemfire
