var randomString = require('random-string');
var _ = require('lodash');
var microtime = require("microtime");

var gemfire = require('../..');
var cache = new gemfire.Cache('benchmark/xml/BenchmarkClient.xml');
var region = cache.getRegion("exampleRegion");

console.log("Gemfire version " + gemfire.version());

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

var randomObject = require('../data/randomObject.json');
var gemfireKey = randomString(keyOptions);

var suffix = 0;
function putNValues(n){
  _.times(n, function(pair) {
    suffix++;
    region.put(gemfireKey + suffix, randomObject);
  });
}

function benchmark(numberOfPuts){
  var start = microtime.now();

  putNValues(numberOfPuts);

  var microseconds = microtime.now() - start;
  var seconds = (microseconds / 1000000);

  var putsPerSecond = Math.round(numberOfPuts / seconds);
  var usecPerPut = Math.round(microseconds / numberOfPuts);

  console.log(
    "" + numberOfPuts + " puts: ", + usecPerPut + " usec/put " + putsPerSecond + " puts/sec"
  );
}

_.each([1, 10], function(numberOfPuts){
  benchmark(numberOfPuts);
});
