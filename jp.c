#define JP_C						// Stops the extern jp_error_code from being created.
#include "jp.h"

int jp_error_code = JP_ERROR_NONE;	// Error code from the last failed function.
char const * jp_error_json = 0;		// Pointer to the position in the JSON string which caused the last error.

// Returns a pointer to the char in [json] where the value starts for [key].
// Returns 0 on failure.
// Returns a pointer after the end of the JSON object if key is set to 0.
char const * jp_value(char const * json, char const * key)
{
	if (json == 0) { return 0; }

	#define STATE_START 0		// { 'key' : 'value' }
	#define STATE_KEY_L 1		//  'key' : 'value' }
	#define STATE_KEY_I 2		// key' : 'value' }
	#define STATE_KEY_R 3		//  : 'value' }
	#define STATE_VAL_L 4		//  'value' }
	#define STATE_VAL_I 5		// value' }
	#define STATE_VAL_R 6		//  }

	int state = STATE_START;	// State of the process (see above).
	int key_index = -1;			// Index of [key] which is currently being matched. If -1, key did not match.
	char quote = '\0';			// Quote used during keys / strings.
	int escape = 0;				// If the last char was a \.

	// Run through the JSON string.
	// The json pointer is incremented until it points to '\0'.
	while (*json != '\0')
	{
		if (state == STATE_START)
		{
			if (*json == '{')
			{
				// Start of an object.
				state = STATE_KEY_L;
			}
			else if (*json == '[')
			{
				// Start of a list.
				// This function doesn't read lists, so raise an error.
				jp_error_code = JP_ERROR_LIST;
				jp_error_json = json;
				return 0;
			}
			else if (*json == ' ' || *json == '\n' || *json == '\t' || *json == '\r')
			{
				// Ignore blank space.
			}
			else
			{
				// Unexpected character.
				jp_error_code= JP_ERROR_SYNTAX;
				jp_error_json = json;
				return 0;
			}
		}
		else if (state == STATE_KEY_L)
		{
			if (*json == '\'' || *json == '"')
			{
				// Start of key string.
				quote = *json;
				state = STATE_KEY_I;
				key_index = (key == 0) ? -1 : 0;	// Set key_index to -1 if no key is provided.
			}
			else if (*json == ' ' || *json == '\n' || *json == '\t' || *json == '\r')
			{
				// Ignore blank space.
			}
			else
			{
				// Unexpected character.
				jp_error_code= JP_ERROR_SYNTAX;
				jp_error_json = json;
				return 0;
			}
		}
		else if (state == STATE_KEY_I)
		{
			if (*json == quote)
			{
				// End of key.
				state = STATE_KEY_R;
			}
			else if (key_index != -1)
			{
				if (*json != key[key_index])
				{
					// This key does not match.
					key_index = -1;
				}
				else
				{
					// Check for the next key character.
					key_index++;
				}
			}
			else
			{
				// Do nothing until the end of the key is found.
			}
		}
		else if (state == STATE_KEY_R)
		{
			if (*json == ':')
			{
				state = STATE_VAL_L;
			}
			else if (*json == ' ' || *json == '\n' || *json == '\t' || *json == '\r')
			{
				// Ignore blank space.
			}
			else
			{
				// Unexpected character found.
				jp_error_code= JP_ERROR_SYNTAX;
				jp_error_json = json;
				return 0;
			}
		}
		else if (state == STATE_VAL_L)
		{
			if (*json == '\'' || *json == '"')
			{
				// String value found!.
				if (key_index != -1)
				{
					// This is the value we are looking for!
					return json;
				}

				// Continue searching.
				quote = *json;
				escape = 0;
				state = STATE_VAL_I;
			}
			else if ((*json >= 48 && *json <= 57) || *json == '-')
			{
				// Numeric value found!
				if (key_index != -1)
				{
					// This is the value we are looking for!
					return json;
				}

				// Continue searching.
				quote = '\0';
				escape = 0;
				state = STATE_VAL_I;
			}
			else if (*json == 't' || *json == 'f' || *json == 'n')
			{
				// true, false, or null.
				// Messy, but fast...
				int found = 0;
				if (json[0] == 't' && json[1] == 'r' && json[2] == 'u' && json[3] == 'e') { found = 1; }
				else if (json[0] == 'f' && json[1] == 'a' && json[2] == 'l' && json[3] == 's' && json[4] == 'e') { found = 1; }
				else if (json[0] == 'n' && json[1] == 'u' && json[2] == 'l' && json[3] == 'l') { found = 1; }

				if (found == 0)
				{
					// The value was not true, false, or null.
					jp_error_code= JP_ERROR_SYNTAX;
					jp_error_json = json;
					return 0;
				}

				if (key_index != -1)
				{
					// This is the value we are looking for!
					return json;
				}

				// Keep searching.
				quote = '\0';
				escape = 0;
				state = STATE_VAL_I;
			}
			else if (*json == '{')
			{
				// Object found!
				if (key_index != -1)
				{
					// This is the object we are looking for!
					return json;
				}
				
				// Skip over this object.
				json = jp_value(json, 0);
				
				if (json == 0)
				{
					// jp_key should have already set the error code in this case.
					return 0;
				}
				
				state = STATE_VAL_R;
				continue;	// Skip incremitation this round as json is already set.
			}
			else if (*json == '[')
			{
				// List found!
				if (key_index != -1)
				{
					// This is the list we are looking for!
					return json;
				}
				
				// Skip over this list.
				json = jp_index(json, -1);
				
				if (json == 0)
				{
					// jp_index should have already set the error code in this case.
					return 0;
				}
				
				state = STATE_VAL_R;
				continue;	// Skip incremitation this round as json is already set.
			}
			else if (*json == ' ' || *json == '\n' || *json == '\t' || *json == '\r')
			{
				// Ignore blank spaces.
			}
			else
			{
				// Unexpected character.
				jp_error_code = JP_ERROR_SYNTAX;
				jp_error_json = json;
				return 0;
			}
		}
		else if (state == STATE_VAL_I)
		{
			if (*json == quote && quote != '\0' && escape == 0)
			{
				// End of string value.
				state = STATE_VAL_R;
			}
			else if ((*json == ' ' || *json == '\n' || *json == '\t' || *json == '\r') && quote == '\0')
			{
				// End of numeric value.
				state = STATE_VAL_R;
			}
			else if (*json == ',' && quote == '\0')
			{
				// End of numeric value with nothing after.
				state = STATE_KEY_L;
			}
			else if (*json == '}' && quote == '\0')
			{
				// End of object.
				if (key == 0)
				{
					// If key was not given, the end of the JSON object is returned.
					return json + 1;
				}
				
				// Return NULL as the key was not found.
				jp_error_code = JP_ERROR_KEY;
				jp_error_json = json;
				return 0;
			}
			else if (*json == '\\' && escape == 0 && quote != '\0')
			{
				escape = 1;
			}
			else
			{
				escape = 0;
			}
		}
		else if (state == STATE_VAL_R)
		{
			if (*json == ',')
			{
				// Next value.
				state = STATE_KEY_L;
			}
			else if (*json == '}')
			{
				// End of object.
				if (key == 0)
				{
					// If key was not given, the end of the JSON object is returned.
					return json + 1;
				}
				
				// Return NULL as the key was not found.
				jp_error_code = JP_ERROR_KEY;
				jp_error_json = json;
				return 0;
			}
			else if (*json == ' ' || *json == '\n' || *json == '\t' || *json == '\r')
			{
				// Ignore blank space.
			}
			else
			{
				jp_error_code = JP_ERROR_SYNTAX;
				jp_error_json = json;
				return 0;
			}
		}

		json++;
	}

	// The JSON string ended without closing properly.
	jp_error_code= JP_ERROR_SYNTAX;
	jp_error_json = json;
	return 0;
}

