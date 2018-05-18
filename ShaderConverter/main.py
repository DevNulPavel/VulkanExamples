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


def analyseUniformsSize(filePath) -> int:
    with open(filePath, "r") as file:
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

    testUniformsNamesList = []

    # Обходим слова и ищем аттрибуты
    uniformsSize = 0
    i = 0
    while i < len(words):
        if words[i] == "uniform":
            # TODO: ???
            # Если после uniform идет описание точности - пропускаем его
            i += 1
            if words[i] in precisionWords:
                i += 1

            # Если после uniform идет sampler2D - не обрабоатываем
            if words[i] != "sampler2D":
                # Получаем тип аттрибута
                testUniformType = words[i]

                # Получаем имя аттрибута
                i += 1
                testUniformName = words[i].replace(";", "")

                if (testUniformName not in testUniformsNamesList) and (testUniformName in mainFunctionText):
                    uniformsSize += convertTypeToSize(testUniformType)
                    testUniformsNamesList.append(testUniformName)
        i += 1

    return uniformsSize


def processShaderFile(isVertexShader, inputPath, outputPath, setIndex, inputVaryingLocations, previousStageUniforms, pushConstantOffset, usePushConstant) -> (int, str, int, dict):
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
    resultVaryingLocations = {}

    # Обходим слова и ищем аттрибуты
    attributeIndex = 0
    varyingIndex = 0
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
                if samplerName not in samplersNamesList:
                    samplersNamesList.append(samplerName)

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

                    if name in mainFunctionText:
                        for index in range(0, count):
                            varyingCountName = "%s_%d" % (name, index)

                            # Добавляем аттрибут к тексту нового шейдера
                            if varyingCountName not in varyingsMap:
                                if isVertexShader:
                                    resultVaryingLocations[varyingCountName] = varyingIndex
                                    newShaderVariableName = "layout(location = %d) out %s %s;\n" % (varyingIndex, varyingType, varyingCountName)
                                else:
                                    if varyingCountName in inputVaryingLocations:
                                        inputIndex = inputVaryingLocations[varyingCountName]
                                        newShaderVariableName = "layout(location = %d) in %s %s;\n" % (inputIndex, varyingType, varyingCountName)
                                    else:
                                        print("Is not compatible varyings for %s with vertex shader" % inputPath)
                                        sys.exit(2)

                                varyingsMap[varyingCountName] = newShaderVariableName
                                varyingsNamesList.append(varyingCountName)
                                varyingIndex += 1

                                # Замена в тексте
                                expression = "%s\[[ ]*%d[ ]*\]" % (name, index)
                                mainFunctionText = re.sub(expression, varyingCountName, mainFunctionText)
            else:
                # Добавляем аттрибут к тексту нового шейдера
                if (varyingName not in varyingsMap) and (varyingName in mainFunctionText):
                    newShaderVariableName = ""
                    if isVertexShader:
                        resultVaryingLocations[varyingName] = varyingIndex
                        newShaderVariableName = "layout(location = %d) out %s %s;\n" % (varyingIndex, varyingType, varyingName)
                    else:
                        if varyingName in inputVaryingLocations:
                            inputIndex = inputVaryingLocations[varyingName]
                            newShaderVariableName = "layout(location = %d) in %s %s;\n" % (inputIndex, varyingType, varyingName)
                        else:
                            print("Is not compatible varyings for %s with vertex shader" % inputPath)
                            sys.exit(2)

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

            if usePushConstant:
                for uniformName in uniformsNamesList:
                    uniformDict = uniformsMap[uniformName]
                    newShaderVariableName = "    layout(offset = %d) %s %s;\n" % (pushConstantOffset, uniformDict["type"], uniformDict["name"])
                    pushConstantOffset += convertTypeToSize(uniformDict["type"])
                    # Only used uniforms
                    pushConstantsText += newShaderVariableName

                if len(pushConstantsText) > 0:
                    resultShaderText += "// Push constants\n" \
                                        "layout(push_constant) uniform PushConstants {\n"
                    resultShaderText += pushConstantsText
                    resultShaderText += "} uni;\n\n"  # TODO: ???
            else:
                for uniformName in uniformsNamesList:
                    uniformDict = uniformsMap[uniformName]
                    newShaderVariableName = "    %s %s;\n" % (uniformDict["type"], uniformDict["name"])
                    pushConstantsText += newShaderVariableName

                if len(pushConstantsText) > 0:
                    previousStageUniforms += pushConstantsText

                    resultShaderText += "// Uniform buffer\n" \
                                        "layout(set = 0, binding = 0) uniform UniformBufferObject {\n"
                    resultShaderText += pushConstantsText
                    resultShaderText += "} uni;\n\n"  # TODO: ???

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

        # Uniforms
        if len(uniformsNamesList) > 0:
            pushConstantsText = ""

            # Сортируем по убыванию размера
            def sortFunction(uniformName):
                return convertTypeToSortPriority(uniformsMap[uniformName]["type"])

            uniformsNamesList = sorted(uniformsNamesList, key=sortFunction, reverse=True)

            if usePushConstant:
                for uniformName in uniformsNamesList:
                    uniformDict = uniformsMap[uniformName]
                    newShaderVariableName = "    layout(offset = %d) %s %s;\n" % (pushConstantOffset, uniformDict["type"], uniformDict["name"])
                    pushConstantOffset += convertTypeToSize(uniformDict["type"])
                    pushConstantsText += newShaderVariableName

                if len(pushConstantsText) > 0:
                    resultShaderText += "// Push constants\n" \
                                        "layout(push_constant) uniform PushConstants {\n"
                    resultShaderText += pushConstantsText
                    resultShaderText += "} uni;\n\n"  # TODO: ???
            else:
                for uniformName in uniformsNamesList:
                    uniformDict = uniformsMap[uniformName]
                    newShaderVariableName = "    %s %s;\n" % (uniformDict["type"], uniformDict["name"])
                    pushConstantsText += newShaderVariableName

                if len(pushConstantsText) > 0:
                    resultShaderText += "// Uniform buffer\n" \
                                        "layout(set = 0, binding = 0) uniform UniformBufferObject {\n"
                    resultShaderText += previousStageUniforms
                    resultShaderText += pushConstantsText
                    resultShaderText += "} uni;\n\n"  # TODO: ???
                    setIndex += 1

        # Samplers
        if len(samplersNamesList) > 0:
            resultShaderText += "// Samplers\n"
            for samplerName in samplersNamesList:
                resultShaderText += "layout(set = %s, binding = 0) uniform sampler2D %s;\n" % (setIndex, samplerName)
                setIndex += 1
            resultShaderText += "\n"

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
        replaceValue = "uni.%s" % uniformName

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
    if (pushConstantOffset % 16) != 0:
        pushConstantOffset += 16
        pushConstantOffset -= pushConstantOffset % 16

    return setIndex, previousStageUniforms, pushConstantOffset, resultVaryingLocations


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

                vertexUniformsSize = analyseUniformsSize(sourceVertexFilePath)
                fragmentUniformsSize = analyseUniformsSize(sourceFragmentFilePath)
                totalUniformsSize = vertexUniformsSize + fragmentUniformsSize

                usePushConstants = False
                if totalUniformsSize <= 128:
                    usePushConstants = True

                # Обработка шейдеров
                setIndex, previousStageUniforms, pushConstantOffset, varyingLocations = processShaderFile(True, sourceVertexFilePath, resultVertexFilePath, 0, {}, "", 0, usePushConstants)
                processShaderFile(False, sourceFragmentFilePath, resultFragmentFilePath, setIndex, varyingLocations, previousStageUniforms, pushConstantOffset, usePushConstants)


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