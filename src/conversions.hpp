#ifndef __CONVERSIONS_HPP__
#define __CONVERSIONS_HPP__

#include <v8.h>
#include <nan.h>
#include <geode/PdxInstanceFactory.hpp>
#include <geode/CacheFactory.hpp>
#include <string>
#include <cstdint>
#ifdef __linux__
#include <sys/time.h>
#endif

namespace node_gemfire {

apache::geode::client::CacheablePtr gemfireValue(const v8::Local<v8::Value> & v8Value,
                                         const apache::geode::client::CachePtr & cachePtr);
apache::geode::client::PdxInstancePtr gemfireValue(const v8::Local<v8::Object> & v8Object,
                                           const apache::geode::client::CachePtr & cachePtr);
apache::geode::client::CacheableArrayListPtr gemfireValue(const v8::Local<v8::Array> & v8Value,
                                         const apache::geode::client::CachePtr & cachePtr);
apache::geode::client::CacheableDatePtr gemfireValue(const v8::Local<v8::Date> & v8Value);

apache::geode::client::CacheableKeyPtr gemfireKey(const v8::Local<v8::Value> & v8Value,
                                          const apache::geode::client::CachePtr & cachePtr);
apache::geode::client::VectorOfCacheableKeyPtr gemfireKeys(const v8::Local<v8::Array> & v8Value,
                                          const apache::geode::client::CachePtr & cachePtr);

apache::geode::client::HashMapOfCacheablePtr gemfireHashMap(const v8::Local<v8::Object> & v8Object,
                                           const apache::geode::client::CachePtr & cachePtr);
apache::geode::client::CacheableVectorPtr gemfireVector(const v8::Local<v8::Array> & v8Array,
                                           const apache::geode::client::CachePtr & cachePtr);

v8::Local<v8::Value> v8Value(const apache::geode::client::CacheablePtr & valuePtr);
v8::Local<v8::Value> v8Value(const apache::geode::client::CacheableKeyPtr & keyPtr);
v8::Local<v8::Value> v8Value(const apache::geode::client::CacheableInt64Ptr & valuePtr);
v8::Local<v8::Object> v8Value(const apache::geode::client::StructPtr & structPtr);
v8::Local<v8::Value> v8Value(const apache::geode::client::PdxInstancePtr & pdxInstancePtr);
v8::Local<v8::Object> v8Value(const apache::geode::client::SelectResultsPtr & selectResultsPtr);
v8::Local<v8::Object> v8Value(const apache::geode::client::CacheableHashMapPtr & hashMapPtr);
v8::Local<v8::Object> v8Value(const apache::geode::client::HashMapOfCacheablePtr & hashMapPtr);
v8::Local<v8::Object> v8Value(const apache::geode::client::RegionEntryPtr & regionEntryPtr);
v8::Local<v8::Array> v8Value(const apache::geode::client::VectorOfCacheablePtr & vectorPtr);
v8::Local<v8::Array> v8Value(const apache::geode::client::VectorOfCacheableKeyPtr & vectorPtr);
v8::Local<v8::Array> v8Value(const apache::geode::client::VectorOfRegionEntry & vectorPtr);
v8::Local<v8::Date> v8Value(const apache::geode::client::CacheableDatePtr & datePtr);
v8::Local<v8::Boolean> v8Value(bool value);

template<typename T>
v8::Local<v8::Array> v8Array(const apache::geode::client::SharedPtr<T> & iterablePtr) {
  Nan::EscapableHandleScope scope;

  unsigned int length = iterablePtr->size();
  v8::Local<v8::Array> v8Array(Nan::New<v8::Array>(length));

  unsigned int i = 0;
  for (typename T::Iterator iterator(iterablePtr->begin());
       iterator != iterablePtr->end();
       ++iterator) {
    v8Array->Set(i, v8Value(*iterator));
    i++;
  }

  return scope.Escape(v8Array);
}

template<typename T>
v8::Local<v8::Object> v8Object(const apache::geode::client::SharedPtr<T> & hashMapPtr) {
  Nan::EscapableHandleScope scope;
  v8::Local<v8::Object> v8Object(Nan::New<v8::Object>());

  for (typename T::Iterator iterator = hashMapPtr->begin();
       iterator != hashMapPtr->end();
       iterator++) {
    apache::geode::client::CacheablePtr keyPtr(iterator.first());
    apache::geode::client::CacheablePtr valuePtr(iterator.second());

    v8Object->Set(v8Value(keyPtr),
        v8Value(valuePtr));
  }

  return scope.Escape(v8Object);
}

std::string getClassName(const v8::Local<v8::Object> & v8Object);

}  // namespace node_gemfire

#endif
