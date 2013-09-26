#include "stringmanager.h"

StringManager *newLanguage(char *lang_code, char *lang_name)
{
	StringManager *sm = (StringManager *) malloc(sizeof(StringManager));
	strncpy(sm->lang_code, lang_code, 2);
	sm->lang_name = lang_name;
	sm->strings = NULL;
	sm->next = NULL;
	return sm;
}

StringEntry *newString(char *str, Strings name)
{
	StringEntry *se = (StringEntry *) malloc(sizeof(StringEntry));
	se->data = str;
	se->length = strlen(str);
	se->next = NULL;
	return se;
}

StringManager *getLanguage(StringManager *languages, char *lang_code)
{
	StringManager *lang;
	for (lang=languages; lang!=NULL; lang=lang->next) {
		if (strncmp(lang_code, lang->lang_code, 2) == 0) {
			/* we found the language we are looking for */
			return lang;
		}
	}

	return NULL;
}

StringEntry *getString(StringManager *language, Strings string_name)
{
	StringEntry *str;
	for (str=language->strings; str!=NULL; str=str->next) {
		if (str->string_name == string_name) {
			/* we found the string we are looking for */
			return str;
		}
	}

	return NULL;
}

void addLanguage(StringManager **languages, StringManager *newLanguage)
{
	newLanguage->next = *languages;
	*languages = newLanguage;
}

void addString(StringManager *lang, StringEntry *se)
{
	se->next = lang->strings;
	lang->strings = se;
}

void freeLanguageList(StringManager *languages)
{
	if (languages->next != NULL) {
		freeLanguageList(languages->next);
	}

	free(languages);
}

void freeStringEntry(StringEntry *stringentry) 
{
	if (stringentry->next != NULL) {
		freeStringEntry(stringentry->next);
	}

	free(stringentry);
}
