#!/bin/bash

export CONNERY_PATH=$(eval echo ~$user)"/Connery";

(cd "$CONNERY_PATH" || exit ; cd ../ && git clone --single-branch --branch release https://github.com/willcipriano/Connery.git) || true ;
(cd "$CONNERY_PATH" || exit ; make -f Buildfile local.install ; make -f Buildfile local.build && make -f Buildfile local.run)






