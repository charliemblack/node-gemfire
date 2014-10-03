# API Documentation

## Cache

A GemFire cache is an in-memory data store composed of many Regions, which may or may not participate in a cluster. A cache is constructed with an xml configuration file:

```javascript
var gemfire = require('gemfire');
var cache = new gemfire.Cache('config/gemfire.xml');
```

For more information on cache configuration files, see [the documentation](http://gemfire.docs.pivotal.io/latest/userguide/gemfire_nativeclient/cache-init-file/chapter-overview.html#chapter-overview).

### cache.executeQuery(query, callback)

Executes an OQL query on the cluster. The callback will be called with an `error` argument and a `response` argument. The `response` argument is an object responding to `toArray` and `each`.

 * `results.toArray()`: Return the entire result set as an Array.
 * `results.each(callback)`: Call the callback with a `result` argument, once for each result.

Example:
```javascript
cache.executeQuery("SELECT DISTINCT * FROM /exampleRegion", function(error, response) {
  if(error) { throw error; }
  
  var results = response.toArray();
  // allResults could now be this:
  //   [ 'value1', 'value2', { foo: 'bar' } ]
  
  // alternately, you could use the `each` iterator:
  response.each(function(result) {
  	// this callback will be called with 'value1', then 'value2', then { foo: 'bar' }
  });
}
```

For more information on OQL, see [the documentation](http://gemfire.docs.pivotal.io/latest/userguide/developing/querying_basics/chapter_overview.html).

### cache.getRegion(regionName)

Retrieves a Region from the Cache. An error will be thrown if the region is not present.

Example:
```javascript
var region = cache.getRegion('exampleRegion');
```

### cache.rootRegions(callback)

Retrieves an array of all root Regions from the Cache. Example:

```javascript
var regions = cache.rootRegions();
// if there are three Regions defined in your cache, regions could now be:
// [firstRegionName, secondRegionName, thirdRegionName]
```

## Region

A GemFire region is a collection of key-value pair entries. The most common way to get a region is to call `cache.getRegion('regionName')` on a Cache.

### region.clear([callback])

Removes all entries from the region. The callback will be called with an `error` argument. If the callback is not supplied, and an error occurs, the region will emit an `error` event.

Example:
```javascript
region.clear(function(error){
  if(error) { throw error; }
  // region is clear
});
```

### region.executeFunction(functionName, options)

Executes a Java function on any servers in the cluster containing the region. `functionName` is the full Java class name of the function that will be called. Options may be either an array of arguments, or an options object containing the optional fields `arguments` and `filters`.

 * `options.arguments`: the arguments to be passed to the Java function
 * `options.filter`: an array of keys to be sent to the Java function as the filter. 

executeFunction returns an EventEmitter which emits the following events:

 * `data`: Emitted once for each result sent by the Java function.
 * `error`: Emitted if the function throws or returns an Exception.
 * `end`: Called after the Java function has finally returned.

```javascript
region.executeFunction("com.example.FunctionName", 
    { arguments: [1, 2, 3], filters: ["key1", "key2"] }
  )
  .on("error", function(error) { throw error; })
  .on("data", function(result) {
    // ...
  })
  .on("end", function() {
    // ...
  });
```

For more information, please see the [GemFire documentation for Function Execution](http://gemfire.docs.pivotal.io/latest/userguide/developing/function_exec/chapter_overview.html).

### region.executeFunction(functionName, arguments)

Shorthand for `executeFunction` with an array of arguments. Equivalent to:

```javascript
region.executeFunction(functionName, { arguments: arguments })
```

### region.existsValue(predicate, callback)

Indicates whether or not a value matching the OQL predicate `predicate` is present in the region. The callback will be called with an `error` and the boolean `response`.

In the predicate, you can use `this` to refer to the entire value. If you use any other expression, it will be treated as a field name on object values.

Example:
```javascript
region.existsValue("this > 'value1'", function(error, response) {
  if(error) { throw error; }
  // response will be set to true if any value is the string 'value1'
});

region.existsValue("foo > 2", function(error, response) {
  if(error) { throw error; }
  // response will be set to true if any value is an object such
  // as { foo: 3 } where the value of foo is greater than 2.
});
```

See also `region.query` and `region.selectValue`.

### region.get(key, callback)

Retrieves the value of an entry in the Region. The callback will be called with an `error` and the `value`. If the key is not present in the Region, an error will be passed to the callback.

Example
```javascript
region.get("key", function(error, value){
  if(error) { throw error; }
  // an entry was found and its value is now accessible
});
```

### region.getAll(keys, callback)

Retrieves the values of multiple keys in the Region. The keys should be passed in as an `Array`. The callback will be called with an `error` and a `values` object. If one or more keys are not present in the region, their values will be undefined.

Example
```javascript
region.getAll(["key1", "key2", "unknownKey"], function(error, values){
  if(error) { throw error; }
  // if key1 and key2 are present in the region, but unknownKey is not,
  // then values may look like this:
  // { key1: 'value1', key2: { foo: 'bar' } }
});
```

### region.keys(callback)

Retrieves all keys in the Region. The callback will be called with an `error` argument, and an Array of keys.

Example:
```
region.keys(function(error, keys) {
  if(error) { throw error; }
  // keys is now an array of all keys in the region, for example:
  //   [ 'key1', 'key2', 'key3' ]
});
```

### region.name

Returns the name of the region.

### region.put(key, value, [callback])

Stores an entry in the region. The callback will be called with an `error` argument. If the callback is not supplied, an an error occurs, the region will emit an `error` event.

GemFire supports most JavaScript types for the value. Some types, such as `Function` and `null` cannot be stored. 

GemFire supports several JavaScript types for the key, but the safest choice is to always use a `String`.

Example:
```javascript
region.put('key', { foo: 'bar' }, function(error) {
  if(error) { throw error; }
  // the entry at key "key" now has value { foo: 'bar' }
});
```

### region.putAll(entries, [callback])

Stores multiple entries in the region. The callback will be called with an `error` argument. If the callback is not supplied, and an error occurs, the Region will emit an `error` event.

Example:
```javascript
region.putAll(
  {
    key1: 'value1', 
    key2: { foo: 'bar' } 
  }, 
  function(error) {
    if(error) { throw error; }
    // the entry at key "key1" now has value "value1"
    // the entry at key "key2" now has value { foo: 'bar' }
  }
);
```

### region.remove(key, [callback])

Removes the entry specified by the indicated key from the Region, or, if no such entry is present, passes an `error` to the callback. If the argument is not supplied, and an error occurs, the Region will emit an `error` event.

Example:
```javascript
region.remove('key1', function(error) {
  if(error) { throw error; }
  // the entry with key 'key1' has been removed from the region
});
```

### region.selectValue(predicate, callback)

Retrieves exactly one entry from the Region matching the OQL `predicate`. The callback will be called with an `error` argument, and a `result`.

If more than one result matches the predicate, an error will be passed to the callback. If you want to select multiple results for a predicate, see `region.query`.

Example:
```javascript
region.selectValue("this = 'value1'", function(error, result) {
  if(error) { throw error; }
  // if there is exactly one entry with value 'value1' in the region,
  // result will now be set to 'value1'
});
```

See also `region.query` and `region.existsValue`.

### region.query(predicate, callback)

Retrieves all values from the Region matching the OQL `predicate`. The callback will be called with an `error` argument, and a `response` object. For more information on `response` objects, please see `cache.executeQuery`.

Example:
```javascript
region.query("this like '% Smith', function(error, response) {
  if(error) { throw error; }
  
  var results = response.toArray();
  // results will now be an array of all values in the region ending in " Smith"
  
  response.each(function(result) {
    // this callback will be passed each value in the region ending in " Smith", one at a time
  });
});
```

See also `region.selectValue` and `region.existsValue`.

### Event: 'error'

* error: `Error` object.

Emitted when an error occurs and no callback was passed to the method that caused the error.

Example:
```javascript
region.on("error", function(error) {
  // handle errors
});

// emits an error because null is not a supported value for put()
region.put("foo", null);

// does not emit an error, because a callback was passed
region.put("foo", null, function(error) {});
```