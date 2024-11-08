// codeGenerator.c
#include "codeGenerator.h"
#include "semantic.h" // For TAC and FuncTAC definitions
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


FILE* outputFile; // Global file pointer for output

// Add prototypes at the top of the file or ensure they are included via codeGenerator.h
void generateArrIntStore(TAC* current);
void generateArrIntLoad(TAC* current);
void generateArrFloatStore(TAC* current);
void generateArrFloatLoad(TAC* current);
extern SymbolTable* symTab; // Add this line to access symTab

DataElement* dataConsts[MAX_CONSTS];
int constCount = 0;

// Array of temporary registers for integers
MIPSRegister tempIntRegisters[NUM_TEMP_REGISTERS] = {
    {"$t0", false}, {"$t1", false}, {"$t2", false}, {"$t3", false},
    {"$t4", false}, {"$t5", false}, {"$t6", false}, {"$t7", false},
    {"$t8", false}, {"$t9", false}
};

// Array of temporary registers for floats
MIPSRegister tempFloatRegisters[NUM_TEMP_REGISTERS] = {
    {"$f0", false}, {"$f1", false}, {"$f2", false}, {"$f3", false},
    {"$f4", false}, {"$f5", false}, {"$f6", false}, {"$f7", false},
    {"$f8", false}, {"$f9", false}
};

// External declaration of funcTacHeads
extern FuncTAC* funcTacHeads;

// Function prototypes for new functions
void generateFunctionCall(TAC* current);
void generateFuncStart(TAC* current);
void generateReturn(TAC* current);
void generateFunctionMIPS(FuncTAC* funcTac);

// Initialize the code generator and open the file where the output will be saved
void initCodeGenerator(const char* outputFilename) {
    outputFile = fopen(outputFilename, "w"); // Open in write mode
    if (outputFile == NULL) {
        perror("Failed to open output file");
        exit(EXIT_FAILURE);
    }

    // Start the MIPS code
    fprintf(outputFile, ".text\n");
    fprintf(outputFile, ".globl main\n");
    fprintf(outputFile, "main:\n");
}

// Translate TAC instructions to MIPS assembly and output to a file
void generateMIPS(TAC* tacInstructions, const SymbolTable* table) {
    TAC* current = tacInstructions;

    while (current != NULL) {
        // Handle function-specific TACs
        if (strcmp(current->op, "functionCall") == 0) {
            generateFunctionCall(current);
        } else if (strcmp(current->op, "funcStart") == 0) {
            // funcStart is handled within generateFunctionMIPS
            // Skip here
        } else if (strcmp(current->op, "return") == 0) {
            // return is handled within generateFunctionMIPS
            // Skip here
        } else {
            // Handle other TAC instructions
            if (strcmp(current->op, "assign.int") == 0) {
                generateIntAssign(current);
            } else if (strcmp(current->op, "+.int") == 0) {
                generateIntAddition(current);
            } else if (strcmp(current->op, "-.int") == 0) {
                generateIntSubtraction(current);
            } else if (strcmp(current->op, "*.int") == 0) {
                generateIntMultiplication(current);
            } else if (strcmp(current->op, "/.int") == 0) {
                generateIntDivision(current);
            } else if (strcmp(current->op, "write.int") == 0) {
                generateIntWrite(current);
            } else if (strcmp(current->op, "store.int") == 0) {
                generateIntStore(current);
            } else if (strcmp(current->op, "load.int") == 0) {
                generateIntLoad(current);
            } else if (strcmp(current->op, "assign.float") == 0) {
                generateFloatAssign(current);
            } else if (strcmp(current->op, "+.float") == 0) {
                generateFloatAddition(current);
            } else if (strcmp(current->op, "-.float") == 0) {
                generateFloatSubtraction(current);
            } else if (strcmp(current->op, "*.float") == 0) {
                generateFloatMultiplication(current);
            } else if (strcmp(current->op, "/.float") == 0) {
                generateFloatDivision(current);
            } else if (strcmp(current->op, "write.float") == 0) {
                generateFloatWrite(current);
            } else if (strcmp(current->op, "store.float") == 0) {
                generateFloatStore(current);
            } else if (strcmp(current->op, "load.float") == 0) {
                generateFloatLoad(current);
            } else if (strcmp(current->op, "assign.char") == 0) { //Char operations
                generateCharAssign(current);
            } else if (strcmp(current->op, "write.char") == 0) {
                generateCharWrite(current);
            } else if (strcmp(current->op, "store.char") == 0) {
                generateCharStore(current);
            } else if (strcmp(current->op, "load.char") == 0) {
                generateCharLoad(current);
            } else if (strcmp(current->op, "store.intIndex") == 0) {    //Array operations
                generateArrIntStore(current);
            } else if (strcmp(current->op, "load.intIndex") == 0) {
                generateArrIntLoad(current);
            } else if (strcmp(current->op, "store.floatIndex") == 0) {
                generateArrFloatStore(current);
            } else if (strcmp(current->op, "load.floatIndex") == 0) {
                generateArrFloatLoad(current);
            } else if (strcmp(current->op, "store.charIndex") == 0) {
                generateArrCharStore(current);
            } else if (strcmp(current->op, "load.charIndex") == 0) {
                generateArrCharLoad(current);
            } else if (strcmp(current->op, "intToFloat") == 0) {    //Type conversion operators
                generateIntToFloat(current);
            } else if (strcmp(current->op, "floatToInt") == 0) {
                generateFloatToInt(current);
            }
            // Add more cases as needed
        }
        current = current->next;
    }
    //End of main, generate EXIT syscall
    fprintf(outputFile, "\tli $v0, 10 #EXIT\n");
    fprintf(outputFile, "\tsyscall\n");

    // Generate MIPS code for functions after main
    FuncTAC* currentFunc = funcTacHeads;
    while (currentFunc != NULL) {
        generateFunctionMIPS(currentFunc);
        currentFunc = currentFunc->nextFunc; // Correct member name
    }	
}

