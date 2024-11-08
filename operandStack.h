#ifndef OPERAND_STACK_H
#define OPERAND_STACK_H

#include "commons/types.h"

#define OPERAND_STACK_SIZE 100

typedef struct Operand{
    char* operandID;
    VarType operandType;
}Operand;

// extern Operand* operandStack[OPERAND_STACK_SIZE];
// extern int operandCursor = 0;

extern Operand* operandStack[];
extern int operandCursor;

// void initOperandStack();

Operand* createOperandStruct(char* opString, VarType type);
void freeOperand(Operand* op);

void pushOperand(Operand* op);
Operand* popOperand();
void printOperandStack();

#endif