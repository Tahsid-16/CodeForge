# Compiler & Tooling Configuration
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Isrc -Ibuild
FLEX     = flex
BISON    = bison

# Directories & Output Binary
BUILD_DIR = build
TARGET    = codeforge_compiler

# Generated Lexer & Parser Sources
PARSER_SRC = $(BUILD_DIR)/parser.tab.cpp
PARSER_HDR = $(BUILD_DIR)/parser.tab.hpp
LEXER_SRC  = $(BUILD_DIR)/lex.yy.cpp

# Core C++ Modules (Automatically discovers .cpp files in each module folder)
CPP_SRCS = src/main.cpp \
           $(wildcard src/common/*.cpp) \
           $(wildcard src/ast/*.cpp) \
           $(wildcard src/symbol_table/*.cpp) \
           $(wildcard src/semantic/*.cpp) \
           $(wildcard src/codegen/*.cpp)

ALL_SRCS = $(PARSER_SRC) $(LEXER_SRC) $(CPP_SRCS)
OBJS     = $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(notdir $(ALL_SRCS)))

# Default Target
all: $(TARGET)

# Rule 1: Bison Parser Generation (Must run BEFORE Flex so token definitions exist)
$(PARSER_SRC) $(PARSER_HDR): src/parser/parser.y
	@mkdir -p $(BUILD_DIR)
	$(BISON) -d -o $(PARSER_SRC) src/parser/parser.y

# Rule 2: Flex Lexer Generation (Depends on parser.tab.hpp for token IDs)
$(LEXER_SRC): src/lexer/lexer.l $(PARSER_HDR)
	@mkdir -p $(BUILD_DIR)
	$(FLEX) -o $(LEXER_SRC) src/lexer/lexer.l

# Rule 3: Link all object files into the final compiler binary
$(TARGET): $(PARSER_SRC) $(LEXER_SRC) $(CPP_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(ALL_SRCS) -o $(TARGET)

# Rule 4: Run automated test suite
test: $(TARGET)
	@echo "Running CodeForge automated tests..."
	bash tests/test_lexer.sh
	bash tests/test_parser.sh
	bash tests/test_semantic.sh

# Rule 5: Clean all generated artifacts and executables
clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TARGET).exe

.PHONY: all test clean