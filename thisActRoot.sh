#!/bin/sh

# Self locate script when sourced
if [ -n "$BASH_VERSION" ]; then
    SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
elif [ -n "$ZSH_VERSION" ]; then
    SCRIPT_DIR=$( cd -- "$( dirname -- "${0}" )" &> /dev/null && pwd )
else
    echo "thisActRoot.sh: unsupported shell to source"
    exit 1
fi

# Add to system libraries
lib="${SCRIPT_DIR}/install/lib"
if [[ "$(uname)" == "Darwin" ]]; then # MacOS
    export DYLD_LIBRARY_PATH="${DYLD_LIBRARY_PATH}:${lib}"
elif [[ "$(uname)" == "Linux" ]]; then # GNU/Linux
    export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${lib}"
else
    echo "thisActRoot.sh: unsupported system to source"
    exit 1
fi
# Add includes for root interpreter
export ROOT_INCLUDE_PATH=${ROOT_INCLUDE_PATH}:"${SCRIPT_DIR}/install/include"
# Add executables path
if [ -d ${SCRIPT_DIR}/install/bin ]; then
    export PATH=${PATH}:"${SCRIPT_DIR}/install/bin"
fi
# Export a global constant pointing to main directory
export ACTROOT=${SCRIPT_DIR}
