#include "optimizer.h"
#include "codeGenerator.h"
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Optimize the TAC by applying constant folding, propagation, and dead code elimination.
TAC* optimizeTAC(TAC* head) {
    //Need to use global `tacHead` rather than the parameter because of seg fault errors when the head gets freed in some cases
    //Design-wise? I hate it. It will have to do for now.

    // constantFolding(&head);          // Simplify constant expressions
                                        // (Not really needed due to the structure of our TACs, const propagation handles this)
    constantPropagation(&tacHead);      // Propagate constants through the TAC
    copyPropagation(&tacHead);          // Replace variables with assigned values
    // deadCodeElimination(&head);      // Remove unused assignments
    return head;  // Return the optimized TAC list
}

// Perform constant propagation on TAC instructions.
void constantPropagation(TAC** head) {
    TAC* current = *head;
    while (current != NULL) {
        if (current->op != NULL && (strcmp(current->op, "+") == 0)) {
            // printf("OPTIMIZER: This TAC was detected as addition: \n");
            // printTAC(current);
            
            TAC* arg1Assignment = NULL;    //Stores the TAC which last assigned a result to arg1
            TAC* arg2Assignment = NULL;    //Stores the TAC which last assigned a result to arg2
            if (constantCheck(current, &arg1Assignment, &arg2Assignment))
            {
                // printf("constantCheck() success!\n");
                
                //Convert constants to int, sum them, then convert sum back into a string (convert to helper function later)
                int constSum = atoi(arg1Assignment->arg1) + atoi(arg2Assignment->arg1);
                int sumDigits = snprintf(NULL, 0, "%d", constSum); //Get length of const sum so we can allocate appropriate string length
                char* sumStr = malloc(sumDigits + 1);
                snprintf(sumStr, sumDigits + 1, "%d", constSum);
                // printf("    NEW SUM OF CONSTANTS: %s\n", sumStr);
                
                //Generate a new "assign" TAC
                TAC* replacementTAC = createTAC(current->result, sumStr, "assign", NULL);
                
                //Replace `current` with new TAC, properly linking .prev and .next with neighbors
                replaceTAC(&current, &replacementTAC);
                current = replacementTAC;
                //Destroy arg1Assignment
                removeTAC(&arg1Assignment);
                //Destory arg2Assignment
                removeTAC(&arg2Assignment);

                //Additional cleanup
                free(sumStr); //sumStr is duplicated by replacement; must be destroyed or a memory leak will occur
            }
        } 
        else if (current->op != NULL && strcmp(current->op, "-") == 0) {
            // printf("OPTIMIZER: This TAC was detected as subtraction: \n");
            // printTAC(current);
            
            TAC* arg1Assignment = NULL;    //Stores the TAC which last assigned a result to arg1
            TAC* arg2Assignment = NULL;    //Stores the TAC which last assigned a result to arg2
            if (constantCheck(current, &arg1Assignment, &arg2Assignment))
            {
                // printf("constantCheck() success!\n");
                
                //Convert constants to int, find the difference between them, then convert difference back into a string (convert to helper function later)
                int constSum = atoi(arg1Assignment->arg1) - atoi(arg2Assignment->arg1);
                int sumDigits = snprintf(NULL, 0, "%d", constSum); //Get length of const sum so we can allocate appropriate string length
                char* sumStr = malloc(sumDigits + 1);
                snprintf(sumStr, sumDigits + 1, "%d", constSum);
                // printf("    NEW SUM OF CONSTANTS: %s\n", sumStr);
                
                //Generate a new "assign" TAC
                TAC* replacementTAC = createTAC(current->result, sumStr, "assign", NULL);
                
                //Replace `current` with new TAC, properly linking .prev and .next with neighbors
                replaceTAC(&current, &replacementTAC);
                current = replacementTAC;
                //Destroy arg1Assignment
                removeTAC(&arg1Assignment);
                //Destory arg2Assignment
                removeTAC(&arg2Assignment);

                //Additional cleanup
                free(sumStr); //sumStr is duplicated by replacement; must be destroyed or a memory leak will occur
            }
        }
        current = current->next;
    }
}
//I will be burned at the stake if a professional ever sees how nested this code used to be.   -Spencer

bool constantCheck(TAC* start, TAC** arg1Return, TAC** arg2Return)
{
    TAC* current = start->prev;
    while (current && !(*arg1Return && *arg2Return)) //While both args are not found yet
    {
        // printf("OPTIMIZER: Checking the following TAC for constant assignment... \n");
        // printTAC(current);
        //Check if either arg is listed as the result of the current resultFinder TAC
        if ((!*arg1Return) && (strcmp(start->arg1,current->result) == 0))
        {
            if (strcmp(current->op, "assign") == 0) {
                // printf("OPTIMIZER: Arg1 found! \n");
                *arg1Return = current;
                continue;
            } else {
                // printf("OPTIMIZER: Not a constant! Aborting check... \n");
                return false; //Last assignment of an argument was not a constant assignment, abort check
            }
        }
        if ((!*arg2Return) && (strcmp(start->arg2,current->result) == 0))
        {
            if (strcmp(current->op, "assign") == 0) {
                // printf("OPTIMIZER: Arg2 found! \n");
                *arg2Return = current;
                continue;
            } else {
                // printf("OPTIMIZER: Not a constant! Aborting check... \n");
                return false; //Last assignment of an argument was not a constant assignment, abort check
            }
        }
        current = current->prev;
    }
    if (!current) printf("OPTIMIZER: I am 99 percent sure that 'current' is not supposed to be null right now.\n");
    return true;
}


