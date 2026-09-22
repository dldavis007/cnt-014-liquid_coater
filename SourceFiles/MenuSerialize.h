#ifndef MENU_SERIALIZE_H // Typically named after the file, so assuming this is Subroutines.h
#define MENU_SERIALIZE_H

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stddef.h> // For size_t
#include "Subroutines.h"
#include "Packets.h" // For sendPackets and receivePackets functions

//---------------------------------------------------------------------------
// Compilation Flag for testing (e.g., in dummy functions if needed)
// This should ideally be managed by your build system (e.g., -D ONLINEGDB_TESTING)
// #define ONLINEGDB_TESTING
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// Core Constants
//---------------------------------------------------------------------------
#define MAX_MENU_TEXT_LENGTH 20  // Max characters for a menu entry text (excluding null)
#define NUM_MENU_ITEMS_PER_PAGE 11 // Number of interactive items (VarPntr/FunctPtr) per page
                                 // MenuStruct.Entry has 12 rows: 0 for title, 1-11 for items

// Limits for data collection arrays in main_program.c
#define MAX_UNIQUE_VARS         190
#define MAX_UNIQUE_FUNCS        20 // Your previous .c had 25, then 20. Keeping 20.
#define MAX_UNIQUE_ENUM_STRINGS 15
#define MAX_MENU_PAGES          25 // Maximum number of pages Menuc can hold
#define S_trim_spaces trim_spaces                // Alias for use within serializer functions

/* --- Markers & Token Constants for Compressed Mode --- */
#define ASCII_STX ((unsigned char)0x02)         // Start of TeXt, marks beginning of a data section
#define ASCII_ETX ((unsigned char)0x03)         // End of TeXt, marks end of a data section

// Numerals identifying the type of data section following an STX
#define SECTION_TYPE_START_DATA      '0'        // Section for the start of data
#define SECTION_TYPE_TOKEN_DICT      '1'        // Section for the token dictionary itself
#define SECTION_TYPE_TITLES          '2'        // Section for menu page titles
#define SECTION_TYPE_ENUM_DICT       '3'        // Section for unique enumeration strings
#define SECTION_TYPE_FUNC_DICT       '4'        // Section for unique function identifiers
#define SECTION_TYPE_VAR_ARCH_DICT   '5'        // Section for unique variable archetypes
#define SECTION_TYPE_VAR_INSTANCES   '6'        // Section for variable instances
#define SECTION_TYPE_SINGLE_VAR      '7'        // Section for a single variable instance
#define SECTION_TYPE_SINGLE_ENUM     '8'        // Section for a single enumeration string
#define SECTION_TYPE_SINGLE_STR      '9'        // Section for a single string instance


#define TOKEN_INTRODUCER ((unsigned char)0x01) // SOH (Start of Heading), indicates a token follows
#define MAX_FIELD_LENGTH 20  
//---------------------------------------------------------------------------
// Extern Declarations for Global Data & Functions defined in MenuData.c
//---------------------------------------------------------------------------

// Standard Function Declarations
extern int NullFunction(void);
extern int StdVarFunction(void);
extern int ExitMenu(void);
extern int RestoreDefaults(void);
extern int BlowerOn(void);
//extern int FocusNearFunct(void);
extern int RetractLA(void);
extern int ExtendLA(void);
extern int Advance(void);
//extern int ZoomInFunct(void);
//extern int ZoomOutFunct(void);
//extern int FocusFarFunct(void);
extern int ArrayVarFunction(void);
//extern int RetractAll(void);
//extern int ExtendAll(void);
//extern int StopAll(void);
extern int SkipVarFunction(void);
void sendBufferedPackets(unsigned char data[], int length);
void trim_spaces(char *str);
void send_section_start(char section_type_numeral);
void send_marked_string(const char* str);
void send_section_end(void);
int send_serialized_menu(void);
int process_var_instance_data(void);
void process_token_dictionary(void);
void process_menu_titles(void);
void process_func_dict(void);
void process_enum_dict(void);
void process_var_arch_dict(void);
char* finalize_and_tokenize_string(const char* original_input_string,
                                     char* output_buffer,
                                     int output_buffer_length,
                                     int final_max_length);
void reset_var_processed_flags(void);
int get_enum_dict_idx(const char* target_enum_str_ptr);
void finalize_menu_text(char* text_buffer, int max_length);
int are_var_archetypes_equal(const struct menu_var* var1, const struct menu_var* var2);
void reset_all_archetype_indices(void);
void assign_archetype_index_to_matching_variables(struct menu_var* reference_var, char archetype_idx);
void print_g_value (char buf[], float val);
char* itoa_0_to_50(int value, char* buffer);
int is_plausible_header_line(const char* line_text, int min_expected_words);
const char* get_nth_word(const char* source_string, int n, char* output_buffer, int buffer_length);
int get_variable_chain_depth(const struct menu_var* chain_head, const struct menu_var* target_variable);
const char* generate_descriptive_var_name(const struct menu_var* target_variable,
                                          size_t current_page_index,
                                          int current_item_index_on_page, // 0-based index of the menu item row
                                          char* output_buffer,
                                          int buffer_length);


// Functions to help Communicate with Python GUI
typedef int (*MenuFunctionPointer)(void);
void handle_buffer(char buf[]);
int handle_function(char *buf);
int handle_str(char *buf);
int handle_var(char *buf);
int MCO_ProcessStack_Menu(void);
struct menu_var* scan_menu_variables(int variable_index);
int find_menu_variable_index(struct menu_var* var);
int is_function_ignored(int (*function_pointer)(void));
MenuFunctionPointer scan_menu_functions(int variable_index);
int update_value_gui(struct menu_var* var);



// custom typedef to define the states of the State machine used by send_serialized_menu
typedef enum {
    SERIALIZE_IDLE,
    SERIALIZE_INIT,
    SERIALIZE_TOKEN_DICT,
    SERIALIZE_MENU_TITLES,
    SERIALIZE_ENUM_DICT,
    SERIALIZE_FUNC_DICT,
    SERIALIZE_VAR_ARCH_DICT, 
    SERIALIZE_VAR_INSTANCE_DATA, 
    SERIALIZE_FINALIZE,
    SERIALIZE_ABORT
} serialize_state_t;

// custom struct to hold related variables for the state machine 
typedef struct {
    serialize_state_t state;
    int VAR_ARCH_DICT_counter;
    int VAR_INSTANCE_DATA_counter;
} process_context_t;
extern process_context_t menu_context;


extern struct menu_var NullVar; // Defined in MenuData.c

extern const char enum_NULL_str[];
extern const char enum_off_on_str[];
extern const char enum_stop_retract_str[];
extern const char enum_base_iso_str[];
extern const char enum_lin_act_str[];
extern const char enum_alpha_str[];
extern const char enum_number_str[];
extern const char enum_ext_ret[];
extern const char enum_open_close_default[];
// Extern Menu Structure Array Declaration
//extern const struct MenuStruct Menuc[]; // Defined and initialized in MenuData.c
// extern const size_t MenuSize;           // Number of pages in Menuc, defined in MenuData.c



#endif // SUBROUTINES_H