import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import accuracy_score, classification_report
from sklearn.metrics import precision_score, recall_score, f1_score, confusion_matrix
import seaborn as sns
import matplotlib.pyplot as plt
from sklearn.tree import plot_tree
import numpy as np
import te2rules
from te2rules.explainer import ModelExplainer

from sklearn.tree import export_text

from sklearn.tree import _tree


import glob
import re
import os



SEED = 42
N_ESTIMATORS = 1  # Augmenté pour plus de stabilité
TEST_SIZE = 0.5
N_SPLITS = 5
RANDOM_STATES_TO_TEST = 50

# Fonction pour extraire les règles pour une classe donnée
def extract_rules_for_class(tree, feature_names, target_class):
    tree_ = tree.tree_  # Accès à la structure interne
    rules = []

    def traverse(node, conditions):
        if tree_.feature[node] != _tree.TREE_UNDEFINED:  # Noeud interne
            feature = feature_names[tree_.feature[node]]
            threshold = tree_.threshold[node]

            left_conditions = conditions + [f"{feature} <= {threshold:.2f}"]
            # left_conditions = conditions + [f"{feature} <= {threshold:.2f}", f"{feature} != 0"]
            right_conditions = conditions + [f"{feature} > {threshold:.2f}"]

            traverse(tree_.children_left[node], left_conditions)
            traverse(tree_.children_right[node], right_conditions)

        else:  # Noeud feuille
            class_probs = tree_.value[node][0]  # Probabilités par classe
            predicted_class = class_probs.argmax()

            if predicted_class == target_class:  # Vérifie si c'est la classe 0
                rule = " IF " + " AND ".join(conditions) + f" THEN class = {predicted_class}"
                rules.append(rule)

    traverse(0, [])  # Commence depuis la racine
    return rules



output_directory = "results_output"





# Charger le fichier CSV
file_path = "results_output/z_array_combined.csv"  # Remplacez par le chemin de votre fichier CSV
data = pd.read_csv(file_path)

cols = list(data.columns)
feature_names = cols[1:-1]
print(feature_names)
X = data.iloc[:, 1:-1] 
y = data['label']  
filenames = data.iloc[:, 0]

# Diviser les données en train et test avec un mélange des données
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.5,shuffle=True, stratify=y,random_state=50)

# Initialiser le modèle de forêt aléatoire
clf = RandomForestClassifier(n_estimators=1,random_state=70)

# Entraîner le modèle
clf.fit(X_train, y_train)

print(clf.classes_)
labels = [str(cls) for cls in clf.classes_]
print(labels)

# y_train_pred = clf.predict(X_train)
# Prédictions
y_pred = clf.predict(X_test)

# Évaluer les performances
accuracy = accuracy_score(y_test, y_pred)
precision = precision_score(y_test, y_pred, average='weighted')
recall = recall_score(y_test, y_pred, average='weighted')
f1 = f1_score(y_test, y_pred, average='weighted')

print(f"Precision: {precision:.2f}")
print(f"Recall: {recall:.2f}")
print(f"F1-Score: {f1:.2f}")
print(f"Accuracy: {accuracy:.2f}")
#print("\nClassification Report:")


# Matrice de confusion
# labels = ["upx", "telock", "petite","molebox","mew" ,"unpacked"]
#labels = [packer_name ,"unpacked"]
conf_matrix = confusion_matrix(y_test, y_pred, labels=labels)

plt.figure(figsize=(8, 8))
sns.heatmap(conf_matrix, annot=True, fmt='d', xticklabels=labels, yticklabels=labels)
plt.ylabel("Actual")
plt.xlabel("Predicted")
plt.show()


# plt.figure(figsize=(15, 10))
# plot_tree(clf.estimators_[0], filled=True, feature_names=X.columns, class_names=labels, fontsize=6)
# plt.title("Arbre de décision du Random Forest")
# plt.show()
plt.figure(figsize=(15, 10))
num_trees_to_display = min(3, len(clf.estimators_))  # Afficher jusqu'à 3 arbres
for i in range(num_trees_to_display):  
    plt.figure(figsize=(15, 10))
    plot_tree(clf.estimators_[i], filled=True, feature_names=X.columns, class_names=labels, fontsize=3)
    plt.title(f"Arbre de décision {i+1} du Random Forest")
    plt.show()
#print(len(clf.estimators_))
print(clf.estimators_[0])
os.makedirs("rules", exist_ok=True)
for i in range(len(labels)):
    with open(f"rules/rules_{labels[i]}.txt", "w", encoding="utf-8") as file:
            if labels[i] in clf.classes_:
                packer_class_index = np.where(clf.classes_ == labels[i])[0][0]
                print(packer_class_index)
            else:
                print(f"⚠️ Avertissement : La classe '{labels[i]}' n'a pas été trouvée dans clf.classes_.")
                packer_class_index = 0

            # Extraire les règles pour la classe 0 de cet arbre
            rules_for_class_0 = extract_rules_for_class(clf.estimators_[0], list(X.columns), target_class=packer_class_index)

            #file.write(f"\n🔍 Règles de l'arbre {i+1} pour la classe 0:\n\n")
            for j, rule in enumerate(rules_for_class_0, 1):
                file.write(f"Règle {j}: {rule}\n")

tree_text = export_text(clf.estimators_[0], feature_names=list(X.columns))
print(tree_text)



# Prédictions
y_pred = clf.predict(X)

# Évaluer les performances
accuracy = accuracy_score(y, y_pred)
precision = precision_score(y, y_pred, average='weighted')
recall = recall_score(y, y_pred, average='weighted')
f1 = f1_score(y, y_pred, average='weighted')

print(f"Precision: {precision:.2f}")
print(f"Recall: {recall:.2f}")
print(f"F1-Score: {f1:.2f}")
print(f"Accuracy: {accuracy:.2f}")
#print("\nClassification Report:")


# Matrice de confusion
# labels = ["upx", "telock", "petite","molebox","mew" ,"unpacked"]
#labels = [packer_name ,"unpacked"]
conf_matrix = confusion_matrix(y, y_pred, labels=labels)

plt.figure(figsize=(8, 8))
sns.heatmap(conf_matrix, annot=True, fmt='d', xticklabels=labels, yticklabels=labels)
plt.ylabel("Actual")
plt.xlabel("Predicted")
plt.show()

# Trouver les indices des erreurs de classification
misclassified_indices = np.where(y != y_pred)[0]

# Extraire les exemples mal classés avec leur nom de fichier
misclassified_samples = X.iloc[misclassified_indices].copy()
misclassified_samples['filename'] = filenames.iloc[misclassified_indices].values
misclassified_samples['true_label'] = y.iloc[misclassified_indices].values
misclassified_samples['predicted_label'] = y_pred[misclassified_indices]

# Réorganiser les colonnes pour lisibilité
cols_order = ['filename'] + ['true_label', 'predicted_label']
misclassified_samples = misclassified_samples[cols_order]

# Afficher les premières erreurs
print("\n🔍 Exemples mal classés :")
print(misclassified_samples.head())

# Sauvegarder dans un fichier CSV
misclassified_samples.to_csv("misclassified_samples.csv", index=False)

print(f"\n💾 Misclassifications sauvegardées dans 'results_output/misclassified_samples.csv'")
