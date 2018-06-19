const _ = require("lodash");
const util = require("util");
const async = require('async');
const randomString = require("random-string");

const factories = require('./support/factories.js');
const errorMatchers = require("./support/error_matchers.js");
const until = require("./support/until.js");
const waitUntil = require("./support/wait_until.js");
const itExecutesFunctions = require("./support/it_executes_functions.js");
const itDestroysTheRegion = require("./support/it_destroys_the_region.js");

const invalidKeys = [null, undefined, []];
const invalidValues = [null];

describe("gemfire.Region", function() {
  var region, proxyRegion, cache;

  beforeEach(function(done) {
    jasmine.addMatchers(errorMatchers);
    cache = factories.getCache();
    region = cache.getRegion("exampleRegion");
    region.clear(done);
  });

  describe(".get", function() {
    it("throws an error if a key is not passed to .get", function() {
      function getWithoutKey() {
        region.get();
      }
      expect(getWithoutKey).toThrow(new Error("You must pass a key and a callback to get()."));
    });

    it("throws an error if a non-function is passed as a callback", function() {
      function getWithNonFunctionCallback() {
        region.get("foo", "bar");
      }

      expect(getWithNonFunctionCallback).toThrow(new Error("You must pass a function as the callback to get()."));
    });

    it("returns the region object to support chaining", function(done) {
      var returnValue = region.get("foo", function(error, value) {
        done();
      });

      expect(returnValue).toEqual(region);
    });

    it("returns a null for a nonexisitant key", function(done) {
      region.get("baz", function(error, value) {
        expect(value).toBeNull();
        done();
      });
    });

    _.each(invalidKeys, function(invalidKey) {
      it("passes an error to the callback when passed the invalid key " + util.inspect(invalidKey), function(done) {
        region.get(invalidKey, function(error, value) {
          expect(error).toBeError("InvalidKeyError", "Invalid GemFire key.");
          expect(value).toBeUndefined();
          done();
        });
      });
    });

    it("throws an error when passed a function as a key", function() {
      function callWithFunctionKey() {
        region.get(function(){}, function(){});
      }

      expect(callWithFunctionKey).toThrow(new Error("Unable to serialize to GemFire; functions are not supported."));
    });

    it("passes the value into the callback", function(done) {
      region.put('foo', 'bar', function (error) {
        expect(error).not.toBeError();
        region.get("foo", function(error, value) {
          expect(error).not.toBeError();
          expect(value).toEqual("bar");
          done();
        });
      });
    });
  });

  describe(".getSync", function() {
    it("throws an error if a key is not passed to .getSync", function() {
      function getWithoutKey() {
        region.getSync();
      }
      expect(getWithoutKey).toThrow(new Error("You must pass a key to getSync()."));
    });

    it("test for get for a nonexistent key", function() {
      
      var value = region.getSync("baz");
      expect(value).toBeNull();
    });

    it("executes synchronously", function(done) {
      async.series([
        function(next) { region.clear(next); },
        function(next) {
          region.put('foo', 'bar', function (error) {
            expect(error).not.toBeError();
            next();
          });
        },
        function(next) {
          var returnValue = region.getSync("foo");

          expect(returnValue).toEqual("bar");
          next();
        }
      ], done);
    });
  });

  describe(".put", function() {
    it("throws an error when no key is passed", function() {
      function putWithNoArgs() {
        region.put();
      }

      expect(putWithNoArgs).toThrow(new Error("You must pass a key and value to put()."));
    });

    it("throws an error when no value is passed", function() {
      function putWithOnlyKey() {
        region.put('foo');
      }

      expect(putWithOnlyKey).toThrow(new Error("You must pass a key and value to put()."));

    });

    it("throws an error if a non-function is passed as the callback", function() {
      function putWithNonFunctionCallback() {
        region.put('foo', 'bar', 'not a function');
      }

      expect(putWithNonFunctionCallback).toThrow(new Error("You must pass a function as the callback to put()."));
    });

    it("returns the region object to support chaining", function(done) {
      var returnValue = region.put("foo", "bar", function(error) {
        done();
      });

      expect(returnValue).toEqual(region);
    });

    it("succeeds when the value is put into the cache", function(done) {
      region.put("foo", "bar", function(error) {
        expect(error).not.toBeError();
        done();
      });
    });

    _.each(invalidValues, function(invalidValue) {
      it("passes an error to the callback when passed invalid value " + util.inspect(invalidValue), function(done) {
        region.put("foo", invalidValue, function(error) {
          expect(error).toBeError("InvalidValueError", "Invalid GemFire value.");
          done();
        });
      });
    });

    it("throws an error when passed a function as a value", function() {
      function callWithFunctionValue() {
        region.put("foo", function(){}, function(){});
      }

      expect(callWithFunctionValue).toThrow(new Error("Unable to serialize to GemFire; functions are not supported."));
    });

    _.each(invalidKeys, function(invalidKey) {
      it("passes an error to the callback when passed invalid key " + util.inspect(invalidKey), function(done) {
        region.put(invalidKey, "foo", function(error) {
          expect(error).toBeError("InvalidKeyError", "Invalid GemFire key.");
          done();
        });
      });
    });

    it("throws an error when passed a function as a key", function() {
      function callWithFunctionKey() {
        region.put(function(){}, "foo", function(){});
      }

      expect(callWithFunctionKey).toThrow(new Error("Unable to serialize to GemFire; functions are not supported."));
    });

    it("does not emit an event when an error occurs and there is a callback", function(done) {
      const errorHandler = jasmine.createSpy("errorHandler");

      region.on("error", errorHandler);

      region.put("foo", null, function(){
        expect(errorHandler).not.toHaveBeenCalled();
        done();
      });
    });

    it("emits an event when an error occurs and there is no callback", function(done) {
      const errorHandler = jasmine.createSpy("errorHandler").and.callFake(function(error){
        expect(error).toBeError();
        done();
      });

      region.on("error", errorHandler);
      region.put("foo", null);

      _.delay(function(){
        expect(errorHandler).toHaveBeenCalled();
        done();
      }, 1000);
    });

  });

  describe(".putSync", function() {
    it("throws an error when no key is passed", function() {
      function putWithNoArgs() {
        region.putSync();
      }

      expect(putWithNoArgs).toThrow(new Error("You must pass a key and value to putSync()."));
    });

    it("throws an error when no value is passed", function() {
      function putWithOnlyKey() {
        region.putSync('foo');
      }

      expect(putWithOnlyKey).toThrow(new Error("You must pass a key and value to putSync()."));

    });

    it("executes synchronously returning the region object to support chaining", function() {
      var returnValue = region.putSync("foo", "bar");
      expect(returnValue).toEqual(region);
    });

    it("throws an error if the key is invalid", function() {
      function putWithInvalidKey() {
        region.putSync(null, 'bar');
      }

      expect(putWithInvalidKey).toThrow(new Error("Invalid GemFire key."));
    });

    it("throws an error if the value is invalid", function() {
      function putWithInvalidValue() {
        region.putSync('foo', null);
      }

      expect(putWithInvalidValue).toThrow(new Error("Invalid GemFire value."));
    });

  });

  describe(".get/.put", function() {
    function testRoundTrip(value, done) {
      const key = "foo";

      async.series([
        function(next) { region.clear(next); },
        function(next) {
          region.put(key, value, function(error) {
            expect(error).not.toBeError();
            next();
          });
        },
        function(next) {
          region.get(key, function(error, getValue) {
            expect(error).not.toBeError();
            expect(getValue).toEqual(value);
            next();
          });
        }
      ], done);
    }

    it("stores and retrieves values in the correct region", function(done) {
      var region1 = cache.getRegion("exampleRegion");
      var region2 = cache.getRegion("anotherRegion");

      async.series([
        function(next) { region1.clear(next); },
        function(next) { region2.clear(next); },
        function(next) { region1.put("foo", "bar", next); },
        function(next) { region2.put("foo", 123, next); },
        function(next) {
          region1.get("foo", function(error, value) {
            expect(error).not.toBeError();
            expect(value).toEqual("bar");
            next();
          });
        },
        function(next) {
          region2.get("foo", function(error, value) {
            expect(error).not.toBeError();
            expect(value).toEqual(123);
            next();
          });
        },
      ], done);
    });

    _.each([
      "",
      "foo",
      "\u0123\u4567\u89AB\uCDEF\uabcd\uef4A",
    ], function(string) {
      it("stores and retrieves strings like " + util.inspect(string), function(done) {
        testRoundTrip(string, done);
      });
    });

    it("stores and retrieves large strings", function(done) {
      const largeString = randomString({length: 65536});
      testRoundTrip(largeString, done);
    });

    it("stores and retrieves large wide strings", function(done) {
      const largeString = randomString({length: 65533}) + "☃";
      testRoundTrip(largeString, done);
    });

    it("stores and retrieves string objects", function(done) {
      /* jshint -W053 */
      region.put("foo", new String("string object"), function(error) {
        expect(error).not.toBeError();
        region.get("foo", function(error, value) {
          expect(error).not.toBeError();
          expect(value).toEqual("string object");
          done();
        });
      });
    });

    it("stores and retrieves number objects", function(done) {
      /* jshint -W053 */
      region.put("foo", new Number(123), function(error) {
        expect(error).not.toBeError();
        region.get("foo", function(error, value) {
          expect(error).not.toBeError();
          expect(value).toEqual(123);
          done();
        });
      });
    });

    it("stores and retrieves true", function(done) {
      testRoundTrip(true, done);
    });

    it("stores and retrieves false", function(done) {
      testRoundTrip(false, done);
    });

    it("stores and retrieves boolean objects", function(done) {
      /* jshint -W053 */
      region.putAll({
        "true": new Boolean(true),
        "false": new Boolean(false)
      }, function(error) {
        expect(error).not.toBeError();

        region.getAll(["true", "false"], function(error, response) {
          expect(error).not.toBeError();

          expect(response).toEqual({
            "true": true,
            "false": false
          });

          done();
        });
      });
    });

    _.each([
      [],
      ['a string', 2, true, null, false, { an: 'object' }, new Date(), ['another array', true]],
      [[[[[[[[[[[[[[[[[[["Not too deep"]]]]]]]]]]]]]]]]]]]
    ], function(array) {
      it("stores and retrieves arrays like " + util.inspect(array), function(done) {
        testRoundTrip(array, done);
      });
    });

    it("stores and retrieves sparse arrays", function(done) {
      var sparseArray = [];
      sparseArray[10] = 'an element';
      testRoundTrip(sparseArray, done);
    });

    _.each([
      1,
      1.23,
      0.0,
      Number.MAX_VALUE,
      Number.MIN_VALUE,
      Number.NEGATIVE_INFINITY,
      Number.POSITIVE_INFINITY,
    ], function(number) {
      it("stores and retrieves numbers like " + util.inspect(number), function(done) {
        testRoundTrip(number, done);
      });
    });

    it("stores and retrieves NaN", function(done) {
      region.put("foo", NaN, function(error){
        expect(error).not.toBeError();
        region.get("foo", function(error, value) {
          expect(error).not.toBeError();
          expect(value).toBeNaN();
          done();
        });
      });
    });

    it("can use the empty string as a key", function(done) {
      region.put("", "value", function(error){
        expect(error).not.toBeError();
        region.get("", function(error, value) {
          expect(error).not.toBeError();
          expect(value).toEqual("value");
          done();
        });
      });
    });

    it("can use objects as a key", function(done) {
      const interestingObjects = [
        {},
        { foo: 'bar' },
        { qux: 'bar' },
        { foo: null },
        { foo: [] },
        { foo: ['bar'] },
        { foo: ['bar', 'baz'] },
        { foo: { bar: 'baz' } },
        { foo: { bar: 'qux' } },
        { foo: [{ bar: 'baz' }] },
        { foo: [{ bar: 'qux' }] },
        { foo: [{ bar: 'baz' }, { bar: 'qux' }] },
      ];

      const puts = _.map(interestingObjects, function(object, index){
        return function(next) { region.put(object, index, next); };
      });

      const expectations = _.map(interestingObjects, function(object, index){
        return function(next) {
          region.get(object, function(error, value) {
            expect(error).not.toBeError();
            expect(value).toEqual(index);
            next();
          });
        };
      });

      async.series(puts.concat(expectations), done);
    });

    it("treats the same object as the same key in the region", function(done) {
      async.series([
        function(next) { region.put({foo: 'bar'}, 'old value', next); },
        function(next) { region.put({foo: 'bar'}, 'new value', next); },
        function(next) {
          region.get({foo: 'bar'}, function(error, value) {
            expect(error).not.toBeError();
            expect(value).toEqual('new value');
            next();
          });
        },
      ], done);
    });

    it("stores and retrieves undefined", function(done) {
      testRoundTrip(undefined, done);
    });

    it("stores and retrieves Date objects", function(done) {
      const date = new Date();
      testRoundTrip(date, done);
    });

    describe("for objects", function() {
      it("stores and retrieves empty objects", function(done) {
        testRoundTrip({}, done);
      });

      it("stores and retrieves objects containing strings", function(done) {
        testRoundTrip({ baz: 'quuux' }, done);
      });

      it("stores and retrieves objects with wide string keys and values", function(done) {
        testRoundTrip({ '日': '本' }, done);
      });

      it("stores and retrieves nested objects", function(done) {
        testRoundTrip({ foo: { bar: "baz" } }, done);
      });

      it("stores and retrieves objects containing arrays", function(done) {
        testRoundTrip({ foo: [] }, done);
      });

      it("stores and retrieves objects containing booleans", function(done) {
        testRoundTrip({ foo: true }, done);
      });

      it("stores and retrieves objects containing integers", function(done) {
        testRoundTrip({ foo: 123 }, done);
      });

      it("stores and retrieves objects containing floats", function(done) {
        testRoundTrip({ foo: 123.456 }, done);
      });

      it("stores and retrieves objects containing dates", function(done) {
        const date = new Date();
        testRoundTrip({ foo: date }, done);
      });

      it("stores and retrieves objects containing null", function(done) {
        testRoundTrip({ foo: null }, done);
      });

      it("stores and retrieves objects containing undefined", function(done) {
        testRoundTrip({ foo: undefined }, done);
      });

      it("stores and retrieves a stress test object", function(done) {
        const object = require("./fixtures/stress_test.json");
        testRoundTrip(object, done);
      });

      it("stores and retrieves objects with numerical keys", function(done) {
        async.series([
          function(callback) { region.put(1, "number key", callback); },
          function(callback) { region.put("1", "string key", callback); },
          function(callback) {
            region.get(1, function(error, value){
              expect(error).not.toBeError();
              expect(value).toEqual("number key");
              callback();
            });
          },
          function(callback) {
            region.get("1", function(error, value){
              expect(error).not.toBeError();
              expect(value).toEqual("string key");
              callback();
            });
          },
        ], done);
      });

      it("stores and retrieves objects with date keys", function(done) {
        var date = new Date();
        region.put(date, "date key", function(error){
          expect(error).not.toBeError();
          region.get(date, function(error, value) {
            expect(error).not.toBeError();
            expect(value).toEqual("date key");
            done();
          });
        });
      });

      it("stores and retrieves objects with boolean keys", function(done) {
        async.series([
          function(callback) {
            region.put(true, "true key", function(error) {
              expect(error).not.toBeError();
              callback();
            });
          },
          function(callback) {
            region.put(false, "false key", function(error) {
              expect(error).not.toBeError();
              callback();
            });
          },
          function(callback) {
            region.put("true", "true string key", function(error) {
              expect(error).not.toBeError();
              callback();
            });
          },
          function(callback) {
            region.put("false", "false string key", function(error) {
              expect(error).not.toBeError();
              callback();
            });
          },
          function(callback) {
            region.get(true, function(error, value) {
              expect(error).not.toBeError();
              expect(value).toEqual("true key");
              callback();
            });
          },
          function(callback) {
            region.get(false, function(error, value) {
              expect(error).not.toBeError();
              expect(value).toEqual("false key");
              callback();
            });
          },
          function(callback) {
            region.get("true", function(error, value) {
              expect(error).not.toBeError();
              expect(value).toEqual("true string key");
              callback();
            });
          },
          function(callback) {
            region.get("false", function(error, value) {
              expect(error).not.toBeError();
              expect(value).toEqual("false string key");
              callback();
            });
          },
        ], done);
      });

      it("allows dynamic schema", function(done) {
        async.series([
          function(callback) { region.put('foo', {}, callback); },
          function(callback) {
            region.get('foo', function(error, value) {
              expect(error).not.toBeError();
              expect(value).toEqual({});
              callback();
            });
          },
          function(callback) { region.put('foo', { baz: 'qux' }, callback); },
          function(callback) {
            region.get('foo', function(error, value) {
              expect(error).not.toBeError();
              expect(value).toEqual({ baz: "qux" });
              callback();
            });
          },
          function(callback) { region.put('foo', { baz: [] }, callback); },
          function(callback) {
            region.get('foo', function(error, value) {
              expect(error).not.toBeError();
              expect(value).toEqual({ baz: [] });
              callback();
            });
          },
        ], done);
      });
    });
  });

  describe(".clear", function(){
    it("removes all keys, then calls the callback", function(done){
      async.series([
        function(next) { region.put('key', 'value', next); },
        function(next) { region.clear(next); },
        function(next) {
          region.get('key', function(error, value) {
            expect(value).toBeNull();
            next();
          });
        }
      ], done);
    });

    it("returns the region to support chaining", function(done) {
      expect(region.clear(done)).toEqual(region);
    });

    it("only removes keys for the region, not for other regions", function(done) {
      var region1 = cache.getRegion("exampleRegion");
      var region2 = cache.getRegion("anotherRegion");

      async.series([
        function(next) { region1.clear(next); },
        function(next) { region2.clear(next); },
        function(next) { region1.put('key', 'value', next); },
        function(next) { region2.put('key', 'value', next); },
        function(next) { region1.clear(next); },
        function(next) {
          region1.get("key", function(error, value) {
            expect(value).toBeNull();
            next();
          });
        },
        function(next) { region2.get("key", next); },
      ], done);
    });

    it("throws an error if the callback is not a function", function() {
      function callWithNonFunction() {
        region.clear("this is not a function");
      }

      expect(callWithNonFunction).toThrow(new Error("You must pass a function as the callback to clear()."));
    });

    // Pending us figuring out how to cause an error in clear()
    //xit("emits an event when an error occurs and there is no callback", function(){});
    it("throws an error when trying to clear a partitioned region", function(){
      var region1 = cache.getRegion("partitionRegion");
	  region1.clear(function(error) {
        expect(error).toBeError("apache::geode::client::CacheServerException", /UnsupportedOperationException/);
	  });
	});

    it("does not emit an event when no error occurs and there is no callback 1", function(done) {
      // if an error event is emitted, the test suite will crash here
      region.put("foo", "bar", function(error) {
        expect(error).not.toBeError();

        region.clear();

        until(
          function(test) { region.get("foo", test); },
          function(error, result) { return result === null },
          done
        );
      });
    });
  });

  describe(".executeFunction", function() {
    const expectFunctionsToThrowExceptionsCorrectly = true;
    itExecutesFunctions(
      function(){ return region; },
      expectFunctionsToThrowExceptionsCorrectly
    );

    it("gives the function access to the region", function(done) {
      const functionName = "io.pivotal.node_gemfire.SumRegion";
      const anotherRegion = cache.getRegion("anotherRegion");

      async.series([
        function(next) { region.put("one", 1, next); },
        function(next) { region.put("two", 2, next); },
        function(next) { anotherRegion.clear(next); },
        function(next) { anotherRegion.put("thousand", 1000, next); },
        function(next) {
          const dataCallback = jasmine.createSpy("dataCallback");

          region.executeFunction(functionName)
            .on("data", dataCallback)
            .on("end", function(){
              expect(dataCallback.calls.count()).toEqual(1);
              expect(dataCallback).toHaveBeenCalledWith(3);
              next();
            });
        },
        function(next) {
          const dataCallback = jasmine.createSpy("dataCallback");

          anotherRegion.executeFunction(functionName)
            .on("data", dataCallback)
            .on("end", function(){
              expect(dataCallback.calls.count()).toEqual(1);
              expect(dataCallback).toHaveBeenCalledWith(1000);
              next();
            });
        }
      ], done);
    });

    it("supports an Array of keys as a filter", function(done) {
      const dataCallback = jasmine.createSpy("dataCallback");
      region.executeFunction(
        "io.pivotal.node_gemfire.ReturnFilter", {
          arguments: { foo: 'bar' },
          filter: ["key1", 2, 3.1]
        }
      ) .on("data", dataCallback)
        .on("end", function(){
          expect(dataCallback.calls.count()).toEqual(3);
          expect(dataCallback).toHaveBeenCalledWith("key1");
          expect(dataCallback).toHaveBeenCalledWith(2);
          expect(dataCallback).toHaveBeenCalledWith(3.1);
          done();
        });
    });

    it("does not pass a filter if none is provided", function(done) {
      region.executeFunction("io.pivotal.node_gemfire.ReturnFilter", { arguments: { foo: 'bar' } })
        .on("error", function(error) {
          //TODO: Check and see what the error type is and the message.
          expect(error).toBeError();
          done();
        });
    });

    it("throws an error if the filters aren't an array", function() {
      function callWithBadFilter() {
        region.executeFunction(
          "io.pivotal.node_gemfire.ReturnFilter",
          { filter: "this string is not an array" }
        );
      }

      expect(callWithBadFilter).toThrow(
        new Error("You must pass an Array of keys as the filter for executeFunction().")
      );
    });
  });

  describe(".inspect", function() {
    it("returns a user-friendly display string describing the region", function() {
      expect(region.inspect()).toEqual('[Region name="exampleRegion"]');
    });
  });

  describe(".name", function() {
    it("returns the name of the region", function() {
      expect(region.name).toEqual("exampleRegion");
    });
  });

  describe(".attributes", function() {
    describe(".cachingEnabled", function() {
      describe("for a caching proxy region", function() {
        it("is set to true", function() {
          expect(region.attributes.cachingEnabled).toEqual(true);
        });
      });

      describe("for a caching proxy region", function() {
        it("is set to false", function() {
          const proxyRegion = cache.getRegion("exampleProxyRegion");
          expect(proxyRegion.attributes.cachingEnabled).toEqual(false);
        });
      });
    });

    it("has a clientNotificationEnabled property", function() {
      expect(region.attributes.clientNotificationEnabled).toEqual(true);
    });

    it("has a concurrencyChecksEnabled property", function() {
      expect(region.attributes.concurrencyChecksEnabled).toEqual(true);
    });

    it("has a concurrencyLevel property", function() {
      expect(region.attributes.concurrencyLevel).toEqual(16);
    });

    it("has a diskPolicy property", function() {
      expect(region.attributes.diskPolicy).toEqual("none");
    });

    it("has an entryIdleTimeout property", function() {
      expect(region.attributes.entryIdleTimeout).toEqual(0);
    });

    it("has an entryTimeToLive property", function() {
      expect(region.attributes.entryTimeToLive).toEqual(0);
    });

    it("has a initialCapacity property", function() {
      expect(region.attributes.initialCapacity).toEqual(10000);
    });

    it("has a loadFactor property", function() {
      expect(region.attributes.loadFactor).toEqual(0.75);
    });

    it("has a lruEntriesLimit property", function() {
      expect(region.attributes.lruEntriesLimit).toEqual(0);
    });

    it("has a lruEvicationAction property", function() {
      expect(region.attributes.lruEvicationAction).toEqual("LOCAL_DESTROY");
    });

    it("has a poolName property", function() {
      expect(region.attributes.poolName).toEqual("myPool");
    });

    it("has a regionIdleTimeout property", function() {
      expect(region.attributes.regionIdleTimeout).toEqual(0);
    });

    it("has a regionTimeToLive property", function() {
      expect(region.attributes.regionTimeToLive).toEqual(0);
    });

    it("has a scope property", function() {
      //TODO there is no scope in GemFire 9
      //expect(region.attributes.scope).toEqual("DISTRIBUTED_NO_ACK");
    });
  });

  describe(".remove", function() {
    it("throws an error if no key is given", function() {
      function callNoArgs() {
        region.remove();
      }

      expect(callNoArgs).toThrow(new Error("You must pass a key to remove()."));
    });

    it("throws an error if a non-function is passed as the callback", function() {
      function callNoCallback() {
        region.remove('foo', 'not a function');
      }

      expect(callNoCallback).toThrow(new Error("You must pass a function as the callback to remove()."));
    });

    it("removes the entry at the given key", function(done) {
      region.put("foo", "bar", function(error){
        expect(error).not.toBeError();
        region.remove("foo", function(error) {
          expect(error).not.toBeError();
          region.get("foo", function(error, value) {
            expect(value).toBe(null);
            done();
          });
        });
      });
    });

    it("passes no error when it succeeds", function(done) {
      region.put("foo", "bar", function(error){
        expect(error).not.toBeError();
        region.remove("foo", function(error, result) {
          expect(error).not.toBeError();
          done();
        });
      });
    });

    it("returns itself for chaining", function(done) {
      region.put("foo", "bar", function(error){
        expect(error).not.toBeError();
        expect(region.remove("foo", done)).toEqual(region);
      });
    });

    it("passes an error to the callback if the entry is not present", function(done) {
      region.remove("foo", function(error, result) {
        expect(error).toBeError("KeyNotFoundError", "Key not found in region.");
        expect(result).toBeUndefined();
        done();
      });
    });

    _.each(invalidKeys, function(invalidKey) {
      it("passes an error to the callback when passed invalid key " + util.inspect(invalidKey), function(done) {
        region.remove(invalidKey, function(error, value) {
          expect(error).toBeError("InvalidKeyError", "Invalid GemFire key.");
          expect(value).toBeUndefined();
          done();
        });
      });
    });

    it("throws an error when passed a function as a key", function() {
      function callWithFunctionKey() {
        region.remove(function(){}, function(){});
      }

      expect(callWithFunctionKey).toThrow(new Error("Unable to serialize to GemFire; functions are not supported."));
    });

    it("emits an event when an error occurs and there is no callback", function(done) {
      const errorHandler = jasmine.createSpy("errorHandler").and.callFake(function(error){
        expect(error).toBeError();
        done();
      });

      region.on("error", errorHandler);
      region.remove("foo");

      _.delay(function(){
        expect(errorHandler).toHaveBeenCalled();
        done();
      }, 1000);
    });

    it("does not emit an event when an error occurs and there is a callback", function(done) {
      const errorHandler = jasmine.createSpy("errorHandler");

      region.on("error", errorHandler);

      region.remove("foo", function(){
        expect(errorHandler).not.toHaveBeenCalled();
        done();
      });
    });

    it("does not emit an event when no error occurs and there is no callback 2", function(done) {
      region.put("foo", "bar", function(error) {
        region.remove("foo");

        until(
          function(test) { region.get("foo", test); },
          function(error, result) { return result === null },
          done
        );
      });
    });
  });

  describe(".query", function() {
    it("passes the results into the callback for the passed-in predicate", function(done) {
      async.series([
        function (callback) { region.put("key1", 1, callback); },
        function (callback) { region.put("key2", 2, callback); },
        function (callback) { region.put("key3", 3, callback); },
        function (callback) {
          region.query("this > 1", function(error, response) {
            expect(error).not.toBeError();
            const results = response.toArray();

            expect(results.length).toEqual(2);
            expect(results).toContain(2);
            expect(results).toContain(3);

            callback();
          });
        },
      ], done);
    });

    it("requires a query predicate string", function() {
      function queryWithoutPredicate(){
        region.query();
      }

      expect(queryWithoutPredicate).toThrow(new Error("You must pass a query predicate string and a callback to query()."));
    });

    it("requires a callback", function() {
      function queryWithoutCallback(){
        region.query("true");
      }

      expect(queryWithoutCallback).toThrow(new Error("You must pass a query predicate string and a callback to query()."));
    });

    it("requires the callback to be a function", function() {
      function queryWithNonCallback(){
        region.query("true", "not a function");
      }

      expect(queryWithNonCallback).toThrow(new Error("You must pass a function as the callback to query()."));
    });

    it("passes along errors from an invalid query 1", function(done) {
      region.query("Invalid query", function(error, response) {
        expect(error).toBeError("apache::geode::client::QueryException", /Syntax error in query/);
        done();
      });
    });

    it("returns the region for chaining", function() {
      expect(region.query("true", function(){})).toEqual(region);
    });
  });

  describe(".selectValue", function() {
    it("passes the result into the callback for the passed-in predicate", function(done) {
      async.series([
        function (callback) { region.put("key1", { foo: 1 }, callback); },
        function (callback) { region.put("key2", { foo: 2 }, callback); },
        function (callback) { region.put("key3", { foo: 3 }, callback); },
        function (callback) {
          region.selectValue("foo = 2", function(error, result) {
            expect(error).not.toBeError();
            expect(result).toEqual({ foo: 2 });
            callback();
          });
        },
      ], done);
    });

    it("passes an error to the callback when there is more than one result", function(done) {
      async.series([
        function (next) {
          region.putAllSync({
            key1: { foo: 'bar', baz: 'qux' },
            key2: { foo: 'bar' }
          });
          next();
        },
        function (next) {
          region.selectValue("foo = 'bar'", function(error, result) {
            expect(error).toBeError("apache::geode::client::QueryException", "selectValue has more than one result");
            next();
          });
         
        }
      ], done);
    });

    it("requires a query predicate string", function() {
      function queryWithoutPredicate(){
        region.selectValue();
      }

      expect(queryWithoutPredicate).toThrow(new Error("You must pass a query predicate string and a callback to selectValue()."));
    });

    it("requires a callback", function() {
      function queryWithoutCallback(){
        region.selectValue("true");
      }

      expect(queryWithoutCallback).toThrow(new Error("You must pass a query predicate string and a callback to selectValue()."));
    });

    it("requires the callback to be a function", function() {
      function queryWithNonCallback(){
        region.selectValue("true", "not a function");
      }

      expect(queryWithNonCallback).toThrow(new Error("You must pass a function as the callback to selectValue()."));
    });

    it("passes along errors from an invalid query 2", function(done) {
      region.selectValue("Invalid query", function(error, response) {
        expect(error).toBeError("apache::geode::client::QueryException", /Syntax error in query/);
        done();
      });
    });

    it("returns the region for chaining", function() {
      expect(region.selectValue("true", function(){})).toEqual(region);
    });
  });

  describe(".existsValue", function() {
    it("passes the result into the callback for the passed-in predicate", function(done) {
      async.series([
        function (callback) { region.put("key1", { foo: 1 }, callback); },
        function (callback) { region.put("key2", { foo: 2 }, callback); },
        function (callback) { region.put("key3", { foo: 3 }, callback); },
        function (callback) {
          region.existsValue("foo = 2", function(error, result) {
            expect(error).not.toBeError();
            expect(result).toEqual(true);
            callback();
          });
        },
        function (callback) {
          region.existsValue("foo = 4", function(error, result) {
            expect(error).not.toBeError();
            expect(result).toEqual(false);
            callback();
          });
        },
      ], done);
    });

    it("requires a query predicate string", function() {
      function queryWithoutPredicate(){
        region.existsValue();
      }

      expect(queryWithoutPredicate).toThrow(new Error("You must pass a query predicate string and a callback to existsValue()."));
    });

    it("requires a callback", function() {
      function queryWithoutCallback(){
        region.existsValue("true");
      }

      expect(queryWithoutCallback).toThrow(new Error("You must pass a query predicate string and a callback to existsValue()."));
    });

    it("requires the callback to be a function", function() {
      function queryWithNonCallback(){
        region.existsValue("true", "not a function");
      }

      expect(queryWithNonCallback).toThrow(new Error("You must pass a function as the callback to existsValue()."));
    });

    it("passes along errors from an invalid query 3", function(done) {
      region.existsValue("Invalid query", function(error, response) {
        expect(error).toBeError('apache::geode::client::QueryException', /Syntax error in query/);
        done();
      });
    });

    it("returns the region for chaining", function() {
      expect(region.existsValue("true", function(){})).toEqual(region);
    });
  });

  describe(".putAll", function() {
    it("sets multiple values at once async", function(done) {
      async.series([
        function(next) {
          region.putAll({ key1: 'foo', key2: 'bar', "1": "one"}, next);
        },
        function(next) {
          region.get('key1', function(error, value) {
            expect(error).not.toBeError();
            expect(value).toEqual('foo');

            next();
          });
        },
        function(next) {
          region.get('key2', function(error, value) {
            expect(error).not.toBeError();
            expect(value).toEqual('bar');

            next();
          });
        },
        function(next) {
          region.get("1", function(error, value) {
            expect(error).not.toBeError();
            expect(value).toEqual('one');

            next();
          });
        },
      ], done);
    });

    _.each(invalidValues, function(invalidValue) {
      it("passes an error to the callback when passed invalid value " + util.inspect(invalidValue), function(done) {
        region.putAll({"foo": "bar", "baz": invalidValue}, function(error) {
          expect(error).toBeError("InvalidValueError", "Invalid GemFire value.");
          done();
        });
      });
    });

    it("throws an error when passed a function as a value", function() {
      function callWithFunctionValue() {
        region.putAll({"foo": "bar", "baz": function(){}}, function(){});
      }

      expect(callWithFunctionValue).toThrow(new Error("Unable to serialize to GemFire; functions are not supported."));
    });

    it("returns the region for chaining", function() {
      expect(region.putAll({ key: 'value' }, function(){})).toEqual(region);
    });

    it("requires a map argument", function() {
      function callWithNoArgs() {
        region.putAll();
      }

      expect(callWithNoArgs).toThrow(new Error("You must pass an object and a callback to putAll()."));
    });

    it("requires the callback to be a function", function() {
      function callWithNonFunction() {
        region.putAll({ foo: 'foo' }, 'this thing is not a function');
      }

      expect(callWithNonFunction).toThrow(new Error("You must pass a function as the callback to putAll()."));
    });

    it("requires the object argument to be an object", function() {
      function callWithNull() {
        region.putAll(null, function (){});
      }

      function callWithString() {
        region.putAll('foobar', function (){});
      }

      errorMessage = "You must pass an object and a callback to putAll().";
      expect(callWithNull).toThrow(new Error(errorMessage));
      expect(callWithString).toThrow(new Error(errorMessage));
    });

    it("emits an event when an error occurs and there is no callback", function(done) {
      const errorHandler = jasmine.createSpy("errorHandler").and.callFake(function(error){
        expect(error).toBeError();
        done();
      });

      region.on("error", errorHandler);
      region.putAll({ foo: null, bar: "baz" });

      _.delay(function(){
        expect(errorHandler).toHaveBeenCalled();
        done();
      }, 1000);
    });

    it("does not emit an event when an error occurs and there is a callback", function(done) {
      const errorHandler = jasmine.createSpy("errorHandler");

      region.on("error", errorHandler);

      region.putAll({ foo: null, bar: "baz" }, function(){
        expect(errorHandler).not.toHaveBeenCalled();
        done();
      });
    });

    it("does not emit an event when no error occurs and there is no callback 3", function(done) {
      // if it fails, an uncaught event will blow up the suite
      region.putAll({ foo: "bar" });

      until(
        function(test) { region.get("foo", test); },
        function(error, value) { return value === "bar"; },
        done
      );
    });
  });

  describe(".putAllSync", function() {
    it("sets multiple values at once sync", function(done) {
      async.series([
        function(next) {
          region.putSync('key1', 'foo');
          region.putAllSync({ key1: 'foo', key2: 'bar', "1": "one"});
          next();
        },
        function(next) {
          region.get('key1', function(error, value) {
            expect(error).not.toBeError();
            expect(value).toEqual('foo');

            next();
          });
        },
        function(next) {
          region.get('key2', function(error, value) {
            expect(error).not.toBeError();
            expect(value).toEqual('bar');

            next();
          });
        },
        function(next) {
          region.get("1", function(error, value) {
            expect(error).not.toBeError();
            expect(value).toEqual('one');

            next();
          });
        },
      ], done);
    });

    _.each(invalidValues, function(invalidValue) {
      it("returns an error when passed invalid value " + util.inspect(invalidValue), function() {
        function invalidKey() {
          region.putAllSync({"foo": "bar", "baz": invalidValue});
        }
        expect(invalidKey).toThrow(new Error("Invalid GemFire value."));
      });
    });

    it("throws an error when passed a function as a value", function() {
      function callWithFunctionValue() {
        region.putAllSync({"foo": "bar", "baz": function(){}}, function(){});
      }

      expect(callWithFunctionValue).toThrow(new Error("You must pass an object to putAllSync()."));
    });

    it("returns the region for chaining", function() {
      expect(region.putAllSync({ key: 'value' })).toEqual(region);
    });

    it("requires a map argument", function() {
      function callWithNoArgs() {
        region.putAllSync();
      }

      expect(callWithNoArgs).toThrow(new Error("You must pass an object to putAllSync()."));
    });

    it("requires the object argument to be an object", function() {
      function callWithNull() {
        region.putAllSync(null);
      }

      function callWithString() {
        region.putAllSync('foobar');
      }

      errorMessage = "You must pass an object to putAllSync().";
      expect(callWithNull).toThrow(new Error(errorMessage));
      expect(callWithString).toThrow(new Error(errorMessage));
    });
  });

  describe(".getAll", function() {
    it("passes the results as an array to the callback", function(done) {
      async.series([
        function(next) { region.put('key1', 'value1', next); },
        function(next) { region.put('key2', 'value2', next); },
        function(next) { region.put('key3', 'value3', next); },
        function(next) {
          region.getAll(['key1', 'key3'], function(error, response){
            expect(error).not.toBeError();

            expect(response.key1).toEqual('value1');
            expect(response.key3).toEqual('value3');
            expect(response.key2).toBeUndefined();

            next();
          });
        }
      ], done);
    });

    it("returns the region for chaining", function() {
      expect(region.getAll(['key'], function(){})).toEqual(region);
    });

    it("requires a keys argument", function() {
      function callWithNoArgs() {
        region.getAll();
      }

      expect(callWithNoArgs).toThrow(new Error("You must pass an array of keys and a callback to getAll()."));
    });

    it("requires a callback", function() {
      function callWithNoCallback() {
        region.getAll(['foo']);
      }

      expect(callWithNoCallback).toThrow(new Error("You must pass a callback to getAll()."));
    });

    it("requires the callback to be a function", function() {
      function callWithNonFunction() {
        region.getAll(['foo'], 'not a function');
      }

      expect(callWithNonFunction).toThrow(new Error("You must pass a function as the callback to getAll()."));
    });

    it("requires the keys argument to be an array", function() {
      function callWithNonArray() {
        region.getAll('not an array', function (){});
      }

      expect(callWithNonArray).toThrow(new Error("You must pass an array of keys and a callback to getAll()."));
    });

    it("passes an empty object to the callback when provided with no keys", function(done){
      region.getAll([], function(error, response) {
        expect(error).not.toBeError();
        expect(response).toEqual({});

        done();
      });
    });

    _.each(invalidKeys, function(invalidKey) {
      it("passes an error to the callback when passed the invalid key " + util.inspect(invalidKey), function(done) {
        region.getAll(["foo", invalidKey], function(error, value) {
          expect(error).toBeError("InvalidKeyError", "Invalid GemFire key.");
          expect(value).toBeUndefined();
          done();
        });
      });
    });

    it("throws an error when passed a function as a key", function() {
      function callWithFunctionKey() {
        region.getAll([function(){}], function(){});
      }

      expect(callWithFunctionKey).toThrow(new Error("Unable to serialize to GemFire; functions are not supported."));
    });
  });

  describe(".getAllSync", function() {
    it("returns the results as an array", function(done) {
      async.series([
        function(next) { region.put('key1', 'value1', next); },
        function(next) { region.put('key2', 'value2', next); },
        function(next) { region.put('key3', 'value3', next); },
        function(next) {
          var results = region.getAllSync(['key1', 'key3']);
          expect(results.key1).toEqual('value1');
          expect(results.key3).toEqual('value3');
          expect(results.key2).toBeUndefined();
          next();
        }
      ], done);
    });

    it("requires a keys argument", function() {
      function callWithNoArgs() {
        region.getAllSync();
      }

      expect(callWithNoArgs).toThrow(new Error("You must pass an array of keys to getAllSync()."));
    });

    it("requires the keys argument to be an array", function() {
      function callWithNonArray() {
        region.getAllSync('not an array');
      }

      expect(callWithNonArray).toThrow(new Error("You must pass an array of keys to getAllSync()."));
    });

    it("returns an empty object when provided with no keys", function(){
      var results = region.getAllSync([]);
      expect(results).toEqual({});
    });

    _.each(invalidKeys, function(invalidKey) {
      it("returns an error when passed the invalid key " + util.inspect(invalidKey), function() {
        function callWithInvalidKeys() {
          var results = region.getAllSync(["foo", invalidKey]);
        }

        expect(callWithInvalidKeys).toThrow(new Error("Invalid GemFire key."));
      });
    });

    it("throws an error when passed a function as a key", function() {
      function callWithFunctionKey() {
        region.getAllSync([function(){}]);
      }

      expect(callWithFunctionKey).toThrow(new Error("Invalid GemFire key."));
    });
  });

  describe(".serverKeys", function() {
    it("passes an array of key names in the region on the server to the callback", function(done) {
      async.series([
        function(next) { region.putAll({"foo": 1, "bar": 2, "baz": 3}, next); },
        function(next) {
          region.serverKeys(function(error, serverKeys) {
            expect(error).not.toBeError();
            expect(serverKeys.length).toEqual(3);
            expect(serverKeys).toContain("foo");
            expect(serverKeys).toContain("bar");
            expect(serverKeys).toContain("baz");
            next();
          });
        },
      ], done);
    });

    it("throws an exception when no callback is passed", function() {
      function callWithNoArgs() {
        region.serverKeys();
      }

      expect(callWithNoArgs).toThrow(new Error("You must pass a callback to serverKeys()."));
    });

    it("throws an exception when a non-function is passed as the callback", function() {
      function callWithNonFunction() {
        region.serverKeys("this is some crazy string");
      }

      expect(callWithNonFunction).toThrow(new Error("You must pass a function as the callback to serverKeys()."));
    });
  });

  describe(".keys", function() {
    it("passes an array of key names in the region to the callback", function(done) {
      async.series([
        function(next) { region.putAll({"foo": 4, "bar": 5, "baz": 6}, next); },
        function(next) {
          region.keys(function(error, keys) {
            expect(error).not.toBeError();
            expect(keys.length).toEqual(3);
            expect(keys).toContain("foo");
            expect(keys).toContain("bar");
            expect(keys).toContain("baz");
            next();
          });
        },
      ], done);
    });

    it("throws an exception when no callback is passed", function() {
      function callWithNoArgs() {
        region.keys();
      }

      expect(callWithNoArgs).toThrow(new Error("You must pass a callback to keys()."));
    });

    it("throws an exception when a non-function is passed as the callback", function() {
      function callWithNonFunction() {
        region.keys("this is some crazy string");
      }

      expect(callWithNonFunction).toThrow(new Error("You must pass a function as the callback to keys()."));
    });
  });

  describe(".values", function() {
    it("passes an array of values in the region to the callback", function(done) {
      async.series([
        function(next) { region.putAll({"foo": 7, "bar": "quz", "baz": 9}, next); },
        function(next) {
          region.values(function(error, values) {
            expect(error).not.toBeError();
            expect(values.length).toEqual(3);
            expect(values).toContain(7);
            expect(values).toContain("quz");
            expect(values).toContain(9);
            next();
          });
        },
      ], done);
    });

    it("throws an exception when no callback is passed", function() {
      function callWithNoArgs() {
        region.values();
      }

      expect(callWithNoArgs).toThrow(new Error("You must pass a callback to values()."));
    });

    it("throws an exception when a non-function is passed as the callback", function() {
      function callWithNonFunction() {
        region.values("this is some crazy string");
      }

      expect(callWithNonFunction).toThrow(new Error("You must pass a function as the callback to values()."));
    });
  });

  describe(".entries", function() {
    it("passes an array of key-value pairs in the region to the callback", function(done) {
      async.series([
        function(next) { region.putAll({"foo": 10, "bar": "quz", "baz": 12}, next); },
        function(next) {
          region.entries(function(error, pairs) {
            expect(error).not.toBeError();
            expect(pairs.length).toEqual(3);
            expect(pairs).toContain({ "key" : "foo", "value" : 10});
            expect(pairs).toContain({ "key" : "bar", "value" : "quz"});
            expect(pairs).toContain({ "key" : "baz", "value" : 12});
            next();
          });
        },
      ], done);
    });

    it("throws an exception when no callback is passed", function() {
      function callWithNoArgs() {
        region.entries();
      }

      expect(callWithNoArgs).toThrow(new Error("You must pass a callback to entries()."));
    });

    it("throws an exception when a non-function is passed as the callback", function() {
      function callWithNonFunction() {
        region.entries("this is some crazy string");
      }

      expect(callWithNonFunction).toThrow(new Error("You must pass a function as the callback to entries()."));
    });
  });

  describe("events", function() {
    describe("create", function() {
      beforeEach(function() {
        region = cache.getRegion("createEventTest");
      });

      it("is emitted when an entry is created on a region from getRegion", function(done) {
        region.on("create", function(event) {
          expect(event).toEqual(jasmine.objectContaining({
            key: "foo",
            oldValue: null,
            newValue: "bar"
          }));
          done();
        });

        region.put("foo", "bar");
      });

      it("is emitted when an entry is created on a region from createRegion", function(done) {
        const region = cache.createRegion("createRegionCreateEventTest",
                                          {type: "CACHING_PROXY", poolName: "myPool"});

        async.series([
          function(next) { region.clear(next); },
          function(next) {
            region.on("create", function(event) {
              expect(event).toEqual(jasmine.objectContaining({
                key: "foo",
                oldValue: null,
                newValue: "bar"
              }));
              done();
            });

            next();
          },
          function(next) { region.put("foo", "bar", next); }
        ]);
      });

      it("emits events on each JS object for the GemFire region", function(done) {
        const anotherRegion = cache.getRegion("anotherRegion");
        const region1 = cache.getRegion("exampleRegion");
        const region2 = cache.getRegion("exampleRegion");

        var callback1Called = false;
        region1.on('create', function() {
          callback1Called = true;
        });

        var callback2Called = false;
        region2.on('create', function() {
          callback2Called = true;
        });

        var anotherCallbackCalled = false;
        anotherRegion.on('create', function() {
          anotherCallbackCalled = true;
        });

        async.series([
          function(next) { region1.put("foo", "bar", next); },
          function(next) {
            waitUntil(function(){
              return callback1Called && callback2Called;
            }, next);
          },
          function(next) {
            expect(anotherCallbackCalled).toBeFalsy();
            next();
          },
        ], done);
      });
    });

    describe("update", function() {
      beforeEach(function() {
        region = cache.getRegion("updateEventTest");
      });

      it("is emitted when an entry is updated on a region from getRegion", function(done) {
        region.on("update", function(event) {
          expect(event).toEqual(jasmine.objectContaining({
            key: "foo",
            oldValue: "bar",
            newValue: "baz"
          }));
          done();
        });

        async.series([
          function(next) { region.put("foo", "bar", next); },
          function(next) { region.put("foo", "baz", next); }
        ]);
      });

      it("is emitted when an entry is updated on a region from createRegion", function(done) {
        const region = cache.createRegion("createRegionUpdateEventTest",
                                          {type: "CACHING_PROXY", poolName: "myPool"});

        async.series([
          function(next) { region.clear(next); },
          function(next) {
            region.on("update", function(event) {
              expect(event).toEqual(jasmine.objectContaining({
                key: "foo",
                oldValue: "bar",
                newValue: "baz"
              }));
              done();
            });

            next();
          },
          function(next) { region.put("foo", "bar", next); },
          function(next) { region.put("foo", "baz", next); }
        ]);
      });

      it("emits events on each JS object for the GemFire region", function(done) {
        const anotherRegion = cache.getRegion("anotherRegion");
        const region1 = cache.getRegion("exampleRegion");
        const region2 = cache.getRegion("exampleRegion");

        var callback1Called = false;
        region1.on('update', function() {
          callback1Called = true;
        });

        var callback2Called = false;
        region2.on('update', function() {
          callback2Called = true;
        });

        var anotherCallbackCalled = false;
        anotherRegion.on('update', function() {
          anotherCallbackCalled = true;
        });

        async.series([
          function(next) { region1.put("foo", "bar", next); },
          function(next) { region1.put("foo", "baz", next); },
          function(next) {
            waitUntil(function(){
              return callback1Called && callback2Called;
            }, next);
          },
          function(next) {
            expect(anotherCallbackCalled).toBeFalsy();
            next();
          },
        ], done);
      });
    });

    describe("destroy", function() {
      beforeEach(function() {
        region = cache.getRegion("destroyEventTest");
      });

      it("is emitted when an entry is destroyed on a region from getRegion", function(done) {
        region.on("destroy", function(event) {
          expect(event).toEqual(jasmine.objectContaining({
            key: "foo",
            oldValue: "bar",
            newValue: null
          }));
          done();
        });

        async.series([
          function(next) { region.put("foo", "bar", next); },
          function(next) { region.remove("foo"); }
        ]);
      });

      it("is emitted when an entry is destroyed on a region from createRegion", function(done) {
        const region = cache.createRegion("createRegionDestroyEventTest",
                                          {type: "CACHING_PROXY", poolName: "myPool"});

        async.series([
          function(next) { region.clear(next); },
          function(next) {
            region.on("destroy", function(event) {
              expect(event).toEqual(jasmine.objectContaining({
                key: "foo",
                oldValue: "bar",
                newValue: null
              }));
              done();
            });

            next();
          },
          function(next) { region.put("foo", "bar", next); },
          function(next) { region.remove("foo"); }
        ]);
      });

      it("emits events on each JS object for the GemFire region", function(done) {
        const anotherRegion = cache.getRegion("anotherRegion");
        const region1 = cache.getRegion("exampleRegion");
        const region2 = cache.getRegion("exampleRegion");

        var callback1Called = false;
        region1.on('destroy', function() {
          callback1Called = true;
        });

        var callback2Called = false;
        region2.on('destroy', function() {
          callback2Called = true;
        });

        var anotherCallbackCalled = false;
        anotherRegion.on('destroy', function() {
          anotherCallbackCalled = true;
        });

        async.series([
          function(next) { region1.put("foo", "bar", next); },
          function(next) { region1.remove("foo", next); },
          function(next) {
            waitUntil(function(){
              return callback1Called && callback2Called;
            }, next);
          },
          function(next) {
            expect(anotherCallbackCalled).toBeFalsy();
            next();
          },
        ], done);
      });
    });
  });

  describe(".registerAllKeys", function() {
    it("registers interest in all keys in the region for external events", function(done) {
      const region = cache.getRegion("registerInterestTest");

      function externalPut(key, value, next) {
        region.executeFunction("io.pivotal.node_gemfire.Put", [key, value])
          .on("error", function(error) { throw(error); })
          .on("end", next);
      }

      const createCallback = jasmine.createSpy();
      region.on("create", createCallback);

      async.series([
        function(next) {
          region.registerAllKeys();
          next();
        },
        function(next) { region.clear(next); },
        function(next) { externalPut("foo", "bar", next); },
        function(next) {
          waitUntil(function(){
            return createCallback.calls.count() == 1;
          }, next);
        },
        function(next) {
          expect(createCallback).toHaveBeenCalledWith(jasmine.objectContaining({
            key: "foo",
            oldValue: null,
            newValue: "bar"
          }));
          createCallback.calls.reset();
          next();
        },
        function(next) {
          region.unregisterAllKeys();
          next();
        },
        function(next) { region.clear(next); },
        function(next) {
          region.on("create", function(){
            throw(
              "create callback should not be called for external puts when interest is not registered"
            );
          });
          next();
        },
        function(next) { externalPut("foo", "bar", next); },
        function(next) { process.nextTick(next); }
      ], done);
    });
  });

  describe(".destroyRegion", function() {
    itDestroysTheRegion('destroyRegion');

    // NOTE: The C++ Native Client documentation contradicts this finding.
    //   http://gemfire.docs.pivotal.io/latest/cpp_api/cppdocs/classgemfire_1_1Region.html#2b07a940ee17e375c45421e85b27a8ff
    it("fails to destroy a CACHING_PROXY region that is not backed by a server region", function(done) {
      const regionName = "destroyCachingProxyRegionWithoutServerRegion";
      const region = cache.createRegion(regionName, {type: "CACHING_PROXY", poolName: "myPool"});

      expect(cache.getRegion(regionName)).toBeDefined();

      region.destroyRegion(function(error) {
        expect(error).toBeError();
        expect(cache.getRegion(regionName)).toBeDefined();
        done();
      });
    });
  });

  describe(".localDestroyRegion", function() {
    itDestroysTheRegion('localDestroyRegion');

    it("destroys a CACHING_PROXY region that is not backed by a server region", function(done) {
      const regionName = "localDestroyCachingProxyRegionWithoutServerRegion";
      const region = cache.createRegion(regionName, {type: "CACHING_PROXY", poolName: "myPool"});

      expect(cache.getRegion(regionName)).toBeDefined();

      region.localDestroyRegion(function(error) {
        expect(error).not.toBeError();
        expect(cache.getRegion(regionName)).toBeUndefined();
        done();
      });
    });
  });
});