// Return a pointer within [json] at which the value at [index] in the list starts.
// Returns the end of the list if index is -1;
// Returns 0 on failure.
// This function only works for lists.
char const * jp_index(char const * json, int index)
{
	if (json == 0) { return 0; }

	#define STATE_LSTART 0		// [ 'value' , 'value' ]
	#define STATE_LVAL_L 1		//  'value' , 'value' ]
	#define STATE_LVAL_I 2		// value' , 'value' ]
	#define STATE_LVAL_R 3		//  , 'value' ]

	int state = STATE_LSTART;	// State of the process (see above).
	char quote = '\0';			// Quote used during keys / strings.
	int escape = 0;				// If the last char was a \.

	// Run through the JSON string.
	// The json pointer is incremented until it points to '\0'.
	while (*json != '\0')
	{
		if (state == STATE_LSTART)
		{
			if (*json == '[')
			{
				// Start of the list.
				state = STATE_LVAL_L;
			}
			else if (*json == ' ' || *json == '\n' || *json == '\t' || *json == '\r')
			{
				// Ignore blank space.
			}
			else
			{
				// Unexpected character.
				jp_error_code = JP_ERROR_SYNTAX;
				jp_error_json = json;
				return 0;
			}
		}
		else if (state == STATE_LVAL_L)
		{
			if (*json == '\'' || *json == '"')
			{
				// String value found!.
				if (index == 0)
				{
					// This is the value we are looking for!
					return json;
				}

				// Shift index if able.
				if (index > 0) { index--; }

				// Continue searching.
				quote = *json;
				escape = 0;
				state = STATE_LVAL_I;
			}
			else if ((*json >= 48 && *json <= 57) || *json == '-')
			{
				// Numeric value found!
				if (index == 0)
				{
					// This is the value we are looking for!
					return json;
				}
				
				// Shift index if able.
				if (index > 0) { index--; }

				// Continue searching.
				quote = '\0';
				escape = 0;
				state = STATE_LVAL_I;
			}
			else if (*json == 't' || *json == 'f' || *json == 'n')
			{
				// true, false, or null.
				// Messy, but fast...
				int found = 0;
				if (json[0] == 't' && json[1] == 'r' && json[2] == 'u' && json[3] == 'e') { found = 1; }
				else if (json[0] == 'f' && json[1] == 'a' && json[2] == 'l' && json[3] == 's' && json[4] == 'e') { found = 1; }
				else if (json[0] == 'n' && json[1] == 'u' && json[2] == 'l' && json[3] == 'l') { found = 1; }

				if (found == 0)
				{
					// The value was not true, false, or null.
					jp_error_code = JP_ERROR_SYNTAX;
					jp_error_json = json;
					return 0;
				}

				if (index == 0)
				{
					// This is the value we are looking for!
					return json;
				}
				
				// Shift index if able.
				if (index > 0) { index--; }

				// Keep searching.
				quote = '\0';
				escape = 0;
				state = STATE_LVAL_I;
			}
			else if (*json == '{')
			{
				// Object found!
				if (index == 0)
				{
					// This is the object we are looking for!
					return json;
				}
				
				// Shift index if able.
				if (index > 0) { index--; }
				
				// Skip over this object.
				json = jp_value(json, 0);
				
				if (json == 0)
				{
					// jp_key should have already set the error code in this case.
					return 0;
				}
				
				state = STATE_LVAL_R;
				continue;	// Skip incremitation this round as json is already set.
			}
			else if (*json == '[')
			{
				// List found!
				if (index == 0)
				{
					// This is the list we are looking for!
					return json;
				}
				
				// Shift index if able.
				if (index > 0) { index--; }
				
				// Skip over this list.
				json = jp_index(json, -1);
				
				if (json == 0)
				{
					// jp_index should have already set the error code in this case.
					return 0;
				}
				
				state = STATE_LVAL_R;
				continue;	// Skip incremitation this round as json is already set.
			}
			else if (*json == ']')
			{
				// End of list.
				if (index == -1)
				{
					// Return the end of the list.
					return json + 1;
				}
				
				// index not found.
				jp_error_code = JP_ERROR_INDEX;
				jp_error_json = json;
				return 0;
			}
			else if (*json == ' ' || *json == '\n' || *json == '\t' || *json == '\r')
			{
				// Ignore blank spaces.
			}
			else
			{
				// Unexpected character.
				jp_error_code= JP_ERROR_SYNTAX;
				jp_error_json = json;
				return 0;
			}
		}
		else if (state == STATE_LVAL_I)
		{
			if (*json == quote && quote != '\0' && escape == 0)
			{
				// End of string value.
				state = STATE_LVAL_R;
			}
			else if ((*json == ' ' || *json == '\n' || *json == '\t' || *json == '\r') && quote == '\0')
			{
				// End of numeric value.
				state = STATE_LVAL_R;
			}
			else if (*json == ',' && quote == '\0')
			{
				// End of numeric value with nothing after.
				state = STATE_LVAL_L;
			}
			else if (*json == ']' && quote == '\0')
			{
				// End of list.
				if (index == -1)
				{
					// If key was not given, the end of the JSON object is returned.
					return json + 1;
				}
				
				// Return NULL as the index was not found.
				jp_error_code= JP_ERROR_INDEX;
				jp_error_json = json;
				return 0;
			}
			else if (*json == '\\' && escape == 0 && quote != '\0')
			{
				escape = 1;
			}
			else
			{
				escape = 0;
			}
		}
		else if (state == STATE_LVAL_R)
		{
			if (*json == ',')
			{
				// Next value.
				state = STATE_LVAL_L;
			}
			else if (*json == ']')
			{
				// End of list.
				if (index == -1)
				{
					// If index was not given, the end of the JSON list is returned.
					return json + 1;
				}
				
				// Return NULL as the index was not found.
				jp_error_code = JP_ERROR_KEY;
				jp_error_json = json;
				return 0;
			}
			else if (*json == ' ' || *json == '\n' || *json == '\t' || *json == '\r')
			{
				// Ignore blank space.
			}
			else
			{
				jp_error_code= JP_ERROR_SYNTAX;
				jp_error_json = json;
				return 0;
			}
		}
		
		json++;
	}
	
	// The listed did not end before the end of the string.
	jp_error_code = JP_ERROR_SYNTAX;
	jp_error_json = json - 1;
	return 0;
}


