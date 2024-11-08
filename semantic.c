#include "semantic.h"
#include "symbolTable.h"
#include "codeGenerator.h"
#include "operandStack.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global head and tail of the TAC list
TAC* tacHead = NULL;
TAC* tacTail = NULL;

// Function TAC heads and tails
FuncTAC* funcTacHeads = NULL;
FuncTAC* funcTacTails = NULL;

// Current TAC being operated on
TAC** currentTacHead = NULL;
TAC** currentTacTail = NULL;
FuncTAC* currentFuncTAC = NULL;
bool inFunction = false;

// Reference to the symbol table
SymbolTable* symTabRef = NULL;

// Temporary variable counters
int tempIntCount = 0;
int tempFloatCount = 0;
int tempCharCount = 0;

// Initialize Semantic Analyzer with the symbol table reference
void initSemantic(SymbolTable* symbolTable) {
    symTabRef = symbolTable;
    currentTacHead = &tacHead;
    currentTacTail = &tacTail;
}

// General function to append a TAC to the TAC list
void appendTAC(TAC** head, TAC** tail, TAC* newInstruction) {
    if (!*head) {
        // printf("appendTAC(): TAC is empty, creating new head\n");
        *head = newInstruction;
        *tail = newInstruction;
        newInstruction->prev = NULL;
        newInstruction->next = NULL;
    } else {
        // printf("appendTAC(): TAC exists, appending new TAC\n");
        (*tail)->next = newInstruction;
        newInstruction->prev = *tail;
        newInstruction->next = NULL;
        (*tail) = newInstruction;
    }
}

// General function to create TAC instructions
TAC* createTAC(char* result, char* arg1, char* op, char* arg2) {
    TAC* newTAC = (TAC*)malloc(sizeof(TAC));
    newTAC->result = result ? strdup(result) : NULL;
    newTAC->arg1 = arg1 ? strdup(arg1) : NULL;
    newTAC->op = op ? strdup(op) : NULL;
    newTAC->arg2 = arg2 ? strdup(arg2) : NULL;
    newTAC->next = newTAC->prev = NULL;
    return newTAC;
}

// Helper to generate a temporary variable based on type
char* createTempVar(VarType type) {
    char* tempVar = malloc(30);  // Allocate space for temp variable name
    if (type == VarType_Int) {
        snprintf(tempVar, 30, "i%d", tempIntCount++);
    } else if (type == VarType_Float) {
        snprintf(tempVar, 30, "f%d", tempFloatCount++);
    } else if (type == VarType_Char) {
        snprintf(tempVar, 30, "c%d", tempCharCount++);
    } else {
        fprintf(stderr, "Unsupported type for temporary variable (type %d, %s)\n", type, varTypeToString(type));
        exit(1);
    }
    return tempVar;
}

// General function to perform semantic analysis on an AST node
void semanticAnalysis(ASTNode* node) {
    if (!node) return;

    switch (node->type) {
        case NodeType_Program:
            printf("Performing semantic analysis on program\n");
            semanticAnalysis(node->data.program.varDeclList);
            semanticAnalysis(node->data.program.stmtList);
            break;

        case NodeType_DeclList:
            semanticAnalysis(node->data.declList.decl);
            semanticAnalysis(node->data.declList.next);
            break;

        case NodeType_VarDecl:
            // Variable declarations are handled during parsing
            break;

        case NodeType_ArrDecl:
            // Array declarations are handled during parsing
            break;

        case NodeType_FuncDecl:
            handleFunctionDeclaration(node);
            break;

        case NodeType_ParamList:
            handleParameterList(node);
            break;

        case NodeType_Param:
            // handleParameter(node);
            break;

        case NodeType_FuncCall:
            handleFunctionCall(node);
            break;
        
        case NodeType_ArgList:
            handleArgList(node);
            break;
        
        case NodeType_Arg:
            handleArg(node);
            break;

        case NodeType_VarDeclList:
            handleFuncVarDeclList(node);
            break;

        case NodeType_StmtList:
            semanticAnalysis(node->data.stmtList.stmt);
            semanticAnalysis(node->data.stmtList.stmtList);
            break;

        case NodeType_AssignStmt:
            semanticAnalysis(node->data.assignStmt.expr);
            generateTACForAssign(node);
            break;

        case NodeType_AssignArrStmt:
            semanticAnalysis(node->data.assignArrStmt.indexExpr);
            semanticAnalysis(node->data.assignArrStmt.expr);
            generateTACForAssignArr(node);
            break;
        
        case NodeType_ReturnStmt:
            handleReturnStmt(node);
            break;

        case NodeType_BinOp:
            semanticAnalysis(node->data.binOp.left);
            semanticAnalysis(node->data.binOp.right);
            generateTACForExpr(node);
            break;

        case NodeType_WriteStmt:
            generateTACForWrite(node);
            break;

        case NodeType_IntExpr:
        case NodeType_FloatExpr:
        case NodeType_CharExpr:
        case NodeType_SimpleID:
        case NodeType_ArrAccess:
            generateTACForExpr(node);
            break;

        default:
            fprintf(stderr, "Unhandled Node Type in semantic analysis: %d\n", node->type);
            exit(1);
            break;
    }
}

