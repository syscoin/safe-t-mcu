PYTHON = python3

POT_SOURCES += bl_check.pot
POT_SOURCES += trezor.pot
POT_SOURCES += protect.pot
POT_SOURCES += u2f.pot
POT_SOURCES += signing.pot
POT_SOURCES += nem2.pot
POT_SOURCES += reset.pot
POT_SOURCES += storage.pot
POT_SOURCES += fsm.pot
POT_SOURCES += messages.pot
POT_SOURCES += layout2.pot
POT_SOURCES += ethereum.pot
POT_SOURCES += recovery.pot

LANGUAGES = fr de

LINE_WIDTH = 100
MSGCAT = msgcat -w $(LINE_WIDTH) --sort-by-file
rm = rm -rf

%.pot: ../%.c .FORCE
	xgettext -w $(LINE_WIDTH) --keyword=_ -c// --language=C --add-comments -o $@ $<

%po: %pot
	msginit -w $(LINE_WIDTH) --no-translator --input=$< --locale=$(LOCALE) -o $@

fr.pot de.pot: $(POT_SOURCES)
	$(MSGCAT) $^ -o $@

# Be shure to have the locale installed on your computer (locale -a to check)
init:
	$(MAKE) LOCALE=fr_FR.UTF-8 fr.po
	$(MAKE) LOCALE=de_DE.utf8 de.po
	$(MAKE) clean

update:
	$(foreach language, $(LANGUAGES), $(MAKE) $(language).pot; msgmerge $(language).po $(language).pot --update --sort-by-file -w $(LINE_WIDTH);)

headers:
	$(PYTHON) generate_translation_headers.py $(LANGUAGES)

clean:
	@$(rm) $(POT_SOURCES) fr.pot de.pot

.PHONY: clean

.FORCE: