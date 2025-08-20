#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include "json.hpp"
#include <set>

using json = nlohmann::json;
namespace fs = std::filesystem;

using namespace std;


#include <unordered_set>



// Inclure le fichier d'en-tête de la fonction C
extern "C" {
    #include "stree_strmat.h"
    #include "stree_ukkonen.h"
    #include "repeats_supermax.h"
    #include "repeats_primitives.h"
    //#include "repeats_tandem.h"
  
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
std::set<std::string> uniqueFunctionNames(const std::vector<std::string>& functionNames) {
    std::set<std::string> uniqueNames;
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


// Fonction pour récupérer les répétitions maximales
std::vector<std::tuple<std::vector<int>, std::vector<int>>> get_maximal_repeats1(std::vector<int> dataVector, int minPercent, int minLength) {
    // Trouver les supermaximaux dans la chaîne d'entrée
    SUPERMAXIMALS supermaximals = supermax_find(dataVector.data(), dataVector.size(), minPercent, minLength);

    // Préparer le résultat sous forme de tuples
    std::vector<std::tuple<std::vector<int>, std::vector<int>>> supermaximalTuples;
    SUPERMAXIMALS current = supermaximals;

    while (current != NULL) {
        std::vector<int> supermaximalSequence(current->S, current->S + current->M);
        std::vector<int> indicesList(current->start_indices, current->start_indices + current->num_start_indices);

        supermaximalTuples.emplace_back(std::make_tuple(supermaximalSequence, indicesList));

        current = current->next;
    }

    return supermaximalTuples;
}


// Fonction pour associer des noms de fonctions avec des nombres
std::map<int, std::string> reversefnMap(std::map<std::string, int>& fnMap) {
    std::map<int, std::string> reverseFnMap;
    for (const auto& entry : fnMap) {
        reverseFnMap[entry.second] = entry.first;
    }
    return reverseFnMap;
}



// Fonction pour écrire le JSON dans un fichier
void writeJsonToFile(const json& j, const string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Unable to open file: " << filename << endl;
        return;
    }
    file << j.dump(4);  // 4 pour l'indentation
    file.close();
}


std::vector<int> replaceTandemRepeats(const std::vector<int>& dataVector, std::vector<Triplet>& tandemRepeatSequences) {
    std::vector<int> newTrace;
    size_t i = 0;

    while (i < dataVector.size()) {
        bool foundRepeat = false;

        for (const auto& repeatSequence : tandemRepeatSequences) {
            size_t len = repeatSequence.length;
            // std::cout << "Sequence contents: ";
            // for (size_t j = 0; j < len; ++j) {
            //     std::cout << repeatSequence.sequence[j] << " ";
            // }
            // cout<<repeatSequence.length<<endl;
            // Vérification des pointeurs et des limites
            if (repeatSequence.sequence == nullptr || len <= 0 || len > dataVector.size()) {
                
                throw std::runtime_error("Séquence ou longueur invalide dans Triplet !");
            }

            // Vérification des répétitions
            if (i + len <= dataVector.size() && std::equal(repeatSequence.sequence, repeatSequence.sequence + len, dataVector.begin() + i)) {


                // std::cout<<"tandem repeat found"<<std::endl;
                // Insertion de la séquence
                newTrace.insert(newTrace.end(), repeatSequence.sequence, repeatSequence.sequence + len);

                // Avancement de l'indice pour sauter les répétitions
                while (i + len <= dataVector.size() && std::equal(repeatSequence.sequence, repeatSequence.sequence + len, dataVector.begin() + i)) {
                    i += len;
                }

                foundRepeat = true;
                break;
            }
        }

        // Si aucune répétition n'est trouvée, ajoutez l'élément courant
        if (!foundRepeat) {
            newTrace.push_back(dataVector[i]);
            ++i;
        }
    }

    return newTrace;
}



std::vector<Triplet> find_primitive_repests(std::vector<int> dataVector){
    primitives_struct *pr_struct;
    /*
    * Allocate the memory.
    */
    pr_struct = primitives_prep(dataVector.data(), dataVector.data(), dataVector.size());
    primitives_find(pr_struct);
    sort_triplets();
    group_and_verify_tandem_repeats(dataVector.data());
    TripletNode* triplet_list_head = NULL;
    triplet_list_head = get_triplet_list_head();
    std::vector<Triplet>  primitive_repeats = convert_list_to_vector(triplet_list_head);
    primitives_free(pr_struct);
    return primitive_repeats;
}


int findMaxLength(const std::vector<Triplet>& primitive_repeats) {
if (primitive_repeats.empty()) return 0;

int max_length = 0;

for (const auto& triplet : primitive_repeats) {
    if (triplet.length > max_length) {
        max_length = triplet.length;
    }
}

return max_length;
}

// Fonction pour obtenir les indices des délimiteurs dans la trace
std::vector<int> getDelimiterIndices(const std::vector<int>& newTrace, const std::vector<int>& delimitors) {
    std::vector<int> delimiterIndices;

    // Parcourir la trace pour trouver les indices des délimiteurs
    for (size_t i = 0; i < newTrace.size(); ++i) {
        if (std::find(delimitors.begin(), delimitors.end(), newTrace[i]) != delimitors.end()) {
            delimiterIndices.push_back(i); // Ajouter l'indice du délimiteur
        }
    }

    return delimiterIndices;
}

int findTraceForIndex(int index, const std::vector<int>& delimiterIndices) {
    for (size_t i = 0; i < delimiterIndices.size(); ++i) {
        if (index-1 < delimiterIndices[i]) {
            return i; // Index belongs to trace `i`
        }
    }
    return delimiterIndices.size(); // Index belongs to the last trace
}

int countUniqueTraces(const std::vector<int>& repeatIndices, const std::vector<int>& delimiterIndices) {
    std::vector<int> traceOcc;
    
    // Step 1: Determine the trace for each index in repeatIndices
    for (int index : repeatIndices) {
        traceOcc.push_back(findTraceForIndex(index, delimiterIndices));
    }
    
    // Step 2: Convert traceOcc to a set to get unique trace numbers
    std::unordered_set<int> uniqueTraces(traceOcc.begin(), traceOcc.end());
    
    // Step 3: Return the size of the set, which is the count of unique traces
    return uniqueTraces.size();
}


    std::vector<vector<int>> filterSuperMaximalsInMultipleTraces(
    const std::vector<std::tuple<vector<int>, std::vector<int>>>& maximal_repeats_with_indices,
    const std::vector<int>& delimiterIndices) {

    std::vector<vector<int>> maximalRepeats;

    // Iterate over each repeat and its indices
    for (const auto& [repeat, indices] : maximal_repeats_with_indices) {
        // Check if repeat appears in at least two unique traces
        if (countUniqueTraces(indices, delimiterIndices) >= 2 && repeat.size()>=3 ) {
            maximalRepeats.push_back(repeat);
        }
    }

    return maximalRepeats;
}

// Fonction pour sauvegarder mapFn dans un fichier JSON
void saveMapFn(const std::string& filename, const std::map<std::string, int>& mapFn) {
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Erreur : Impossible d'ouvrir le fichier pour l'écriture " << filename << std::endl;
        return;
    }

    json j(mapFn);
    file << j.dump(4);  // 4 espaces d'indentation
}

