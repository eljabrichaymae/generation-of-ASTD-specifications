#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "json.hpp"
#include <set>

using json = nlohmann::json;
using namespace std;


#include <unordered_set>



// Inclure le fichier d'en-tête de la fonction C
extern "C" {
    #include "stree_strmat.h"
    #include "stree_ukkonen.h"
    #include "repeats_supermax.h"
    #include "repeats_primitives.h"
}

int cpt = 0;

class Node {
public:
    std::string fn;
    std::vector<std::vector<std::string>> IN;
    std::vector<std::string> OUT;
    std::string name;
    std::string address;
    std::string timestamp;

public:
    Node(const std::unordered_map<std::string, std::string>& line) {
    if(line.find("FunctionName") != line.end()){
        fn = line.at("FunctionName");
        //std::cout << fn << std::endl;
    }else if(line.find("Functionname") != line.end() ){
        fn = line.at("Functionname");
    }
    name = "N" + std::to_string(++cpt);
    if(line.find("Address") != line.end()){
        address = line.at("Address");
        //std::cout << address  << std::endl;
    }
    if(line.find("Timesptamp") != line.end()){
        timestamp = line.at("Timesptamp"); // Correct the spelling of "Timestamp"
        //std::cout << timestamp  << std::endl;
    }

    // // Check if "Arguments" field exists in the JSON
    // if (line.find("Arguments") != line.end()) {
    //     // Extract the array of arguments
    //     json arguments = json::parse(line.at("Arguments"));

    //     // Iterate over each argument and add it to the IN vector
    //     for (const auto& arg : arguments) {
    //         std::vector<std::string> argList;
    //         // Iterate over each element in the argument array
    //         for (const auto& element : arg) {
    //             // Convert each element to string and add to argList
    //             argList.push_back(element.get<std::string>());
    //         }
    //         // Add the argument list to the IN vector
    //         IN.push_back(argList);
    //     }
    // }
    if(line.find("Return") != line.end()){
        OUT.push_back(line.at("Return"));
    }
}



    std::string str() const {
        std::string result = fn + "(";
        for (const auto& arg : IN) {
            for (const auto& val : arg) {
                result += val + ",";
            }
        }
        result.pop_back(); // Remove trailing comma
        result += ")";
        return result;
    }

    std::string repr() const {
        return str();
    }

       static std::vector<Node> build_graph(const std::vector<json>& lines) {
        std::vector<Node> nodes;
        for (const auto& line : lines) {
            //if (line.find("FunctionName") != line.end()) {
                std::unordered_map<std::string, std::string> data;
                for (auto it = line.begin(); it != line.end(); ++it) {
                   
                    if (it.key() == "FunctionName" || it.key() == "Functionname" || it.key() == "Address" || it.key() == "Timesptamp"
                    || it.key() == "Return") {
                        //std::cout <<"key :" <<it.key() << std::endl;
                        //std::cout <<"value :" <<it.value() << std::endl;
                        data[it.key()] = it.value();
                    }            
                }
                nodes.emplace_back(data);
           //}
        }
        return nodes;
    }
};


// Using a variant to store different types of data
using RepeatData = std::variant<std::vector<int>, Triplet>;

// Method to extract FunctionNames from a list of nodes
std::vector<std::string> extractFunctionNames(const std::vector<Node>& nodes) {
    std::vector<std::string> functionNames;
    for (const auto& node : nodes) {
        // Assuming FunctionName is a member of Node struct
        functionNames.push_back(node.fn);
    }
    return functionNames;
}

// Method to determine unique function names from a list of nodes
std::unordered_set<std::string> uniqueFunctionNames(const std::vector<std::string>& functionNames) {
    std::unordered_set<std::string> uniqueNames;
    for (const auto& fn : functionNames) {
        uniqueNames.insert(fn);
    }
    return uniqueNames;
}


// Method to determine and order unique function names from a list of nodes
std::set<std::string> orderedUniqueFunctionNames(const std::unordered_set<std::string>& list) {
    std::set<std::string> uniqueNames(list.begin(), list.end());
    return uniqueNames;
}


// Method to associate unique function names with numbers in order
std::map<std::string, int> associateFunctionNamesWithNumbers(const std::set<std::string>& uniqueFunctionNames) {
    std::map<std::string, int> functionNameToNumberMap;
    int number = 1; // Start numbering from 1
    for (const auto& functionName : uniqueFunctionNames) {
        functionNameToNumberMap[functionName] = number++;
    }
    return functionNameToNumberMap;
}


