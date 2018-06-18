#!/bin/sh

echo *********************************************
echo ****** Installing Node GemFire/Bundler ******
echo *********************************************

set -e

cd /vagrant
npm install
gem install bundler
rbenv rehash
bundle install

# debug
npm install -g node-inspect

if [ ! -d "tmp/gemfire" ]; then
  mkdir tmp/gemfire
fi

echo ""
echo ""
echo "Ready to go! Run the following to get started:"
echo ""
echo "$ vagrant ssh"
echo "$ cd /vagrant"
echo "$ grunt"
