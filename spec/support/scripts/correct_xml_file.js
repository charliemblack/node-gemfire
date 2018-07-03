const gemfire = require("../gemfire.js");
gemfire.configure("xml/ExampleClient.xml");
const cache = gemfire.getCache();

if(!cache.getRegion('exampleRegion')) {
  throw("Region not found");
}
