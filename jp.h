#ifndef JP_H
#define JP_H

#define JP_ERROR_NONE 0				// No error.
#define JP_ERROR_SYNTAX 1			// Invalid JSON found.
#define JP_ERROR_KEY 2				// The key was not found.
#define JP_ERROR_INDEX 3			// The index was not found.
#define JP_ERROR_ESCAPE 4			// Unknown escape sequence.
#define JP_ERROR_LIST 5				// A list was found where it was not expected (e.g. if jp_key was used instead of jp_index).
#define JP_ERROR_NUMBER 6			// Invalid number character.
#define JP_ERROR_OVERFLOW 7			// The number is too large to fit in the desired data type.

// Export for C++ projects.
#ifdef __cplusplus
	extern "C" {
#endif

int jp_error(char const * const json, char * value_buffer, int value_size);

char const * jp_value(char const * json, char const * key);
char const * jp_index(char const * json, int index);

int jp_keys(char const * json, char const ** keys, int keys_size);

int jp_char(char const * json, char * value_buffer, int value_size);
int jp_int(char const * json, int * value);
int jp_uint(char const * json, unsigned int * value);


#ifdef __cplusplus
	}
#endif

#endif	// ifndef JP_H

// Add error variable into every file.
#ifndef JP_C
	extern int jp_error_code;
#endif
