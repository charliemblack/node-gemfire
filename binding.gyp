# vim: set ft=javascript
{
  'variables': {
    'Build_Debug%': 'false'
  },
  "target_defaults": {
    "include_dirs" : [
      "$(GFCPP)/include",
      "<!(node -e \"require('nan')\")"
    ],
    'cflags_cc!': [
      '-fno-rtti', 
      '-fno-exceptions'
    ],
    "cflags": [ 
      '-std=c++11' 
      ],
    "defines": [
      "_REENTRANT"
    ],
    "libraries": [ 
      "-lpivotal-gemfire", 
      "-L$(GFCPP)/lib"
    ],
    "sources": [
      "src/dependencies.cpp",
      "src/exceptions.cpp",
      "src/conversions.cpp",
      "src/cache.cpp",
      "src/region.cpp",
      "src/select_results.cpp",
      "src/gemfire_worker.cpp",
      "src/streaming_result_collector.cpp",
      "src/result_stream.cpp",
      "src/events.cpp",
      "src/functions.cpp",
      "src/region_event_listener.cpp",
      "src/region_event_registry.cpp",
      "src/event_stream.cpp",
      "src/region_shortcuts.cpp",
      "src/cache_factory.cpp",
    ]
  },
  "targets": [
    {
      "target_name": "<(module_name)",
      "sources": [
        "src/binding.cpp"
      ]
    },
	# Remove because this will fail if nodejs source not installed
    #{
    #   "target_name": "test",
    #   "libraries": [ "-lgtest" ],
    #    "sources": [ "spec/cpp/test.cpp", ],
    #},
    {
      "target_name": "action_after_build",
      "type": "none",
      "dependencies": [ "<(module_name)" ],
      "copies": [
        {
          "files": [ "<(PRODUCT_DIR)/<(module_name).node" ],
          "destination": "<(module_path)"
        }
      ]
    },
  ]
}
