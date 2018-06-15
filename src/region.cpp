#include "region.hpp"
#include <geode/Region.hpp>
#include <uv.h>
#include <sstream>
#include <string>
#include <vector>
#include "conversions.hpp"
#include "exceptions.hpp"
#include "cache.hpp"
#include "gemfire_worker.hpp"
#include "events.hpp"
#include "functions.hpp"
#include "region_event_registry.hpp"
#include "dependencies.hpp"

using namespace v8;
using namespace apache::geode::client;

namespace node_gemfire {

inline bool isFunctionOrUndefined(const Local<Value> & value) {
  return value->IsUndefined() || value->IsFunction();
}

inline Nan::Callback * getCallback(const Local<Value> & value) {
  if (value->IsUndefined()) {
    return NULL;
  }
  return new Nan::Callback(Local<Function>::Cast(value));
}

v8::Local<v8::Object> Region::NewInstance(RegionPtr regionPtr) {
  Nan::EscapableHandleScope scope;
  const unsigned int argc = 0;
  Local<Value> argv[argc] = {};
  Local<Object> instance(Nan::New(Region::constructor())->NewInstance(argc, argv));
  Region *region = new Region(regionPtr);
  RegionEventRegistry::getInstance()->add(region);
  region->Wrap(instance);
  return scope.Escape(instance);
}

class GemfireEventedWorker : public GemfireWorker {
 public:
  GemfireEventedWorker( const Local<Object> & v8Object, Nan::Callback * callback) :
      GemfireWorker(callback) {
        SaveToPersistent("v8Object", v8Object);
      }

  virtual void HandleOKCallback() {
    if (callback) {
      Nan::Call(*callback, 0, NULL);
    }
  }

  virtual void HandleErrorCallback() {
    Nan::HandleScope scope;
    if (callback) {
      Local<Value> argv[1] = { errorObject() };
      Nan::Call(*callback, 1, argv);
    } else {
      Local<Object> v8Object = GetFromPersistent("v8Object")->ToObject();
      emitError(v8Object, errorObject());
    }
  }
};

class ClearWorker : public GemfireEventedWorker {
 public:
  ClearWorker(
    const Local<Object> & regionObject,
    Region * region,
    Nan::Callback * callback) :
      GemfireEventedWorker(regionObject, callback),
      region(region) {}

  void ExecuteGemfireWork() {
    // Workaround: We don't want to call clear on the region if the cache is closed.
    // After cache is cleared, getCache() will throw an exception, whereas clear() causes a segfault.
    region->regionPtr->getRegionService()->isClosed();
    region->regionPtr->clear();
  }
  Region * region;
};

NAN_METHOD(Region::Clear) {
  Nan::HandleScope scope;

  if (!isFunctionOrUndefined(info[0])) {
    Nan::ThrowError("You must pass a function as the callback to clear().");
    return;
  }
  Region * region = Nan::ObjectWrap::Unwrap<Region>(info.Holder());
  Nan::Callback * callback = getCallback(info[0]);
  ClearWorker * worker = new ClearWorker(info.Holder(), region, callback);
  Nan::AsyncQueueWorker(worker);

  info.GetReturnValue().Set(info.Holder());
}

std::string unableToPutValueError(Local<Value> v8Value) {
  std::stringstream errorMessageStream;
  errorMessageStream << "Unable to put value " << *String::Utf8Value(v8Value->ToDetailString());
  return errorMessageStream.str();
}

CachePtr getCacheFromRegion(RegionPtr regionPtr) {
  Nan::HandleScope scope;
  try {
    //TODO: need to fix this since it gets any instance and doesn't use the region.
    CachePtr cachePtr = CacheFactory::getAnyInstance();
    if(cachePtr == NULLPTR || cachePtr->isClosed()){
      if(regionPtr != NULLPTR){
        std::string msg("Region name ");
        msg += regionPtr->getName();
        msg += " is invalid because the Cache is Closed.";
        Nan::ThrowError(Nan::New(msg).ToLocalChecked());
      } else {
        Nan::ThrowError(Nan::New("Cache is closed.").ToLocalChecked());
      }
      return NULLPTR;
    }
    return CacheFactory::getAnyInstance();
  } catch (const RegionDestroyedException & exception) {
    ThrowGemfireException(exception);
  }
  return NULLPTR;
}

class PutWorker : public GemfireEventedWorker {
 public:
  PutWorker(
    const Local<Object> & regionObject,
    Region * region,
    const CacheableKeyPtr & keyPtr,
    const CacheablePtr & valuePtr,
    Nan::Callback * callback) :
      GemfireEventedWorker(regionObject, callback),
      region(region),
      keyPtr(keyPtr),
      valuePtr(valuePtr) { }

