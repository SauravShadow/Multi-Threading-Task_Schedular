#!/bin/bash
# Rebuild the project (only compiles files that changed)
echo "🔨 Building..."
cmake --build build

# If build succeeded, run the executable
if [ $? -eq 0 ]; then
    echo "🚀 Running Task Scheduler..."
    echo "--------------------------"
    ./build/task_schedular
else
    echo "❌ Build failed!"
fi
