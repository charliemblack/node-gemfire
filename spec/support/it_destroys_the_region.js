const _ = require("lodash");
const async = require('async');

const factories = require('./factories.js');
const until = require("./until.js");

module.exports = function itDestroysTheRegion(methodName) {
  var cache;
  var region;
  var regionName;
  var otherCopyOfRegion;

  beforeEach(function() {
    try{
    cache = factories.getCache();

    regionName = "regionToDestroy" + Date.now();
    expect(cache.getRegion(regionName)).toBeUndefined();

    region = cache.createRegion(regionName, {type: "LOCAL"});
    otherCopyOfRegion = cache.getRegion(regionName);
    }catch(ex){
      console.log(ex)
    }
  });

  it("destroys the region", function(done) {
    region[methodName](function (error) {
      expect(error).not.toBeError();
      console.log("deleted region " + regionName);
      console.log("getting region " + cache.getRegion(regionName));
      expect(cache.getRegion(regionName)).not.toBeDefined();
      done();
    });
  });

  it("throws an error if the callback is not a function", function() {
    function callWithNonFunction() {
      region[methodName]("this is not a function");
    }

    expect(callWithNonFunction).toThrow(
      new Error("You must pass a function as the callback to " + methodName + "().")
    );
  });

  it("emits an event when an error occurs and there is no callback", function(done) {
    try{
    region[methodName]();

    const errorHandler = jasmine.createSpy("errorHandler").and.callFake(function(error){
      expect(error).toBeError();
      done();
    });

    region.on("error", errorHandler);

    region[methodName](); // destroying already destroyed region should emit an error

    _.delay(function(){
      expect(errorHandler).toHaveBeenCalled();
      done();
    }, 1000);
    }catch(ex){
      console.log(ex);
    }
  });

  // if an error event is emitted, the test suite will crash here
  it("does not emit an event when no error occurs and there is no callback", function(done) {
    console.log("Running test - does not emit an event when no error occurs and there is no callback" )
    
    until(
      function(test) { test(cache.getRegion(regionName)); },
      function(destroyedRegion) { return !destroyedRegion; },
      done
    );

    region[methodName]();
    console.log("Running test - does not emit an event when no error occurs and there is no callback" )
  });

  it("prevents subsequent operations on the region object that received the call", function(done) {
    console.log("Running test - prevents subsequent operations on the region object that received the call" )
    try {
      console.log("method name = " + methodName);
      region[methodName](function (error) {
        try{
        expect(error).not.toBeError();
        expect(function(){ 
          try{
            console.log("a ********");
          region.put("foo", "bar"); 
          console.log("b *****");
          }catch(ex){
            console.log(ex);
          }
        }).toThrow(
          "apache::geode::client::RegionDestroyedException",
          "LocalRegion::getCache: region /" + regionName + " destroyed"
        );
        done();
      }catch(ex){
        console.log(ex)
      }
        
      });
    } catch (ex) {
      console.log(ex);
    }
    console.log("Running test - prevents subsequent operations on the region object that received the call" )
  });

  it("prevents subsequent operations on other pre-existing region objects", function(done) {
    region[methodName](function (error) {
      console.log("error +++++++++ = " + error);
      expect(error).not.toBeError();
      console.log("error ========= = " + error);
      expect(function(){
        console.log("foo *************************");
        otherCopyOfRegion.put("foo", "bar");
        console.log("bar *************************");
      }).toBeError(
        "apache::geode::client::RegionDestroyedException",
        "Region::put: Named Region Destroyed"
      );
      done();
    });
  });

  it("passes GemFire exceptions into the callback", function(done) {
    async.series([
      function(next) {
      region[methodName](function(error) {
        expect(error).not.toBeError();
        next();
      });
    },

    function(next) {
      // destroying an already destroyed region causes an error
      region[methodName](function (error) {
        expect(error).toBeError(
          "apache::geode::client::RegionDestroyedException",
          "Region::" + methodName + ": Named Region Destroyed"
        );
        next();
      });
    }

    ], done);
  });
};
