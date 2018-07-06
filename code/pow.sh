#!/bin/bash

# Update remote
git remote update

# Start checking
UPSTREAM=${1:-'@{u}'}
LOCAL=$(git rev-parse @)
REMOTE=$(git rev-parse "$UPSTREAM")
BASE=$(git merge-base @ "$UPSTREAM")

if [ $LOCAL = $REMOTE ]; then
	echo "No need to update"
elif [ $LOCAL = $BASE ]; then
	PLATFORMIO_BUILD_FLAGS="'-DUSE_CUSTOM_H'" ./build.sh itead-sonoff-pow
fi