// Finalize the code generation and close the output file
void finalizeCodeGenerator(const char* outputFilename) {
    // Exit the program
    // fprintf(outputFile, "\tli $v0, 10 #END\n");
    // fprintf(outputFile, "\tsyscall\n");

    // Append data segment
    fprintf(outputFile, "\n.data\n");
    fprintf(outputFile, "   newline: .asciiz \"\\n\"\n");
    fprintf(outputFile, "   returnInt: .word 0\n");
    fprintf(outputFile, "   returnFloat: .float 0.0\n");

    declareMipsVars(symTab);
    printConstsToFile();

    if (outputFile) {
        fclose(outputFile);
        printf("MIPS code generated and saved to file %s\n", outputFilename);
        outputFile = NULL;
    }
}

// Function to handle function calls
void generateFunctionCall(TAC* current) {
    // Jump and link to the function label
    fprintf(outputFile, "\tjal %s #FUNCTION CALL\n", current->arg1); // arg1 contains the function name with "_func" suffix
}

// Function to handle function definitions
void generateFunctionMIPS(FuncTAC* funcTac) {
    // Print the function label
    fprintf(outputFile, "%s:\n", funcTac->funcName);

    TAC* current = funcTac->func; // Correct member name

    while (current != NULL) {
        if (strcmp(current->op, "funcStart") == 0) {
            generateFuncStart(current);
        } else if (strcmp(current->op, "return") == 0) {
            generateReturn(current);
        }else if (strcmp(current->op, "functionCall") == 0) {
            generateFunctionCall(current);
        } else {
            // Handle other TAC instructions within the function
            if (strcmp(current->op, "assign.int") == 0) {   //Int operations
                generateIntAssign(current);
            } else if (strcmp(current->op, "+.int") == 0) {
                generateIntAddition(current);
            } else if (strcmp(current->op, "-.int") == 0) {
                generateIntSubtraction(current);
            } else if (strcmp(current->op, "*.int") == 0) {
                generateIntMultiplication(current);
            } else if (strcmp(current->op, "/.int") == 0) {
                generateIntDivision(current);
            } else if (strcmp(current->op, "write.int") == 0) {
                generateIntWrite(current);
            } else if (strcmp(current->op, "store.int") == 0) {
                generateIntStore(current);
            } else if (strcmp(current->op, "load.int") == 0) {
                generateIntLoad(current);
            } else if (strcmp(current->op, "assign.float") == 0) { //Float operations
                generateFloatAssign(current);
            } else if (strcmp(current->op, "+.float") == 0) {
                generateFloatAddition(current);
            } else if (strcmp(current->op, "-.float") == 0) {
                generateFloatSubtraction(current);
            } else if (strcmp(current->op, "*.float") == 0) {
                generateFloatMultiplication(current);
            } else if (strcmp(current->op, "/.float") == 0) {
                generateFloatDivision(current);
            } else if (strcmp(current->op, "write.float") == 0) {
                generateFloatWrite(current);
            } else if (strcmp(current->op, "store.float") == 0) {
                generateFloatStore(current);
            } else if (strcmp(current->op, "load.float") == 0) {
                generateFloatLoad(current);
            } else if (strcmp(current->op, "assign.char") == 0) { //Char operations
                generateCharAssign(current);
            } else if (strcmp(current->op, "write.char") == 0) {
                generateCharWrite(current);
            } else if (strcmp(current->op, "store.char") == 0) {
                generateCharStore(current);
            } else if (strcmp(current->op, "load.char") == 0) {
                generateCharLoad(current);
            } else if (strcmp(current->op, "store.intIndex") == 0) { //Array operations
                generateArrIntStore(current);
            } else if (strcmp(current->op, "load.intIndex") == 0) {
                generateArrIntLoad(current);
            } else if (strcmp(current->op, "store.floatIndex") == 0) {
                generateArrFloatStore(current);
            } else if (strcmp(current->op, "load.floatIndex") == 0) {
                generateArrFloatLoad(current);
            } else if (strcmp(current->op, "intToFloat") == 0) {    //Type conversion operators
                generateIntToFloat(current);
            } else if (strcmp(current->op, "floatToInt") == 0) {
                generateFloatToInt(current);
            }
            // Add more cases as needed
        }
        current = current->next;
    }
}

// Function to handle the start of a function
void generateFuncStart(TAC* current) {
    // Push the return address onto the stack
    fprintf(outputFile, "\taddi $sp, $sp, -4 #FUNCTION START\n");
    fprintf(outputFile, "\tsw $ra, 0($sp)\n");
}

// Function to handle function returns
void generateReturn(TAC* current) {
    // Pop the return address from the stack
    fprintf(outputFile, "\tlw $ra, 0($sp) #RETURN\n");
    fprintf(outputFile, "\taddi $sp, $sp, 4\n");
    // Return to the caller
    fprintf(outputFile, "\tjr $ra\n");
}

// --- Existing functions for integer and float operations go here ---
// You can include your existing generateIntAddition, generateIntSubtraction, etc.

// For brevity, I'll include one example:

// Example: Integer Addition
void generateIntAddition(TAC* current) {
    int regIndex1, regIndex2;

    // Allocate registers
    regIndex1 = allocateIntRegister();
    if (regIndex1 == -1) {
        printf("Error: No available registers\n");
        return;
    }
    regIndex2 = allocateIntRegister();
    if (regIndex2 == -1) {
        printf("Error: No available registers\n");
        deallocateIntRegister(regIndex1);
        return;
    }

    // Load operands
    fprintf(outputFile, "\tlw %s, %s #ADD INT\n", tempIntRegisters[regIndex1].name, current->arg1);
    fprintf(outputFile, "\tlw %s, %s\n", tempIntRegisters[regIndex2].name, current->arg2);

    // Perform addition
    fprintf(outputFile, "\tadd %s, %s, %s\n",
            tempIntRegisters[regIndex1].name,
            tempIntRegisters[regIndex1].name,
            tempIntRegisters[regIndex2].name);

    // Store result
    fprintf(outputFile, "\tsw %s, %s\n", tempIntRegisters[regIndex1].name, current->result);

    // Deallocate registers
    deallocateIntRegister(regIndex1);
    deallocateIntRegister(regIndex2);
}

// --- Rest of your operation functions should be included similarly ---

// Allocate a register from the temp register pool
int allocateIntRegister() {
    for (int i = 0; i < NUM_TEMP_REGISTERS; i++) {
        if (!tempIntRegisters[i].inUse) {
            tempIntRegisters[i].inUse = true;
            return i; // Return the register index
        }
    }
    return -1; // Indicate failure (no available register)
}

