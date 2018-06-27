const _ = require("lodash");

module.exports = function(grunt) {
  var startServer = 'cd tmp/gemfire && gfsh run --file /vagrant/bin/startServer.gfsh';
  var ensureServerRunning = 'test -e tmp/gemfire/server/vf.gf.server.pid && ps ax | grep `cat tmp/gemfire/server/vf.gf.server.pid` | grep -qv grep && echo "Server already running..." || (' + startServer + ')';

  var startLocator = 'cd tmp/gemfire && gfsh run --file /vagrant/bin/startLocator.gfsh';
  var ensureLocatorRunning = 'test -e tmp/gemfire/locator/vf.gf.locator.pid && ps ax | grep `cat tmp/gemfire/locator/vf.gf.locator.pid` | grep -qv grep && echo "Locator already running..." || (' + startLocator + ')';

  var nodeCommand = "node";
  var postNodeCommand;

  function runNode(args) {
    return(function() {
      var command = nodeCommand + " " + args;
      if(postNodeCommand) {
        command = command + " && " + postNodeCommand;
      }
      return command;
    });
  }

  grunt.initConfig(
    {
      pkg: grunt.file.readJSON('package.json'),
      shell: {
        ci: {
          command: './bin/ci'
        },
        buildDebug: {
          command: './node_modules/.bin/node-pre-gyp --debug build'
        },
        rebuildDebug: {
          command: './node_modules/.bin/node-pre-gyp --debug rebuild'
        },
        buildRelease: {
          command: './node_modules/.bin/node-pre-gyp build'
        },
        rebuildRelease: {
          command: './node_modules/.bin/node-pre-gyp rebuild'
        },
        ensureServerRunning: {
          command: ensureServerRunning
        },
        startServer: {
          command: startServer
        },
        stopServer: {
          command: 'cd tmp/gemfire && gfsh run --file /vagrant/bin/stopServer.gfsh'
        },
        ensureLocatorRunning: {
          command: ensureLocatorRunning
        },
        startLocator: {
          command: startLocator
        },
        stopLocator: {
          command: 'cd tmp/gemfire && gfsh run --file /vagrant/bin/stopLocator.gfsh'
        },
        buildTestFunction: {
          command: 'cd spec/support/java/function/ && ./gradlew build',
          src: [
            "spec/support/java/function/src/**/*.java",
            "spec/support/java/function/build.gradle",
          ]
        },
        deployTestFunction: {
          command: 'cd tmp/gemfire && gfsh run --file /vagrant/bin/deployTestFunction.gfsh',
          src: [
            'tmp/gemfire/server/vf.gf.server.pid',
            'spec/support/java/function/build/libs/function.jar'
          ]
        },
        lint: {
          command: "cpplint.py --verbose=1 --linelength=110 --extensions=cpp,hpp src/*"
        },
        console: {
          command: runNode("bin/console.js"),
          options: {
            stdinRawMode: true
          }
        },
        cppUnitTests: {
          command: runNode("spec/cpp/runner.js"),
          options: {
            execOptions: {
              env: _.extend({}, process.env, {
                "GTEST_COLOR": "yes"
              })
            }
          }
        },
        jasmine: {
          command: "./node_modules/jasmine/bin/jasmine.js"
        },
        release: {
          command: "./node_modules/.bin/node-pre-gyp rebuild package testpackage publish"
        },
        licenseFinder: {
          command: "npm prune && ./bin/license_finder --quiet"
        }
      },
      jshint: {
        options: {
          jshintrc: true
        },
        all: ['Gruntfile.js', './*.js', 'spec/**/*.js']
      }
    }
  );

  grunt.loadNpmTasks('grunt-contrib-jshint');
  grunt.loadNpmTasks('grunt-shell');
  grunt.loadNpmTasks('grunt-newer');

  grunt.registerTask('build', ['shell:buildDebug']);
  grunt.registerTask('rebuild', ['shell:rebuildDebug']);
  grunt.registerTask('test', ['build', 'shell:cppUnitTests', 'server:ensure', 'server:deploy', 'shell:jasmine']);
  grunt.registerTask('lint', ['shell:lint', 'jshint']);
  grunt.registerTask('console', ['build', 'shell:console']);
  grunt.registerTask('license_finder', ['shell:licenseFinder']);

  grunt.registerTask('server:start', ['locator:ensure', 'shell:startServer']);
  grunt.registerTask('server:stop', ['shell:stopServer']);
  grunt.registerTask('server:restart', ['server:stop', 'server:start']);
  grunt.registerTask('server:ensure', ['locator:ensure', 'shell:ensureServerRunning']);

  grunt.registerTask('locator:start', ['shell:startLocator']);
  grunt.registerTask('locator:stop', ['server:stop', 'shell:stopLocator']);
  grunt.registerTask('locator:restart', ['locator:stop', 'locator:start', 'server:start']);
  grunt.registerTask('locator:ensure', ['shell:ensureLocatorRunning']);

  grunt.registerTask('server:deploy', ['newer:shell:buildTestFunction', 'newer:shell:deployTestFunction']);

  grunt.registerTask('valgrind', function() {
    grunt.log.writeln('Running with valgrind...');
    nodeCommand = "valgrind --suppressions=spec/valgrind/suppressions-gemfire-8.0.0.supp node";
  });

  grunt.registerTask('callgrind', function() {
    grunt.log.writeln('Running with callgrind...');
    nodeCommand = "valgrind --tool=callgrind node";
  });

  grunt.registerTask('gperftools', function() {
    grunt.log.writeln('Running with gperftools profiler...');
    nodeCommand = "LD_PRELOAD=/usr/lib64/libprofiler.so CPUPROFILE=tmp/cpuprofiler.out CPUPROFILE_FREQUENCY=100 CPUPROFILE_REALTIME=1 node";
    postNodeCommand = "pprof --callgrind `which node` tmp/cpuprofiler.out > callgrind.gperftools.out.$$";
  });

  grunt.registerTask('default', ['test', 'lint']);

  grunt.registerTask('ci:node', ['default']);
  grunt.registerTask('ci:other', ['license_finder']);

  grunt.registerTask('ci', ["shell:ci"]);
  grunt.registerTask('release', ['shell:release']);
};

