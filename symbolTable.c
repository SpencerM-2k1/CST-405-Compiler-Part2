#include "symbolTable.h"
#include "commons/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Hash function for symbol table (simple mod hash)
unsigned int hash(const char* str, int tableSize) {
    unsigned int hashval = 0;
    while (*str != '\0') {
        hashval = (hashval << 5) + *str++;  // Left shift and add char value
    }
    return hashval % tableSize;
}

// Create a new symbol table
SymbolTable* createSymbolTable(int size) {
    SymbolTable* newTable = (SymbolTable*)malloc(sizeof(SymbolTable));
    if (!newTable) {
        perror("Failed to create symbol table");
        exit(EXIT_FAILURE);
    }
    newTable->table = (Symbol**)malloc(sizeof(Symbol*) * size);
    if (!newTable->table) {
        perror("Failed to allocate memory for symbol table");
        free(newTable);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < size; i++) {
        newTable->table[i] = NULL;
    }
    newTable->currentScope = 0;  // Global scope starts at 0
    return newTable;
}

// Initialize the symbol table with initial capacity
void initSymbolTable(SymbolTable* symTab) {
    symTab->table = (Symbol**)malloc(sizeof(Symbol*) * TABLE_SIZE);
    if (!symTab->table) {
        perror("Failed to allocate memory for symbol table");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < TABLE_SIZE; i++) {
        symTab->table[i] = NULL;
    }
    symTab->currentScope = 0;
}

// Free the memory for the symbol table
void freeSymbolTable(SymbolTable* symTab) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        Symbol* sym = symTab->table[i];
        while (sym) {
            Symbol* nextSym = sym->next;
            free(sym->name);
            // free(sym->type);
            free(sym);
            sym = nextSym;
        }
    }
    free(symTab->table);
    free(symTab);
}

// Enter a new scope
void enterScope(SymbolTable* table) {
    table->currentScope++;
}

// Exit the current scope
void exitScope(SymbolTable* table) {
    table->currentScope--;
}

// Add a symbol to the symbol table
Symbol* addSymbol(SymbolTable* table, const char* name, const char* typeString) {
    if (lookupSymbolInCurrentScope(table, name)) {
        printf("Error: Symbol '%s' already exists in the current scope.\n", name);
        return;
    }

    VarType type;
    if ((strcmp(typeString, "int") == 0) || (strcmp(typeString, "VarType_Int") == 0)) {
    // if (strcmp(typeString, "int") == 0) {
        type = VarType_Int;
    } else if ((strcmp(typeString, "float") == 0)  || (strcmp(typeString, "VarType_Float") == 0)) {
        type = VarType_Float;
    } else if ((strcmp(typeString, "char") == 0)  || (strcmp(typeString, "VarType_Char") == 0)) {
        type = VarType_Char;
    } else if ((strcmp(typeString, "void") == 0)  || (strcmp(typeString, "VarType_Void") == 0)) {
        //Remove this when function declaration is split into its own function
        type = VarType_Void;
        printf("SEMANTIC: WARNING! Void type is only supported for functions.\n");
    } else {
        printf("ERROR: VarType %s not recognized. Halting compilation...\n", varTypeToString(type));
        printf("offending symbol: %s\n", name);
        exit(1);
    }

    unsigned int hashval = hash(name, TABLE_SIZE);
    Symbol* newSymbol = (Symbol*)malloc(sizeof(Symbol));
    newSymbol->name = strdup(name);
    printf("newSymbol->name: %s\n",newSymbol->name);
    newSymbol->type = type;
    newSymbol->scopeLevel = table->currentScope;
    newSymbol->next = table->table[hashval];

    //Array elements are unused
    newSymbol->isArray = false;
    newSymbol->arrSize = 0;

    //Function parameters are unused for non-functions
    //  Specialized function will be added to assign params
    newSymbol->params = NULL;

    table->table[hashval] = newSymbol;

    return newSymbol; //Allows the created symbol to be immediately accessed if necessary
}

// Add an array symbol to the symbol table
void addArrSymbol(SymbolTable* table, const char* name, const char* typeString, int size) {
    if (size <= 0) {
        printf("ERROR: Array size must be greater than zero.");
        exit(1);
    }
    if (lookupSymbolInCurrentScope(table, name)) {
        printf("Error: Symbol '%s' already exists in the current scope.\n", name);
        return;
    }

    VarType type;
    if ((strcmp(typeString, "int") == 0) || (strcmp(typeString, "VarType_Int") == 0)) {
    // if (strcmp(typeString, "int") == 0) {
        type = VarType_Int;
    } else if ((strcmp(typeString, "float") == 0)  || (strcmp(typeString, "VarType_Float") == 0)) {
        type = VarType_Float;
    } else if ((strcmp(typeString, "char") == 0)  || (strcmp(typeString, "VarType_Char") == 0)) {
        type = VarType_Char;
    } else if ((strcmp(typeString, "void") == 0)  || (strcmp(typeString, "VarType_Void") == 0)) {
        //Remove this when function declaration is split into its own function
        type = VarType_Void;
        printf("SEMANTIC: WARNING! Void type is only supported for functions.\n");
    } else {
        printf("ERROR: VarType %s not recognized. Halting compilation...\n", varTypeToString(type));
        printf("offending symbol: %s\n", name);
        exit(1);
    }

    unsigned int hashval = hash(name, TABLE_SIZE);
    Symbol* newSymbol = (Symbol*)malloc(sizeof(Symbol));
    newSymbol->name = strdup(name);
    printf("newSymbol->name: %s\n",newSymbol->name);
    newSymbol->type = type;
    newSymbol->scopeLevel = table->currentScope;
    newSymbol->next = table->table[hashval];

    //Set array elements
    newSymbol->isArray = true;
    newSymbol->arrSize = size;

    //Arrays are always non-functions
    newSymbol->params = NULL;

    table->table[hashval] = newSymbol;
}

