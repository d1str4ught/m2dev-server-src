#!/bin/bash

# Navigate to build directory
cd ./build

# Configure with CMake
cmake ..

# Build the project
cmake --build .

# Navigate to bin directory
cd bin

# Copy binaries to m2dev-server
cp db /home/metin2_srv/m2dev-server/share/bin/
cp qc /home/metin2_srv/m2dev-server/share/bin/
cp game /home/metin2_srv/m2dev-server/share/bin/

# Return to project root
cd ../..

echo "Build and deployment completed successfully!"
