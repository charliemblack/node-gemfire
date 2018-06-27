#!/bin/sh

set -e

echo "*********************************************************"
echo "****** Validating Node GemFire/Ruby Bundler/Bundle ******"
echo "*********************************************************"

cd /vagrant

if [ ! -d "tmp/gemfire" ]; then
  mkdir tmp/gemfire
fi

if [ ! -d "lib/binding/Release/node-v57-linux-x64" ]; then
  echo "*************************************"
  echo "****** Installing Node GemFire ******"
  echo "*************************************"
  npm install-test
fi

BUNDLER=`gem list | grep "bundler (1.16.2)" | wc -l`
if [ "$BUNDLER" != "1" ]; then 
  echo "********************************"
  echo "****** Installing Bundler ******"
  echo "********************************"
  gem install bundler
fi  
  
rbenv rehash

BUNDLE=`bundle check | grep "dependencies are satisfied" | wc -l`
if [ "$BUNDLE" != "1" ]; then
  echo "*******************************"
  echo "****** Installing bundle ******"
  echo "*******************************"
  bundle install
fi

NPM=`npm version | grep "npm: '6.1.0'" | wc -l`
if [ "$NPM" != "1" ]; then
  echo "****************************"
  echo "****** Installing NPM ******"
  echo "****************************"
  npm install -g npm@6
fi 

echo ""
echo ""
echo "Ready to go! Run the following to get started:"
echo ""
echo "$ vagrant ssh"
echo "$ cd /vagrant"
echo "$ grunt"
