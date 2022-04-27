//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include "engine.h"
#include "buffer_management.h"
#include "assimp_model_loading.h"
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>

GLuint CreateProgramFromSource(String programSource, const char* shaderName)
{
    GLchar  infoLogBuffer[1024] = {};
    GLsizei infoLogBufferSize = sizeof(infoLogBuffer);
    GLsizei infoLogSize;
    GLint   success;

    char versionString[] = "#version 430\n";
    char shaderNameDefine[128];
    sprintf(shaderNameDefine, "#define %s\n", shaderName);
    char vertexShaderDefine[] = "#define VERTEX\n";
    char fragmentShaderDefine[] = "#define FRAGMENT\n";

    const GLchar* vertexShaderSource[] = {
        versionString,
        shaderNameDefine,
        vertexShaderDefine,
        programSource.str
    };
    const GLint vertexShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(vertexShaderDefine),
        (GLint) programSource.len
    };
    const GLchar* fragmentShaderSource[] = {
        versionString,
        shaderNameDefine,
        fragmentShaderDefine,
        programSource.str
    };
    const GLint fragmentShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(fragmentShaderDefine),
        (GLint) programSource.len
    };

    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, ARRAY_COUNT(vertexShaderSource), vertexShaderSource, vertexShaderLengths);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with vertex shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, ARRAY_COUNT(fragmentShaderSource), fragmentShaderSource, fragmentShaderLengths);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with fragment shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint programHandle = glCreateProgram();
    glAttachShader(programHandle, vshader);
    glAttachShader(programHandle, fshader);
    glLinkProgram(programHandle);
    glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programHandle, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glLinkProgram() failed with program %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    glUseProgram(0);

    glDetachShader(programHandle, vshader);
    glDetachShader(programHandle, fshader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);

    return programHandle;
}

u32 LoadProgram(App* app, const char* filepath, const char* programName)
{
    String programSource = ReadTextFile(filepath);

    Program program = {};
    program.handle = CreateProgramFromSource(programSource, programName);
    program.filepath = filepath;
    program.programName = programName;
    program.lastWriteTimestamp = GetFileLastWriteTimestamp(filepath);
    app->programs.push_back(program);

    return app->programs.size() - 1;
}

Image LoadImage(const char* filename)
{
    Image img = {};
    stbi_set_flip_vertically_on_load(true);
    img.pixels = stbi_load(filename, &img.size.x, &img.size.y, &img.nchannels, 0);
    if (img.pixels)
    {
        img.stride = img.size.x * img.nchannels;
    }
    else
    {
        ELOG("Could not open file %s", filename);
    }
    return img;
}

void FreeImage(Image image)
{
    stbi_image_free(image.pixels);
}

