#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "semantic.h"
#include <stdbool.h>
#include <ctype.h>

/**
 * Perform a series of optimizations on the given TAC list.
 * This function applies constant folding, constant propagation,
 * copy propagation, and dead code elimination.
 *
 * @param head Pointer to the head of the TAC list.
 * @return The optimized TAC list (head of the list).
 */
TAC* optimizeTAC(TAC* head);

/**
 * Check if a string represents an integer constant.
 * 
 * @param str The string to check.
 * @return true if the string is an integer constant, false otherwise.
 */
TAC* isConstant(const char* str, TAC* current);

/**
 * Check if a string represents a valid variable name according to
 * a simple naming convention.
 * 
 * @param str The string to check.
 * @return true if the string is a valid variable name, false otherwise.
 */
bool isVariable(const char* str);

/**
 * Perform constant folding on the TAC list. This optimization evaluates
 * expressions involving constants at compile time.
 *
 * @param head Pointer to the head of the TAC list.
 */
void constantFolding(TAC** head);

/**
 * Perform constant propagation on the TAC list. This optimization replaces
 * variables with known constant values.
 *
 * @param head Pointer to the head of the TAC list.
 */
void constantPropagation(TAC** head);

/**
 * Perform check for potential constant propogation.
 *
 * @param start Pointer to starting TAC to backtrack from.
 * @param arg1Assignment Returns TAC of last assignment of temp var in arg1
 * @param arg2Assignment Returns TAC of last assignment of temp var in arg2
 * @return true if both args of TAC were last assigned a constant, false otherwise.
 */
bool constantCheck(TAC* start, TAC** arg1Assignment, TAC** arg2Assignment);

/**
 * Perform copy propagation on the TAC list. This optimization replaces uses
 * of a variable that has been assigned the value of another variable.
 *
 * @param head Pointer to the head of the TAC list.
 */
void copyPropagation(TAC** head);

void replaceCopies(TAC* start);

/**
 * Perform dead code elimination on the TAC list. This optimization removes
 * instructions that compute values not used by subsequent instructions or
 * the program's output.
 *
 * @param head Pointer to the head of the TAC list.
 */
void deadCodeElimination(TAC** head);

/**
 * Print the optimized TAC list to a file and also print to the terminal.
 *
 * @param filename The output file where the optimized TAC will be saved.
 * @param head Pointer to the head of the TAC list.
 */
void printOptimizedTAC(const char* filename, TAC* head);


// void mapTemps(TAC** head);

#endif // OPTIMIZER_H

