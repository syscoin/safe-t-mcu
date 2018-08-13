# Translation on Safe-T Mini

- Languages currently availables:
  - English (Originial text)
  - French
  - German

Header files used for translation (located at [intl folder](../intl/intl.h)) are generated automatically by a [script](./generate_translation_headers.py) reading Portable Object files generated thanks to [gettext tools](https://www.gnu.org/software/gettext/)

## Generating / Updating .po files from code
### Generating a new .po file for a new language
To generate a po file for the first time for a new language, take a look at the `make init` command from the [Makefile](./Makefile)
You must have a locale installed for the language you want to have in the application. 
### Updating exisiting po files to add new translations
When modifying the source code, if you add a new string and want it to be translated, use the Macro function defined in [`gettext.h`](../gettext.h):
```
_("My new string")
```
then run, in the current folder:  
```
make update
```
This will update existing .po files with new string to be translated.

You can now pass the .po files to translators or translate them yourself.
The translations are formatted like below in the .po file:
```
msgid "String to Translate"
msgstr "Translated string"
```

## Generating / Updating intl header files
Once you have updated the .po files and add the translations to them, run in the current folder:
```
make headers
```

**CAUTION:** when using this command, make shure all languages supported by the application are stored in the Makefile in the `LANGUAGES` variable