// Méthode pour remplacer les appels systèmes par leurs numéros dans une liste de noms de fonctions
std::vector<int> remplacerAppelsSystemeParNumeros(const std::vector<std::string>& listeNoms, const std::map<std::string, int>& systemCallsMap) {
    std::vector<int> numeros;
    for (const auto& nom : listeNoms) {
        auto it = systemCallsMap.find(nom);
        if (it != systemCallsMap.end()) {
            numeros.push_back(it->second); // Ajouter le numéro correspondant
        }
    }
    return numeros;
}




// Comparateur pour les triplets pour les utiliser comme clé dans la map
struct TripletComparator {
    bool operator()(const Triplet& a, const Triplet& b) const {
        if (a.length != b.length) return a.length < b.length;
        return std::lexicographical_compare(a.sequence, a.sequence + a.length, b.sequence, b.sequence + b.length);
    }
};

// Fonction pour vérifier si deux séquences sont des traductions circulaires
bool are_circular_permutations(const std::vector<int>& seq1, const std::vector<int>& seq2) {
    if (seq1.size() != seq2.size()) return false;

    std::vector<int> doubled_seq1(seq1);
    doubled_seq1.insert(doubled_seq1.end(), seq1.begin(), seq1.end());

    for (size_t i = 0; i < seq1.size(); ++i) {
        if (std::equal(seq2.begin(), seq2.end(), doubled_seq1.begin() + i)) {
            return true;
        }
    }
    return false;
}

// Fonction pour afficher une séquence
void print_sequence(const std::vector<int>& sequence) {
    for (int num : sequence) {
        std::cout << num << " ";
    }
}

// Fonction pour afficher les informations d'un triplet
void print_triplet(const Triplet& triplet) {
    std::cout << "Start: " << triplet.start << ", Length: " << triplet.length
              << ", Iteration: " << triplet.iteration << ", Sequence: ";
    print_sequence(std::vector<int>(triplet.sequence, triplet.sequence + triplet.length));
    std::cout << std::endl;
}

// Fonction pour afficher les triplets dans la map
void print_map(const std::map<Triplet, std::vector<Triplet>, TripletComparator>& result_map) {
    for (const auto& entry : result_map) {
        std::cout << "Canonical Sequence: ";
        print_triplet(entry.first);
        std::cout << "\nTriplet Info:\n";
        
        for (const auto& triplet : entry.second) {
            print_triplet(triplet);
        }
        std::cout << std::endl;
    }
}

// Fonction pour convertir la liste en une map de séquences
std::map<Triplet, std::vector<Triplet>, TripletComparator> convert_list_to_map(TripletNode* list) {
    std::map<Triplet, std::vector<Triplet>, TripletComparator> result_map;

    TripletNode* current = list;

    while (current != nullptr) {
        std::vector<int> current_seq(current->triplet.sequence, current->triplet.sequence + current->triplet.length);

        bool key_exists = false;
        Triplet* existing_key = nullptr;

        // Vérifier si la séquence est déjà une permutation circulaire d'une clé existante
        for (auto& [key, _] : result_map) {
            std::vector<int> key_seq(key.sequence, key.sequence + key.length);
            if (are_circular_permutations(current_seq, key_seq)) {
                key_exists = true;
                existing_key = const_cast<Triplet*>(&key);
                break;
            }
        }

        if (!key_exists) {
            // Ajouter une nouvelle clé avec les triplets associés
            result_map[current->triplet] = std::vector<Triplet>();
        } else {
            // Ajouter le triplet actuel à la clé existante
            result_map[*existing_key].push_back(current->triplet);
        }

        current = current->next;
    }

    return result_map;
}

// Fonction pour convertir la liste en vecteur
std::vector<Triplet> convert_list_to_vector(TripletNode* list){

    std::vector<Triplet> tandem_repeats;
    TripletNode* current = list;
    while(current != nullptr){
        tandem_repeats.push_back(current->triplet);
        current = current->next;
    }
    return tandem_repeats;
}

// Function to check if a set is a subset of another set
bool is_subset(const std::set<int>& subset, const std::set<int>& superset) {
    return std::includes(superset.begin(), superset.end(), subset.begin(), subset.end());
}