// Populates the [keys] array of maximum length [keys_size] with pointers to
// [json] where the key names start.
// Returns the number of keys found on success and -1 on failure.
int jp_keys(char const * json, char const ** keys, int keys_size)
{
	if (json == 0) { return -1; }

	#define STATE_START 0		// { 'key' : 'value' }
	#define STATE_KEY_L 1		//  'key' : 'value' }
	#define STATE_KEY_I 2		// key' : 'value' }
	#define STATE_KEY_R 3		//  : 'value' }
	#define STATE_VAL_L 4		//  'value' }
	#define STATE_VAL_I 5		// value' }
	#define STATE_VAL_R 6		//  }

	int state = STATE_START;	// State of the process (see above).
	char quote = '\0';			// Quote used during keys / strings.
	int escape = 0;				// If the last char was a \.
	int key_count = 0;			// Count of how many keys were found in the JSON object.

	// Run through the JSON string.
	// The json pointer is incremented until it points to '\0'.
	while (*json != '\0')
	{
		if (state == STATE_START)
		{
			if (*json == '{')
			{
				// Start of an object.
				state = STATE_KEY_L;
			}
			else if (*json == '[')
			{
				// Start of a list.
				// This function doesn't read lists, so raise an error.
				jp_error_code = JP_ERROR_LIST;
				jp_error_json = json;
				return -1;
			}
			else if (*json == ' ' || *json == '\n' || *json == '\t' || *json == '\r')
			{
				// Ignore blank space.
			}
			else
			{
				// Unexpected character.
				jp_error_code= JP_ERROR_SYNTAX;
				jp_error_json = json;
				return -1;
			}
		}
		else if (state == STATE_KEY_L)
		{
			if (*json == '\'' || *json == '"')
			{
				// Start of key string.
				quote = *json;
				state = STATE_KEY_I;

				// Record the key if space.
				if (key_count < keys_size)
				{
					keys[key_count] = json;
				}

				key_count++;
			}
			else if (*json == ' ' || *json == '\n' || *json == '\t' || *json == '\r')
			{
				// Ignore blank space.
			}
			else
			{
				// Unexpected character.
				jp_error_code= JP_ERROR_SYNTAX;
				jp_error_json = json;
				return -1;
			}
		}
		else if (state == STATE_KEY_I)
		{
			if (*json == quote)
			{
				// End of key.
				state = STATE_KEY_R;
			}
			else
			{
				// Do nothing until the end of the key is found.
			}
		}
		else if (state == STATE_KEY_R)
		{
			if (*json == ':')
			{
				state = STATE_VAL_L;
			}
			else if (*json == ' ' || *json == '\n' || *json == '\t' || *json == '\r')
			{
				// Ignore blank space.
			}
			else
			{
				// Unexpected character found.
				jp_error_code= JP_ERROR_SYNTAX;
				jp_error_json = json;
				return -1;
			}
		}
		else if (state == STATE_VAL_L)
		{
			if (*json == '\'' || *json == '"')
			{
				// String value found.
				quote = *json;
				escape = 0;
				state = STATE_VAL_I;
			}
			else if ((*json >= 48 && *json <= 57) || *json == '-')
			{
				// Numeric value found.
				quote = '\0';
				escape = 0;
				state = STATE_VAL_I;
			}
			else if (*json == 't' || *json == 'f' || *json == 'n')
			{
				// true, false, or null.
				// Messy, but fast...
				int found = 0;
				if (json[0] == 't' && json[1] == 'r' && json[2] == 'u' && json[3] == 'e') { found = 1; }
				else if (json[0] == 'f' && json[1] == 'a' && json[2] == 'l' && json[3] == 's' && json[4] == 'e') { found = 1; }
				else if (json[0] == 'n' && json[1] == 'u' && json[2] == 'l' && json[3] == 'l') { found = 1; }

				if (found == 0)
				{
					// The value was not true, false, or null.
					jp_error_code= JP_ERROR_SYNTAX;
					jp_error_json = json;
					return -1;
				}

				quote = '\0';
				escape = 0;
				state = STATE_VAL_I;
			}
			else if (*json == '{')
			{
				// Object found.
				// Skip over.
				json = jp_value(json, 0);

				if (json == 0)
				{
					// jp_value should have already set the error code in this case.
					return -1;
				}

				state = STATE_VAL_R;
				continue;	// Skip incremitation this round as json is already set.
			}
			else if (*json == '[')
			{
				// List found.
				// Skip over.
				json = jp_index(json, -1);

				if (json == 0)
				{
					// jp_value should have already set the error code in this case.
					return -1;
				}

				state = STATE_VAL_R;
				continue;	// Skip incremitation this round as json is already set.
			}
			else if (*json == ' ' || *json == '\n' || *json == '\t' || *json == '\r')
			{
				// Ignore blank spaces.
			}
			else
			{
				// Unexpected character.
				jp_error_code = JP_ERROR_SYNTAX;
				jp_error_json = json;
				return -1;
			}
		}
		else if (state == STATE_VAL_I)
		{
			if (*json == quote && quote != '\0' && escape == 0)
			{
				// End of string value.
				state = STATE_VAL_R;
			}
			else if ((*json == ' ' || *json == '\n' || *json == '\t' || *json == '\r') && quote == '\0')
			{
				// End of numeric value.
				state = STATE_VAL_R;
			}
			else if (*json == ',' && quote == '\0')
			{
				// End of numeric value with nothing after.
				state = STATE_KEY_L;
			}
			else if (*json == '}' && quote == '\0')
			{
				// End of object.
				// Return number of keys found.
				return key_count;
			}
			else if (*json == '\\' && escape == 0 && quote != '\0')
			{
				escape = 1;
			}
			else
			{
				escape = 0;
			}
		}
		else if (state == STATE_VAL_R)
		{
			if (*json == ',')
			{
				// Next value.
				state = STATE_KEY_L;
			}
			else if (*json == '}')
			{
				// End of object.
				// Return number of keys found.
				return key_count;
			}
			else if (*json == ' ' || *json == '\n' || *json == '\t' || *json == '\r')
			{
				// Ignore blank space.
			}
			else
			{
				jp_error_code = JP_ERROR_SYNTAX;
				jp_error_json = json;
				return -1;
			}
		}

		json++;
	}

	// The JSON string ended without closing properly.
	jp_error_code= JP_ERROR_SYNTAX;
	jp_error_json = json;
	return -1;
}





