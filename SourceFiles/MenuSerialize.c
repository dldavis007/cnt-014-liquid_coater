#include "MenuSerialize.h"

/* Debugging output control for PC/GDB vs. an actual embedded system. */
#define EMBEDDED_SYSTEM 
//#undef EMBEDDED_SYSTEM /* For GDB/PC, enables more verbose console output. */

long total_raw_bytes_sent_by_sendPackets = 0;

/* Packet Buffer Globals */
#define BUFFERED_PACKET_SIZE 6
#define PACKETS_PER_FLUSH 10
#define MAX_UNIQUE_ARCHETYPES 32
#define FLUSH_THRESHOLD_BYTES (BUFFERED_PACKET_SIZE * PACKETS_PER_FLUSH) // 60 bytes
static unsigned char G_packet_buffer[FLUSH_THRESHOLD_BYTES];
static int current_buffer_fill = 0; // How many bytes are currently in G_packet_buffer
int handle_buffer_fail = 0;

// initialize enum_dict variables 
struct menu_var *enum_dict[MAX_UNIQUE_ENUM_STRINGS] = {NULL};
int enum_dict_counter = 0;

// initialize global menu_context variable, maintains state and counters for state machine send_serialize_menu function
process_context_t menu_context = {SERIALIZE_IDLE, 0, 0};

// precomputed int to string array for integers 0-50, used in the itoa_0_to_50 function to replace sprintf calls 
// TODO could likely use this in more places
static const char* int_to_str_0_50[] = {
    "0",  "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8",  "9",
    "10", "11", "12", "13", "14", "15", "16", "17", "18", "19",
    "20", "21", "22", "23", "24", "25", "26", "27", "28", "29",
    "30", "31", "32", "33", "34", "35", "36", "37", "38", "39",
    "40", "41", "42", "43", "44", "45", "46", "47", "48", "49",
    "50"
};


/* --- Token Dictionary --- */
// This dictionary maps frequently occurring strings to single-character tokens.
const char *token_dictionary_ptr = "MENU\0SETTINGS\0STATUS\0CAMERA\0DIAGNOSTICS\0SETUP\0STROKE\0PUMP\0SPEED\0PRESSURE\0SYRINGE-\0HEATER\0 TEMP\0 SETP\0LIGHTING\0ZOOM\0FOCUS\0ADVANCE\0SERIAL NUM\0EXIT\0 ILIM\0SOFTWARE \0DIAGNOSTIC\0STOP\0EXTEND\0RETRACT\0 TYPE\0SYRINGE\0RATIO\0DISPLAY\0LIN. ACT.\0COATER\0RESTORE\0DEFAULTS\0SY1\0SY2\0SY3\0SY4\0SY5\0SY6\0HEAT-1\0HEAT-2\0HEAT-3\0HEAT-4\0HEAT-5\0HEAT-6\0";/* The compiler adds the final \0 here, creating \0\0 */

// Characters used to represent tokens. '0' maps to token_dictionary_ptr[0], '1' to token_dictionary_ptr[1], etc. -- the index of token_dictionary_ptr
const char token_chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";



/**
 * @brief Manages a buffer for outgoing data to optimize sending small packets.
 * @details This function accumulates data in a global buffer (G_packet_buffer)
 *          and sends it in larger chunks (flushes) when the buffer reaches a
 *          predefined threshold (FLUSH_THRESHOLD_BYTES). This reduces the overhead
 *          of sending many small, individual packets. It can be commanded to
 *          flush its contents immediately. It also checks a global 'data_sending'
 *          flag, allowing an ongoing serialization process to be aborted.
 * @param value int value to be converted, effectively the 'index'
 * @param buffer buffer to copy the converted int_to_str value
 * @return char pointer to buffer (though I think if we're passing a buffer in, we don't really need it to be returned)
 */

char* itoa_0_to_50(int value, char* buffer) {
    if (value >= 0 && value <= 50) {
        strcpy(buffer, int_to_str_0_50[value]);
        return buffer;
    }
    /* Handle out of range - you might want to return NULL or handle differently */
    buffer[0] = '\0';
    return buffer;
}

/**
 * @brief Manages a buffer for outgoing data to optimize sending small packets.
 * @details This function clears the global G_packet_buffer. This is mostly for error handling
 *          
 * @param void
 * @return void
 */
void clear_packet_buffer(void) {
    int i;
    for (i = 0; i < FLUSH_THRESHOLD_BYTES; i++) {
        G_packet_buffer[i] = 0;
    }
}


/**
 * @brief Manages a buffer for outgoing data to optimize sending small packets.
 * @details This function accumulates data in a global buffer (G_packet_buffer)
 *          and sends it in larger chunks (flushes) when the buffer reaches a
 *          predefined threshold (FLUSH_THRESHOLD_BYTES). This reduces the overhead
 *          of sending many small, individual packets. It can be commanded to
 *          flush its contents immediately. It also checks a global 'data_sending'
 *          flag, allowing an ongoing serialization process to be aborted.
 * @param data The byte array containing the data to be buffered or sent.
 * @param length The number of bytes in the 'data' array. If 'length' is -1,
 *               the function performs a forced flush of any data currently in the buffer.
 *               If 'length' is <= 0 (but not -1), the function does nothing.
 * @return void
 */

void sendBufferedPackets(unsigned char data[], int length) {
    int bytes_to_copy;
    int data_offset; /* How much of the input 'data' we've processed */

    MCO_ProcessStack_Menu(); 

    /* FLUSH command */
    if (length == -1) {
        if (current_buffer_fill > 0) {
            sendPackets(G_packet_buffer, current_buffer_fill);
            current_buffer_fill = 0;
        }
        return;
    }

    /* Ignore invalid non-flush lengths */
    if (length <= 0) {
        return;
    }

    /* Scenario 1: Incoming data chunk is larger than our buffer's flush threshold. */
    if (length >= FLUSH_THRESHOLD_BYTES) {
        if (current_buffer_fill > 0) {
            sendPackets(G_packet_buffer, current_buffer_fill);
            current_buffer_fill = 0;
        }
        sendPackets(data, length);
        return;
    }

    /* Scenario 2: Incoming data can be added to the buffer. */
    data_offset = 0;
    while (data_offset < length) {
        int space_remaining_in_buffer = FLUSH_THRESHOLD_BYTES - current_buffer_fill;
        int data_chunk_len_to_process = length - data_offset;

        bytes_to_copy = data_chunk_len_to_process < space_remaining_in_buffer ? data_chunk_len_to_process : space_remaining_in_buffer;

        memcpy(G_packet_buffer + current_buffer_fill, data + data_offset, bytes_to_copy);
        current_buffer_fill += bytes_to_copy;
        data_offset += bytes_to_copy;

        /* If the buffer is now full (or has met the threshold), flush it. */
        if (current_buffer_fill >= FLUSH_THRESHOLD_BYTES) {
            /* current_buffer_fill should be exactly FLUSH_THRESHOLD_BYTES here,
             * because bytes_to_copy would not have allowed it to go over
             * significantly in one step if the input 'length' was < FLUSH_THRESHOLD_BYTES.
             */
            sendPackets(G_packet_buffer, FLUSH_THRESHOLD_BYTES); /* Send exactly the threshold amount */
            current_buffer_fill = 0; /* Buffer was flushed completely, so it's empty */
        }
    }
}


/**
 * @brief Removes leading and trailing whitespace from a string in-place.
 * @details This function modifies the provided string by moving pointers to find the
 *          first and last non-whitespace characters. It then null-terminates the
 *          string after the last non-whitespace character and, if necessary, moves
 *          the trimmed content to the beginning of the original buffer to remove
 *          leading whitespace.
 * @param str A pointer to the null-terminated string to be trimmed. The string is
 *            modified directly by the function.
 * @return void
 */

void trim_spaces(char *str) {
    char *start_ptr, *end_ptr;
    int length = strlen(str);
	
    if (str == NULL || *str == '\0') return;
	
   	start_ptr = str;
    end_ptr = str + length - 1;

    // Trim leading space
    while (*start_ptr && isspace((unsigned char)*start_ptr)) {
        start_ptr++;
    }

    // Trim trailing space
    while (end_ptr >= start_ptr && isspace((unsigned char)*end_ptr)) {
        end_ptr--;
    }

    // Null terminate the new end of the string
    *(end_ptr + 1) = '\0';

    // Shift the string if there was leading space
    if (start_ptr != str) {
        memmove(str, start_ptr, (size_t)(end_ptr - start_ptr) + 2); // +1 for char, +1 for null
    }
}

/**
 * @brief Finalizes menu text by trimming, intelligently truncating, and enforcing a maximum length.
 * @details This function prepares a string for display or serialization. It first trims all
 *          leading/trailing whitespace. It then applies heuristics to shorten the string at
 *          word boundaries, for instance, by truncating after the first or second word if they
 *          are separated by more than one space. Finally, it ensures the string does not
 *          exceed a specified maximum length, attempting to break at the last possible space
 *          before resorting to a hard truncation.
 * @param text_buffer The string to be finalized. This buffer is modified in-place.
 * @param max_length The maximum allowed length for the final string.
 * @return void
 */
void finalize_menu_text(char* text_buffer, int max_length) {
    int current_length;
    const char* ptr;
    const char* end_of_first_word;
	const char* first_non_space_after_first_word;
	const char* first_non_space_after_second_word;
	const char* start_of_second_word;
	const char* end_of_second_word;
	
	if (!text_buffer || max_length <= 0) return;

    if (strlen(text_buffer) != 1) {S_trim_spaces(text_buffer);}
    current_length = strlen(text_buffer);
    if (current_length == 0) return;

    ptr = text_buffer;
    end_of_first_word = text_buffer;
    while (*end_of_first_word && !isspace((unsigned char)*end_of_first_word)) {
        end_of_first_word++;
    }

    first_non_space_after_first_word = end_of_first_word;
    while (*first_non_space_after_first_word && isspace((unsigned char)*first_non_space_after_first_word)) {
        first_non_space_after_first_word++;
    }

    // If there's more than one space between first and second word, truncate to first word.
    if (*end_of_first_word && (first_non_space_after_first_word - end_of_first_word) > 1) {
        
        text_buffer[(int)(end_of_first_word - text_buffer)] = '\0';
        
        // Re-trim if truncation happened
        S_trim_spaces(text_buffer); 
    
    // If there's at least a second word
    } else if (*end_of_first_word) { 
        
        start_of_second_word = first_non_space_after_first_word;
        end_of_second_word = start_of_second_word;
        
        while (*end_of_second_word && !isspace((unsigned char)*end_of_second_word)) {
            end_of_second_word++;
        }
        
        first_non_space_after_second_word = end_of_second_word;
        
        while (*first_non_space_after_second_word && isspace((unsigned char)*first_non_space_after_second_word)) {
            first_non_space_after_second_word++;
        }
        // If there's more than one space between second and third word, truncate to first two words.
        
        if (*end_of_second_word && (first_non_space_after_second_word - end_of_second_word) > 1) {
        
            text_buffer[(int)(end_of_second_word - text_buffer)] = '\0';
            // Re-trim
            S_trim_spaces(text_buffer);
        }
    }

    current_length = strlen(text_buffer);
    
    if (current_length > max_length) {
        
        int break_at = -1;
        
        int i;
		// Try to break at the last space within max_length
        for (i = max_length - 1; i >= 0; --i) {
            if (isspace((unsigned char)text_buffer[i])) {
                
                break_at = i;
                break;
            }
        }
        if (break_at != -1) {
        
            text_buffer[break_at] = '\0';
            // Trim any space now at the end
            S_trim_spaces(text_buffer); 
        
        } else {
            // No space to break at, hard truncate
            text_buffer[max_length] = '\0';
        }
    }
}

