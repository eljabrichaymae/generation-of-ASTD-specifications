import os
import shutil
import random
import re

source_dir = "trace3"
train_dir = "training"
test_dir = "test"
train_ratio = 0.5  # proportion pour le train

os.makedirs(train_dir, exist_ok=True)
os.makedirs(test_dir, exist_ok=True)

# Lister tous les fichiers
files = [f for f in os.listdir(source_dir) if os.path.isfile(os.path.join(source_dir, f))]

# Regrouper par préfixe (avant '_' ou '-')
groups = {}
for f in files:
    base_name = os.path.splitext(f)[0]
    prefix = base_name.split('_')[0] 
    groups.setdefault(prefix, []).append(f)

# Identifier les fichiers qui n'appartiennent qu'à un seul fichier (préfixe unique)
singletons = [files[0] for files in groups.values() if len(files) == 1]

# Supprimer les singletons du dictionnaire des groupes
groups = {k: v for k, v in groups.items() if len(v) > 1}

# Ajouter le groupe spécial pour les fichiers uniques
if singletons:
    groups["ungrouped"] = singletons

# Répartir tous les groupes (y compris "ungrouped") en train et test
for prefix, group_files in groups.items():
    random.shuffle(group_files)
    
    num_train = max(1, int(len(group_files) * train_ratio))
    train_files = group_files[:num_train]
    test_files = group_files[num_train:]
    
    for f in train_files:
        shutil.move(os.path.join(source_dir, f), os.path.join(train_dir, f))
    for f in test_files:
        shutil.move(os.path.join(source_dir, f), os.path.join(test_dir, f))

print("All files, including ungrouped ones, have been split into training and test sets.")
num_train_files = len(os.listdir(train_dir))
num_test_files = len(os.listdir(test_dir))

print(f"Number of files in training: {num_train_files}")
print(f"Number of files in test: {num_test_files}")