// Perform copy propagation on TAC instructions.
//      Specifically, if a load operation is encountered, any time the referenced variable is loaded,
//  remove the load operation and instead use the first temp variable loaded into
//      Stop once a store operation is encountered, as this will overwrite the original value of the var
void copyPropagation(TAC** head) {
    TAC* current = *head;

    while (current) {
        if ((current->op) && (strcmp(current->op, "load") == 0)) {
            printf("OPTIMIZER (copyPropagation): The following load operation was detected:\n");
            printTAC(current);
            // char* originalTemp = current->op; //Should only need a shallow copy, original var is not being deleted
            replaceCopies(current);
        }
        else
        {
            printf("Rejected the following operation:\n");
            printTAC(current);
        }
        current = current->next;
    }
}


//Replace all tempVar copies that occur after `start` TAC
//  Assumes `start` is a load instruction
void replaceCopies(TAC* start)
{
    TAC* current = start->next;
    while(current)
    {
        if ((current->result) && (strcmp(current->result, start->arg1) == 0)) { //If a store operation which overwrites our variable is read...
            return; //Stop; our variable has been overwritten
        }

        //A load operation for the same variable occurs before a store operation -- a copy is created
        if ((strcmp(current->op, "load") == 0) && (strcmp(start->arg1, current->arg1) == 0))
        {
            char* redundantVar = current->result; //ID of temp var to be replaced
            TAC* redundancyCursor = current->next;
            while(redundancyCursor)
            {
                if ((redundancyCursor->result) && (strcmp(redundancyCursor->result, start->arg1) == 0)) { //If a store operation which overwrites our variable is read...
                    break; //Stop; our variable has been overwritten
                }
                if (strcmp(redundancyCursor->arg1, redundantVar) == 0) {
                    free(redundancyCursor->arg1);
                    redundancyCursor->arg1 = strdup(start->result);
                }
                if ((redundancyCursor->arg2) && (strcmp(redundancyCursor->arg2, redundantVar) == 0)) {
                    free(redundancyCursor->arg2);
                    redundancyCursor->arg2 = strdup(start->result);
                }

                redundancyCursor = redundancyCursor->next;
            }
            //NOTE: TAC deletion is too buggy for the time being. We need a better
            //  algorithm for liveness analysis.
            // printf("OPTIMIZER(replaceCopies): Deleting the following TAC:\n");
            // printTAC(current);
            // removeTAC(&current);
            return;
        }
        current = current->next;
    }
    //EOF; no more copies to be found
    return;
}



// Perform dead code elimination on TAC instructions.
// void deadCodeElimination(TAC** head) {
//     TAC* current = *head;
//     TAC* prev = NULL;

//     while (current != NULL) {
//         if (current->op != NULL && strcmp(current->op, "assign") == 0) {
//             // Check if the result is ever used
//             int isUsed = 0;
//             TAC* temp = current->next;
//             while (temp != NULL) {
//                 if ((temp->arg1 != NULL && strcmp(temp->arg1, current->result) == 0) ||
//                     (temp->arg2 != NULL && strcmp(temp->arg2, current->result) == 0)) {
//                     isUsed = 1;
//                     break;
//                 }
//                 temp = temp->next;
//             }

//             if (!isUsed) {
//                 // Remove the dead code (the assignment is not used)
//                 if (prev == NULL) {
//                     *head = current->next; // Move head forward
//                 } else {
//                     prev->next = current->next; // Skip the current TAC instruction
//                 }
//                 free(current->op);
//                 free(current->arg1);
//                 free(current->arg2);
//                 free(current->result);
//                 free(current);
//                 current = prev ? prev->next : *head;
//                 continue;
//             }
//         }
//         prev = current;
//         current = current->next;
//     }
// }

// Print optimized TAC to terminal and file
void printOptimizedTAC(const char* filename, TAC* head) {
    
    FILE* outputFile = fopen(filename, "w");
    if (outputFile == NULL) {
        perror("Failed to open output file");
        exit(EXIT_FAILURE);
    }

    // if (head->arg1) printf("arg1 = %s\n", head->arg1);
    // if (head->op) printf("op = %s\n", head->op);
    // if (head->arg2) printf("arg2 = %s\n", head->arg2);
    // if (head->result) printf("result = %s\n", head->result);

    TAC* current = head;
    while (current != NULL) {
        // printf("%s = %s %s %s\n", current->result ? current->result : "(null)", current->arg1 ? current->arg1 : "(null)", current->op ? current->op : "(null)", current->arg2 ? current->arg2 : "(null)"); 
        printf("%s = %s %s %s\n", current->result, current->arg1, current->op, current->arg2);
        fprintf(outputFile, "%s = %s %s %s\n", current->result, current->arg1, current->op, current->arg2);
        current = current->next;
    }

    printf("Optimized TAC written to %s\n", filename);
    fclose(outputFile);
}