/**
 * @brief Checks if a function pointer corresponds to an internal or non-serializable menu function.
 * @details This utility function is used to filter out function pointers that should not be
 *          included in the serialized function dictionary. These are typically internal
 *          menu-handling functions (like NullFunction, ExitMenu) or standard variable
 *          processors (like StdVarFunction) that do not represent a unique, user-callable action.
 * @param func_ptr The function pointer to check.
 * @return int Returns 1 (true) if the function should be ignored, otherwise returns 0 (false).
 */
int is_function_ignored(int (*func_ptr)(void)) {
    return (func_ptr == NullFunction ||
            func_ptr == ExitMenu ||
            func_ptr == StdVarFunction ||
            func_ptr == SkipVarFunction ||
            func_ptr == ArrayVarFunction);
}


/**
 * @brief Sends a two-byte sequence to mark the beginning of a data section.
 * @details This helper function transmits a standard section header used in the serialization
 *          protocol. The header consists of the ASCII Start of Text (STX) character,
 *          followed by a numeral that identifies the type of data section that follows
 *          (e.g., titles, functions, variables).
 * @param section_type_numeral A character representing the section type.
 * @return void
 */
void send_section_start(char section_type_numeral) {
    
    // create 2 byte array for start markers
    unsigned char start_sequence[2];
    
    // add generic start marker
    start_sequence[0] = ASCII_STX;
    
    // add section type numeral
    start_sequence[1] = (unsigned char)section_type_numeral;
    
    // send both bytes
    sendBufferedPackets(start_sequence, 2);
}

/**
 * @brief Sends a single-byte marker to signify the end of a data section.
 * @details This helper function transmits the ASCII End of Text (ETX - 03) character, which
 *          is the standard terminator for a data section in our serialization protocol.
 * @param void
 * @return void
 */
void send_section_end(void) {
    
    // create single byte array for end marker
    unsigned char end_marker = ASCII_ETX;
    
    // send single byte end marker
    sendBufferedPackets(&end_marker, 1);

}

/**
 * @brief Sends a string using a special "marked field" format.
 * @details This function encodes a string for transmission by setting the most significant
 *          bit (bit 7) of its first character. This marking allows the receiver to detect
 *          the start of a new field without requiring a length prefix. If the input string
 *          is NULL or empty, it sends a single byte (0x80) to represent an empty field.
 * @param str The null-terminated string to send.
 * @return void
 */
void send_marked_string(const char* str) {
    unsigned char first_byte_to_send;
    int length;
    unsigned char temp_payload_buffer[MAX_FIELD_LENGTH + 1];

    if (str == NULL || *str == '\0') {
        first_byte_to_send = 0x80; // Marker for empty string
        sendBufferedPackets(&first_byte_to_send, 1);
        return;
    }

    length = strlen(str);
    // Temporary buffer on stack for payload construction to avoid modifying original string.
    // MAX_FIELD_LENGTH must be > 0.

    if (length == 0) { // Should have been caught by *str == '\0' but defensive
        first_byte_to_send = 0x80;
        sendBufferedPackets(&first_byte_to_send, 1);
        return;
    }

    if (length > MAX_FIELD_LENGTH) { // Safety truncate if string is too long
    #ifndef EMBEDDED_SYSTEM
        printf("send_marked_string: WARN - input string truncated from %d to %d\n", length, MAX_FIELD_LENGTH);
    #endif
        length = MAX_FIELD_LENGTH;
    }

    // Set bit 7 of the first character
    temp_payload_buffer[0] = ((unsigned char)str[0]) | 0x80;
    if (length > 1) {
        memcpy(temp_payload_buffer + 1, str + 1, length - 1);
    }
    sendBufferedPackets(temp_payload_buffer, length);
}

/**
 * @brief Processes a string by finalizing it and then compressing it via tokenization.
 * @details This is a two-stage processing function. First, it calls finalize_menu_text
 *          on a copy of the input string to trim and truncate it. Second, it iterates
 *          through the finalized string, replacing any known substrings from a global
 *          dictionary with a compact two-character token. Characters not found in the
 *          dictionary are passed through unchanged.
 * @param original_input_string The original string to be processed.
 * @param output_buffer A buffer where the resulting finalized and tokenized string will be stored.
 * @param output_buffer_length The total size of the output_buffer.
 * @param final_max_length The maximum length to enforce during the finalization stage,
 *                         before tokenization.
 * @return char* A pointer to the output_buffer containing the processed string.
 */
char* finalize_and_tokenize_string(const char* original_input_string,
                                     char* output_buffer,
                                     int output_buffer_length,
                                     int final_max_length) {
    // Workspace buffer for finalize_menu_text, as it modifies in-place.
    char working_copy[MAX_FIELD_LENGTH + 1];
    // Pointer to read from the (finalized) working_copy
    const char* read_ptr; 
    // Pointer to write into the output_buffer
    char* write_ptr;      
    int remaining_output_buffer_len;
    
    if (!original_input_string || !output_buffer || output_buffer_length <= 0) {
        if (output_buffer && output_buffer_length > 0) output_buffer[0] = '\0';
        return output_buffer;
    }
    output_buffer[0] = '\0'; // Ensure output buffer is initially an empty string

    // Copy original string to working buffer and finalize it
    strncpy(working_copy, original_input_string, MAX_FIELD_LENGTH);
    working_copy[MAX_FIELD_LENGTH] = '\0'; // Ensure null termination
    finalize_menu_text(working_copy, final_max_length);

    read_ptr = working_copy;
    write_ptr = output_buffer;
    remaining_output_buffer_len = output_buffer_length;

    // Optimized tokenization loop using single-pass dictionary traversal
    while (*read_ptr != '\0' && remaining_output_buffer_len > 1) {
        int best_match_length = 0;
        int best_match_index = -1;
        const char *dict_ptr = token_dictionary_ptr;
        int token_index = 0;

        // Single pass through dictionary to find longest match
        while (*dict_ptr != '\0') {
            // Get current token length by finding next null terminator
            const char* token_end = dict_ptr;
            int token_len;
            while (*token_end != '\0') token_end++;
            token_len = token_end - dict_ptr;
            
            // Skip empty tokens and tokens longer than remaining input
            if (token_len > 0 && token_len <= (working_copy + MAX_FIELD_LENGTH - read_ptr)) {
                // Use optimized comparison - check last char first for quick rejection
                if (read_ptr[token_len - 1] == dict_ptr[token_len - 1] &&
                    (token_len == 1 || memcmp(read_ptr, dict_ptr, token_len) == 0)) {
                    
                    // Found a match - check if it's the longest so far
                    if (token_len > best_match_length) {
                        best_match_length = token_len;
                        best_match_index = token_index;
                    }
                }
            }
            
            dict_ptr = token_end + 1; // Move to next token
            token_index++;
        }

        // Apply the best match found
        if (best_match_index != -1 && remaining_output_buffer_len > 2) {
            *write_ptr++ = TOKEN_INTRODUCER;
            *write_ptr++ = token_chars[best_match_index];
            read_ptr += best_match_length;
            remaining_output_buffer_len -= 2;
        } else {
            // No token match or insufficient space, copy character as-is
            *write_ptr++ = *read_ptr++;
            remaining_output_buffer_len--;
        }
    }
    
    *write_ptr = '\0'; // Null-terminate the result
    return output_buffer;
}


/**
 * @brief Compares two menu_var structures to determine if they share the same archetype.
 * @details An "archetype" defines the fundamental properties of a variable, such as its
 *          increment step, min/max limits, display format, and associated enumerations.
 *          This function compares these specific fields to check if two variables behave
 *          identically, irrespective of their current value or name.
 * @param var1 Pointer to the first menu_var structure for comparison.
 * @param var2 Pointer to the second menu_var structure for comparison.
 * @return int Returns 1 (true) if the archetypes are equal, otherwise returns 0 (false).
 */
int are_var_archetypes_equal(const struct menu_var* var1, const struct menu_var* var2) {
    if (!var1 || !var2) return 0; // Null check
    return (var1->inc == var2->inc &&
            var1->min == var2->min &&
            var1->max == var2->max &&
            var1->dec_pos == var2->dec_pos &&
            var1->len_str == var2->len_str &&
            var1->str_enum == var2->str_enum); // Compare pointers for str_enum (assumes unique strings)
}

/**
 * @brief Determines if a line of text is likely a header for columnar data.
 * @details This heuristic function counts the number of space-separated words in a given
 *          string. If the word count meets or exceeds a specified minimum, the line is
 *          considered a plausible candidate for a header row, which is useful for
 *          generating descriptive names for array-like variables.
 * @param line_text The string to be analyzed.
 * @param min_expected_words The minimum number of words required for the line to be
 *                           considered a header.
 * @return int Returns 1 (true) if the line is a plausible header, otherwise returns 0 (false).
 */
int is_plausible_header_line(const char* line_text, int min_expected_words) {
    int word_count;
	const char* ptr;
	
	if (!line_text || min_expected_words <= 0) return 0;

    word_count = 0;
    ptr = line_text;
    while (*ptr) {
        while (*ptr && isspace((unsigned char)*ptr)) ptr++; // Skip leading spaces
        if (*ptr == '\0') break; // End of string
        word_count++;
        while (*ptr && !isspace((unsigned char)*ptr)) ptr++; // Skip to end of word
    }
    return word_count >= min_expected_words;
}

/**
 * @brief Extracts the Nth word from a space-delimited string.
 * @details This function scans a source string to locate and copy a specific word
 *          based on its 1-based position. Words are defined as sequences of non-whitespace
 *          characters separated by one or more whitespace characters.
 * @param source_string The string from which to extract a word.
 * @param n The 1-based index of the word to retrieve (e.g., 1 for the first word).
 * @param output_buffer A buffer to store the extracted word.
 * @param buffer_length The size of the output_buffer.
 * @return const char* Returns a pointer to the output_buffer on success, or NULL if the
 *                     Nth word does not exist or an input parameter is invalid.
 */
