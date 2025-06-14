import numpy as np
import json
import glob
import csv
import os
import re
def z_array(s):
    """ Utiliser l'algorithme Z pour traiter s """
    z = [len(s)] + [0] * (len(s)-1)
    for i in range(1, len(s)):
        if s[i] == s[i-1]:
            z[1] += 1
        else:
            break
    r, l = 0, 0
    if z[1] > 0:
        r, l = z[1], 1
    for k in range(2, len(s)):
        if k > r:
            for i in range(k, len(s)):
                if s[i] == s[i-k]:
                    z[k] += 1
                else:
                    break
            r, l = k + z[k] - 1, k
        else:
            nbeta = r - k + 1
            zkp = z[k - l]
            if nbeta > zkp:
                z[k] = zkp
            else:
                nmatch = 0
                for i in range(r+1, len(s)):
                    if s[i] == s[i - k]:
                        nmatch += 1
                    else:
                        break
                l, r = k, r + nmatch
                z[k] = r - k + 1
    return z


def z_algorithm(p, t):
    """ Calculer le score de similarité en utilisant l'algorithme Z """
    combined = p + [-1] + t
    z = z_array(combined)
    score = max(z[len(p)+1:])
    return score


# Dossiers d'entrée et de sortie
results_directory = "results"
training_directory = "extraction_of_patterns/training"
output_csv = "results_output/z_array_combined"
patterns_json_output = "results_output/patterns_with_ids.json"
map_fn_path = "extraction_of_patterns/mapFn.json"



# Charger le fichier de mappage JSON
with open(map_fn_path, 'r') as map_file:
    map_fn = json.load(map_file)  # Chargement du dictionnaire de mapping

# Inverser le dictionnaire pour obtenir un mapping {numéro: fonction}
inverse_map_fn = {v: k for k, v in map_fn.items()}




# Regrouper tous les patrons dans une liste commune
all_patterns = []
for file_path in glob.glob(f"{training_directory}/*_near_supermaximals.json"):
    print(f"Chargement des motifs à partir de : {file_path}")
    with open(file_path, 'r') as file:
        patterns = json.load(file)
        all_patterns.extend(patterns)

print(f"Nombre total de patrons chargés : {len(all_patterns)}")



# Générer une version des motifs avec les noms équivalents (sans modifier les données utilisées pour le calcul)
patterns_with_equivalents = {
    f"Patron_{i+1}": [inverse_map_fn.get(value, value) for value in pattern]
    for i, pattern in enumerate(all_patterns)
}

# Sauvegarde des motifs convertis dans un fichier JSON
os.makedirs(os.path.dirname(patterns_json_output), exist_ok=True)
with open(patterns_json_output, 'w') as json_file:
    json.dump(patterns_with_equivalents, json_file, indent=4)


# Préparer le fichier CSV de sortie
os.makedirs(os.path.dirname(output_csv+".csv"), exist_ok=True)
with open(output_csv+".csv", mode='w', newline='') as csv_file:
    csv_writer = csv.writer(csv_file)

    # Écrire les en-têtes (colonnes pour chaque motif)
    headers = ["Fichier"] + [f"Patron_{i+1}" for i in range(len(all_patterns))]+["label"]
    csv_writer.writerow(headers)

    # Calculer les scores pour chaque fichier de résultats
    for target_file_path in glob.glob(f"{results_directory}/*.json"):
    #if target_file_path.endswith("_near_supermaximals.json"):
        #    continue  # Ignorer les fichiers de motifs eux-mêmes
        
        file_name = os.path.basename(target_file_path)
        ground_truth = re.findall(r'(upx|telock|petite|molebox|mew|amber|bero|yoda)', file_name.lower())
        #ground_truth = re.findall(packer, file_name.lower())
        label = ground_truth[0] if ground_truth else "unpacked"

        # Charger la séquence cible
        with open(target_file_path, 'r') as target_file:
            sequence = json.load(target_file)
        #print(sequence)
        # Calculer les scores de similarité pour tous les patrons
        similarity_scores = []
        for pat in all_patterns:
            score = z_algorithm(pat, sequence)
            similarity_scores.append(score)

        # Écrire les résultats dans le fichier CSV
        row = [os.path.basename(target_file_path)] + similarity_scores + [label]
        csv_writer.writerow(row)

print(f"Les scores Z combinés ont été enregistrés dans {output_csv}.csv.")
