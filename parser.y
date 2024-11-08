%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "AST.h"
#include "symbolTable.h"
#include "semantic.h"
#include "codeGenerator.h"
#include "optimizer.h"
#include "commons/types.h"

#define TABLE_SIZE 100
#define MAX_ID_LENGTH 10
#define VAR_SUFFIX_LENGTH 4

bool drawVertical[100] = { false }; // Adjust size as needed

extern int yylex();   // Declare yylex, the lexer function
extern int yyparse(); // Declare yyparse, the parser function
extern FILE* yyin;    // Declare yyin, the file pointer for the input file
extern int yylineno;  // Declare yylineno, the line number counter
extern TAC* tacHead;  // Declare the head of the linked list of TAC entries



void yyerror(const char* s);
char* getMipsVarName(char* varName, char* currentFunction);

extern int chars;
extern int lines;

ASTNode* root = NULL; 
SymbolTable* symTab = NULL;
Symbol* symbol = NULL;

char* currentFunction = NULL; 	//Name of current function-- used for scope
								//	NULL when not in a function

%}

%printer { fprintf(yyoutput, "%s", $$); } ID;

%union {
    char* sval;
    int intVal;
    float floatVal;
	char charVal;
    struct ASTNode* ast;
}

%token <sval> TYPE
%token <sval> ID
%token SEMI
%token <sval> ASSIGN
/* %token PLUS MINUS MULTIPLY DIVIDE POWER */
%token <intVal> INT_NUMBER
%token <floatVal> FLOAT_NUMBER
%token <charVal> CHAR_VALUE
%token WRITE
%token ARRAY
%token RETURN
%token LPAREN RPAREN LBRACE RBRACE LBRACKET RBRACKET COMMA
/* %token LBRACKET RBRACKET */
/* %token LPAREN RPAREN */
/* %token FUNCTION RETURN ARRAY */

%left <sval> PLUS MINUS
%left <sval> MULTIPLY DIVIDE
/* %right POWER */
/* %nonassoc UMINUS */

%type <ast> Program VarDecl VarDeclList DeclList Stmt StmtList Expr
%type <ast> FuncDecl ParamList Param FuncCall ArgList Arg ReturnStmt
/* %type <ast> FuncDeclList FuncDecl FuncCall ParamList ArgList */
/* %type <ast> ArrayDecl ArrayIndex */

%start Program

%%

Program: DeclList StmtList    { printf("The PARSER has started\n"); 
									root = createNode(NodeType_Program);
									root->data.program.varDeclList = $1;
									root->data.program.stmtList = $2;
									// Set other fields as necessary
								}

;

//I don't know why, I don't WANT to know why, but for some reason
//function declarations just don't work unless we merge VarDeclList
//and FuncDeclList into one node type.

//I guess it's a win-win, it makes the grammar less strict... but why?
//Probably some jank because FuncDecl's start with TYPE ID and Bison
//fails to cut off the VarDeclList because it's still trying to parse
//the next variable. Maybe? Don't know for sure.

// -Spencer

DeclList:  {/*empty, i.e. it is possible not to declare a variable*/
				// $$ = createNode(NodeType_DeclList); //Empty ASTNode prevents Unknown Node Error
				$$ = NULL;
			  }
	| VarDecl DeclList {  printf("PARSER: Recognized declaration list (variable entry)\n"); 

							// Create AST node for VarDeclList
							$$ = createNode(NodeType_DeclList);
							$$->data.declList.decl = $1;
							$$->data.declList.next = $2;
				
							// Set other fields as necessary

							
							}
	| FuncDecl DeclList {  printf("PARSER: Recognized declaration list (function entry)\n"); 

							// Create AST node for VarDeclList
							$$ = createNode(NodeType_DeclList);
							$$->data.declList.decl = $1;
							$$->data.declList.next = $2;
				
							// Set other fields as necessary

							
							}
;

VarDeclList:  {/*empty, i.e. it is possible not to declare a variable*/
				// $$ = createNode(NodeType_DeclList); //Empty ASTNode prevents Unknown Node Error
				$$ = NULL;
			  }
	| VarDecl VarDeclList {  printf("PARSER: Recognized function-scoped variable declaration list\n"); 

							// Create AST node for VarDeclList
							$$ = createNode(NodeType_VarDeclList);
							$$->data.varDeclList.varDecl = $1;
							$$->data.varDeclList.next = $2;
				
							// Set other fields as necessary

							
							}
							;

