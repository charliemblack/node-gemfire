#include "cache.hpp"
#include <v8.h>
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

void Cache::Init(Local<Object> exports) {
  NanScope();

  Local<FunctionTemplate> cacheConstructorTemplate =
    NanNew<FunctionTemplate>(Cache::New);

  cacheConstructorTemplate->SetClassName(NanNew("Cache"));
  cacheConstructorTemplate->InstanceTemplate()->SetInternalFieldCount(1);

  NanSetPrototypeTemplate(cacheConstructorTemplate, "close",
      NanNew<FunctionTemplate>(Cache::Close)->GetFunction());
  NanSetPrototypeTemplate(cacheConstructorTemplate, "executeFunction",
      NanNew<FunctionTemplate>(Cache::ExecuteFunction)->GetFunction());
  NanSetPrototypeTemplate(cacheConstructorTemplate, "executeQuery",
      NanNew<FunctionTemplate>(Cache::ExecuteQuery)->GetFunction());
  NanSetPrototypeTemplate(cacheConstructorTemplate, "createRegion",
      NanNew<FunctionTemplate>(Cache::CreateRegion)->GetFunction());
  NanSetPrototypeTemplate(cacheConstructorTemplate, "getRegion",
      NanNew<FunctionTemplate>(Cache::GetRegion)->GetFunction());
  NanSetPrototypeTemplate(cacheConstructorTemplate, "rootRegions",
      NanNew<FunctionTemplate>(Cache::RootRegions)->GetFunction());
  NanSetPrototypeTemplate(cacheConstructorTemplate, "inspect",
      NanNew<FunctionTemplate>(Cache::Inspect)->GetFunction());

  exports->Set(NanNew("Cache"), cacheConstructorTemplate->GetFunction());
}

NAN_METHOD(Cache::New) {
  NanScope();

  if (args.Length() < 1) {
    NanThrowError("Cache constructor requires a path to an XML configuration file as its first argument.");
    NanReturnUndefined();
  }

  CacheFactoryPtr cacheFactory(CacheFactory::createCacheFactory());
  cacheFactory->set("cache-xml-file", *NanAsciiString(args[0]));
  cacheFactory->setSubscriptionEnabled(true);

  CachePtr cachePtr;
  try {
    cachePtr = cacheFactory->create();
  } catch(const apache::geode::client::Exception & exception) {
    ThrowGemfireException(exception);
    NanReturnUndefined();
  }

  if (!cachePtr->getPdxReadSerialized()) {
    cachePtr->close();
    NanThrowError("<pdx read-serialized='true' /> must be set in your cache xml");
    NanReturnUndefined();
  }

  Cache * cache = new Cache(cachePtr);
  cache->Wrap(args.This());

  Local<Object> process(NanNew(dependencies)->Get(NanNew("process"))->ToObject());
  static const int argc = 2;
  Handle<Value> argv[argc] = { NanNew("exit"), cache->exitCallback() };
  NanMakeCallback(process, "on", argc, argv);

  NanReturnValue(args.This());
}

Local<Function> Cache::exitCallback() {
  NanEscapableScope();

  Local<Function> unboundExitCallback(NanNew<FunctionTemplate>(Cache::Close)->GetFunction());

  static const int argc = 1;
  Handle<Value> argv[argc] = { NanObjectWrapHandle(this) };
  Local<Function> boundExitCallback(
      NanMakeCallback(unboundExitCallback, "bind", argc, argv).As<Function>());

  return NanEscapeScope(boundExitCallback);
}

NAN_METHOD(Cache::Close) {
  NanScope();

  Cache * cache = ObjectWrap::Unwrap<Cache>(args.This());
  cache->close();

  NanReturnUndefined();
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
                     NanCallback * callback) :
      GemfireWorker(callback),
      queryPtr(queryPtr),
      queryParamsPtr(queryParamsPtr) {}

  void ExecuteGemfireWork() {
    selectResultsPtr = queryPtr->execute(queryParamsPtr);
  }

  void HandleOKCallback() {
    NanScope();

    static const int argc = 2;
    Local<Value> argv[2] = { NanUndefined(), v8Value(selectResultsPtr) };
    callback->Call(argc, argv);
  }

  QueryPtr queryPtr;
  CacheableVectorPtr queryParamsPtr;
  SelectResultsPtr selectResultsPtr;
};

