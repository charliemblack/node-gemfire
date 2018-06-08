const async = require("async");
const _ = require("lodash");

const gemfire = require("./support/gemfire.js");
const factories = require("./support/factories.js");
const errorMatchers = require("./support/error_matchers.js");
const itExecutesFunctions = require("./support/it_executes_functions.js");

const expectExternalSuccess = require("./support/external_scripts.js").expectExternalSuccess;
const expectExternalFailure = require("./support/external_scripts.js").expectExternalFailure;

describe("gemfire.Cache", function() {
  beforeEach(function() {
    jasmine.addMatchers(errorMatchers);
  });

  afterEach(function(done) {
    setTimeout(done, 0);
  });

  describe(".configure", function() {
    it("throws an error if .configure has been called already", function() {
      function callConfigureAgain() {
        gemfire.configure("./config/ExampleClient.xml");
      }

      expect(callConfigureAgain).toThrow(
        "gemfire: configure() can only be called once per process. " +
        "Please call configure() once in an application initializer. " +
        "Afterwards, you can call getCache() multiple times to get the cache singleton object."
      );
    });

    it("accepts a correct XML file path", function(done) {
      expectExternalSuccess("correct_xml_file", done);
    });

    it("throws an error if the XML file is not found", function(done) {
      var expectedMessage = "failed to load external entity";
      expectExternalFailure("missing_xml_file", done, expectedMessage);
    });

    it("throws an error if setReadSerialized not set to true in the XML", function(done) {
      var expectedMessage = "must be set in your cache xml";
      expectExternalFailure("not_pdx_read_serialized", done, expectedMessage);
    });

  });

  describe(".configure/.getCache", function(){
    it("returns the Cache singleton", function() {
      const cache = gemfire.getCache();
      expect(cache.constructor.name).toEqual("Cache");
    });

    it("returns the same object on subsequent calls", function() {
      const cache1 = gemfire.getCache();
      const cache2 = gemfire.getCache();
      expect(cache1 === cache2).toBeTruthy();
    });

    it("throws an error if gemfire is not configured", function(done) {
      expectExternalFailure(
        "not_configured",
        done,
        "gemfire: You must call configure() before calling getCache()."
      );
    });
  });

  describe(".close", function() {
    it("does not throw an error", function(done) {
      expectExternalSuccess("close_cache", done);
    });

    it("causes subsequent cache operations to throw", function(done) {
      // The script catches expected exceptions and throws unexpected ones
      expectExternalSuccess("close_cache_then_call_cache_methods", done);
    });

    it("causes subsequent region operations to throw", function(done) {
      // The script catches expected exceptions and throws unexpected ones
      expectExternalSuccess("close_cache_then_call_region_methods", done);
    });
  });

  describe(".getRegion", function() {
    var cache;

    beforeEach(function() {
      cache = factories.getCache();
    });

    it("validates arguments", function(){
      function callWithZeroArguments(){
        cache.getRegion();
      }
      function callWithOneArgument(){
        cache.getRegion("exampleRegion");
      }
      function callWithTwoArguments(){
        cache.getRegion("exampleRegion", "foo");
      }

      expect(callWithZeroArguments).toThrow(new Error("You must pass the name of a GemFire region to getRegion."));
      expect(callWithOneArgument).not.toThrow();
      expect(callWithTwoArguments).toThrow(new Error("You must pass the name of a GemFire region to getRegion."));
    });

    it("returns a gemfire.Region object", function() {
      var region = cache.getRegion("exampleRegion");
      expect(region.constructor.name).toEqual("Region");
    });

    it("returns undefined if the region is unknown", function(){
      expect(cache.getRegion("there is no such region")).toBeUndefined();
    });

    it("throws an error when a non-string name is passed in", function() {
      function getRegionWithNonStringArguments(){
        cache.getRegion({});
      }

      expect(getRegionWithNonStringArguments).toThrow(
        new Error("You must pass a string as the name of a GemFire region to getRegion.")
      );
    });

  });

  describe(".executeQuery", function () {
    var cache, region;

    beforeEach(function(done) {
      cache = factories.getCache();
      region = cache.getRegion("exampleRegion");
      region.clear(done);
    });

    it("executes a query that can retrieve string results", function(done) {
      async.parallel(
        [
          function(callback) { region.put("string1", "a string", callback); },
          function(callback) { region.put("string2", "another string", callback); },
          function(callback) { region.put("string3", "a string", callback); },
        ],
        function(){
          const query = "SELECT DISTINCT * FROM /exampleRegion";

          cache.executeQuery(query, {poolName: "myPool"}, function(error, response) {
            expect(error).not.toBeError();
            if(error) { return; }

            const results = response.toArray();

            expect(results.length).toEqual(2);
            expect(results).toContain("a string");
            expect(results).toContain("another string");

            done();
          });
        }
      );
    });

    it("executes a query with an OQL predicate", function(done) {
      async.parallel(
        [
          function(callback) { region.put("string1", "a string", callback); },
          function(callback) { region.put("string2", "another string", callback); },
        ],
        function() {
          const query = "SELECT entry.value FROM /exampleRegion.entries entry WHERE entry.key = 'string2'";

          cache.executeQuery(query, {poolName: "myPool"}, function(error, response) {
            expect(error).not.toBeError();
            if(error) { return; }

            const results = response.toArray();

            expect(results.length).toEqual(1);
            expect(results).toContain("another string");

            done();
          });
        }
      );
    });

    it("executes a query with a where clause on a field", function(done){
      const object = { foo: 'bar' };

      async.parallel(
        [
          function(callback) { region.put("object", object, callback); },
          function(callback) { region.put("other object", { foo: 'qux' }, callback); },
          function(callback) { region.put("empty", {}, callback); },
        ],
        function() {
          const query = "SELECT * FROM /exampleRegion WHERE foo = 'bar'";

          cache.executeQuery(query, {poolName: "myPool"}, function(error, response) {
            expect(error).not.toBeError();
            if(error) { return; }

            const results = response.toArray();

            expect(results.length).toEqual(1);
            expect(results).toContain(object);

            done();
          });
        }
      );
    });

    it("executes a query with case insensitive matching on a field", function(done){
      const value = 'fooBaRbaz';
      const otherValue = 'foobarbaz';

      async.parallel(
        [
          function(callback) { region.put("value", value, callback); },
          function(callback) { region.put("other value", otherValue, callback); },
          function(callback) { region.put("no match value", 'foobazbaz', callback); },
          function(callback) { region.put("empty", '', callback); },
        ],
        function() {
          const query = "SELECT entry.value FROM /exampleRegion.entries entry WHERE entry.value.toUpperCase LIKE '%BAR%'";

          cache.executeQuery(query, {poolName: "myPool"}, function(error, response) {
            expect(error).not.toBeError();
            if(error) { return; }

            const results = response.toArray();

            expect(results.length).toEqual(2);
            expect(results).toContain(value);
            expect(results).toContain(otherValue);

            done();
          });
        }
      );
    });

    it("executes a query with case insensitive matching on a JSON field", function(done){
      const object = { foo: 'fooBaRbaz' };
      const otherObject = { foo: 'foobarbaz' };

      async.parallel(
        [
          function(callback) { region.put("object", object, callback); },
          function(callback) { region.put("other object", otherObject, callback); },
          function(callback) { region.put("no match object", { foo: 'foobazbaz' }, callback); },
          function(callback) { region.put("empty", {}, callback); },
        ],
        function() {
          const query = "SELECT * FROM /exampleRegion WHERE foo.toLowerCase() LIKE '%bar%'";

          cache.executeQuery(query, {poolName: "myPool"}, function(error, response) {

           
            expect(error).not.toBeError();

            if(error) { return; }
            const results = response.toArray();
            expect(results.length).toEqual(2);
            expect(results).toContain(object);
            expect(results).toContain(otherObject);
            done();
          });
        }
      );
    });

    it("executes a query with parameterized values", function(done){
      const object = { foo: 'bar' };

      async.parallel(
        [
          function(callback) { region.put("object", object, callback); },
          function(callback) { region.put("other object", { foo: 'qux' }, callback); },
          function(callback) { region.put("empty", {}, callback); },
        ],
        function() {
          const query = "SELECT * FROM /exampleRegion WHERE foo = $1 OR foo = $2";

          cache.executeQuery(query, ['bar', 'baz'], {poolName: "myPool"}, function(error, response) {
            expect(error).not.toBeError();
            if(error) { return; }

            const results = response.toArray();

            expect(results.length).toEqual(1);
            expect(results).toContain(object);

            done();
          });
        }
      );
    });

    it("executes a query with empty parameters", function(done){
      const object = { foo: 'bar' };

      async.parallel(
        [
          function(callback) { region.put("object", object, callback); },
          function(callback) { region.put("other object", { foo: 'qux' }, callback); },
          function(callback) { region.put("empty", {}, callback); },
        ],
        function() {
          const query = "SELECT * FROM /exampleRegion WHERE foo = 'bar' OR foo = 'baz'";

          cache.executeQuery(query, [], {poolName: "myPool"}, function(error, response) {
            expect(error).not.toBeError();
            if(error) { return; }

            const results = response.toArray();

            expect(results.length).toEqual(1);
            expect(results).toContain(object);

            done();
          });
        }
      );
    });

    it("executes a query with null parameters", function(done){
      const object = { foo: 'bar' };

      async.parallel(
        [
          function(callback) { region.put("object", object, callback); },
          function(callback) { region.put("other object", { foo: 'qux' }, callback); },
          function(callback) { region.put("empty", {}, callback); },
        ],
        function() {
          const query = "SELECT * FROM /exampleRegion WHERE foo = 'bar' OR foo = 'baz'";

          cache.executeQuery(query, null, {poolName: "myPool"}, function(error, response) {
            expect(error).not.toBeError();
            if(error) { return; }

            const results = response.toArray();

            expect(results.length).toEqual(1);
            expect(results).toContain(object);

            done();
          });
        }
      );
    });

    it("executes a query with a nested object", function(done) {
      const object1 = { foo: [{ bar: 'baz' }] };

      async.parallel(
        [
          function(callback) { region.put("object1", object1, callback); },
          function(callback) { region.put("object2", { foo: [{ bar: 'qux' }] }, callback); },
        ],
        function(){
          const query = "SELECT record FROM /exampleRegion AS record, record.foo AS foo WHERE foo.bar = 'baz'";

          cache.executeQuery(query, {poolName: "myPool"}, function(error,response) {
            expect(error).not.toBeError();
            if(error) { return; }

            const results = response.toArray();

            expect(results.length).toEqual(1);
            expect(results).toContain(object1);

            done();
          });
        }
      );
    });

    it("executes a query that returns a GemFire struct", function(done) {
      region.put("object", { foo: 1, bar: 2, baz: 3 }, function(error) {
        expect(error).not.toBeError();
        if(error) { return; }

        const query = "SELECT foo, bar FROM /exampleRegion";

        cache.executeQuery(query, {poolName: "myPool"}, function(error, response){
          expect(error).not.toBeError();
          if(error) { return; }

          const results = response.toArray();

          expect(results.length).toEqual(1);
          expect(results[0]).toEqual({ foo: 1, bar: 2 });

          done();
        });
      });
    });

    it("executes a query that can retrieve results of all types", function(done) {
      async.parallel(
        [
          function(callback) { region.put("a string", "a string", callback); },
          function(callback) { region.put("an object", {"an": "object"}, callback); },
        ],
        function(){
          const query = "SELECT DISTINCT * FROM /exampleRegion";

          cache.executeQuery(query, {poolName: "myPool"}, function(error, response) {
            expect(error).not.toBeError();
            if(error) { return; }

            const results = response.toArray();

            expect(results.length).toEqual(2);
            expect(results).toContain({"an": "object"});
            expect(results).toContain("a string");

            done();
          });
        }
      );
    });

    it("can search for wide strings", function(done){
      async.series(
        [
          function(callback) { region.put("narrow string", "Japan", callback); },
          function(callback) { region.put("wide string", "日本", callback); },
          function(callback) {
            const narrowQuery = "SELECT key FROM /exampleRegion.entrySet WHERE value = 'Japan';";
            cache.executeQuery(narrowQuery, {poolName: "myPool"}, function(error, response){
              expect(error).not.toBeError();
              if(error) { return; }

              const results = response.toArray();
              expect(results).toEqual(["narrow string"]);
              callback();
            });
          },
          function(callback) {
            const wideQuery = "SELECT key FROM /exampleRegion.entrySet WHERE value = '日本';";
            cache.executeQuery(wideQuery, {poolName: "myPool"}, function(error, response){
              expect(error).not.toBeError();
              if(error) { return; }

              const results = response.toArray();
              expect(results).toEqual(["wide string"]);
              callback();
            });
          },
        ],
        done
      );
    });

    it("returns 'undefined' for fields that do not apply", function(done) {
      async.series([
        function(next) { region.put("foo", {}, next); },
        function(next) { region.put("bar", { my_field_name: 'baz' }, next); },
        function(next) {
          cache.executeQuery("SELECT my_field_name FROM /exampleRegion", {poolName: "myPool"}, function(error, response) {
            expect(error).not.toBeError();
            if(error) { return; }

            const results = response.toArray();
            expect(results.length).toEqual(2);
            expect(results.indexOf(undefined)).not.toEqual(-1); //toContain does not support undefined
            expect(results).toContain('baz');

            next();
          });
        }
      ], done);
    });

    it("throws an error when no query is passed", function() {
      function callWithoutQuery() {
        cache.executeQuery();
      }

      expect(callWithoutQuery).toThrow(new Error("You must pass a query string and callback to executeQuery()."));
    });

    it("throws an error if you don't pass a callback", function(){
      function callWithoutCallback() {
        cache.executeQuery("SELECT * FROM /exampleRegion");
      }

      expect(callWithoutCallback).toThrow(new Error("You must pass a callback to executeQuery()."));
    });

    it("throws an error if you pass a non-function as the callback", function(){
      function callWithNonCallback() {
        cache.executeQuery("SELECT * FROM /exampleRegion", "Not a callback");
      }

      expect(callWithNonCallback).toThrow(new Error("You must pass a function as the callback to executeQuery()."));
    });

    it("returns the cache for chaining", function(done) {
      var query = "SELECT DISTINCT * FROM /exampleRegion;";
      var returnValue = cache.executeQuery(query, {poolName: "myPool"}, function(error, results) {
        done();
      });
      expect(returnValue).toEqual(cache);
    });

    it("passes an error to the callback for invalid queries", function(done) {
      var exception;

      cache.executeQuery("INVALID;", {poolName: "myPool"}, function(error, results) {
        expect(error).toBeError('apache::geode::client::QueryException');
        expect(results).toBeUndefined();
        done();
      });
    });

    describe("when a valid poolName is given", function() {
      it("executes the query on that pool", function(done) {
        const query = "SELECT DISTINCT * FROM /exampleRegion;";

        async.series([
          function(next) { region.clear(next); },
          function(next) { region.put("foo", "bar", next); },
          function(next) {
            cache.executeQuery(query, {poolName: "myPool"}, function(error, results) {
              expect(error).not.toBeError();
              expect(results.toArray()).toEqual(["bar"]);
              next();
            });
          }
        ], done);
      });
    });

    describe("when an invalid poolName is given", function() {
      it("throws an error", function() {
        const query = "SELECT DISTINCT * FROM /exampleRegion;";

        function executeQueryWithInvalidPoolName() {
          cache.executeQuery(query, {poolName: "invalidPool"}, function(){});
        }

        expect(executeQueryWithInvalidPoolName).toThrow(
          new Error("executeQuery: `invalidPool` is not a valid pool name")
        );
      });
    });
  });

  describe(".inspect", function() {
    it("returns a user-friendly display string describing the cache", function() {
      expect(factories.getCache().inspect()).toEqual('[Cache]');
    });
  });

  describe(".rootRegions", function() {
    it("returns an array of top level regions", function() {
      const cache = factories.getCache();
      const rootRegions = cache.rootRegions();

      _.each(rootRegions, function(rootRegion) {
        expect(rootRegion.constructor.name).toEqual("Region");
      });

      const actualRegionNames = _.pluck(rootRegions, "name");
      expect(actualRegionNames).toContain("exampleRegion");
      expect(actualRegionNames).toContain("anotherRegion");
    });
  });

  describe(".executeFunction", function() {
    const expectFunctionsToThrowExceptionsCorrectly = false;
    itExecutesFunctions(
      factories.getCache,
      expectFunctionsToThrowExceptionsCorrectly
    );

    // TODO: reenable this test when the Native Client is updated to throw errors for Cache.executeFunction
    // See https://groups.google.com/a/pivotal.io/d/topic/labs-node-gemfire/HGOnikEWtNw/discussion
    xit("does not give the function access to a region", function(done) {
      const cache = factories.getCache();
      const functionName = "io.pivotal.node_gemfire.SumRegion";

      cache.executeFunction(functionName)
        .on("error", function(error) {
          expect(error).toBeError(
            /cannot be cast to com.gemstone.gemfire.cache.execute.RegionFunctionContext/
          );
          done();
        });
    });

    it("throws an error if filters are provided", function() {
      const cache = factories.getCache();

      function callWithFilter() {
        cache.executeFunction(
          "io.pivotal.node_gemfire.ReturnFilter",
          { filter: ["foo", "bar"] }
        );
      }

      expect(callWithFilter).toThrow(
        new Error("You cannot pass a filter to executeFunction for a Cache.")
      );
    });

    it("successfully executes the function even if there are no regions in the client XML", function(done) {
      expectExternalSuccess("execute_function_without_regions", done);
    });

    describe("when a valid poolName is given", function() {
      it("executes the function on that pool", function(done) {
        const cache = factories.getCache();
        const functionName = "io.pivotal.node_gemfire.Passthrough";

        var result;

        cache.executeFunction(functionName, { arguments: [1,2], poolName: "myPool" })
          .on("data", function(data) {
            result = data;
          })
          .on("end", function(){
            expect(result).toEqual([1,2]);
            done();
          });
      });
    });

    describe("when an invalid poolName is given", function() {
      it("throws an error", function() {
        const cache = factories.getCache();
        const functionName = "io.pivotal.node_gemfire.Passthrough";

        function executeFunctionWithInvalidPoolName() {
          cache.executeFunction(functionName, { arguments: [1,2], poolName: "invalidPool" });
        }

        expect(executeFunctionWithInvalidPoolName).toThrow(
          new Error("executeFunction: `invalidPool` is not a valid pool name")
        );
      });
    });

    describe("when the function is executed synchronously", function() {
      const cache = factories.getCache();
      const functionName = "io.pivotal.node_gemfire.SynchronousPut";

      beforeEach(function() {
        originalDefaultTimeoutInterval = jasmine.DEFAULT_TIMEOUT_INTERVAL;
        jasmine.DEFAULT_TIMEOUT_INTERVAL = 8000;
      });

      it("executes function in the same thread and blocks", function() {
        var region = cache.getRegion("exampleRegion");
        var returned = region.executeFunction(functionName, { arguments: ["key1", "value1", 3, 4], synchronous: true, poolName: "myPool" });

        expect(returned).toEqual([3,4]);

        region.get("key1", function(error, result) {
          expect(result).toEqual("value1");
        });
      });

      it("executes a function without blocking but will not find the value yet", function(done) {
        var region = cache.getRegion("exampleRegion");

        async.series([
          function(next) {
            region.executeFunction(functionName, { arguments: ["key2", "value2", 5, 6], synchronous: false, poolName: "myPool" })
              .on("error", function(error) { throw(error); })
              .on("end", function(data) {
                expect(result).toEqual([5,6]);
                // not calling next() here to force the test to move on before it is finished. this can cause spurious test failures
              })
              .on("data", function(data) {
                result = data;
              });
            next();
          },
          function(next) {
            region.get("key2", function(error, result) {
              expect(result).toEqual(undefined);
              expect(error.name).toEqual("KeyNotFoundError");
            });
            next();
          },
          function(next) {
            setTimeout(function() {
              region.get("key2", function(error, result) {
                expect(error).not.toBeError();
                expect(result).toEqual("value2");
              });
              next();
            }, 3000);
          }
        ], done);
      });

      describe("when the synchronous flag is not a boolean", function() {
        it("throws an error", function() {
          function executeFunctionWithInvalidFlag() {
            cache.executeFunction(functionName, { arguments: [], synchronous: 5, poolName: "myPool" });
          }

          expect(executeFunctionWithInvalidFlag).toThrow(
            new Error("You must pass true or false for the synchronous option for executeFunction().")
          );
        });
      });

      afterEach(function() {
        jasmine.DEFAULT_TIMEOUT_INTERVAL = originalDefaultTimeoutInterval;
      });
    });
  });

  describe(".createRegion", function() {
    it("creates a region", function() {
      const cache = factories.getCache();

      expect(cache.getRegion("newRegion")).toBeUndefined();

      const newRegion = cache.createRegion("newRegion", {type: "LOCAL"});

      expect(newRegion).toBeDefined();
      expect(cache.getRegion("newRegion")).toEqual(newRegion);
    });

    it("throws an error when a region already exists", function() {
      const cache = factories.getCache();

      function createExistingRegion(){
        cache.createRegion("exampleRegion", {type: "CACHING_PROXY", poolName: "myPool"});
      }

      expect(createExistingRegion).toThrowNamedError(
        'apache::geode::client::RegionExistsException',
        'Cache::createRegion: "exampleRegion" region exists in local cache'
      );
    });

    it("throws an error when no name is passed in", function() {
      const cache = factories.getCache();

      function createRegionWithNoArguments(){
        cache.createRegion();
      }

      expect(createRegionWithNoArguments).toThrow(
        new Error(
          "createRegion: You must pass the name of a GemFire region to create and a region configuration object."
        )
      );
    });

    it("throws an error when a non-string name is passed in", function() {
      const cache = factories.getCache();

      function createRegionWithNonStringArguments(){
        cache.createRegion({});
      }

      expect(createRegionWithNonStringArguments).toThrow(
        new Error("createRegion: You must pass a string as the name of a GemFire region.")
      );
    });

    it("throws an error when no configuration object is passed in", function() {
      const cache = factories.getCache();

      function createRegionWithOnlyName(){
        cache.createRegion("regionName");
      }

      expect(createRegionWithOnlyName).toThrow(
        new Error('createRegion: You must pass a configuration object as the second argument.')
      );
    });

    it("throws an error when the passed-in configuration object has no type property", function() {
      const cache = factories.getCache();

      function createRegionWithOnlyName(){
        cache.createRegion("regionName", {});
      }

      expect(createRegionWithOnlyName).toThrow(
        new Error('createRegion: The region configuration object must have a type property.')
      );
    });

    it("throws an error when the passed-in type is not a valid GemFire client region type", function() {
      const cache = factories.getCache();

      function createRegionWithOnlyName(){
        cache.createRegion("regionName", {type: 123});
      }

      expect(createRegionWithOnlyName).toThrow(
        new Error('createRegion: This type is not a valid GemFire client region type')
      );
    });

    describe("when the client type is set to PROXY", function() {
      it("creates a PROXY region", function() {
        const cache = factories.getCache();
        const region = cache.createRegion("createRegionCachingProxyTest", { type: "PROXY", poolName: "myPool" });

        expect(region.attributes.cachingEnabled).toBeFalsy();
        //TODO - scope isn't a thing anymore in the CPP API.
        //expect(region.attributes.scope).toEqual("DISTRIBUTED_NO_ACK");
      });
    });

    describe("when the client type is set to CACHING_PROXY", function() {
      it("creates a CACHING_PROXY region", function() {
        const cache = factories.getCache();
        const region = cache.createRegion("createRegionProxyTest", { type: "CACHING_PROXY", poolName: "myPool" });

        expect(region.attributes.cachingEnabled).toBeTruthy();
        //TODO - scope isn't a thing anymore in the CPP API.
        //expect(region.attributes.scope).toEqual("DISTRIBUTED_NO_ACK");
      });
    });

    describe("when the client type is set to LOCAL", function() {
      it("creates a LOCAL region", function() {
        const cache = factories.getCache();
        const region = cache.createRegion("createRegionLocalTest", { type: "LOCAL" });
        expect(region.attributes.cachingEnabled).toBeTruthy();

        //TODO - scope isn't a thing anymore in the CPP API.
        //expect(region.attributes.scope).toEqual("LOCAL");
      });
    });

    describe("when the pool is set", function() {
      it("creates a region for that pool", function() {
        const cache = factories.getCache();
        const region = cache.createRegion("createRegionPoolNameSetTest", { type: "PROXY", poolName: "myPool" });
        expect(region.attributes.poolName).toEqual("myPool");
      });
    });
  });
});