// Handle function declarations
void handleFunctionDeclaration(ASTNode* node) {
    // Check for redeclaration
    Symbol* funcSymbol = lookupSymbol(symTabRef, node->data.funcDecl.name);

    ASTNode* currentParamList = node->data.funcDecl.paramList;

    enterScope(symTabRef);
    initFuncTAC(node->data.funcDecl.name, node->data.funcDecl.returnType);

    while (currentParamList) {
        ASTNode* currentParam = currentParamList->data.paramList.param;
        addParameter(funcSymbol, currentParam->data.param.name, currentParam->data.param.type);
        currentParamList = currentParamList->data.paramList.next;
    }

    semanticAnalysis(node->data.funcDecl.paramList);
    semanticAnalysis(node->data.funcDecl.varDeclList);
    semanticAnalysis(node->data.funcDecl.stmtList);

    finalizeFuncTAC();
    exitScope(symTabRef);
}

// Handle parameter lists
void handleParameterList(ASTNode* node) {
    if (!node) return;

    semanticAnalysis(node->data.paramList.param);
    handleParameterList(node->data.paramList.next);
}

// Handle function calls
void handleFunctionCall(ASTNode* node) {
    if (!lookupSymbol(symTabRef, node->data.funcCall.name)) {
        fprintf(stderr, "Function '%s' not declared\n", node->data.funcCall.name);
        exit(1);
    }

    // ASTNode* argList = node->data.funcCall.argList;
    // while (argList) {
    //     semanticAnalysis(argList->data.argList.arg);
    //     argList = argList->data.argList.next;
    // }
    handleArgList(node->data.funcCall.argList);

    generateTACForFuncCall(node);
}

// Handle Return Statement
void handleReturnStmt(ASTNode* node)
{
    //Mark function as containing a return statement
    currentFuncTAC->returnsValue = true;

    //Check if we are in a function. If not, halt compilation
    //   ?: Should return outside of a function abort the program, similar to
    //      calling return in main in C?
    if (!inFunction)
    {
        printf("SEMANTIC: Return called outside of a function, halting...\n");
        exit(1);
    }

    //Check for return expression
    //      TODO: Throw an error if a return expression is specified in
    //          a void-returning function, or if no return expression is
    //          specified in a function with a return type.
    if (currentFuncTAC->returnType == VarType_Void)
    {
        //If void-returning function attempts to return a value, halt compiler
        if (node->data.returnStmt.returnExpr)
        {
            printf("SEMANTIC: Attempted to return a value from a void-returning function\n");
            exit(1);
        }
    }
    else
    {
        VarType returnExprType = getExprType(node->data.returnStmt.returnExpr);
        if (returnExprType != currentFuncTAC->returnType)
        {
            printf("SEMANTIC: Attempted to return %s from a %s-returning function\n", varTypeToString(returnExprType), varTypeToString(currentFuncTAC->returnType));
            exit(1);
        }
    }

    if (node->data.returnStmt.returnExpr)
    {
        semanticAnalysis(node->data.returnStmt.returnExpr);
        Operand* returnOperand = popOperand();
        char* result;
        char* operator;
        
        switch (returnOperand->operandType)
        {
            case (VarType_Int):
                result = "returnInt";
                operator = "store.int";
                break;
            case (VarType_Float):
                result = "returnFloat";
                operator = "store.float";
                break;
            case (VarType_Char):
                result = "returnChar";
                operator = "store.char";
                break;
            default:
                printf("Invalid return type: %s\n", varTypeToString(returnOperand->operandType));
                break;
        }
        TAC* returnValueInstr = createTAC(  //Output return value
            result,
            returnOperand->operandID,
            operator,
            NULL            
        );
        appendTAC(currentTacHead, currentTacTail, returnValueInstr);
    }
    else
    {
        //empty for now
    }

    TAC* returnStmtInstr = createTAC(  //Exit function
        NULL,
        NULL,
        "return",
        NULL
    );
    appendTAC(currentTacHead, currentTacTail, returnStmtInstr);
}