NAN_METHOD(Cache::ExecuteQuery) {
  NanScope();

  int argsLength = args.Length();

  if (argsLength == 0 || !args[0]->IsString()) {
    NanThrowError("You must pass a query string and callback to executeQuery().");
    NanReturnUndefined();
  }

  if (argsLength < 2) {
    NanThrowError("You must pass a callback to executeQuery().");
    NanReturnUndefined();
  }

  Local<Function> callbackFunction;
  Local<Value> poolNameValue(NanUndefined());
  Local<Value> queryParams;

  // .executeQuery(query, function)
  if (args[1]->IsFunction()) {
    callbackFunction = args[1].As<Function>();
    // .executeQuery(query, optionsHash, function)
  } else if (argsLength > 2 && args[2]->IsFunction()) {
    callbackFunction = args[2].As<Function>();

    if (args[1]->IsObject() && !args[1]->IsFunction()) {
      Local<Object> optionsObject = args[1]->ToObject();
      poolNameValue = optionsObject->Get(NanNew("poolName"));
    }
    // .executeQuery(query, paramsArray, optionsHash, function)
  } else if (argsLength > 3 && args[3]->IsFunction()) {
    callbackFunction = args[3].As<Function>();

    if (args[1]->IsArray() && !args[1]->IsFunction()) {
      queryParams = args[1];
    }

    if (args[2]->IsObject() && !args[2]->IsFunction()) {
      Local<Object> optionsObject = args[2]->ToObject();
      poolNameValue = optionsObject->Get(NanNew("poolName"));
    }
  } else {
    NanThrowError("You must pass a function as the callback to executeQuery().");
    NanReturnUndefined();
  }

  Cache * cache = ObjectWrap::Unwrap<Cache>(args.This());
  CachePtr cachePtr(cache->cachePtr);

  if (cache->cachePtr->isClosed()) {
    NanThrowError("Cannot execute query; cache is closed.");
    NanReturnUndefined();
  }

  QueryServicePtr queryServicePtr;
  CacheableVectorPtr queryParamsPtr = NULLPTR;

  std::string queryString(*NanUtf8String(args[0]));

  try {
    if (poolNameValue->IsUndefined()) {
      queryServicePtr = cachePtr->getQueryService();
    } else {
      std::string poolName(*NanUtf8String(poolNameValue));
      PoolPtr poolPtr(getPool(poolNameValue));

      if (poolPtr == NULLPTR) {
        std::string poolName(*NanUtf8String(poolNameValue));
        std::stringstream errorMessageStream;
        errorMessageStream << "executeQuery: `" << poolName << "` is not a valid pool name";
        NanThrowError(errorMessageStream.str().c_str());
        NanReturnUndefined();
      }

      queryServicePtr = cachePtr->getQueryService(poolName.c_str());
    }
  } catch (const apache::geode::client::Exception & exception) {
    ThrowGemfireException(exception);
    NanReturnUndefined();
  }
  if (!(queryParams.IsEmpty() || queryParams->IsUndefined())) {
    queryParamsPtr = gemfireVector(queryParams.As<Array>(), cachePtr);
  }

  QueryPtr queryPtr(queryServicePtr->newQuery(queryString.c_str()));

  NanCallback * callback = new NanCallback(callbackFunction);

  ExecuteQueryWorker * worker = new ExecuteQueryWorker(queryPtr, queryParamsPtr, callback);
  NanAsyncQueueWorker(worker);

  NanReturnValue(args.This());
}