GLuint CreateTexture2DFromImage(Image image)
{
    GLenum internalFormat = GL_RGB8;
    GLenum dataFormat     = GL_RGB;
    GLenum dataType       = GL_UNSIGNED_BYTE;

    switch (image.nchannels)
    {
        case 3: dataFormat = GL_RGB; internalFormat = GL_RGB8; break;
        case 4: dataFormat = GL_RGBA; internalFormat = GL_RGBA8; break;
        default: ELOG("LoadTexture2D() - Unsupported number of channels");
    }

    GLuint texHandle;
    glGenTextures(1, &texHandle);
    glBindTexture(GL_TEXTURE_2D, texHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.size.x, image.size.y, 0, dataFormat, dataType, image.pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texHandle;
}

u32 LoadTexture2D(App* app, const char* filepath)
{
    for (u32 texIdx = 0; texIdx < app->textures.size(); ++texIdx)
        if (app->textures[texIdx].filepath == filepath)
            return texIdx;

    Image image = LoadImage(filepath);

    if (image.pixels)
    {
        Texture tex = {};
        tex.handle = CreateTexture2DFromImage(image);
        tex.filepath = filepath;

        u32 texIdx = app->textures.size();
        app->textures.push_back(tex);

        FreeImage(image);
        return texIdx;
    }
    else
    {
        return UINT32_MAX;
    }
}

void LoadTexturesQuad(App* app)
{
    const float vertices[] = {
    -1,-1,0.0,0.0,0.0,
    1,-1,0.0,1.0,0.0,
    1,1,0.0,1.0,1.0,
    -1,1,0.0,0.0,1.0
    };

    const unsigned short indices[] = {
        0,1,2,0,2,3
    };

    glGenBuffers(1, &app->embeddedVertices);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    int a = sizeof(vertices);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    //glBindBuffer(GL_ARRAY_BUFFER,0);

    glGenBuffers(1, &app->embeddedElements);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glGenVertexArrays(1, &app->vao);
    glBindVertexArray(app->vao);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    glVertexAttribPointer(0,3, GL_FLOAT, GL_FALSE,sizeof(float)*5,(void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(float)*5,(void*)12);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    glBindVertexArray(0);

    app->texturedGeometryProgramIdx = LoadProgram(app, "shaders.glsl", "TEXTURED_GEOMETRY");
    Program& texturedGeometryProgram = app->programs.back();
    app->programUniformTexture = glGetUniformLocation(texturedGeometryProgram.handle, "uTexture");

    app->diceTexIdx = LoadTexture2D(app, "dice.png");
    app->whiteTexIdx = LoadTexture2D(app, "color_white.png");
    app->blackTexIdx = LoadTexture2D(app, "color_black.png");
    app->normalTexIdx = LoadTexture2D(app, "color_normal.png");
    app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");

    //app->mode = Mode_TexturedQuad;
}

void LoadPatrik(App* app)
{
    u32 modelAppIdx = LoadModel(app, "Patrick/Patrick.obj");
    app->patrickProgramIdx = LoadProgram(app, "shaders2.glsl", "TEXTURED_PATRICE");

    Program& program = app->programs[app->patrickProgramIdx];
    app->patrickProgramUniform = glGetUniformLocation(program.handle, "uTexture");


    int maxVariableNameLength;
    glGetProgramiv(program.handle, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxVariableNameLength);
    char* attributeName = new char[maxVariableNameLength];
    int attributeNameLength;
    int attributeSize;
    int attributteByteSize;
    int usableAttributeSize;
    GLenum usableAttributeType;
    GLenum attributeType;
    int attributeCount;
    int offset = 0;
    int attributeLocation;
    glGetProgramiv(program.handle, GL_ACTIVE_ATTRIBUTES, &attributeCount);
    for (int i = 0; i < attributeCount; ++i) {
        glGetActiveAttrib(program.handle, i, /*ARRAY_COUNT(attributeName)*/maxVariableNameLength,
            &attributeNameLength,
            &attributeSize,
            &attributeType,
            attributeName);

        attributeLocation = glGetAttribLocation(program.handle, attributeName);

        //careful with the stride!!!!
        switch (attributeType)
        {
        case GL_FLOAT:
            attributteByteSize = sizeof(float) * attributeSize;
            usableAttributeType = GL_FLOAT;
            usableAttributeSize = 1;
            break;
        case GL_FLOAT_VEC2:
            attributteByteSize = sizeof(float) * 2 * attributeSize;
            usableAttributeType = GL_FLOAT;
            usableAttributeSize = 2;
            break;
        case GL_FLOAT_VEC3:
            attributteByteSize = sizeof(float) * 3 * attributeSize;
            usableAttributeType = GL_FLOAT;
            usableAttributeSize = 3;
            break;
        case GL_FLOAT_MAT2:
        case GL_FLOAT_VEC4:
            attributteByteSize = sizeof(float) * 4 * attributeSize;
            usableAttributeType = GL_FLOAT;
            usableAttributeSize = 4;
            break;
        case GL_FLOAT_MAT3:
            attributteByteSize = sizeof(float) * 9 * attributeSize;
            usableAttributeType = GL_FLOAT;
            usableAttributeSize = 9;
            break;
        case GL_FLOAT_MAT4:
            attributteByteSize = sizeof(float) * 16 * attributeSize;
            usableAttributeType = GL_FLOAT;
            usableAttributeSize = 16;
            break;
        case GL_FLOAT_MAT2x3:
        case GL_FLOAT_MAT3x2:
            attributteByteSize = sizeof(float) * 6 * attributeSize;
            usableAttributeType = GL_FLOAT;
            usableAttributeSize = 6;
            break;
        case GL_FLOAT_MAT2x4:
        case GL_FLOAT_MAT4x2:
            attributteByteSize = sizeof(float) * 8 * attributeSize;
            usableAttributeType = GL_FLOAT;
            usableAttributeSize = 8;
            break;
        case GL_FLOAT_MAT3x4:
        case GL_FLOAT_MAT4x3:
            attributteByteSize = sizeof(float) * 12 * attributeSize;
            usableAttributeType = GL_FLOAT;
            usableAttributeSize = 12;
            break;
        case GL_INT:
            attributteByteSize = sizeof(int) * attributeSize;
            usableAttributeType = GL_INT;
            usableAttributeSize = 1;
            break;
        case GL_INT_VEC2:
            attributteByteSize = sizeof(int) * 2 * attributeSize;
            usableAttributeType = GL_INT;
            usableAttributeSize = 2;
            break;
        case GL_INT_VEC3:
            attributteByteSize = sizeof(int) * 3 * attributeSize;
            usableAttributeType = GL_INT;
            usableAttributeSize = 3;
            break;
        case GL_INT_VEC4:
            attributteByteSize = sizeof(int) * 4 * attributeSize;
            usableAttributeType = GL_INT;
            usableAttributeSize = 4;
            break;
        case GL_UNSIGNED_INT:
            attributteByteSize = sizeof(unsigned int) * attributeSize;
            usableAttributeType = GL_UNSIGNED_INT;
            usableAttributeSize = 1;
            break;
        case GL_UNSIGNED_INT_VEC2:
            attributteByteSize = sizeof(unsigned int) * 2 * attributeSize;
            usableAttributeType = GL_UNSIGNED_INT;
            usableAttributeSize = 2;
            break;
        case GL_UNSIGNED_INT_VEC3:
            attributteByteSize = sizeof(unsigned int) * 3 * attributeSize;
            usableAttributeType = GL_UNSIGNED_INT;
            usableAttributeSize = 2;
            break;
        case GL_UNSIGNED_INT_VEC4:
            attributteByteSize = sizeof(unsigned int) * 4 * attributeSize;
            usableAttributeType = GL_UNSIGNED_INT;
            usableAttributeSize = 4;
            break;
        case GL_DOUBLE:
            attributteByteSize = sizeof(double) * attributeSize;
            usableAttributeType = GL_DOUBLE;
            usableAttributeSize = 1;
            break;
        case GL_DOUBLE_VEC2:
            attributteByteSize = sizeof(double) * 2 * attributeSize;
            usableAttributeType = GL_DOUBLE;
            usableAttributeSize = 2;
            break;
        case GL_DOUBLE_VEC3:
            attributteByteSize = sizeof(double) * 3 * attributeSize;
            usableAttributeType = GL_DOUBLE;
            usableAttributeSize = 3;
            break;
        case GL_DOUBLE_MAT2:
        case GL_DOUBLE_VEC4:
            attributteByteSize = sizeof(double) * 4 * attributeSize;
            usableAttributeType = GL_DOUBLE;
            usableAttributeSize = 4;
            break;
        case GL_DOUBLE_MAT3:
            attributteByteSize = sizeof(double) * 9 * attributeSize;
            usableAttributeType = GL_DOUBLE;
            usableAttributeSize = 9;
            break;
        case GL_DOUBLE_MAT4:
            attributteByteSize = sizeof(double) * 16 * attributeSize;
            usableAttributeType = GL_DOUBLE;
            usableAttributeSize = 16;
            break;
        case GL_DOUBLE_MAT2x3:
        case GL_DOUBLE_MAT3x2:
            attributteByteSize = sizeof(double) * 6 * attributeSize;
            usableAttributeType = GL_DOUBLE;
            usableAttributeSize = 6;
            break;
        case GL_DOUBLE_MAT2x4:
        case GL_DOUBLE_MAT4x2:
            attributteByteSize = sizeof(double) * 8 * attributeSize;
            usableAttributeType = GL_DOUBLE;
            usableAttributeSize = 8;
            break;
        case GL_DOUBLE_MAT3x4:
        case GL_DOUBLE_MAT4x3:
            attributteByteSize = sizeof(double) * 12 * attributeSize;
            usableAttributeType = GL_DOUBLE;
            usableAttributeSize = 12;
            break;
        default:
            //TODO: what if we fail???
            ELOG("Error: Unknown type of program attribute");
        }
        //TODO: stride
        //glVertexAttribPointer(attributeLocation, usableAttributeSize, usableAttributeType, GL_FALSE, sizeof(float) * 5, ((void*)offset));
        offset += attributteByteSize;
        //glEnableVertexAttribArray(i);

        //location + component count
        program.vertexInputLayout.attributes.push_back({(u8)attributeLocation,(u8)usableAttributeSize});
    }
    delete[] attributeName;

    app->mode = Mode_Patrick;
    glEnable(GL_DEPTH_TEST);
}

glm::mat4 TransformScale(const glm::vec3& scaleFactors)
{
    glm::mat4 transform = scale(scaleFactors);
    return transform;
}

glm::mat4 TransformPositionScale(const glm::vec3& pos, const glm::vec3& scaleFactor)
{
    glm::mat4 transform = glm::translate(pos);
    transform = glm::scale(transform, scaleFactor);
    return transform;
}

constexpr vec3 GetAttenuationValuesFromRange(unsigned int range)
{
    if (range <= 7) { return vec3(1, 0.7, 1.8); }
    else if (range <= 13) { return vec3(1, 0.35, 0.44); }
    else if (range <= 20) { return vec3(1, 0.22, 0.20); }
    else if (range <= 32) { return vec3(1, 0.14, 0.07); }
    else if (range <= 50) { return vec3(1, 0.09, 0.032); }
    else if (range <= 65) { return vec3(1, 0.07, 0.017); }
    else if (range <= 100) { return vec3(1, 0.045, 0.0075); }
    else if (range <= 160) { return vec3(1, 0.027, 0.0028); }
    else if (range <= 200) { return vec3(1, 0.022, 0.0019); }
    else if (range <= 325) { return vec3(1, 0.014, 0.0007); }
    else if (range <= 600) { return vec3(1, 0.007, 0.0002); }
    else if (range <= 3250) { return vec3(1, 0.0014, 0.000007); }
}

GLuint GenerateFrameBuffer(App*app)
{
    glGenTextures(1, &app->colorAttachmentHandle0);
    glBindTexture(GL_TEXTURE_2D, app->colorAttachmentHandle0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &app->colorAttachmentHandle1);
    glBindTexture(GL_TEXTURE_2D, app->colorAttachmentHandle1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &app->colorAttachmentHandle2);
    glBindTexture(GL_TEXTURE_2D, app->colorAttachmentHandle2);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &app->colorAttachmentHandle3);
    glBindTexture(GL_TEXTURE_2D, app->colorAttachmentHandle3);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &app->depthAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->depthAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLuint frameBufferHandle;
    glGenFramebuffers(1, &frameBufferHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferHandle);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->colorAttachmentHandle0, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, app->colorAttachmentHandle1, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, app->colorAttachmentHandle2, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, app->colorAttachmentHandle3, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, app->depthAttachmentHandle, 0);

    GLenum frameBufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (frameBufferStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        switch(frameBufferStatus)
        {
        case GL_FRAMEBUFFER_UNDEFINED: ELOG("GL_FRAMEBUFFER_UNDEFINED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: ELOG("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: ELOG("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: ELOG("GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: ELOG("GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); break;
        case GL_FRAMEBUFFER_UNSUPPORTED: ELOG("GL_FRAMEBUFFER_UNSUPPORTED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: ELOG("GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: ELOG("GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"); break;
        default: ELOG("Unknown framebuffer status error");
        }
    }

    GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
    glDrawBuffers(4, buffers);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return frameBufferHandle;
}

void Init(App* app)
{
    // TODO: Initialize your resources here!
    // - vertex buffers
    // - element/index buffers
    // - vaos
    // - programs (and retrieve uniform indices)
    // - textures

    //const float vertices[] = {
    //    -0.5,-0.5,0.0,0.0,0.0,
    //    0.5,-0.5,0.0,1.0,0.0,
    //    0.5,0.5,0.0,1.0,1.0,
    //    -0.5,0.5,0.0,0.0,1.0
    //};
    //
    //const unsigned short indices[] = {
    //    0,1,2,0,2,3
    //};
    //
    //glGenBuffers(1,&app->embeddedVertices);
    //glBindBuffer(GL_ARRAY_BUFFER,app->embeddedVertices);
    //int a = sizeof(vertices);
    //glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
    ////glBindBuffer(GL_ARRAY_BUFFER,0);
    //
    //glGenBuffers(1,&app->embeddedElements);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    //glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(indices),indices,GL_STATIC_DRAW);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
    //
    //glGenVertexArrays(1,&app->vao);
    //glBindVertexArray(app->vao);
    //glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    ////glVertexAttribPointer(0,3, GL_FLOAT, GL_FALSE,sizeof(float)*5,(void*)0);
    ////glEnableVertexAttribArray(0);
    ////glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(float)*5,(void*)12);
    ////glEnableVertexAttribArray(1);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,app->embeddedElements);
    //
    //app->texturedGeometryProgramIdx = LoadProgram(app,"shaders.glsl","TEXTURED_GEOMETRY");
    ////Program& texturedGeometryProgram = app->programs[app->texturedGeometryProgramIdx];
    ////app->patriceGeoProgramIdx = LoadProgram(app, "shaders2.glsl", "TEXTURED_PATRICE");
    //Program& texturedGeometryProgram = app->programs.back();
    //app->programUniformTexture = glGetUniformLocation(texturedGeometryProgram.handle,"uTexture");
    //
    //int maxVariableNameLength;
    //glGetProgramiv(texturedGeometryProgram.handle, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxVariableNameLength);
    //char* attributeName = new char[maxVariableNameLength];
    //int attributeNameLength;
    //int attributeSize;
    //int attributteByteSize;
    //int usableAttributeSize;
    //GLenum usableAttributeType;
    //GLenum attributeType;
    //int attributeCount;
    //int offset = 0;
    //int attributeLocation;
    //glGetProgramiv(texturedGeometryProgram.handle, GL_ACTIVE_ATTRIBUTES, &attributeCount);
    //for (int i = 0; i < attributeCount; ++i) {
    //    glGetActiveAttrib(texturedGeometryProgram.handle, i, /*ARRAY_COUNT(attributeName)*/maxVariableNameLength,
    //        &attributeNameLength,
    //        &attributeSize,
    //        &attributeType,
    //        attributeName);
    //
    //    attributeLocation = glGetAttribLocation(texturedGeometryProgram.handle, attributeName);
    //
    //    //careful with the stride!!!!
    //    switch (attributeType)
    //    {
    //    case GL_FLOAT:
    //        attributteByteSize = sizeof(float) * attributeSize;
    //        usableAttributeType = GL_FLOAT;
    //        usableAttributeSize = 1;
    //        break;
    //    case GL_FLOAT_VEC2:
    //        attributteByteSize = sizeof(float) * 2 * attributeSize;
    //        usableAttributeType = GL_FLOAT;
    //        usableAttributeSize = 2;
    //        break;
    //     case GL_FLOAT_VEC3:
    //         attributteByteSize = sizeof(float) * 3 * attributeSize;
    //         usableAttributeType = GL_FLOAT;
    //         usableAttributeSize = 3;
    //        break;
    //     case GL_FLOAT_MAT2:
    //     case GL_FLOAT_VEC4:
    //         attributteByteSize = sizeof(float) * 4 * attributeSize;
    //         usableAttributeType = GL_FLOAT;
    //         usableAttributeSize = 4;
    //        break;
    //    case GL_FLOAT_MAT3:
    //        attributteByteSize = sizeof(float) * 9 * attributeSize;
    //        usableAttributeType = GL_FLOAT;
    //        usableAttributeSize = 9;
    //        break;
    //    case GL_FLOAT_MAT4:
    //        attributteByteSize = sizeof(float) * 16 * attributeSize;
    //        usableAttributeType = GL_FLOAT;
    //        usableAttributeSize = 16;
    //        break;
    //    case GL_FLOAT_MAT2x3:
    //    case GL_FLOAT_MAT3x2:
    //        attributteByteSize = sizeof(float) * 6 * attributeSize;
    //        usableAttributeType = GL_FLOAT;
    //        usableAttributeSize = 6;
    //        break;
    //    case GL_FLOAT_MAT2x4:
    //    case GL_FLOAT_MAT4x2:
    //        attributteByteSize = sizeof(float) * 8 * attributeSize;
    //        usableAttributeType = GL_FLOAT;
    //        usableAttributeSize = 8;
    //        break;
    //    case GL_FLOAT_MAT3x4:
    //    case GL_FLOAT_MAT4x3:
    //        attributteByteSize = sizeof(float) * 12 * attributeSize;
    //        usableAttributeType = GL_FLOAT;
    //        usableAttributeSize = 12;
    //        break;
    //    case GL_INT:
    //        attributteByteSize = sizeof(int) * attributeSize;
    //        usableAttributeType = GL_INT;
    //        usableAttributeSize = 1;
    //        break;
    //    case GL_INT_VEC2:
    //        attributteByteSize = sizeof(int) * 2 *attributeSize;
    //        usableAttributeType = GL_INT;
    //        usableAttributeSize = 2;
    //        break;
    //    case GL_INT_VEC3:
    //        attributteByteSize = sizeof(int) * 3 * attributeSize;
    //        usableAttributeType = GL_INT;
    //        usableAttributeSize = 3;
    //        break;
    //    case GL_INT_VEC4:
    //        attributteByteSize = sizeof(int) * 4 * attributeSize;
    //        usableAttributeType = GL_INT;
    //        usableAttributeSize = 4;
    //        break;
    //    case GL_UNSIGNED_INT:
    //        attributteByteSize = sizeof(unsigned int) * attributeSize;
    //        usableAttributeType = GL_UNSIGNED_INT;
    //        usableAttributeSize = 1;
    //        break;
    //    case GL_UNSIGNED_INT_VEC2:
    //        attributteByteSize = sizeof(unsigned int) * 2 * attributeSize;
    //        usableAttributeType = GL_UNSIGNED_INT;
    //        usableAttributeSize = 2;
    //        break;
    //    case GL_UNSIGNED_INT_VEC3:
    //        attributteByteSize = sizeof(unsigned int) * 3 * attributeSize;
    //        usableAttributeType = GL_UNSIGNED_INT;
    //        usableAttributeSize = 2;
    //        break;
    //    case GL_UNSIGNED_INT_VEC4:
    //        attributteByteSize = sizeof(unsigned int) * 4 * attributeSize;
    //        usableAttributeType = GL_UNSIGNED_INT;
    //        usableAttributeSize = 4;
    //        break;
    //    case GL_DOUBLE:
    //        attributteByteSize = sizeof(double) * attributeSize;
    //        usableAttributeType = GL_DOUBLE;
    //        usableAttributeSize = 1;
    //        break;
    //    case GL_DOUBLE_VEC2:
    //        attributteByteSize = sizeof(double) * 2 * attributeSize;
    //        usableAttributeType = GL_DOUBLE;
    //        usableAttributeSize = 2;
    //        break;
    //    case GL_DOUBLE_VEC3:
    //        attributteByteSize = sizeof(double) * 3 * attributeSize;
    //        usableAttributeType = GL_DOUBLE;
    //        usableAttributeSize = 3;
    //        break;
    //    case GL_DOUBLE_MAT2:
    //    case GL_DOUBLE_VEC4:
    //        attributteByteSize = sizeof(double) * 4 * attributeSize;
    //        usableAttributeType = GL_DOUBLE;
    //        usableAttributeSize = 4;
    //        break;
    //    case GL_DOUBLE_MAT3:
    //        attributteByteSize = sizeof(double) * 9 * attributeSize;
    //        usableAttributeType = GL_DOUBLE;
    //        usableAttributeSize = 9;
    //        break;
    //    case GL_DOUBLE_MAT4:
    //        attributteByteSize = sizeof(double) * 16 * attributeSize;
    //        usableAttributeType = GL_DOUBLE;
    //        usableAttributeSize = 16;
    //        break;
    //    case GL_DOUBLE_MAT2x3:
    //    case GL_DOUBLE_MAT3x2:
    //        attributteByteSize = sizeof(double) * 6 * attributeSize;
    //        usableAttributeType = GL_DOUBLE;
    //        usableAttributeSize = 6;
    //        break;
    //    case GL_DOUBLE_MAT2x4:
    //    case GL_DOUBLE_MAT4x2:
    //        attributteByteSize = sizeof(double) * 8 * attributeSize;
    //        usableAttributeType = GL_DOUBLE;
    //        usableAttributeSize = 8;
    //        break;
    //    case GL_DOUBLE_MAT3x4:
    //    case GL_DOUBLE_MAT4x3:
    //        attributteByteSize = sizeof(double) * 12 * attributeSize;
    //        usableAttributeType = GL_DOUBLE;
    //        usableAttributeSize = 12;
    //        break;
    //    default:
    //        //TODO: what if we fail???
    //        ELOG("Error: Unknown type of program attribute");
    //    }
    //    //TODO: stride
    //    glVertexAttribPointer(attributeLocation, usableAttributeSize, usableAttributeType, GL_FALSE, sizeof(float) * 5, ((void*)offset));
    //    offset += attributteByteSize;
    //    glEnableVertexAttribArray(i);
    //}
    //delete[] attributeName;
    //
    //app->diceTexIdx = LoadTexture2D(app, "dice.png");
    //app->whiteTexIdx = LoadTexture2D(app, "color_white.png");
    //app->blackTexIdx = LoadTexture2D(app, "color_black.png");
    //app->normalTexIdx= LoadTexture2D(app, "color_normal.png");
    //app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");
    //
    //app->mode = Mode_TexturedQuad;
    
    app->framebufferHandle = GenerateFrameBuffer(app);
    app->currentAttachmentTextureHandle = app->colorAttachmentHandle0;
    app->currentAttachmentType = AttachmentOutputs::SCENE;
    //for the screen quad
    LoadTexturesQuad(app);
    LoadPatrik(app);

    //loading lights
    //app->lights.push_back({vec3(1,1,1), vec3(1,-1,-1), vec3(0,0,0), LightType_Directional });
    app->lights.push_back({vec3(1,1,1), GetAttenuationValuesFromRange(20), vec3(0,-2,0), LightType_Point });

    app->cbuffer.head = 0;
    app->cbuffer.data = nullptr;
    GLint maxUniformBufferSize;
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBufferSize);
    app->cbuffer.size = maxUniformBufferSize;
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBlockAlignment);
    app->cbuffer.type = GL_UNIFORM_BUFFER;

    glGenBuffers(1, &app->cbuffer.handle);
    glBindBuffer(GL_UNIFORM_BUFFER, app->cbuffer.handle);
    glBufferData(GL_UNIFORM_BUFFER, app->cbuffer.size, NULL, GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    float x = 2.8f;
    //Load x patrick entities
    for (int i = 0; i < 3; ++i) {
        glm::mat4 world = TransformPositionScale(vec3(x, 1.5f, -2.0f), vec3(0.45f));
        x-=3;
        Entity one = {};
        one.worldMatrix = world;
        one.modelIndex = app->models.size() - 1;
        one.localParamsOffset = 0;
        one.localParamsSize = 0;
        app->entities.push_back(one);
    }
}

void Gui(App* app)
{
    ImGui::Begin("Info");
    ImGui::Text("FPS: %f", 1.0f/app->deltaTime);
    char strMem[8];
    char* currentValue = strMem;
    switch (app->currentAttachmentType)
    {
    case AttachmentOutputs::SCENE: currentValue = (char*)"SCENE"; break;
    case AttachmentOutputs::ALBEDO: currentValue = (char*)"ALBEDO"; break;
    case AttachmentOutputs::NORMALS: currentValue = (char*)"NORMALS"; break;
    case AttachmentOutputs::DEPTH: currentValue = (char*)"DEPTH"; break;
    default:
        break;
    }
    if (ImGui::BeginCombo("##Screen Output", currentValue, ImGuiComboFlags_PopupAlignLeft))
    {
        if (ImGui::Selectable("SCENE")) {
            app->currentAttachmentTextureHandle = app->colorAttachmentHandle0;
            app->currentAttachmentType = AttachmentOutputs::SCENE;
        }
        if (ImGui::Selectable("ALBEDO")) {
            app->currentAttachmentTextureHandle = app->colorAttachmentHandle1;
            app->currentAttachmentType = AttachmentOutputs::ALBEDO;
        }
        if (ImGui::Selectable("NORMALS")) {
            app->currentAttachmentTextureHandle = app->colorAttachmentHandle2;
            app->currentAttachmentType = AttachmentOutputs::NORMALS;
        }
        if (ImGui::Selectable("DEPTH")) {
            app->currentAttachmentTextureHandle = app->colorAttachmentHandle3;
            app->currentAttachmentType = AttachmentOutputs::DEPTH;
        }
        ImGui::EndCombo();
    }
    ImGui::End();
}

void Update(App* app)
{
    float aspectRario = (float)app->displaySize.x / (float)app->displaySize.y;
    float znear = 0.1f;
    float zfar = 1000.0f;
    glm::mat4 projection = glm::perspective(glm::radians(60.f), aspectRario, znear, zfar);
    glm::vec3 cameraPos = glm::vec3(0.f, 0.f, 4.f);
    glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0.0f), glm::vec3(0.f, 1.f, 0.f));

    ++app->angle;

    glBindBuffer(GL_UNIFORM_BUFFER, app->cbuffer.handle);

    //u8* bufferData = (u8*)glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    //u32 bufferHead = 0;
    app->cbuffer.head = 0;
    app->cbuffer.data = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);

    // -- Global Params
    app->globalParamsOffset = app->cbuffer.head;
    PushVec3(app->cbuffer, cameraPos);
    PushUInt(app->cbuffer, app->lights.size());
    
    for (u32 i = 0; i < app->lights.size(); ++i)
    {
        AlignHead(app->cbuffer, sizeof(vec4));
    
        Light& light = app->lights[i];
        PushVec3(app->cbuffer, light.color);
        PushVec3(app->cbuffer, light.direction);
        PushVec3(app->cbuffer, light.position);
        PushUInt(app->cbuffer, light.type);
    }
    app->globalParamsSize = app->cbuffer.head - app->globalParamsOffset;

    // -- Local Params
    for (int i = 0; i<app->entities.size(); ++i)
    {

        AlignHead(app->cbuffer, app->uniformBlockAlignment);
        app->entities[i].localParamsOffset = app->cbuffer.head;

        glm::mat4 world = glm::rotate(app->entities[i].worldMatrix, glm::radians(app->angle), vec3(0, 1, 0));
        glm::mat4 MVP = projection * view * world;

        PushMat4(app->cbuffer, world);
        PushMat4(app->cbuffer, MVP);

        app->entities[i].localParamsSize = app->cbuffer.head - app->entities[i].localParamsOffset;
    }

    glUnmapBuffer(GL_UNIFORM_BUFFER);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

}

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program) {
    Submesh& submesh = mesh.submeshes[submeshIndex];
    for (u32 i = 0; i < (u32)submesh.vaos.size(); ++i)
    {
        if (submesh.vaos[i].programHandle == program.handle)
            return submesh.vaos[i].handle;
    }      GLuint vaoHandle = 0;
    //Create a Vao
    glGenVertexArrays(1, &vaoHandle);
    glBindVertexArray(vaoHandle);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);
    for (u32 i = 0; i < program.vertexInputLayout.attributes.size(); ++i)
    {
        bool attributeWasLinked = false;
        for (u32 j = 0; j < submesh.vertexBufferLayout.attributes.size(); ++j)
        {
            if (program.vertexInputLayout.attributes[i].location == submesh.vertexBufferLayout.attributes[j].location)
            {
                const u32 index = submesh.vertexBufferLayout.attributes[j].location;
                const u32 ncomp = submesh.vertexBufferLayout.attributes[j].componentCount;
                const u32 offset = submesh.vertexBufferLayout.attributes[j].offset + submesh.vertexOffset;
                const u32 stride = submesh.vertexBufferLayout.stride;
                glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)(u64)offset);
                glEnableVertexAttribArray(index);
                attributeWasLinked = true;
                break;
            }
        }
        assert(attributeWasLinked);
    }
    glBindVertexArray(0);
    Vao vao = { vaoHandle, program.handle };
    submesh.vaos.push_back(vao);
    return vaoHandle;
}