// Handle a function call's argument list
void handleArgList(ASTNode* node) {
    if (!node) return;

    semanticAnalysis(node->data.argList.arg);
    handleParameterList(node->data.argList.next);
}

// Handle a function call's argument list
void handleArg(ASTNode* node) {
    //TODO: Handle Arguments here


    //The final value of an expression representing an argument should
    //be assigned to the parameter variable of the function corresponding
    //to the argument in question.
    semanticAnalysis(node->data.arg.expr);
}

// Handle parameter lists
void handleFuncVarDeclList(ASTNode* node) {
    if (!node) return;

    semanticAnalysis(node->data.varDeclList.varDecl);
    handleFuncVarDeclList(node->data.varDeclList.next);
}

// Handle parameters
void handleFuncVarDecl(ASTNode* node) {
    // Add parameter to symbol table
    char* varName = node->data.varDecl.varName;

    //TODO: varDecl.varType is a string, not a VarType enum
    //      I want to fix this but I don't want to break things
    //      things right now, will fix this later
    // VarType varType = node->data.varDecl.varType;
    char* varType = node->data.varDecl.varType;

    // if (lookupSymbol(symTabRef, varName)) {
    //     fprintf(stderr, "Variable '%s' already declared in current scope\n", varName);
    //     exit(1);
    // }

    // addSymbol(symTabRef, varName, varType);
}