// Function to add primitive_repeats to the map based on the specified conditions
void add_primitive_repeats_to_map(std::map<std::set<int>, std::vector<RepeatData>>& repeats_map, 
                                  const std::vector<Triplet>& primitive_repeats) {
    for (auto& triplet : primitive_repeats) {
        // Create a set from the repeat vector
        std::set<int> unique_chars(triplet.sequence, triplet.sequence + triplet.length);
        bool added = false;

        // Try adding to the key with the same alphabet
        for (auto& pair : repeats_map) {
            if (unique_chars == pair.first) {
                pair.second.push_back(triplet);
                added = true;
                break;
            }
        }

        // If not added, try adding to a key containing the alphabet as a subset
        if (!added) {
            for (auto& pair : repeats_map) {
                if (is_subset(unique_chars, pair.first)) {
                    pair.second.push_back(triplet);
                    break; // Once added, no need to check other keys
                }
            }
        }
    }
}

// Fonction pour retrouver les supermaximals repeats
std::vector<std::vector<int>> get_maximal_repeats(std::vector<int> dataVector,int minPercent, int minLength){
       // Find supermaximals in the input string
   SUPERMAXIMALS supermaximals = supermax_find(dataVector.data(), dataVector.size(), minPercent, minLength);

    // Output the results
    std::vector<std::vector<int>> supermaximalVectors;

    SUPERMAXIMALS current = supermaximals;
    while (current != NULL) {
    std::vector<int> supermaximalList;

    // Ajoutez les éléments de la liste S au vecteur supermaximalList
    for (int i = 0; i < current->M; i++) {
        supermaximalList.push_back(current->S[i]);
    }

    // Ajoutez le vecteur supermaximalList au vecteur de vecteurs supermaximalVectors
    supermaximalVectors.push_back(supermaximalList);

    current = current->next;
    }
    return supermaximalVectors;
}


// Function to create a map with sets of unique integers as keys and corresponding vectors as values
std::map<std::set<int>, std::vector<RepeatData>>  create_repeats_map(const std::vector<std::vector<int>>& maximal_repeats) {
    // Map to hold sets of unique integers as keys and corresponding vectors as values
    std::map<std::set<int>, std::vector<RepeatData>> repeats_map;
    
    // Iterate over the maximal_repeats vector
    for (const auto& repeat : maximal_repeats) {
        // Create a set from the repeat vector
        std::set<int> unique_chars(repeat.begin(), repeat.end());
        
        // Insert the set and the corresponding repeat vector into the map
        repeats_map[unique_chars].push_back(repeat);
    }
    
    return repeats_map;
}


// Function to subtract maximal_repeats from near_maximal_repeats
std::vector<std::vector<int>> subtract_repeats(const std::vector<std::vector<int>>& near_maximal_repeats, const std::vector<std::vector<int>>& maximal_repeats) {
    std::vector<std::vector<int>> result;
    
    // Use a set for maximal_repeats for faster look-up
    std::set<std::vector<int>> maximal_repeats_set(maximal_repeats.begin(), maximal_repeats.end());
    
    for (const auto& repeat : near_maximal_repeats) {
        if (maximal_repeats_set.find(repeat) == maximal_repeats_set.end()) {
            result.push_back(repeat);
        }
    }
    
    return result;
}




// Fonction pour ajouter les near_super_maximals à la map selon les critères spécifiés
void add_near_super_maximals_to_map(std::map<std::set<int>, std::vector<RepeatData>>& repeats_map, 
                                    const std::vector<std::vector<int>>& filtered_near_super_maximals) {
    for (const auto& repeat : filtered_near_super_maximals) {
        std::set<int> unique_chars(repeat.begin(), repeat.end());
        bool added = false;

        // Essayez d'ajouter à la clé avec le même alphabet
        for (auto& pair : repeats_map) {
            if (unique_chars == pair.first) {
                pair.second.push_back(repeat);
                added = true;
                break;
            }
        }

        // Si non ajouté, essayez d'ajouter à une clé contenant l'alphabet comme sous-ensemble
        if (!added) {
            for (auto& pair : repeats_map) {
                if (is_subset(unique_chars, pair.first)) {
                    pair.second.push_back(repeat);
                    break; // Une fois ajoutée, pas besoin de vérifier d'autres clés
                }
            }
        }
    }
}