const char* get_nth_word(const char* source_string, int n, char* output_buffer, int buffer_length) {
    // Counter for words found so far
    int word_count = 0;
    // Pointer to traverse the source string
    const char* ptr;
    // Pointer to mark the beginning of current word
    const char* word_start;

    // Validate input parameters
    if (!source_string || !output_buffer || buffer_length <= 0 || n <= 0) {
        // Clear buffer if valid
        if (output_buffer && buffer_length > 0) output_buffer[0] = '\0';
        return NULL;
    }
    // Initialize output buffer as empty string
    output_buffer[0] = '\0';

    // Reset word counter
    word_count = 0;
    // Start at beginning of source string
    ptr = source_string;
    // Initialize word start pointer
    word_start = NULL;

    // Loop through each character in source string
    while (*ptr) {
        // Skip leading spaces
        while (*ptr && isspace((unsigned char)*ptr)) ptr++; 
        // Break if we've reached end of string
        if (*ptr == '\0') break;

        // Mark the start of current word
        word_start = ptr;
        // Increment word counter
        word_count++;
        // Move to end of word
        while (*ptr && !isspace((unsigned char)*ptr)) ptr++; // Move to end of word

        // Found the Nth word
        if (word_count == n) {
            // Calculate word length
            int length_of_word = (int)(ptr - word_start);
            
            // Word fits in buffer
            if (length_of_word < buffer_length) {
                // Copy word to buffer
                strncpy(output_buffer, word_start, (size_t)length_of_word);
                
                // Null terminate
                output_buffer[length_of_word] = '\0';
            } else { 
                // Word is too long for buffer, truncate
                strncpy(output_buffer, word_start, (size_t)buffer_length - 1);
                // Null terminate
                output_buffer[buffer_length - 1] = '\0';
            }
            // Return pointer to filled buffer
            return output_buffer;
        }
        // End of string after finding a word
        if (*ptr == '\0') break; 
    }
    // Nth word not found
    return NULL; 
}

/**
 * @brief Calculates the 0-based position of a variable within its linked-list chain.
 * @details In the menu system, a single menu item can be associated with a linked list
 *          (chain) of variables. This function traverses the chain to find the position
 *          (depth) of a target variable, which corresponds to its column index in a
 *          multi-variable row.
 * @param chain_head Pointer to the first variable in the linked list.
 * @param target_variable Pointer to the variable whose depth is to be determined.
 * @return int The 0-based depth of the variable if found, otherwise -1.
 */
int get_variable_chain_depth(const struct menu_var* chain_head, const struct menu_var* target_variable) {
    // Initialize depth counter starting at 0
    int depth = 0;
    
    // Start traversal from the head of the chain
    const struct menu_var* current_var = chain_head;
    
    // Traverse the linked list until we reach the end
    while (current_var) {
        // Check if current node matches the target variable
        if (current_var == target_variable) {
            // Found the target, return current depth
            return depth;
        }
        // Move to the next node in the chain
        current_var = current_var->next_var;
        // Increment depth counter for next iteration
        depth++;
    }
    // Not found
    return -1; 
}

/**
 * @brief Generates a user-friendly name for a menu variable, especially for array-like structures.
 * @details This function creates a descriptive name for a variable. For simple variables,
 *          it may use the variable's default menu text. For variables in an array-like
 *          chain, it attempts to combine the row's label (e.g., "SY1") with the
 *          corresponding column header text (e.g., "SPEED") to create a more specific name
 *          like "SY1 SPEED".
 * @param target_variable The variable for which to generate a name.
 * @param current_page_index The index of the page containing the variable.
 * @param current_item_index_on_page The 0-based row index of the menu item for this variable.
 * @param output_buffer A buffer to store the generated name.
 * @param buffer_length The size of the output_buffer.
 * @return const char* A pointer to the output_buffer containing the finalized name.
 */
const char* generate_descriptive_var_name(const struct menu_var* target_variable,
                                          size_t current_page_index,
                                          int current_item_index_on_page, // 0-based index of the menu item row
                                          char* output_buffer,
                                          int buffer_length) {
    // Buffers for words extracted from menu entries
    char raw_item_text_word[MAX_MENU_TEXT_LENGTH + 1];
    char column_header_word[MAX_MENU_TEXT_LENGTH + 1];
    char fallback_var_word[MAX_MENU_TEXT_LENGTH + 1];
    // Pointer to the chosen header line text for array-like variables
    const char* chosen_header_line_text = NULL;
    // Depth of target variable within its chain
    int chain_depth_of_target;
    // Indices for which words to extract from header and variable text
    int header_word_to_extract_idx, variable_word_to_extract_idx;
    // Usually the menu entry text for the variable itself
    const char* fallback_base_text; 
    // Pointer to the head of the variable chain
    const struct menu_var* head_of_variable_chain;
    // Result of sprintf operations
    int sprintf_result;
    // True if the variable is part of a chain (array-like)
    int is_array_type_flag = 0; 
    // Count of variables in the current chain
    int num_vars_in_current_chain = 0;
    // Temporary pointer for navigating the variable chain
    const struct menu_var* temp_var_navigator;
    // Index of next item on screen (1-based for Entry array)
    int next_item_screen_index;
    // Candidate text that might serve as a header
    const char* candidate_header_text;

    // Initialize output buffer
    if (!output_buffer || buffer_length <= 0) return ""; // Should not happen with sane inputs
    output_buffer[0] = '\0';

    // Fallback base text is usually the text of the menu item directly associated with the variable
    // Menuc[page].Entry[item_index + 1] is a common pattern for variable text
    fallback_base_text = Menuc[current_page_index].Entry[current_item_index_on_page + 1];

    // Get the head of the variable chain for this menu item
    head_of_variable_chain = Menuc[current_page_index].VarPntr[current_item_index_on_page];

    // No variable associated with this menu item
    if (!head_of_variable_chain) goto use_fallback_name_generation; // No variable associated with this menu item

    // Find the depth of target variable within the chain
    chain_depth_of_target = get_variable_chain_depth(head_of_variable_chain, target_variable);
    
    // Target variable not in this chain
    if (chain_depth_of_target == -1) goto use_fallback_name_generation; // Target variable not in this chain

    // Count variables in the chain
    temp_var_navigator = head_of_variable_chain;
    while (temp_var_navigator) {
        num_vars_in_current_chain++;
        
        // Move to next variable in chain
        temp_var_navigator = temp_var_navigator->next_var;
    }

    // More than one var in the chain implies array-like structure
    if (head_of_variable_chain->next_var != NULL) { 
        is_array_type_flag = 1;
    }

    // Handle array-like variables with special naming logic
    if (is_array_type_flag) {
        // Initialize header text pointer
        chosen_header_line_text = NULL;
        
        // Index on screen (1-based for Entry array)
        next_item_screen_index = current_item_index_on_page + 1; 

        // Heuristic: Try to find a header line.
        // First, check if the *current* item's text line (Menuc[page].Entry[item_index])
        // looks like a header for the *next* item if it's also an array.
        // This handles cases where a header applies to multiple subsequent array rows.
        if (current_item_index_on_page > 0 && /* Not the first item on page */
            head_of_variable_chain->next_var != NULL && /* Current is array-like */
            next_item_screen_index < NUM_MENU_ITEMS_PER_PAGE && /* Next item is on page */
            Menuc[current_page_index].VarPntr[next_item_screen_index] != NULL &&
            Menuc[current_page_index].VarPntr[next_item_screen_index]->next_var != NULL /* Next item is also array-like */) {

            candidate_header_text = Menuc[current_page_index].Entry[0]; // Text of current item
            if (is_plausible_header_line(candidate_header_text, num_vars_in_current_chain)) {
                chosen_header_line_text = candidate_header_text;
            }
        }

        // If not found, try using the page title (Entry[0]) as a header.
        if (chosen_header_line_text == NULL) {
            
            // Check if page title could serve as header
            if (is_plausible_header_line(Menuc[current_page_index].Entry[0], num_vars_in_current_chain)) {
                chosen_header_line_text = Menuc[current_page_index].Entry[0];
            }
        }

        // If we found a suitable header line, proceed with complex name generation
        if (chosen_header_line_text != NULL) {
            
            // Initialize and null terminate word buffer
            raw_item_text_word[0] = '\0';
            
            // Get the first word of the variable's own menu entry text
            if (get_nth_word(fallback_base_text, 1, raw_item_text_word, sizeof(raw_item_text_word))) {
                
                // Remove leading/trailing spaces
                S_trim_spaces(raw_item_text_word);
            }
            // Initialize column header word buffer
            column_header_word[0] = '\0';
            
            // The word to pick from the header corresponds to the variable's position in the chain (+2 for 1-based indexing and potential "label" word)
            header_word_to_extract_idx = chain_depth_of_target + 2;
            
            // Extract the appropriate word from the header line
            if (get_nth_word(chosen_header_line_text, header_word_to_extract_idx, column_header_word, sizeof(column_header_word))) {
                // Remove leading/trailing spaces
                S_trim_spaces(column_header_word);
            }
            // If both words were successfully extracted, combine them
            if (raw_item_text_word[0] != '\0' && column_header_word[0] != '\0') {
                
                // Combine: "ItemWord HeaderWord"
                sprintf_result = sprintf(output_buffer, "%s %s", raw_item_text_word, column_header_word);
                
                // Check for sprintf errors or buffer overflow 
                if (sprintf_result < 0 || sprintf_result >= buffer_length) goto use_fallback_name_generation;
                
                // Max 16 chars for descriptive name
                finalize_menu_text(output_buffer, 16); 
                
                return output_buffer;
            } else if (raw_item_text_word[0] != '\0') {
                // If header word extraction failed, try using a word from the variable's own line.
                fallback_var_word[0] = '\0';
                
                // Same logic for word index
                variable_word_to_extract_idx = chain_depth_of_target + 2; 
                
                // Extract word from variable's own menu text
                if (get_nth_word(fallback_base_text, variable_word_to_extract_idx, fallback_var_word, sizeof(fallback_var_word))) {
                    
                    // Remove leading/trailing spaces
                    S_trim_spaces(fallback_var_word);
                    // Check if word extraction was successful
                    if (fallback_var_word[0] != '\0') {
                        // Combine: "ItemWord VariableWordFromOwnLine"
                        sprintf_result = sprintf(output_buffer, "%s %s", raw_item_text_word, fallback_var_word);
                        // Check for sprintf errors or buffer overflow
                        if (sprintf_result < 0 || sprintf_result >= buffer_length) goto use_fallback_name_generation;
                        // Finalize the text to 16 characters
                        finalize_menu_text(output_buffer, 16);
                        return output_buffer;
                    }
                }
            }
        }
    }

use_fallback_name_generation:
    // If complex name generation fails, use the variable's direct menu entry text.
    strncpy(output_buffer, fallback_base_text, (size_t)buffer_length - 1);
    // Ensure null termination
    output_buffer[buffer_length - 1] = '\0';
    // Max 16 char
    finalize_menu_text(output_buffer, 16); 
    
    return output_buffer;
}


/**
 * @brief Serializes and sends the global token dictionary.
 * @details This function iterates through the master token string
 *          (token_dictionary_ptr), which contains multiple null-separated
 *          substrings. Each substring is a token that it sends as an individual
 *          "marked string" field within the token dictionary section of the
 *          serialization stream.
 * @param void
 * @return void
 */
void process_token_dictionary(void) {
    int i;
	int current_token_length;
    const char *p_token = token_dictionary_ptr;
    send_section_start(SECTION_TYPE_TOKEN_DICT);
    
    while (*p_token != '\0'){
        current_token_length = strlen(p_token);
        
        send_marked_string(p_token);

        p_token += current_token_length + 1; // Move to the next token (assuming null-terminated tokens)

    }
    send_section_end();
}

