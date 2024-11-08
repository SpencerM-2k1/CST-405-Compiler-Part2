#ifndef CODE_GENERATOR_H
#define CODE_GENERATOR_H

#include "AST.h"       // Include your AST definition
#include "semantic.h"  // Include your TAC definition
#include <stdbool.h>

#define NUM_TEMP_REGISTERS 10
#define MAX_CONSTS 100

// MIPSRegister struct definition
typedef struct {
    char* name;  // Name of the register, e.g., "$t0"
    bool inUse;  // Whether the register is currently in use

    // Optional: Mapping temp vars to registers (commented out)
    // struct MappedTemps {
    //     char* tempIDs[10];
    //     int cursor;
    // } mappedTemps;
} MIPSRegister;

// Used to declare constants in memory
typedef struct {
    char* varName;
    char* dataType;
    char* contents;
} DataElement;

extern DataElement* dataConsts[];
extern int constCount;

// Function prototypes
void initCodeGenerator(const char* outputFilename);
void generateMIPS(TAC* tacInstructions, const SymbolTable* table);
void declareMipsVars(const SymbolTable* table);
void finalizeCodeGenerator(const char* outputFilename);

int allocateIntRegister();
void deallocateIntRegister(int regIndex);
int allocateFloatRegister();
void deallocateFloatRegister(int regIndex);

// Int operators
void generateIntAddition(TAC* current);
void generateIntSubtraction(TAC* current);
void generateIntMultiplication(TAC* current);
void generateIntDivision(TAC* current);
void generateIntAssign(TAC* current);
void generateIntWrite(TAC* current);
void generateIntStore(TAC* current);
void generateIntLoad(TAC* current);

// Float operators
void generateFloatAddition(TAC* current);
void generateFloatSubtraction(TAC* current);
void generateFloatMultiplication(TAC* current);
void generateFloatDivision(TAC* current);
void generateFloatAssign(TAC* current);
void generateFloatWrite(TAC* current);
void generateFloatStore(TAC* current);
void generateFloatLoad(TAC* current);

// Array functions
void generateArrIntStore(TAC* current);
void generateArrIntLoad(TAC* current);
void generateArrFloatStore(TAC* current);
void generateArrFloatLoad(TAC* current);

// Function handling
void generateFunctionCall(TAC* current);
void generateFuncStart(TAC* current);
void generateReturn(TAC* current);
void generateFunctionMIPS(FuncTAC* funcTac);

// Type conversion
void generateIntToFloat(TAC* current);

// MappedTemps management (if used)
void mapTemp(MIPSRegister** reg, char* tempID);

DataElement* createConst(const char* type, const char* contents);
void printConstsToFile();

#endif // CODE_GENERATOR_H