int allocateFloatRegister() {
    for (int i = 0; i < NUM_TEMP_REGISTERS; i++) {
        if (!tempFloatRegisters[i].inUse) {
            tempFloatRegisters[i].inUse = true;
            return i; // Return the register index
        }
    }
    return -1; // Indicate failure (no available register)
}

// Deallocate a register (mark it as free)
void deallocateIntRegister(int regIndex) {
    if (regIndex >= 0 && regIndex < NUM_TEMP_REGISTERS) {
        tempIntRegisters[regIndex].inUse = false;
    }
}

void deallocateFloatRegister(int regIndex) {
    if (regIndex >= 0 && regIndex < NUM_TEMP_REGISTERS) {
        tempFloatRegisters[regIndex].inUse = false;
    }
}

// Create a const to be included at the end of the MIPS file
DataElement* createConst(const char* type, const char* contents) {
    dataConsts[constCount] = malloc(sizeof(DataElement));
    dataConsts[constCount]->dataType = strdup(type);
    dataConsts[constCount]->contents = strdup(contents);

    char buffer[30];
    sprintf(buffer, "%s_const_%d", type, constCount);
    dataConsts[constCount]->varName = strdup(buffer);
    constCount++;

    return dataConsts[constCount - 1];
}

// Print constants to the output file
void printConstsToFile() {
    for (int i = 0; i < constCount; i++) {
        fprintf(outputFile, "\t%s: .%s %s\n",
                dataConsts[i]->varName,
                dataConsts[i]->dataType,
                dataConsts[i]->contents);
    }
}

// Declare variables in MIPS
void declareMipsVars(const SymbolTable* table) {
    for (int i = 0; i < 100; i++) {
        Symbol* current = table->table[i];
        while (current) {
            char* repeatToken = NULL;
            switch (current->type)
            {
                case (VarType_Int):
                    fprintf(outputFile, "\t%s: .word 0", current->name);
                    if (current->isArray) repeatToken = ", 0";
                    break;
                
                case (VarType_Float):
                    fprintf(outputFile, "\t%s: .float 0.0", current->name);
                    if (current->isArray) repeatToken = ", 0.0";
                    break;

                case (VarType_Char):
                    if (current->isArray) {
                        fprintf(outputFile, "\t%s: .asciiz \"U", current->name);
                        repeatToken = "U";
                    } else {
                        fprintf(outputFile, "\t%s: .byte 'U'", current->name);
                    }
                    break;
                
                default:
                    printf("Invalid VarType in declareMipsVars(): %s\n",varTypeToString(current->type));
                    break;
            }
            
            if (current->isArray) {
                for (int j = 1; j < current->arrSize; j++) {
                    fprintf(outputFile, "%s", repeatToken);
                }
                if (current->type == VarType_Char) { //Special case: close quotations on string
                    fprintf(outputFile, "\"");
                }
            }
            fprintf(outputFile, "\n");
            current = current->next;
        }
    }
    // Declare temporary variables
    for (int i = 0; i < getTempIntCount(); i++) {
        fprintf(outputFile, "\ti%d: .word 0\n", i);
    }
    for (int i = 0; i < getTempFloatCount(); i++) {
        fprintf(outputFile, "\tf%d: .float 0.0\n", i);
    }
    for (int i = 0; i < getTempCharCount(); i++) {
        fprintf(outputFile, "\tc%d: .byte 0\n", i);
    }
}

// --- Include your existing functions for array operations, float operations, etc. ---

// Example: Integer Assignment
void generateIntAssign(TAC* current) {
    int regIndex;

    regIndex = allocateIntRegister();
    if (regIndex == -1) {
        printf("Error: No available registers\n");
        return;
    }

    // Load immediate value
    fprintf(outputFile, "\tli %s, %s #ASSIGN INT VALUE\n", tempIntRegisters[regIndex].name, current->arg1);

    // Store the value into the result variable
    fprintf(outputFile, "\tsw %s, %s\n", tempIntRegisters[regIndex].name, current->result);

    deallocateIntRegister(regIndex);
}

// Continue with the rest of your operation functions

// Integer Subtraction
void generateIntSubtraction(TAC* current) {
    int regIndex1, regIndex2;

    // Allocate registers
    regIndex1 = allocateIntRegister();
    regIndex2 = allocateIntRegister();

    // Error checking
    if (regIndex1 == -1 || regIndex2 == -1) {
        printf("Error: No available integer registers\n");
        return;
    }

    // Load operands
    fprintf(outputFile, "\tlw %s, %s #SUBTRACT INT\n",
            tempIntRegisters[regIndex1].name, current->arg1);
    fprintf(outputFile, "\tlw %s, %s\n",
            tempIntRegisters[regIndex2].name, current->arg2);

    // Perform subtraction
    fprintf(outputFile, "\tsub %s, %s, %s\n",
            tempIntRegisters[regIndex1].name,
            tempIntRegisters[regIndex1].name,
            tempIntRegisters[regIndex2].name);

    // Store result
    fprintf(outputFile, "\tsw %s, %s\n",
            tempIntRegisters[regIndex1].name, current->result);

    // Deallocate registers
    deallocateIntRegister(regIndex1);
    deallocateIntRegister(regIndex2);
}

// Integer Multiplication
void generateIntMultiplication(TAC* current) {
    int regIndex1, regIndex2;

    // Allocate registers
    regIndex1 = allocateIntRegister();
    regIndex2 = allocateIntRegister();

    // Error checking
    if (regIndex1 == -1 || regIndex2 == -1) {
        printf("Error: No available integer registers\n");
        return;
    }

    // Load operands
    fprintf(outputFile, "\tlw %s, %s #MULTIPLY INT\n",
            tempIntRegisters[regIndex1].name, current->arg1);
    fprintf(outputFile, "\tlw %s, %s\n",
            tempIntRegisters[regIndex2].name, current->arg2);

    // Perform multiplication
    fprintf(outputFile, "\tmul %s, %s, %s\n",
            tempIntRegisters[regIndex1].name,
            tempIntRegisters[regIndex1].name,
            tempIntRegisters[regIndex2].name);

    // Store result
    fprintf(outputFile, "\tsw %s, %s\n",
            tempIntRegisters[regIndex1].name, current->result);

    // Deallocate registers
    deallocateIntRegister(regIndex1);
    deallocateIntRegister(regIndex2);
}