/**
 * @brief Serializes and sends all unique menu page titles.
 * @details This function iterates through every page in the menu structure. For each
 *          page, it determines the correct title (which can sometimes involve a
 *          complex lookup on other pages), finalizes and tokenizes the title string,
 *          and sends it as a marked string. It implicitly handles de-duplication
 *          by processing each page index once.
 * @param void
 * @return void
 */

void process_menu_titles(void) {
    char final_formatted_buffer[MAX_MENU_TEXT_LENGTH * 2 + 1]; // Buffer for tokenized string
    const char* title_source_ptr;
    size_t j_page_scan_counter;
    int first_pass_found_title_flag;
	size_t page_index;
    int current_page_idx0;
    int current_page_idx1;
    int current_page_idx2;
    int current_page_idx3_for_item_on_other_page;
	
    send_section_start(SECTION_TYPE_TITLES);

    for (page_index = 0; page_index < MenuSize; ++page_index) {
        title_source_ptr = NULL;
        first_pass_found_title_flag = 0;

        // Complex logic to find the "best" title for a page, especially if pages share indices.
        // Menuc[page_index].Index seems to store a multi-level index for the page.
        current_page_idx0 = (int)((unsigned char)Menuc[page_index].Index[0]);
        current_page_idx1 = (int)((unsigned char)Menuc[page_index].Index[1]);
        current_page_idx2 = (int)((unsigned char)Menuc[page_index].Index[2]);
        // Menuc[page_index].Index[3] seems to be an item index on a *different* page that holds this title.
        current_page_idx3_for_item_on_other_page = (int)((unsigned char)Menuc[page_index].Index[3]);

        // This heuristic seems to be for cases where a menu item on one page (e.g., page 0, item X)
        // actually serves as the title for another page.
        if (NUM_MENU_ITEMS_PER_PAGE > 0 &&
            Menuc[page_index].VarPntr[0] != NULL &&
            Menuc[page_index].VarPntr[0]->next_var != NULL) { // Heuristic: if page has array vars at first item

            int search_key_idx0 = 0; // Typically searching on page 0
            int search_key_idx1 = current_page_idx0;
            int search_key_idx2 = current_page_idx1;
            int search_key_idx3 = current_page_idx2;

            for (j_page_scan_counter = 0; j_page_scan_counter < MenuSize; ++j_page_scan_counter) {
                if ((int)((unsigned char)Menuc[j_page_scan_counter].Index[0]) == search_key_idx0 &&
                    (int)((unsigned char)Menuc[j_page_scan_counter].Index[1]) == search_key_idx1 &&
                    (int)((unsigned char)Menuc[j_page_scan_counter].Index[2]) == search_key_idx2 &&
                    (int)((unsigned char)Menuc[j_page_scan_counter].Index[3]) == search_key_idx3) {

                    if (current_page_idx3_for_item_on_other_page > 0 &&
                        current_page_idx3_for_item_on_other_page <= NUM_MENU_ITEMS_PER_PAGE) {
                        // The title is Menuc[j_page_scan_counter].Entry[current_page_idx3_for_item_on_other_page]
                        title_source_ptr = Menuc[j_page_scan_counter].Entry[current_page_idx3_for_item_on_other_page];
                        first_pass_found_title_flag = 1;
                        break;
                    }
                }
            }
        }

        if (title_source_ptr == NULL) { // Fallback to the page's own Entry[0]
            title_source_ptr = Menuc[page_index].Entry[0];
        }

        finalize_and_tokenize_string(title_source_ptr,
                                       final_formatted_buffer,
                                       sizeof(final_formatted_buffer),
                                       MAX_MENU_TEXT_LENGTH);
        send_marked_string(final_formatted_buffer);
    }
    send_section_end();
}

/**
 * @brief Scans all variables to create and serialize a dictionary of enum strings.
 * @details This function traverses the entire menu structure to find all variables that
 *          use enumerations. We initialize the dictionary, and then everytime we find a  
 *          enum_str, we compare it to the dictionary we've been building. If we've seen 
 *          it before, then we move on, if not, we add it to our dictionary and also
 *          send it over CAN after splitting it by commas, each option of the enum tokenized
 *          with a terminator
 * @param void
 * @return void
 */

void process_enum_dict(void){

    // pointer to current var
    struct menu_var* current_var;

    // Pointer to current enum string being examined
    const char* current_enum_string_ptr;
    
    // counter variables 
    int page_index = 0;
    int item_index = 0;


    
    // Buffer for strtok, which modifies its input, 95 seems to be limit without cropping data
    char temp_multistring_copy[95]; 

    // Pointer to current token from strtok
    char* token_ptr;

    // Delimiter for splitting comma-separated enum values
    const char* delimiter = ",";
    // Buffer for final tokenized output
    char final_tokenized_buffer[MAX_FIELD_LENGTH * 2 + 1];

    int been_added = 0;
    int i = 0;

    // reset global variables (because current logic only 'sends' enums when adding to dict, if full, nothing is sent)
    
    enum_dict_counter = 0;

    for (i = 0; i < MAX_UNIQUE_ENUM_STRINGS; i++) {
        enum_dict[i] = NULL;
    }


    // send section start
    send_section_start(SECTION_TYPE_ENUM_DICT);

    // loop through all pages of menu
    for (page_index = 0; page_index < MenuSize; page_index++){
        // loop through all items on page
        for (item_index = 0; item_index < NUM_MENU_ITEMS_PER_PAGE; item_index++){
            // get current var
            current_var = Menuc[page_index].VarPntr[item_index];

            // while current var is not none
            while (current_var){
                // Get enum string pointer from current variable
                current_enum_string_ptr = current_var->str_enum;
                // Check if enum string is valid (not NULL, not empty, not null enum)
                if (current_enum_string_ptr != NULL &&
                    current_enum_string_ptr != enum_NULL_str &&
                    *current_enum_string_ptr != '\0'){

                        
                        // check if enum has been seen
                        for (i = 0, been_added = 0; i < enum_dict_counter; i++)
                        {
                            
                            if (enum_dict[i] != NULL && strcmp(current_enum_string_ptr, enum_dict[i]->str_enum) == 0){
                                // we've already seen this
                                been_added = 1;
                                break;
                            }
                        }
                        // if we haven't seen this
                        if (been_added == 0 && enum_dict_counter < MAX_UNIQUE_ENUM_STRINGS){
                            
                            // add pointer to dictionary, technically we reference it as ->str_enum whenever we want the value 
                            enum_dict[enum_dict_counter] = current_var;

                            // increment counter since we added something to dictionary
                            enum_dict_counter++;

                            if (strlen(current_enum_string_ptr) < sizeof(temp_multistring_copy)) {
                                // Copy string to modifiable buffer for strtok
                                strcpy(temp_multistring_copy, current_enum_string_ptr);
                                // Get first token (split by comma)
                                token_ptr = strtok(temp_multistring_copy, delimiter);
                                // Process each comma-separated token
                                while (token_ptr != NULL) {
                                    // Tokenize and send each enum option
                                    finalize_and_tokenize_string(token_ptr, final_tokenized_buffer, sizeof(final_tokenized_buffer), MAX_FIELD_LENGTH);
                                    send_marked_string(final_tokenized_buffer);
                                    // Get next token
                                    token_ptr = strtok(NULL, delimiter);
                                }
                           
                            } else {
                                // String too long for strtok buffer, send as is (assuming it's a single, very long enum option)
                                finalize_and_tokenize_string(current_enum_string_ptr, final_tokenized_buffer, sizeof(final_tokenized_buffer), MAX_FIELD_LENGTH);
                                send_marked_string(final_tokenized_buffer);
                            }

                            // Send an empty marked string as a terminator for this enum set
                            send_marked_string(""); 
                        }

                    }
                // iterate to next variable in chain
                current_var = current_var->next_var;

            }

        }
    }
    // Send section end marker
    send_section_end();
}


/**
 * @brief Scans all menu items to create and serialize a de-duplicated dictionary of functions.
 * @details This function traverses the menu structure to find all assignable functions,
 *          skipping ignored ones. For each valid function, it performs a "look-behind"
 *          scan to see if the same function pointer has already been processed. If it's a
 *          new, unique function, its identifier (composed of its associated menu text and
 *          page index) is serialized.
 * @param void
 * @return void
 */
void process_func_dict(void) {
    // Function pointers for outer and inner loop comparison
    int (*outer_func_ptr)(void), (*inner_func_ptr)(void);
    // Buffer for menu item text associated with function
    char item_text_buffer[MAX_MENU_TEXT_LENGTH + 1];
    // Buffer for string representation of page index
    char page_index_buffer_str[10];
    // Buffer for final tokenized output
    char final_tokenized_buffer[MAX_MENU_TEXT_LENGTH * 2 + 1];
    // Flag to track if current function has already been defined
    int is_already_defined_flag;
    // Outer loop page index
	size_t outer_page_idx;
    // Outer loop item index
	int outer_item_idx;
    // Inner loop page index for deduplication check
	size_t inner_page_idx;
    // Inner loop item index for deduplication check
	int inner_item_idx;
	
    // Send section header for function dictionary
    send_section_start(SECTION_TYPE_FUNC_DICT);

    // Loop through all pages in menu system (outer loop)
    for (outer_page_idx = 0; outer_page_idx < MenuSize; ++outer_page_idx) {
        // Loop through all items on current page (outer loop)
        for (outer_item_idx = 0; outer_item_idx < NUM_MENU_ITEMS_PER_PAGE; ++outer_item_idx) {
            // Get function pointer for current menu item
            outer_func_ptr = Menuc[outer_page_idx].FunctPtr[outer_item_idx];

            // Only process non-NULL functions that are not ignored
            if (outer_func_ptr != NULL && !is_function_ignored(outer_func_ptr)) {
                // Initialize deduplication flag
                is_already_defined_flag = 0;
                // Look-behind de-duplication, Check all previous pages and current page up to current position
                for (inner_page_idx = 0; inner_page_idx <= outer_page_idx; ++inner_page_idx) {
                    // Set item limit: full page for previous pages, current item for current page
                    int inner_loop_item_limit = (inner_page_idx == outer_page_idx) ? outer_item_idx : NUM_MENU_ITEMS_PER_PAGE;
                    // Loop through items in inner page up to limit
                    for (inner_item_idx = 0; inner_item_idx < inner_loop_item_limit; ++inner_item_idx) {
                        // Get function pointer for comparison
                        inner_func_ptr = Menuc[inner_page_idx].FunctPtr[inner_item_idx];
                        // Check if function pointers match (same function)
                        if (inner_func_ptr == outer_func_ptr) {
                            // Mark as already defined and exit nested loops
                            is_already_defined_flag = 1;
                            goto func_dedup_check_done;
                        }
                    }
                }
                // Label for breaking out of nested deduplication loops
                func_dedup_check_done:;

                // If function is unique (not already defined)
                if (!is_already_defined_flag) {
                    // Function is new, send its "name" (menu text) and page index
                    // Menuc[page].Entry[item_idx + 1] is typically the text for item_idx
                    strncpy(item_text_buffer, Menuc[outer_page_idx].Entry[outer_item_idx + 1], MAX_MENU_TEXT_LENGTH);
                    // Ensure null termination
                    item_text_buffer[MAX_MENU_TEXT_LENGTH] = '\0';
                    // finalize_menu_text is done by finalize_and_tokenize_string with 16 char limit.

                    // Convert page index to string
                    sprintf(page_index_buffer_str, "%u", (unsigned int)outer_page_idx);

                    // Send function name (menu text) - limited to 16 characters
                    finalize_and_tokenize_string(item_text_buffer, final_tokenized_buffer, sizeof(final_tokenized_buffer), 16); // Name limited to 16
                    send_marked_string(final_tokenized_buffer);

                    // Send page index string
                    finalize_and_tokenize_string(page_index_buffer_str, final_tokenized_buffer, sizeof(final_tokenized_buffer), 10); // Page index string
                    send_marked_string(final_tokenized_buffer);
                }
            }
        }
    }
    // Send section end marker
    send_section_end();
}

