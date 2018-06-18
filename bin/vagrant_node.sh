echo **********************************************
echo ****** Installing NVM/NPM/Grunt/Jasmine ******
echo **********************************************

if ! [ -e ~/.nvm/nvm.sh ]; then
  curl -o- https://raw.githubusercontent.com/creationix/nvm/v0.33.11/install.sh | bash
fi
source ~/.nvm/nvm.sh

nvm list 8 || nvm install -s 8 || npm install -g npm@6
nvm exec 8 which grunt || nvm exec 8 npm install -g grunt jasmine

# nvm list 10 || nvm install -s 10
# nvm exec 10 which grunt || nvm exec 10 npm install -g grunt jasmine

nvm alias default 8
