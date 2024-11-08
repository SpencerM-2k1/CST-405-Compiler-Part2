#ifndef AST_H
#define AST_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "commons/types.h"

// NodeType enum to differentiate between different kinds of AST nodes
typedef enum { 
    NodeType_Program,       //root
    NodeType_DeclList,      //Linked list containing var/arr and func declaration nodes
    NodeType_VarDeclList,   //Linked list containing var/arr declaration nodes (used for functions)
    NodeType_VarDecl,       //Declaration of a var
    NodeType_ArrDecl,       //Declaration of an array
    // NodeType_FuncDeclList,  //Linked list containing function declaration nodes
    NodeType_FuncDecl,      //Declaration of a function
    NodeType_ParamList,     //Linked list containing function parameter nodes
    NodeType_Param,         //Function parameter node
    NodeType_IntExpr,       //Int literal
    NodeType_FloatExpr,     //Float literal
    NodeType_CharExpr,      //Char literal
    NodeType_SimpleID,      //Variable ID
    NodeType_ArrAccess,     //Array ID + Index
    NodeType_FuncCall,      //Function call
    NodeType_ArgList,       //Function Argument List
    NodeType_Arg,           //Individual Function Argument
    NodeType_ReturnStmt,    //Return Statement
    NodeType_BinOp,         //Binary operator
    NodeType_StmtList,      //Linked list containing statement nodes
    NodeType_AssignStmt,    //Assign a value to a variable
    NodeType_AssignArrStmt, //Assign a value to the index of an array
    NodeType_WriteStmt,     //Print an expression to the console
} NodeType;

// Forward declaration of ASTNode to use in struct definitions
struct ASTNode;

// Structure for AST nodes
typedef struct ASTNode {
    NodeType type;
    union NodeData {
        struct ProgramNode {
            struct ASTNode* varDeclList;
            struct ASTNode* stmtList;
        } program;

        struct DeclListNode {
            struct ASTNode* decl;
            struct ASTNode* next;
        } declList;

        struct VarDeclListNode {
            struct ASTNode* varDecl;
            struct ASTNode* next;
        } varDeclList;

        struct VarDeclNode {
            char* varType;
            char* varName;
        } varDecl;

        struct ArrayNode {
            char* varType;
            char* varName;
            int arrSize;
        } arrDecl;

        // struct FuncDeclListNode {
        //     struct ASTNode* funcDecl;
        //     struct ASTNode* next;
        // } funcDeclList;

        struct FuncDeclNode {
            char* name;
            VarType returnType;
            struct ASTNode* paramList;
            struct ASTNode* varDeclList;
            struct ASTNode* stmtList;
        } funcDecl;

        struct ParamListNode {
            struct ASTNode* param;
            struct ASTNode* next;
        } paramList;

        struct ParamNode {
            VarType type;
            char* name;
        } param;

        struct IntExprNode {
            int number;
        } intExpr;

        struct FloatExprNode {
            float number;
        } floatExpr;

        struct CharExprNode {
            char character;
        } charExpr;

        struct SimpleIDNode {
            char* name;
        } simpleID;

        struct ArrAccessNode {
            char* name;
            struct ASTNode* indexExpr;
        } arrAccess;

        struct FuncCallNode {
            char* name;
            struct ASTNode* argList;
            bool ignoreReturn;  //Might be unnecessary, might help prevent a memory leak in the operand stack
                                //(I'm worried that calling a function outside of an assign statement might
                                // leave an operand on the stack, forever unused. - Spencer Meren)
        } funcCall;

        struct ArgListNode {
            struct ASTNode* arg;
            struct ASTNode* next;
        } argList;

        struct ArgNode {
            struct ASTNode* expr;
        } arg;

        struct ReturnStmtNode {
            struct ASTNode* returnExpr;    //Will be NULL for empty return calls
        } returnStmt;

        struct BinOpNode {
            char* operator;
            struct ASTNode* left;
            struct ASTNode* right;
        } binOp;

        struct StmtListNode {
            struct ASTNode* stmt;
            struct ASTNode* stmtList;
        } stmtList;

        struct AssignStmtNode {
            char* operator; // e.g., '='
            char* varName;
            struct ASTNode* expr;
        } assignStmt;

        struct AssignArrStmtNode {
            char* operator; // e.g., '='
            char* varName;
            struct ASTNode* indexExpr;
            struct ASTNode* expr;
        } assignArrStmt;

        struct WriteStmtNode {
            struct ASTNode* expr;
        } writeStmt;
    } data;
} ASTNode;

// Function prototypes for AST handling
ASTNode* createNode(NodeType type);
void freeAST(ASTNode* node);
void traverseAST(ASTNode* node, int level, bool* drawVertical, bool isLast);
void printIndent(bool* drawVertical, int level);

#endif // AST_H

