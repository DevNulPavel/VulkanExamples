#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import os.path
import getopt
import sys
import re


def processShaderFile(inputPath, outputPath):
    with open(inputPath, "r") as file:
        inputFileText = file.read()

    # Табы заменяем на пробелы
    inputFileText = inputFileText.replace("\t", "    ")

    # Удаляем комментарии
    inputFileText = re.sub("\/\*.+\*\/", "", inputFileText, flags=0) #re.DOTALL
    inputFileText = re.sub("\/\*.+\*\/\n", "\n", inputFileText, flags=0)
    inputFileText = re.sub("\/\/.+\n", "\n", inputFileText, flags=0)

    # Удаляем пустые строки
    # inputFileText = re.sub("^\s*$", "", inputFileText, flags=re.MULTILINE)

    words = inputFileText.replace("\n", " ").split(" ")

    attributesList = []
    uniformsList = []
    varyingsList = []
    samplersList = []

    # Обходим слова и ищем аттрибуты
    attributeIndex = 0
    varyingIndex = 0
    samplerIndex = 0
    i = 0
    while i < len(words):
        if words[i] == "attribute":
            # TODO: ???
            # Если после attribute идет описание точности - пропускаем его
            i += 1
            if words[i] in ["lowp", "mediump", "highp", "PRECISION_LOW", "PRECISION_MEDIUM", "PRECISION_HIGH", "PLATFORM_PRECISION"]:
                i += 1

            # Получаем тип аттрибута
            attributeType = words[i]

            # Получаем имя аттрибута
            i += 1
            attributeName = words[i]

            # Добавляем аттрибут к тексту нового шейдера
            newShaderVariableName = "layout(location = %d) in %s %s\n" % (attributeIndex, attributeType, attributeName)
            attributesList.append(newShaderVariableName)
            attributeIndex += 1

        if words[i] == "uniform":
            # TODO: ???
            # Если после uniform идет описание точности - пропускаем его
            i += 1
            if words[i] in ["lowp", "mediump", "highp", "PRECISION_LOW", "PRECISION_MEDIUM", "PRECISION_HIGH", "PLATFORM_PRECISION"]:
                i += 1

            # Если после uniform идет sampler2D - не обрабоатываем
            if words[i] != "sampler2D":
                # Получаем тип аттрибута
                uniformType = words[i]

                # Получаем имя аттрибута
                i += 1
                uniformName = words[i]

                # Добавляем аттрибут к тексту нового шейдера
                newShaderVariableName = "    %s %s\n" % (uniformType, uniformName)
                uniformsList.append(newShaderVariableName)
            else:
                # Получаем имя семплера
                i += 1
                samplerName = words[i]

                # Добавляем аттрибут к тексту нового шейдера
                newShaderVariableName = "layout(set = %s, binding = 0) uniform sampler2D %s\n" % (samplerIndex, samplerName)
                samplersList.append(newShaderVariableName)
                samplerIndex += 1


        if words[i] == "varying":
            # TODO: ???
            # Если после varying идет описание точности - пропускаем его
            i += 1
            if words[i] in ["lowp", "mediump", "highp", "PRECISION_LOW", "PRECISION_MEDIUM", "PRECISION_HIGH", "PLATFORM_PRECISION"]:
                i += 1

            # Получаем тип аттрибута
            varyingType = words[i]

            # Получаем имя аттрибута
            i += 1
            varyingName = words[i]

            # Добавляем аттрибут к тексту нового шейдера
            newShaderVariableName = "layout(location = %d) out %s %s\n" % (varyingIndex, varyingType, varyingName)
            varyingsList.append(newShaderVariableName)
            varyingIndex += 1

        i += 1

    # Result shader header
    resultShaderText = "#version 450\n" \
                       "#extension GL_ARB_separate_shader_objects : enable\n\n"

    # Attributes
    if len(attributesList) > 0:
        resultShaderText += "// Input\n"
        for attributeString in attributesList:
            resultShaderText += attributeString
        resultShaderText += "\n"

    # Uniforms
    if len(uniformsList) > 0:
        resultShaderText += "// Push constants\n" \
                            "layout(push_constant) uniform PushConstants {\n"
        for uniformName in uniformsList:
            resultShaderText += uniformName
        resultShaderText += "} pc;\n\n"   # TODO: ???

    # Samplers
    if len(samplersList) > 0:
        resultShaderText += "// Samplers\n"
        for samplerName in samplersList:
            resultShaderText += samplerName
        resultShaderText += "\n"

    # Varying
    if len(varyingsList) > 0:
        resultShaderText += "// Out variables\n"
        for varyingName in varyingsList:
            resultShaderText += varyingName
        resultShaderText += "\n"


    print(resultShaderText)






def processShadersFolder(inputPath, outputPath):
    for root, dirs, files in os.walk(inputPath, topdown=True):
        for fileName in files:
            if (".vsh" in fileName) or (".psh" in fileName):
                resultFolder = root.replace(inputPath, outputPath)

                sourceFilePath = os.path.join(root, fileName)
                resultFilePath = os.path.join(resultFolder, fileName)

                processShaderFile(sourceFilePath, resultFilePath)


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