/**
 * @brief looks for given enum_str_ptr in the enum_dictionary, returning index if found, -1 if not
 * @details This function traverses the enum_dictionary (assumed to have been populated earlier in the process)
 *          and returns the index of the entry in the dictionary, or -1 if not found. Should be noted
 *          that enum_dict and enum_dict_counter are global variables that are reset in the process_enum_dict
 *          function. This function assumes that process_enum_dict has been run before and will return -2 if 
 *          it hasn't 
 * @param target_enum_str_ptr a given pointer to a target enum that we're trying to find and return the index of
 * @return int, index of the given target_enum_str_ptr if found, -1 if not found
 */

int get_enum_dict_idx(const char* target_enum_str_ptr){

    int i;

    // Different error code for "dictionary not initialized", assumes enum_dict_counter will be > 0 if enum_dict has been made
    if (enum_dict_counter == 0) {
        
        return -2; 
    }
	// Return -1 for NULL, empty, or null enum string, basic error check
    if (target_enum_str_ptr == NULL || 
        target_enum_str_ptr == enum_NULL_str || 
        *target_enum_str_ptr == '\0') {
        return -1;
    }

    // check if enum is in dictionary
    for (i = 0; i < enum_dict_counter; i++)
    {
        // if we find it, return index, else return -1
        if (enum_dict[i] != NULL && strcmp(target_enum_str_ptr, enum_dict[i]->str_enum) == 0){
            return i;
        }
    }
    // return -1 if enum not in dictionary
    return -1;
}



/**
 * @brief A safe wrapper for printing a float value using the "%g" format specifier.
 * @details This function formats a float into a string. It includes a specific
 *          workaround for compilers (c89) that may fail to correctly print a value of 0.0,
 *          ensuring it is always represented as the string "0".
 * @param buf The character buffer to store the resulting string.
 * @param val The float value to format.
 * @return void
 */
void print_g_value (char buf[], float val){
	sprintf(buf, "%g", val);
    if (val == 0.0)
		sprintf(buf, "0");
}
/**
 * @brief   iterates through all menu_vars, if archetype flag has not been seen, we send it over the can and set all 
 *          archetype_index of all menu_vars who have the same archetype
 * @details This is part of the send_serialized_menu state machine function. It first resets all the archetype indexes
 *          (this is because the current logic ONLY sends archetypes when it finds new ones), looks for a unique archetype
 *          (archetype_index which is 0xFF, which is our reset value), sends archetype over CAN, then sets archetype_index
 *          of all menu_vars that match that archetype (ensuring that 0xFF indicates archetypes that are unique) 
 * @param void
 * @return void
 */

void process_var_arch_dict(void){
    struct menu_var* current_var;
    int page_idx;
    int item_idx;
    int enum_dictionary_index;
    char final_tokenized_buffer[MAX_FIELD_LENGTH * 2 + 1];

    // Current archetype index counter
    char current_archetype_idx = 0;

    // reset all archetype_indices to 0xFF before starting
    // TODO -- consider only doing this for the first call, and save for later calls
    reset_all_archetype_indices();
    // send section header for variable archetype dictionary
    send_section_start(SECTION_TYPE_VAR_ARCH_DICT);


    for (page_idx = 0; page_idx < MenuSize; page_idx++){
        // loop through all items on this page
        for (item_idx = 0; item_idx < NUM_MENU_ITEMS_PER_PAGE; item_idx++) {
            // loop through all chained variables for this item
            current_var = Menuc[page_idx].VarPntr[item_idx];
            // while current_var is not NULL
            while (current_var) {
                // if current_var is not NullVar and not yet processed (is unprocess if 0xFF)
                if (current_var != &NullVar && current_var->archetype_index == 0xFF) {
                    char numeric_buffer[20];

                    // Get enum dictionary index
                    // TODO -- consider optimizing the following function which uses deduplication logic
                    enum_dictionary_index = get_enum_dict_idx(current_var->str_enum);

                    // Send increment value over CAN                    
                    print_g_value(numeric_buffer, current_var->inc);
                    finalize_and_tokenize_string(numeric_buffer, final_tokenized_buffer, sizeof(final_tokenized_buffer), 10);
                    send_marked_string(final_tokenized_buffer);

                    // Send minimum value over CAN
                    print_g_value(numeric_buffer, current_var->min);
                    finalize_and_tokenize_string(numeric_buffer, final_tokenized_buffer, sizeof(final_tokenized_buffer), 10);
                    send_marked_string(final_tokenized_buffer);

                    // Send maximum value over CAN
                    print_g_value(numeric_buffer, current_var->max);
                    finalize_and_tokenize_string(numeric_buffer, final_tokenized_buffer, sizeof(final_tokenized_buffer), 10);
                    send_marked_string(final_tokenized_buffer);

                    // Send decimal position over CAN
                    sprintf(numeric_buffer, "%d", current_var->dec_pos);
                    finalize_and_tokenize_string(numeric_buffer, final_tokenized_buffer, sizeof(final_tokenized_buffer), 3);
                    send_marked_string(final_tokenized_buffer);

                    // Send string length over CAN
                    sprintf(numeric_buffer, "%d", current_var->len_str);
                    finalize_and_tokenize_string(numeric_buffer, final_tokenized_buffer, sizeof(final_tokenized_buffer), 3);
                    send_marked_string(final_tokenized_buffer);

                    // Send enum dictionary index over CAN
                    sprintf(numeric_buffer, "%d", enum_dictionary_index);
                    finalize_and_tokenize_string(numeric_buffer, final_tokenized_buffer, sizeof(final_tokenized_buffer), 3);
                    send_marked_string(final_tokenized_buffer);

                    // IMPORTANT assign this archetype index to all variables with the same archetype, this sets the archetype_index field
                    // and ensures we don't send duplicates
                    assign_archetype_index_to_matching_variables(current_var, current_archetype_idx);

                    // Increment the current archetype index for next unique archetype
                    current_archetype_idx++;
                }
                current_var = current_var->next_var;
            }
        }
    }
    
    send_section_end();
}


/**
 * @brief Resets the 'processed_flag' for every variable in the entire menu system.
 * @details This function performs a complete traversal of the menu structure, accessing
 *          each 'menu_var' instance and setting its 'processed_flag' member to 0. It is
 *          a utility function to ensure a clean state before a serialization pass that
 *          relies on this flag for de-duplication.
 * @param void
 * @return void
 */
void reset_var_processed_flags(void) {
    // Index for iterating through menu pages
    size_t page_idx;
    // Index for iterating through menu items on each page
    int item_idx;
    // Pointer to current variable being processed
    struct menu_var* current_var;

    // Loop through all pages in the menu system
    for (page_idx = 0; page_idx < MenuSize; ++page_idx) {
        // Loop through all menu items on current page
        for (item_idx = 0; item_idx < NUM_MENU_ITEMS_PER_PAGE; ++item_idx) {
            
            // Get the first variable associated with this menu item
            current_var = Menuc[page_idx].VarPntr[item_idx];
            
            // Traverse the linked list of variables for this menu item
            while (current_var) {
                // Skip the special NullVar placeholder
                if (current_var != &NullVar) {
                    // Reset the processed flag to 0
                    current_var->processed_flag = 0;
                }
                // Move to the next variable in the chain
                current_var = current_var->next_var;
            }
        }
    }
}

/**
 * @brief Serializes the instance data for every unique variable using pre-computed indices.
 * @details This function performs the final stage of variable serialization. It iterates
 *          through all variables in the menu. For each unique variable instance, it sends
 *          a data packet containing its current value, its descriptive name, its page index,
 *          and its archetype index (which was pre-calculated by
 *          process_var_arch_dict).
 * @param void
 * @return void
 */

int process_var_instance_data(void) {
    // Pointer to current variable being processed
    struct menu_var* current_var;
    // Buffer for generated descriptive variable name
    char descriptive_name_buffer[MAX_MENU_TEXT_LENGTH + 1];
    // Temporary buffer for string value (max 11 chars + null terminator)
    char string_value_temp[12];
    // Index of variable's archetype in dictionary
    int archetype_dictionary_index;
    // Index of page title in dictionary
    int title_dictionary_index;
    // Buffer for sprintf of numeric values
    char numeric_buffer[20];
    // Buffer for final tokenized output
    char final_tokenized_buffer[MAX_FIELD_LENGTH * 2 + 1];
    // Page index for iteration
    int page_idx;
    // Item index for iteration
    int item_idx;


    // Check if we're starting fresh
    if (menu_context.VAR_INSTANCE_DATA_counter == 0) {
        // First call - do initialization that was at the start of original function
        reset_var_processed_flags();
        send_section_start(SECTION_TYPE_VAR_INSTANCES);
    }
    
    // Check if we've processed all pages
    if (((int)(menu_context.VAR_INSTANCE_DATA_counter / NUM_MENU_ITEMS_PER_PAGE)) >= (int)MenuSize) {
        // All pages processed - do cleanup
        send_section_end();
        return 1;
    }

    // with each call of doevents we process one current_var
    page_idx = (int)menu_context.VAR_INSTANCE_DATA_counter / NUM_MENU_ITEMS_PER_PAGE;
    item_idx = menu_context.VAR_INSTANCE_DATA_counter % NUM_MENU_ITEMS_PER_PAGE;
        
    // Get first variable associated with this menu item
    current_var = Menuc[page_idx].VarPntr[item_idx];
    
    // Traverse variable chain for current menu item
    while (current_var) {
        // Skip NullVar sentinel
        if (current_var != &NullVar) {
        
            //check the flag and Only process if variable hasn't been processed yet
            if (current_var->processed_flag == 0) {
                // Mark this variable as processed so we don't handle it again.
                current_var->processed_flag = 1;

                // Copy string value with length limit (11 chars max)
                strncpy(string_value_temp, current_var->str_value, 11);
                // Ensure null termination
                string_value_temp[11] = '\0';

                // Use the pre-computed archetype index
                archetype_dictionary_index = (int)current_var->archetype_index; 
                // Use page index as title dictionary index
                title_dictionary_index = (int)page_idx;

                // Generate user-friendly descriptive name for the variable
                generate_descriptive_var_name(current_var, page_idx, item_idx,
                                                descriptive_name_buffer, sizeof(descriptive_name_buffer));

                // Send string value (tokenized and finalized to 11 chars)
                finalize_and_tokenize_string(string_value_temp, final_tokenized_buffer, sizeof(final_tokenized_buffer), 11);
                send_marked_string(final_tokenized_buffer);

                // Send archetype dictionary index (asssumes number is <=50) also doesn't need finalize_and_tokenize_string (unless we add numbers
                // to the dictionary)
                itoa_0_to_50(archetype_dictionary_index, final_tokenized_buffer);
                // finalize_and_tokenize_string(numeric_buffer, final_tokenized_buffer, sizeof(final_tokenized_buffer), 3);
                send_marked_string(final_tokenized_buffer);

                // Send descriptive name (tokenized and finalized to 16 chars)
                finalize_and_tokenize_string(descriptive_name_buffer, final_tokenized_buffer, sizeof(final_tokenized_buffer), 16);
                send_marked_string(final_tokenized_buffer);

                // send title dictionary index (asssumes number is <=50), also doesn't need finalize_and_tokenize_string (unless we add numbers
                //to the dictionary)
                itoa_0_to_50(title_dictionary_index, final_tokenized_buffer);
                // finalize_and_tokenize_string(numeric_buffer, final_tokenized_buffer, sizeof(final_tokenized_buffer), 3);
                send_marked_string(final_tokenized_buffer);
            }
        }
        // Move to next variable in chain
        current_var = current_var->next_var;
    }

    // Increment counter to move to next page
    menu_context.VAR_INSTANCE_DATA_counter++;

    return 0; // Indicate that processing is not yet complete

}