VarDecl: TYPE ID SEMI { printf("PARSER: Recognized variable declaration: %s\n", $2);

								printf("PARSER: Printing symbol table\n");
								printSymbolTable(symTab);

								printf("PARSER: Checking if variable has already been declared\n");

								//First, append _var to the end of the variable name
								//This will prevent conflicts with reserved instruction names in MIPS (e.g. "b")
								char* varName = getMipsVarName($2, currentFunction);
								printf("Name with suffix: %s\n", varName);
								
								// Check if variable has already been declared
								symbol = lookupSymbol(symTab, varName);
							
								if (symbol != NULL) {	// Check if variable has already been declared
									printf("PARSER: Variable %s at line %d has already been declared - COMPILATION HALTED\n", $2, yylineno);
									exit(0);
								} else {	
									// Variable has not been declared yet	
									// Create AST node for VarDecl
									$$ = createNode(NodeType_VarDecl);
									$$->data.varDecl.varType = strdup($1);
									$$->data.varDecl.varName = strdup(varName);
									// Set other fields as necessary

									// Add variable to symbol table
									addSymbol(symTab, varName, $1);
									printSymbolTable(symTab);
								}
								
							 }
		| ARRAY TYPE ID LBRACKET INT_NUMBER RBRACKET SEMI{
			printf("PARSER: Printing symbol table\n");
			printSymbolTable(symTab);

			printf("PARSER: Checking if variable has already been declared\n");

			//First, append _var to the end of the variable name
			//This will prevent conflicts with reserved instruction names in MIPS (e.g. "b")
			char* varName = getMipsVarName($3, currentFunction);
			printf("Name with suffix: %s\n", varName);
			
			
			// Check if variable has already been declared
			symbol = lookupSymbol(symTab, varName);
		
			if (symbol != NULL) {	// Check if variable has already been declared
				printf("PARSER: Variable %s at line %d has already been declared - COMPILATION HALTED\n", $2, yylineno);
				exit(0);
			} else {	
				// Variable has not been declared yet	
				// Create AST node for VarDecl

				$$ = createNode(NodeType_ArrDecl);
				$$->data.arrDecl.varType = strdup($2);
				$$->data.arrDecl.varName = strdup(varName);
				$$->data.arrDecl.arrSize = $5;
				// Set other fields as necessary

				// Add variable to symbol table
				addArrSymbol(symTab, varName, $2, $5);
				printSymbolTable(symTab);
			}

			};
		/* | TYPE ID {
                  printf ("Missing semicolon after declaring variable: %s\n", $2);
             } */

FuncDecl
    : TYPE ID 
		{
			currentFunction = $2;
		}
	  LPAREN ParamList RPAREN LBRACE VarDeclList StmtList RBRACE
        {
            //Mid-rule required in order to manage function scope easily
            $$ = createNode(NodeType_FuncDecl);
            $$->data.funcDecl.name = getMipsVarName($2, NULL);
			$$->data.funcDecl.returnType = stringToVarType($1);
			
			
			printf("PARSER: Recognized function declaration: %s\n", $2);

            // Add function to symbol table
            // addSymbol(symTab, $2, "function");
			
			addSymbol(symTab, $$->data.funcDecl.name, $1);

            // Create AST node for FuncDecl
            $$->data.funcDecl.paramList = $5;
			$$->data.funcDecl.varDeclList = $8;
            $$->data.funcDecl.stmtList = $9;

			currentFunction = NULL;
        }
    ;

ParamList
    : /* empty */
        {
            $$ = NULL;
        }
    | Param
        {
            // Create AST node for single-item parameter list
            $$ = createNode(NodeType_ParamList);
			$$->data.paramList.param = $1;
            $$->data.paramList.next = NULL;
        }
    | Param COMMA ParamList
        {
            // Create AST node for parameter list
            $$ = createNode(NodeType_ParamList);
			$$->data.paramList.param = $1;
            $$->data.paramList.next = $3;
        }
    ;

Param : TYPE ID {
			$$ = createNode(NodeType_Param);
            $$->data.param.type = stringToVarType($1);
            // $$->data.paramList.varType = strdup($1);
            // $$->data.param.name = strdup($2);

			//TODO: Var names need to be labeled with current scope
            $$->data.param.name = getMipsVarName($2, currentFunction);
			
			addSymbol(symTab, $$->data.param.name, $1);
		}
	;

