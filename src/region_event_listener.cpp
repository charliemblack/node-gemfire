#include "region_event_listener.hpp"
#include "region_event_registry.hpp"

using namespace apache::geode::client;

namespace node_gemfire {
void RegionEventListener::afterCreate(const EntryEvent & event) {
  RegionEventRegistry::getInstance()->emit("create", event);
}
void RegionEventListener::afterUpdate(const EntryEvent & event) {
  RegionEventRegistry::getInstance()->emit("update", event);
}
void RegionEventListener::afterDestroy(const EntryEvent & event) {
  RegionEventRegistry::getInstance()->emit("destroy", event);
}
RegionEventRegistry RegionEventRegistry::instance = RegionEventRegistry();
}  // namespace node_gemfire
