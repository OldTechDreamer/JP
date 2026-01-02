# JSON Parser
A simple JSON parsing library written in C.
This library does not allocate any memory.
Instead of converting a JSON string into some group of structs, it uses functions like `jp_value()` and `kp_index()` to find the start address of the value in the JSON. You then use functions such as `jp_char()` and `jp_int()` to decode the values.

Please see `test.c` for some examples.
