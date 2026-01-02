// This program tests the functionality of the JSON Parse library.
// Please compile it with jp.c e.g. `gcc test.c jp.c && ./a.out`

#include "jp.h"
#include <stdio.h>

void print_error(char const * const json);

int main(void)
{
	// Load the example json.
	FILE * file = fopen("test.json", "r");
	
	// Get file size.
	fseek(file, 0L, SEEK_END);
	int json_size = ftell(file);
	
	// Read file.
	char json[json_size + 1];
	fseek(file, 0L, SEEK_SET);
	fread(json, sizeof(char), json_size, file);
	json[json_size] = '\0';
	fclose(file);

	// Parse the JSON.
	char buffer[64];
	int size;
	char const * p;
	

	size = jp_char(jp_value(jp_value(json, "author"), "name"), buffer, 64);
	if (size == -1) { print_error(json); return 1; }
	printf("The author's name is: %s\n", buffer);
	
	size = jp_char(jp_value(jp_value(json, "compact"), "value"), buffer, 64);
	if (size == -1) { print_error(json); return 1; }
	printf("Compact json: %s\n", buffer);
	
	printf("\nThe author has the following items:\n");
	char const * items = jp_value(jp_value(json, "author"), "items");
	int i = 0;
	
	while(1)
	{
	  char const * item = jp_index(items, i);
	  
	  if (item == 0)
	  {
	    break;
	  }
	  
	  jp_char(item, buffer, 64);
	  printf("\tItem %d: %s\n", i, buffer);
	  
	  i++;
	}
	
	return 0;
}

void print_error(char const * const json)
{
	char error[128];
	jp_error(json, error, 128);
	printf("%s\n", error);
}