// Fonction pour remplacer les appels système par leurs numéros
std::vector<int> replaceSystemCalls(std::vector<std::string>& systemCalls, std::map<std::string, int>& mapFn) {
    std::vector<int> callNumbers;
    int nextNumber = mapFn.size() + 1;

    for (const std::string& call : systemCalls) {
        // Si l'appel système est dans mapFn, utiliser le numéro existant
        if (mapFn.find(call) != mapFn.end()) {
            callNumbers.push_back(mapFn[call]);
        } else {
            // Sinon, ajouter l'appel avec un nouveau numéro dans mapFn
            mapFn[call] = nextNumber;
            callNumbers.push_back(nextNumber);
            nextNumber++;
        }
    }

    return callNumbers;
}

// Fonction pour inverser mapfn
std::unordered_map<int, std::string> invertMap(const std::map<std::string, int>& mapfn) {
    std::unordered_map<int, std::string> inverseMap;
    for (const auto& pair : mapfn) {
        inverseMap[pair.second] = pair.first;
    }
    return inverseMap;
}

std::vector<std::vector<std::string>> convertToStrings(const std::vector<std::vector<int>>& filtredMaximalRepeats, 
                                                       const std::unordered_map<int, std::string>& inverseMap) {
    std::vector<std::vector<std::string>> maximalrepeatsstring;

    for (const auto& repeat : filtredMaximalRepeats) {
        std::vector<std::string> stringRepeat;
        for (int val : repeat) {
            if (inverseMap.find(val) != inverseMap.end()) {
                stringRepeat.push_back(inverseMap.at(val)); // Remplace par la chaîne correspondante
            } else {
                stringRepeat.push_back("unknown"); // Valeur par défaut si non trouvée
            }
        }
        maximalrepeatsstring.push_back(stringRepeat);
    }

    return maximalrepeatsstring;

}