//Add a parameter to a function symbol
//  Now that I'm writing this, I don't like that repeatedly calling this method is 
//  O(n^2) time. Should probably just handle parameters when a func symbol is declared,
//  maybe with a unique addFuncSymbol function

//  (Or maybe not. Programmers reasonably won't be using enough arguments for O(n^2) time
//   complexity to matter. Still something to think about.) -Spencer
void addParameter(Symbol* symbol, const char* name, VarType type) {
    
    FuncParam* newParam = malloc(sizeof(FuncParam));
    newParam->name = strdup(name);  //Name duplication might be unnecessary (source ASTNode won't be freed until program ends)
                                    //...However, I'm not taking chances.
    newParam->type = type;
    newParam->next = NULL;
    printf("newParam->name: %s\n",newParam->name);
    printf("newParam->type: %s\n",varTypeToString(newParam->type));
    
    printf("SYMBOL_TABLE: Adding parameter %s %s to function %s\n",varTypeToString(type), name, symbol->name);

    if (!symbol->params) { //Insert into empty linked list
        newParam->prev = NULL;
        symbol->params = newParam;
    } else {                //Insert into existing linked list
        FuncParam** current = &(symbol->params);
        while ((*current)->next)
        {
            current = &((*current)->next); //Traverse to next param until end of list
        }
        newParam->prev = (*current);
        (*current)->next = newParam; //When next element is null, insert newParam into next
    }

}

// Lookup a symbol in the symbol table
Symbol* lookupSymbol(SymbolTable* table, const char* name) {
    // printf("lookupSymbol(\"%s\")\n", name);
    unsigned int hashval = hash(name, TABLE_SIZE);
    for (Symbol* sym = table->table[hashval]; sym != NULL; sym = sym->next) {
        if (strcmp(name, sym->name) == 0) {
            return sym;
        }
    }
    return NULL;  // Symbol not found
}

// Lookup a symbol in the current scope only
Symbol* lookupSymbolInCurrentScope(SymbolTable* table, const char* name) {
    unsigned int hashval = hash(name, TABLE_SIZE);
    for (Symbol* sym = table->table[hashval]; sym != NULL; sym = sym->next) {
        if (strcmp(name, sym->name) == 0 && sym->scopeLevel == table->currentScope) {
            return sym;
        }
    }
    return NULL;
}

FuncParam* getParamsTail(Symbol* symbol) {
    
    if (symbol->params) { //If params is not null
        FuncParam** paramsTail = &(symbol->params);
        // printf("param: %s %s\n", varTypeToString((*paramsTail)->type), (*paramsTail)->name);
        while ((*paramsTail)->next)
        {
            printf("param: %s %s\n", varTypeToString((*paramsTail)->type), (*paramsTail)->name);
            // printf("&param: %d\n", (*paramsTail));
            // printf("param->next: %d\n", (*paramsTail)->next);
            paramsTail = &((*paramsTail)->next);
        }
        // printf("paramsTail: %s %s\n", varTypeToString((*paramsTail)->type), (*paramsTail)->name);    //Debug
        return (*paramsTail);
    }

    return NULL;
}

// Print the symbol table contents
void printSymbolTable(SymbolTable* table) {
    printf("Symbol Table:\n");
    for (int i = 0; i < TABLE_SIZE; i++) {
        Symbol* sym = table->table[i];
        while (sym != NULL) {
            if (!sym->isArray) { //Non-array
                printf("Name: %s, Type: %s, Scope Level: %d\n", sym->name, varTypeToString(sym->type), sym->scopeLevel);
            } else { //Array
                printf("Name: %s, Type: %s[%d], Scope Level: %d\n", sym->name, varTypeToString(sym->type), sym->arrSize, sym->scopeLevel);
            }
            if (sym->params) printf("\t^function\n");
            sym = sym->next;
        }
    }
}

//Appends _var to end of var in order to prevent conflict with reserved keywords
char* getMipsVarName(char* varName, char* functionName) {
    if (varName == NULL) {
        fprintf(stderr, "Error: varName is NULL.\n");
        return NULL;
    }

    int idLength = 0;
    char* varID = NULL;

    if (functionName)   //In function scope
    {
        idLength = snprintf(NULL, 0, "%s.%s_var", functionName, varName); //Get length of const sum so we can allocate appropriate string length
        varID = malloc(idLength + 1);
        snprintf(varID, idLength + 1, "%s.%s_var", functionName, varName);
    }
    else                //Out of function scope
    {
        idLength = snprintf(NULL, 0, "%s_var", varName); //Get length of const sum so we can allocate appropriate string length
        varID = malloc(idLength + 1);
        snprintf(varID, idLength + 1, "%s_var", varName);
    }
    printf("getMipsVarName(): %s --> %s\n", varName, varID);
	return varID;
}

//
