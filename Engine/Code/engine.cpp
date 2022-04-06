//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include "engine.h"
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
    -0.5,-0.5,0.0,0.0,0.0,
    0.5,-0.5,0.0,1.0,0.0,
    0.5,0.5,0.0,1.0,1.0,
    -0.5,0.5,0.0,0.0,1.0
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

    app->texturedGeometryProgramIdx = LoadProgram(app, "shaders.glsl", "TEXTURED_GEOMETRY");
    Program& texturedGeometryProgram = app->programs[app->patriceGeoProgramIdx];
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
    LoadModel(app, "Patrik/Patrick.obj");
}

void Init(App* app)
{
    // TODO: Initialize your resources here!
    // - vertex buffers
    // - element/index buffers
    // - vaos
    // - programs (and retrieve uniform indices)
    // - textures
    const float vertices[] = {
        -0.5,-0.5,0.0,0.0,0.0,
        0.5,-0.5,0.0,1.0,0.0,
        0.5,0.5,0.0,1.0,1.0,
        -0.5,0.5,0.0,0.0,1.0
    };

    const unsigned short indices[] = {
        0,1,2,0,2,3
    };

    glGenBuffers(1,&app->embeddedVertices);
    glBindBuffer(GL_ARRAY_BUFFER,app->embeddedVertices);
    int a = sizeof(vertices);
    glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
    //glBindBuffer(GL_ARRAY_BUFFER,0);

    glGenBuffers(1,&app->embeddedElements);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(indices),indices,GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);

    glGenVertexArrays(1,&app->vao);
    glBindVertexArray(app->vao);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    //glVertexAttribPointer(0,3, GL_FLOAT, GL_FALSE,sizeof(float)*5,(void*)0);
    //glEnableVertexAttribArray(0);
    //glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(float)*5,(void*)12);
    //glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,app->embeddedElements);

    app->texturedGeometryProgramIdx = LoadProgram(app,"shaders.glsl","TEXTURED_GEOMETRY");
    //Program& texturedGeometryProgram = app->programs[app->texturedGeometryProgramIdx];
    //app->patriceGeoProgramIdx = LoadProgram(app, "shaders2.glsl", "TEXTURED_PATRICE");
    Program& texturedGeometryProgram = app->programs[app->patriceGeoProgramIdx];
    app->programUniformTexture = glGetUniformLocation(texturedGeometryProgram.handle,"uTexture");

    int maxVariableNameLength;
    glGetProgramiv(texturedGeometryProgram.handle, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxVariableNameLength);
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
    glGetProgramiv(texturedGeometryProgram.handle, GL_ACTIVE_ATTRIBUTES, &attributeCount);
    for (int i = 0; i < attributeCount; ++i) {
        glGetActiveAttrib(texturedGeometryProgram.handle, i, /*ARRAY_COUNT(attributeName)*/maxVariableNameLength,
            &attributeNameLength,
            &attributeSize,
            &attributeType,
            attributeName);
    
        attributeLocation = glGetAttribLocation(texturedGeometryProgram.handle, attributeName);
    
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
            attributteByteSize = sizeof(int) * 2 *attributeSize;
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
        glVertexAttribPointer(attributeLocation, usableAttributeSize, usableAttributeType, GL_FALSE, sizeof(float) * 5, ((void*)offset));
        offset += attributteByteSize;
        glEnableVertexAttribArray(i);
    }
    delete[] attributeName;

    app->diceTexIdx = LoadTexture2D(app, "dice.png");
    app->whiteTexIdx = LoadTexture2D(app, "color_white.png");
    app->blackTexIdx = LoadTexture2D(app, "color_black.png");
    app->normalTexIdx= LoadTexture2D(app, "color_normal.png");
    app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");

    app->mode = Mode_TexturedQuad;
}

void Gui(App* app)
{
    ImGui::Begin("Info");
    ImGui::Text("FPS: %f", 1.0f/app->deltaTime);
    ImGui::End();
}

void Update(App* app)
{
    // You can handle app->input keyboard/mouse here
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

        default:;
    }
}