// Generate TAC for binary operations
TAC* generateTACForExpr(ASTNode* expr) {
    if (!expr) return NULL;

    TAC* instruction = NULL;
    TAC* convInstruction = NULL; //Only used if mixed-type operands are detected

    switch (expr->type) {
        case NodeType_BinOp: {
            Operand* arg2Operand = popOperand();
            Operand* arg1Operand = popOperand();

            char buffer[20];
            VarType resultType;
            if (arg1Operand->operandType == VarType_Int && arg2Operand->operandType == VarType_Int) {
                snprintf(buffer, 20, "%s.int", expr->data.binOp.operator);
                resultType = VarType_Int;
            } else if (arg1Operand->operandType == VarType_Float || arg2Operand->operandType == VarType_Float) {
                snprintf(buffer, 20, "%s.float", expr->data.binOp.operator);
                resultType = VarType_Float;

                if (arg1Operand->operandType == VarType_Int)
                {
                    convInstruction = createTAC(createTempVar(VarType_Float), arg1Operand->operandID, "intToFloat", NULL); //Type Conversion
                    appendTAC(currentTacHead, currentTacTail, convInstruction);
                    freeOperand(arg1Operand);
                    arg1Operand = createOperandStruct(convInstruction->result, VarType_Float);
                }
                if (arg2Operand->operandType == VarType_Int)
                {
                    convInstruction = createTAC(createTempVar(VarType_Float), arg2Operand->operandID, "intToFloat", NULL); //Type Conversion
                    appendTAC(currentTacHead, currentTacTail, convInstruction);
                    freeOperand(arg2Operand);
                    arg2Operand = createOperandStruct(convInstruction->result, VarType_Float);
                }
            } else {
                fprintf(stderr, "Unsupported operand types for binary operation\n");
                exit(1);
            }

            instruction = createTAC(
                createTempVar(resultType),
                arg1Operand->operandID,
                buffer,
                arg2Operand->operandID
            );

            pushOperand(createOperandStruct(instruction->result, resultType));
            appendTAC(currentTacHead, currentTacTail, instruction);

            freeOperand(arg1Operand);
            freeOperand(arg2Operand);
            break;
        }

        case NodeType_IntExpr: {
            char buffer[20];
            snprintf(buffer, 20, "%d", expr->data.intExpr.number);

            instruction = createTAC(
                createTempVar(VarType_Int),
                buffer,
                "assign.int",
                NULL
            );

            pushOperand(createOperandStruct(instruction->result, VarType_Int));
            // printOperandStack();
            // appendTAC(&tacHead, &tacTail, instruction);
            appendTAC(currentTacHead, currentTacTail, instruction);
            break;
        }

        case NodeType_FloatExpr: {
            char buffer[20];
            snprintf(buffer, 20, "%f", expr->data.floatExpr.number);

            instruction = createTAC(
                createTempVar(VarType_Float),
                buffer,
                "assign.float",
                NULL
            );

            pushOperand(createOperandStruct(instruction->result, VarType_Float));
            // printOperandStack();
            // appendTAC(&tacHead, &tacTail, instruction);
            appendTAC(currentTacHead, currentTacTail, instruction);
            break;
        }

        case NodeType_CharExpr: {
            char buffer[20];
            snprintf(buffer, 20, "%c", expr->data.charExpr.character);

            instruction = createTAC(
                createTempVar(VarType_Char),
                buffer,
                "assign.char",
                NULL
            );

            pushOperand(createOperandStruct(instruction->result, VarType_Char));
            appendTAC(currentTacHead, currentTacTail, instruction);
            break;
        }

        case NodeType_SimpleID: {
            // printf("expr->data.simpleID.name: %s\n", expr->data.simpleID.name);
            // printf("currentFuncTAC->funcName: %s\n", currentFuncTAC->funcName);
            // char* symbolID = getMipsVarName(expr->data.simpleID.name, currentFuncTAC->funcName);

            // Symbol* argSymbol = lookupSymbol(symTabRef, symbolID);
            Symbol* argSymbol = lookupSymbol(symTabRef, expr->data.simpleID.name);
            if (!argSymbol) {
                fprintf(stderr, "Variable '%s' not declared (generateTACForExpr())\n", expr->data.simpleID.name);
                printSymbolTable(symTabRef);
                exit(1);
            }

            char* op;
            switch (argSymbol->type)
            {
                case (VarType_Int):
                    op = "load.int";
                    break;
                case (VarType_Float):
                    op = "load.float";
                    break;
                case (VarType_Char):
                    op = "load.char";
                    break;
                default:
                    printf("Invalid return type: %s\n", varTypeToString(argSymbol->type));
                    break;
            }
            VarType resultType = argSymbol->type;

            instruction = createTAC(
                createTempVar(resultType),
                expr->data.simpleID.name,
                op,
                NULL
            );

            pushOperand(createOperandStruct(instruction->result, resultType));
            appendTAC(currentTacHead, currentTacTail, instruction);
            break;
        }

        case NodeType_ArrAccess: {
            Symbol* arrSymbol = lookupSymbol(symTabRef, expr->data.arrAccess.name);
            if (!arrSymbol) {
                fprintf(stderr, "Array '%s' not declared\n", expr->data.arrAccess.name);
                exit(1);
            }

            semanticAnalysis(expr->data.arrAccess.indexExpr);
            Operand* indexOperand = popOperand();

            //Check type of index
            if (indexOperand->operandType != VarType_Int) {
                printf("SEMANTIC: Array index must be an integer!\n");
                exit(1);
            }

            char* op;
            switch (arrSymbol->type)
            {
                case (VarType_Int):
                    op = "load.intIndex";
                    break;
                case (VarType_Float):
                    op = "load.floatIndex";
                    break;
                case (VarType_Char):
                    op = "load.charIndex";
                    break;
                default:
                    printf("Invalid return type: %s\n", varTypeToString(arrSymbol->type));
                    break;
            }
            VarType resultType = arrSymbol->type;

            instruction = createTAC(
                createTempVar(resultType),
                expr->data.arrAccess.name,
                op,
                indexOperand->operandID
            );

            pushOperand(createOperandStruct(instruction->result, resultType));
            appendTAC(currentTacHead, currentTacTail, instruction);

            freeOperand(indexOperand);
            break;
        }

        default:
            fprintf(stderr, "Unhandled expression type in generateTACForExpr: %d\n", expr->type);
            exit(1);
            break;
    }

    return instruction;
}

