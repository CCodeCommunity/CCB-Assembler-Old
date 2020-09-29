#include <stdio.h>
#include <string.h>
#include "assembler.h"

int main(int argc, char* argv[]) {
	if (argc > 1) {
		if (argc > 2 && (strcmp(argv[2], "-w") == 0 || strcmp(argv[2], "--watch") == 0)) {
			// watching
			printf("watching %s...\n", argv[1]);
		} else {
			// assembling
			printf("assembling '%s'\n", argv[1]);
			cca_assemble(argv[1]);
			puts("done!");
		}
	}

	return 1;
}