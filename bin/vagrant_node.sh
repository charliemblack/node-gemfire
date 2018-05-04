if ! [ -e ~/.nvm/nvm.sh ]; then
  curl -o- https://raw.githubusercontent.com/creationix/nvm/v0.33.11/install.sh | bash
fi
source ~/.nvm/nvm.sh

nvm list 0.10 || nvm install -s 0.10
nvm exec 0.10 which grunt || nvm exec 0.10 npm install -g grunt jasmine

nvm list 0.11 || nvm install -s 0.11
nvm exec 0.11 which grunt || nvm exec 0.11 npm install -g grunt jasmine

nvm list 0.12 || nvm install -s 0.12
nvm exec 0.12 which grunt || nvm exec 0.12 npm install -g grunt jasmine

nvm alias default 0.10
