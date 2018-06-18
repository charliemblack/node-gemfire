#!/bin/bash
GEMFIRE_SERVER_FILENAME="pivotal-gemfire-9.4.0.zip"
GEMFIRE_DIRECTORY="/opt/pivotal/gemfire/pivotal-gemfire-9.4.0"
GEMFIRE_LINK_DIRECTORY="/opt/pivotal/gemfire/Pivotal_GemFire"

NATIVE_CLIENT_FILENAME="pivotal-gemfire-native-9.2.0-build.10-Linux-64bit.tar.gz"
NATIVE_CLIENT_DIRECTORY="/opt/pivotal/gemfire/pivotal-gemfire-native"
NATIVE_LINK_DIRECTORY="/opt/pivotal/gemfire/NativeClient"

JAVA_RPM_FILENAME="jdk-8u171-linux-x64.rpm"
JAVA_RPM_URL="http://download.oracle.com/otn-pub/java/jdk/8u171-b11/512cd62ec5174c3487ac17c61aaa89e8/$JAVA_RPM_FILENAME"

set -e

echo "Setting up Centos 6.5"

if ! yum -C repolist | grep epel ; then
  rpm --import https://fedoraproject.org/static/0608B895.txt
  rpm -Uvh http://dl.fedoraproject.org/pub/epel/6/x86_64/epel-release-6-8.noarch.rpm
fi

yum -y install \
  gcc-c++ \
  gdb \
  git \
  gperftools \
  gtest \
  gtest-devel \
  htop \
  libyaml \
  man \
  openssl-devel \
  sqlite-devel \
  unzip \
  valgrind \
  wget \
  yum-utils \
  yum-plugin-auto-update-debug-info.noarch

if [ ! -d $GEMFIRE_DIRECTORY ]; then
  if [ ! -e /vagrant/tmp/$GEMFIRE_SERVER_FILENAME ]; then
    echo "----------------------------------------------------"
    echo "Please download $GEMFIRE_SERVER_FILENAME"
    echo "from https://network.pivotal.io/products/pivotal-gemfire"
    echo "(Pivotal GemFire v9.5.0)"
    echo "and place it in the ./tmp subdirectory of node-gemfire."
    echo "Then re-run \`vagrant provision\`."
    echo "----------------------------------------------------"
    exit 1
  fi
  mkdir -p /opt/pivotal/gemfire > /dev/null 2>&1

  cd /opt/pivotal/gemfire
  unzip /vagrant/tmp/$GEMFIRE_SERVER_FILENAME
fi

if [ ! -e $NATIVE_CLIENT_DIRECTORY ]; then
  if [ ! -e /vagrant/tmp/$NATIVE_CLIENT_FILENAME ]; then
    echo "----------------------------------------------------"
    echo "Please download $NATIVE_CLIENT_FILENAME"
    echo "from https://network.pivotal.io/products/pivotal-gemfire"
    echo "(Pivotal GemFire Native Client Linux 64bit v9.2.0.0)"
    echo "and place it in the ./tmp subdirectory of node-gemfire."
    echo "Then re-run \`vagrant provision\`."
    echo "----------------------------------------------------"
    exit 1
  fi
  cd /opt/pivotal/gemfire
  tar zxvf /vagrant/tmp/$NATIVE_CLIENT_FILENAME
  chmod +x ${GEMFIRE_DIRECTORY}/bin/*
fi

if [ ! -e /usr/bin/javac ]; then
  if [ ! -e /vagrant/tmp/$JAVA_RPM_FILENAME ]; then
    wget --no-verbose -O /vagrant/tmp/$JAVA_RPM_FILENAME --no-check-certificate --no-cookies --header "Cookie: oraclelicense=accept-securebackup-cookie" $JAVA_RPM_URL
  fi
  rpm -ivh /vagrant/tmp/$JAVA_RPM_FILENAME
fi

sh -c "cat > /etc/profile.d/gfcpp.sh" <<EOF
if [ ! -e $NATIVE_LINK_DIRECTORY ]; then
  ln -sf $NATIVE_CLIENT_DIRECTORY $NATIVE_LINK_DIRECTORY
fi
if [ ! -e $GEMFIRE_LINK_DIRECTORY ]; then
  ln -sf $GEMFIRE_DIRECTORY $GEMFIRE_LINK_DIRECTORY
fi
export GFCPP=$NATIVE_LINK_DIRECTORY
export GEMFIRE=$GEMFIRE_LINK_DIRECTORY
EOF
sh -c "cat >> /etc/profile.d/gfcpp.sh" <<'EOF'
export JAVA_HOME=/usr/java/default
export PATH=$GEMFIRE/bin:$GFCPP/bin:/usr/local/bin:$PATH
export LD_LIBRARY_PATH=$GFCPP/lib:$LD_LIBRARY_PATH
EOF

wget --no-verbose https://raw.githubusercontent.com/google/styleguide/gh-pages/cpplint/cpplint.py -O /usr/local/bin/cpplint.py
chmod +x /usr/local/bin/cpplint.py

if [ ! -e /vagrant/tmp/gppfs-0.2 ]; then
  if [ ! -e /vagrant/tmp/gppfs-0.2.tar.bz2 ]; then
    wget --no-verbose -O /vagrant/tmp/gppfs-0.2.tar.bz2 http://www.joachim-reichel.de/software/gppfs/gppfs-0.2.tar.bz2
  fi
  cd /vagrant/tmp
  tar jxf gppfs-0.2.tar.bz2
fi
