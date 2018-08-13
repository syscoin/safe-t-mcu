#!/usr/bin/env python3
# Generate header files containing translations and intl.h
# Usage example: python3 enerate_translation_headers.py fr de
# English translation file is always generated as considered as genuine language
import re
import sys

intlDirectory = "../intl/"

warningMessage = [
    "/**\n",
    " * CAUTION !!\n",
    " * This file is generated directly by generate_translation_headers.h,\n",
    " * DO NOT MODIFY IT DIRECTLY !\n",
    " * To add/edit a translation, modify the .po related file"
    " **/\n\n"
];

def extractTranslation(language, original, translation):
    translationFile = open(language + ".po", "r")
    for line in translationFile:
        matchedLine = re.search(r'^(msgid|msgstr) (.*)', line)
        if matchedLine:
            if matchedLine.group(1) == "msgid":
                original.append(matchedLine.group(2))
            if matchedLine.group(1) == "msgstr":
                translation.append(matchedLine.group(2))
    translationFile.close

def checkLength(originalLength, translationLength):
    if originalLength != translationLength :
        print("Error: Lists don\'t have the same size")
        print("Original length: " + str(originalLength))
        print("translation length: " + str(translationLength))
        quit()

def getFileName(filePointer):
    return re.search(r".*/([^/]*)", filePointer.name).group(1)

def getFileVar(fileName):
    return fileName.replace('.','_').upper()

def writeBeginning(filePointer):
    fileName = getFileName(filePointer)
    fileVar =  getFileVar(fileName)
    filePointer.writelines(warningMessage)
    filePointer.writelines([
        "#ifndef " + fileVar +"\n",
        "#define " + fileVar +"\n",
        "\n",
        "#include <stdint.h>\n"
        "#include \"intl.h\"\n"
        "\n",
        "const char *" + re.search(r"(.*)\.h",fileName).group(1) + "_strings[STR_NUM] = {\n"
    ])

def writeEnd(filePointer):
    fileVar =  getFileVar(getFileName(filePointer))
    filePointer.writelines([
        "};\n",
        "\n",
        "#endif // !" + fileVar + "\n"
    ])

def writeIntlFile(languages, originalLength):
    intlFile = open(intlDirectory + "intl.h", "w+")
    fileVar = getFileVar(getFileName(intlFile))
    intlFile.writelines(warningMessage)
    intlFile.writelines([
        "#ifndef " + fileVar +"\n",
        "#define " + fileVar +"\n",
        "\n",
        "#define STR_NUM " + str(originalLength) + "\n", 
        "\n",
        "#include \"en.h\"\n"
    ])

    for language in languages:
        intlFile.write("#include \"" + language + ".h\"\n")

    intlFile.writelines([
        "\n",
        "#endif // !" + fileVar + "\n"
    ])
    intlFile.close

def writeTranslationFiles(language):
    original = []
    translation = []

    extractTranslation(language, original, translation)

    originalLength = len(original)
    translationLength = len(translation)

    checkLength(originalLength, translationLength)

    originalHeader = open(intlDirectory + "en.h", "w+")
    translationHeader = open(intlDirectory + language + ".h", "w+")

    writeBeginning(originalHeader)
    writeBeginning(translationHeader)

    # first element is empty (comment in the po file), starting loop to 1
    for x in range(1, originalLength):
        originalHeader.write("\t" + original[x] + ",\n")
        if translation[x] == "\"\"":
            translationHeader.write("\t" + original[x] + ",\n")
        else:
            translationHeader.write("\t" + translation[x] + ",\n")

    writeEnd(originalHeader)
    writeEnd(translationHeader)
    originalHeader.close
    translationHeader.close
    return originalLength-1

traductionNum = 0
languages = sys.argv[1:len(sys.argv)]

for language in languages:
    traductionNum = writeTranslationFiles(language)

writeIntlFile(languages, traductionNum)