bool isValidUTF8(unsigned char c) {
    return (c >= 32 && c <= 126) || (c >= 160); // Garde les caractères imprimables
}

std::string removeInvalidUTF8(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.size(); ++i) {
        // Vérifier si le caractère est valide UTF-8
        if ((str[i] & 0x80) == 0) {
            // Caractère ASCII, ajouter tel quel
            result += str[i];
        } else {
            // Ignorer les caractères invalides
            // Tu peux aussi choisir d'ajouter un remplacement comme '?'
        }
    }
    return result;
}




// Fonction pour traiter les fichiers et générer des traces combinées
std::vector<int> processFiles(string folderPath,const std::vector<std::string>& filenames, int& delimiterCounter, 
                              std::map<std::string, int>& fnMap, std::vector<int>& delimitors) {
    std::vector<std::string> functionNamesGlobal;
    for (const auto& filename : filenames) {
        // Ouvrir le fichier JSON
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Erreur: Impossible d'ouvrir le fichier " << filename << std::endl;
            continue;
        }
        
        // Lire le JSON ligne par ligne
        std::string line;
        std::vector<json> lines;
        while (std::getline(file, line)) {
            
            line = removeInvalidUTF8(line);
            if (!line.empty()) {
                try {
                    json jsonLine = json::parse(line);
                    //std::cout<<jsonLine<<std::endl;
                    lines.push_back(jsonLine);
                } catch (const std::exception& e) {
                    std::cerr << "Erreur: Échec de l'analyse de la ligne JSON: " << line << std::endl;
                }
            }
        }
        
        // Construire le graphe et extraire les noms de fonctions
        std::vector<Node> nodes = Node::build_graph(lines);
        std::vector<std::string> functionNames = extractFunctionNames(nodes);
        functionNamesGlobal.insert(functionNamesGlobal.end(), functionNames.begin(), functionNames.end());
        
        // Ajouter un délimiteur unique
        functionNamesGlobal.push_back("DELIM_"+folderPath + std::to_string(delimiterCounter));
        delimiterCounter++;
    }
    
    // Utiliser un ensemble pour stocker les noms uniques et assurer un mappage commun
    std::set<std::string> uniqueNamesSet = uniqueFunctionNames(functionNamesGlobal);
    
    // Associer chaque nom unique à un numéro s'il n'est pas déjà dans le dictionnaire
    for (const auto& name : uniqueNamesSet) {
        if (fnMap.find(name) == fnMap.end()) {
            int newId = fnMap.size() + 1;
            fnMap[name] = newId;
        }
    }

    // Remplacer les appels système par leurs numéros en utilisant le même dictionnaire `fnMap`
    return remplacerAppelsSystemeParNumeros(functionNamesGlobal, fnMap);
}

std::vector<std::vector<int>> generateSubsequences(const std::vector<int>& sequence) {
    std::vector<std::vector<int>> subsequences;

    // Parcourir chaque position de départ dans la séquence
    for (size_t start = 0; start < sequence.size(); ++start) {
        // Parcourir chaque position de fin en commençant par la fin de la séquence
        for (size_t end = sequence.size(); end > start ; --end) {
            // Ajouter la sous-séquence de [start, end)
            subsequences.emplace_back(sequence.begin() + start, sequence.begin() + end);
        }
    }

    return subsequences;
}

// Comparateur pour std::set pour comparer deux vecteurs
struct VectorCompare {
    bool operator()(const std::vector<int>& a, const std::vector<int>& b) const {
        return a < b; // Ordre lexicographique
    }
};

