#ifndef ccvm_assembler_assembler
#define ccvm_assembler_assembler

#define BOOL char
#define TRUE 1
#define FALSE 0

#define CCA_TOK_IDENTIFIER 0
#define CCA_TOK_NUMBER 1
#define CCA_TOK_DIVIDER 2
#define CCA_TOK_OPCODE 3
#define CCA_TOK_REGISTER 4
#define CCA_TOK_LABEL 5
#define CCA_TOK_END 6
#define CCA_TOK_ADDRESS 7
#define CCA_TOK_STRING 8

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

typedef struct cca_file_content {
	unsigned int fileSize;
	char* content;
} cca_file_content;

typedef struct cca_token {
	char type;
	union value {
		unsigned int numeric;
		char* string;
	} value;
} cca_token;

typedef struct cca_marker {
	char* name;
	unsigned int marks;
} cca_marker;

typedef struct cca_definition {
	char* name;
	char* value;
	unsigned int pointer;
} cca_definition;

typedef struct cca_definition_list {
	cca_definition* definitions;
	unsigned int length;
} cca_definition_list;

char* cca_token_type_str(char type) {
	switch (type) {
		case 0: return "identifier";
		case 1: return "number";
		case 2: return "divider";
		case 3: return "opcode";
		case 4: return "register";
		case 5: return "label";
		case 6: return "end";
		case 7: return "address";
		case 8: return "string";
		default: return "unknown";
	}
}

void cca_token_print(cca_token tok) {
	if (tok.type == 1 || tok.type == 7) {
		printf("Token[type: %s(%d), value: %d]\n", cca_token_type_str(tok.type), tok.type, tok.value.numeric);
	} else {
		printf("Token[type: %s(%d), value: %s]\n", cca_token_type_str(tok.type), tok.type, tok.type == 6 ? "EOP" : tok.value.string);
	}
}

