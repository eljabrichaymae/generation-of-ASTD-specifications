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



# --- Dossiers ---
# --- Dossiers ---
results_root = "results"
training_results_dir = "training"
test_results_dir = "test"
training_directory = "data"  # motifs
patterns_json_output = "results_output/patterns_with_ids.json"
map_fn_path = "extraction_of_patterns/mapFn.json"

# --- Charger mapFn ---
with open(map_fn_path, 'r') as f:
    map_fn = json.load(f)
inverse_map_fn = {v: k for k, v in map_fn.items()}

# --- Charger tous les motifs depuis training_directory ---
all_patterns = []
for file_path in glob.glob(f"{training_directory}/*_near_supermaximals.json"):
    print(f"Chargement des motifs à partir de : {file_path}")
    with open(file_path, 'r') as file:
        patterns = json.load(file)
        all_patterns.extend(patterns)
print(f"Nombre total de patrons chargés : {len(all_patterns)}")

# --- Sauvegarder motifs avec équivalents ---
patterns_with_equivalents = {
    f"Patron_{i+1}": [inverse_map_fn.get(v, v) for v in pattern]
    for i, pattern in enumerate(all_patterns)
}
os.makedirs(os.path.dirname(patterns_json_output), exist_ok=True)
with open(patterns_json_output, 'w') as json_file:
    json.dump(patterns_with_equivalents, json_file, indent=4)

# --- Fonction pour générer CSV ---
def generate_csv(subdir, output_csv_path):
    os.makedirs(os.path.dirname(output_csv_path), exist_ok=True)
    with open(output_csv_path, 'w', newline='') as csv_file:
        csv_writer = csv.writer(csv_file)
        headers = ["Fichier"] + [f"Patron_{i+1}" for i in range(len(all_patterns))] + ["label"]
        csv_writer.writerow(headers)

        for target_file_path in glob.glob(f"{subdir}/*.json"):
            file_name = os.path.basename(target_file_path)
            ground_truth = re.findall(r'(upx|telock|petite|molebox|mew|amber|bero|yoda)', file_name.lower())
            label = ground_truth[0] if ground_truth else "unpacked"

            sequence = []
            try:
                with open(target_file_path, 'r', errors='ignore') as f:
                    for line in f:
                        try:
                            data = json.loads(line)
                            func_name = data.get('FunctionName') or data.get('Functionname')
                            if func_name:
                                # Map function name to integer using map_fn
                                # If not found, use -1 (or skip? -1 is safer to avoid false matches)
                                seq_val = map_fn.get(func_name, -1)
                                sequence.append(seq_val)
                        except json.JSONDecodeError:
                            continue # Skip invalid lines
            except Exception as e:
                print(f"Error reading {target_file_path}: {e}")
                continue

            if not sequence:
                print(f"Warning: Empty sequence for {file_name}")
                # Should we skip or write row with 0s?
                # Writing row with 0s is probably safer than crashing or skipping
                similarity_scores = [0] * len(all_patterns)
            else:
                similarity_scores = [z_algorithm(pat, sequence) for pat in all_patterns]
            
            row = [file_name] + similarity_scores + [label]
            csv_writer.writerow(row)
    print(f"CSV généré : {output_csv_path}")

# --- Générer CSV pour training et test ---
generate_csv(training_results_dir, "results_output/z_array_training.csv")
generate_csv(test_results_dir, "results_output/z_array_test.csv")
