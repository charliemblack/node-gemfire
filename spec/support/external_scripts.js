const childProcess = require('child_process');

//jasmine.getEnv().addReporter({
//  specStarted: function(result) {
//      console.log(result.fullName);
//  }
//});
//jasmine.getEnv().afterEach(function(){
//  console.log("done with last test")
//});
//process.on("uncaughtException",function(e) {
//  console.log("Caught unhandled exception: " + e);
//  console.log(" ---> : " + e.stack);
//});

const errorMatchers = require("./error_matchers");

function runExternalTest(name, callback) {
  if(!callback) { throw("You must pass a callback");  }

  var filename = "spec/support/scripts/" + name + ".js";

  childProcess.execFile("node", [filename], callback);
}

exports.expectExternalSuccess = function expectExternalSuccess(name, callback){
  jasmine.addMatchers(errorMatchers);

  runExternalTest(name, function(error, stdout, stderr) {
    expect(error).not.toBeError();
    expect(stderr).toEqual('');
    callback();
  });
};

exports.expectExternalFailure = function expectExternalFailure(name, callback, message){
  jasmine.addMatchers(errorMatchers);

  runExternalTest(name, function(error, stdout, stderr) {
    expect(error).not.toBeNull();
    expect(stderr.indexOf(message) >= 0).toBe(true);
    callback();
  });
};