/**
 * @brief Given reference menu_var and archetype_index, finds all menu_var with matching archetypes and sets them
 * @details This is part of the process_var_dict_arch function. Once we find a unique archetype (a variable
 *          who's archetype_index is 0xFF, our reset value) we send it and then after sending it, we iterate 
 *          through the entire menu and set all archetype_indexes of matching menu_vars. It's necessary to process_var_dict_arch
 *          because it assumes that any menu_var that's 0xFF is unique (because otherwise this function would set it)
 *          and thus is how we ensure only unique archetypes are used    
 * @param reference_var pointer to menu_var struct, pointer to reference var to compare archetypes
 * @param archetype_idx archetype index to assign to matching menu_var variables when searching
 * @return void
 */
void assign_archetype_index_to_matching_variables(struct menu_var* reference_var, char archetype_idx) {
    struct menu_var* current_var;
    size_t page_idx;
    int item_idx;
    
    // Search through all variables and assign archetype index to matching archetypes
    for (page_idx = 0; page_idx < MenuSize; ++page_idx) {
        for (item_idx = 0; item_idx < NUM_MENU_ITEMS_PER_PAGE; ++item_idx) {
            current_var = Menuc[page_idx].VarPntr[item_idx];
            
            while (current_var) {
                // only assign if not NullVar, archetype_index is 0xFF (not yet assigned), and archetypes match
                if (current_var != &NullVar && current_var->archetype_index == 0xFF &&
                    are_var_archetypes_equal(reference_var, current_var)) {
                    current_var->archetype_index = archetype_idx;
                }
                current_var = current_var->next_var;
            }
        }
    }
}

/**
 * @brief Iterates through the entire menu, and sets all archetype_indexes to 0xFF to clear them before using them
 * @details This is part of the process_var_dict_arch function, which assumes that all menu_vars with archetype_indexs
 *          set to 0xFF haven't been seen before. It's crucial that this function is run at the start of process_var_dict_arch
 * @param void 
 * @return void
 */
void reset_all_archetype_indices(void) {
    struct menu_var* current_var;
    size_t page_idx;
    int item_idx;
    
    for (page_idx = 0; page_idx < MenuSize; ++page_idx) {
        for (item_idx = 0; item_idx < NUM_MENU_ITEMS_PER_PAGE; ++item_idx) {
            current_var = Menuc[page_idx].VarPntr[item_idx];
            
            while (current_var) {
                // if it's not NullVar, set archetype_index to 0xFF value
                if (current_var != &NullVar) {
                    current_var->archetype_index = 0xFF; // Mark as unprocessed
                }
                current_var = current_var->next_var;
            }
        }
    }
}


// TODO would the following function be improved if we only iterated through the menuc once? 

/**
 * @brief Main function that serializes the Menuc structure and sends it over the CAN using a State Machine Architecture to ensure non-blocking
 * @details This function is called every iteration of doevents. Depending on which STATE that menu_context is set to (initialized to SERIALIZE_IDLE)
 *          it will preform a part of the process to serialize the menuc structure (containing the unit menu) and send it out over the CAN. By default
 *          it runs sequentially, meaning that once the SERIALIZE_INIT function is called, it will move 'down' the switch case statement list
 * 
 *          SERIALIZE_IDLE: basic do-nothing case, default state
 *          SERIALIZE_INIT: clears related global and menu_context variables and sends the init start markers, moves to next state
 *          SERIALIZE_SERIALIZE_TOKEN_DICT: sends the 'hard-coded' token dictionary designed for this specific unit (mostly used in the LIQUID COATER case)
 *              but it still needs to be sent, even if we're not really using it for the parser to work. CHecks if Packet data has been set and runs those functions
 *              (via MCO_ProcessStack_menu)
 *          SERIALIZE_SERIALIZE_MENU_TITLES: sends the Menu titles that will serve as section titles in the python GUI, Checks if Packet data has been set and runs those functions
 *              (via MCO_ProcessStack_menu)
 *          SERIALIZE_ENUM_DICT: creates enum_dictionary while iterating through Menu structure and sends it, Checks if Packet data has been set and runs those functions
 *              (via MCO_ProcessStack_menu)
 *          SERIALIZE_FUNC_DICT: uses deduplication logic to iterate through menu structure, sending only unique functions, Checks if Packet data has been set and runs those functions
 *              (via MCO_ProcessStack_menu)
 *          SERIALIZE_VAR_ARCH_DICT: iterates through menuc, sets archetype_index for all menu_vars (after clearing them), sends each unique archetype in the order encountered
 *              in the menu then sets archetype_index for all menu_vars with matching archetype. Checks if Packet data has been set and runs those functions (via MCO_ProcessStack_menu)
 *          SERIALIZE_VAR_INSTANCE_DATA: iterates through menuc one variable at a time, clears processed_flags and sets processed_flag when the menu_var is sent. 
 *              Checks if Packet data has been set and runs those functions (via MCO_ProcessStack_menu)
 *          SERIALIZE_FINALIZE: End state, clears relevant variables, sends section end markers, clears buffer and then sets state to SERIALIZE_IDLE
 * 
 * @param void 
 * @return int: returns 1 on completion (not doing anything with it now but might be used for error handling in the future)
 */

int send_serialized_menu(void){
    //TODO: documentation, this is a state machine that accomplishes the same as S_serialize_complete_menu_look_behind but in a non-blocking way
    
    switch(menu_context.state){
        case SERIALIZE_IDLE:
            // Not currently sending, just return
            break;
        
        case SERIALIZE_INIT:
            // Reset byte counter
            total_raw_bytes_sent_by_sendPackets = 0; 
            // Initialize buffer fill state 
            current_buffer_fill = 0; 

            // clear global_packet_buffer current_buffer_fill refers to 
            clear_packet_buffer();
            
            // Reset context counters
            menu_context.VAR_INSTANCE_DATA_counter = 0;
            menu_context.VAR_ARCH_DICT_counter = 0;
            
            // just send initial section start marker
            send_section_start(SECTION_TYPE_START_DATA);

            // Move to next stage without doing mco process stack here, assuming time delay so small we can just wait
            menu_context.state = SERIALIZE_TOKEN_DICT;
            break;
        // Stage 1: Send the token dictionary itself
        case SERIALIZE_TOKEN_DICT:
            // small enough we don't need to chunk it (break it up if it gets bigger)
            process_token_dictionary();

            // process packets received during processing
            MCO_ProcessStack_Menu();

            // Move to next stage
            menu_context.state = SERIALIZE_MENU_TITLES;
            break;
        // Stage 2: Send unique menu titles
        case SERIALIZE_MENU_TITLES:
            // small enough we don't need to chunk it (break it up if it gets bigger)  
            process_menu_titles();

            // process packets received during processing
            MCO_ProcessStack_Menu();

            // Move to next stage
            menu_context.state = SERIALIZE_ENUM_DICT;
            break;

        // Stage 3: Send unique enum strings
        case SERIALIZE_ENUM_DICT:
            // might need to consider chunking this
            process_enum_dict();

            // process packets received during processing
            MCO_ProcessStack_Menu();
            
            // Move to next stage
            menu_context.state = SERIALIZE_FUNC_DICT;
            break;

        // Stage 4: Send unique function name and index (first unique encounter in menu) pairs
        case SERIALIZE_FUNC_DICT:
            // might need to consider chunking this
            process_func_dict();

            // process packets received during processing
            MCO_ProcessStack_Menu();

            menu_context.state = SERIALIZE_VAR_ARCH_DICT;
            break;

        // Stage 5: Send unique variable archetypes and populate archetype indices for all variables
        case SERIALIZE_VAR_ARCH_DICT:
            
            // sends var archetypes all at once 
            process_var_arch_dict();

            // process packets received during processing
            MCO_ProcessStack_Menu();

            menu_context.state = SERIALIZE_VAR_INSTANCE_DATA;
            break;


        // Stage 6: Send variable instance data
        case SERIALIZE_VAR_INSTANCE_DATA:
            // send variable data one variable at a time, return when done
            // returns 1 when done, 0 if more to do
            if (process_var_instance_data()){
                // only move to finalize when done
                menu_context.state = SERIALIZE_FINALIZE;
            }

            // process packets received during processing
            MCO_ProcessStack_Menu();
    
            break;

        // Stage 7: send end markers and clear buffer and reset menu_context 
        case SERIALIZE_FINALIZE:
            
            // Send section end marker
            send_section_end();
        
            // perform final buffer flush, make sure this is called after send_section_end
            sendBufferedPackets(NULL, -1);

            // move to IDLE state, wait until next call
            menu_context.state = SERIALIZE_IDLE;
            break;

        //Error/Abort State: Triggered if an error, simply returns to IDLE state without sending end markers or flushing packet buffer
        case SERIALIZE_ABORT:

            // move to IDLE state, wait until next call
            menu_context.state = SERIALIZE_IDLE;
            break;

    }
        
    return 1;
}


/**
 * @brief Sends a real-time update for a single variable's value to the GUI.
 * @details This function is called when a variable's value changes locally. It identifies
 *          the variable's type (numeric, enum, or string), finds its unique index in the
 *          menu structure, and constructs a type-specific packet containing the index,
 *          new value, and a checksum. This allows the GUI to stay synchronized without
 *          requiring a full menu reload.
 * @param var Pointer to the menu_var structure that has been updated.
 * @return int Returns 0 on successful transmission, -1 on failure (e.g., variable not found).
 */
