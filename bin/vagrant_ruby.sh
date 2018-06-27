#!/bin/sh

echo "******************************************"
echo "****** Validating Ruby Installation ******"
echo "******************************************"

set -e

if [ ! -e ~/.rbenv ]; then
  git clone https://github.com/sstephenson/rbenv.git ~/.rbenv
  echo 'export PATH="$HOME/.rbenv/bin:$PATH"' >> ~/.bash_profile
  echo 'eval "$(rbenv init -)"' >> ~/.bash_profile
fi

if [ ! -e ~/.rbenv/plugins/rvm-download ]; then
  git clone https://github.com/garnieretienne/rvm-download.git ~/.rbenv/plugins/rvm-download
fi

export PATH="~/.rbenv/bin:$PATH"
eval "$(rbenv init -)"

RUBY=
if [ -f ~/.rbenv/version ]; then
  RUBY=`cat ~/.rbenv/version`
fi	
	
if [ "$RUBY" != "2.4.2" ]; then
  echo "*****************************"
  echo "****** Installing Ruby ******"
  echo "*****************************"
  rbenv download 2.4.2
  rbenv global 2.4.2
fi