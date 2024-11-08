# Compiler and flags
CC = gcc
CFLAGS = -Wall -g

# Files
LEXER = lex.yy.c
PARSER = parser.tab.c
AST = AST.c
SYMBOL_TABLE = symbolTable.c
SEMANTIC = semantic.c
CODE_GENERATOR = codeGenerator.c
OPTIMIZER = optimizer.c
OPERAND_STACK = operandStack.c

TYPES = commons/types.c

# Header Files
HEADERS = AST.h codeGenerator.h symbolTable.h semantic.h parser.tab.h operandStack.h codeGenerator.h commons/types.h
# COMMONS = types.h

# Object Files
OBJS = $(LEXER:.c=.o) $(PARSER:.c=.o) $(AST:.c=.o) $(SYMBOL_TABLE:.c=.o) $(SEMANTIC:.c=.o) $(CODE_GENERATOR:.c=.o) $(OPTIMIZER:.c=.o) $(OPERAND_STACK:.c=.o) $(TYPES:.c=.o)

# Output executable
EXEC = parser

# Directory to store MIPS output
INPUT_DIR = samples
OUTPUT_DIR = output

# Format for Function TAC files
FUNCTION_TAC := FunctionTAC[0-9].ir
FUNCTION_TAC2 := FunctionTAC[0-9][0-9]*.ir

# Default rule to build the executable
all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJS)

# Rules to generate .o files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Flex and Bison
lex.yy.c: lexer.l parser.tab.h
	flex lexer.l

parser.tab.c parser.tab.h: parser.y
	bison -d -v -t parser.y

# Run the parser and redirect output to output.txt
run: $(EXEC)
	./$(EXEC) $(INPUT_DIR)/testProg.cmm > $(OUTPUT_DIR)/output.txt
	@echo "Parsing output saved to $(OUTPUT_DIR)/output.txt"

# Generate the MIPS assembly and save to output.asm
mips: $(EXEC)
	./$(EXEC) $(INPUT_DIR)/classProg.cmm > $(OUTPUT_DIR)/output.txt # This generates MIPS assembly via codeGenerator.c
	@echo "MIPS code saved to $(OUTPUT_DIR)/output.asm"
	@echo "Output log saved to $(OUTPUT_DIR)/output.asm"

# Test 1: Online Classroom example
test1: $(EXEC)
	./$(EXEC) $(INPUT_DIR)/testProg1.cmm > $(OUTPUT_DIR)/output.txt # This generates MIPS assembly via codeGenerator.c
	@echo "MIPS code saved to $(OUTPUT_DIR)/output.asm"
	@echo "Output log saved to $(OUTPUT_DIR)/output.txt"

# Test 2: Simple expressions
test2: $(EXEC)
	./$(EXEC) $(INPUT_DIR)/testProg2.cmm > $(OUTPUT_DIR)/output.txt # This generates MIPS assembly via codeGenerator.c
	@echo "MIPS code saved to $(OUTPUT_DIR)/output.asm"
	@echo "Output log saved to $(OUTPUT_DIR)/output.txt"

# Test 3: Modified classroom example - demonstrates preservation of associativity
test3: $(EXEC)
	./$(EXEC) $(INPUT_DIR)/testProg3.cmm > $(OUTPUT_DIR)/output.txt # This generates MIPS assembly via codeGenerator.c
	@echo "MIPS code saved to $(OUTPUT_DIR)/output.asm"
	@echo "Output log saved to $(OUTPUT_DIR)/output.txt"

# Test 4: Function Call Test
test4: $(EXEC)
	./$(EXEC) $(INPUT_DIR)/testProg4.cmm > $(OUTPUT_DIR)/output.txt # This generates MIPS assembly via codeGenerator.c
	@echo "MIPS code saved to $(OUTPUT_DIR)/output.asm"
	@echo "Output log saved to $(OUTPUT_DIR)/output.txt"

test5: $(EXEC)
	./$(EXEC) $(INPUT_DIR)/testProg5.cmm > $(OUTPUT_DIR)/output.txt # This generates MIPS assembly via codeGenerator.c
	@echo "MIPS code saved to $(OUTPUT_DIR)/output.asm"
	@echo "Output log saved to $(OUTPUT_DIR)/output.txt"

test6: $(EXEC)
	./$(EXEC) $(INPUT_DIR)/testProg6.cmm > $(OUTPUT_DIR)/output.txt # This generates MIPS assembly via codeGenerator.c
	@echo "MIPS code saved to $(OUTPUT_DIR)/output.asm"
	@echo "Output log saved to $(OUTPUT_DIR)/output.txt"

# Debug with gdb
debug: $(EXEC)
	gdb --args $(EXEC) $(INPUT_DIR)/testProg6.cmm

# Clean rule to remove compiled files
clean:
	rm -f $(EXEC) $(OBJS) parser.tab.c parser.tab.h lex.yy.c parser.output
	rm -f $(OUTPUT_DIR)/output.txt $(OUTPUT_DIR)/output.asm $(OUTPUT_DIR)/TAC.ir $(OUTPUT_DIR)/TACOptimized.ir $(OUTPUT_DIR)/$(FUNCTION_TAC) $(OUTPUT_DIR)/$(FUNCTION_TAC2)

