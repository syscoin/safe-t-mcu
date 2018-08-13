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

rm = rm -rf

%.pot: ../%.c
	xgettext --keyword=_ --language=C --add-comments --sort-output -o $@ $<

%po: %pot
	msginit --no-translator --input=$< --locale=$(LOCALE) -o $@

fr.pot: $(POT_SOURCES)
	msgcat $^ -o $@

de.pot: $(POT_SOURCES)
	msgcat $^ -o $@

# Be shure to have the locale installed on your computer (locale -a to check)
init:
	$(MAKE) LOCALE=fr_FR.UTF-8 fr.po
	$(MAKE) LOCALE=de_DE.utf8 de.po
	$(MAKE) clean

update:
	$(MAKE) fr.pot de.pot
	msgmerge fr.po fr.pot --update
	msgmerge de.po de.pot --update

clean:
	@$(rm) $(POT_SOURCES) fr.pot de.pot