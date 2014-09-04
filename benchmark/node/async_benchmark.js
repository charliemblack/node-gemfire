var randomString = require('random-string');
var _ = require('lodash');
var microtime = require("microtime");

var gemfire = require('../..');
var cache = new gemfire.Cache('benchmark/xml/BenchmarkClient.xml');
var region = cache.getRegion("exampleRegion");

region.clear();

console.log("node-gemfire version " + gemfire.version);
console.log("GemFire version " + gemfire.gemfireVersion);

region.put('smoke', { test: 'value' });
if(JSON.stringify(region.get('smoke')) !== JSON.stringify({ test: 'value' })) {
  throw "Smoke test failed.";
}

var keyOptions = {
  length: 8,
  numeric: true,
  letter: true,
  special: false
};

var valueOptions = {
  length: 15 * 1024,
  numeric: true,
  letter: true,
  special: true
};

var randomObject = require('../data/randomObject.json');
var stringValue = randomString(valueOptions);
var gemfireKey = randomString(keyOptions);

var suffix = 0;
function benchmark(numberOfPuts, title, done, callback) {
  var start = microtime.now();

  callback(numberOfPuts, function(){
    var microseconds = microtime.now() - start;
    var seconds = (microseconds / 1000000);

    var putsPerSecond = Math.round(numberOfPuts / seconds);
    var usecPerPut = Math.round(microseconds / numberOfPuts);

    console.log(
      "(" + title + ") " + numberOfPuts + " puts: ", + usecPerPut + " usec/put " + putsPerSecond + " puts/sec"
    );

    done();
  });
}

function putNValues(value) {
  return function(n, done) {
    var success = 0;

    var i = 0;

    _.times(n, function(pair) {
      suffix++;
      region.put(gemfireKey + suffix, value, function(error){
        if(error) {
          throw error;
        }

        i++;

        if(i == n) {
          done();
        }
      } );
    });
  };
}

function benchmarkObjects(numberOfPuts, done){
  benchmark(numberOfPuts, "object", done, putNValues(randomObject));
}

function benchmarkStrings(numberOfPuts, done){
  benchmark(numberOfPuts, "string", done, putNValues(stringValue));
}

benchmarkObjects(1, function(){
  benchmarkObjects(10, function () {
    benchmarkObjects(100, function () {
      benchmarkStrings(100, function () {
        benchmarkStrings(1000, function () {
          benchmarkStrings(10000, function () {
          });
        });
      });
    });
  });
});
