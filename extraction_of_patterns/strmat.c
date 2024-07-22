/*
 * strmat.c
 *
 *  GENERAL DESCRIPTION
 *  This file contains the main body of the user interface.
 *  All other functions are accessed through the functions
 *  (directly or indirectly) here.    
 *
 *  ADDING ALGORITHMS
 *  All patterns and texts for particular algorithms are
 *  returned by the get_string function.  The get_string
 *  function returns a *STRING after querying the user.
 *  The get_string function returns NULL if the
 *  user chooses to return the the previous menu, so
 *  calls made to get_string should check for NULL, and
 *  cease execution of the algorithm.
 *
 * NOTES:
 *    8/94  -  Original Implementation (as menu.c) (Gene Fodor)
 *    3/96  -  Combined main.c and menu.c, changed its name to strmat.c
 *             and altered the code as the algorithm code was modularized
 *             (James Knight)
 *    7/96  -  Finished the modularization (James Knight)
 *    2/00  -  Changed type of function main() to int (Jens Stoye)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "strmat.h"




#include "stree_ukkonen.h"
#include "repeats_supermax.h"





#define ON 1
#define OFF 0



static char *choice;
static int ch_len;
static int stree_build_policy = SORTED_LIST;
static int stree_build_threshold = 10;
static int stree_print_flag = ON;
static int stats_flag = ON;


// Function to find the number of unique elements in the list
int countUnique(int *inputString, int size) {
    int uniqueCount = 0;

    // Iterate through the list
    for (int i = 0; i < size; i++) {
        int count = 0;

        // Count the occurrences of inputString[i]
        for (int j = 0; j < size; j++) {
            if (inputString[j] == inputString[i]) {
                count++;
            }
        }

        // If count is 1, inputString[i] is unique
        if (count == 1) {
            uniqueCount++;
        }
    }

    return uniqueCount;
}


/**********************************************************************
 *  Function main()                                              
 *                                                                   
 *  Parameter:                                                       
 * 
 *  This is the main function.  It allocates space for the initial data
 *  structures and displays the main menu. You can choose an algorithm
 *  sub-menu, or the String Utilities Menu.
 *                                                                   
 **********************************************************************/
// int main(void)
// {



//     //   // Define command line arguments
//     // std::string filename;
//     // bool basic_graph = false;

   

//     // filename = argv[1];
  

//     // // Open the JSON file
//     // std::ifstream file(filename);
//     // if (!file.is_open()) {
//     //     std::cerr << "Error: Unable to open file " << filename << std::endl;
//     //     return 1;
//     // }

//     // // Read the JSON content line by line
//     // std::string line;
//     // std::vector<json> lines;
//     // while (std::getline(file, line)) {
//     //     if (!line.empty()) {
//     //         try {
//     //             json jsonLine = json::parse(line);
//     //             lines.push_back(jsonLine);
//     //         } catch (const std::exception& e) {
//     //             std::cerr << "Error: Failed to parse JSON line: " << line << std::endl;
//     //         }
//     //     }
//     // }

//     // // Process the JSON data as needed
//     // // Example: print the parsed JSON lines
//     // for (const auto& jsonLine : lines) {
//     //     std::cout << jsonLine.dump() << std::endl;
//     // }



//     int inputString[] =  {4,1,2,3,4,1,2,3,2,2}; // Input string
//     int size = sizeof(inputString) / sizeof(inputString[0]);
//     int uniqueCount = countUnique(inputString, size);
//     int minPercent = 100; // Minimum percent for supermaximal
//     int minLength = 3; // Minimum length for supermaximal

//     // Find supermaximals in the input string
//     SUPERMAXIMALS supermaximals = supermax_find(inputString, 10, minPercent, minLength);

//     // Output the results
//     printf("Supermaximals found:\n");
//     SUPERMAXIMALS current = supermaximals;
//     while (current != NULL) {
//         printf("Supermaximal substring: ");
//         printf("M : %d\n", current->M);
//         for (int i = 0; i < current->M; i++) {
//             printf("%d ", current->S[i]);
//         }
//         printf("\n");
//         current = current->next;
//     }
  
// } /* main() */