  void ExecuteGemfireWork() {
    if (keyPtr == NULLPTR) {
      SetError("InvalidKeyError", "Invalid GemFire key.");
      return;
    }
    if (valuePtr == NULLPTR) {
      SetError("InvalidValueError", "Invalid GemFire value.");
      return;
    }
    region->regionPtr->put(keyPtr, valuePtr);
  }
  Region * region;
  CacheableKeyPtr keyPtr;
  CacheablePtr valuePtr;
};

NAN_METHOD(Region::Put) {
  Nan::HandleScope scope;
  unsigned int argsLength = info.Length();

  if (argsLength < 2) {
    Nan::ThrowError("You must pass a key and value to put().");
    return;
  }
  if (!isFunctionOrUndefined(info[2])) {
    Nan::ThrowError("You must pass a function as the callback to put().");
    return;
  }

  Region * region = Nan::ObjectWrap::Unwrap<Region>(info.Holder());

  CachePtr cachePtr(getCacheFromRegion(region->regionPtr));
  if (cachePtr == NULLPTR) {
    return;
  }

  CacheableKeyPtr keyPtr(gemfireKey(info[0], cachePtr));
  CacheablePtr valuePtr(gemfireValue(info[1], cachePtr));

  Nan::Callback * callback = getCallback(info[2]);
  PutWorker * putWorker = new PutWorker(info.Holder(), region, keyPtr, valuePtr, callback);
  Nan::AsyncQueueWorker(putWorker);

  info.GetReturnValue().Set(info.Holder());
}

NAN_METHOD(Region::PutSync) {
  Nan::HandleScope scope;

  try{
    unsigned int argsLength = info.Length();

    if (argsLength != 2) {
      Nan::ThrowError("You must pass a key and value to putSync().");
      return;
    }

    Region * region = Nan::ObjectWrap::Unwrap<Region>(info.Holder());

    CachePtr cachePtr(getCacheFromRegion(region->regionPtr));
    if (cachePtr == NULLPTR) {
      return;
    }

    CacheableKeyPtr keyPtr(gemfireKey(info[0], cachePtr));
    CacheablePtr valuePtr(gemfireValue(info[1], cachePtr));

    if (keyPtr == NULLPTR) {
      Nan::ThrowError("Invalid GemFire key.");
      return;
    }

    if (valuePtr == NULLPTR) {
      Nan::ThrowError("Invalid GemFire value.");
      return;
    }
    region->regionPtr->put(keyPtr, valuePtr);
    info.GetReturnValue().Set(info.Holder());
  } catch(apache::geode::client::Exception & exception) {
    std::string msg(exception.getName());
    msg.append(" ").append(exception.getMessage());
    Nan::ThrowError(msg.c_str());
    info.GetReturnValue().Set(Nan::Undefined());
  }
}

class GetWorker : public GemfireWorker {
 public:
  GetWorker(Nan::Callback * callback,
           const RegionPtr & regionPtr,
           const CacheableKeyPtr & keyPtr) :
      GemfireWorker(callback),
      regionPtr(regionPtr),
      keyPtr(keyPtr) {}

  void ExecuteGemfireWork() {
    if (keyPtr == NULLPTR) {
      SetError("InvalidKeyError", "Invalid GemFire key.");
      return;
    }

    valuePtr = regionPtr->get(keyPtr);

    if (valuePtr == NULLPTR) {
      SetError("KeyNotFoundError", "Key not found in region.");
    }
  }

  void HandleOKCallback() {
    Nan::HandleScope scope;
    Local<Value> argv[2] = { Nan::Undefined(), v8Value(valuePtr) };
    Nan::Call(*callback, 2, argv);
  }

