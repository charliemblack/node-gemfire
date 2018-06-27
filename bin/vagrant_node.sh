if ! [ -e ~/.nvm/nvm.sh ]; then
  curl -o- https://raw.githubusercontent.com/creationix/nvm/v0.33.11/install.sh | bash
fi
source ~/.nvm/nvm.sh

echo "***********************************************************"
echo "****** Validating NVM/NPM/Grunt/Jasmine installation ******"
echo "***********************************************************"

NVM=`nvm list 8 | grep v8.11.3 | wc -l`
if [ "$NVM" != "1" ]; then
  echo "****************************"
  echo "****** Installing NVM ******"
  echo "****************************"
  nvm install -s 8 
fi 

GRUNT=`nvm exec which grunt | grep grunt | wc -l`
if [ "$GRUNT" != "1" ]; then
  echo "******************************************"
  echo "****** Installing Grunt and Jasmine ******"
  echo "******************************************"
  nvm exec 8 npm install -g grunt jasmine
fi

# For script debugging
NODE_INSPECT=`npm search node-inspect | grep "Node Inspect" | wc -l`
if [ "$NODE_INSPECT" != "1" ]; then
  echo "*************************************"
  echo "****** Installing node-inspect ******"
  echo "*************************************"
  npm install -g node-inspect
fi

#nvm list 8 || nvm install -s 8 || npm install -g npm@6
#nvm exec 8 which grunt || nvm exec 8 npm install -g grunt jasmine

nvm alias default 8
