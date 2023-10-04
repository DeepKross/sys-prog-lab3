#include <stdio.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    char value[255];
    char type[50];
} Lexeme;

Lexeme lexemeTable[1000];
int lexemeCount = 0;

char *reservedWords[] = {
        "program", "var", "const", "type", "function",
        "procedure", "begin", "end", "if", "then",
        "else", "while", "do", "for", "to", "downto",
        "repeat", "until", "case", "of", "record",
        "array", "file", "set", "goto", "label", "and",
        "or", "not", "div", "mod", "in", "with",
        "nil", "true", "false", NULL
};
char *operators[] = {"+", "-", "*", "/", "=", "<>", "<", ">", "<=", ">=", "and", "or", "not", NULL};

int isOperatorChar(char c) {
    return strchr("+-*/=<>:", c) != NULL;
}

void addLexeme(char *value, char *type) {
    strcpy(lexemeTable[lexemeCount].value, value);
    strcpy(lexemeTable[lexemeCount].type, type);
    lexemeCount++;
}

void processToken(char *token) {
    int isReserved = 0;
    int isNumber = 0;
    int isStringOrCharConstant = 0;
    int isOperator = 0;

    for (int i = 0; operators[i] != NULL; i++) {
        if (strcmp(token, operators[i]) == 0 || strcmp(token, ":=") == 0) {
            addLexeme(token, "Operator");
            isOperator = 1;
            break;
        }
    }

    // reserved words check
    for (int i = 0; reservedWords[i] != NULL; i++) {
        if (strcmp(token, reservedWords[i]) == 0) {
            addLexeme(token, "Reserved Word");
            isReserved = 1;
            break;
        }
    }

    // Check for numbers
    if (isdigit(token[0]) || (token[0] == '.' && isdigit(token[1]))) {
        char *dotPointer = strchr(token, '.');
        if (dotPointer) {
            if (strchr(dotPointer + 1, '.')) {
                addLexeme(token, "Error: Multiple decimal points");
            } else {
                addLexeme(token, "Floating-Point Number");
            }
        } else {
            addLexeme(token, "Decimal Number");
        }
        isNumber = 1;
    }

    // Check for string and char constants
    if (token[0] == '\'') {
        if (strlen(token) == 3 && token[2] == '\'') {
            // Single-character constant
            addLexeme(token, "Character Constant");
            isStringOrCharConstant = 1;
        } else if (token[strlen(token) - 1] == '\'') {
            // String constant
            addLexeme(token, "String Constant");
            isStringOrCharConstant = 1;
        } else {
            // Error: unmatched single quote
            addLexeme(token, "Error: Unmatched single quote");
            isStringOrCharConstant = 1;
        }
    }

    // Check for delimiters
    if (strlen(token) == 1 && strchr(";:,.()[]{}", token[0]) != NULL) {
        addLexeme(token, "Delimiter");
    } else if (!isReserved && !isNumber && !isStringOrCharConstant && !isOperator) {
        addLexeme(token, "Identifier");
    }
}

void processComment(char *programText, int *i, char *tokenBuffer, int *bufferIndex, int *inComment) {
    char currentChar = programText[*i];
    tokenBuffer[(*bufferIndex)++] = currentChar;
    if ((*inComment == 1 && currentChar == '}') ||
        (*inComment == 2 && currentChar == ')' && programText[*i-1] == '*')) {
        tokenBuffer[*bufferIndex] = '\0';
        addLexeme(tokenBuffer, "Comment");
        *bufferIndex = 0;
        *inComment = 0;
    }
}

void processStringConstant(char *programText, int *i, char *tokenBuffer, int *bufferIndex, int *inStringConstant) {
    char currentChar = programText[*i];
    tokenBuffer[(*bufferIndex)++] = currentChar;
    if (currentChar == '\'') {
        tokenBuffer[*bufferIndex] = '\0';
        processToken(tokenBuffer);
        *bufferIndex = 0;
        *inStringConstant = 0;
    }
}

void processDirective(char *programText, int *i, char *tokenBuffer, int *bufferIndex) {
    char currentChar = programText[*i];
    tokenBuffer[(*bufferIndex)++] = currentChar;
    while (programText[*i + 1] != '}') {
        (*i)++;
        tokenBuffer[(*bufferIndex)++] = programText[*i];
    }
    (*i)++;  // Skip the closing brace
    tokenBuffer[*bufferIndex] = '\0';
    addLexeme(tokenBuffer, "Directive");
    *bufferIndex = 0;
}

