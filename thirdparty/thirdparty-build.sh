#!/bin/bash


echo "BUILDING JSONCPP..."

cd ./json
tar -xvf jsoncpp-src-0.6.0.tar.gz
rm -rf jsoncpp-src
mv jsoncpp-src-0.6.0 jsoncpp-src
cp scons-local-2.1.0.tar.gz jsoncpp-src
cd jsoncpp-src
tar -xvf scons-local-2.1.0.tar.gz
python scons.py platform=linux-gcc

if [ ! -d "build" ]; then
    mkdir build
fi
cd build
cmake ..
make

echo "BUILDING LIBEVENT..."

cd ../../../event
tar -xvf libevent-2.0.21-stable.tar.gz
cd libevent-2.0.21-stable

CURRENTDIR=$(pwd)
echo "INSTALLING LIBEVENT in $CURRENTDIR/../"

./configure --prefix=$CURRENTDIR/../
make install

cd ../../libunwind
tar -xvf libunwind-0.99-beta.tar.gz
cd libunwind-0.99-beta

CURRENTDIR=$(pwd)
echo "INSTALLING libunwind in $CRRENTDIR/../"
CFLAGS=-fPIC ./configure --prefix=$CURRENTDIR/../
make CFLAGS=-fPIC install

cd ../../gperftools
tar -xvf gperftools-2.0.tar.gz
cd gperftools-2.0

CURRENTDIR=$(pwd)
echo "INSTALLING google perftools in $CURRENTDIR/../"
LDFLAGS=-L$CURRENTDIR/../../libunwind/lib/ ./configure --prefix=$CURRENTDIR/../
make && make install

cd ../../mongo-cxx-driver
tar -xvf mongo-cxx-driver-legacy-0.0-26compat-2.6.2.tar.gz
rm -rf mongo-cxx-driver
mv mongo-cxx-driver-legacy-0.0-26compat-2.6.2 mongo-cxx-driver
cd mongo-cxx-driver
CURRENTDIR=$(pwd)
echo "Building mongodb driver in $CURRENTDIR"

if [ "$(uname)" == "Darwin" ]; then
    echo "Building mongodb driver under MAC_OS"
    python ../../json/jsoncpp-src/scons.py --osx-version-min=10.7 --use-system-boost --sharedclient --full install-mongoclient 
else
    echo "Building mongodb driver under LINUX"
    python ../../json/jsoncpp-src/scons.py --prefix=srch2 --use-system-boost --sharedclient --full install-mongoclient
fi


cd ../../pymongo
tar -xvf pymongo.tar.gz
cd pymongo
CURRENTDIR=$(pwd)
echo "Building python mongodb driver for mongodb system test in $CURRENTDIR"

cd ../../mysql-connector-c++
echo "BUILDING MYSQL CONNECTOR C++... in $CURRENTDIR"
tar -xvf mysql-connector-c++-1.1.4.tar.gz
rm -rf mysql-connector-c++
mv mysql-connector-c++-1.1.4 mysql-connector-c++
cd mysql-connector-c++
cmake -DCMAKE_INSTALL_PREFIX=./build
make install
mv ./build/lib/*/* ./build/lib/

cd ../../mysql-connector-c++
echo "BUILDING MySQL replication listener... in $CURRENTDIR"
rm -rf mysql-replication-listener
tar -xvf mysql-replication-listener.tar.gz
cd mysql-replication-listener
mkdir build
cd build
cmake ..
make

cd ../../../mysql-connector-c++
echo "BUILDING MySQL python driver for MySQL system test... in $CURRENTDIR"
tar -xvf MySQL-python-1.2.5.tar.gz
rm -rf MySQL-python
mv MySQL-python-1.2.5 MySQL-python
cd MySQL-python
python setup.py build

