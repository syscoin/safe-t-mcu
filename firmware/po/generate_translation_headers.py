#!/usr/bin/env python3
# Generate header files containing translations and intl.h
# Usage example: python3 enerate_translation_headers.py fr de
# English translation file is always generated as considered as genuine language
import re
import sys

intlDirectory = "../intl/"

poFilesDirectory = "./"

fontDirectory = "../../gen/"

fontCharWidth = [0]*256

translations_fit = True;

# 128 wide OLED, icon needs 16 pixels + 4 pixels space
PIXEL_LINE_WIDTH=128-20

warningMessage = [
    "/**\n",
    " * CAUTION !!\n",
    " * This file is generated directly by generate_translation_headers.h,\n",
    " * DO NOT MODIFY IT DIRECTLY !\n",
    " * To add/edit a translation, modify the .po related file\n"
    " **/\n\n"
];

def extractTranslation(language, original, translation):
    global translations_fit
    translationFile = open(poFilesDirectory + language + ".po", "r", encoding='utf-8')
    line = translationFile.readline()
    commentLineBefore = False
    commentLineNow = False
    DisplayLines=0
    while line:
        commentLineBefore = commentLineNow
        commentLineNow = bool(re.search(r'^#.*', line))
        if commentLineNow:
            matchDisplayStr = re.search(r'^#\. DISPLAY.*: (.*) line.*', line)
            if matchDisplayStr:
                DisplayLines = int(matchDisplayStr.group(1))
        matchMsgid = re.search(r'^msgid "(.*)"', line)
        if matchMsgid:
            string = matchMsgid.group(1)
            line = translationFile.readline()
            matchMsgstr = re.search(r'^msgstr "(.*)"', line)
            while not matchMsgstr:
                string += re.search(r'"(.*)"', line).group(1)
                line = translationFile.readline()
                matchMsgstr = re.search(r'^msgstr "(.*)"', line)
            original.append('"' + string + '"')
            string = matchMsgstr.group(1)
            line = translationFile.readline()
            while(line != '\n' and line != ''):
                string += re.search(r'"(.*)"', line).group(1)
                line = translationFile.readline()
            translation.append('"' + string + '"')
            if commentLineBefore and DisplayLines > 0:
                if (checkWidth(DisplayLines, string)):
                    print("Max. " + str(DisplayLines) + " lines")
                    print("\"" + string + "\" too long!!")
                    translations_fit = False

            DisplayLines = 0
        line = translationFile.readline()
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
    translationHeader = open(intlDirectory + language + ".h", "w+", encoding='utf-8')

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

def readFontMetric(fontFilename):
    fontIncFile = open(fontFilename, "r")
    line = fontIncFile.readline()
    while line:
        matchFontline = re.search(r'^.*/* 0x(..) . \*/ \(uint8_t \*\)"\\x(..)\\x.*', line)
        if matchFontline:
            fontCharWidth[int(matchFontline.group(1), 16)] = int(matchFontline.group(2), 16)
        line = fontIncFile.readline()
    fontIncFile.close
def checkWidth(maxLines, string):
    wordPixelLen = 0
    linePixelLen = 0
    lineIndex = 0
    for i in range (0, len(string)):
        charWidth = fontCharWidth[ord(string[i])] + 1
        if ((string[i] == 'n' and i>0 and string[i-1] == '\\') or string[i] == '\0'):
            lineIndex += 1
            linePixelLen = 0
            wordPixelLen = 0
        elif (string[i] == ' '):
            if(linePixelLen + wordPixelLen <= PIXEL_LINE_WIDTH):
                linePixelLen += wordPixelLen + charWidth
            else:
               lineIndex += 1
               linePixelLen = wordPixelLen + charWidth
            wordPixelLen = 0
        wordPixelLen += charWidth

    return (lineIndex + 1 > maxLines)

readFontMetric(fontDirectory + "font.inc")

traductionNum = 0
languages = sys.argv[1:len(sys.argv)]

for language in languages:
    traductionNum = writeTranslationFiles(language)
if (translations_fit == False):
    print("does not fit\n")
    sys.exit(1)

writeIntlFile(languages, traductionNum)
