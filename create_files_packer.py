import os
import shutil
import random

train_dir = "training"
data_dir = "data"  # dossier parent pour stocker les groupes

# Créer le dossier data s'il n'existe pas
os.makedirs(data_dir, exist_ok=True)

files = [f for f in os.listdir(train_dir) if os.path.isfile(os.path.join(train_dir, f))]
groups = {}
for f in files:
    print(f)
    base_name = os.path.splitext(f)[0]        # nom sans extension
    prefix = base_name.split('_')[0]          # préfixe avant le premier underscore
    groups.setdefault(prefix, []).append(f)

for prefix, group_files in groups.items():
    # Ignorer les groupes avec 2 fichiers ou moins
    if len(group_files) <= 2:
        continue
    
    # Créer le dossier du groupe dans data
    group_dir = os.path.join(data_dir, prefix)
    os.makedirs(group_dir, exist_ok=True)
    
    # Choisir 5 fichiers aléatoires ou moins si le groupe contient moins de 5 fichiers
    selected_files = random.sample(group_files, min(5, len(group_files)))
    # Déplacer les fichiers vers le dossier du groupe dans data
    for f in selected_files:
        shutil.copy(os.path.join(train_dir, f), os.path.join(group_dir, f))

print("Grouping completed, small groups ignored and files moved to 'data'.")
