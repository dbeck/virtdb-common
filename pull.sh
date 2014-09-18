#!/bin/sh
git submodule update --init --recursive
git submodule update --init --remote --recursive
git checkout master
git pull --recurse-submodules 
git remote add upstream https://github.com/starschema/virtdb-common.git
git fetch origin -v
git fetch upstream -v
git merge upstream/master
git status
