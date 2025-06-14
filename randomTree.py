import pandas as pd
from sklearn.model_selection import train_test_split, GridSearchCV  # ← MODIFICATION
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
from sklearn.tree import DecisionTreeClassifier
import json
import re
import glob
from collections import defaultdict
import math

SEED = 42
TEST_SIZE = 0.5
RANDOM_STATES_TO_TEST = 42



def constructDFA(P, alphabet):
    m = len(P)
    delta = [{} for _ in range(m + 1)]  # delta[q][c] = next state from q on symbol c

    # Initialisation
    for c in alphabet:
        delta[0][c] = 0
    delta[0][P[0]] = 1

    x = 0  # état de rappel

    for j in range(1, m):
        # Copier les transitions depuis l'état x vers l'état j
        for c in alphabet:
            delta[j][c] = delta[x][c]

        # Définir la transition correcte correspondant à P[j]
        delta[j][P[j]] = j + 1

        # Mettre à jour x
        x = delta[x][P[j]]

    return delta

def convert_dfa_to_transitions(dfa):
    transitions_list = []
    for src_state, transitions in enumerate(dfa):
        for symbol, dest_state in transitions.items():
            transitions_list.append({
                "src_state": src_state,
                "dest_state": dest_state,
                "symbol": symbol
            })
    return transitions_list

def process_packer_rules(rules_directory, json_file):
    rules_thresholds = []
    packer_global_conditions = {}
    packer_var_names = {}  # Dictionnaire pour stocker les var_name par packer

    for file_path in glob.glob(f"{rules_directory}/rules_*.txt"):
        match = re.search(r"rules_([a-zA-Z]+)\.txt", file_path)
        if not match:
            print(f"⚠️ Avertissement : Impossible d'extraire le nom du packer du fichier {file_path}. Ignorer ce fichier.")
            continue
        
        packer_name = match.group(1)
        print(f"\n{'#'*40}\nTraitement du packer : {packer_name}\n{'#'*40}")
            
        try:
            with open(file_path, "r", encoding="utf-8") as file:
                rules_text = file.read().strip()
        except FileNotFoundError:
            print(f"❌ Erreur : Fichier '{file_path}' introuvable.")
            exit(1)

        try:
            with open(json_file, "r", encoding="utf-8") as file:
                patrons_data = json.load(file)
        except FileNotFoundError:
            print(f"❌ Erreur : Fichier '{json_file}' introuvable.")
            exit(1)

        rule_lines = [line.strip() for line in rules_text.split('\n') if line.strip().startswith("Règle")]
        threshold_pattern = r"(Patron_\d+)\s*([<>]=?|==)\s*([\d.]+)"
        
        packer_rules = []
        packer_conditions = []  # 👈 global conditions pour ce packer

        # Initialiser la liste des var_name pour ce packer
        packer_var_names[packer_name] = []

        for rule_line in rule_lines:
            rule_id_match = re.search(r"Règle\s+(\d+):", rule_line)
            rule_id = rule_id_match.group(1) if rule_id_match else "unknown"

            rule_thresholds = {}

            for patron, operator, value in re.findall(threshold_pattern, rule_line):
                k = math.ceil(float(value))
                if patron not in rule_thresholds:
                    rule_thresholds[patron] = []
                rule_thresholds[patron].append({
                    'operator': operator,
                    'value': float(value),
                    'threshold': k,
                    'k_elements': patrons_data.get(patron, [])[:k]
                })

            rule_info = {
                'packer_name': packer_name,
                'rule_id': int(rule_id),
                'thresholds': rule_thresholds
            }

            condition_clauses = []
            for patron, entries in rule_thresholds.items():
                for entry in entries:
                    op = entry['operator']
                    var_name = f"{packer_name}_R{rule_id}_{patron}"
                    
                    # Ajouter le var_name à la liste pour ce packer
                    packer_var_names[packer_name].append(var_name)
                    
                    if op == ">":
                        condition = f"{var_name} == 1"
                    elif op == "<=":
                        condition = f"{var_name} == 0"
                    else:
                        condition = f"# unsupported operator for {var_name}"
                    condition_clauses.append(condition)

            global_condition = " && ".join(condition_clauses)
            rule_info["global_condition"] = global_condition

            packer_rules.append(rule_info)
            packer_conditions.append(f"({global_condition})")  # on l’ajoute ici

        rules_thresholds.extend(packer_rules)

        # Créer la condition globale pour le packer (join avec ||)
        packer_global_conditions[packer_name] = " || ".join(packer_conditions)

    return rules_thresholds, packer_global_conditions, packer_var_names