  RegionPtr regionPtr;
  CacheableKeyPtr keyPtr;
  CacheablePtr valuePtr;
};

NAN_METHOD(Region::Get) {
  Nan::HandleScope scope;

  unsigned int argsLength = info.Length();

  if (argsLength != 2) {
    Nan::ThrowError("You must pass a key and a callback to get().");
    return;
  }

  if (!info[1]->IsFunction()) {
    Nan::ThrowError("You must pass a function as the callback to get().");
    return;
  }

  Region * region = Nan::ObjectWrap::Unwrap<Region>(info.Holder());
  RegionPtr regionPtr(region->regionPtr);

  CachePtr cachePtr(getCacheFromRegion(region->regionPtr));
  if (cachePtr == NULLPTR) {
    return;
  }

  CacheableKeyPtr keyPtr(gemfireKey(info[0], cachePtr));

  Nan::Callback * callback = new Nan::Callback(info[1].As<Function>());
  GetWorker * getWorker = new GetWorker(callback, regionPtr, keyPtr);
  Nan::AsyncQueueWorker(getWorker);

  info.GetReturnValue().Set(info.Holder());
}

NAN_METHOD(Region::GetSync) {
  Nan::HandleScope scope;

  unsigned int argsLength = info.Length();

  if (argsLength == 0) {
    Nan::ThrowError("You must pass a key to getSync().");
    return;
  }

  Region * region = Nan::ObjectWrap::Unwrap<Region>(info.Holder());
  RegionPtr regionPtr(region->regionPtr);

  CachePtr cachePtr(getCacheFromRegion(region->regionPtr));
  if (cachePtr == NULLPTR) {
    return;
  }

  CacheableKeyPtr keyPtr(gemfireKey(info[0], cachePtr));
  CacheablePtr valuePtr = regionPtr->get(keyPtr);

  if (valuePtr == NULLPTR) {
    Nan::ThrowError("Key not found in region.");
  }

  info.GetReturnValue().Set(v8Value(valuePtr));
}

class GetAllWorker : public GemfireWorker {
 public:
  GetAllWorker(
      const RegionPtr & regionPtr,
      const VectorOfCacheableKeyPtr & gemfireKeysPtr,
      Nan::Callback * callback) :
    GemfireWorker(callback),
    regionPtr(regionPtr),
    gemfireKeysPtr(gemfireKeysPtr) {}

  void ExecuteGemfireWork() {
    resultsPtr = new HashMapOfCacheable();

    if (gemfireKeysPtr == NULLPTR) {
      SetError("InvalidKeyError", "Invalid GemFire key.");
      return;
    }

    if (gemfireKeysPtr->size() == 0) {
      return;
    }

    regionPtr->getAll(*gemfireKeysPtr, resultsPtr, NULLPTR);
  }

  void HandleOKCallback() {
    Nan::HandleScope scope;

    Local<Value> argv[2] = { Nan::Undefined(), v8Value(resultsPtr) };
    Nan::Call(*callback, 2, argv);
  }

 private:
  RegionPtr regionPtr;
  VectorOfCacheableKeyPtr gemfireKeysPtr;
  HashMapOfCacheablePtr resultsPtr;
};

NAN_METHOD(Region::GetAll) {
  Nan::HandleScope scope;

  if (info.Length() == 0 || !info[0]->IsArray()) {
    Nan::ThrowError("You must pass an array of keys and a callback to getAll().");
    return;
  }

  if (info.Length() == 1) {
    Nan::ThrowError("You must pass a callback to getAll().");
    return;
  }

  if (!info[1]->IsFunction()) {
    Nan::ThrowError("You must pass a function as the callback to getAll().");
    return;
  }

  Region * region = Nan::ObjectWrap::Unwrap<Region>(info.Holder());
  RegionPtr regionPtr(region->regionPtr);

  CachePtr cachePtr(getCacheFromRegion(region->regionPtr));
  if (cachePtr == NULLPTR) {
    return;
  }

  VectorOfCacheableKeyPtr gemfireKeysPtr(gemfireKeys(Local<Array>::Cast(info[0]), cachePtr));

  Nan::Callback * callback = new Nan::Callback(info[1].As<Function>());

  GetAllWorker * worker = new GetAllWorker(regionPtr, gemfireKeysPtr, callback);
  Nan::AsyncQueueWorker(worker);

  info.GetReturnValue().Set(info.Holder());
}

NAN_METHOD(Region::GetAllSync) {
  Nan::HandleScope scope;

  if (info.Length() != 1 || !info[0]->IsArray()) {
    Nan::ThrowError("You must pass an array of keys to getAllSync().");
    info.GetReturnValue().Set(Nan::Undefined());
    return;
  }

  Region * region = Nan::ObjectWrap::Unwrap<Region>(info.Holder());
  RegionPtr regionPtr(region->regionPtr);

  CachePtr cachePtr(getCacheFromRegion(region->regionPtr));
  if (cachePtr == NULLPTR) {
    info.GetReturnValue().Set(Nan::Undefined());
    return;
  }

  VectorOfCacheableKeyPtr gemfireKeysPtr(gemfireKeys(Local<Array>::Cast(info[0]), cachePtr));

  if (gemfireKeysPtr == NULLPTR) {
    Nan::ThrowError("Invalid GemFire key.");
    info.GetReturnValue().Set(Nan::Undefined());
    return;
  }

  HashMapOfCacheablePtr resultsPtr(new HashMapOfCacheable());
  if (gemfireKeysPtr->size() == 0) {
    info.GetReturnValue().Set(v8Object(resultsPtr));
  }else{
    regionPtr->getAll(*gemfireKeysPtr, resultsPtr, NULLPTR);
    info.GetReturnValue().Set(v8Value(resultsPtr));
  }
}

class PutAllWorker : public GemfireEventedWorker {
 public:
  PutAllWorker(
      const Local<Object> & regionObject,
      const RegionPtr & regionPtr,
      const HashMapOfCacheablePtr & hashMapPtr,
      Nan::Callback * callback) :
    GemfireEventedWorker(regionObject, callback),
    regionPtr(regionPtr),
    hashMapPtr(hashMapPtr) { }

