#include <node.h>
#include <cstdint>
#include <nan.h>
#include <v8.h>
#include <math.h>
#include <geode/GeodeCppCache.hpp>
#include <string>
#include <sstream>
#include <set>
#include "conversions.hpp"
#include "exceptions.hpp"
#include "select_results.hpp"

using namespace std;
using namespace chrono;
using namespace v8;
using namespace apache::geode::client;

namespace node_gemfire {

std::string getClassName(const Local<Object> & v8Object) {
   Nan::HandleScope scope;

  std::set<std::string> fieldNames;
  uint totalSize = 0;

  Local<Array> v8Keys(v8Object->GetOwnPropertyNames());
  unsigned int numKeys = v8Keys->Length();
  for (unsigned int i = 0; i < numKeys; i++) {
    Local<Value> v8Key(v8Keys->Get(i));
    Nan::Utf8String utf8FieldName(v8Key);
    char * fieldName = *utf8FieldName;

    unsigned int size = utf8FieldName.length();
    std::string fullFieldName;
    fullFieldName.reserve((size * 2) + 3);  // escape every character, plus '[],'

    for (unsigned int j = 0; j < size - 1; j++) {
      char fieldNameChar = fieldName[j];
      switch (fieldNameChar) {
        case ',':
        case '[':
        case ']':
        case '\\':
          fullFieldName += '\\';
      }
      fullFieldName += fieldNameChar;
    }

    Local<Value> v8Value(v8Object->Get(v8Key));
    if (v8Value->IsArray() && !v8Value->IsString()) {
      fullFieldName += "[]";
    }
    fullFieldName += ',';

    fieldNames.insert(fullFieldName);
    totalSize += fullFieldName.length();
  }

  std::string className;
  className.reserve(totalSize + 7);
  className += "JSON: ";

  for (std::set<std::string>::iterator i(fieldNames.begin()); i != fieldNames.end(); ++i) {
    className += *i;
  }
  return className;
}

std::wstring wstringFromV8String(const Local<String> & v8String) {
  Nan::HandleScope scope;

  String::Value v8StringValue(v8String);
  uint16_t * v8StringData(*v8StringValue);

  unsigned int length = v8String->Length();
  wchar_t * buffer = new wchar_t[length + 1];
  for (unsigned int i = 0; i < length; i++) {
    buffer[i] = v8StringData[i];
  }
  buffer[length] = 0;

  std::wstring wstring(buffer);
  delete[] buffer;

  return wstring;
}

Local<String> v8StringFromWstring(const std::wstring & wideString) {
  Nan::EscapableHandleScope scope;

  unsigned int length = wideString.length();
  uint16_t * buffer = new uint16_t[length + 1];
  for (unsigned int i = 0; i < length; i++) {
    buffer[i] = wideString[i];
  }
  buffer[length] = 0;
 
  Local<String> v8String = Nan::New(buffer).ToLocalChecked();
  delete[] buffer;

  return scope.Escape(v8String);
}

void ConsoleWarn(const char * message) {
   Nan::HandleScope scope;

  auto global =Nan::GetCurrentContext()->Global();
  auto consoleObject = global->Get( Nan::New("console").ToLocalChecked()).As<Object>();
  auto warnFunction = consoleObject->Get(Nan::New("warn").ToLocalChecked()).As<Function>();
  Local<Value> argv[1] = { Nan::New(message).ToLocalChecked() };
  Nan::Callback callback(warnFunction);
  callback.Call(1, argv);
}

CacheablePtr gemfireValue(const Local<Value> & v8Value, const CachePtr & cachePtr) {
  if (v8Value->IsString() || v8Value->IsStringObject()) {
    std::wstring wideString = wstringFromV8String(v8Value->ToString());
    const wchar_t* readOnlyWideChar = wideString.c_str();
    return CacheableString::create(readOnlyWideChar);
  } else if (v8Value->IsBoolean()) {
    return CacheableBoolean::create(v8Value->ToBoolean()->Value());
  } else if (v8Value->IsNumber() || v8Value->IsNumberObject()) {
    return CacheableDouble::create(v8Value->ToNumber()->Value());
  } else if (v8Value->IsDate()) {
    return gemfireValue(Local<Date>::Cast(v8Value));
  } else if (v8Value->IsArray()) {
    return gemfireValue(Local<Array>::Cast(v8Value), cachePtr);
  } else if (v8Value->IsBooleanObject()) {
#if (NODE_MODULE_VERSION > 0x000B)
    return CacheableBoolean::create(BooleanObject::Cast(*v8Value)->ValueOf());
#else
    return CacheableBoolean::create(BooleanObject::Cast(*v8Value)->BooleanValue());
#endif
  } else if (v8Value->IsFunction()) {
    Nan::ThrowError("Unable to serialize to GemFire; functions are not supported.");
    return NULLPTR;
  } else if (v8Value->IsObject()) {
    return gemfireValue(v8Value->ToObject(), cachePtr);
  } else if (v8Value->IsUndefined()) {
    return CacheableUndefined::create();
  } else if (v8Value->IsNull()) {
    return NULLPTR;
  } else {
    std::string errorMessage("Unable to serialize to GemFire; unknown JavaScript object: ");
    errorMessage.append(*Nan::Utf8String(v8Value->ToDetailString()));
    Nan::ThrowError(errorMessage.c_str());
    return NULLPTR;
  }
}

PdxInstancePtr gemfireValue(const Local<Object> & v8Object, const CachePtr & cachePtr) {
    Nan::EscapableHandleScope scope;
  try {
    std::string pdxClassName = getClassName(v8Object);
    PdxInstanceFactoryPtr pdxInstanceFactory = cachePtr->createPdxInstanceFactory(pdxClassName.c_str());
    Local<Array> v8Keys(v8Object->GetOwnPropertyNames());
    unsigned int length = v8Keys->Length();
    for (unsigned int i = 0; i < length; i++) {
      Local<Value> v8Key(v8Keys->Get(i));
      Local<Value> v8Value(v8Object->Get(v8Key));
      Nan::Utf8String fieldName(v8Key);
      CacheablePtr cacheablePtr(gemfireValue(v8Value, cachePtr));
      pdxInstanceFactory->writeObject(*fieldName, cacheablePtr);
    }
    return pdxInstanceFactory->create();
  }
  catch(const apache::geode::client::Exception & exception) {
    ThrowGemfireException(exception);
    return NULLPTR;
  }
}

apache::geode::client::CacheableArrayListPtr gemfireValue(const Local<Array> & v8Array,
                                         const apache::geode::client::CachePtr & cachePtr) {
  CacheableArrayListPtr arrayListPtr(CacheableArrayList::create());
  unsigned int length = v8Array->Length();
  for (unsigned int i = 0; i < length; i++) {
    arrayListPtr->push_back(gemfireValue(v8Array->Get(i), cachePtr));
  }
  return arrayListPtr;
}

apache::geode::client::CacheableDatePtr gemfireValue(const Local<Date> & v8Date) {
  long int millisecondsSinceEpoch = v8Date->NumberValue();
  std::chrono::milliseconds dur(millisecondsSinceEpoch);
  std::chrono::time_point<std::chrono::system_clock> dt(dur);
  return CacheableDate::create(dt);
}

CacheableKeyPtr gemfireKey(const Local<Value> & v8Value, const CachePtr & cachePtr) {
  CacheableKeyPtr keyPtr;
  try {
    keyPtr = gemfireValue(v8Value, cachePtr);
  }
  catch(const ClassCastException & exception) {
    return NULLPTR;
  }

  return keyPtr;
}

VectorOfCacheableKeyPtr gemfireKeys(const Local<Array> & v8Value,
                                          const CachePtr & cachePtr) {
  VectorOfCacheableKeyPtr vectorPtr(new VectorOfCacheableKey());

  for (unsigned int i = 0; i < v8Value->Length(); i++) {
    CacheableKeyPtr keyPtr = gemfireKey(v8Value->Get(i), cachePtr);

    if (keyPtr == NULLPTR) {
      return NULLPTR;
    } else {
      vectorPtr->push_back(keyPtr);
    }
  }

  return vectorPtr;
}

HashMapOfCacheablePtr gemfireHashMap(const Local<Object> & v8Object,
                                           const CachePtr & cachePtr) {
  Nan::HandleScope scope;

  HashMapOfCacheablePtr hashMapPtr(new HashMapOfCacheable());

  Local<Array> v8Keys(v8Object->GetOwnPropertyNames());
  unsigned int length = v8Keys->Length();

  for (unsigned int i = 0; i < length; i++) {
    Local<String> v8Key(v8Keys->Get(i)->ToString());

    CacheableKeyPtr keyPtr(gemfireKey(v8Key, cachePtr));
    CacheablePtr valuePtr(gemfireValue(v8Object->Get(v8Key), cachePtr));

    if (valuePtr == NULLPTR) {
      return NULLPTR;
    }

    hashMapPtr->insert(keyPtr, valuePtr);
  }

  return hashMapPtr;
}

CacheableVectorPtr gemfireVector(const Local<Array> & v8Array, const CachePtr & cachePtr) {
  Nan::HandleScope scope;

  unsigned int length = v8Array->Length();
  CacheableVectorPtr vectorPtr = CacheableVector::create();

  for (unsigned int i = 0; i < length; i++) {
    vectorPtr->push_back(gemfireValue(v8Array->Get(i), cachePtr));
  }

  return vectorPtr;
}

Local<Value> v8Value(const CacheablePtr & valuePtr) {
 Nan::EscapableHandleScope scope;

  if (valuePtr == NULLPTR) {
    return scope.Escape(Nan::Null());
  }

  int typeId = valuePtr->typeId();
  switch (typeId) {
    case GeodeTypeIds::CacheableASCIIString:
    case GeodeTypeIds::CacheableASCIIStringHuge:
    case GeodeTypeIds::CacheableString:
    case GeodeTypeIds::CacheableStringHuge:
    {  
      CacheableStringPtr cacheableStringPtr = static_cast<CacheableStringPtr>(valuePtr);
      if(cacheableStringPtr->isWideString()){
        return scope.Escape(v8StringFromWstring(cacheableStringPtr->asWChar()));
      }
      //else
      return scope.Escape(Nan::New(cacheableStringPtr->asChar()).ToLocalChecked());
    }
    case GeodeTypeIds::CacheableBoolean:
      return scope.Escape(Nan::New((static_cast<CacheableBooleanPtr>(valuePtr))->value()));
    case GeodeTypeIds::CacheableDouble:
      return scope.Escape(Nan::New((static_cast<CacheableDoublePtr>(valuePtr))->value()));
    case GeodeTypeIds::CacheableFloat:
      return scope.Escape(Nan::New((static_cast<CacheableFloatPtr>(valuePtr))->value()));
    case GeodeTypeIds::CacheableInt16:
      return scope.Escape(Nan::New((static_cast<CacheableInt16Ptr>(valuePtr))->value()));
    case GeodeTypeIds::CacheableInt32:
      return scope.Escape(Nan::New((static_cast<CacheableInt32Ptr>(valuePtr))->value()));
    case GeodeTypeIds::CacheableInt64:
      return scope.Escape(v8Value(static_cast<CacheableInt64Ptr>(valuePtr)));
    case GeodeTypeIds::CacheableDate:
      return scope.Escape(v8Value(static_cast<CacheableDatePtr>(valuePtr)));
    case GeodeTypeIds::CacheableUndefined:
      return scope.Escape(Nan::Undefined());
    case GeodeTypeIds::Struct:
      return scope.Escape(v8Value(static_cast<StructPtr>(valuePtr)));
    case GeodeTypeIds::CacheableObjectArray:
      return scope.Escape(v8Array(static_cast<CacheableObjectArrayPtr>(valuePtr)));
    case GeodeTypeIds::CacheableArrayList:
      return scope.Escape(v8Array(static_cast<CacheableArrayListPtr>(valuePtr)));
    case GeodeTypeIds::CacheableVector:
      return scope.Escape(v8Array(static_cast<CacheableVectorPtr>(valuePtr)));
    case GeodeTypeIds::CacheableHashMap:
      return scope.Escape(v8Object(static_cast<CacheableHashMapPtr>(valuePtr)));
    case GeodeTypeIds::CacheableHashSet:
      return scope.Escape(v8Array(static_cast<CacheableHashSetPtr>(valuePtr)));
    case 0:
      try {
        UserFunctionExecutionExceptionPtr functionExceptionPtr =
          static_cast<UserFunctionExecutionExceptionPtr>(valuePtr);

        return scope.Escape(v8Error(functionExceptionPtr));
      } catch (ClassCastException & exception) {
        // fall through to default error case
      }
      break;
  }

  if (typeId > GeodeTypeIds::CacheableStringHuge) {
    // We are assuming these are Pdx
    return scope.Escape(v8Value(static_cast<PdxInstancePtr>(valuePtr)));
  }
  std::stringstream errorMessageStream;
  errorMessageStream << "Unable to serialize value from GemFire; unknown typeId: " << typeId;
  Nan::ThrowError(errorMessageStream.str().c_str());
  return scope.Escape(Nan::Undefined());
}

Local<Value> v8Value(const PdxInstancePtr & pdxInstance) {
  Nan::EscapableHandleScope scope;

  try {
    CacheableStringArrayPtr gemfireKeys(pdxInstance->getFieldNames());

    if (gemfireKeys == NULLPTR) {
      return scope.Escape(Nan::New<Object>());
    }

    Local<Object> v8Object = Nan::New<Object>();
    int length = gemfireKeys->length();

    for (int i = 0; i < length; i++) {
      const char * key = gemfireKeys[i]->asChar();
      CacheablePtr value;
      if (pdxInstance->getFieldType(key) == apache::geode::client::PdxFieldTypes::OBJECT_ARRAY) {
        CacheableObjectArrayPtr valueArray;
        pdxInstance->getField(key, valueArray);
        value = valueArray;
      } else {
        pdxInstance->getField(key, value);
      }
      Nan::Set(v8Object, Nan::New(key).ToLocalChecked(),v8Value(value)); 
    }

    return scope.Escape(v8Object);
  }
  catch(const apache::geode::client::Exception & exception) {
    ThrowGemfireException(exception);
    Nan::EscapableHandleScope scope;
    return scope.Escape(Nan::Undefined());
  }
}

Local<Value> v8Value(const CacheableInt64Ptr & valuePtr) {
   Nan::EscapableHandleScope scope;

  static const int64_t maxSafeInteger = pow(2, 53) - 1;
  static const int64_t minSafeInteger = -1 * maxSafeInteger;

  int64_t value = static_cast<CacheableInt64Ptr>(valuePtr)->value();
  if (value > maxSafeInteger) {
    ConsoleWarn("Received 64 bit integer from GemFire greater than Number.MAX_SAFE_INTEGER (2^53 - 1)");
  } else if (value < minSafeInteger) {
    ConsoleWarn("Received 64 bit integer from GemFire less than Number.MIN_SAFE_INTEGER (-1 * 2^53 + 1)");
  }

  return scope.Escape(Nan::New<Number>(value));
}

Local<Date> v8Value(const CacheableDatePtr & datePtr) {
  Nan::EscapableHandleScope scope;
  double epochMillis = datePtr->milliseconds();
  return scope.Escape(Nan::New<Date>(epochMillis).ToLocalChecked());
}


Local<Value> v8Value(const CacheableKeyPtr & keyPtr) {
  return v8Value(static_cast<CacheablePtr>(keyPtr));
}

Local<Object> v8Value(const StructPtr & structPtr) {
  Nan::EscapableHandleScope scope;
  Local<Object> v8Object(Nan::New<Object>());
  unsigned int length = structPtr->length();
  for (unsigned int i = 0; i < length; i++) {
    Nan::Set(v8Object, Nan::New(structPtr->getFieldName(i)).ToLocalChecked(), v8Value((*structPtr)[i]));
  }
  return scope.Escape(v8Object);
}

Local<Object> v8Value(const HashMapOfCacheablePtr & hashMapPtr) {
  return v8Object(hashMapPtr);
}

Local<Array> v8Value(const VectorOfCacheablePtr & vectorPtr) {
  return v8Array(vectorPtr);
}

Local<Array> v8Value(const VectorOfCacheableKeyPtr & vectorPtr) {
  return v8Array(vectorPtr);
}

Local<Object> v8Value(const RegionEntryPtr & regionEntryPtr) {
  Nan::EscapableHandleScope scope;
  Local<Object> v8Object(Nan::New<Object>());
  Nan::Set(v8Object, Nan::New("key").ToLocalChecked(), v8Value(regionEntryPtr->getKey()));
  Nan::Set(v8Object, Nan::New("value").ToLocalChecked(), v8Value(regionEntryPtr->getValue()));
  return scope.Escape(v8Object);
}

Local<Array> v8Value(const VectorOfRegionEntry & vectorOfRegionEntries) {
  Nan::EscapableHandleScope scope;
  Local<Array> v8Array(Nan::New<Array>());
  unsigned int length = vectorOfRegionEntries.length();
  for (unsigned int i = 0; i < length; i++) {
    Nan::Set(v8Array, i, v8Value(vectorOfRegionEntries[i]));
  }
  return scope.Escape(v8Array);
}

Local<Object> v8Value(const SelectResultsPtr & selectResultsPtr) {
  Nan::EscapableHandleScope scope;

  Local<Object> selectResults(SelectResults::NewInstance(selectResultsPtr));

  return scope.Escape(selectResults);
}

Local<Boolean> v8Value(bool value) {
  Nan::EscapableHandleScope scope;
  return scope.Escape(Nan::New(value));
}

}  // namespace node_gemfire