// Integer Division
void generateIntDivision(TAC* current) {
    int regIndex1, regIndex2;

    // Allocate registers
    regIndex1 = allocateIntRegister();
    regIndex2 = allocateIntRegister();

    // Error checking
    if (regIndex1 == -1 || regIndex2 == -1) {
        printf("Error: No available integer registers\n");
        return;
    }

    // Load operands
    fprintf(outputFile, "\tlw %s, %s #DIVIDE INT\n",
            tempIntRegisters[regIndex1].name, current->arg1);
    fprintf(outputFile, "\tlw %s, %s\n",
            tempIntRegisters[regIndex2].name, current->arg2);

    // Perform division
    fprintf(outputFile, "\tdiv %s, %s\n",
            tempIntRegisters[regIndex1].name, tempIntRegisters[regIndex2].name);

    // Move the quotient to the destination register
    fprintf(outputFile, "\tmflo %s\n", tempIntRegisters[regIndex1].name);

    // Store result
    fprintf(outputFile, "\tsw %s, %s\n",
            tempIntRegisters[regIndex1].name, current->result);

    // Deallocate registers
    deallocateIntRegister(regIndex1);
    deallocateIntRegister(regIndex2);
}

// Float Addition
void generateFloatAddition(TAC* current) {
    int regIndex1, regIndex2;

    // Allocate registers
    regIndex1 = allocateFloatRegister();
    regIndex2 = allocateFloatRegister();

    // Error checking
    if (regIndex1 == -1 || regIndex2 == -1) {
        printf("Error: No available float registers\n");
        return;
    }

    // Load operands
    fprintf(outputFile, "\tl.s %s, %s #ADD FLOAT\n",
            tempFloatRegisters[regIndex1].name, current->arg1);
    fprintf(outputFile, "\tl.s %s, %s\n",
            tempFloatRegisters[regIndex2].name, current->arg2);

    // Perform addition
    fprintf(outputFile, "\tadd.s %s, %s, %s\n",
            tempFloatRegisters[regIndex1].name,
            tempFloatRegisters[regIndex1].name,
            tempFloatRegisters[regIndex2].name);

    // Store result
    fprintf(outputFile, "\ts.s %s, %s\n",
            tempFloatRegisters[regIndex1].name, current->result);

    // Deallocate registers
    deallocateFloatRegister(regIndex1);
    deallocateFloatRegister(regIndex2);
}

// Float Subtraction
void generateFloatSubtraction(TAC* current) {
    int regIndex1, regIndex2;

    // Allocate registers
    regIndex1 = allocateFloatRegister();
    regIndex2 = allocateFloatRegister();

    // Error checking
    if (regIndex1 == -1 || regIndex2 == -1) {
        printf("Error: No available float registers\n");
        return;
    }

    // Load operands
    fprintf(outputFile, "\tl.s %s, %s #SUBTRACT FLOAT\n",
            tempFloatRegisters[regIndex1].name, current->arg1);
    fprintf(outputFile, "\tl.s %s, %s\n",
            tempFloatRegisters[regIndex2].name, current->arg2);

    // Perform subtraction
    fprintf(outputFile, "\tsub.s %s, %s, %s\n",
            tempFloatRegisters[regIndex1].name,
            tempFloatRegisters[regIndex1].name,
            tempFloatRegisters[regIndex2].name);

    // Store result
    fprintf(outputFile, "\ts.s %s, %s\n",
            tempFloatRegisters[regIndex1].name, current->result);

    // Deallocate registers
    deallocateFloatRegister(regIndex1);
    deallocateFloatRegister(regIndex2);
}

// Float Multiplication
void generateFloatMultiplication(TAC* current) {
    int regIndex1, regIndex2;

    // Allocate registers
    regIndex1 = allocateFloatRegister();
    regIndex2 = allocateFloatRegister();

    // Error checking
    if (regIndex1 == -1 || regIndex2 == -1) {
        printf("Error: No available float registers\n");
        return;
    }

    // Load operands
    fprintf(outputFile, "\tl.s %s, %s #MULTIPLY FLOAT\n",
            tempFloatRegisters[regIndex1].name, current->arg1);
    fprintf(outputFile, "\tl.s %s, %s\n",
            tempFloatRegisters[regIndex2].name, current->arg2);

    // Perform multiplication
    fprintf(outputFile, "\tmul.s %s, %s, %s\n",
            tempFloatRegisters[regIndex1].name,
            tempFloatRegisters[regIndex1].name,
            tempFloatRegisters[regIndex2].name);

    // Store result
    fprintf(outputFile, "\ts.s %s, %s\n",
            tempFloatRegisters[regIndex1].name, current->result);

    // Deallocate registers
    deallocateFloatRegister(regIndex1);
    deallocateFloatRegister(regIndex2);
}

// Float Division
void generateFloatDivision(TAC* current) {
    int regIndex1, regIndex2;

    // Allocate registers
    regIndex1 = allocateFloatRegister();
    regIndex2 = allocateFloatRegister();

    // Error checking
    if (regIndex1 == -1 || regIndex2 == -1) {
        printf("Error: No available float registers\n");
        return;
    }

    // Load operands
    fprintf(outputFile, "\tl.s %s, %s #DIVIDE FLOAT\n",
            tempFloatRegisters[regIndex1].name, current->arg1);
    fprintf(outputFile, "\tl.s %s, %s\n",
            tempFloatRegisters[regIndex2].name, current->arg2);

    // Perform division
    fprintf(outputFile, "\tdiv.s %s, %s, %s\n",
            tempFloatRegisters[regIndex1].name,
            tempFloatRegisters[regIndex1].name,
            tempFloatRegisters[regIndex2].name);

    // Store result
    fprintf(outputFile, "\ts.s %s, %s\n",
            tempFloatRegisters[regIndex1].name, current->result);

    // Deallocate registers
    deallocateFloatRegister(regIndex1);
    deallocateFloatRegister(regIndex2);
}