int update_value_gui(struct menu_var *var) {
	// This function updates the value of a menu variable and returns 0 on success.
	// It assumes that the variable is valid and can be updated.
	// IMPORTANT: this function currently handles only variables that have updated both string (value_str) and float (value) representations 
	int is_string = 0;
	int is_enum = 0;
	int var_menu_index = 0;
	char tmp_send_buffer[STR_VALUE_LEN]; // buffer for formated packets, STR_VALUE_LEN as it will be the maximum length the buffer should need to hold
	float new_value;
	int checksum = 0;
	int i;

	if (var == NULL) {
		return -1; // Error: variable is NULL
	}

	// strings will have non-null str_enum pointer and a negative len_str
	is_string = (var->str_enum != NULL && var->len_str < 0);

	// enums will have a non-null str_enum pointer and a positive len_str
	is_enum = (var->str_enum != NULL && var->str_enum != enum_NULL_str && var->str_enum != '\0' && var->len_str > 0);

	// is string case
	if (is_string) {
		send_section_start(SECTION_TYPE_SINGLE_STR);

		// find string variable in menu
		var_menu_index = find_menu_variable_index(var);

		if (var_menu_index < 0) {
            return -1; // Error: variable not found in menu    
		}

        // format the variable index as a string
		sprintf(tmp_send_buffer, "%d", var_menu_index); 
		
		for (i = 0; tmp_send_buffer[i] != '\0'; i++) {
             // Update the checksum with each character
			checksum ^= (unsigned char)tmp_send_buffer[i];
		}

        // send the variable index
		send_marked_string(tmp_send_buffer); 
		
		if (var->str_value == NULL) {
			// Error: variable does not have a string value pointer
            return -1; 
		}

        // Update the checksum with each character of the string
		for (i = 0; var->str_value[i] != '\0'; i++) {
			checksum ^= (unsigned char)var->str_value[i]; 
		}

        // send the value string
		send_marked_string(var->str_value); 

        // Format the checksum as a two-digit hexadecimal string
		sprintf(tmp_send_buffer, "%02X", checksum); 
        // Send the checksum
		send_marked_string(tmp_send_buffer); 

        // Send the end of section marker
		send_section_end(); 
		// Final flush of any remaining data in the buffer 
		sendBufferedPackets(NULL, -1);

        // Success
		return 0; 
	}
	// is enum case
	else if (is_enum) {
        // Send the start of section marker
		send_section_start(SECTION_TYPE_SINGLE_ENUM); 

		// find enum variable in menu
		var_menu_index = find_menu_variable_index(var);

		if (var_menu_index < 0) {
			// Error: variable not found in menu
            return -1; 
		}

		// format the variable index as a string
		sprintf(tmp_send_buffer, "%d", var_menu_index); 

		// Iterate through the characters of the index string and update the checksum
		for (i = 0; tmp_send_buffer[i] != '\0'; i++) {
			checksum ^= (unsigned char)tmp_send_buffer[i];
		}
		
		// send the variable index
		send_marked_string(tmp_send_buffer); 

        // Get the current value of the variable
		new_value = var->value; 

		// Error: value out of bounds
		if (new_value < var->min || new_value > var->max) {
			// Only clear data_sending if it wasn't set when we entered
            return -1; 
		}
        // Format the value as a string
		sprintf(tmp_send_buffer, "%f", new_value); 

		// Iterate through the characters of the index string and update the checksum
		for (i = 0; tmp_send_buffer[i] != '\0'; i++) {
			checksum ^= (unsigned char)tmp_send_buffer[i];
		}

		// send the value
		send_marked_string(tmp_send_buffer); 


		// send the checksum
		sprintf(tmp_send_buffer, "%02X", checksum); // Format the checksum as a two-digit hexadecimal string
		send_marked_string(tmp_send_buffer); // Send the checksum

		// send regular numeric variable specific end markers
		send_section_end(); // Send the end of section marker
		
		// Final flush of any remaining data in the buffer 
    	sendBufferedPackets(NULL, -1);	

        // Success
		return 0; 
	}
	// regular numeric variable case
	else {
		
		// send regular numeric variable specific start markers
		send_section_start(SECTION_TYPE_SINGLE_VAR); // Send the start of section marker

        // find first instance of variable in menu, returning index
		var_menu_index = find_menu_variable_index(var);
		
		if (var_menu_index < 0) {
            return -1; // Error: variable not found in menu
		}

		new_value = var->value; // Get the current value of the variable
		
		// Error: value out of bounds
		if (new_value < var->min || new_value > var->max) {
            return -1; 
		}

		// format the variable index as a string
		sprintf(tmp_send_buffer, "%d", var_menu_index); 

		// Iterate through the characters of the index string and update the checksum
		for (i = 0; tmp_send_buffer[i] != '\0'; i++) {
			checksum ^= (unsigned char)tmp_send_buffer[i];
		}
		
		// send the variable index
		send_marked_string(tmp_send_buffer); 


		if (var->str_value == NULL){
            return -1; // Error: variable does not have a string value pointer
		} 

		// Iterate through the characters of the value string and update the checksum
		for (i = 0; var->str_value[i] != '\0'; i++) {
			checksum ^= (unsigned char)var->str_value[i];
		}

		// send the value
		send_marked_string(var->str_value); 

		// send the checksum
		sprintf(tmp_send_buffer, "%02X", checksum); // Format the checksum as a two-digit hexadecimal string
		send_marked_string(tmp_send_buffer); // Send the checksum

		// send regular numeric variable specific end markers
		send_section_end(); // Send the end of section marker
		
		// Final flush of any remaining data in the buffer 
    	sendBufferedPackets(NULL, -1);	

		return 0;

	}

	
}
/**
 * @brief Finds the unique, sequential index of a specific variable instance.
 * @details This function scans the entire menu structure using the same de-duplication logic
 *          as the serialization process to determine a consistent, 0-based index for a
 *          given variable pointer. This unique index is used by the GUI to identify and
 *          modify specific variables. We're 
 *          using the deduplication logic because of two reasons 1. the single pass logic 
 *          uses the processed_flag in the menu_vars which is already being used by the 
 *          send_serialized_menu process -- these functions have the possibility of 'interrupting'
 *          that process and clearing the process_flags -- and 2. since we're only 
 *          calling this function once, the deduplication logic, while inefficient 
 *          doesn't create that much delay
 * @param var A pointer to the menu_var instance whose index is needed.
 * @return int The unique index of the variable if found, otherwise -1.
 */

int find_menu_variable_index(struct menu_var* var) {
	// This function scans the menu structure for a specific variable and returns the index 
	
	// Variables used in the scanning process
	size_t outer_page_idx, inner_page_idx;
	int outer_item_idx, inner_item_idx;
	// int max_items_per_page = 11; // Assuming each page has 11 items, adjust as necessary
	
	// Pointers to the current variable being checked
	
	struct menu_var* outer_current_var, *inner_current_var;
    int is_already_defined_instance_flag;
	int local_scan_index = 0; // This will track the unique variable instances found


    for (outer_page_idx = 0; outer_page_idx < MenuSize; ++outer_page_idx) {
        for (outer_item_idx = 0; outer_item_idx < NUM_MENU_ITEMS_PER_PAGE; ++outer_item_idx) {
            
            outer_current_var = Menuc[outer_page_idx].VarPntr[outer_item_idx];
            
            while (outer_current_var) {
                
                if (outer_current_var != &NullVar) {
                    
                    is_already_defined_instance_flag = 0;
                    // Look-behind de-duplication for variable instances (pointer equality)
                    for (inner_page_idx = 0; inner_page_idx <= outer_page_idx; ++inner_page_idx) {
                        int inner_loop_item_limit = (inner_page_idx == outer_page_idx) ? outer_item_idx : NUM_MENU_ITEMS_PER_PAGE;
                        
                        for (inner_item_idx = 0; inner_item_idx < inner_loop_item_limit; ++inner_item_idx) {
                            
                            inner_current_var = Menuc[inner_page_idx].VarPntr[inner_item_idx];
                            
                            while (inner_current_var) {
                                
                                if (inner_current_var == outer_current_var) { // Pointer comparison
                                    is_already_defined_instance_flag = 1;
                                    goto var_instance_dedup_check_done;
                                }
                                inner_current_var = inner_current_var->next_var;
                            }
                        }
                        if (inner_page_idx == outer_page_idx) {
                            
                            inner_current_var = Menuc[outer_page_idx].VarPntr[outer_item_idx];
                             
                            while (inner_current_var && inner_current_var != outer_current_var) {
                                
                                if (inner_current_var == outer_current_var) {
                                    
                                    is_already_defined_instance_flag = 1;
                                    goto var_instance_dedup_check_done;
                                }
                                inner_current_var = inner_current_var->next_var;
                            }
                        }
                    }
                    var_instance_dedup_check_done:;

                    if (!is_already_defined_instance_flag) {
	
						// this is the variable we are looking for
						if (outer_current_var == var){
							return local_scan_index; // Return the index of the variable
						}
						else {
							// this is not the variable we are looking for
							local_scan_index++;
						}
                    }
                }
                outer_current_var = outer_current_var->next_var;
            }
        }
    }
	return -1; // If not found, return -1
}

// this is a better function but we have to table it because it might interfere with the send_serialize_menu state machine (specifically the processed_flags
// it relies on to find unique menu_vars)

// int find_menu_variable_index(struct menu_var* target_var){
//     struct menu_var* current_var;
//     int page_idx;
//     int item_idx;
//     char final_tokenized_buffer[MAX_FIELD_LENGTH * 2 + 1];

//     // Current index counter
//     char local_scan_index = 0;

//     // reset all processed_flags to 0 before starting
//     reset_var_processed_flags();

//     for (page_idx = 0; page_idx < MenuSize; page_idx++){
//         // loop through all items on this page
//         for (item_idx = 0; item_idx < NUM_MENU_ITEMS_PER_PAGE; item_idx++) {
//             // loop through all chained variables for this item
//             current_var = Menuc[page_idx].VarPntr[item_idx];
//             // while current_var is not NULL
//             while (current_var) {

//                 if (current_var != &NullVar && current_var->processed_flag == 0){
//                     // set processed flag to indicate it's the first time we've seen this variable
//                     current_var->processed_flag = 1;

//                     // if current_var is the target var, return index 
//                     if (current_var == target_var){
//                         return local_scan_index;
//                     }
//                     else{
//                         // increment index if current_var is not the target var
//                         local_scan_index++;
//                     }
//                 }
//                 // move to next var in Chain
//                 current_var = current_var->next_var;
//             }
//         }
//     }
//     // return -1 if var wasn't found
//     return -1;
// }


/**
 * @brief Retrieves a pointer to a menu variable based on its unique sequential index.
 * @details This function performs the reverse operation of find_menu_variable_index. It
 *          scans the menu structure, counting unique variable instances until it finds
 *          the one corresponding to the provided index. We're 
 *          using the deduplication logic because of two reasons 1. the single pass logic 
 *          uses the processed_flag in the menu_vars which is already being used by the 
 *          send_serialized_menu process -- these functions have the possibility of 'interrupting'
 *          that process and clearing the process_flags -- and 2. since we're only 
 *          calling this function once, the deduplication logic, while inefficient 
 *          doesn't create that much delay
 * @param variable_index The unique, 0-based index of the variable to find.
 * @return struct menu_var* A pointer to the found variable, or NULL if the index is out of bounds.
 */
struct menu_var* scan_menu_variables(int variable_index) {
    // This function scans the menu structure for a variable with a specific index.
	// It returns a pointer to the variable if found, or NULL if not found.
	
