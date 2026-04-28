#!/bin/bash
CPU=6502
SYNTAX=oldstyle
PROGRAM=vasm${CPU}_${SYNTAX}

# Install vasm in .local directory
pushd ~/.local

# Download and extract vasm daily snapshot
wget http://sun.hasenbraten.de/vasm/daily/vasm.tar.gz
tar -xzf vasm.tar.gz

# Compile program
pushd vasm
make CPU=$CPU SYNTAX=$SYNTAX

# Copy executable to .local/bin
cp $PROGRAM ~/.local/bin

# Check if installed correctly
if command -v $PROGRAM >/dev/null 2>&1; then
    echo "${PROGRAM} is available"
else
    echo "Adding ${PROGRAM} to PATH"
cat <<'EOF' >> ~/.profile
if [ -d "$HOME/.local/bin" ] ; then
    PATH="$HOME/.local/bin:$PATH"
fi
EOF
``
fi
popd

# Remove build files
rm -rf vasm*

popd

# Check if installed correctly
source ~/.profile
if command -v $PROGRAM >/dev/null 2>&1; then
    echo "${PROGRAM} is available"
    exit 0
else
    echo "${PROGRAM} is NOT found on PATH"
    exit -1
fi