// Integer Write
void generateIntWrite(TAC* current) {
    int regIndex;

    // Allocate register
    regIndex = allocateIntRegister();

    // Error checking
    if (regIndex == -1) {
        printf("Error: No available integer registers\n");
        return;
    }

    // Load the value to print
    fprintf(outputFile, "\tlw %s, %s #WRITE INT\n",
            tempIntRegisters[regIndex].name, current->arg1);

    // Move the value to $a0
    fprintf(outputFile, "\tmove $a0, %s\n", tempIntRegisters[regIndex].name);

    // Print integer syscall
    fprintf(outputFile, "\tli $v0, 1\n");
    fprintf(outputFile, "\tsyscall\n");

    // Print newline
    fprintf(outputFile, "\tli $v0, 4\n");
    fprintf(outputFile, "\tla $a0, newline\n");
    fprintf(outputFile, "\tsyscall\n");

    // Deallocate register
    deallocateIntRegister(regIndex);
}

// Float Write
void generateFloatWrite(TAC* current) {
    int regIndex;

    // Allocate register
    regIndex = allocateFloatRegister();

    // Error checking
    if (regIndex == -1) {
        printf("Error: No available float registers\n");
        return;
    }

    // Load the value to print
    fprintf(outputFile, "\tl.s %s, %s #WRITE FLOAT\n",
            tempFloatRegisters[regIndex].name, current->arg1);

    // Move the value to $f12
    fprintf(outputFile, "\tmov.s $f12, %s\n", tempFloatRegisters[regIndex].name);

    // Print float syscall
    fprintf(outputFile, "\tli $v0, 2\n");
    fprintf(outputFile, "\tsyscall\n");

    // Print newline
    fprintf(outputFile, "\tli $v0, 4\n");
    fprintf(outputFile, "\tla $a0, newline\n");
    fprintf(outputFile, "\tsyscall\n");

    // Deallocate register
    deallocateFloatRegister(regIndex);
}

// Integer Store
void generateIntStore(TAC* current) {
    int regIndex;

    // Allocate register
    regIndex = allocateIntRegister();

    // Error checking
    if (regIndex == -1) {
        printf("Error: No available integer registers\n");
        return;
    }

    // Load value from source
    fprintf(outputFile, "\tlw %s, %s #STORE INT\n",
            tempIntRegisters[regIndex].name, current->arg1);

    // Store the value into the destination variable
    fprintf(outputFile, "\tsw %s, %s\n",
            tempIntRegisters[regIndex].name, current->result);

    // Deallocate register
    deallocateIntRegister(regIndex);
}

// Integer Load
void generateIntLoad(TAC* current) {
    int regIndex;

    // Allocate register
    regIndex = allocateIntRegister();

    // Error checking
    if (regIndex == -1) {
        printf("Error: No available integer registers\n");
        return;
    }

    // Load value from variable
    fprintf(outputFile, "\tlw %s, %s #LOAD INT\n",
            tempIntRegisters[regIndex].name, current->arg1);

    // Store the value into the temporary variable
    fprintf(outputFile, "\tsw %s, %s\n",
            tempIntRegisters[regIndex].name, current->result);

    // Deallocate register
    deallocateIntRegister(regIndex);
}

// Float Store
void generateFloatStore(TAC* current) {
    int regIndex;

    // Allocate register
    regIndex = allocateFloatRegister();

    // Error checking
    if (regIndex == -1) {
        printf("Error: No available float registers\n");
        return;
    }

    // Load value from source
    fprintf(outputFile, "\tl.s %s, %s #STORE FLOAT\n",
            tempFloatRegisters[regIndex].name, current->arg1);

    // Store the value into the destination variable
    fprintf(outputFile, "\ts.s %s, %s\n",
            tempFloatRegisters[regIndex].name, current->result);

    // Deallocate register
    deallocateFloatRegister(regIndex);
}

// Float Load
void generateFloatLoad(TAC* current) {
    int regIndex;

    // Allocate register
    regIndex = allocateFloatRegister();

    // Error checking
    if (regIndex == -1) {
        printf("Error: No available float registers\n");
        return;
    }

    // Load value from variable
    fprintf(outputFile, "\tl.s %s, %s #LOAD FLOAT\n",
            tempFloatRegisters[regIndex].name, current->arg1);

    // Store the value into the temporary variable
    fprintf(outputFile, "\ts.s %s, %s\n",
            tempFloatRegisters[regIndex].name, current->result);

    // Deallocate register
    deallocateFloatRegister(regIndex);
}

// Char Assign
void generateCharAssign(TAC* current) {
    int regIndex;
    int addrRegIndex;

    // Allocate register
    regIndex = allocateIntRegister();

    // Error checking
    if (regIndex == -1) {
        printf("Error: No available int registers\n");
        return;
    }

    addrRegIndex = allocateIntRegister();

    // Error checking
    if (addrRegIndex == -1) {
        printf("Error: No available int registers\n");
        return;
    }


    int asciiValue = current->arg1[0]; //Get numerical ascii value of char
    // Load immediate value
    fprintf(outputFile, "\tli %s, %d #ASSIGN CHAR VALUE\n", tempIntRegisters[regIndex].name, asciiValue);

    // Load char address
    fprintf(outputFile, "\tla %s, %s\n", tempIntRegisters[addrRegIndex].name, current->result);

    // Store the value into the result variable
    fprintf(outputFile, "\tsb %s, 0(%s)\n", tempIntRegisters[regIndex].name, tempIntRegisters[addrRegIndex].name);


    // Deallocate register
    deallocateIntRegister(regIndex);
    deallocateIntRegister(addrRegIndex);
}


// Char Load
void generateCharLoad(TAC* current) {
    int regIndex;
    int addrRegIndex;

    // Allocate register
    regIndex = allocateIntRegister();

    // Error checking
    if (regIndex == -1) {
        printf("Error: No available integer registers\n");
        return;
    }

    addrRegIndex = allocateIntRegister();

    // Error checking
    if (addrRegIndex == -1) {
        printf("Error: No available int registers\n");
        return;
    }

    // Load value from variable
    fprintf(outputFile, "\tlb %s, %s #LOAD CHAR\n",
            tempIntRegisters[regIndex].name, current->arg1);

    // Load char address
    fprintf(outputFile, "\tla %s, %s\n", tempIntRegisters[addrRegIndex].name, current->result);

    // Store the value into the temporary variable
    fprintf(outputFile, "\tsb %s, 0(%s)\n",
            tempIntRegisters[regIndex].name, tempIntRegisters[addrRegIndex].name);

    // Deallocate register
    deallocateIntRegister(regIndex);
    deallocateIntRegister(addrRegIndex);
}

