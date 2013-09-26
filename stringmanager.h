#ifndef STRINGMANAGER_H
#define STRINGMANAGER_H

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

typedef enum {
	/* commands the user writes. during a select *, cmd_quit is positioned
	 * at index 4 (starting with 1) */
	cmd_quit = 4, cmd_balance, cmd_deposit, cmd_withdraw, cmd_help, 

	/* error messages */
	error_unknown_command, error_balance, error_deposit, error_withdraw, 

	/* success messages */
	msg_welcome, msg_balance, msg_deposit, msg_withdraw,

	/* requests to the user */
	rqst_enter_amount, rqst_enter_otp, rqst_card_number, rqst_pin,
} Strings;

typedef struct StringEntry {
	Strings string_name; /**> the name of the string */

	char *data; /**> the actual string */

	size_t length; /**> the length of the string, excluding the null byte */

	struct StringEntry *next; /**> the next string entry */
} StringEntry;

typedef struct StringManager {
	char lang_code[2]; /**> iso 639-1 language code */

	char *lang_name; /**> a human friendly language name */

	StringEntry *strings; /**> the strings */

	struct StringManager *next; /**> The next language */
} StringManager;

/**
 * Create a new string entry.
 * @param str The string to use
 * @param name The name of the string
 * @return The string entry. Should be free()'d after use.
 */
StringEntry *newString(char *str, Strings name);

/**
 * Create a new language.
 * The string manager should be passed to addString to add strings.
 * @param lang_code An iso 639-1 code for the language
 * @param lang_name A human frinendly language name
 * @return A string manager. Should be free()'d after use.
 */
StringManager *newLanguage(char *lang_code, char *lang_name);

/**
 * Get the language manager for a language.
 * @param languages A list of languages
 * @param lang_code An iso-639-1 code for the language to get
 * @return The language manager, or NULL if it does not exist
 */
StringManager *getLanguage(StringManager *languages, char *lang_code);

/**
 * Get a string from the string manager.
 * @param langauge The string manager of the language
 * @param string_name The name of the string
 * @return The stirng, or NULL if it does not exist.
 */
StringEntry *getString(StringManager *language, Strings string_name);

/**
 * Add a language to the list.
 * The new language will be put in the front of the list.
 * @param languages The list of languages
 * @param newLanguage The language to add
 */
void addLanguage(StringManager **languages, StringManager *newLanguage);

/**
 * Add a string to the language.
 * The string will be put in the front of the strings list.
 * @param lang The language
 * @param se The string to add
 */
void addString(StringManager *lang, StringEntry *se);

/**
 * Free the languages in the list.
 * @param languages The list to free
 */
void freeLanguageList(StringManager *languages);

/** 
 * Free the string entry list.
 * @param stringentry The list to free
 */
void freeStringEntry(StringEntry *stringentry);
#endif /* STRINGMANAGER_H */
