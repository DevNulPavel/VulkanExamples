#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import os.path
import getopt
import sys
import re

def convertTypeToSize(typeName):
    if typeName == "int":
        return 4
    elif typeName == "float":
        return 4
    elif typeName == "vec2":
        return 4*2
    elif typeName == "vec3":
        return 4*3
    elif typeName == "vec4":
        return 4*4
    elif typeName == "mat2":
        return 4*2*2
    elif typeName == "mat3":
        return 4*3*3
    elif typeName == "mat4":
        return 4*4*4
    else:
        print("Invalid constant buffer type: %s" % typeName)
        sys.exit(2)

def convertTypeToSortPriority(typeName):
    if typeName == "int":
        return 1
    elif typeName == "float":
        return 2
    elif typeName == "vec2":
        return 3
    elif typeName == "vec3":
        return 4
    elif typeName == "vec4":
        return 5
    elif typeName == "mat2":
        return 6
    elif typeName == "mat3":
        return 7
    elif typeName == "mat4":
        return 8
    else:
        print("Invalid constant buffer type: %s" % typeName)
        sys.exit(2)


def processShaderFile(isVertexShader, inputPath, outputPath, constantBufferOffset: int) -> int:
    with open(inputPath, "r") as file:
        inputFileText = file.read()

    # Табы заменяем на пробелы
    inputFileText = inputFileText.replace("\t", "    ")

    # Удаляем комментарии
    inputFileText = re.sub("\/\*.+\*\/", "", inputFileText, flags=0) #re.DOTALL
    inputFileText = re.sub("\/\*.+\*\/\n", "\n", inputFileText, flags=0)
    inputFileText = re.sub("\/\/.+\n", "\n", inputFileText, flags=0)

    # Удаляем странные дефайны
    inputFileText = inputFileText.replace("FLOAT", "float")

    # Удаляем пустые строки
    # inputFileText = re.sub("^\s*$", "", inputFileText, flags=re.MULTILINE)

    words = inputFileText.replace("\n", " ").split(" ")
    words = list(filter(lambda a: a != "", words))

    # Main function handle
    mainFunctionText = re.search("void main\s*\(void\)\s*{[\s\S=]+}", inputFileText, flags=re.MULTILINE)[0]

    # Remove precision words
    precisionWords = ["lowp", "mediump", "highp", "PRECISION_LOW", "PRECISION_MEDIUM", "PRECISION_HIGH", "PLATFORM_PRECISION"]
    for precisionWord in precisionWords:
        mainFunctionText = mainFunctionText.replace(precisionWord, "PRECISION")
    mainFunctionText = mainFunctionText.replace("PRECISION ", "")

    # Ignore defines
    ignoreDefinesList = ["float", "FLOAT", "VEC2"]

    # Lists
    definesNamesList = []
    definesMap = {}
    attributesNamesList = []
    attributesMap = {}
    uniformsNamesList = []
    uniformsMap = {}
    varyingsNamesList = []
    varyingsMap = {}
    samplersNamesList = []
    samplersMap = {}

    # Обходим слова и ищем аттрибуты
    attributeIndex = 0
    varyingIndex = 0
    samplerIndex = 0
    i = 0
    while i < len(words):
        if words[i] == "#define":
            # Получаем тип аттрибута
            i += 1
            defineName = words[i]

            if (defineName in mainFunctionText) and (defineName not in ignoreDefinesList):
                # Получаем значение
                i += 1
                defineValue = words[i]

                # Добавляем аттрибут к тексту нового шейдера
                if defineName not in definesNamesList:
                    newShaderDefineName = "#define %s %s\n" % (defineName, defineValue)
                    definesMap[defineName] = newShaderDefineName
                    definesNamesList.append(defineName)

        if words[i] == "attribute":
            # TODO: ???
            # Если после attribute идет описание точности - пропускаем его
            i += 1
            if words[i] in precisionWords:
                i += 1

            # Получаем тип аттрибута
            attributeType = words[i]

            # Получаем имя аттрибута
            i += 1
            attributeName = words[i].replace(";", "")

            # Добавляем аттрибут к тексту нового шейдера
            if attributeName not in attributesMap:
                newShaderVariableName = "layout(location = %d) in %s %s;\n" % (attributeIndex, attributeType, attributeName)
                attributesMap[attributeName] = newShaderVariableName
                attributesNamesList.append(attributeName)
                attributeIndex += 1

        if words[i] == "uniform":
            # TODO: ???
            # Если после uniform идет описание точности - пропускаем его
            i += 1
            if words[i] in precisionWords:
                i += 1

            # Если после uniform идет sampler2D - не обрабоатываем
            if words[i] != "sampler2D":
                # Получаем тип аттрибута
                uniformType = words[i]

                # Получаем имя аттрибута
                i += 1
                uniformName = words[i].replace(";", "")

                # Добавляем аттрибут к тексту нового шейдера
                if (uniformName not in uniformsMap):
                    if uniformName in mainFunctionText:
                        uniformsMap[uniformName] = {"type": uniformType, "name": uniformName}
                        uniformsNamesList.append(uniformName)
                    else:
                        print("Unused uniform variable %s in shader %s" % (uniformName, inputPath))


            else:
                # Получаем имя семплера
                i += 1
                samplerName = words[i].replace(";", "")

                # Добавляем аттрибут к тексту нового шейдера
                if samplerName not in samplersMap:
                    newShaderVariableName = "layout(set = %s, binding = 0) uniform sampler2D %s;\n" % (samplerIndex, samplerName)
                    samplersMap[samplerName] = newShaderVariableName
                    samplersNamesList.append(samplerName)
                    samplerIndex += 1

        if words[i] == "varying":
            # TODO: ???
            # Если после varying идет описание точности - пропускаем его
            i += 1
            if words[i] in precisionWords:
                i += 1

            # Получаем тип аттрибута
            varyingType = words[i]

            # Получаем имя аттрибута
            i += 1
            varyingName = words[i].replace(";", "")

            # Может быть у нас массив varying переменных
            if "[" in varyingName:
                varyingMatches = re.search("([a-zA-Z_]+)\[([0-9]+)\]", varyingName)
                if varyingMatches:
                    name = varyingMatches.group(1)
                    count = int(varyingMatches.group(2))

                    for index in range(0, count):
                        varyingCountName = "%s_%d" % (name, index)

                        # Добавляем аттрибут к тексту нового шейдера
                        if varyingCountName not in varyingsMap:
                            if isVertexShader:
                                newShaderVariableName = "layout(location = %d) out %s %s;\n" % (varyingIndex, varyingType, varyingCountName)
                            else:
                                newShaderVariableName = "layout(location = %d) in %s %s;\n" % (varyingIndex, varyingType, varyingCountName)
                            varyingsMap[varyingCountName] = newShaderVariableName
                            varyingsNamesList.append(varyingCountName)
                            varyingIndex += 1

                            # Замена в тексте
                            expression = "%s\[[ ]*%d[ ]*\]" % (name, index)
                            mainFunctionText = re.sub(expression, varyingCountName, mainFunctionText)
            else:
                # Добавляем аттрибут к тексту нового шейдера
                if varyingName not in varyingsMap:
                    if isVertexShader:
                        newShaderVariableName = "layout(location = %d) out %s %s;\n" % (varyingIndex, varyingType, varyingName)
                    else:
                        newShaderVariableName = "layout(location = %d) in %s %s;\n" % (varyingIndex, varyingType, varyingName)
                    varyingsMap[varyingName] = newShaderVariableName
                    varyingsNamesList.append(varyingName)
                    varyingIndex += 1
        i += 1

    # Result shader header
    resultShaderText = "#version 450\n\n"
                        #"#extension GL_ARB_separate_shader_objects : enable\n\n"

    if isVertexShader:
        resultShaderText += "// Vertex shader\n\n"

        # Defines
        if len(definesNamesList) > 0:
            resultShaderText += "// Defines\n"
            for defineName in definesNamesList:
                resultShaderText += definesMap[defineName]
            resultShaderText += "\n"

        # Attributes
        if len(attributesNamesList) > 0:
            resultShaderText += "// Input\n"
            for attributeString in attributesNamesList:
                resultShaderText += attributesMap[attributeString]
            resultShaderText += "\n"

        # Uniforms
        if len(uniformsNamesList) > 0:
            pushConstantsText = ""

            # Сортируем по убыванию размера
            def sortFunction(uniformName):
                return convertTypeToSortPriority(uniformsMap[uniformName]["type"])

            uniformsNamesList = sorted(uniformsNamesList, key=sortFunction, reverse=True)

            for uniformName in uniformsNamesList:
                uniformDict = uniformsMap[uniformName]
                newShaderVariableName = "    layout(offset = %d) %s %s;\n" % (constantBufferOffset, uniformDict["type"], uniformDict["name"])
                constantBufferOffset += convertTypeToSize(uniformDict["type"])
                # Only used uniforms
                pushConstantsText += newShaderVariableName

            if len(pushConstantsText) > 0:
                resultShaderText += "// Push constants\n" \
                                    "layout(push_constant) uniform PushConstants {\n"
                resultShaderText += pushConstantsText
                resultShaderText += "} pc;\n\n"  # TODO: ???

        # Varying
        if len(varyingsNamesList) > 0:
            resultShaderText += "// Varying variables\n"
            for varyingName in varyingsNamesList:
                resultShaderText += varyingsMap[varyingName]
            resultShaderText += "\n"

        # Выходные переменные
        resultShaderText += "// Vertex output\n" \
                            "out gl_PerVertex {\n" \
                            "    vec4 gl_Position;\n" \
                            "};\n"
    else:
        resultShaderText += "// Fragment shader\n\n"

        # Defines
        if len(definesNamesList) > 0:
            resultShaderText += "// Defines\n"
            for defineName in definesNamesList:
                resultShaderText += definesMap[defineName]
            resultShaderText += "\n"

        # Varying
        if len(varyingsNamesList) > 0:
            resultShaderText += "// Varying variables\n"
            for varyingName in varyingsNamesList:
                resultShaderText += varyingsMap[varyingName]
            resultShaderText += "\n"

        # Samplers
        if len(samplersNamesList) > 0:
            resultShaderText += "// Samplers\n"
            for samplerName in samplersNamesList:
                resultShaderText += samplersMap[samplerName]
            resultShaderText += "\n"

        # Uniforms
        if len(uniformsNamesList) > 0:
            pushConstantsText = ""

            # Сортируем по убыванию размера
            def sortFunction(uniformName):
                return convertTypeToSortPriority(uniformsMap[uniformName]["type"])

            uniformsNamesList = sorted(uniformsNamesList, key=sortFunction, reverse=True)

            for uniformName in uniformsNamesList:
                uniformDict = uniformsMap[uniformName]
                newShaderVariableName = "    layout(offset = %d) %s %s;\n" % (constantBufferOffset, uniformDict["type"], uniformDict["name"])
                constantBufferOffset += convertTypeToSize(uniformDict["type"])
                # Only used uniforms
                pushConstantsText += newShaderVariableName

            if len(pushConstantsText) > 0:
                resultShaderText += "// Push constants\n" \
                                    "layout(push_constant) uniform PushConstants {\n"
                resultShaderText += pushConstantsText
                resultShaderText += "} pc;\n\n"  # TODO: ???

        # Выходные переменные
        resultShaderText += "// Fragment output\n" \
                            "layout(location = 0) out vec4 outputFragColor;\n"


    functionDeclaration = re.search("void main\s*\(void\)\s*{", mainFunctionText, flags=0)[0]

    # Function declaration replace
    mainFunctionText = mainFunctionText.replace(functionDeclaration, "void main(void) {")

    # Replace uniforms on push constants
    for uniformName in uniformsNamesList:
        #expression = "[^a-zA-Z_](%s)[^a-zA-Z_]" % uniformName
        expression = r"[\+\-\ * \ /(<>=](%s)[\+\-\ * \ /, ;.\[)<>=]" % uniformName
        replaceValue = "pc.%s" % uniformName

        matches = re.search(expression, mainFunctionText)

        while matches:
            for groupNum in range(0, len(matches.groups())):
                groupNum = groupNum + 1
                start = matches.start(groupNum)
                end = matches.end(groupNum)
                # group = matches.group(groupNum)
                mainFunctionText = mainFunctionText[0:start] + replaceValue + mainFunctionText[end:]
            matches = re.search(expression, mainFunctionText)

        # mainFunctionText = re.sub(expression, mainFunctionText, replaceValue)
        # mainFunctionText = mainFunctionText.replace(uniformName, "pc."+uniformName)

    # Fragment out variable
    if isVertexShader == False:
        mainFunctionText = mainFunctionText.replace("texture2D", "texture")
        mainFunctionText = mainFunctionText.replace("gl_FragColor", "outputFragColor")

    resultShaderText += "\n// Main function\n"
    resultShaderText += mainFunctionText

    # Сохраняем
    with open(outputPath, "w") as file:
        file.write(resultShaderText)

    # Выравнивание
    if (constantBufferOffset % 16) != 0:
        constantBufferOffset += 16
        constantBufferOffset -= constantBufferOffset % 16
    return constantBufferOffset


