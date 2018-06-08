const _ = require("lodash");
const expectErrorMessage = require("./expect_error_message.js");

const cache = require("../factories.js").getCache();
const region = cache.getRegion('exampleRegion');

cache.close();

try {
  region.put("foo", "bar");
  throw new Error("region.put() did not throw an error after cache.close().");
} catch (error) {
  expectErrorMessage(error, "Region name exampleRegion is invalid because the Cache is Closed.");
}

var clearEmittedError = false;

region.on("error", function(error) {
  expectErrorMessage(error, "LocalRegion::getRegionService: region /exampleRegion destroyed");
  clearEmittedError = true;
});

region.clear();

process.on("exit", function(){
  if(!clearEmittedError) {
    throw new Error("region.clear() did not emit an error after cache.close().");
  }
});