StmtList:  	{/*empty, i.e. it is possible not to have any statement*/
				// $$ = createNode(NodeType_StmtList); //Empty ASTNode prevents Unknown Node Error
				$$ = NULL;
			}
	| Stmt StmtList { printf("PARSER: Recognized statement list\n");
						$$ = createNode(NodeType_StmtList);
						$$->data.stmtList.stmt = $1;
						$$->data.stmtList.stmtList = $2;
						// Set other fields as necessary
					}
;

Stmt: ID ASSIGN Expr SEMI { /* code TBD */
								printf("PARSER: Recognized assignment statement\n");
								$$ = createNode(NodeType_AssignStmt);
								char* varName = getMipsVarName($1, currentFunction);

								$$->data.assignStmt.varName = strdup(varName);
								$$->data.assignStmt.operator = strdup($2);
								$$->data.assignStmt.expr = $3;
								// Set other fields as necessary
 }
	| ID LBRACKET Expr RBRACKET ASSIGN Expr SEMI { /* code TBD */
								printf("PARSER: Recognized assignment statement\n");
								$$ = createNode(NodeType_AssignArrStmt);
								char* varName = getMipsVarName($1, currentFunction);

								$$->data.assignArrStmt.varName = strdup(varName);
								$$->data.assignArrStmt.operator = strdup($5);
								$$->data.assignArrStmt.expr = $6;
								$$->data.assignArrStmt.indexExpr = $3;
								// Set other fields as necessary
 }	
	//TODO: Allow write statement to write expr, rather than just simpleID variables
	| WRITE Expr SEMI { 	printf("PARSER: Recognized write statement\n"); 
						$$ = createNode(NodeType_WriteStmt);
						
						//Append _var to the end of the variable name
						//char* varName = getMipsVarName($2);
							
						// $$->data.writeStmt.varName = strdup(varName);

						$$->data.writeStmt.expr = $2;
					}
	| FuncCall SEMI
	| ReturnStmt SEMI
;

//TODO: Exponent binOp
Expr: Expr PLUS Expr { printf("PARSER: Recognized expression\n");
						$$ = createNode(NodeType_BinOp);
						$$->data.binOp.left = $1;
						$$->data.binOp.right = $3;
						$$->data.binOp.operator = strdup($2);
						
						// Set other fields as necessary
					  }
 	| Expr MINUS Expr { printf("PARSER: Recognized expression\n");
						$$ = createNode(NodeType_BinOp);
						$$->data.binOp.left = $1;
						$$->data.binOp.right = $3;
						$$->data.binOp.operator = strdup($2);
						
						// Set other fields as necessary
					  }
	| Expr MULTIPLY Expr { printf("PARSER: Recognized expression\n");
						$$ = createNode(NodeType_BinOp);
						$$->data.binOp.left = $1;
						$$->data.binOp.right = $3;
						$$->data.binOp.operator = strdup($2);
						
						// Set other fields as necessary
					  }
	| Expr DIVIDE Expr { printf("PARSER: Recognized expression\n");
						$$ = createNode(NodeType_BinOp);
						$$->data.binOp.left = $1;
						$$->data.binOp.right = $3;
						$$->data.binOp.operator = strdup($2);
						
						// Set other fields as necessary
					  }
	| ID { printf("ASSIGNMENT statement \n"); 
			
			$$ = createNode(NodeType_SimpleID);

			//Append _var to the end of the variable name
			char* varName = getMipsVarName($1, currentFunction);

			$$->data.simpleID.name = varName;
			// Set other fields as necessary	
		}
	| ID LBRACKET Expr RBRACKET { printf("ARRAY ACCESS statement \n"); 
			$$ = createNode(NodeType_ArrAccess);

			//Append _var to the end of the variable name
			// char varName[MAX_ID_LENGTH];
			char* varName = getMipsVarName($1, currentFunction);

			$$->data.arrAccess.name = varName;
			$$->data.arrAccess.indexExpr = $3;
			// Set other fields as necessary	
		}
	| INT_NUMBER { 
				printf("PARSER: Recognized int number\n");
				$$ = createNode(NodeType_IntExpr);
				$$->data.intExpr.number = $1;
				// Set other fields as necessary
			 }
	| FLOAT_NUMBER { 
				printf("PARSER: Recognized float number\n");
				$$ = createNode(NodeType_FloatExpr);
				$$->data.floatExpr.number = $1;
				// Set other fields as necessary
			 }
	| CHAR_VALUE { 
				printf("PARSER: Recognized character\n");
				$$ = createNode(NodeType_CharExpr);
				$$->data.charExpr.character = $1;
				// Set other fields as necessary
			 }
	| FuncCall {$$ = $1;}
	| LPAREN Expr RPAREN {$$ = $2;}
