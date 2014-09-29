const inspect = require('util').inspect;

// For now, we don't care if it's actually an instance of Error.
function isError(error) {
  return !!error;
}

function isErrorWithMessage(error, expectedMessage) {
  if (!isError(error)) {
    return false;
  }

  if (expectedMessage.constructor === RegExp) {
    return expectedMessage.test(error.message);
  } else {
    return error.message === expectedMessage;
  }
}

exports.toBeError = function toBeError(expectedMessage) {
  const actual = this.actual;
  const isNot = this.isNot;

  if(isNot && expectedMessage) {
    throw("not.toBeError() matcher received unexpected argument");
  }

  this.message = function message() {
    if(isNot) {
      return "Expected " + inspect(actual) + " not to be an error";
    }

    if(expectedMessage) {
      if(expectedMessage.constructor === RegExp) {
        return "Expected " + inspect(actual) + " to be an error with message matching " + inspect(expectedMessage);
      } else {
        return "Expected " + inspect(actual) + " to be an error with message " + inspect(expectedMessage);
      }
    } else {
      return "Expected " + inspect(actual) + " to be an error";
    }
  };

  if(expectedMessage) {
    return isErrorWithMessage(actual, expectedMessage);
  } else {
    return isError(actual);
  }
};