// Generate TACs for function call
void generateTACForFuncCall(ASTNode* funcCall) {
    //Copy all argument variables into function parameter variables
    Symbol* funcSymbol = lookupSymbol(symTabRef, funcCall->data.funcCall.name);
    FuncParam* currentParam = getParamsTail(funcSymbol); //Arguments pop in reverse order-- get end of params list
    printf("paramsTail: %s %s\n", varTypeToString(currentParam->type), currentParam->name);

    while (currentParam) {
        //Arguments are special expr nodes. Each argument should still be on the stack.
        // printOperandStack();
        Operand* argOperand = popOperand();
        printf("argOperand: %s %s\n", varTypeToString(argOperand->operandType), argOperand->operandID);
        printf("currentParam: %s %s\n", varTypeToString(currentParam->type), currentParam->name);
        if (argOperand->operandType != currentParam->type)
        {
            printf("SEMANTIC: Parameter-Argument type mismatch, halting...\n");
            exit(1);
        }

        char* operandString;
        switch (argOperand->operandType)
        {
            case (VarType_Int):
                operandString = "load.int";
                break;
            case (VarType_Float):
                operandString = "load.float";
                break;
            default:
                printf("SEMANTIC: Invalid argument type, halting...\n");
                exit(1);
                break;
        }

        TAC* argInstr = createTAC(
            currentParam->name,     //result
            argOperand->operandID,  //operand 1
            operandString,          //operator
            NULL                    //operand 2
        );
        // appendTAC(&tacHead, &tacTail, argInstr);
        appendTAC(currentTacHead, currentTacTail, argInstr);
        freeOperand(argOperand);
        currentParam = currentParam->prev; //Traverse backwards
    }
    //Jump to function label
    //  Not implemented yet. IMO, the easiest way to handle this would be saving function
    //  declarations in a separate TAC structure. We could use the address of the head of
    //  a function TAC as an operand in order to call the function call in question, like so:

    //  (Assume &functionTAC = 4539821)
    //  returnVar = 4539821 functionCall (NULL)
    
    //First, get the return variable's name
    char* result;
    switch(funcSymbol->type)
    {
        case (VarType_Int):
            result = "returnInt";
            break;
        case (VarType_Float):
            result = "returnFloat";
            break;
        case (VarType_Void):
            result = NULL;
            break;
        default:
            break;
    }

    //Get the name of the label, used here as an operand
    int labelLength = snprintf(NULL, 0, "%s_func", funcCall->data.funcCall.name); //Get length of const sum so we can allocate appropriate string length
    char* labelName = malloc(labelLength + 1);
    snprintf(labelName, labelLength + 1, "%s_func", funcCall->data.funcCall.name);
    // char* labelName = currentFuncTAC->funcName;

    //Create a return operand and push it to the stack (if return is non-void)
    if (funcSymbol->type != VarType_Void) 
    {
        pushOperand(createOperandStruct(result, funcSymbol->type));
    }
    
    //Create and append TAC
    TAC* functionCallTAC = createTAC(
        result,
        labelName,
        "functionCall",
        NULL
    );
    appendTAC(currentTacHead, currentTacTail, functionCallTAC);
}

// Generate TAC for variable assignment
TAC* generateTACForAssign(ASTNode* assignStmt) {
    printf("assignStmt->data.assignStmt.varName: %s\n", assignStmt->data.assignStmt.varName);
    Operand* rhsOperand = popOperand();
    
    //Set up any conversions necessary if there is a type mismatch
    VarType resultType = lookupSymbol(symTabRef,assignStmt->data.assignStmt.varName)->type;
    if ((rhsOperand->operandType == VarType_Int) && (resultType == VarType_Float))
    {
        TAC* convInstruction = createTAC(createTempVar(VarType_Float), rhsOperand->operandID, "intToFloat", NULL); //Type Conversion
        appendTAC(currentTacHead, currentTacTail, convInstruction);
        freeOperand(rhsOperand);
        rhsOperand = createOperandStruct(convInstruction->result, VarType_Float);
    }
    else if ((rhsOperand->operandType == VarType_Float) && (resultType == VarType_Int))
    {
        TAC* convInstruction = createTAC(createTempVar(VarType_Int), rhsOperand->operandID, "floatToInt", NULL); //Type Conversion
        appendTAC(currentTacHead, currentTacTail, convInstruction);
        freeOperand(rhsOperand);
        rhsOperand = createOperandStruct(convInstruction->result, VarType_Int);
    }

    char* resultString;
    switch (resultType)
    {
        case (VarType_Int):
            resultString = "store.int";
            break;
        case (VarType_Float):
            resultString = "store.float";
            break;
        case (VarType_Char):
            resultString = "store.char";
            break;
        default:
            break;
    }
    
    //Generate TAC
    TAC* instruction = createTAC(
        assignStmt->data.assignStmt.varName,
        rhsOperand->operandID,
        resultString,
        NULL
    );
                
    // appendTAC(&tacHead, &tacTail, instruction);
    appendTAC(currentTacHead, currentTacTail, instruction);
    freeOperand(rhsOperand);

    return instruction;
}

