import json
import re
import glob
from collections import defaultdict
import math

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
            packer_conditions.append(f"({global_condition})")  # 👈 on l’ajoute ici

        rules_thresholds.extend(packer_rules)

        # 👇 Créer la condition globale pour le packer (join avec ||)
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



def generate_end_automaton(packer_name , patron_name, transitions, packer_global_conditions):
    states = [{"state_id": i, "is_entry": "True" if i == 0 else "False", "is_final": "True" if i == len(transitions) else "False"} 
            for i in range(len(transitions) + 1)]
    invariant_conditions = packer_global_conditions[packer_name]
    transitions_list = [{"src_state": 0, "dest_state": 0, "symbol": transitions[0]}]
    transitions_list[0]["action"] = f'{{ if ({invariant_conditions}) {{ std::cout << "c\'est le packer {packer_name}" << std::endl; }} }}'
    return {
        "name": patron_name+"_"+packer_name,
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


def generate_ASTD_automaton(packer_name, rule_id, pattern_name,transitions,k,pattern_list):
    states = [{"state_id": i, "is_entry": "True" if i == 0 else "False", "is_final": "True" if i == k+1 else "False"} 
                for i in range(k + 1)]
    
    for trans in transitions:
        if trans["src_state"] == k-1 and trans["dest_state"] == k:
            trans["action"] = f"{packer_name}_R{rule_id}_{pattern_name}=1"
    guard_conditions = " && ".join(f'x!="{symbol}"' for symbol in pattern_list)
    guard = f"{guard_conditions}"
    return {
                "name": "Aut1_"+str(rule_id)+"_"+pattern_name+"_"+packer_name,
                "parameters": [],
                "type": "Automaton",
                "invariant": "",
                "typed_astd": {
                    "attributes": [],
                    "code": "",
                    "interruptCode": "",
                    "states": [
                        {
                            "name": f"Aut2_{rule_id}_{pattern_name}_{packer_name}",
                            "astd": {
                                "name":  f"Aut2_{rule_id}_{pattern_name}_{packer_name}",
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
                                "from_state_name": f"Aut2_{rule_id}_{pattern_name}_{packer_name}",
                                "to_state_name": f"Aut2_{rule_id}_{pattern_name}_{packer_name}"
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
                    "initial_state_name": f"Aut2_{rule_id}_{pattern_name}_{packer_name}"
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

    




# Main execution
if __name__ == "__main__":
    rules_dir = "rules"
    patterns_file = "results_output/patterns_with_ids.json"
    output_json_file = "output.json"
    
    flows,conditions,packer_var_names = process_packer_rules(rules_dir, patterns_file)
    
    automatons_by_packer = generate_astd_automaton_for_rules(flows)
    with open(output_json_file, "w", encoding="utf-8") as json_file:
        json.dump(automatons_by_packer, json_file, ensure_ascii=False, indent=4)
    
    flows = []
    for packer_name in automatons_by_packer:
        automata_list = automatons_by_packer[packer_name]
        nouveau_patron = "patron_end"
        nouvelles_transitions = ["end"] 
        end_automaton = generate_end_automaton(packer_name , nouveau_patron, nouvelles_transitions, conditions)
        automata_list.append(end_automaton)
        var_names = packer_var_names[packer_name]
        if len(automata_list) > 1:
            while len(automata_list) > 2:
                dernier_kleene = automata_list.pop()
                automata_list[-1] = generate_flow(f"FlowASTD_o{len(automata_list)-1}_{packer_name}", automata_list[-1], dernier_kleene)


            attributes = []
            for attr in var_names:
                # Ajouter l'attribut occ_{patron}_{packer_name}
                attributes.append({
                    "name": f"{attr}",
                    "type": "int",
                    "initial_value": 0
                })
            

            top_flow = {
            "name": "FlowASTD_"+packer_name,
            "type": "Flow",
            "invariant": "",
            "typed_astd": {
            "attributes": attributes,
            "code": "",
            "interruptCode": "",
            "left_astd": automata_list[0] if len(automata_list) > 0 else None,
            "right_astd": automata_list[1] if len(automata_list) > 1 else None
            }
            }
            flows.append(top_flow)
        else:
            single_automaton = automata_list[0]
            single_automaton["typed_astd"]["attributes"] = [
                {"name": f"{attr}", "type": "int", "initial_value": 0} for attr in var_names
            ]
            flows.append(single_automaton)

    json_data = {
            "target": "c++",
            "imports": [],
            "type_definitions": {"schemas": [], "native_types": {}, "events": []},
            "top_level_astds": [],
            "conf": []
        }

    if len(flows)>1:
        while len(flows) > 2:
            dernier_flow = flows.pop()
            flows[-1] = generate_flow(f"FlowASTD_b{len(flows)-1}", flows[-1], dernier_flow)

        top_flows = {
        "name": "FlowASTD",
        "parameters": [],
        "type": "Flow",
        "invariant": "",
        "typed_astd": {
        "attributes": [],
        "code": "",
        "interruptCode": "",
        "left_astd": flows[0] if len(flows) > 0 else None,
        "right_astd":flows[1] if len(flows) > 1 else None
        }
        }

        json_data["top_level_astds"].append(top_flows)

    else:
        json_data["top_level_astds"].append(flows[0])




    with open(f'specV6.json', 'w') as jsonfile:
        json.dump(json_data, jsonfile, indent=2)
    print("JSON file generated successfully")