// Char Store
void generateCharStore(TAC* current) {
    int regIndex;
    int addrRegIndex;

    // Allocate register
    regIndex = allocateIntRegister();

    // Error checking
    if (regIndex == -1) {
        printf("Error: No available integer registers\n");
        return;
    }

    addrRegIndex = allocateIntRegister();

    // Error checking
    if (addrRegIndex == -1) {
        printf("Error: No available int registers\n");
        return;
    }

    // Load value from source
    fprintf(outputFile, "\tlb %s, %s #STORE CHAR\n",
            tempIntRegisters[regIndex].name, current->arg1);

    // Load char address
    fprintf(outputFile, "\tla %s, %s\n", tempIntRegisters[addrRegIndex].name, current->result);

    // Store the value into the destination variable
    fprintf(outputFile, "\tsb %s, 0(%s)\n",
            tempIntRegisters[regIndex].name, tempIntRegisters[addrRegIndex].name);

    // Deallocate register
    deallocateIntRegister(regIndex);
    deallocateIntRegister(addrRegIndex);
}

// Char Write
void generateCharWrite(TAC* current) {
    int regIndex;
    int addrRegIndex;

    // Allocate register
    regIndex = allocateIntRegister();

    // Error checking
    if (regIndex == -1) {
        printf("Error: No available integer registers\n");
        return;
    }

    addrRegIndex = allocateIntRegister();

    // Error checking
    if (addrRegIndex == -1) {
        printf("Error: No available int registers\n");
        return;
    }


    // Load char address
    fprintf(outputFile, "\tla %s, %s #WRITE CHAR\n", tempIntRegisters[addrRegIndex].name, current->arg1);

    // Load the value to print
    fprintf(outputFile, "\tlb %s, 0(%s)\n",
            tempIntRegisters[regIndex].name, tempIntRegisters[addrRegIndex].name);

    // Move the value to $a0
    fprintf(outputFile, "\tmove $a0, %s\n", tempIntRegisters[regIndex].name);

    // Print integer syscall
    fprintf(outputFile, "\tli $v0, 11\n");
    fprintf(outputFile, "\tsyscall\n");

    // Print newline
    fprintf(outputFile, "\tli $v0, 4\n");
    fprintf(outputFile, "\tla $a0, newline\n");
    fprintf(outputFile, "\tsyscall\n");

    // Deallocate register
    deallocateIntRegister(regIndex);
    deallocateIntRegister(addrRegIndex);
}

// Array Integer Store
void generateArrIntStore(TAC* current) {
    int baseRegIndex, indexRegIndex, valueRegIndex, addressRegIndex;

    // Allocate registers
    baseRegIndex = allocateIntRegister();
    indexRegIndex = allocateIntRegister();
    valueRegIndex = allocateIntRegister();
    addressRegIndex = allocateIntRegister();

    // Error checking
    if (baseRegIndex == -1 || indexRegIndex == -1 ||
        valueRegIndex == -1 || addressRegIndex == -1) {
        printf("Error: No available integer registers\n");
        return;
    }

    // Load base address of array
    fprintf(outputFile, "\tla %s, %s #STORE INTO INT ARRAY\n",
            tempIntRegisters[baseRegIndex].name, current->result);

    // Load index
    fprintf(outputFile, "\tlw %s, %s\n",
            tempIntRegisters[indexRegIndex].name, current->arg2);

    // Calculate offset (index * 4)
    fprintf(outputFile, "\tsll %s, %s, 2\n",
            tempIntRegisters[indexRegIndex].name, tempIntRegisters[indexRegIndex].name);

    // Calculate address (base + offset)
    fprintf(outputFile, "\tadd %s, %s, %s\n",
            tempIntRegisters[addressRegIndex].name,
            tempIntRegisters[baseRegIndex].name,
            tempIntRegisters[indexRegIndex].name);

    // Load value to store
    fprintf(outputFile, "\tlw %s, %s\n",
            tempIntRegisters[valueRegIndex].name, current->arg1);

    // Store value into array
    fprintf(outputFile, "\tsw %s, 0(%s)\n",
            tempIntRegisters[valueRegIndex].name, tempIntRegisters[addressRegIndex].name);

    // Deallocate registers
    deallocateIntRegister(baseRegIndex);
    deallocateIntRegister(indexRegIndex);
    deallocateIntRegister(valueRegIndex);
    deallocateIntRegister(addressRegIndex);
}

// Array Integer Load
void generateArrIntLoad(TAC* current) {
    int baseRegIndex, indexRegIndex, valueRegIndex, addressRegIndex;

    // Allocate registers
    baseRegIndex = allocateIntRegister();
    indexRegIndex = allocateIntRegister();
    valueRegIndex = allocateIntRegister();
    addressRegIndex = allocateIntRegister();

    // Error checking
    if (baseRegIndex == -1 || indexRegIndex == -1 ||
        valueRegIndex == -1 || addressRegIndex == -1) {
        printf("Error: No available integer registers\n");
        return;
    }

    // Load base address of array
    fprintf(outputFile, "\tla %s, %s #LOAD FROM INT ARRAY\n",
            tempIntRegisters[baseRegIndex].name, current->arg1);

    // Load index
    fprintf(outputFile, "\tlw %s, %s\n",
            tempIntRegisters[indexRegIndex].name, current->arg2);

    // Calculate offset (index * 4)
    fprintf(outputFile, "\tsll %s, %s, 2\n",
            tempIntRegisters[indexRegIndex].name, tempIntRegisters[indexRegIndex].name);

    // Calculate address (base + offset)
    fprintf(outputFile, "\tadd %s, %s, %s\n",
            tempIntRegisters[addressRegIndex].name,
            tempIntRegisters[baseRegIndex].name,
            tempIntRegisters[indexRegIndex].name);

    // Load value from array
    fprintf(outputFile, "\tlw %s, 0(%s)\n",
            tempIntRegisters[valueRegIndex].name, tempIntRegisters[addressRegIndex].name);

    // Store value into result variable
    fprintf(outputFile, "\tsw %s, %s\n",
            tempIntRegisters[valueRegIndex].name, current->result);

    // Deallocate registers
    deallocateIntRegister(baseRegIndex);
    deallocateIntRegister(indexRegIndex);
    deallocateIntRegister(valueRegIndex);
    deallocateIntRegister(addressRegIndex);
}