cca_file_content ccvm_program_load(char *filename) {
    cca_file_content content;

    // open file
    FILE *fp = fopen(filename, "rb");

    // error detection
    if (fp == NULL) {
        printf("[ERROR] could not open file: %s\n", filename);
        exit(1);
    }

    // get buffer size
    fseek(fp, 0L, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    // buffer allocation
    char *buffer = (char *) malloc(size + 1 * sizeof(char*));

    fread(buffer, 1, size, fp);

    content.fileSize = size;
    content.content = buffer;

    return content;
}

// recognizer functions
char cca_is_identifier(char character) {
	return (isalpha(character) || character == '_');
}

char cca_is_number(char character) {
	return character <= '9' && character >= '0';
}

char cca_is_ignorable(char character) {
	return (character == ' ' || character == '\n' || character == '\t' || character == '\r');
}

char cca_is_divider(char character) {
	return character == ',';
}

char cca_is_address(char character) {
	return character == '&';
}

char cca_is_comment(char character) {
	return character == ';';
}

char cca_is_string(char character) {
	return character == '\'' || character == '"' || character == '`';
}

char cca_is_marker(char character) {
	return character == ':';
}

char strContainedIn(char* string, char* array[], unsigned int arrayLength) {
	for (int i = 0; i < arrayLength; i++) {
		if (strcmp(string, array[i]) == 0) {
			return 1;
		}
	}

	return 0;
}

char cca_is_opcode_or_register(cca_token t) {
	char* mnemonics[] = { "mov", "stp", "psh", "pop", "dup", "mov", "add", "sub", "mul", "div", "not", "and", "or", "xor", "jmp", "cmp", "frs", "inc", "dec", "call", "ret", "syscall", "je", "jne", "jg", "js", "jo" };
	unsigned int mnemonicsCount = 27;

	if (strContainedIn(t.value.string, mnemonics, mnemonicsCount))
		return CCA_TOK_OPCODE;

	else if (strcmp(t.value.string, "a") == 0 || strcmp(t.value.string, "b") == 0 || strcmp(t.value.string, "c") == 0 || strcmp(t.value.string, "d") == 0)
		return CCA_TOK_REGISTER;
		
	else
		return CCA_TOK_IDENTIFIER;
}

// parse functions
cca_token cca_parse_identifier(char* code, unsigned int* readingPos) {
	cca_token tok = {0};
	tok.type = CCA_TOK_IDENTIFIER;

	unsigned int stringCap = 100;
	unsigned int stringLen = 0;
	char* string = malloc(stringCap * sizeof(char));

	while(cca_is_identifier(code[*readingPos])) {
		++stringLen;

		if (stringLen >= stringCap) {
			stringCap *= 2;
			string = realloc(string, stringCap * sizeof(char));
		}

		string[stringLen-1] = code[*readingPos];

		++*readingPos;
	}

	--*readingPos;
	string = realloc(string, stringLen * sizeof(char));
	string[stringLen] = '\0';

	tok.value.string = string;

	return tok;
}

cca_token cca_parse_number(char* code, unsigned int* readingPos) {
	unsigned int n = 0;

	cca_token tok = {0};
	tok.type = CCA_TOK_NUMBER;

	while(cca_is_number(code[*readingPos])) {
		n = n*10 + code[*readingPos] - 48;
		++*readingPos;
	}

	tok.value.numeric = n;
	--*readingPos;
	return tok;
}

cca_token cca_parse_address(char* code, unsigned int* readingPos) {
	unsigned int n = 0;

	cca_token tok = {0};
	tok.type = CCA_TOK_ADDRESS;

	++*readingPos;
	while(cca_is_number(code[*readingPos])) {
		n = n * 10 + code[*readingPos] - 48;
		++*readingPos;
	}

	tok.value.numeric = n;
	--*readingPos;
	return tok;
}

void cca_parse_comment(char* code, unsigned int* readingPos) {
	while(code[*readingPos] != '\n') {
		++*readingPos;
	}
}

cca_marker cca_parse_marker(char* code, unsigned int* readingPos) {
	cca_marker mark = {0};

	unsigned int stringCap = 100;
	unsigned int stringLen = 0;

	char* string = malloc(stringCap * sizeof(char));
	++*readingPos;
	while(cca_is_identifier(code[*readingPos])) {
		++stringLen;

		if (stringLen >= stringCap) {
			stringCap *= 2;
			string = realloc(string, stringCap * sizeof(char));
		}

		string[stringLen-1] = code[*readingPos];

		++*readingPos;
	}

	--*readingPos;
	string = realloc(string, stringLen * sizeof(char));
	string[stringLen] = '\0';

	mark.name = string;

	return mark;
}

cca_token cca_parse_string(char* code, unsigned int* readingPos) {
	cca_token tok = {0};
	tok.type = 8;
	unsigned int stringCap = 100;
	unsigned int stringLen = 1;
	char* string = malloc(stringCap * sizeof(char));
	char quote = code[*readingPos];
	++*readingPos;
	
	while(code[*readingPos] != quote) {
		++stringLen;

		if (stringLen >= stringCap) {
			stringCap *= 2;
			string = realloc(string, stringCap * sizeof(char));
		}

		string[stringLen-2] = code[*readingPos];
		++*readingPos;
	}

	string = realloc(string, stringLen * sizeof(char));

	string[stringLen] = '\0';

	tok.value.string = string;
	return tok;
}

cca_token* cca_assembler_lex(cca_file_content content) {
	// file data
	unsigned int size = content.fileSize;
	char* assembly = content.content;
	unsigned int byteIndex = 0;

	// markers
	unsigned int markerCapacity = 100;
	cca_marker* markers = malloc(markerCapacity * sizeof(cca_marker));
	unsigned int markerCount = 0;

	// tokens
	unsigned int tokCapacity = 100;
	cca_token* tokens = malloc(tokCapacity * sizeof(cca_token));
	unsigned int readingPos = 0;
	unsigned int tokCount = 0;
	BOOL foundDef = FALSE;

	// first lexing loop
	while(readingPos < size - 1) {
		char current = assembly[readingPos];

		if (current == 0x00)
			break;

		if (cca_is_ignorable(current)) {
			// ignore it and continue to next itteration
		} else if (cca_is_marker(current)) {
			cca_marker newMarker = cca_parse_marker(assembly, &readingPos);
			puts("- placing BI to marker");
			newMarker.marks = byteIndex;
			++markerCount;
			if (markerCount >= markerCapacity) {
				markerCapacity *= 2;
				markers = realloc(markers, markerCapacity);
			}
			markers[markerCount - 1] = newMarker;
		} else if (cca_is_divider(current)) {
			cca_token newTok = {0};
			newTok.type = CCA_TOK_DIVIDER;
			newTok.value.string = ",";
			++tokCount;
			if (tokCount >= tokCapacity) {
				tokCapacity *= 2;
				tokens = realloc(tokens, tokCapacity);
			}
			tokens[tokCount - 1] = newTok;
		} else if (cca_is_identifier(current)){
			cca_token newTok = cca_parse_identifier(assembly, &readingPos);
			newTok.type = cca_is_opcode_or_register(newTok);
			++tokCount;
			if (tokCount >= tokCapacity) {
				tokCapacity *= 2;
				tokens = realloc(tokens, tokCapacity);
			}
			tokens[tokCount - 1] = newTok;
			byteIndex += 1;

			if (foundDef) {
				foundDef = FALSE;
				--byteIndex;
			} else if (newTok.type == CCA_TOK_IDENTIFIER && strcmp(newTok.value.string, "def") == 0) {
				foundDef = TRUE;
				--byteIndex;
			} else if (newTok.type == CCA_TOK_IDENTIFIER) {
				byteIndex += 3;
				printf("- found identifier increasing, BI is now %d\n", byteIndex);
			} else
				printf("- found opcode/register increasing, BI is now %d\n", byteIndex);
		} else if (cca_is_number(current)) {
			cca_token newTok = cca_parse_number(assembly, &readingPos);
			++tokCount;
			if (tokCount >= tokCapacity) {
				tokCapacity *= 2;
				tokens = realloc(tokens, tokCapacity);
			}
			tokens[tokCount - 1] = newTok;
			byteIndex += 4;
			printf("- found number increasing, BI is now %d\n", byteIndex);
		} else if (cca_is_address(current)){
			cca_token newTok = cca_parse_address(assembly, &readingPos);
			++tokCount;
			if (tokCount >= tokCapacity) {
				tokCapacity *= 2;
				tokens = realloc(tokens, tokCapacity);
			}
			tokens[tokCount - 1] = newTok;
			byteIndex += 4;
			printf("- found address increasing, BI is now %d\n", byteIndex);
		} else if (cca_is_string(current)) {
			cca_token newTok = cca_parse_string(assembly, &readingPos);
			++tokCount;
			if (tokCount >= tokCapacity) {
				tokCapacity *= 2;
				tokens = realloc(tokens, tokCapacity);
			}
			tokens[tokCount - 1] = newTok;
		} else if (cca_is_comment(current)) {
			cca_parse_comment(assembly, &readingPos);
		} else {
			printf("[ERROR] unknown syntax: %c\n", current);
			exit(1);
		}

		++readingPos;
	}

	// shrink marker array
	markers = realloc(markers, markerCount * sizeof(cca_token));

	// shrink tokens array
	tokens = realloc(tokens, (tokCount + 1) * sizeof(cca_token));
	cca_token end;
	end.type = CCA_TOK_END;
	tokens[tokCount] = end;

	// marker replacement
	for (int i = 0; i < markerCount; i++) {
		int j = 0;
		printf(":%s @ %d\n", markers[i].name, markers[i].marks);
		while(tokens[j].type != 6) {
			if (tokens[j].type == 0) {
				if (strcmp(tokens[j].value.string, markers[i].name) == 0) {
					tokens[j].type = 7;
					tokens[j].value.numeric = markers[i].marks;
				}
			}

			++j;
		}
	}
	
	return tokens;
}

cca_definition_list cca_assembler_define_parser(cca_token** tokens) {
	unsigned int tokCapacity = 100;
	unsigned int tokCount = 0;
	unsigned int readingPosition = 0;
	cca_token* newTokens = malloc(tokCapacity * sizeof(cca_token));

	unsigned int definitionCapacity = 100;
	unsigned int definitionLength = 0;
	cca_definition* definitions = malloc(definitionCapacity * sizeof(cca_definition*));

	unsigned int totalHeaderLength = 0;

	while ((*tokens)[readingPosition].type != 6) {
		// check if define
		if ((*tokens)[readingPosition].type == 0 && strcmp((*tokens)[readingPosition].value.string, "def") == 0) {			
			cca_definition def = {
				.name = (*tokens)[readingPosition+1].value.string,
				.value = (*tokens)[readingPosition+2].value.string,
				.pointer = totalHeaderLength
			};

			totalHeaderLength += strlen((*tokens)[readingPosition+2].value.string);

			if (definitionLength >= definitionCapacity) {
				definitionCapacity *= 2;
				cca_definition* definitions = realloc(definitions, definitionCapacity * sizeof(cca_definition*));
			}

			definitions[definitionLength++] = def;
			
			readingPosition += 3;
			continue;
		}

		// add the token
		++tokCount;
		if (tokCount >= tokCapacity) {
			tokCapacity *= 2;
			newTokens = realloc(newTokens, tokCapacity * sizeof(cca_token));
		}

		newTokens[tokCount - 1] = (*tokens)[readingPosition++];
	}

	++tokCount;

	if (tokCount >= tokCapacity) {
		tokCapacity *= 2;
		newTokens = realloc(newTokens, tokCapacity * sizeof(cca_token));
	}

	newTokens[tokCount - 1] = (*tokens)[readingPosition++];

	//cca_token* toBeDestroyed = tokens;
	*tokens = newTokens;
	//free(toBeDestroyed);

	definitions = realloc(definitions, (definitionLength) * sizeof(cca_definition));

	cca_definition_list definitionList = {
		.definitions = definitions,
		.length = definitionLength
	};

	return definitionList;
}

void cca_assembler_replace_defs(cca_token** tokens, cca_definition_list defs) {
	// replace defs
	for (int i = 0; i < defs.length; i++) {
		int j = 0;
		while ((*tokens)[j].type != CCA_TOK_END) {
			if ((*tokens)[j].type == CCA_TOK_IDENTIFIER) {
				(*tokens)[j].type = CCA_TOK_NUMBER;
				(*tokens)[j].value.numeric = defs.definitions[i].pointer;
			}
			++j;
		}
	}
}

typedef struct cca_bytecode {
	char* bytecode;
	unsigned int bytecodeCapacity;
	unsigned int bytecodeLength;
} cca_bytecode;

void cca_bytecode_add_byte(cca_bytecode* bytecode, char byte) {
	bytecode->bytecodeLength += 1;
	
	if (bytecode->bytecodeLength >= bytecode->bytecodeCapacity) {
		bytecode->bytecodeCapacity *= 2;
		bytecode->bytecode = realloc(bytecode->bytecode, bytecode->bytecodeLength);
	}

	bytecode->bytecode[bytecode->bytecodeLength - 1] = byte;
}

void cca_bytecode_add_reg(cca_bytecode* bytecode, char* reg) {
	switch(reg[0]) {
		case 'a': {cca_bytecode_add_byte(bytecode, 0x00);break;}
		case 'b': {cca_bytecode_add_byte(bytecode, 0x01);break;}
		case 'c': {cca_bytecode_add_byte(bytecode, 0x02);break;}
		case 'd': {cca_bytecode_add_byte(bytecode, 0x03);break;}
	}
}

void cca_bytecode_add_uint(cca_bytecode* bytecode, unsigned int n) {
	bytecode->bytecodeLength += 4;
	
	if (bytecode->bytecodeLength >= bytecode->bytecodeCapacity) {
		bytecode->bytecodeCapacity *= 2;
		bytecode->bytecode = realloc(bytecode->bytecode, bytecode->bytecodeLength);
	}

	bytecode->bytecode[bytecode->bytecodeLength - 4] = (n >> 24) & 0xff;
	bytecode->bytecode[bytecode->bytecodeLength - 3] = (n >> 16) & 0xff;
	bytecode->bytecode[bytecode->bytecodeLength - 2] = (n >> 8) & 0xff;
	bytecode->bytecode[bytecode->bytecodeLength - 1] = n & 0xff;
}

char cca_assembler_bytegeneration(cca_token* tokens, cca_definition_list defs) {
	FILE* fp = fopen("test.ccb", "wb+");

	cca_bytecode bytecode;
	bytecode.bytecodeCapacity = 100;
	bytecode.bytecodeLength = 0;
	bytecode.bytecode = malloc(bytecode.bytecodeCapacity);

	// generate bytes of the header
	for (int i = 0; i < defs.length; i++) {
		char* bytes = defs.definitions[i].value;
		for (int j = 0; bytes[j] != '\0'; j++) {
			cca_bytecode_add_byte(&bytecode, bytes[j]);
		}
	}

	cca_bytecode_add_uint(&bytecode, 0x1d1d1d1d);

	char error = 0;
	unsigned int i = 0;
	while(tokens[i].type != 6) {
		if (tokens[i].type != 3) {
			if (tokens[i].type == 1 || tokens[i].type == 7)
				printf("[ERROR] unexpected token: '%d' while generating bytecode\n", tokens[i].value.numeric);
			else
				printf("[ERROR] unexpected token: '%s' while generating bytecode\n", tokens[i].value.string);
			i += 1;
			error = 1;
		} else if (strcmp(tokens[i].value.string, "stp") == 0) {
			if (tokens[i + 1].type == 3 || tokens[i + 1].type == 6) {
				cca_bytecode_add_byte(&bytecode, 0x00);
				i += 1;
			} else {
				puts("[ERROR] on 'stp' instruction, illegal combination of operands");
				error = 1;
				i += 1;
			}
		} else if (strcmp(tokens[i].value.string, "psh") == 0) {
			if (tokens[i + 1].type == 1) {
				cca_bytecode_add_byte(&bytecode, 0x01);
				cca_bytecode_add_uint(&bytecode, tokens[i + 1].value.numeric);
				i += 2;
			} else if (tokens[i + 1].type == 4){
				cca_bytecode_add_byte(&bytecode, 0x02);
				cca_bytecode_add_reg(&bytecode, tokens[i + 1].value.string);
				i += 2;
			} else {
				puts("[ERROR] on 'psh' instruction, illegal combination of operands");
				error = 1;
				i += 1;
			}
		} else if (strcmp(tokens[i].value.string, "pop") == 0) {
			if (tokens[i + 1].type == 4) {
				cca_bytecode_add_byte(&bytecode, 0x03);
				cca_bytecode_add_reg(&bytecode, tokens[i + 1].value.string);
				i += 2;
			} else if (tokens[i + 1].type == 7) {
				cca_bytecode_add_byte(&bytecode, 0x04);
				cca_bytecode_add_uint(&bytecode, tokens[i + 1].value.numeric);
				i += 2;
			} else {
				puts("[ERROR] on 'pop' instruction, illegal combination of operands");
				error = 1;
				i += 1;
			}
		} else if (strcmp(tokens[i].value.string, "dup") == 0) {
			if (tokens[i + 1].type == 3 || tokens[i + 1].type == 6) {
				cca_bytecode_add_byte(&bytecode, 0x05);
				i += 1;
			} else {
				puts("[ERROR] on 'dup' instruction, illegal combination of operands");
				error = 1;
				i += 1;
			}
		} else if (strcmp(tokens[i].value.string, "mov") == 0) {
			if (tokens[i + 1].type == 4 && tokens[i + 2].type == 2 && tokens[i + 3].type == 1) {
				cca_bytecode_add_byte(&bytecode, 0x06);
				cca_bytecode_add_reg(&bytecode, tokens[i + 1].value.string);
				cca_bytecode_add_uint(&bytecode, tokens[i + 3].value.numeric);
				i += 4;
			} else if (tokens[i + 1].type == 7 && tokens[i + 2].type == 2 && tokens[i + 3].type == 1) {
				cca_bytecode_add_byte(&bytecode, 0x07);
				cca_bytecode_add_uint(&bytecode, tokens[i + 1].value.numeric);
				cca_bytecode_add_uint(&bytecode, tokens[i + 3].value.numeric);
				i += 4;
			} else if (tokens[i + 1].type == 4 && tokens[i + 2].type == 2 && tokens[i + 3].type == 7) {
				cca_bytecode_add_byte(&bytecode, 0x08);
				cca_bytecode_add_reg(&bytecode, tokens[i + 1].value.string);
				cca_bytecode_add_uint(&bytecode, tokens[i + 3].value.numeric);
				i += 4;
			} else if (tokens[i + 1].type == 7 && tokens[i + 2].type == 2 && tokens[i + 3].type == 4) {
				cca_bytecode_add_byte(&bytecode, 0x09);
				cca_bytecode_add_uint(&bytecode, tokens[i + 1].value.numeric);
				cca_bytecode_add_reg(&bytecode, tokens[i + 3].value.string);
				i += 4;
			} else {
				puts("[ERROR] on 'mov' instruction, illegal combination of operands");
				error = 1;
				i += 1;
			}
		} else if (strcmp(tokens[i].value.string, "add") == 0) {
			if (tokens[i + 1].type == 4 && tokens[i + 2].type == 2 && tokens[i + 3].type == 4) {
				cca_bytecode_add_byte(&bytecode, 0x10);
				cca_bytecode_add_reg(&bytecode, tokens[i + 1].value.string);
				cca_bytecode_add_reg(&bytecode, tokens[i + 3].value.string);
				i += 4;
			} else if (tokens[i + 1].type == 3 || tokens[i + 1].type == 6) {
				cca_bytecode_add_byte(&bytecode, 0x11);
				i += 1;
			} else {
				puts("[ERROR] on 'add' instruction, illegal combination of operands");
				error = 1;
				i += 1;
			}
		} else if (strcmp(tokens[i].value.string, "sub") == 0) {
			if (tokens[i + 1].type == 4 && tokens[i + 2].type == 2 && tokens[i + 3].type == 4) {
				cca_bytecode_add_byte(&bytecode, 0x12);
				cca_bytecode_add_reg(&bytecode, tokens[i + 1].value.string);
				cca_bytecode_add_reg(&bytecode, tokens[i + 3].value.string);
				i += 4;
			} else if (tokens[i + 1].type == 3 || tokens[i + 1].type == 6) {
				cca_bytecode_add_byte(&bytecode, 0x13);
				i += 1;
			} else {
				puts("[ERROR] on 'sub' instruction, illegal combination of operands");
				error = 1;
				i += 1;
			}
		} else if (strcmp(tokens[i].value.string, "mul") == 0) {
			if (tokens[i + 1].type == 4 && tokens[i + 2].type == 2 && tokens[i + 3].type == 4) {
				cca_bytecode_add_byte(&bytecode, 0x14);
				cca_bytecode_add_reg(&bytecode, tokens[i + 1].value.string);
				cca_bytecode_add_reg(&bytecode, tokens[i + 3].value.string);
				i += 4;
			} else if (tokens[i + 1].type == 3 || tokens[i + 1].type == 6) {
				cca_bytecode_add_byte(&bytecode, 0x15);
				i += 1;
			} else {
				puts("[ERROR] on 'mul' instruction, illegal combination of operands");
				error = 1;
				i += 1;
			}
		} else if (strcmp(tokens[i].value.string, "div") == 0) {
			if (tokens[i + 1].type == 4 && tokens[i + 2].type == 2 && tokens[i + 3].type == 4) {
				cca_bytecode_add_byte(&bytecode, 0x16);
				cca_bytecode_add_reg(&bytecode, tokens[i + 1].value.string);
				cca_bytecode_add_reg(&bytecode, tokens[i + 3].value.string);
				i += 4;
			} else if (tokens[i + 1].type == 3 || tokens[i + 1].type == 6) {
				cca_bytecode_add_byte(&bytecode, 0x17);
				i += 1;
			} else {
				puts("[ERROR] on 'div' instruction, illegal combination of operands");
				error = 1;
				i += 1;
			}
		} else if (strcmp(tokens[i].value.string, "not") == 0) {
			if (tokens[i + 1].type == 4) {
				cca_bytecode_add_byte(&bytecode, 0x18);
				cca_bytecode_add_reg(&bytecode, tokens[i + 1].value.string);
				i += 2;
			} else if (tokens[i + 1].type == 3 || tokens[i + 1].type == 6) {
				cca_bytecode_add_byte(&bytecode, 0x19);
				i += 1;
			} else {
				puts("[ERROR] on 'not' instruction, illegal combination of operands");
				error = 1;
				i += 1;
			}
		} else if (strcmp(tokens[i].value.string, "and") == 0) {
			if (tokens[i + 1].type == 4 && tokens[i + 2].type == 2 && tokens[i + 3].type == 4) {
				cca_bytecode_add_byte(&bytecode, 0x20);
				cca_bytecode_add_reg(&bytecode, tokens[i + 1].value.string);
				cca_bytecode_add_reg(&bytecode, tokens[i + 3].value.string);
				i += 4;
			} else if (tokens[i + 1].type == 3 || tokens[i + 1].type == 6) {
				cca_bytecode_add_byte(&bytecode, 0x21);
				i += 1;
			} else {
				puts("[ERROR] on 'and' instruction, illegal combination of operands");
				error = 1;
				i += 1;
			}
		} else if (strcmp(tokens[i].value.string, "or") == 0) {
			if (tokens[i + 1].type == 4 && tokens[i + 2].type == 2 && tokens[i + 3].type == 4) {
				cca_bytecode_add_byte(&bytecode, 0x22);
				cca_bytecode_add_reg(&bytecode, tokens[i + 1].value.string);
				cca_bytecode_add_reg(&bytecode, tokens[i + 3].value.string);
				i += 4;
			} else if (tokens[i + 1].type == 3 || tokens[i + 1].type == 6) {
				cca_bytecode_add_byte(&bytecode, 0x23);
				i += 1;
			} else {
				puts("[ERROR] on 'or' instruction, illegal combination of operands");
				error = 1;
				i += 1;
			}
		} else if (strcmp(tokens[i].value.string, "xor") == 0) {
			if (tokens[i + 1].type == 4 && tokens[i + 2].type == 2 && tokens[i + 3].type == 4) {
				cca_bytecode_add_byte(&bytecode, 0x24);
				cca_bytecode_add_reg(&bytecode, tokens[i + 1].value.string);
				cca_bytecode_add_reg(&bytecode, tokens[i + 3].value.string);
				i += 4;
			} else if (tokens[i + 1].type == 3 || tokens[i + 1].type == 6) {
				cca_bytecode_add_byte(&bytecode, 0x25);
				i += 1;
			} else {
				puts("[ERROR] on 'xor' instruction, illegal combination of operands");
				error = 1;
				i += 1;
			}
		} else if (strcmp(tokens[i].value.string, "inc") == 0) {
			if (tokens[i + 1].type == 4) {
				cca_bytecode_add_byte(&bytecode, 0x50);
				cca_bytecode_add_reg(&bytecode, tokens[i + 1].value.string);
				i += 2;
			} else if (tokens[i + 1].type == 3 || tokens[i + 1].type == 6) {
				cca_bytecode_add_byte(&bytecode, 0x52);
				i += 1;
			} else {
				puts("[ERROR] on 'inc' instruction, illegal combination of operands");
				error = 1;
				i += 1;
			}
		} else if (strcmp(tokens[i].value.string, "dec") == 0) {
			if (tokens[i + 1].type == 4) {
				cca_bytecode_add_byte(&bytecode, 0x51);
				cca_bytecode_add_reg(&bytecode, tokens[i + 1].value.string);
				i += 2;
			} else if (tokens[i + 1].type == 3 || tokens[i + 1].type == 6) {
				cca_bytecode_add_byte(&bytecode, 0x53);
				i += 1;
			} else {
				puts("[ERROR] on 'dec' instruction, illegal combination of operands");
				error = 1;
				i += 1;
			}
		} else if (strcmp(tokens[i].value.string, "syscall") == 0) {
			if (tokens[i + 1].type == 3 || tokens[i + 1].type == 6) {
				cca_bytecode_add_byte(&bytecode, 0xff);
			} else {
				error = 1;
				puts("[ERROR] on 'syscall' instuction, illegal combination of operands");
			}

			i += 1;
		} else if (strcmp(tokens[i].value.string, "frs") == 0) {
			if (tokens[i + 1].type == 3 || tokens[i + 1].type == 6) {
				cca_bytecode_add_byte(&bytecode, 0x40);
			} else {
				error = 1;
				puts("[ERROR] on 'syscall' instuction, illegal combination of operands");
			}

			i += 1;
		} else if (strcmp(tokens[i].value.string, "call") == 0) {
			if (tokens[i + 1].type == 7) {
				cca_bytecode_add_byte(&bytecode, 0x60);
				cca_bytecode_add_uint(&bytecode, tokens[i + 1].value.numeric);
			} else {
				error = 1;
				puts("[ERROR] on 'call' instuction, illegal combination of operands");
			}
			i += 2;
		} else if (strcmp(tokens[i].value.string, "ret") == 0) {
			if (tokens[i + 1].type == 3 || tokens[i + 1].type == 6) {
				cca_bytecode_add_byte(&bytecode, 0x61);
			} else {
				error = 1;
				puts("[ERROR] on 'ret' instuction, illegal combination of operands");
			}
			i += 1;
		} else if (strcmp(tokens[i].value.string, "cmp") == 0) {
			if (tokens[i + 1].type == 4 && tokens[i + 2].type == 2 && tokens[i + 3].type == 4) {
				cca_bytecode_add_byte(&bytecode, 0x30);
				cca_bytecode_add_reg(&bytecode, tokens[i + 1].value.string);
				cca_bytecode_add_reg(&bytecode, tokens[i + 3].value.string);
				i += 4;
			} else if (tokens[i + 1].type == 4 && tokens[i + 2].type == 2 && tokens[i + 3].type == 1) {
				cca_bytecode_add_byte(&bytecode, 0x31);
				cca_bytecode_add_reg(&bytecode, tokens[i + 1].value.string);
				cca_bytecode_add_uint(&bytecode, tokens[i + 3].value.numeric);
				i += 4;
			} else if (tokens[i + 1].type == 1 && (tokens[i + 2].type == 3 || tokens[i + 2].type == 6)) {
				cca_bytecode_add_byte(&bytecode, 0x32);
				cca_bytecode_add_uint(&bytecode, tokens[i + 1].value.numeric);
				i += 2;
			} else {
				puts("[ERROR] on 'cmp' instuction, illegal combination of operands");
				error = 1;
				i += 1;
			}
		} else if (strcmp(tokens[i].value.string, "jmp") == 0) {
			if (tokens[i + 1].type == 7) {
				cca_bytecode_add_byte(&bytecode, 0x20);
				cca_bytecode_add_uint(&bytecode, tokens[i + 1].value.numeric);
				i += 2;
			} else {
				puts("[ERROR] on 'jmp' instuction, illegal combination of operands");
				error = 1;
				i += 1;
			}
		} else if (strcmp(tokens[i].value.string, "je") == 0) {
			if (tokens[i + 1].type == 7) {
				cca_bytecode_add_byte(&bytecode, 0x33);
				cca_bytecode_add_uint(&bytecode, tokens[i + 1].value.numeric);
				i += 2;
			} else {
				puts("[ERROR] on 'je' instuction, illegal combination of operands");
				error = 1;
				i += 1;
			}
		} else if (strcmp(tokens[i].value.string, "jne") == 0) {
			if (tokens[i + 1].type == 7) {
				cca_bytecode_add_byte(&bytecode, 0x34);
				cca_bytecode_add_uint(&bytecode, tokens[i + 1].value.numeric);
				i += 2;
			} else {
				puts("[ERROR] on 'jne' instuction, illegal combination of operands");
				error = 1;
				i += 1;
			}
		} else if (strcmp(tokens[i].value.string, "jg") == 0) {
			if (tokens[i + 1].type == 7) {
				cca_bytecode_add_byte(&bytecode, 0x35);
				cca_bytecode_add_uint(&bytecode, tokens[i + 1].value.numeric);
				i += 2;
			} else {
				puts("[ERROR] on 'jg' instuction, illegal combination of operands");
				error = 1;
				i += 1;
			}
		} else if (strcmp(tokens[i].value.string, "js") == 0) {
			if (tokens[i + 1].type == 7) {
				cca_bytecode_add_byte(&bytecode, 0x36);
				cca_bytecode_add_uint(&bytecode, tokens[i + 1].value.numeric);
				i += 2;
			} else {
				puts("[ERROR] on 'js' instuction, illegal combination of operands");
				error = 1;
				i += 1;
			}
		} else if (strcmp(tokens[i].value.string, "jo") == 0) {
			if (tokens[i + 1].type == 7) {
				cca_bytecode_add_byte(&bytecode, 0x37);
				cca_bytecode_add_uint(&bytecode, tokens[i + 1].value.numeric);
				i += 2;
			} else {
				puts("[ERROR] on 'jo' instuction, illegal combination of operands");
				error = 1;
				i += 1;
			}
		} else {
			printf("[ERROR] unknown opcode '%s'\n", tokens[i].value.string);
			exit(1);
		}
	}

	if (!error) {
		fwrite(bytecode.bytecode, 1, bytecode.bytecodeLength, fp);
	}

	fclose(fp);
	return error;
}

char cca_assemble(char* fileName) {
	// optain the assembly code
	cca_file_content content = ccvm_program_load(fileName);
	
	// lex the assembly code into tokens
	cca_token* tokens = cca_assembler_lex(content);

	// parse the defines and get rid of them in the tokens
	cca_definition_list defs = cca_assembler_define_parser(&tokens);

	// replace all the unknown identifiers with their corresponding define pointer
	cca_assembler_replace_defs(&tokens, defs);

	int i = 0;
	while (tokens[i++].type != 6) {
		cca_token_print(tokens[i]);
	}

	// generate bytecode
	if (cca_assembler_bytegeneration(tokens, defs)) {
		free(tokens);
		free(content.content);
		return 0;
	}

	free(tokens);
	free(content.content);
	return 1;
}

#endif