// Generate TAC for array assignment
TAC* generateTACForAssignArr(ASTNode* assignArrStmt) {
    Operand* rhsOperand = popOperand();
    Operand* indexOperand = popOperand();

    //Check type of index
    if (getExprType(assignArrStmt->data.assignArrStmt.indexExpr) != VarType_Int) {
        printf("SEMANTIC: Array index must be an integer!\n");
        exit(1);
    }

    //Set up any conversions necessary if there is a type mismatch
    VarType resultType = lookupSymbol(symTabRef, assignArrStmt->data.assignArrStmt.varName)->type;
    if ((rhsOperand->operandType == VarType_Int) && (resultType == VarType_Float))
    {
        TAC* convInstruction = createTAC(createTempVar(VarType_Float), rhsOperand->operandID, "intToFloat", NULL); //Type Conversion
        appendTAC(currentTacHead, currentTacTail, convInstruction);
        freeOperand(rhsOperand);
        rhsOperand = createOperandStruct(convInstruction->result, VarType_Float);
    }
    else if ((rhsOperand->operandType == VarType_Float) && (resultType == VarType_Int))
    {
        TAC* convInstruction = createTAC(createTempVar(VarType_Int), rhsOperand->operandID, "floatToInt", NULL); //Type Conversion
        appendTAC(currentTacHead, currentTacTail, convInstruction);
        freeOperand(rhsOperand);
        rhsOperand = createOperandStruct(convInstruction->result, VarType_Int);
    }

    char* operatorString;
    switch (rhsOperand->operandType)
    {
        case (VarType_Int):
            operatorString = "store.intIndex";
            break;
        case (VarType_Float):
            operatorString = "store.floatIndex";
            break;
        case (VarType_Char):
            operatorString = "store.charIndex";
            break;
        default:
            break;
    }

    TAC* instruction = createTAC(
        assignArrStmt->data.assignArrStmt.varName,
        rhsOperand->operandID,
        operatorString,
        indexOperand->operandID
    );

    // appendTAC(&tacHead, &tacTail, instruction);
    appendTAC(currentTacHead, currentTacTail, instruction);
    freeOperand(rhsOperand);
    freeOperand(indexOperand);

    return instruction;
}

// Generate TAC for write statements
TAC* generateTACForWrite(ASTNode* expr) {

    semanticAnalysis(expr->data.writeStmt.expr);
    
    //Get the result of the most recent expr evaluation
    Operand* writeOperand = popOperand();
    
    char* loadOp;
    char* writeOp;
    switch (writeOperand->operandType)
    {
        case (VarType_Int):
            loadOp = "load.int";
            writeOp = "write.int";
            break;
        case (VarType_Float):
            loadOp = "load.float";
            writeOp = "write.float";
            break;
        case (VarType_Char):
            loadOp = "load.char";
            writeOp = "write.char";
            break;
        default:
            printf("Invalid expr type in write expr: %s\n", varTypeToString(writeOperand->operandType));
            break;
    }
    VarType resultType = writeOperand->operandType;

    TAC* loadInstr = createTAC(
        createTempVar(resultType),
        writeOperand->operandID,
        loadOp,
        NULL
    );

    TAC* writeInstr = createTAC(
        NULL,
        loadInstr->result,
        writeOp,
        NULL
    );

    appendTAC(currentTacHead, currentTacTail, loadInstr);
    appendTAC(currentTacHead, currentTacTail, writeInstr);

    return writeInstr;
}

// Function to retrieve expression type based on ASTNode
VarType getExprType(ASTNode* expr) {
    switch (expr->type) {
        case NodeType_IntExpr:
            return VarType_Int;

        case NodeType_FloatExpr:
            return VarType_Float;

        case NodeType_CharExpr:
            return VarType_Char;

        case NodeType_SimpleID: {
            Symbol* varSymbol = lookupSymbol(symTabRef, expr->data.simpleID.name);
            if (!varSymbol) {
                fprintf(stderr, "Variable '%s' not declared (getExprType())\n", expr->data.simpleID.name);
                exit(1);
            }
            return varSymbol->type;
        }

        case NodeType_ArrAccess: {
            Symbol* arrSymbol = lookupSymbol(symTabRef, expr->data.arrAccess.name);
            if (!arrSymbol) {
                fprintf(stderr, "Array '%s' not declared\n", expr->data.arrAccess.name);
                exit(1);
            }
            return arrSymbol->type;
        }

        case NodeType_BinOp:
            return determineBinOpType(expr);
        
        case NodeType_FuncCall:
            Symbol* funcReturnType = lookupSymbol(symTabRef, expr->data.funcCall.name);
            if (!funcReturnType) {
                fprintf(stderr, "Function '%s' not declared\n", expr->data.funcCall.name);
                exit(1);
            }
            return funcReturnType->type;

        default:
            fprintf(stderr, "Invalid node type in getExprType: %d\n", expr->type);
            exit(1);
    }
}