  void ExecuteGemfireWork() {
    if (hashMapPtr == NULLPTR) {
      SetError("InvalidValueError", "Invalid GemFire value.");
      return;
    }
    regionPtr->putAll(*hashMapPtr);
  }

 private:
  RegionPtr regionPtr;
  HashMapOfCacheablePtr hashMapPtr;
};

NAN_METHOD(Region::PutAll) {
  Nan::HandleScope scope;
  if (info.Length() == 0 || !info[0]->IsObject()) {
    Nan::ThrowError("You must pass an object and a callback to putAll().");
    return;
  }
  if (!isFunctionOrUndefined(info[1])) {
    Nan::ThrowError("You must pass a function as the callback to putAll().");
    return;
  }
  Region * region = Nan::ObjectWrap::Unwrap<Region>(info.Holder());
  RegionPtr regionPtr(region->regionPtr);

  CachePtr cachePtr(getCacheFromRegion(region->regionPtr));
  if (cachePtr == NULLPTR) {
    return;
  }

  HashMapOfCacheablePtr hashMapPtr(gemfireHashMap(info[0]->ToObject(), cachePtr));
  Nan::Callback * callback = getCallback(info[1]);
  PutAllWorker * worker = new PutAllWorker(info.Holder(), regionPtr, hashMapPtr, callback);
  Nan::AsyncQueueWorker(worker);

  info.GetReturnValue().Set(info.Holder());
}

NAN_METHOD(Region::PutAllSync) {
  Nan::HandleScope scope;

  if (info.Length() != 1 || !info[0]->IsObject()) {
    Nan::ThrowError("You must pass an object to putAllSync().");
    return;
  }

  Region * region = Nan::ObjectWrap::Unwrap<Region>(info.Holder());
  RegionPtr regionPtr(region->regionPtr);

  CachePtr cachePtr(getCacheFromRegion(region->regionPtr));
  if (cachePtr == NULLPTR) {
    return;
  }
  try{
    HashMapOfCacheablePtr hashMapPtr(gemfireHashMap(info[0]->ToObject(), cachePtr));
    if (hashMapPtr == NULLPTR) {
      Nan::ThrowError("Invalid GemFire value.");
      return;
    }
    regionPtr->putAll(*hashMapPtr);
    info.GetReturnValue().Set(info.Holder());
  }catch (const apache::geode::client::Exception & exception) {
    ThrowGemfireException(exception);
    info.GetReturnValue().Set(Nan::Undefined());
  }
}

class RemoveWorker : public GemfireEventedWorker {
 public:
  RemoveWorker(
      const Local<Object> & regionObject,
      const RegionPtr & regionPtr,
      const CacheableKeyPtr & keyPtr,
      Nan::Callback * callback) :
    GemfireEventedWorker(regionObject, callback),
    regionPtr(regionPtr),
    keyPtr(keyPtr) {}

  void ExecuteGemfireWork() {
    if (keyPtr == NULLPTR) {
      SetError("InvalidKeyError", "Invalid GemFire key.");
      return;
    }

    try {
      regionPtr->destroy(keyPtr);
    } catch (const EntryNotFoundException & exception) {
      SetError("KeyNotFoundError", "Key not found in region.");
    }
  }