def generate_astd_automaton_for_rules(rules_thresholds):
    """
    Generates an ASTD automaton for each rule in the rules and their patterns,
    grouped by packer.

    Args:
        rules_thresholds (list): List of rule information containing patterns and thresholds.
    
    Returns:
        dict: A dictionary where the key is the packer name, and the value is a list of ASTD automaton objects for each rule.
    """
    packer_automata = {}

    for rule_info in rules_thresholds:
        packer_name = rule_info['packer_name']
        rule_id = rule_info['rule_id']
        
        # Initialize the list for this packer if not already created
        if packer_name not in packer_automata:
            packer_automata[packer_name] = []

        # For each pattern in the rule, generate the transitions and construct the ASTD automaton
        for patron, entries in rule_info['thresholds'].items():
            # Assuming k_elements contains the pattern
            pattern = entries[0]['k_elements']
            k = entries[0]['threshold']
            
            # Generate transitions list using the KMP automaton for this pattern
            transitions = []
            for entry in entries:
                kmp_automaton = constructDFA(pattern,set(pattern))
                # kmp_transitions = kmp_automaton.transition_table
                transitions.extend(convert_dfa_to_transitions(kmp_automaton))
            
            # Create the ASTD automaton for this rule and pattern
            astd_automaton = generate_ASTD_automaton(packer_name, rule_id, patron, transitions, k, pattern)
            packer_automata[packer_name].append(astd_automaton)
    return packer_automata



def generate_end_automaton(packer_name , patron_name, transitions, conditions):
    states = [{"state_id": i, "is_entry": "True" if i == 0 else "False", "is_final": "True" if i == len(transitions) else "False"} 
            for i in range(len(transitions) + 1)]
    #invariant_conditions = packer_global_conditions[packer_name]
    transitions_list = [{"src_state": 0, "dest_state": 0, "symbol": transitions[0]}]
    transitions_list[0]["action"] = f'{{ if ({conditions}) {{ std::cout << "c\'est le packer {packer_name}" << std::endl; }} }}'
    return {
        "name": str(patron_name)+"_"+str(packer_name),
        "type": "Automaton",
        "typed_astd": {
            "attributes": [],
            "code": "",
            "interruptCode": "",
            "states": [{"name": f"S{state['state_id']}", 
                        "astd": {
                            "type": "Elem", 
                            "typed_astd": {}
                            }, 
                        "entry_code": "",
                        "stay_code": "", 
                        "exit_code": "", 
                        "invariant": ""} 
                    for state in states],
            "transitions": [{"arrow_type": "Local", 
                            "arrow": {
                                "from_state_name": f"S{trans['src_state']}", 
                                "to_state_name": f"S{trans['dest_state']}"},
                                "event_template": 
                                {
                                    "label": trans['symbol'], 
                                    "parameters": [], 
                                    "when": []
                                },
                            "guard": "", 
                            "action": "{"+trans["action"]+"}", 
                            "step": False, 
                            "from_final_state_only": False} 
                            for trans in transitions_list],
            "shallow_final_state_names": ["S0"],
            "deep_final_state_names": [],
            "initial_state_name": "S0"
        }
    }