// Determine type for binary operations
VarType determineBinOpType(ASTNode* expr) {
    VarType leftType = getExprType(expr->data.binOp.left);
    VarType rightType = getExprType(expr->data.binOp.right);

    if (leftType == VarType_Error || rightType == VarType_Error) {
        return VarType_Error;
    }

    if (leftType == VarType_Char || rightType == VarType_Char) {
        printf("SEMANTIC ERROR: Chars are not compatible with arithmetic operations!\n");
        exit(1);
        //return VarType_Error;
    }

    if (leftType == VarType_Float || rightType == VarType_Float) {
        return VarType_Float;
    }

    return VarType_Int;
}

// Get the count of temporary integer variables
int getTempIntCount() {
    return tempIntCount;
}

// Get the count of temporary float variables
int getTempFloatCount() {
    return tempFloatCount;
}

// Get the count of temporary char variables
int getTempCharCount() {
    return tempCharCount;
}

// Print TAC to the console
void printTAC(TAC* tac) {
    if (!tac) return;
    printf("%s = %s %s %s\n", tac->result ? tac->result : "(null)",
           tac->arg1 ? tac->arg1 : "(null)",
           tac->op ? tac->op : "(null)",
           tac->arg2 ? tac->arg2 : "(null)");
}

// Print TAC to a file
void printTACToFile(const char* filename, TAC* tac) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("Failed to open file");
        return;
    }
    TAC* current = tac;
    while (current) {
        fprintf(file, "%s = %s %s %s\n", current->result ? current->result : "(null)",
                current->arg1 ? current->arg1 : "(null)",
                current->op ? current->op : "(null)",
                current->arg2 ? current->arg2 : "(null)");
        current = current->next;
    }
    fclose(file);
    printf("TAC written to %s\n", filename);
}

// Print Function TACs to a file
void printFuncTACsToFile() {
    int functionNum = 0;
    FuncTAC* currentFunc = funcTacHeads; //Get the start of all func TACs

    while(currentFunc) {
        //Allocate space for filename, and print the name to the allocated buffer
        int filenameLength = snprintf(NULL, 0, "output/FunctionTAC%d.ir", functionNum); //Get length of const sum so we can allocate appropriate string length
	    char* filename = malloc(filenameLength + 1);
        snprintf(filename, filenameLength + 1, "output/FunctionTAC%d.ir", functionNum);
        //Create file with generated name
        FILE* file = fopen(filename, "w");
        if (!file) {
            perror("Failed to open file");
            return;
        }
        //Iterate through each TAC in the current function TAC list
        TAC* currentTAC = currentFunc->func;
        printf("==Function %d TAC==\n", functionNum);
        // printf("currentTAC->next: %d\n", currentTAC->next);
        while (currentTAC) {
            fprintf(file, "%s = %s %s %s\n", currentTAC->result ? currentTAC->result : "(null)",
                    currentTAC->arg1 ? currentTAC->arg1 : "(null)",
                    currentTAC->op ? currentTAC->op : "(null)",
                    currentTAC->arg2 ? currentTAC->arg2 : "(null)");
            printTAC(currentTAC);
            currentTAC = currentTAC->next;
        }
        fclose(file);
        printf("Function TAC written to %s\n", filename);

        //Go to the next function, if it exists
        currentFunc = currentFunc->nextFunc;
        functionNum++;
    }
}

// Destroy a TAC and re-link list
void removeTAC(TAC** del) {
    if (!*del) return;

    if (tacHead == (*del)) {
        tacHead = (*del)->next;
        if (tacHead) tacHead->prev = NULL;
    } else {
        (*del)->prev->next = (*del)->next;
    }
    if (tacTail == (*del)) {
        tacTail = (*del)->prev;
        if (tacTail) tacTail->next = NULL;
    } else {
        (*del)->next->prev = (*del)->prev;
    }
    freeTAC(del);
}

