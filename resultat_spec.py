import os
import csv
import subprocess
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from sklearn.metrics import confusion_matrix

def process_files(directory, output_csv="results.csv", misclassified_csv="results_output/misclassified_samples_usingASTD.csv"):
    # Ouvrir les fichiers CSV
    with open(output_csv, mode="w", newline="") as file, \
         open(misclassified_csv, mode="w", newline="") as mis_file:
        
        writer = csv.writer(file)
        mis_writer = csv.writer(mis_file)
        
        # En-têtes des fichiers
        writer.writerow(["filename", "y_true", "y_pred"]) 
        mis_writer.writerow(["filename", "y_true", "y_pred"])
        
        for filename in os.listdir(directory):
            file_path = os.path.join(directory, filename)
            original_name_prefix = filename.replace("-", "_").split("_")[0].lower()
            
            # Déterminer la vraie classe
            if original_name_prefix not in ["upx", "mew", "molebox", "telock", "petite", "amber", "yoda"]:
                original_name_prefix = "unpacked"

            # Supprimer l'ancien fichier function_names.txt s'il existe
            if os.path.exists("function_names.txt"):
                os.remove("function_names.txt")
            
            # Exécuter les commandes d'analyse
            extract_command = f"python3 extractFunctionnames.py {file_path}"
            try:
                subprocess.run(extract_command, shell=True, check=True)
            except subprocess.CalledProcessError as e:
                print(f"Erreur lors de l'extraction pour {filename}: {e}")
                continue
                
            # Vérifier que function_names.txt a été créé
            if not os.path.exists("function_names.txt"):
                print(f"Fichier function_names.txt non créé pour {filename}")
                packer_name = "unpacked"
            else:
                flowastd_command = "./flow_patron_133_0 -i function_names.txt"
                result = subprocess.run(flowastd_command, shell=True, 
                                      capture_output=True, text=True)
                
                # Traiter le résultat
                output_lines = result.stdout.splitlines()
                if output_lines and output_lines[-1].startswith("c'est le packer"):
                    packer_name = output_lines[-1].split(" ")[-1].strip('"') or "unpacked"
                else:
                    packer_name = "unpacked"
                
                # Supprimer le fichier function_names.txt après utilisation
                try:
                    os.remove("function_names.txt")
                except OSError as e:
                    print(f"Erreur lors de la suppression de function_names.txt: {e}")
            
            # Écrire dans le fichier principal
            writer.writerow([filename, original_name_prefix, packer_name])
            
            # Stocker les mal classés
            if original_name_prefix != packer_name:
                mis_writer.writerow([filename, original_name_prefix, packer_name])
                print(f"Mal classé: {filename} ({original_name_prefix} -> {packer_name})")

# Exécuter l'analyse
directory_path = "./trace3"
process_files(directory_path)

# Générer la matrice de confusion
df = pd.read_csv("results.csv")
y_true = df["y_true"]
y_pred = df["y_pred"]


conf_matrix = confusion_matrix(y_true, y_pred, labels=sorted(y_true.unique()))
plt.figure(figsize=(8, 8))
sns.heatmap(conf_matrix, annot=True, fmt="d", 
            xticklabels=sorted(y_true.unique()),
            yticklabels=sorted(y_true.unique()))
plt.ylabel("Actual")
plt.xlabel("Predicted")

#plt.tight_layout()
plt.savefig("confusion_matrix.png")
plt.show()
# Générer un rapport détaillé
print("\nRapport de classification:")
print(pd.crosstab(y_true, y_pred, rownames=['Réel'], colnames=['Prédit'], margins=True))

# Afficher les fichiers mal classés
misclassified = pd.read_csv("results_output/misclassified_samples_usingASTD.csv")
print("\nFichiers mal classés:")
print(misclassified[['filename', 'y_true', 'y_pred']])