// Fonction pour sauvegarder le repeats_map dans un fichier texte
void save_repeats_map_to_file(const std::map<std::set<int>, std::vector<RepeatData>>& repeats_map, const std::string& filename) {
    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        std::cerr << "Could not open the file!" << std::endl;
        return;
    }

    for (const auto& pair : repeats_map) {
        outfile << "{";
        for (auto it = pair.first.begin(); it != pair.first.end(); ++it) {
            if (it != pair.first.begin()) outfile << ",";
            outfile << *it;
        }
        outfile << "} = {";

        for (auto it = pair.second.begin(); it != pair.second.end(); ++it) {
            std::visit([&outfile](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, std::vector<int>>) {
                    outfile << "[";
                    for (size_t i = 0; i < arg.size(); ++i) {
                        if (i != 0) outfile << ",";
                        outfile << arg[i];
                    }
                    outfile << "]";
                } else if constexpr (std::is_same_v<T, Triplet>) {
                    outfile << "(" << arg.start << ", [";
                    for (int i = 0; i < arg.length; ++i) {
                        if (i != 0) outfile << ",";
                        outfile << arg.sequence[i];
                    }
                    outfile << "], " << arg.iteration << ")";
                }
            }, *it);

            if (std::next(it) != pair.second.end()) {
                outfile << ", ";
            }
        }
        outfile << "}\n";
    }

    outfile.close();
}





// Fonction pour fusionner les occurrences des séquences dans la map
std::map<Triplet, std::vector<Triplet>, TripletComparator> merge_occurrences(std::map<Triplet, std::vector<Triplet>, TripletComparator>& result_map) {
    std::map<Triplet, std::vector<Triplet>, TripletComparator> new_result_map;

    for (auto& [key, triplets] : result_map) {
        std::vector<Triplet> merged_triplets;
        Triplet current = key;
        Triplet new_key = current;

        for (auto& triplet : triplets) {
            int current_end = new_key.start + (new_key.length * new_key.iteration);
            int next_start = triplet.start;

            if (current_end >= next_start) {
                int next_end = triplet.start + (triplet.length * triplet.iteration);
                if ((next_end - current_end)>= new_key.length &&
                ((next_end - current_end)-(next_start - new_key.start)) % new_key.length == 0) {
                    int value = ((next_end - current_end)-(next_start - new_key.start)) / new_key.length;
                    // Étendre l'itération actuelle
                    if(value<=1){
                        new_key.iteration +=1;
                    }else{
                        new_key.iteration += value;
                    }
                }else if((next_end - current_end)-(next_start - new_key.start) < new_key.length){
                    continue;
                }else {
                    // Ajouter le triplet actuel au résultat et continuer
                    merged_triplets.push_back(triplet);
                }
                            
            } else {
                // Ajouter le triplet actuel au résultat et commencer une nouvelle fusion
                merged_triplets.push_back(triplet);
            }
        }
        // Ajouter les triplets fusionnés à la nouvelle map
        new_result_map[new_key] = merged_triplets;
    }
    // Retourner la nouvelle map avec les valeurs fusionnées
    return new_result_map;
}