// Store the value in the JSON string starting at [json] into [value_buffer] of maximum size [value_size].
// [json] should come from the return value of jp_value() or jq_index().
// Returns the length of the value (excluding null terminator) on success, and -1 on failure.
// If [value_size] is 0, the function will still return the length of the string.
// Numbers, null, true, and false are set as "null", "true", etc...
int jp_char(char const * json, char * value_buffer, int value_size)
{
	if (json == 0)
	{
		// Some previous step failed.
		// jp_key and jp_index should have set the error code.
		return -1;
	}
	
	char quote = '\0';	// '\0' means it is a number, null, true or false.
	int escape = 0;
	int size = 0;
	
	// The first character should be the start of a string or the value itself.
	if (*json == '\'' || *json == '"')
	{
		quote = *json;
		json++;
	}
	
	// Copy the value.
	while (*json != '\0')
	{
		if (*json == quote && quote != '\0' && escape == 0)
		{
			// End of string.
			break;
		}
		else if ((*json == ',' || *json == ' ' || *json == '}' || *json == ']') && quote == '\0')
		{
			// End of number, null, or boolean.
			break;
		}
		else if (*json == '\\' && quote != '\0' && escape == 0)
		{
			// Escape enabled.
			escape = 1;
		}
		else if (escape == 1)
		{
			escape = 0;
			
			// Translate escape codes.
			char c = '?';
			
			switch (*json)
			{
				case 'n': c = '\n'; break;
				case 'r': c = '\r'; break;
				case 't': c = '\t'; break;
				case '\'': c = '\''; break;
				case '"': c = '"'; break;
				
				default:
					jp_error_code = JP_ERROR_ESCAPE;
					jp_error_json = json;
					return -1;
			}
			
			// Add to value.
			if (size < value_size - 1)	// -1 to allow space for the null terminator.
			{
				value_buffer[size] = c;
			}
			
			size++;
		}
		else
		{
			// Add normal character.
			if (size < value_size - 1)	// -1 to allow space for the null terminator.
			{
				value_buffer[size] = *json;
			}
			
			size++;
		}
		
		json++;
	}
	
	
	// Terminate value if able.
	if (size < value_size)
	{
		value_buffer[size] = '\0';
	}
	else if (value_size > 0)
	{
		value_buffer[value_size - 1] = '\0';
	}
	
	return size;
}