void processHexNumber(char *programText, int *i, char *tokenBuffer, int *bufferIndex) {
    char currentChar = programText[*i];
    if (programText[*i + 1] == '{') {  // check if it's a directive
        processDirective(programText, i, tokenBuffer, bufferIndex);
        return;
    }
    tokenBuffer[(*bufferIndex)++] = currentChar;
    while (isxdigit(programText[*i + 1])) {
        (*i)++;
        tokenBuffer[(*bufferIndex)++] = programText[*i];
    }
    tokenBuffer[*bufferIndex] = '\0';
    addLexeme(tokenBuffer, "Hexadecimal Number");
    *bufferIndex = 0;
}

void processNumber(char *programText, int *i, char *tokenBuffer, int *bufferIndex) {
    char currentChar = programText[*i];
    tokenBuffer[(*bufferIndex)++] = currentChar;
    while(isdigit(programText[*i + 1]) || programText[*i + 1] == '.') {
        (*i)++;
        tokenBuffer[(*bufferIndex)++] = programText[*i];
    }
    tokenBuffer[*bufferIndex] = '\0';
    processToken(tokenBuffer);
    *bufferIndex = 0;
}

void lexAnalysis(char *programText) {
    char tokenBuffer[255] = {0};
    int bufferIndex = 0;
    int inStringConstant = 0;
    int inComment = 0;
    int inDirective = 0;

    for (int i = 0; i < strlen(programText); i++) {
        char currentChar = programText[i];

        if (inComment) {
            processComment(programText, &i, tokenBuffer, &bufferIndex, &inComment);
        } else if (inDirective) {
            processDirective(programText, &i, tokenBuffer, &bufferIndex);
            inDirective = 0;
        } else if (!inStringConstant && currentChar == '{') {
            if (programText[i + 1] == '$') {
                inDirective = 1;
                tokenBuffer[bufferIndex++] = currentChar;
            } else {
                inComment = 1;
                tokenBuffer[bufferIndex++] = currentChar;
            }
        } else if (!inStringConstant && currentChar == '(' && programText[i+1] == '*') {
            inComment = 2;
            tokenBuffer[bufferIndex++] = currentChar;
            tokenBuffer[bufferIndex++] = programText[++i];
        } else {
            if (inStringConstant) {
                processStringConstant(programText, &i, tokenBuffer, &bufferIndex, &inStringConstant);
            } else if (currentChar == '\'') {
                inStringConstant = 1;
                tokenBuffer[bufferIndex++] = currentChar;
            } else if (currentChar == '$' && programText[i + 1] == '{') {
                // Recognize directive start
                processDirective(programText, &i, tokenBuffer, &bufferIndex);
            } else if (currentChar == '$') {
                processHexNumber(programText, &i, tokenBuffer, &bufferIndex);
            } else if (strchr(" \t\n;,.()[]{}", currentChar) != NULL || programText[i + 1] == '\0') {
                if (bufferIndex > 0) {
                    tokenBuffer[bufferIndex] = '\0';
                    processToken(tokenBuffer);
                    bufferIndex = 0;
                }

                if (strchr(";:,.()[]{}", currentChar) != NULL) {
                    char delimiter[2] = {currentChar, '\0'};
                    processToken(delimiter);
                }
            } else if (isdigit(currentChar) || (currentChar == '.' && isdigit(programText[i + 1]))) {
                processNumber(programText, &i, tokenBuffer, &bufferIndex);
            } else if (isOperatorChar(currentChar)) {
                tokenBuffer[bufferIndex++] = currentChar;
                if (isOperatorChar(programText[i + 1]) || programText[i + 1] == '=') {
                    i++;
                    tokenBuffer[bufferIndex++] = programText[i];
                }
                tokenBuffer[bufferIndex] = '\0';
                processToken(tokenBuffer);
                bufferIndex = 0;
            } else {
                tokenBuffer[bufferIndex++] = currentChar;
            }
        }
    }
}

void displayLexemes() {
    for (int i = 0; i < lexemeCount; i++) {
        printf("< %s | %s >\n", lexemeTable[i].value, lexemeTable[i].type);
    }
}

int main() {
    char programText[] = "program ExampleProgram;\n"
                         "{$MODE DELPHI}  { Set mode to Delphi }\n"
                         "{$DEFINE DEBUG} { Define DEBUG symbol }\n"
                         "{$IFDEF DEBUG}  { Check if DEBUG is defined }"
                         "var\n"
                         "begin\n"
                         "  decimalNumber := 10;\n"
                         "  hexNumber := $1A;  { Hexadecimal representation of 10 }\n"
                         "  floatingPointNumber := 10.5;\n"
                         "  {*we are the champions*}\n"
                         "  aaa := aaa * ddd / ccc;\n"
                         "  { Display the sum }\n"
                         "  sum := decimalNumber + floatingPointNumber;\n"
                         "  writeln('Sum = ', 2 >= 3.2341);\n"
                         "end.";
    lexAnalysis(programText);
    displayLexemes();
    return 0;
}
