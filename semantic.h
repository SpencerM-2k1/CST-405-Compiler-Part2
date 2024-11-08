#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "AST.h"          // AST structure definitions
#include "symbolTable.h"  // Symbol Table structure definitions
#include "commons/types.h" // Enum listing valid var types
#include "operandStack.h"  // Operand stack definitions

// TAC structure for Intermediate Representation (IR)
// Doubly-linked list
typedef struct TAC {
    char* op;          // Operator
    char* arg1;        // Argument 1
    char* arg2;        // Argument 2
    char* result;      // Result
    struct TAC* prev;  // Pointer to the previous TAC instruction
    struct TAC* next;  // Pointer to the next TAC instruction
} TAC;

// Linked list of TACs to store functions
typedef struct FuncTAC {
    struct TAC* func;
    struct FuncTAC* nextFunc;
    char* funcName;
    VarType returnType;
    bool returnsValue;  //Does this function include a return statement?
} FuncTAC;

// Global variables
extern TAC* tacHead;     // Global head of the TAC list
extern TAC* tacTail;     // Global tail of the TAC list
extern FuncTAC* funcTacHeads; //Func TAC head
extern FuncTAC* funcTacTails; //Func TAC tail

// extern TAC* currentTA


extern SymbolTable* symTabRef;
extern int tempIntCount;
extern int tempFloatCount;

// Function declarations
void initSemantic(SymbolTable* symbolTable);                    // Initialize semantic analyzer
void appendTAC(TAC** head, TAC** tail, TAC* newInstruction);    // Append TAC instruction to list
TAC* createTAC(char* result, char* arg1, char* op, char* arg2); // Create a new TAC instruction
char* createTempVar(VarType type);                              // Create temporary variable
void semanticAnalysis(ASTNode* node);                           // Perform semantic analysis
void handleFunctionDeclaration(ASTNode* node);
void handleParameterList(ASTNode* node);
void handleParameter(ASTNode* node);
void handleFuncVarDeclList(ASTNode* node);
void handleFunctionCall(ASTNode* node);
void handleArgList(ASTNode* node);
void handleArg(ASTNode* node);
void handleReturnStmt(ASTNode* node);
TAC* generateTACForAssign(ASTNode* node);                       // Generate TAC for assignments
TAC* generateTACForAssignArr(ASTNode* node);                    // Generate TAC for array assignments
TAC* generateTACForExpr(ASTNode* expr);                        // Generate TAC for binary operations
TAC* generateTACForWrite(ASTNode* expr);                        // Generate TAC for write statements
void generateTACForFuncCall(ASTNode* funcCall);
VarType getExprType(ASTNode* expr);                             // Get the type of an expression
VarType determineBinOpType(ASTNode* expr);
int getTempIntCount();                                          // Get count of temporary int variables
int getTempFloatCount();                                        // Get count of temporary float variables (corrected to int)
int getTempCharCount();                                        // Get count of temporary float variables (corrected to int)
void printTAC(TAC* tac);
void printTACToFile(const char* filename, TAC* tac);
void printFuncTACsToFile();
void removeTAC(TAC** del);
void freeTAC(TAC** del);                                        // Free memory of a TAC instruction
void replaceTAC(TAC** oldTAC, TAC** newTAC);

void initFuncTAC(char* funcName, VarType returnType);
void finalizeFuncTAC();

#endif // SEMANTIC_H