// Store the signed integer value in the JSON string starting at [json] into [value].
// [json] should come from the return value of jq_value() or jq_index().
// Returns 0 on success, -1 on failure (either due to syntax or the number exceeded the range -2147483648 to 2147483647).
int jp_int(char const * json, int * value)
{
	if (json == 0)
	{
		return -1;
	}

	*value = 0;

	// Check sign.
	int sign = 1;

	if (*json == '-')
	{
		sign = -1;
		json++;
	}

	// Loop through digits.
	while (*json != '\0')
	{
		if (*json == ',' || *json == ' ' || *json == '}' || *json == ']')
		{
			// End of number, null, or boolean.
			break;
		}
		else if (*json >= '0' && *json <= '9')
		{
			int last = *value;
			*value = *value * 10;
			*value += (*json - '0');

			// Check for overflow.
			if ((*value - (*json - '0')) / 10 != last)
			{
				jp_error_code = JP_ERROR_OVERFLOW;
				jp_error_json = json;
				return -1;
			}
		}
		else
		{
			// Invalid character in number.
			jp_error_code = JP_ERROR_NUMBER;
			jp_error_json = json;
			return -1;
		}

		json++;
	}

	// Add sign.
	*value = *value * sign;

	return 0;
}

// Store the unsigned integer value in the JSON string starting at [json] into [value].
// [json] should come from the return value of jq_key() or jq_index().
// Returns 0 on success, -1 on failure (either due to syntax or the number exceeded the range 0 to 4294967295).
int jp_uint(char const * json, unsigned int * value)
{
	if (json == 0)
	{
		return -1;
	}

	*value = 0;

	// Loop through digits.
	while (*json != '\0')
	{
		if (*json == ',' || *json == ' ' || *json == '}' || *json == ']')
		{
			// End of number, null, or boolean.
			break;
		}
		else if (*json >= '0' && *json <= '9')
		{
			unsigned int last = *value;
			*value = *value * 10;
			*value += (*json - '0');

			// Check for overflow.
			if ((*value - (*json - '0')) / 10 != last)
			{
				jp_error_code = JP_ERROR_OVERFLOW;
				jp_error_json = json;
				return -1;
			}
		}
		else
		{
			// Invalid character in number.
			jp_error_code = JP_ERROR_NUMBER;
			jp_error_json = json;
			return -1;
		}

		json++;
	}

	return 0;
}

