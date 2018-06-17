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
using namespace apache::geode::client;

namespace node_gemfire {

static CachePtr closeThisCache = NULLPTR;

static void closeCacheAtExit(void * arg){
   if(closeThisCache != NULLPTR && !closeThisCache->isClosed()){
     closeThisCache->close();
   }
}
NAN_MODULE_INIT(Cache::Init) {
  Nan::HandleScope scope;

  v8::Local<v8::FunctionTemplate> constructorTemplate = Nan::New<v8::FunctionTemplate>(New);
  constructorTemplate->SetClassName(Nan::New("Cache").ToLocalChecked());
  
  constructorTemplate->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(constructorTemplate, "close", Cache::Close);
  Nan::SetPrototypeMethod(constructorTemplate, "executeFunction", Cache::ExecuteFunction);
  Nan::SetPrototypeMethod(constructorTemplate, "executeQuery", Cache::ExecuteQuery);
  Nan::SetPrototypeMethod(constructorTemplate, "createRegion", Cache::CreateRegion);
  Nan::SetPrototypeMethod(constructorTemplate, "getRegion", Cache::GetRegion);
  Nan::SetPrototypeMethod(constructorTemplate, "rootRegions", Cache::RootRegions);
  Nan::SetPrototypeMethod(constructorTemplate, "inspect", Cache::Inspect);

  constructor().Reset(Nan::GetFunction(constructorTemplate).ToLocalChecked());

  Nan::Set(target, Nan::New("Cache").ToLocalChecked(), Nan::GetFunction(constructorTemplate).ToLocalChecked());
}

NAN_METHOD(Cache::New) {
  Nan::HandleScope scope;

  if (info.Length() < 1) {
    Nan::ThrowError("Cache constructor requires a path to an XML configuration file as its first argument.");
    return;
  }
  CacheFactoryPtr cacheFactory;

  if(info.Length() >1){
    // first param is the XML file and the second is the properties.
    PropertiesPtr gemfireProperties = Properties::create();
    gemfireProperties->load(*Nan::Utf8String(info[1]));
    cacheFactory = CacheFactory::createCacheFactory(gemfireProperties);
  } else {
    cacheFactory = CacheFactory::createCacheFactory();
  }

  cacheFactory->set("cache-xml-file", *Nan::Utf8String(info[0]));
  cacheFactory->setSubscriptionEnabled(true);

  CachePtr cachePtr;
  try {
    cachePtr = cacheFactory->create();
  } catch(const apache::geode::client::Exception & exception) {
    ThrowGemfireException(exception);
    return;
  }

  if (!cachePtr->getPdxReadSerialized()) {
    cachePtr->close();
    Nan::ThrowError("<pdx read-serialized='true' /> must be set in your cache xml");
    return;
  }

  Cache * cache = new Cache(cachePtr);
  cache->Wrap(info.This());

  // Just thinking do we really have to close the cache - maybe to flush some bits to disk???? cmb
  closeThisCache = cachePtr;
  node::AtExit(closeCacheAtExit);
  
  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(Cache::Close) {
  Nan::HandleScope scope;

  Cache * cache = Nan::ObjectWrap::Unwrap<Cache>(info.This());
  if(cache != NULL){
    cache->close();
  }
}

void Cache::close() {
  if (!cachePtr->isClosed()) {
    cachePtr->close();
  }
}

class ExecuteQueryWorker : public GemfireWorker {
 public:
  ExecuteQueryWorker(QueryPtr queryPtr,
                     CacheableVectorPtr queryParamsPtr,
                     Nan::Callback * callback) :
      GemfireWorker(callback),
      queryPtr(queryPtr),
      queryParamsPtr(queryParamsPtr) {}

  void ExecuteGemfireWork() {
    selectResultsPtr = queryPtr->execute(queryParamsPtr);
  }

  void HandleOKCallback() {
    Nan::HandleScope scope;

    static const int argc = 2;
    Local<Value> argv[2] = { Nan::Undefined(), v8Value(selectResultsPtr) };
    callback->Call(argc, argv);
  }

  QueryPtr queryPtr;
  CacheableVectorPtr queryParamsPtr;
  SelectResultsPtr selectResultsPtr;
};

NAN_METHOD(Cache::ExecuteQuery) {
  Nan::HandleScope scope;

  int argsLength = info.Length();

  if (argsLength == 0 || !info[0]->IsString()) {
    Nan::ThrowError("You must pass a query string and callback to executeQuery().");
    return;
  }

  if (argsLength < 2) {
    Nan::ThrowError("You must pass a callback to executeQuery().");
    return;
  }

  Local<Function> callbackFunction;
  Local<Value> poolNameValue(Nan::Undefined());
  Local<Value> queryParams;

  if (info[1]->IsFunction()) {
    callbackFunction = info[1].As<Function>();
  } else if (argsLength > 2 && info[2]->IsFunction()) {
    callbackFunction = info[2].As<Function>();

    if (info[1]->IsObject() && !info[1]->IsFunction()) {
      Local<Object> optionsObject = info[1]->ToObject();
      poolNameValue = optionsObject->Get(Nan::New("poolName").ToLocalChecked());
    }
  } else if (argsLength > 3 && info[3]->IsFunction()) {
    callbackFunction = info[3].As<Function>();

    if (info[1]->IsArray() && !info[1]->IsFunction()) {
      queryParams = info[1];
    }

    if (info[2]->IsObject() && !info[2]->IsFunction()) {
      Local<Object> optionsObject = info[2]->ToObject();
      poolNameValue = optionsObject->Get(Nan::New("poolName").ToLocalChecked());
    }
  } else {
    Nan::ThrowError("You must pass a function as the callback to executeQuery().");
    return;
  }

  Cache * cache = Nan::ObjectWrap::Unwrap<Cache>(info.This());
  CachePtr cachePtr(cache->cachePtr);

  if (cache->cachePtr->isClosed()) {
    Nan::ThrowError("Cannot execute query; cache is closed.");
    return;
  }

  QueryServicePtr queryServicePtr;
  CacheableVectorPtr queryParamsPtr = NULLPTR;

  std::string queryString(*Nan::Utf8String(info[0]));

  try {
    if (poolNameValue->IsUndefined()) {
      queryServicePtr = cachePtr->getQueryService();
    } else {
      std::string poolName(*Nan::Utf8String(poolNameValue));
      PoolPtr poolPtr(getPool(poolNameValue));

      if (poolPtr == NULLPTR) {
        std::string poolName(*Nan::Utf8String(poolNameValue));
        std::stringstream errorMessageStream;
        errorMessageStream << "executeQuery: `" << poolName << "` is not a valid pool name";
        Nan::ThrowError(errorMessageStream.str().c_str());
        return;
      }

      queryServicePtr = cachePtr->getQueryService(poolName.c_str());
    }
  } catch (const apache::geode::client::Exception & exception) {
    ThrowGemfireException(exception);
    return;
  }
  if (!(queryParams.IsEmpty() || queryParams->IsUndefined())) {
    queryParamsPtr = gemfireVector(queryParams.As<Array>(), cachePtr);
  }

  QueryPtr queryPtr(queryServicePtr->newQuery(queryString.c_str()));

  Nan::Callback * callback = new Nan::Callback(callbackFunction);

  ExecuteQueryWorker * worker = new ExecuteQueryWorker(queryPtr, queryParamsPtr, callback);
  Nan::AsyncQueueWorker(worker);

  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(Cache::CreateRegion) {
  Nan::HandleScope scope;

  if (info.Length() < 1) {
    Nan::ThrowError(
        "createRegion: You must pass the name of a GemFire region to create "
        "and a region configuration object.");
    return;
  }

  if (!info[0]->IsString()) {
    Nan::ThrowError("createRegion: You must pass a string as the name of a GemFire region.");
    return;
  }

  if (!info[1]->IsObject()) {
    Nan::ThrowError("createRegion: You must pass a configuration object as the second argument.");
    return;
  }

  Local<Object> regionConfiguration(info[1]->ToObject());

  Local<Value> regionType(regionConfiguration->Get(Nan::New("type").ToLocalChecked()));
  if (regionType->IsUndefined()) {
    Nan::ThrowError("createRegion: The region configuration object must have a type property.");
    return;
  }

  Local<Value> regionPoolName(regionConfiguration->Get(Nan::New("poolName").ToLocalChecked()));

  RegionShortcut regionShortcut(getRegionShortcut(*Nan::Utf8String(regionType)));
  if (regionShortcut == invalidRegionShortcut) {
    Nan::ThrowError("createRegion: This type is not a valid GemFire client region type");
    return;
  }

  Cache * cache = Nan::ObjectWrap::Unwrap<Cache>(info.This());
  CachePtr cachePtr(cache->cachePtr);

  RegionPtr regionPtr;
  try {
    RegionFactoryPtr regionFactoryPtr(cachePtr->createRegionFactory(regionShortcut));

    if (!regionPoolName->IsUndefined()) {
      regionFactoryPtr->setPoolName(*Nan::Utf8String(regionPoolName));
    }

    regionPtr = regionFactoryPtr->create(*Nan::Utf8String(info[0]));
  } catch (const apache::geode::client::Exception & exception) {
    ThrowGemfireException(exception);
    return;
  }

  info.GetReturnValue().Set(Region::NewInstance(regionPtr));
}

NAN_METHOD(Cache::GetRegion) {
  Nan::HandleScope scope;

  if (info.Length() != 1) {
    Nan::ThrowError("You must pass the name of a GemFire region to getRegion.");
    info.GetReturnValue().Set(Nan::Undefined());
    return;
  }

  if (!info[0]->IsString()) {
    Nan::ThrowError("You must pass a string as the name of a GemFire region to getRegion.");
    info.GetReturnValue().Set(Nan::Undefined());
    return;
  }

  Cache * cache = Nan::ObjectWrap::Unwrap<Cache>(info.This());
  CachePtr cachePtr(cache->cachePtr);
  RegionPtr regionPtr(cachePtr->getRegion(*Nan::Utf8String(info[0])));

  if(regionPtr == NULLPTR){
      info.GetReturnValue().Set(Nan::Undefined());
  }else{
     info.GetReturnValue().Set(Region::NewInstance(regionPtr));
  }
}

NAN_METHOD(Cache::RootRegions) {
  Nan::HandleScope scope;

  Cache * cache = Nan::ObjectWrap::Unwrap<Cache>(info.This());

  VectorOfRegion regions;
  cache->cachePtr->rootRegions(regions);

  unsigned int size = regions.size();
  Local<Array> rootRegions(Nan::New<Array>(size));

  for (unsigned int i = 0; i < size; i++) {
    rootRegions->Set(i, Region::NewInstance(regions[i]));
  }

  info.GetReturnValue().Set(rootRegions);
}

NAN_METHOD(Cache::Inspect) {
  Nan::HandleScope scope;
  info.GetReturnValue().Set(Nan::New("[Cache]").ToLocalChecked());
}

NAN_METHOD(Cache::ExecuteFunction) {
  Nan::HandleScope scope;

  Cache * cache = Nan::ObjectWrap::Unwrap<Cache>(info.This());
  CachePtr cachePtr(cache->cachePtr);
  if (cachePtr->isClosed()) {
    Nan::ThrowError("Cannot execute function; cache is closed.");
    return;
  }

  Local<Value> poolNameValue(Nan::Undefined());
  if (info[1]->IsObject() && !info[1]->IsArray()) {
    Local<Object> optionsObject = info[1]->ToObject();

    Local<Value> filter = optionsObject->Get(Nan::New("filter").ToLocalChecked());
    if (!filter->IsUndefined()) {
      Nan::ThrowError("You cannot pass a filter to executeFunction for a Cache.");
      return;
    }

    poolNameValue = optionsObject->Get(Nan::New("poolName").ToLocalChecked());
  }

  try {
    PoolPtr poolPtr(getPool(poolNameValue));

    if (poolPtr == NULLPTR) {
      std::string poolName(*Nan::Utf8String(poolNameValue));
      std::stringstream errorMessageStream;
      errorMessageStream << "executeFunction: `" << poolName << "` is not a valid pool name";
      Nan::ThrowError(errorMessageStream.str().c_str());
      return;
    }

    ExecutionPtr executionPtr(FunctionService::onServer(poolPtr));
    info.GetReturnValue().Set(executeFunction(info, cachePtr, executionPtr));
  } catch (const apache::geode::client::Exception & exception) {
    ThrowGemfireException(exception);
    return;
  }
}

PoolPtr Cache::getPool(const Handle<Value> & poolNameValue) {
  if (!poolNameValue->IsUndefined()) {
    std::string poolName(*Nan::Utf8String(poolNameValue));
    return PoolManager::find(poolName.c_str());
  } else {
    // FIXME: Workaround for the situation where there are no regions yet.
    //
    // As of GemFire Native Client 8.0.0.0, if no regions have ever been present, it's possible that
    // the cachePtr has no default pool set. Attempting to execute a function on this cachePtr will
    // throw a NullPointerException.
    //
    // To avoid this problem, we grab the first pool we can find and execute the function on that
    // pool's poolPtr instead of on the cachePtr. Note that this might not be the best choice of
    // poolPtr at the moment.
    //
    // See https://www.pivotaltracker.com/story/show/82079194 for the original bug.
    HashMapOfPools hashMapOfPools(PoolManager::getAll());
    HashMapOfPools::Iterator iterator(hashMapOfPools.begin());
    return iterator.second();
  }
}

}  // namespace node_gemfire