// Array Float Store
void generateArrFloatStore(TAC* current) {
    int baseRegIndex, indexRegIndex, addressRegIndex;
    int valueRegIndex;

    // Allocate registers
    baseRegIndex = allocateIntRegister();
    indexRegIndex = allocateIntRegister();
    addressRegIndex = allocateIntRegister();
    valueRegIndex = allocateFloatRegister();

    // Error checking
    if (baseRegIndex == -1 || indexRegIndex == -1 ||
        addressRegIndex == -1 || valueRegIndex == -1) {
        printf("Error: No available registers\n");
        return;
    }

    // Load base address of array
    fprintf(outputFile, "\tla %s, %s #STORE INTO FLOAT ARRAY\n",
            tempIntRegisters[baseRegIndex].name, current->result);

    // Load index
    fprintf(outputFile, "\tlw %s, %s\n",
            tempIntRegisters[indexRegIndex].name, current->arg2);

    // Calculate offset (index * 4)
    fprintf(outputFile, "\tsll %s, %s, 2\n",
            tempIntRegisters[indexRegIndex].name, tempIntRegisters[indexRegIndex].name);

    // Calculate address (base + offset)
    fprintf(outputFile, "\tadd %s, %s, %s\n",
            tempIntRegisters[addressRegIndex].name,
            tempIntRegisters[baseRegIndex].name,
            tempIntRegisters[indexRegIndex].name);

    // Load value to store
    fprintf(outputFile, "\tl.s %s, %s\n",
            tempFloatRegisters[valueRegIndex].name, current->arg1);

    // Store value into array
    fprintf(outputFile, "\ts.s %s, 0(%s)\n",
            tempFloatRegisters[valueRegIndex].name, tempIntRegisters[addressRegIndex].name);

    // Deallocate registers
    deallocateIntRegister(baseRegIndex);
    deallocateIntRegister(indexRegIndex);
    deallocateIntRegister(addressRegIndex);
    deallocateFloatRegister(valueRegIndex);
}

// Array Float Load
void generateArrFloatLoad(TAC* current) {
    int baseRegIndex, indexRegIndex, addressRegIndex;
    int valueRegIndex;

    // Allocate registers
    baseRegIndex = allocateIntRegister();
    indexRegIndex = allocateIntRegister();
    addressRegIndex = allocateIntRegister();
    valueRegIndex = allocateFloatRegister();

    // Error checking
    if (baseRegIndex == -1 || indexRegIndex == -1 ||
        addressRegIndex == -1 || valueRegIndex == -1) {
        printf("Error: No available registers\n");
        return;
    }

    // Load base address of array
    fprintf(outputFile, "\tla %s, %s #LOAD FROM FLOAT ARRAY\n",
            tempIntRegisters[baseRegIndex].name, current->arg1);

    // Load index
    fprintf(outputFile, "\tlw %s, %s\n",
            tempIntRegisters[indexRegIndex].name, current->arg2);

    // Calculate offset (index * 4)
    fprintf(outputFile, "\tsll %s, %s, 2\n",
            tempIntRegisters[indexRegIndex].name, tempIntRegisters[indexRegIndex].name);

    // Calculate address (base + offset)
    fprintf(outputFile, "\tadd %s, %s, %s\n",
            tempIntRegisters[addressRegIndex].name,
            tempIntRegisters[baseRegIndex].name,
            tempIntRegisters[indexRegIndex].name);

    // Load value from array
    fprintf(outputFile, "\tl.s %s, 0(%s)\n",
            tempFloatRegisters[valueRegIndex].name, tempIntRegisters[addressRegIndex].name);

    // Store value into result variable
    fprintf(outputFile, "\ts.s %s, %s\n",
            tempFloatRegisters[valueRegIndex].name, current->result);

    // Deallocate registers
    deallocateIntRegister(baseRegIndex);
    deallocateIntRegister(indexRegIndex);
    deallocateIntRegister(addressRegIndex);
    deallocateFloatRegister(valueRegIndex);
}

// Array Char Store
void generateArrCharStore(TAC* current) {
    int baseRegIndex;
    int indexRegIndex;
    int addressRegIndex;
    int valueRegIndex;
    int sourceRegIndex;

    // Allocate registers
    baseRegIndex = allocateIntRegister();
    indexRegIndex = allocateIntRegister();
    addressRegIndex = allocateIntRegister();
    valueRegIndex = allocateIntRegister();
    sourceRegIndex = allocateIntRegister();

    // Error checking
    if (baseRegIndex == -1 || indexRegIndex == -1 ||
        addressRegIndex == -1 || valueRegIndex == -1 ||
        sourceRegIndex == -1) {
        printf("Error: No available registers\n");
        return;
    }

    // Load base address of destination array
    fprintf(outputFile, "\tla %s, %s #STORE INTO CHAR ARRAY\n",
            tempIntRegisters[baseRegIndex].name, current->result);

    // Load destination index
    fprintf(outputFile, "\tlw %s, %s\n",
            tempIntRegisters[indexRegIndex].name, current->arg2);

    // Calculate address (base + offset)
    fprintf(outputFile, "\tadd %s, %s, %s\n",
            tempIntRegisters[addressRegIndex].name,
            tempIntRegisters[baseRegIndex].name,
            tempIntRegisters[indexRegIndex].name);

    // Load address of source char
    fprintf(outputFile, "\tla %s, %s\n",
            tempIntRegisters[sourceRegIndex].name, current->arg1);

    // Load value to store
    fprintf(outputFile, "\tlb %s, 0(%s)\n",
            tempIntRegisters[valueRegIndex].name, tempIntRegisters[sourceRegIndex].name);

    // Store value into array
    fprintf(outputFile, "\tsb %s, 0(%s)\n",
            tempIntRegisters[valueRegIndex].name, tempIntRegisters[addressRegIndex].name);

    // Deallocate registers
    deallocateIntRegister(baseRegIndex);
    deallocateIntRegister(indexRegIndex);
    deallocateIntRegister(addressRegIndex);
    deallocateIntRegister(valueRegIndex);
    deallocateIntRegister(sourceRegIndex);
}