int modeApprentissage(){
    // Initial delimiter value
    int delimiterCounter = 1;
    std::string trainingFolder = "../data";

    // Check if the folder exists
    if (!fs::exists(trainingFolder)) {
        std::cerr << "Error: Folder " << trainingFolder << " does not exist." << std::endl;
        return -1;  // Return a non-zero value to indicate an error
    }

    // Declare fnMap inside the function to make it local
    std::map<std::string, int> fnMap;


       for (const auto& entry : fs::directory_iterator(trainingFolder)) {
        if (!entry.is_directory()) {
            continue;
        }

        std::string folderPath = entry.path().string();
        std::cout << "Processing folder: " << folderPath << std::endl;

        int delimiterCounter = 1;
        std::vector<std::string> filenames;

        // Collect all JSON files in the folder
        for (const auto& fileEntry : fs::directory_iterator(folderPath)) {
            if (fileEntry.is_regular_file() && fileEntry.path().extension() == ".json") {
                filenames.push_back(fileEntry.path().string());
            }
        }

    //std::vector<std::string> filenamesUnpacked = {"traces/bootcfg.exe.json","traces/cipher.exe.json",
    //"traces/ping.exe.json","traces/shutdown.exe.json","/Users/eljc3201/Downloads/max_repeats/strmat/traces/msg.exe.json"};
  
      // Utilisation d'un dictionnaire commun pour packés et non packés
    std::vector<int> delimitorsPacked, delimitorsUnpacked;

    std::vector<int> newTrace = processFiles(folderPath,filenames, delimiterCounter, fnMap, delimitorsPacked);

    delimiterCounter = 1; // Réinitialiser pour les traces non packées
    //std::vector<int> traceUnpacked = processFiles(filenamesUnpacked, delimiterCounter, fnMap, delimitorsUnpacked);


    json jsonData = fnMap;
    string filename = "mapFn.json";
    // Write JSON data to the specified file
    std::ofstream outFile12(filename);
    if (outFile12.is_open()) {
        outFile12 << jsonData.dump(4); // Pretty-print with 4 spaces indentation
        outFile12.close();
        std::cout << "fnMap saved to " << filename << " successfully." << std::endl;
    } else {
        std::cerr << "Error: Unable to open file " << filename << " for writing." << std::endl;
    }
   



//   do {
//         std::vector<int> oldTrace = newTrace;

//         // Trouver les répétitions primitives
//         std::vector<Triplet> primitive_repeats = find_primitive_repests(newTrace);

//         // Extraire les longueurs uniques
//         std::vector<int> unique_lengths;
//         for (const auto& triplet : primitive_repeats) {
//             unique_lengths.push_back(triplet.length);
//         }

//         // Supprimer les doublons et trier
//         std::sort(unique_lengths.begin(), unique_lengths.end());
//         unique_lengths.erase(std::unique(unique_lengths.begin(), unique_lengths.end()), unique_lengths.end());

//         // Parcourir les longueurs triées
//         for (int length : unique_lengths) {
//             // Filtrer les triplets de la longueur actuelle
//             std::vector<Triplet> triplets_of_current_length;
//             for (const auto& triplet : primitive_repeats) {
//                 if (triplet.length == length) {
//                     triplets_of_current_length.push_back(triplet);
//                 }
//             }

//             // Remplacer les répétitions pour la longueur actuelle
//             newTrace = replaceTandemRepeats(newTrace, triplets_of_current_length);
//         }

//         // Si la trace n'a pas changé, arrêter la boucle
//         if (newTrace == oldTrace) {
//             break;
//         }

//     } while (true);





    // Affichage de la trace résultante
    std::cout << "Trace finale: " << std::endl;
    std::cout << "Taille: " << newTrace.size() << std::endl;

    for (const auto& elem : newTrace) {
        std::cout << elem << " ";
    }
    std::cout << std::endl;


   
    int numfiles = filenames.size();
    std::vector<int> delimitors;
    for(int i =1; i<= numfiles;i++){
        std::string delim_key = "DELIM_"+folderPath+ std::to_string(i);
        auto it = fnMap.find(delim_key);
        if(it!= fnMap.end()){
            delimitors.push_back(it->second);
        }
    }

    vector<int> delimiterIndices = getDelimiterIndices(newTrace,delimitors);
    std::vector<std::tuple<std::vector<int>, std::vector<int>>> maximal_repeats_with_indices = get_maximal_repeats1(newTrace,100,3);




    std::vector<vector<int>> filtredMaximalRepeats = filterSuperMaximalsInMultipleTraces(maximal_repeats_with_indices,delimiterIndices);

    // Crée une map inverse pour la recherche
    std::unordered_map<int, std::string> inverseMap = invertMap(fnMap);

    // Conversion en chaînes de caractères
    std::vector<std::vector<std::string>> maximalrepeatsstring = convertToStrings(filtredMaximalRepeats, inverseMap);

    // Affichage du résultat
    for (const auto& repeat : maximalrepeatsstring) {
        for (const auto& val : repeat) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
    }
    json j2 = maximalrepeatsstring;
    std::ofstream outFile2(folderPath+"_near_supermaximals_string.json");
    outFile2 << j2.dump(4);
    outFile2.close();


    // Conversion en JSON
    json j1 = filtredMaximalRepeats;
    // Écrire le JSON dans un fichier
    std::ofstream outFile1(folderPath+"_near_supermaximals.json");
    outFile1 << j1.dump(4);
    outFile1.close();}


    return 0;
}





