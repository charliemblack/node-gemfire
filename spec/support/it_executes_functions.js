module.exports = function itExecutesFunctions(subjectSource, expectFunctionsToThrowExceptionsCorrectly) {
  const testFunctionName = "io.pivotal.node_gemfire.TestFunction";
  var subject;

  beforeEach(function() {
    subject = subjectSource();
  });

  describe("shared behaviors for function execution", function() {
    it("runs a function on the GemFire cluster and emits results via the 'data' event", function(done) {
      const dataCallback = jasmine.createSpy("dataCallback");
      subject
        .executeFunction(testFunctionName)
        .on("data", dataCallback)
        .on("end", function(){
          expect(dataCallback.calls.count()).toEqual(1);
          expect(dataCallback).toHaveBeenCalledWith("TestFunction succeeded.");
          done();
        });
    });

    it("runs a function with arguments as an Array on the GemFire cluster", function(done) {
      const dataCallback = jasmine.createSpy("dataCallback");
      subject.executeFunction("io.pivotal.node_gemfire.Sum", [1, 2, 3])
        .on("data", dataCallback)
        .on("end", function(){
          expect(dataCallback.calls.count()).toEqual(1);
          expect(dataCallback).toHaveBeenCalledWith(6);
          done();
        });
    });

    it("runs a function with arguments as part of an options Object on the GemFire cluster", function(done) {
      const dataCallback = jasmine.createSpy("dataCallback");
      subject.executeFunction("io.pivotal.node_gemfire.Sum", {arguments: [1, 2, 3]})
        .on("data", dataCallback)
        .on("end", function(){
          expect(dataCallback.calls.count()).toEqual(1);
          expect(dataCallback).toHaveBeenCalledWith(6);
          done();
        });
    });

    it("throws an error when no function name is passed in", function(){
      function callWithoutArgs() {
        subject.executeFunction();
      }
      expect(callWithoutArgs).toThrow(new Error("You must provide the name of a function to execute."));
    });

    it("throws an error when no function name is passed in, but a non-name is", function() {
      function callWithoutName() {
        subject.executeFunction(function(){});
      }

      expect(callWithoutName).toThrow(new Error("You must provide the name of a function to execute."));
    });

    it("throws an error when a function is passed in as an argument", function() {
      function callWithBadArgs() {
        subject.executeFunction(testFunctionName, [function(){}]);
      }

      expect(callWithBadArgs).toThrow(new Error("Unable to serialize to GemFire; functions are not supported."));
    });

    it("emits an error when the function is not found", function(done){
      subject.executeFunction("com.example.Nonexistent")
        .on('error', function(error) {
          expect(error).toBeError(
            "apache::geode::client::MessageException",
            "Execute::GET_FUNCTION_ATTRIBUTES: message from server could not be handled"
          );
          done();
        });
    });

    if(expectFunctionsToThrowExceptionsCorrectly) {
      it("emits an error when the function throws an error", function(done) {
        subject.executeFunction("io.pivotal.node_gemfire.TestFunctionException")
          .on('error', function(error) {
            expect(error).toBeError(
              'apache::geode::client::CacheServerException',
              /org.apache.geode.cache.execute.FunctionException: Test exception message thrown by server./
            );
            done();
          });
      });
    } else {
      // TODO: reenable this test when the Native Client is updated to throw errors for Cache.executeFunction
      // See https://groups.google.com/a/pivotal.io/d/topic/labs-node-gemfire/HGOnikEWtNw/discussion
      xit("emits an error when the function throws an error");
    }

    it("passes the results and an error when the function sends an exception with the results", function(done) {
      const dataCallback = jasmine.createSpy("dataCallback");
      subject.executeFunction("io.pivotal.node_gemfire.TestFunctionExceptionResult")
        .on('data', dataCallback)
        .on('error', function(error) {
          expect(error).toBeError('UserFunctionExecutionException',
                                  /java.lang.Exception: Test exception message sent by server./);
        })
        .on('end', function() {
          expect(dataCallback.calls.count()).toEqual(1);
          expect(dataCallback).toHaveBeenCalledWith("First result");
          done();
        });
    });

    it("throws an error when the options are not an Object or an Array", function() {
      function passNonObjectAsOptions() {
        subject.executeFunction(
          "io.pivotal.node_gemfire.Passthrough",
          "this string is not an options object"
        );
      }

      expect(passNonObjectAsOptions).toThrow(
        new Error("You must pass either an Array of arguments or an options Object to executeFunction().")
      );
    });

    it("supports objects as input and output", function(done) {
      const dataCallback = jasmine.createSpy("dataCallback");
      subject.executeFunction("io.pivotal.node_gemfire.Passthrough", { arguments: { foo: 'bar' } })
        .on('data', dataCallback)
        .on('end', function(){
          expect(dataCallback.calls.count()).toEqual(1);
          expect(dataCallback).toHaveBeenCalledWith({ foo: 'bar' });
          done();
        });
    });

    it("treats undefined arguments as missing", function(done) {
      subject.executeFunction("io.pivotal.node_gemfire.Passthrough", {})
        .on('error', function(error) {
          expect(error).toBeError('UserFunctionExecutionException',
                                  /Expected arguments; no arguments received/);
          done();
        });
    });
  });
};