def generate_ASTD_automaton(pattern_name,transitions,k,pattern_list):
    states = [{"state_id": i, "is_entry": "True" if i == 0 else "False", "is_final": "True" if i == k+1 else "False"} 
                for i in range(k + 1)]
    
    for trans in transitions:
        if trans["src_state"] == k-1 and trans["dest_state"] == k:
            trans["action"] = f"occ_{pattern_name}=1"
    guard_conditions = " && ".join(f'x!="{symbol}"' for symbol in pattern_list)
    guard = f"{guard_conditions}"
    return {
                "name": "Aut1_"+pattern_name,
                "parameters": [],
                "type": "Automaton",
                "invariant": "",
                "typed_astd": {
                    "attributes": [],
                    "code": "",
                    "interruptCode": "",
                    "states": [
                        {
                            "name": f"Aut2_{pattern_name}",
                            "astd": {
                                "name":  f"Aut2_{pattern_name}",
                                "type": "Automaton",
                                "invariant": "",
                                "typed_astd": {
                                    "attributes": [],
                                    "code": "",
                                    "interruptCode": "",
                                    "states": [
                                        {
                                            "name": f"S{j}",
                                            "astd": {
                                                "type": "Elem",
                                                "typed_astd": {}
                                            },
                                            "entry_code": "",
                                            "stay_code": "",
                                            "exit_code": "", 
                                            "invariant": ""
                                        }
                                        for j in range(len(states))
                                    ],
                                    "transitions": [
                                        {
                                            "arrow_type": "Local",
                                            "arrow": {
                                                "from_state_name": f"S{trans['src_state']}",
                                                "to_state_name": f"S{trans['dest_state']}"
                                            },
                                            "event_template": {
                                                "label": "e",
                                                "parameters": [
                                                    {
                                                        "parameter_kind": "Capture",
                                                        "parameter": {
                                                            "variable_name": "x",
                                                            "type": "string"
                                                        }
                                                    }
                                                ],
                                                "when": []
                                            },
                                            "guard": f"x==\"{trans["symbol"]}\"",
                                            "action": trans.get("action", ""),
                                            "step": False,
                                            "from_final_state_only": False
                                        }
                                        for trans in transitions
                                    ],
                                    "shallow_final_state_names": [f"S{k}"],
                                    "deep_final_state_names": [],
                                    "initial_state_name": "S0"
                                }
                            },
                            "entry_code": "",
                            "stay_code": "",
                            "exit_code": ""
                        }
                    ],
                    "transitions": [
                        {
                            "arrow_type": "Local",
                            "arrow": {
                                "from_state_name": f"Aut2_{pattern_name}",
                                "to_state_name": f"Aut2_{pattern_name}"
                            },
                            "event_template": {
                                "label": "e",
                                "parameters": [
                                    {
                                        "parameter_kind": "Capture",
                                        "parameter": {
                                            "variable_name": "x",
                                            "type": "string"
                                        }
                                    }
                                ],
                                "when": []
                            },
                            "guard": guard,  # Garde paramétrée
                            "action": "",
                            "step": False,
                            "from_final_state_only": False
                        }
                    ],
                    "shallow_final_state_names": [],
                    "deep_final_state_names": [],
                    "initial_state_name": f"Aut2_{pattern_name}"
                }
            
        }

def generate_flow(name, left_astd, right_astd, patron_names=None):
        """
        Génère un flow qui connecte deux automates et ajoute un attribut de forme occ_nom_du_patron
        pour chaque patron dans le flux.
        """
        flow_data = {
            "name": name,
            "type": "Flow",
            "invariant": "",
            "typed_astd": {
                "attributes": [],
                "code": "",
                "interruptCode": "",
                "left_astd": left_astd,
                "right_astd": right_astd
            }
        }
        return flow_data

def generate_flow(name, left_astd, right_astd, patron_names=None):
        """
        Génère un flow qui connecte deux automates et ajoute un attribut de forme occ_nom_du_patron
        pour chaque patron dans le flux.
        """
        flow_data = {
            "name": name,
            "type": "Flow",
            "invariant": "",
            "typed_astd": {
                "attributes": [],
                "code": "",
                "interruptCode": "",
                "left_astd": left_astd,
                "right_astd": right_astd
            }
        }
        return flow_data


