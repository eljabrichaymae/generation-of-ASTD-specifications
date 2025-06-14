#!/bin/bash

#!/bin/bash
set -e  # Arrêter le script si une commande échoue

echo "===================="
echo "Extraction de fichiers"
echo "===================="

if [[ -f trace3.zip ]]; then
    unzip -x trace3.zip "__MACOSX/*"
else
    echo "Erreur : trace3.zip introuvable."
    exit 1
fi

echo "===================="
echo "Accès au dossier du projet"
echo "===================="

if [[ -d extraction_of_patterns ]]; then
    cd extraction_of_patterns
else
    echo "Erreur : dossier 'extraction_of_patterns' introuvable."
    exit 1
fi

echo "===================="
echo "Compilation en cours..."
echo "===================="

make

echo "===================="
echo "Exécution : ./programme -a"
echo "===================="

./programme -a

echo "===================="
echo "Exécution : ./programme -t"
echo "===================="

./programme -t

echo "===================="
echo "Retour au dossier parent"
echo "===================="

cd ..

echo "===================="
echo "Génération des features"
echo "===================="

python3 approximate_matching.py

echo "===================="
echo "Génération de la spécification"
echo "===================="

python3 randomTree.py

echo "===================="
echo "Génération du code C++"
echo "===================="

java -jar castd.jar -s model.json -o .

echo "===================="
echo "Génération du code C++"
echo "===================="

python3 resultat_spec.py