// Generate an error message using the [json] string which was the subject of the error.
// Returns the length of the error message, this may be larger than size provided, but size will not overflow.
int jp_error(char const * const json, char * value_buffer, int value_size)
{
	int size = 0;
	

	// Decode error code.
	char const * name = "No error\n";
	
	switch (jp_error_code)
	{
		case JP_ERROR_SYNTAX:	name = "Invalid JSON\n";				break;
		case JP_ERROR_KEY:	name = "Key not found\n";				break;
		case JP_ERROR_INDEX:	name = "Index not found\n";				break;
		case JP_ERROR_ESCAPE:	name = "Invalid escape code\n";				break;
		case JP_ERROR_LIST:	name = "List not expected here\n";			break;
		case JP_ERROR_NUMBER:	name = "Invalid character in integer\n";		break;
		case JP_ERROR_OVERFLOW:	name = "Value does not fit in desired data type\n";	break;
	}
	
	while (size < value_size - 1 && *name != '\0')
	{
		value_buffer[size] = *name;
		name++;
		size++;
	}
	
	// Print a snippet of the JSON if possible.
	if (jp_error_json != 0)
	{
		int const window = 20;
		int a = 0;
		
		for (a = 0; a < window; a++)
		{
			if (jp_error_json == json)
			{
				// Found start of JSON string.
				break;
			}	
			
			if (*jp_error_json == '\0')
			{
				// Something went wrong!
				jp_error_json++;
				break;
			}
			
			jp_error_json--;
		}
		
		// Walk forward and print.
		for (int b = 0; b < (window * 2) + 1; b++)
		{
			char c = ' ';
			
			if (c >= window - a && *jp_error_json != '\0')
			{
				c = *jp_error_json;
				jp_error_json++;
			}
			
			// Remove styling.
			if (c == '\n' || c == '\t' || c == '\r') { c = ' '; }
			
			// Add c to error message.
			if (size < value_size - 1)
			{
				value_buffer[size] = c;
				size++;
			}
		}
		
		// Add arrow.
		if (size < value_size - 1)
		{
			value_buffer[size] = '\n';
			size++;
		}
			
		for (int c = 0; c < a; c++)
		{
			if (size < value_size - 1)
			{
				value_buffer[size] = ' ';
				size++;
			}
		}
		
		if (size < value_size - 1)
		{
			value_buffer[size] = '^';
			size++;
		}
	}
	
	// Terminate.
	if (size < value_size - 1)
	{
		value_buffer[size] = '\0';
	}
	else if (value_size > 0)
	{
		value_buffer[value_size - 1] = '\0';
	}
	
	return size;
}