int main(int argc, char *argv[]) {
   
    // if (argc < 4) {
    //     std::cerr << "Usage: " << argv[0] << " <file1> <file2> <file3>" << std::endl;
    //     return 1;
    // }
    // Initial delimiter value
    int delimiterCounter = 1;
    std::vector<std::string> filenames = {"pec2-hostname.exe.bin.json"};
  
    std::vector<std::string> functionNamesglobal;

    for (const auto& filename : filenames) {
        // Open the JSON file
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Unable to open file " << filename << std::endl;
            return 1;
        }

        // Read the JSON content line by line
        std::string line;
        std::vector<json> lines;
        while (std::getline(file, line)) {
            if (!line.empty()) {
                try {
                    json jsonLine = json::parse(line);
                    lines.push_back(jsonLine);
                } catch (const std::exception& e) {
                    std::cerr << "Error: Failed to parse JSON line: " << line << std::endl;
                }
            }
        }

        // Process the JSON data
        std::vector<Node> nodes = Node::build_graph(lines);
        std::vector<std::string> functionNames = extractFunctionNames(nodes);
        functionNamesglobal.insert(functionNamesglobal.end(), functionNames.begin(),functionNames.end());
        // Add a unique delimiter for this file
        functionNamesglobal.push_back("DELIM_" + std::to_string(delimiterCounter));
        delimiterCounter++;
    }
  
     // Ajout de "()" à chaque nom de fonction
    for (std::string& name : functionNamesglobal) {
        cout<<name<<endl;
        name += "()";
    }   

    // Ouverture d'un fichier .txt pour écrire les noms de fonctions modifiés
    std::ofstream outFile("functions.txt");
    if (!outFile) {
        std::cerr << "Erreur d'ouverture du fichier!" << std::endl;
        return 1;
    }

      // Écriture des noms de fonctions dans le fichier
    for (const std::string& name : functionNamesglobal) {
        outFile << name << std::endl;
    }

    // Fermeture du fichier
    outFile.close();



    // // Convertir le vecteur de vecteurs de chaînes en un objet JSON
    json jsonData3 = functionNamesglobal;

    // // Écrire le contenu JSON dans un fichier
    ofstream outputFile11("functionNames.json");
    if (outputFile11.is_open()) {
         outputFile11 << jsonData3.dump(4); // L'argument 4 pour une indentation de 4 espaces
         outputFile11.close();
         cout << "Le fichier JSON a été créé avec succès." << endl;
     } else {
         cout << "Erreur: Impossible d'ouvrir le fichier JSON pour l'écriture." << endl;
     }

    std::unordered_set<std::string> uniqueNamesSet = uniqueFunctionNames(functionNamesglobal);
    std::set<std::string> orderList = orderedUniqueFunctionNames(uniqueNamesSet);
    std::map<std::string, int> fnMap = associateFunctionNamesWithNumbers(orderList);


    // Convertir le vecteur de vecteurs de chaînes en un objet JSON
    json jsonData66 = fnMap;

    // Écrire le contenu JSON dans un fichier
    ofstream outputFile444("mapFn.json");
    if (outputFile444.is_open()) {
        outputFile444 << jsonData66.dump(4); // L'argument 4 pour une indentation de 4 espaces
        outputFile444.close();
        cout << "Le fichier JSON a été créé avec succès." << endl;
    } else {
        cout << "Erreur: Impossible d'ouvrir le fichier JSON pour l'écriture." << endl;
    }

    std::vector<int> dataVector = remplacerAppelsSystemeParNumeros(functionNamesglobal, fnMap);

    for (const auto& num : dataVector) {
        std::cout << num << " ";
    }
    std::cout << std::endl;
      // Print the size of the integer vector
    std::cout << "Size of the vector: " << dataVector.size() << std::endl;
    std::vector<std::vector<int>> maximal_repeats = get_maximal_repeats(dataVector,100,3);
    std::map<std::set<int>, std::vector<RepeatData>> super_maximal_repeats_map = create_repeats_map(maximal_repeats);
    std::vector<std::vector<int>> near_maximal_repeats = get_maximal_repeats(dataVector,0,3);
    std::vector<std::vector<int>> substraction = subtract_repeats(near_maximal_repeats,maximal_repeats);
    // Add the filtered near_super_maximals to the map if the alphabet is included in any key
    add_near_super_maximals_to_map(super_maximal_repeats_map, substraction );

    primitives_struct *pr_struct;
    /*
    * Allocate the memory.
    */
    printf("Preprocessing...\n");
    //Modifier l'entrée
    pr_struct = primitives_prep(dataVector.data(), dataVector.data(), dataVector.size());
    if(pr_struct == nullptr)
        return 0;

    /*
    * Report the results.
    */
    printf("\nThe following primitive tandem repeats were found:\n\n");
    primitives_find(pr_struct);
    sort_triplets();
    group_and_verify_tandem_repeats(dataVector.data());
    TripletNode* triplet_list_head = NULL;
    triplet_list_head = get_triplet_list_head();
    std::vector<Triplet> primitive_repeats = convert_list_to_vector(triplet_list_head);
    add_primitive_repeats_to_map(super_maximal_repeats_map, primitive_repeats);
    print_triplets();
    primitives_free(pr_struct);
        // Sauvegarder les résultats dans un fichier texte
    save_repeats_map_to_file(super_maximal_repeats_map, "repeats_map.txt");

    return 0;
}