;

FuncCall : ID LPAREN ArgList RPAREN {
				$$ = createNode(NodeType_FuncCall);
				$$->data.funcCall.name = getMipsVarName($1, NULL);
				$$->data.funcCall.argList = $3;
			 }
;

ArgList
    : /* empty */
        {
            $$ = NULL;
        }
    | Arg
        {
            // Create AST node for single-item parameter list
            $$ = createNode(NodeType_ArgList);
			$$->data.argList.arg = $1;
            $$->data.argList.next = NULL;
        }
    | Arg COMMA ArgList
        {
            // Create AST node for parameter list
            $$ = createNode(NodeType_ArgList);
			$$->data.argList.arg = $1;
            $$->data.argList.next = $3;
        }
    ;

Arg : Expr
		{
			$$ = createNode(NodeType_Arg);
			$$->data.arg.expr = $1;
		}

ReturnStmt : RETURN 
			{ //No return expression
				$$ = createNode(NodeType_ReturnStmt);
				$$->data.returnStmt.returnExpr = NULL;
			}
		| RETURN Expr 
		{
				$$ = createNode(NodeType_ReturnStmt);
				$$->data.returnStmt.returnExpr = $2;
		}
%%

void yyerror(const char *s) {
    /* fprintf(stderr, "Error: %s\n", s); */
    fprintf(stderr, "Error: %s at (line %d:%d)\n", s, lines, chars);
}

int main(int argc, char **argv) {
    ++argv, --argc;  /* Skip over program name */
    if (argc > 0)
        yyin = fopen(argv[0], "r");
    else
        yyin = stdin;
	
	// Initialize symbol table
	symTab = createSymbolTable(TABLE_SIZE);
    if (symTab == NULL) {
        // Handle error
        return EXIT_FAILURE;
    }

	/* initializeTempVars(); */

	int parseCode = yyparse();
    if (parseCode == 0)
	{
        // Successfully parsed
		printf("Parsing successful!\n");
        traverseAST(root, 0, drawVertical, false);
		// Print symbol table for debugging
		printSymbolTable(symTab);
		// Semantic analysis
		printf("\n=== SEMANTIC ANALYSIS ===\n\n");
		initSemantic(symTab);
		semanticAnalysis(root);
		printf("\n=== TAC GENERATION ===\n");
		printTACToFile("output/TAC.ir", tacHead);
		printFuncTACsToFile();

		// Code optimization
		printf("\n=== CODE OPTIMIZATION ===\n");
		// Traverse the linked list of TAC entries and optimize
		// But - you MIGHT need to traverse the AST again to optimize

		/* optimizeTAC(tacHead); */
		/* printOptimizedTAC("output/TACOptimized.ir", tacHead); */

		// Code generation
		printf("\n=== CODE GENERATION ===\n");
		/* initCodeGenerator("output/output.asm", symTab); */
		initCodeGenerator("output/output.asm");
		/* generateMIPS(tacHead); */
		generateMIPS(tacHead, symTab);
		finalizeCodeGenerator("output/output.asm");

        freeAST(root);
		freeSymbolTable(symTab);
	}
    else
        printf("Parsing failed. (error code: %d)\n", parseCode);

    return 0;
}

//Append _var to name of variable
//This will prevent conflicts with reserved instruction names in MIPS (e.g. "b")
/* void getMipsVarName(char* returnBuffer, unsigned int bufferSize, char* varName)
{
	if ((strlen(varName) + VAR_SUFFIX_LENGTH) < bufferSize)
	{
		strcpy(returnBuffer, varName);
		strcat(returnBuffer, "_var");
	}
	else
	{
		fprintf(stderr, "Error: ID length too long (%s)\n", varName);
		exit(1);
	}
} */
/* char* getMipsVarName(char* varName) {
    int idLength = snprintf(NULL, 0, "%s%s", varName, "_var"); //Get length of const sum so we can allocate appropriate string length
	char* idWithSuffix = malloc(idLength + 1);
    snprintf(idWithSuffix, idLength + 1, "%s%s", varName, "_var"); //Get length of const sum so we can allocate appropriate string length
	return idWithSuffix;
} */