// Free the memory of a TAC
void freeTAC(TAC** del) {
    if (!*del) return;
    if ((*del)->arg1) free((*del)->arg1);
    if ((*del)->arg2) free((*del)->arg2);
    if ((*del)->op) free((*del)->op);
    if ((*del)->result) free((*del)->result);
    free(*del);
    *del = NULL;
}

// Replace a TAC in the list
void replaceTAC(TAC** oldTAC, TAC** newTAC) {
    if (!*oldTAC || !*newTAC) return;

    if (tacHead == (*oldTAC)) {
        tacHead = (*newTAC);
    } else {
        (*oldTAC)->prev->next = (*newTAC);
    }
    if (tacTail == (*oldTAC)) {
        tacTail = (*newTAC);
    } else {
        (*oldTAC)->next->prev = (*newTAC);
    }

    (*newTAC)->prev = (*oldTAC)->prev;
    (*newTAC)->next = (*oldTAC)->next;

    freeTAC(oldTAC);
}

void initFuncTAC(char* funcName, VarType returnType) {
    //  It shouldn't be possible for this to run while already in a function TAC
    // Should that ever change, insert a check to see if we're in global scope
    // at the start.

    //Generate the label name of the function
    int labelLength = snprintf(NULL, 0, "%s_func", funcName); //Get length of const sum so we can allocate appropriate string length
    char* labelName = malloc(labelLength + 1);
    snprintf(labelName, labelLength + 1, "%s_func", funcName);

    //Create new FuncTAC struct for head
    FuncTAC* newFuncTacHead = malloc(sizeof(FuncTAC));
    newFuncTacHead->func = NULL;
    newFuncTacHead->nextFunc = NULL;
    newFuncTacHead->funcName = labelName;
    newFuncTacHead->returnType = returnType;
    newFuncTacHead->returnsValue = false;
    
    //Create new FuncTAC struct for head
    FuncTAC* newFuncTacTail = malloc(sizeof(FuncTAC));
    newFuncTacTail->func = NULL;
    newFuncTacTail->nextFunc = NULL;
    newFuncTacTail->funcName = labelName;
    newFuncTacTail->returnType = returnType;
    // newFuncTacTail->returnType = false; //Should be unused

    // Append to existing lists
    //  TODO: Might start tracking the tail of the FuncTAC lists. Doing so would
    //      reduce the time complexity of the following tail insertion from O(n)
    //      down to O(1).
    if (funcTacHeads) {
        FuncTAC** currentHead = &funcTacHeads;  //Cursor at start of linked lists
        FuncTAC** currentTail = &funcTacTails;
        while ((*currentHead)->nextFunc) {
            currentHead = &((*currentHead)->nextFunc);  //Traverse to end of linked lists
            currentTail = &((*currentTail)->nextFunc);
        }
        (*currentHead)->nextFunc = newFuncTacHead;  //Insert new TAC at end of lists
        (*currentTail)->nextFunc = newFuncTacTail;
    }
    else
    {
        //First function TAC generated, set up linked lists
        funcTacHeads = newFuncTacHead;
        funcTacTails = newFuncTacTail;
    }

    // Set up the new TAC to be modified by other functions
    currentTacHead = &(newFuncTacHead->func); 
    currentTacTail = &(newFuncTacTail->func); 
    
    // Used for easy access to function name
    currentFuncTAC = newFuncTacHead;

    //Every functionTAC should start with "funcStart"
    //  This operation signals MIPS to push the address in $ra to the stack
    //  This is crucial for returning after a function call
    TAC* startInstr = createTAC(
        NULL,
        NULL,
        "funcStart",
        NULL
    );
    appendTAC(currentTacHead, currentTacTail, startInstr);

    inFunction = true;
}

void finalizeFuncTAC() {
    //If non-void function hasn't returned a value, throw an error
    if ((currentFuncTAC->returnType != VarType_Void) && (!currentFuncTAC->returnsValue))
    {
        printf("SEMANTIC: Non-void function %s has no return statement! Halting...\n", currentFuncTAC->funcName);
        exit(1);
    }
    
    //Every functionTAC should end with "return"
    //  This operation signals MIPS to pop a return address from the stack and jump to it
    TAC* returnInstr = createTAC(
        NULL,
        NULL,
        "return",
        NULL
    );
    appendTAC(currentTacHead, currentTacTail, returnInstr);

    currentTacHead = &tacHead;
    currentTacTail = &tacTail;
    currentFuncTAC = NULL;
    inFunction = false;
}
