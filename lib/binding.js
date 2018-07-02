const nodePreGyp = require('node-pre-gyp');
const path = require('path');
const EventEmitter = require('events').EventEmitter;

function inherits(target, source) {
  for (var key in source.prototype) {
    target.prototype[key] = source.prototype[key];
  }
}

module.exports = function binding(options) {
  const bindingPath = nodePreGyp.find(
    path.resolve(path.join(__dirname,'../package.json')),
    options || {}
  );
  const initialize = require(bindingPath).initialize;

  const gemfire = initialize({
    EventEmitter: EventEmitter,
    process: process
  });
  
  var cacheSingleton;

  gemfire.configure = function configure(xmlFilePath, gemfireProperties) {
    if(cacheSingleton) {
      throw(
        "gemfire: configure() can only be called once per process. " +
        "Please call configure() once in an application initializer. " +
        "Afterwards, you can call getCache() multiple times to get the cache singleton object."
      );
    }
    var cacheFactory = new CacheFactory(gemfireProperties,  function(cacheInstance){
      cacheSingleton = cacheInstance;
    });
    cacheFactory.set("cache-xml-file", xmlFilePath);
    var cacheInstance = cacheFactory.create();
  };

  const Cache = gemfire.Cache;
  const CacheFactory = gemfire.CacheFactory;

  gemfire.createCacheFactory = function createCacheFactory(gemfireProperties){
    if(cacheSingleton) {
      throw(
        "gemfire: configure() or createCacheFactory() can only be called once per process. " +
        "Please call configure() or createCacheFactory() after a create() once in an application initializer. " +
        "Afterwards, you can call getCache() multiple times to get the cache singleton object."
      );
    }
    var cacheFactory = new CacheFactory(gemfireProperties,  function(cacheInstance){
        cacheSingleton = cacheInstance;
    });
    return cacheFactory;
  };

  gemfire.getCache = function getCache() {
    if(!cacheSingleton) {
      throw "gemfire: You must call configure() before calling getCache().";
    }
    return cacheSingleton;
  };

  delete gemfire.Cache;
  delete gemfire.CacheFactory;
  inherits(gemfire.Region, EventEmitter);
  delete gemfire.Region;

  return gemfire;
};
