#!/bin/bash

root_path=$(dirname $(readlink -f $0))

cd Server/Common && ./generate_message_grpc.sh

cd $root_path

ln -sf build/compile_commands.json

mkdir -p build && cd build && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .. && make -j4

cd $root_path

mkdir -p bin
mkdir -p bin/ChatServer
mkdir -p bin/ChatServer2
mkdir -p bin/GateServer
mkdir -p bin/StatusServer

cp -f build/Server/StatusServer/StatusServer bin/StatusServer/StatusServer
cp -f Server/StatusServer/config.ini bin/StatusServer/config.ini

cp -f build/Server/GateServer/GateServer bin/GateServer/GateServer
cp -f Server/GateServer/config.ini bin/GateServer/config.ini

cp -f build/Server/ChatServer/ChatServer bin/ChatServer/ChatServer
cp -f Server/ChatServer/config.ini bin/ChatServer/config.ini

cp -f build/Server/ChatServer/ChatServer bin/ChatServer2/ChatServer
cp -f Server/ChatServer/config2.ini bin/ChatServer2/config.ini