  RegionPtr regionPtr;
  CacheableKeyPtr keyPtr;
};

NAN_METHOD(Region::Remove) {
  Nan::HandleScope scope;

  if (info.Length() < 1) {
    Nan::ThrowError("You must pass a key to remove().");
    return;
  }

  if (!isFunctionOrUndefined(info[1])) {
    Nan::ThrowError("You must pass a function as the callback to remove().");
    return;
  }

  Region * region = Nan::ObjectWrap::Unwrap<Region>(info.Holder());
  RegionPtr regionPtr(region->regionPtr);

  CachePtr cachePtr(getCacheFromRegion(region->regionPtr));
  if (cachePtr == NULLPTR) {
    return;
  }

  CacheableKeyPtr keyPtr(gemfireKey(info[0], cachePtr));
  Nan::Callback * callback = getCallback(info[1]);
  RemoveWorker * worker = new RemoveWorker(info.Holder(), regionPtr, keyPtr, callback);
  Nan::AsyncQueueWorker(worker);

  info.GetReturnValue().Set(info.Holder());
}

NAN_METHOD(Region::ExecuteFunction) {
  Nan::HandleScope scope;

  Region * region = Nan::ObjectWrap::Unwrap<Region>(info.Holder());
  RegionPtr regionPtr(region->regionPtr);

  CachePtr cachePtr(getCacheFromRegion(region->regionPtr));
  if (cachePtr == NULLPTR) {
    return;
  }

  try {
    ExecutionPtr executionPtr(FunctionService::onRegion(regionPtr));
    info.GetReturnValue().Set(executeFunction(info, cachePtr, executionPtr));
  } catch (const apache::geode::client::Exception & exception) {
    ThrowGemfireException(exception);
    return;
  }
}


NAN_METHOD(Region::Inspect) {
  Nan::HandleScope scope;

  Region * region = Nan::ObjectWrap::Unwrap<Region>(info.Holder());
  RegionPtr regionPtr(region->regionPtr);

  const char * name = regionPtr->getName();

  std::stringstream inspectStream;
  inspectStream << "[Region name=\"" << name << "\"]";
  info.GetReturnValue().Set(Nan::New(inspectStream.str().c_str()).ToLocalChecked());
}

NAN_GETTER(Region::Name) {
  Nan::HandleScope scope;

  Region * region = Nan::ObjectWrap::Unwrap<Region>(info.Holder());
  RegionPtr regionPtr(region->regionPtr);

  info.GetReturnValue().Set(Nan::New(regionPtr->getName()).ToLocalChecked());
}

NAN_GETTER(Region::Attributes) {
  Nan::HandleScope scope;

  Region * region = Nan::ObjectWrap::Unwrap<Region>(info.Holder());
  RegionPtr regionPtr(region->regionPtr);

  RegionAttributesPtr regionAttributesPtr(regionPtr->getAttributes());

  Local<Object> returnValue(Nan::New<Object>());

  Nan::DefineOwnProperty(returnValue, Nan::New("cachingEnabled").ToLocalChecked(),
      Nan::New(regionAttributesPtr->getCachingEnabled()),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  Nan::DefineOwnProperty(returnValue,Nan::New("clientNotificationEnabled").ToLocalChecked(),
      Nan::New(regionAttributesPtr->getClientNotificationEnabled()),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  Nan::DefineOwnProperty(returnValue,Nan::New("concurrencyChecksEnabled").ToLocalChecked(),
      Nan::New(regionAttributesPtr->getConcurrencyChecksEnabled()),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  Nan::DefineOwnProperty(returnValue,Nan::New("concurrencyLevel").ToLocalChecked(),
      Nan::New(regionAttributesPtr->getConcurrencyLevel()),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  Nan::DefineOwnProperty(returnValue,Nan::New("diskPolicy").ToLocalChecked(),
      Nan::New(DiskPolicyType::fromOrdinal(regionAttributesPtr->getDiskPolicy())).ToLocalChecked(),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  Nan::DefineOwnProperty(returnValue,Nan::New("entryIdleTimeout").ToLocalChecked(),
      Nan::New(regionAttributesPtr->getEntryIdleTimeout()),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  Nan::DefineOwnProperty(returnValue,Nan::New("entryTimeToLive").ToLocalChecked(),
      Nan::New(regionAttributesPtr->getEntryTimeToLive()),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  Nan::DefineOwnProperty(returnValue,Nan::New("initialCapacity").ToLocalChecked(),
      Nan::New(regionAttributesPtr->getInitialCapacity()),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  Nan::DefineOwnProperty(returnValue,Nan::New("loadFactor").ToLocalChecked(),
      Nan::New(regionAttributesPtr->getLoadFactor()),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  Nan::DefineOwnProperty(returnValue,Nan::New("lruEntriesLimit").ToLocalChecked(),
      Nan::New(regionAttributesPtr->getLruEntriesLimit()),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  Nan::DefineOwnProperty(returnValue,Nan::New("lruEvicationAction").ToLocalChecked(),
      Nan::New(ExpirationAction::fromOrdinal(regionAttributesPtr->getLruEvictionAction())).ToLocalChecked(),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  const char * poolName = regionAttributesPtr->getPoolName();
  if (poolName == NULL) {
    Nan::DefineOwnProperty(returnValue,Nan::New("poolName").ToLocalChecked(),
        Nan::Null(),
        static_cast<PropertyAttribute>(ReadOnly | DontDelete));
  } else {
    Nan::DefineOwnProperty(returnValue,Nan::New("poolName").ToLocalChecked(),
        Nan::New(poolName).ToLocalChecked(),
        static_cast<PropertyAttribute>(ReadOnly | DontDelete));
  }
  Nan::DefineOwnProperty(returnValue,Nan::New("regionIdleTimeout").ToLocalChecked(),
      Nan::New(regionAttributesPtr->getRegionIdleTimeout()),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  Nan::DefineOwnProperty(returnValue,Nan::New("regionTimeToLive").ToLocalChecked(),
      Nan::New(regionAttributesPtr->getRegionTimeToLive()),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  info.GetReturnValue().Set(returnValue);
}

template <typename T>
class AbstractQueryWorker : public GemfireWorker {
 public:
  AbstractQueryWorker(
      const RegionPtr & regionPtr,
      const std::string & queryPredicate,
      Nan::Callback * callback) :
    GemfireWorker(callback),
    regionPtr(regionPtr),
    queryPredicate(queryPredicate) {}

  void HandleOKCallback() {
    Local<Value> argv[2] = { Nan::Undefined(), v8Value(resultPtr) };
    Nan::Call(*callback, 2, argv);
  }

  RegionPtr regionPtr;
  std::string queryPredicate;
  T resultPtr;
};

class QueryWorker : public AbstractQueryWorker<SelectResultsPtr> {
 public:
  QueryWorker(
      const RegionPtr & regionPtr,
      const std::string & queryPredicate,
      Nan::Callback * callback) :
    AbstractQueryWorker<SelectResultsPtr>(regionPtr, queryPredicate, callback) {}

  void ExecuteGemfireWork() {
    resultPtr = regionPtr->query(queryPredicate.c_str());
  }

  static std::string name() {
    return "query()";
  }
};

class SelectValueWorker : public AbstractQueryWorker<CacheablePtr> {
 public:
  SelectValueWorker(
      const RegionPtr & regionPtr,
      const std::string & queryPredicate,
      Nan::Callback * callback) :
    AbstractQueryWorker<CacheablePtr>(regionPtr, queryPredicate, callback) {}

  void ExecuteGemfireWork() {
    resultPtr = regionPtr->selectValue(queryPredicate.c_str());
  }

  static std::string name() {
    return "selectValue()";
  }
};

class ExistsValueWorker : public AbstractQueryWorker<bool> {
 public:
  ExistsValueWorker(
      const RegionPtr & regionPtr,
      const std::string & queryPredicate,
      Nan::Callback * callback) :
    AbstractQueryWorker<bool>(regionPtr, queryPredicate, callback) {}

  void ExecuteGemfireWork() {
    resultPtr = regionPtr->existsValue(queryPredicate.c_str());
  }

  static std::string name() {
    return "existsValue()";
  }
};

template<typename T>
NAN_METHOD(Region::Query) {
  Nan::HandleScope scope;

  if (info.Length() < 2) {
    std::stringstream errorStream;
    errorStream << "You must pass a query predicate string and a callback to " << T::name() << ".";
    Nan::ThrowError(errorStream.str().c_str());
    return;
  }

  if (!info[1]->IsFunction()) {
    std::stringstream errorStream;
    errorStream << "You must pass a function as the callback to " << T::name() << ".";
    Nan::ThrowError(errorStream.str().c_str());
    return;
  }

  Region * region = Nan::ObjectWrap::Unwrap<Region>(info.Holder());

  std::string queryPredicate(*Nan::Utf8String(info[0]));
  Nan::Callback * callback = new Nan::Callback(info[1].As<Function>());

  T * worker = new T(region->regionPtr, queryPredicate, callback);
  Nan::AsyncQueueWorker(worker);

  info.GetReturnValue().Set(info.Holder());
}

class ServerKeysWorker : public GemfireWorker {
 public:
  ServerKeysWorker(
      const RegionPtr & regionPtr,
      Nan::Callback * callback) :
    GemfireWorker(callback),
    regionPtr(regionPtr) {}

  void ExecuteGemfireWork() {
    keysVectorPtr = new VectorOfCacheableKey();
    regionPtr->serverKeys(*keysVectorPtr);
  }

  void HandleOKCallback() {
    Local<Value> argv[2] = { Nan::Undefined(), v8Value(keysVectorPtr) };
    Nan::Call(*callback, 2, argv);
  }

 private:
  RegionPtr regionPtr;
  VectorOfCacheableKeyPtr keysVectorPtr;
};

NAN_METHOD(Region::ServerKeys) {
  Nan::HandleScope scope;

  if (info.Length() == 0) {
    Nan::ThrowError("You must pass a callback to serverKeys().");
    return;
  }

  if (!info[0]->IsFunction()) {
    Nan::ThrowError("You must pass a function as the callback to serverKeys().");
    return;
  }

  Region * region = Nan::ObjectWrap::Unwrap<Region>(info.Holder());
  Nan::Callback * callback = new Nan::Callback(info[0].As<Function>());

  ServerKeysWorker * worker = new ServerKeysWorker(region->regionPtr, callback);
  Nan::AsyncQueueWorker(worker);
}

class KeysWorker : public GemfireWorker {
 public:
  KeysWorker(
      const RegionPtr & regionPtr,
      Nan::Callback * callback) :
    GemfireWorker(callback),
    regionPtr(regionPtr) {}

  void ExecuteGemfireWork() {
    keysVectorPtr = new VectorOfCacheableKey();
    regionPtr->keys(*keysVectorPtr);
  }

  void HandleOKCallback() {
    Local<Value> argv[2] = { Nan::Undefined(), v8Value(keysVectorPtr) };
    Nan::Call(*callback, 2, argv);
  }

 private:
  RegionPtr regionPtr;
  VectorOfCacheableKeyPtr keysVectorPtr;
};

NAN_METHOD(Region::Keys) {
  Nan::HandleScope scope;

  if (info.Length() == 0) {
    Nan::ThrowError("You must pass a callback to keys().");
    return;
  }

  if (!info[0]->IsFunction()) {
    Nan::ThrowError("You must pass a function as the callback to keys().");
    return;
  }

  Region * region = Nan::ObjectWrap::Unwrap<Region>(info.Holder());
  Nan::Callback * callback = new Nan::Callback(info[0].As<Function>());

  KeysWorker * worker = new KeysWorker(region->regionPtr, callback);
  Nan::AsyncQueueWorker(worker);
}

NAN_METHOD(Region::RegisterAllKeys) {
  Nan::HandleScope scope;

  Region * region = Nan::ObjectWrap::Unwrap<Region>(info.Holder());
  try {
    region->regionPtr->registerAllKeys();
  } catch (const apache::geode::client::Exception & exception) {
    ThrowGemfireException(exception);
  }
}

NAN_METHOD(Region::UnregisterAllKeys) {
  Nan::HandleScope scope;

  Region * region = Nan::ObjectWrap::Unwrap<Region>(info.Holder());
  try {
    region->regionPtr->unregisterAllKeys();
  } catch (const apache::geode::client::Exception & exception) {
    ThrowGemfireException(exception);
  }
}

class ValuesWorker : public GemfireWorker {
 public:
  ValuesWorker(
      const RegionPtr & regionPtr,
      Nan::Callback * callback) :
    GemfireWorker(callback),
    regionPtr(regionPtr) {}

  void ExecuteGemfireWork() {
    valuesVectorPtr = new VectorOfCacheable();
    regionPtr->values(*valuesVectorPtr);
  }

  void HandleOKCallback() {
    Local<Value> argv[2] = { Nan::Undefined(), v8Value(valuesVectorPtr) };
    Nan::Call(*callback, 2, argv);
  }

 private:
  RegionPtr regionPtr;
  VectorOfCacheablePtr valuesVectorPtr;
};

NAN_METHOD(Region::Values) {
  Nan::HandleScope scope;

  if (info.Length() == 0) {
    Nan::ThrowError("You must pass a callback to values().");
    return;
  }

  if (!info[0]->IsFunction()) {
    Nan::ThrowError("You must pass a function as the callback to values().");
    return;
  }

  Region * region = Nan::ObjectWrap::Unwrap<Region>(info.Holder());
  Nan::Callback * callback = new Nan::Callback(info[0].As<Function>());

  ValuesWorker * worker = new ValuesWorker(region->regionPtr, callback);
  Nan::AsyncQueueWorker(worker);
}

class EntriesWorker : public GemfireWorker {
 public:
  EntriesWorker(
      const RegionPtr & regionPtr,
      Nan::Callback * callback,
      bool recursive = true) :
    GemfireWorker(callback),
    regionPtr(regionPtr),
    recursive(recursive) {}

  void ExecuteGemfireWork() {
    regionEntryVector = new VectorOfRegionEntry();
    regionPtr->entries(*regionEntryVector, recursive);
  }

  void HandleOKCallback() {
    Local<Value> argv[2] = { Nan::Undefined(), v8Value(*regionEntryVector) };
    Nan::Call(*callback, 2, argv);
  }

 private:
  RegionPtr regionPtr;
  VectorOfRegionEntry* regionEntryVector;
  bool recursive;
};


NAN_METHOD(Region::Entries) {
  Nan::HandleScope scope;

  if (info.Length() == 0) {
    Nan::ThrowError("You must pass a callback to entries().");
    return;
  }

  if (!info[0]->IsFunction()) {
    Nan::ThrowError("You must pass a function as the callback to entries().");
    return;
  }

  Region * region = Nan::ObjectWrap::Unwrap<Region>(info.Holder());
  Nan::Callback * callback = new Nan::Callback(info[0].As<Function>());

  EntriesWorker * worker = new EntriesWorker(region->regionPtr, callback, true);
  Nan::AsyncQueueWorker(worker);
}

class DestroyRegionWorker : public GemfireEventedWorker {
 public:
  DestroyRegionWorker(
    const Local<Object> & regionObject,
    Region * region,
    Nan::Callback * callback,
    bool local = true) :
      GemfireEventedWorker(regionObject, callback),
      region(region),
      local(local) {}

  void ExecuteGemfireWork() {
    if (local) {
      region->regionPtr->localDestroyRegion();
    } else {
      region->regionPtr->destroyRegion();
    }
  }

 private:
  Region * region;
  bool local;
};

NAN_METHOD(Region::DestroyRegion) {
  Nan::HandleScope scope;

  if (!isFunctionOrUndefined(info[0])) {
    Nan::ThrowError("You must pass a function as the callback to destroyRegion().");
    return;
  }

  Region * region = Nan::ObjectWrap::Unwrap<Region>(info.Holder());

  Nan::Callback * callback = getCallback(info[0]);
  DestroyRegionWorker * worker = new DestroyRegionWorker(info.Holder(), region, callback, false);
  Nan::AsyncQueueWorker(worker);

  info.GetReturnValue().Set(info.Holder());
}

NAN_METHOD(Region::LocalDestroyRegion) {
  Nan::HandleScope scope;

  if (!isFunctionOrUndefined(info[0])) {
    Nan::ThrowError("You must pass a function as the callback to localDestroyRegion().");
    return;
  }

  Region * region = Nan::ObjectWrap::Unwrap<Region>(info.Holder());

  Nan::Callback * callback = getCallback(info[0]);
  DestroyRegionWorker * worker = new DestroyRegionWorker(info.Holder(), region, callback);
  Nan::AsyncQueueWorker(worker);

  info.GetReturnValue().Set(info.Holder());
}

NAN_MODULE_INIT(Region::Init){
  Nan::HandleScope scope;

  Local<FunctionTemplate> constructorTemplate = Nan::New<FunctionTemplate>();

  constructorTemplate->SetClassName(Nan::New("Region").ToLocalChecked());
  constructorTemplate->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(constructorTemplate, "clear", Region::Clear);
  Nan::SetPrototypeMethod(constructorTemplate, "put", Region::Put);
  Nan::SetPrototypeMethod(constructorTemplate, "putSync",Region::PutSync);
  Nan::SetPrototypeMethod(constructorTemplate, "get", Region::Get);
  Nan::SetPrototypeMethod(constructorTemplate, "getSync",Region::GetSync);
  Nan::SetPrototypeMethod(constructorTemplate, "getAll", Region::GetAll);
  Nan::SetPrototypeMethod(constructorTemplate, "getAllSync", Region::GetAllSync);
  Nan::SetPrototypeMethod(constructorTemplate, "entries", Region::Entries);
  Nan::SetPrototypeMethod(constructorTemplate, "putAll", Region::PutAll);
  Nan::SetPrototypeMethod(constructorTemplate, "putAllSync", Region::PutAllSync);
  Nan::SetPrototypeMethod(constructorTemplate, "remove", Region::Remove);
  Nan::SetPrototypeMethod(constructorTemplate, "query",  Region::Query<QueryWorker>);
  Nan::SetPrototypeMethod(constructorTemplate, "selectValue",  Region::Query<SelectValueWorker>);
  Nan::SetPrototypeMethod(constructorTemplate, "existsValue", Region::Query<ExistsValueWorker>);
  Nan::SetPrototypeMethod(constructorTemplate, "executeFunction", Region::ExecuteFunction);
  Nan::SetPrototypeMethod(constructorTemplate, "serverKeys",  Region::ServerKeys);
  Nan::SetPrototypeMethod(constructorTemplate, "keys", Region::Keys);
  Nan::SetPrototypeMethod(constructorTemplate, "values", Region::Values);
  Nan::SetPrototypeMethod(constructorTemplate, "inspect", Region::Inspect);
  Nan::SetPrototypeMethod(constructorTemplate, "registerAllKeys", Region::RegisterAllKeys);
  Nan::SetPrototypeMethod(constructorTemplate, "unregisterAllKeys",  Region::UnregisterAllKeys);
  Nan::SetPrototypeMethod(constructorTemplate, "destroyRegion", Region::DestroyRegion);
  Nan::SetPrototypeMethod(constructorTemplate, "localDestroyRegion",  Region::LocalDestroyRegion);

  Nan::SetAccessor(constructorTemplate->InstanceTemplate(), Nan::New<String>("name").ToLocalChecked(),  Region::Name);
  Nan::SetAccessor(constructorTemplate->InstanceTemplate(), Nan::New<String>("attributes").ToLocalChecked(),  Region::Attributes);

  constructor().Reset(Nan::GetFunction(constructorTemplate).ToLocalChecked());

  Nan::Set(target, Nan::New("Region").ToLocalChecked(), Nan::GetFunction(constructorTemplate).ToLocalChecked());
}

}  // namespace node_gemfire