// Array Integer Load
void generateArrCharLoad(TAC* current) {
    int baseRegIndex;
    int indexRegIndex;
    int valueRegIndex;
    int addressRegIndex;
    int destinationRegIndex;

    // Allocate registers
    baseRegIndex = allocateIntRegister();
    indexRegIndex = allocateIntRegister();
    valueRegIndex = allocateIntRegister();
    addressRegIndex = allocateIntRegister();
    destinationRegIndex = allocateIntRegister();

    // Error checking
    if (baseRegIndex == -1 || indexRegIndex == -1 ||
        valueRegIndex == -1 || addressRegIndex == -1 ||
        destinationRegIndex == -1) {
        printf("Error: No available integer registers\n");
        return;
    }

    // Load base address of array
    fprintf(outputFile, "\tla %s, %s #LOAD FROM CHAR ARRAY\n",
            tempIntRegisters[baseRegIndex].name, current->arg1);

    // Load index
    fprintf(outputFile, "\tlw %s, %s\n",
            tempIntRegisters[indexRegIndex].name, current->arg2);

    // Calculate offset (index * 4)
    // fprintf(outputFile, "\tsll %s, %s, 2\n",
    //         tempIntRegisters[indexRegIndex].name, tempIntRegisters[indexRegIndex].name);

    // Calculate address (base + offset)
    fprintf(outputFile, "\tadd %s, %s, %s\n",
            tempIntRegisters[addressRegIndex].name,
            tempIntRegisters[baseRegIndex].name,
            tempIntRegisters[indexRegIndex].name);

    // Load value from array
    fprintf(outputFile, "\tlb %s, 0(%s)\n",
            tempIntRegisters[valueRegIndex].name, tempIntRegisters[addressRegIndex].name);

    // Load address of source char
    fprintf(outputFile, "\tla %s, %s\n",
            tempIntRegisters[destinationRegIndex].name, current->result);

    // Store value to address
    fprintf(outputFile, "\tsb %s, 0(%s)\n",
            tempIntRegisters[valueRegIndex].name, tempIntRegisters[destinationRegIndex].name);

    // Deallocate registers
    deallocateIntRegister(baseRegIndex);
    deallocateIntRegister(indexRegIndex);
    deallocateIntRegister(valueRegIndex);
    deallocateIntRegister(addressRegIndex);
    deallocateIntRegister(destinationRegIndex);
}

// Float Assignment
void generateFloatAssign(TAC* current) {
    int regIndex;

    regIndex = allocateFloatRegister();
    if (regIndex == -1) {
        printf("Error: No available float registers\n");
        return;
    }

    // Load immediate value into a float register
    // Since MIPS doesn't have a 'li.s' instruction, we need to load the value from memory
    // Create a constant in the data segment
    DataElement* floatConst = createConst("float", current->arg1);

    // Load the float constant into the register
    fprintf(outputFile, "\tl.s %s, %s #ASSIGN FLOAT VALUE\n", tempFloatRegisters[regIndex].name, floatConst->varName);

    // Store the value into the result variable
    fprintf(outputFile, "\ts.s %s, %s\n", tempFloatRegisters[regIndex].name, current->result);

    deallocateFloatRegister(regIndex);
}

void generateIntToFloat(TAC* current) {
    int intRegIndex;
    int floatRegIndex;

    intRegIndex = allocateIntRegister();
    if (intRegIndex == -1) {
        printf("Error: No available int registers\n");
        return;
    }

    floatRegIndex = allocateFloatRegister();
    if (floatRegIndex == -1) {
        printf("Error: No available float registers\n");
        return;
    }

    // Load int value into register
    fprintf(outputFile, "\tlw %s, %s #CONVERT INT TO FLOAT\n", tempIntRegisters[intRegIndex].name, current->arg1);

    // Transfer int register's binary value into float register
    fprintf(outputFile, "\tmtc1 %s, %s\n", tempIntRegisters[intRegIndex].name, tempFloatRegisters[floatRegIndex].name);

    // Format integer to float
    fprintf(outputFile, "\tcvt.s.w %s, %s\n", tempFloatRegisters[floatRegIndex].name, tempFloatRegisters[floatRegIndex].name);

    // Store float register value into .float address
    fprintf(outputFile, "\ts.s %s, %s\n", tempFloatRegisters[floatRegIndex].name, current->result);

    deallocateIntRegister(intRegIndex);
    deallocateFloatRegister(floatRegIndex);
}

void generateFloatToInt(TAC* current) {
    int intRegIndex;
    int floatRegIndex;

    intRegIndex = allocateIntRegister();
    if (intRegIndex == -1) {
        printf("Error: No available int registers\n");
        return;
    }

    floatRegIndex = allocateFloatRegister();
    if (floatRegIndex == -1) {
        printf("Error: No available float registers\n");
        return;
    }

    // Load int value into register
    fprintf(outputFile, "\tl.s %s, %s #CONVERT FLOAT TO INT\n", tempFloatRegisters[floatRegIndex].name, current->arg1);

    // Format integer to float
    fprintf(outputFile, "\tcvt.w.s %s, %s\n", tempFloatRegisters[floatRegIndex].name, tempFloatRegisters[floatRegIndex].name);

    // Transfer int register's binary value into float register
    fprintf(outputFile, "\tmfc1 %s, %s\n", tempIntRegisters[intRegIndex].name, tempFloatRegisters[floatRegIndex].name);

    // Store float register value into .float address
    fprintf(outputFile, "\tsw %s, %s\n", tempIntRegisters[intRegIndex].name, current->result);

    deallocateIntRegister(intRegIndex);
    deallocateFloatRegister(floatRegIndex);
}
// Implement any additional necessary functions as needed for your compiler.