NAN_METHOD(Cache::CreateRegion) {
  NanScope();

  if (args.Length() < 1) {
    NanThrowError(
        "createRegion: You must pass the name of a GemFire region to create "
        "and a region configuration object.");
    NanReturnUndefined();
  }

  if (!args[0]->IsString()) {
    NanThrowError("createRegion: You must pass a string as the name of a GemFire region.");
    NanReturnUndefined();
  }

  if (!args[1]->IsObject()) {
    NanThrowError("createRegion: You must pass a configuration object as the second argument.");
    NanReturnUndefined();
  }

  Local<Object> regionConfiguration(args[1]->ToObject());

  Local<Value> regionType(regionConfiguration->Get(NanNew("type")));
  if (regionType->IsUndefined()) {
    NanThrowError("createRegion: The region configuration object must have a type property.");
    NanReturnUndefined();
  }

  Local<Value> regionPoolName(regionConfiguration->Get(NanNew("poolName")));

  RegionShortcut regionShortcut(getRegionShortcut(*NanUtf8String(regionType)));
  if (regionShortcut == invalidRegionShortcut) {
    NanThrowError("createRegion: This type is not a valid GemFire client region type");
    NanReturnUndefined();
  }

  Cache * cache = ObjectWrap::Unwrap<Cache>(args.This());
  CachePtr cachePtr(cache->cachePtr);

  RegionPtr regionPtr;
  try {
    RegionFactoryPtr regionFactoryPtr(cachePtr->createRegionFactory(regionShortcut));

    if (!regionPoolName->IsUndefined()) {
      regionFactoryPtr->setPoolName(*NanUtf8String(regionPoolName));
    }

    regionPtr = regionFactoryPtr->create(*NanUtf8String(args[0]));
  } catch (const apache::geode::client::Exception & exception) {
    ThrowGemfireException(exception);
    NanReturnUndefined();
  }

  NanReturnValue(Region::New(args.This(), regionPtr));
}

NAN_METHOD(Cache::GetRegion) {
  NanScope();

  if (args.Length() != 1) {
    NanThrowError("You must pass the name of a GemFire region to getRegion.");
    NanReturnUndefined();
  }

  if (!args[0]->IsString()) {
    NanThrowError("You must pass a string as the name of a GemFire region to getRegion.");
    NanReturnUndefined();
  }

  Cache * cache = ObjectWrap::Unwrap<Cache>(args.This());
  CachePtr cachePtr(cache->cachePtr);
  RegionPtr regionPtr(cachePtr->getRegion(*NanAsciiString(args[0])));

  NanReturnValue(Region::New(args.This(), regionPtr));
}

NAN_METHOD(Cache::RootRegions) {
  NanScope();

  Cache * cache = ObjectWrap::Unwrap<Cache>(args.This());

  VectorOfRegion regions;
  cache->cachePtr->rootRegions(regions);

  unsigned int size = regions.size();
  Local<Array> rootRegions(NanNew<Array>(size));

  for (unsigned int i = 0; i < size; i++) {
    rootRegions->Set(i, Region::New(args.This(), regions[i]));
  }

  NanReturnValue(rootRegions);
}

NAN_METHOD(Cache::Inspect) {
  NanScope();
  NanReturnValue(NanNew("[Cache]"));
}

NAN_METHOD(Cache::ExecuteFunction) {
  NanScope();

  Cache * cache = ObjectWrap::Unwrap<Cache>(args.This());
  CachePtr cachePtr(cache->cachePtr);
  if (cachePtr->isClosed()) {
    NanThrowError("Cannot execute function; cache is closed.");
    NanReturnUndefined();
  }

  Local<Value> poolNameValue(NanUndefined());
  if (args[1]->IsObject() && !args[1]->IsArray()) {
    Local<Object> optionsObject = args[1]->ToObject();

    Local<Value> filter = optionsObject->Get(NanNew("filter"));
    if (!filter->IsUndefined()) {
      NanThrowError("You cannot pass a filter to executeFunction for a Cache.");
      NanReturnUndefined();
    }

    poolNameValue = optionsObject->Get(NanNew("poolName"));
  }

  try {
    PoolPtr poolPtr(getPool(poolNameValue));

    if (poolPtr == NULLPTR) {
      std::string poolName(*NanUtf8String(poolNameValue));
      std::stringstream errorMessageStream;
      errorMessageStream << "executeFunction: `" << poolName << "` is not a valid pool name";
      NanThrowError(errorMessageStream.str().c_str());
      NanReturnUndefined();
    }

    ExecutionPtr executionPtr(FunctionService::onServer(poolPtr));
    NanReturnValue(executeFunction(args, cachePtr, executionPtr));
  } catch (const apache::geode::client::Exception & exception) {
    ThrowGemfireException(exception);
    NanReturnUndefined();
  }
}

PoolPtr Cache::getPool(const Handle<Value> & poolNameValue) {
  if (!poolNameValue->IsUndefined()) {
    std::string poolName(*NanUtf8String(poolNameValue));
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
