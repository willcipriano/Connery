#!/bin/bash

connery_install_path=$(eval echo ~$user);
export CONNERY_INSTALL_PATH=$connery_install_path;
export CONNERY_PATH=$connery_install_path"/Connery"

pushd ./

cd "$CONNERY_INSTALL_PATH" || exit &&
git clone --single-branch --branch feature/traceback_system https://github.com/willcipriano/Connery.git


cd "$CONNERY_PATH" || exit &&
make -f Buildfile local.install ; make -f Buildfile local.build

rm Buildfile || true ;
rm CMakeCache.txt || true ;
rm -rf CMakeFiles || true ;
rm cmake_install.cmake || true ;
rm Dockerfile || true ;
rm -rf Makefile || true ;
rm -rf src || true ;

export PATH=$PATH:$CONNERY_PATH