	// Variables used in the scanning process
	size_t outer_page_idx, inner_page_idx;
	int outer_item_idx, inner_item_idx;
	// int max_items_per_page = 11; // Assuming each page has 11 items, adjust as necessary
	
	// Pointers to the current variable being checked
	
	struct menu_var* outer_current_var, *inner_current_var;
    int is_already_defined_instance_flag;
	int local_scan_index = 0; // This will track the unique variable instances found


    for (outer_page_idx = 0; outer_page_idx < MenuSize; ++outer_page_idx) {
        for (outer_item_idx = 0; outer_item_idx < NUM_MENU_ITEMS_PER_PAGE; ++outer_item_idx) {
            
            outer_current_var = Menuc[outer_page_idx].VarPntr[outer_item_idx];
            
            while (outer_current_var) {
                
                if (outer_current_var != &NullVar) {
                    
                    is_already_defined_instance_flag = 0;
                    // Look-behind de-duplication for variable instances (pointer equality)
                    for (inner_page_idx = 0; inner_page_idx <= outer_page_idx; ++inner_page_idx) {
                        int inner_loop_item_limit = (inner_page_idx == outer_page_idx) ? outer_item_idx : NUM_MENU_ITEMS_PER_PAGE;
                        
                        for (inner_item_idx = 0; inner_item_idx < inner_loop_item_limit; ++inner_item_idx) {
                            
                            inner_current_var = Menuc[inner_page_idx].VarPntr[inner_item_idx];
                            
                            while (inner_current_var) {
                                
                                if (inner_current_var == outer_current_var) { // Pointer comparison
                                    is_already_defined_instance_flag = 1;
                                    goto var_instance_dedup_check_done;
                                }
                                inner_current_var = inner_current_var->next_var;
                            }
                        }
                        if (inner_page_idx == outer_page_idx) {
                            
                            inner_current_var = Menuc[outer_page_idx].VarPntr[outer_item_idx];
                             
                            while (inner_current_var && inner_current_var != outer_current_var) {
                                
                                if (inner_current_var == outer_current_var) {
                                    
                                    is_already_defined_instance_flag = 1;
                                    
                                    goto var_instance_dedup_check_done;
                                }
                                inner_current_var = inner_current_var->next_var;
                            }
                        }
                    }
                    var_instance_dedup_check_done:;

                    if (!is_already_defined_instance_flag) {
	
						// this is the variable we are looking for
						if (local_scan_index == variable_index){
							return outer_current_var;
						}
						else {
							// this is not the variable we are looking for
							local_scan_index++;
						}
                    }
                }
                outer_current_var = outer_current_var->next_var;
            }
        }
    }
	return NULL; // If not found, return NULL
}
/**
 * @brief Retrieves a function pointer based on its unique sequential index.
 * @details This function scans the menu structure for unique, non-ignored functions,
 *          counting them in the same order as the serialization process. It returns the
 *          function pointer that corresponds to the specified index in this sequence. We're 
 *          using the deduplication logic because of two reasons 1. the single pass logic 
 *          uses the processed_flag in the menu_vars which is already being used by the 
 *          send_serialized_menu process -- these functions have the possibility of 'interrupting'
 *          that process and clearing the process_flags -- and 2. since we're only 
 *          calling this function once, the deduplication logic, while inefficient 
 *          doesn't create that much delay
 *          
 * @param variable_index The unique, 0-based index of the function to find.
 * @return MenuFunctionPointer A function pointer (`int (*)(void)`) to the found function, or NULL.
 */
MenuFunctionPointer scan_menu_functions(int variable_index) {
// Variables used in the scanning process
	size_t outer_page_idx, inner_page_idx;
	int outer_item_idx, inner_item_idx;
	// int max_items_per_page = 11; // Assuming each page has 11 items, adjust as necessary

	// Pointers to the current variable being checked
	
	int (*outer_func_ptr)(void), (*inner_func_ptr)(void);
    int is_already_defined_flag = 0;
	int local_scan_index = 0; // This will track the unique variable instances found


    for (outer_page_idx = 0; outer_page_idx < MenuSize; ++outer_page_idx) {
        for (outer_item_idx = 0; outer_item_idx < NUM_MENU_ITEMS_PER_PAGE; ++outer_item_idx) {
            
            outer_func_ptr = Menuc[outer_page_idx].FunctPtr[outer_item_idx];

            if (outer_func_ptr != NULL && !is_function_ignored(outer_func_ptr)) {
                is_already_defined_flag = 0;
                // Look-behind de-duplication
                for (inner_page_idx = 0; inner_page_idx <= outer_page_idx; ++inner_page_idx) {
                    
                    int inner_loop_item_limit = (inner_page_idx == outer_page_idx) ? outer_item_idx : NUM_MENU_ITEMS_PER_PAGE;
                    
                    for (inner_item_idx = 0; inner_item_idx < inner_loop_item_limit; ++inner_item_idx) {
                        
                        inner_func_ptr = Menuc[inner_page_idx].FunctPtr[inner_item_idx];
                        
                        if (inner_func_ptr == outer_func_ptr) {
                            is_already_defined_flag = 1;
                            goto func_dedup_check_done;
                        }
                    }
                }
                func_dedup_check_done:;

                if (!is_already_defined_flag) {
                    
					if (local_scan_index == variable_index) {
						// this is the function we are looking for
						return outer_func_ptr;
					} else {	
						// this is not the function we are looking for
						local_scan_index++;
					}
                }
            }
        }
    }
	return NULL; // If not found, return NULL
}


/**
 * @brief Executes a menu function based on an index received from the GUI.
 * @details This function takes a payload containing a function's unique index. It uses
 *          `scan_menu_functions` to resolve this index to an actual function pointer
 *          and then executes the function.
 * @param buf A pointer to the payload buffer, where the first byte is the function index.
 * @return int Returns 1 on success, 0 on failure (function not found).
 */
int handle_function(char *buf){
    // extract relevant variables
    
    int index;
    // Define a function pointer variable to hold the result
    int (*function_to_call)(void);

	index = buf[0];

	// scan menu functions function
	function_to_call = scan_menu_functions(index);
	// if variable is not a function, return
	if (function_to_call != NULL) {
		function_to_call();
		return 1; // Indicate success
	}
	return 0; // Indicate failure
          
}

/**
 * @brief Updates the value of a string-type variable based on data from the GUI.
 * @details This function processes a command to change a string variable. The payload
 *          contains the variable's index and the new string value. It locates the
 *          variable, updates its `str_value`, and synchronizes its state.
 * @param buf A pointer to the payload buffer, containing the variable index followed
 *            by the new null-terminated string.
 * @return int Returns 1 on success, 0 on failure.
 */
int handle_str(char *buf){
    //extract relevant variables 

    struct menu_var *vp;
    char *value_str;
    int maxlen;
    int i;
	int index;

	index = buf[0];

	value_str = buf + 1; // payload starts after first byte

	// find the variable in the menu
	vp = scan_menu_variables(index);
	// if variable is not found, return
	if (vp == NULL) {
		return 0;
	}
	

	// if variable is not a string, return
	if (vp->len_str >= 0) {
		return 0;
	}

	// initialize str_value to spaces
	for (i = 0; i < STR_VALUE_LEN; i++) {
		vp->str_value[i] = ' ';
	}

    // if len_str is negative, reverse it to make it usable
    maxlen = (vp->len_str >= 0 ? vp->len_str : -vp->len_str);

    //copy data from value_str pointer to rxbuffer to menu_var 
    for (i = 0; i < maxlen && value_str[i] != '\0'; i++){
        vp->str_value[i] = value_str[i];
    } 

    // if enum, pad spaces after null terminator with spaces
    for (; i < maxlen; i++){
        vp->str_value[i] = ' ';
    }

    // null terminate
    vp->str_value[maxlen] = '\0';
    
    // update value based on new string
    getvalue(vp, 0);
	// normally would call update_value_gui here, but since this variable is coming from the GUI, we don't need to update the GUI here

	return 1;
}

/**
 * @brief Updates the value of a numeric variable based on data from the GUI.
 * @details This function processes a command to change a numeric variable. The payload
 *          contains the variable's index and its new value (as a 4-byte integer
 *          representation). It locates the variable, updates its `value`, and updates
 *          its string representation accordingly.
 * @param buf A pointer to the payload buffer, containing the variable index followed by
 *            the 4-byte value.
 * @return int Returns 1 on success, 0 on failure.
 */
int handle_var(char *buf){
    // extract relevant variables
    struct menu_var *vp;
    char *value_str;
    int maxlen;
    int i;
    unsigned long int_representation;
	int index;

	index = buf[0];
    
	//payload starts after first two UID bytes
    value_str = buf + 1;

    // convert rxbuffer into a int
    int_representation = ((unsigned long)value_str[3] << 24) | ((unsigned long)value_str[2] << 16) | ((unsigned long)value_str[1] <<  8) | ((unsigned long)value_str[0]); 

	// find the variable in the menu
	vp = scan_menu_variables(index);
	 
	// if variable is not found, return
	if (vp == NULL) {
		return 0;
	}

	// if variable is not an integer, return
	if (vp->len_str <= 0) {
		return 0;
	}
	
    memcpy(&(vp->value), &int_representation, sizeof(float));

    getstrval(vp);

	// normally would call update_value_gui here, but since this variable is coming from the GUI, we don't need to update the GUI here

	return 1;
}


/**
 * @brief Top-level command parser for data received from the GUI.
 * @details This function inspects an incoming data buffer for a command keyword
 *          (e.g., "fun,", "str,", "menu"). Based on the command, it dispatches the
 *          payload to the appropriate handler function to execute an action, update a
 *          variable, or trigger a full menu serialization.
 * @param buf The character buffer containing the full command string.
 * @return void
 */
void handle_buffer(char buf[]){
    
    // strncmp should be safe 
    // as long as buf (pointer pointing to rxbuffer) is null terminated
    // which it should be 
	int function_success = 0;
	int string_success = 0;
 	int variable_success = 0;

    // buf is command to call function
    if (strncmp(buf, "fun,", 4) == 0){
        
        // remove four command bytes here by setting pointer to spot in buff past 'fun,'
        function_success = handle_function(buf + 4);
    }
    // command is to change string variable
    else if(strncmp(buf, "str,", 4) == 0) {
        
        // remove four command bytes here by sending pointer to spot in buff past 'str,'
        string_success = handle_str(buf + 4);
    }
    // command is to change numeric variable
    else if(strncmp(buf, "var,", 4) == 0){
        
        // remove four command bytes here by sending pointer to spot in buff past 'var,'
        variable_success = handle_var(buf + 4);
    }
	else if(strncmp(buf, "menu", 4) == 0) {
		
        // send complete menu serialization
        menu_context.state = SERIALIZE_INIT; // start state machine from beginning

	}
	else if(strncmp(buf, "save", 4) == 0) {
		
        // save variables to flash
		Save_Variables();
	}
    else if(strncmp(buf, "abort", 5) == 0) {
        
        // move state machine to abort state to stop serialization
        menu_context.state = SERIALIZE_ABORT; 
    }
    else {
        // increment global fail counter for better debugging
        handle_buffer_fail++;
    }

}