# Fonction pour extraire les règles pour une classe donnée
def extract_rules_for_class(tree,classes):
    tree_ = tree.tree_
    attributes = set()
    json_file = "results_output/patterns_with_ids.json"
    try:
        with open(json_file, "r", encoding="utf-8") as file:
            patrons_data = json.load(file)
    except FileNotFoundError:
        print(f"❌ Erreur : Fichier '{json_file}' introuvable.")
        exit(1)
    def traverse(node, conditions,is_root=False):
        nonlocal attributes
        if tree_.feature[node] != _tree.TREE_UNDEFINED:
            feature = feature_names[tree_.feature[node]]
            threshold = tree_.threshold[node]
            k = math.ceil(float(threshold))
            pattern = patrons_data.get(feature, [])[:k]
            transitions = []
            kmp_automaton = constructDFA(pattern,set(pattern))
            # kmp_transitions = kmp_automaton.transition_table
            transitions.extend(convert_dfa_to_transitions(kmp_automaton))
            astd_automaton = generate_ASTD_automaton(feature+"_"+str(node), transitions, k, set(pattern))
        
            left_conditions = conditions + [f"occ_{feature}_{node} == 0"]
            right_conditions = conditions + [f"occ_{feature}_{node} == 1"]
            attr_name = f"occ_{feature}_{node}"
            attributes.add(attr_name)

            left_automaton = traverse(tree_.children_left[node], left_conditions)
            right_automaton = traverse(tree_.children_right[node], right_conditions)

            current_flow = generate_flow(
                name=f"Flow_{feature}_{node}",
                left_astd=astd_automaton,
                right_astd=generate_flow(
                    name=f"SubFlow_{node}",
                    left_astd=left_automaton,
                    right_astd=right_automaton
                )
            )
            if is_root:
                current_flow["typed_astd"]["attributes"] = [
                    {"name": attr, "type": "int", "initial_value": 0}
                    for attr in sorted(attributes)
                ]
            
            return current_flow

        else:
            class_probs = tree_.value[node][0]
            predicted_class = class_probs.argmax()
            packer_name = classes[predicted_class]
            nouveau_patron = f"patron_end_{node}"
            nouvelles_transitions = ["end"] 
           
            end_automaton = generate_end_automaton(packer_name,nouveau_patron,nouvelles_transitions," && ".join(conditions))
            return end_automaton
            
    return traverse(0, [], is_root=True)
    






# # Charger les données
# file_path = "results_output/z_array_combined.csv"
# data = pd.read_csv(file_path)

# cols = list(data.columns)
# feature_names = cols[1:-1]
# X = data.iloc[:, 1:-1]
# y = data['label']
# filenames = data.iloc[:, 0]

# # Train/test split
# X_train, X_test, y_train, y_test = train_test_split(
#     X, y, test_size=TEST_SIZE, shuffle=True, stratify=y, random_state=RANDOM_STATES_TO_TEST
# )

# # ← MODIFICATION : définir la grille des paramètres pour RandomForest
# param_grid = {
  
#     'max_depth': [2, 3, 4,6,8,10],  # ← petites profondeurs
#     'random_state': [SEED]
# }

# # ← MODIFICATION : GridSearchCV
# grid_search = GridSearchCV(DecisionTreeClassifier(), param_grid,
#                            cv=3, scoring='f1_weighted', n_jobs=-1, verbose=1)
# grid_search.fit(X_train, y_train)
# clf = grid_search.best_estimator_
# print("✅ Meilleurs hyperparamètres :", grid_search.best_params_)

# # Labels
# labels = [str(cls) for cls in clf.classes_]
# print("Labels :", labels)

# # Prédiction et évaluation
# y_pred = clf.predict(X_test)

# accuracy = accuracy_score(y_test, y_pred)
# precision = precision_score(y_test, y_pred, average='weighted')
# recall = recall_score(y_test, y_pred, average='weighted')
# f1 = f1_score(y_test, y_pred, average='weighted')

# print(f"Precision: {precision:.2f}")
# print(f"Recall: {recall:.2f}")
# print(f"F1-Score: {f1:.2f}")
# print(f"Accuracy: {accuracy:.2f}")

# # Matrice de confusion
# conf_matrix = confusion_matrix(y_test, y_pred, labels=labels)
# plt.figure(figsize=(8, 8))
# sns.heatmap(conf_matrix, annot=True, fmt='d', xticklabels=labels, yticklabels=labels)
# plt.ylabel("Actual")
# plt.xlabel("Predicted")
# plt.show()

# # Visualiser jusqu’à 3 arbres
# plt.figure(figsize=(15, 10))
# plot_tree(clf, filled=True, feature_names=X.columns, class_names=labels, fontsize=10)
# plt.title("Arbre de décision")
# plt.show()



output_directory = "results_output"





# Charger le fichier CSV
file_path = "results_output/z_array_combined.csv"  # Remplacez par le chemin de votre fichier CSV
data = pd.read_csv(file_path)

cols = list(data.columns)
feature_names = cols[1:-1]
X = data.iloc[:, 1:-1]
y = data['label']
filenames = data.iloc[:, 0]

