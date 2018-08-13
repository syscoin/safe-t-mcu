#include <stdint.h>
#include <string.h>

#include "gettext.h"
#include "storage.h"
#include "intl/intl.h"

char const * strGetTrad(const char * str) {
    const char * language = storage_getLanguage();
    if (language) {
        for(int i = 0; i < STR_NUM; i++)
        {
            if (strcmp(str, en_strings[i]) == 0) {
                if (strcmp(language, "FR") == 0) {
                    return fr_strings[i];
                } else if (strcmp(language, "EN") == 0) {
                    return en_strings[i];
                } else if (strcmp(language, "DE") == 0) {
                    return de_strings[i];
                }
                break;
            }
        }
    }
    return str;
}