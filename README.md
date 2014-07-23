pivotal-gemfire
===============

Proof-of-concept node module connecting to Pivotal Gemfire

## Depends

- Node.js 0.10.x or 0.8.x

## Install

Install from binary:

    npm install

Install from source:

    npm install --build-from-source

## Run the build

```
npm install -g grunt-cli
grunt
```


## Developing

The [node-pre-gyp](https://github.com/mapbox/node-pre-gyp#usage) tool is used to handle building from source and packaging.

To recompile only the C++ files that have changed, first put `node-gyp` and `node-pre-gyp` on your PATH:

    export PATH=`npm explore npm -g -- pwd`/bin/node-gyp-bin:./node_modules/.bin:${PATH}

Then simply run:

    node-pre-gyp build

### Packaging

    node-pre-gyp build package

### Publishing

    npm install aws-sdk
    node-pre-gyp publish