# Train/test split
X_train, X_test, y_train, y_test = train_test_split(
    X, y, test_size=TEST_SIZE, shuffle=True, stratify=y, random_state=RANDOM_STATES_TO_TEST
)

# ← MODIFICATION : définir la grille des paramètres pour RandomForest
param_grid = {
  
    'max_depth': [2, 3, 4,6,8,10],  # ← petites profondeurs
    'random_state': [SEED]
}

# ← MODIFICATION : GridSearchCV
grid_search = GridSearchCV(DecisionTreeClassifier(), param_grid,
                           cv=3, scoring='f1_weighted', n_jobs=-1, verbose=1)
grid_search.fit(X_train, y_train)
clf = grid_search.best_estimator_
print("✅ Meilleurs hyperparamètres :", grid_search.best_params_)

# Labels
labels = [str(cls) for cls in clf.classes_]
print("Labels :", labels)

# Prédiction et évaluation
y_pred = clf.predict(X_test)

accuracy = accuracy_score(y_test, y_pred) * 100
precision = precision_score(y_test, y_pred, average='weighted') * 100
recall = recall_score(y_test, y_pred, average='weighted') * 100
f1 = f1_score(y_test, y_pred, average='weighted') * 100

print(f"Precision: {precision:.2f}")
print(f"Recall: {recall:.2f}")
print(f"F1-Score: {f1:.2f}")
print(f"Accuracy: {accuracy:.2f}")

# Matrice de confusion
conf_matrix = confusion_matrix(y_test, y_pred, labels=labels)
plt.figure(figsize=(8, 8))
sns.heatmap(conf_matrix, annot=True, fmt='d', xticklabels=labels, yticklabels=labels)
plt.ylabel("Actual")
plt.xlabel("Predicted")
plt.show()

# Visualiser jusqu’à 3 arbres
plt.figure(figsize=(15, 10))
plot_tree(clf, filled=True, feature_names=X.columns, class_names=labels, fontsize=10)
plt.title("Arbre de décision")
plt.show()

# Extraire les règles pour chaque classe


tree_text = export_text(clf, feature_names=list(X.columns))
print(tree_text)

# Prédiction sur tout le dataset
y_pred_full = clf.predict(X)
accuracy = accuracy_score(y, y_pred_full)*100
precision = precision_score(y, y_pred_full, average='weighted')*100
recall = recall_score(y, y_pred_full, average='weighted')*100
f1 = f1_score(y, y_pred_full, average='weighted')*100

print(f"Full Precision: {precision:.2f}")
print(f"Full Recall: {recall:.2f}")
print(f"Full F1-Score: {f1:.2f}")
print(f"Full Accuracy: {accuracy:.2f}")

# Matrice de confusion complète
conf_matrix = confusion_matrix(y, y_pred_full, labels=labels)
plt.figure(figsize=(8, 8))
sns.heatmap(conf_matrix, annot=True, fmt='d', xticklabels=labels, yticklabels=labels)
plt.ylabel("Actual")
plt.xlabel("Predicted")
plt.show()

# Erreurs de classification
misclassified_indices = np.where(y != y_pred_full)[0]
misclassified_samples = X.iloc[misclassified_indices].copy()
misclassified_samples['filename'] = filenames.iloc[misclassified_indices].values
misclassified_samples['true_label'] = y.iloc[misclassified_indices].values
misclassified_samples['predicted_label'] = y_pred_full[misclassified_indices]
misclassified_samples = misclassified_samples[['filename', 'true_label', 'predicted_label']]
print("\n🔍 Exemples mal classés :")
print(misclassified_samples.head())
misclassified_samples.to_csv("results_output/misclassified_samples.csv", index=False)
print("\n💾 Misclassifications sauvegardées dans 'results_output/misclassified_samples.csv'")


automata_generator = extract_rules_for_class(clf,clf.classes_)
#print(automata_generator)



json_data = {
            "target": "c++",
            "imports": [],
            "type_definitions": {"schemas": [], "native_types": {}, "events": []},
            "top_level_astds": [],
            "conf": []
        }

json_data["top_level_astds"].append(automata_generator)

with open(f'model.json', 'w') as jsonfile:
    json.dump(json_data, jsonfile, indent=2)