def processShadersFolder(inputPath, outputPath):
    for root, dirs, files in os.walk(inputPath, topdown=True):
        for fileName in files:
            if (not fileName.startswith(".")) and (".psh" in fileName):
                resultFolder = root.replace(inputPath, outputPath)

                sourceFragmentFilePath = os.path.join(root, fileName)
                sourceVertexFilePath = sourceFragmentFilePath.replace(".psh", ".vsh")
                resultFragmentFilePath = os.path.join(resultFolder, fileName).replace(".psh", ".frag")
                resultVertexFilePath = os.path.join(resultFolder, fileName).replace(".psh", ".vert")

                # Проверка налиция обоих файлов
                if not os.path.exists(sourceVertexFilePath) or not os.path.exists(sourceFragmentFilePath):
                    print("Missing shaders %s + %s" % (sourceVertexFilePath, sourceFragmentFilePath))
                    sys.exit(2)

                # Обработка шейдеров
                constantBufferSize = 0
                constantBufferSize += processShaderFile(True, sourceVertexFilePath, resultVertexFilePath, constantBufferSize)
                constantBufferSize += processShaderFile(False, sourceFragmentFilePath, resultFragmentFilePath, constantBufferSize)

if __name__ == '__main__':
    # Params
    exampleString = "main.py -i <input files folder> -o <output files folder>"
    try:
        opts, args = getopt.getopt(sys.argv[1:], "i:o:", ["ifolder=", "ofolder="])
    except getopt.GetoptError:
        print(exampleString)
        sys.exit(2)

    # Check parameters length
    if len(opts) < 2:
        print(exampleString)
        sys.exit(2)

    # Parse parameters
    inputFolder = ''
    outputFolder = ''
    for opt, arg in opts:
        if opt in ("-i", "--ifolder"):
            inputFolder = arg
        elif opt in ("-o", "--ofolder"):
            outputFolder = arg

    # Process config
    if inputFolder and outputFolder:
        # Resources processing
        processShadersFolder(inputFolder, outputFolder)