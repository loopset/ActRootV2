#!/bin/sh

# Self locate script when sourced
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# And now export
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:"${SCRIPT_DIR}/build"
export ROOT_INCLUDE_PATH=${ROOT_INCLUDE_PATH}:"${SCRIPT_DIR}/inc"