int modeTest() {
   std::vector<std::string> inputDirectories = {"/Users/chaymaeeljabri/Desktop/generation-of-ASTD-specifications/training",
                                             "/Users/chaymaeeljabri/Desktop/generation-of-ASTD-specifications/test"};

    std::string outputRoot = "../results";

    // Créer le dossier principal "results" si nécessaire
    if (!fs::exists(outputRoot)) {
        fs::create_directory(outputRoot);
        std::cout << "Dossier de sortie créé : " << outputRoot << std::endl;
    }

    // Charger le mapping global mapFn.json
    std::map<std::string, int> mapFn;
    if (fs::exists("mapFn.json")) {
        std::ifstream mapFile("mapFn.json");
        if (mapFile.is_open()) {
            json j;
            mapFile >> j;
            for (auto it = j.begin(); it != j.end(); ++it) {
                mapFn[it.key()] = it.value();
            }
            mapFile.close();
        }
    }
    std::cout << "Taille initiale du mapping global mapFn : " << mapFn.size() << std::endl;

    for (const auto& inputDirectory : inputDirectories) {
        // Récupérer le nom du dossier de base ("training" ou "test")
        std::string folderName = fs::path(inputDirectory).filename().string();
        std::string outputDirectory = outputRoot + "/" + folderName;

        if (!fs::exists(outputDirectory)) {
            fs::create_directory(outputDirectory);
            std::cout << "Dossier de sortie créé : " << outputDirectory << std::endl;
        }

        for (const auto& entry : fs::directory_iterator(inputDirectory)) {
            if (entry.path().extension() == ".json") {
                std::string filename = entry.path().string();
                std::cout << "Traitement du fichier : " << filename << std::endl;

                std::ifstream file(filename, std::ios::binary);
                if (!file.is_open()) {
                    std::cerr << "Erreur : Impossible d'ouvrir le fichier " << filename << std::endl;
                    continue;
                }

                std::string line;
                std::vector<json> lines;
                while (std::getline(file, line)) {
                    try {
                        line = removeInvalidUTF8(line);
                        if (!line.empty()) {
                            lines.push_back(json::parse(line));
                        }
                    } catch (const json::parse_error& e) {
                        std::cerr << "Erreur parsing JSON : " << line << " (" << e.what() << ")" << std::endl;
                        break;
                    }
                }
                file.close();

                std::vector<Node> nodes = Node::build_graph(lines);
                std::vector<std::string> functionNames = extractFunctionNames(nodes);

                std::vector<int> numCallSystems = replaceSystemCalls(functionNames, mapFn);

                // Nom du fichier de sortie dans le sous-dossier results/training ou results/test
                std::string outputFilename = outputDirectory + "/" + entry.path().stem().string() + "_converted.json";

                std::ofstream outFile(outputFilename);
                if (outFile.is_open()) {
                    json resultJson = numCallSystems;
                    outFile << resultJson.dump(4);
                    outFile.close();
                    std::cout << "Fichier converti et enregistré : " << outputFilename << std::endl;
                } else {
                    std::cerr << "Erreur : Impossible d'écrire dans le fichier " << outputFilename << std::endl;
                }
            }
        }
    }

    // Sauvegarder le mapping global à la fin
    saveMapFn("mapFn.json", mapFn);
    std::cout << "Mapping global mapFn sauvegardé avec " << mapFn.size() << " entrées." << std::endl;

    return 0;
}
int main(int argc, char *argv[]) {

    std::string mode = argv[1];

    if(mode == "-a"){
        int app = modeApprentissage();
    }else if(mode=="-t"){
        int app = modeTest();
    }
   
    return 0;
}