void Render(App* app)
{
    switch (app->mode)
    {
        case Mode_TexturedQuad:
            {
                // TODO: Draw your textured quad here!
                // - clear the framebuffer
                // - set the viewport
                // - set the blending state
                // - bind the texture into unit 0
                // - bind the program 
                //   (...and make its texture sample from unit 0)
                // - bind the vao
                // - glDrawElements() !!!
                glClearColor(0.1f,0.1f,0.1f,1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                glViewport(0,0,app->displaySize.x,app->displaySize.y);

                Program& programTexturedGeometry = app->programs[app->texturedGeometryProgramIdx];
                glUseProgram(programTexturedGeometry.handle);
                glBindVertexArray(app->vao);

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

                glUniform1i(app->programUniformTexture,0);
                glActiveTexture(GL_TEXTURE0);
                GLuint textureHandle = app->textures[app->diceTexIdx].handle;
                glBindTexture(GL_TEXTURE_2D,textureHandle);

                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

                glBindVertexArray(0);
                glUseProgram(0);

            }
            break;
        case Mode_Patrick:
            {
                glBindFramebuffer(GL_FRAMEBUFFER, app->framebufferHandle);
                //glEnable(GL_DEPTH_TEST);
                
                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glViewport(0, 0, app->displaySize.x, app->displaySize.y);
                
                Program& textureMeshProgram = app->programs[app->patrickProgramIdx];
                glUseProgram(textureMeshProgram.handle);
                //binding global uniform buffer params
                glBindBufferRange(GL_UNIFORM_BUFFER, 0, app->cbuffer.handle, app->globalParamsOffset, app->globalParamsSize);
                for(Entity entity : app->entities)
                {
                    //binding local uniform buffer params
                    glBindBufferRange(GL_UNIFORM_BUFFER, 1, app->cbuffer.handle, entity.localParamsOffset, entity.localParamsSize);
                    Model& model = app->models[entity.modelIndex];
                    Mesh& mesh = app->meshes[model.meshIdx]; 
                    for (u32 i = 0; i < mesh.submeshes.size(); ++i) {
                        GLuint vao = FindVAO(mesh, i, textureMeshProgram);
                        glBindVertexArray(vao);
                        u32 submeshMaterialIdx = model.materialIdx[i];
                        Material& submeshMaterial = app->materials[submeshMaterialIdx];
                        glActiveTexture(GL_TEXTURE);
                        glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);
                        glUniform1i(app->programUniformTexture, 0);
                        //glUniformMatrix4fv(glGetUniformLocation(textureMeshProgram.handle, "MVP"), 1, GL_FALSE, glm::value_ptr(app->MVP));
                
                        Submesh& submesh = mesh.submeshes[i];
                        glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
                    }
                }
                glBindVertexArray(0);
                glUseProgram(0);
                
                //draw scene texture to screen
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                
                glUseProgram(app->programs[app->texturedGeometryProgramIdx].handle);
                glBindVertexArray(app->vao);
                
                //glEnable(GL_BLEND);
                //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                
                glUniform1i(app->programUniformTexture, 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, app->currentAttachmentTextureHandle);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
                glBindVertexArray(0);
                glUseProgram(0);

            }
            break;

        default:;
    }
}

