#!/bin/bash
set -e  # Stop the script if a command fails

echo "===================="
echo "Extracting files"
echo "===================="

if [[ -f trace3.zip ]]; then
    unzip -x trace3.zip "__MACOSX/*"
else
    echo "Error: trace3.zip not found."
    exit 1
fi

echo "===================="
echo "Accessing the project folder"
echo "===================="

if [[ -d extraction_of_patterns ]]; then
    cd extraction_of_patterns
else
    echo "Error: 'extraction_of_patterns' folder not found."
    exit 1
fi

echo "===================="
echo "Compiling..."
echo "===================="

make

echo "===================="
echo "Running: ./programme -a"
echo "===================="

./programme -a

echo "===================="
echo "Running: ./programme -t"
echo "===================="

./programme -t

echo "===================="
echo "Returning to parent directory"
echo "===================="

cd ..

echo "===================="
echo "Generating features"
echo "===================="

python3 feature_generation.py

echo "===================="
echo "Generating the specification"
echo "===================="

python3 decision_tree_translation.py

echo "===================="
echo "Generating C++ code"
echo "===================="

java -jar castd.jar -s model.json -o .

echo "===================="
echo "Classification result using the ASTD specification"
echo "===================="

python3 resultat_spec.py



