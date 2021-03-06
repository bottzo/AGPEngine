#include "engine.h"
#include "buffer_management.h"
#include "assimp_model_loading.h"
#include "engine_ui.h"
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>

#define DEGTORAD 0.0174533f

GLuint CreateProgramFromSource(String programSource, const char* shaderName, bool geometry = false)
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
    char geometryShaderDefine[] = "#define GEOMETRY\n";

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
    const GLchar* geometryShaderSource[] = {
        versionString,
        shaderNameDefine,
        geometryShaderDefine,
        programSource.str
    };
    const GLint geometryShaderLengths[] = {
        (GLint)strlen(versionString),
        (GLint)strlen(shaderNameDefine),
        (GLint)strlen(geometryShaderDefine),
        (GLint)programSource.len
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

    GLuint geoshader;
    if (geometry) {
        geoshader = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(geoshader, ARRAY_COUNT(geometryShaderSource), geometryShaderSource, geometryShaderLengths);
        glCompileShader(geoshader);
        glGetShaderiv(geoshader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(geoshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
            ELOG("glCompileShader() failed with geometry shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
        }
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
    if (geometry)
        glAttachShader(programHandle, geoshader);
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
    if(geometry)
        glDetachShader(programHandle, geoshader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);
    if(geometry)
        glDeleteShader(geoshader);

    return programHandle;
}

void LoadProgramAttributes(Program& program)
{
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
        program.vertexInputLayout.attributes.push_back({ (u8)attributeLocation,(u8)usableAttributeSize });
    }
    delete[] attributeName;
}


u32 LoadProgram(App* app, const char* filepath, const char* programName, bool geometryShader = false)
{
    String programSource = ReadTextFile(filepath);

    Program program = {};
    program.handle = CreateProgramFromSource(programSource, programName, geometryShader);
    program.filepath = filepath;
    program.programName = programName;
    program.lastWriteTimestamp = GetFileLastWriteTimestamp(filepath);

    LoadProgramAttributes(program);

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

u32 CreateSphere(App* app)
{
    const u32 H = 32;
    const u32 V = 16;
    struct Vertex { vec3 pos; vec3 norm; };
    Vertex sphere[H][V + 1];

    app->meshes.push_back(Mesh{});
    Mesh& mesh = app->meshes.back();
    app->models.push_back(Model{});
    Model& model = app->models.back();
    model.meshIdx = (u32)app->meshes.size() - 1u;
    u32 modelIdx = (u32)app->models.size() - 1u;

    mesh.submeshes.push_back(Submesh{});
    Submesh& subMesh = mesh.submeshes.back();
    VertexBufferLayout vertexFormat;
    vertexFormat.attributes.push_back({ 0,3,0 });
    vertexFormat.attributes.push_back({ 1,3, 3 * sizeof(float) });
    vertexFormat.attributes.push_back(VertexBufferAttribute{ 2, 2, 6*sizeof(float) });
    vertexFormat.stride = 8 * sizeof(float);
    subMesh.vertexBufferLayout = vertexFormat;
    subMesh.vertices.reserve(32 * 16 * 8);

    for (int h = 0; h < H; ++h)
    {
        for (int v = 0; v < V+1; ++v)
        {
            float nh = float(h) / H;
            float nv = float(v) / V - 0.5f;
            float angleh = 2 * PI * nh;
            float anglev = -PI * nv;
            sphere[h][v].pos.x = sinf(angleh) * cosf(anglev);
            sphere[h][v].pos.y = -sinf(anglev);
            sphere[h][v].pos.z = cosf(angleh) * cosf(anglev);
            sphere[h][v].norm = sphere[h][v].pos;
            subMesh.vertices.push_back(sphere[h][v].pos.x);
            subMesh.vertices.push_back(sphere[h][v].pos.y);
            subMesh.vertices.push_back(sphere[h][v].pos.z);
            subMesh.vertices.push_back(sphere[h][v].norm.x);
            subMesh.vertices.push_back(sphere[h][v].norm.y);
            subMesh.vertices.push_back(sphere[h][v].norm.z);
            subMesh.vertices.push_back(0.f);
            subMesh.vertices.push_back(0.f);

        }
    }

    subMesh.indices.reserve(32 * 16 * 6);
    u32 sphereIndices[H][V][6];
    for (u32 h = 0; h < H; ++h)
    {
        for (u32 v = 0; v < V; ++v)
        {
            sphereIndices[h][v][0] = (h + 0) * (V + 1) + v;
            sphereIndices[h][v][1] = ((h + 1) % H) * (V + 1) + v;
            sphereIndices[h][v][2] = ((h + 1) % H) * (V + 1) + v + 1;
            sphereIndices[h][v][3] = (h + 0) * (V + 1) + v;
            sphereIndices[h][v][4] = ((h + 1) % H) * (V + 1) + v + 1;
            sphereIndices[h][v][5] = (h + 0) * (V + 1) + v + 1;
            subMesh.indices.push_back(sphereIndices[h][v][0]);
            subMesh.indices.push_back(sphereIndices[h][v][1]);
            subMesh.indices.push_back(sphereIndices[h][v][2]);
            subMesh.indices.push_back(sphereIndices[h][v][3]);
            subMesh.indices.push_back(sphereIndices[h][v][4]);
            subMesh.indices.push_back(sphereIndices[h][v][5]);
        }
    }

    glGenBuffers(1, &mesh.vertexBufferHandle);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
    glBufferData(GL_ARRAY_BUFFER, subMesh.vertices.size()*sizeof(float), subMesh.vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &mesh.indexBufferHandle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, subMesh.indices.size()*sizeof(u32), subMesh.indices.data(), GL_STATIC_DRAW);

    Material mat = {};
    mat.albedoTextureIdx = app->magentaTexIdx;
    app->materials.push_back(mat);
    model.materialIdx.push_back(app->materials.size() - 1);

    return modelIdx;
}

u32 CreateWall(App* app)
{
    const float vertices[] = {
    -1,1,0,  0,0,1,  0.0,1.0, 1,0,0, 0,1,0,
    -1,-1,0, 0,0,1,  0.0,0.0, 1,0,0, 0,1,0,
    1,-1,0,  0,0,1,  1.0,0.0, 1,0,0, 0,1,0,
    1,1,0,   0,0,1,  1.0,1.0, 1,0,0, 0,1,0
    };

    const unsigned short indices[] = {
        0,1,2,0,2,3
    };

    app->meshes.push_back(Mesh{});
    Mesh& mesh = app->meshes.back();
    app->models.push_back(Model{});
    Model& model = app->models.back();
    model.meshIdx = (u32)app->meshes.size() - 1u;
    u32 modelIdx = (u32)app->models.size() - 1u;

    mesh.submeshes.push_back(Submesh{});
    Submesh& subMesh = mesh.submeshes.back();
    VertexBufferLayout vertexFormat;
    vertexFormat.attributes.push_back({ 0,3,0 });
    vertexFormat.attributes.push_back({ 1,3, 3 * sizeof(float) });
    vertexFormat.attributes.push_back(VertexBufferAttribute{ 2, 2, 6 * sizeof(float) });
    vertexFormat.attributes.push_back(VertexBufferAttribute{ 3, 3, 8 * sizeof(float) });
    vertexFormat.attributes.push_back(VertexBufferAttribute{ 4, 3, 11 * sizeof(float) });
    vertexFormat.stride = 14 * sizeof(float);
    subMesh.vertexBufferLayout = vertexFormat;
    const unsigned int vertexCount = sizeof(vertices) / sizeof(float);
    subMesh.vertices.reserve(vertexCount);
    for(int i = 0; i<vertexCount; ++i)
        subMesh.vertices.push_back(vertices[i]);

    const unsigned int indexCount = sizeof(indices) / sizeof(unsigned short);
    subMesh.indices.reserve(indexCount);
    for (int i = 0; i < indexCount; ++i)
        subMesh.indices.push_back(indices[i]);

    glGenBuffers(1, &mesh.vertexBufferHandle);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
    glBufferData(GL_ARRAY_BUFFER, subMesh.vertices.size() * sizeof(float), subMesh.vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &mesh.indexBufferHandle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, subMesh.indices.size() * sizeof(u32), subMesh.indices.data(), GL_STATIC_DRAW);

    app->materials.push_back(Material{});
    Material& material = app->materials.back();
    material.albedoTextureIdx = LoadTexture2D(app, "diffuse.png");
    material.normalsTextureIdx = LoadTexture2D(app, "normal.png");
    material.bumpTextureIdx = LoadTexture2D(app, "displacement.png");
    model.materialIdx.push_back(app->materials.size() - 1);

    Mesh planeMesh = mesh;
    Model plane = model;
    Material mat = {};
    mat.albedoTextureIdx = app->whiteTexIdx;
    app->materials.push_back(mat);
    plane.materialIdx.back() = (app->materials.size() - 1);
    app->meshes.push_back(planeMesh);
    app->models.push_back(plane);
    app->planeModelIdx = app->models.size() - 1;

    return modelIdx;
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
    const unsigned int colorAttachments = 5;
    app->ColorAttachmentHandles.reserve(colorAttachments);
    for (unsigned int i = 0; i < colorAttachments; ++i)
    {
        app->ColorAttachmentHandles.push_back(0);
        glGenTextures(1, &app->ColorAttachmentHandles[i]);
        glBindTexture(GL_TEXTURE_2D, app->ColorAttachmentHandles[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glGenTextures(1, &app->depthAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->depthAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLuint frameBufferHandle;
    glGenFramebuffers(1, &frameBufferHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferHandle);
    for (unsigned int i = 0; i < colorAttachments; ++i)
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, app->ColorAttachmentHandles[i], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, app->depthAttachmentHandle, 0);

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

    GLenum buffers[colorAttachments];
    for (unsigned int i = 0; i < colorAttachments; ++i)
        buffers[i] = GL_COLOR_ATTACHMENT0 + i;
    glDrawBuffers(colorAttachments, buffers);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    app->currentAttachmentTextureHandle = app->ColorAttachmentHandles[0];
    app->currentAttachmentType = AttachmentOutputs::SCENE;

    //------------------------------------------------------------------------------------------------------------
    glGenFramebuffers(1, &app->shadowFramebufferHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, app->shadowFramebufferHandle);

    glGenTextures(1, &app->shadowDepthAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->shadowDepthAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, app->shadowMapWidth, app->shadowMapHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float clampColor[] = { 1.f,1.f,1.f,1.f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, clampColor);
    glBindTexture(GL_TEXTURE_2D, 0);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, app->shadowDepthAttachmentHandle, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glGenFramebuffers(1, &app->shadowPointFramebufferHandle);
    glGenTextures(1, &app->shadowPointDepthAttachmentHandle);
    glBindTexture(GL_TEXTURE_CUBE_MAP, app->shadowPointDepthAttachmentHandle);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, app->shadowMapWidth, app->shadowMapHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindFramebuffer(GL_FRAMEBUFFER, app->shadowPointFramebufferHandle);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, app->shadowPointDepthAttachmentHandle, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

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

    app->geometryPassIdx = LoadProgram(app, "GeometryPass.glsl", "GEO_PASS");
    app->normGeoPassIdx = LoadProgram(app, "NormGeometryPass.glsl", "NORM_GEO_PASS");
    app->relGeoPassIdx = LoadProgram(app, "RelifGeometryPass.glsl", "REL_GEO_PASS");
    app->directionalLightIdx = LoadProgram(app, "DirectionalLight.glsl", "DIRECTIONAL_LIGHT");
    app->pointLightIdx = LoadProgram(app, "PointLight.glsl", "POINT_LIGHT");
    app->noFragmentIdx = LoadProgram(app, "NoFragment.glsl", "NO_FRAGMENT");
    app->shadowCubemapIdx = LoadProgram(app, "ShadowCubemap.glsl", "SHADOW_CUBEMAP", true);

    //for the screen quad
    LoadTexturesQuad(app);
    app->patrickModelIdx = LoadModel(app, "Patrick/Patrick.obj");
    app->rockModelIdx = LoadModel(app, "Rocks/Models/rock1.fbx");
    app->cyborgModelIdx = LoadModel(app, "Rocks/cyborg.fbx");
    app->sphereModelIdx = CreateSphere(app);
    app->wallModelIdx = CreateWall(app);

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

    float x = -2.6f;
    float z = -1.5f;
    //Load x patrick entities
    for (int i = 0; i < 3; ++i) {
        Entity one = {};
        one.pos = vec3(x, 1.5f, z);
        one.rot = vec3(0.f);
        one.scale = vec3(0.45f);
        one.worldMatrix = TransformPositionScale(one.pos, one.scale);
        one.modelIndex = app->patrickModelIdx;
        one.localParamsOffset = 0;
        one.localParamsSize = 0;
        one.name = "Patrick " + std::to_string(i);
        app->entities.push_back(one);
    
        x += 3;
        z -= 3;
    }
    
    Entity rock = {};
    rock.pos = vec3(6.f,0.f,0.f);
    rock.rot = vec3(0.f);
    rock.scale = vec3(0.45f);
    rock.worldMatrix = TransformPositionScale(rock.pos, rock.scale);
    rock.modelIndex = app->rockModelIdx;
    rock.localParamsOffset = 0;
    rock.localParamsSize = 0;
    rock.name = "Rock " + std::to_string(app->entities.size());
    app->entities.push_back(rock);
    
    Entity plane = {};
    plane.pos = vec3(0.f, 0.f, 0.f);
    plane.rot = vec3(0.f);
    plane.scale = vec3(80.f);
    plane.worldMatrix = TransformPositionScale(plane.pos, plane.scale);
    plane.worldMatrix = glm::rotate(plane.worldMatrix, -90 * DEGTORAD, glm::vec3(1.f, 0.f, 0.f));
    plane.modelIndex = app->planeModelIdx;
    plane.localParamsOffset = 0;
    plane.localParamsSize = 0;
    plane.name = "Plane " + std::to_string(app->models.size() - 1);
    app->entities.push_back(plane);

    Entity plane2 = {};
    plane2.pos = vec3(0.f, 1.5f, 0.f);
    plane2.rot = vec3(0.f);
    plane2.scale = vec3(3.f,1.5f,2.f);
    plane2.worldMatrix = TransformPositionScale(plane2.pos, plane2.scale);
    plane2.worldMatrix = glm::rotate(plane2.worldMatrix, 0 * DEGTORAD, glm::vec3(1.f, 0.f, 0.f));
    plane2.modelIndex = app->wallModelIdx;
    plane2.localParamsOffset = 0;
    plane2.localParamsSize = 0;
    plane2.name = "wall " + std::to_string(app->models.size() - 1);
    app->entities.push_back(plane2);

    Entity cyborg = {};
    cyborg.pos = vec3(0.f, 0.f, 0.5f);
    cyborg.rot = vec3(0.f);
    cyborg.scale = vec3(1.f);
    cyborg.worldMatrix = TransformPositionScale(cyborg.pos, cyborg.scale);
    cyborg.modelIndex = app->cyborgModelIdx;
    cyborg.localParamsOffset = 0;
    cyborg.localParamsSize = 0;
    cyborg.name = "Cyborg " + std::to_string(app->entities.size());
    app->entities.push_back(cyborg);
    
    //loading lights
    //app->lights.push_back({vec3(1,1,1), vec3(1,-1,-1), vec3(0,0,0), LightType_Directional });
    //TODO: Load screen filling quad model to models for the directional
    //float radius = 103.f;
    //app->lights.push_back({ vec3(0.1569f,0.651f,0.8039f), GetAttenuationValuesFromRange(radius), radius , LightType::LightType_Point, TransformPositionScale(vec3(0.3f, 3.f, 0.f), vec3(radius)), app->sphereModelIdx, 0, 0, 0, 0, vec3(0.3f, 3.f, 0.f) });
    //radius = 35.f;
    //app->lights.push_back({ vec3(1.f,1.f,1.f), GetAttenuationValuesFromRange(radius), radius , LightType::LightType_Point, TransformPositionScale(vec3(0.75f, 3.f, -4.3f), vec3(radius)), app->sphereModelIdx, 0, 0, 0, 0, vec3(0.75f, 3.f, -4.3f) });
    //app->lights.push_back({ vec3(1,1,1), vec3(1,1,1), radius , LightType::LightType_Directional, TransformScale(vec3(1.f)), app->sphereModelIdx, 0, 0, 0, 0 });
   
    float radius = 77.f;
    app->lights.push_back({ vec3(0.878f,0.878f,0.f), GetAttenuationValuesFromRange(radius), radius , LightType::LightType_Point, TransformPositionScale(vec3(-2.2f, 3.f, 1.4f), vec3(radius)), app->sphereModelIdx, 0, 0, 0, 0, vec3(-2.2f, 3.f, 1.4f) });
    radius = 79.f;
    app->lights.push_back({ vec3(0.239f,0.f,1.f), GetAttenuationValuesFromRange(radius), radius , LightType::LightType_Point, TransformPositionScale(vec3(0.75f, 3.f, -8.15f), vec3(radius)), app->sphereModelIdx, 0, 0, 0, 0, vec3(0.75f, 3.f, -8.15f) });
    radius = 66.f;
    app->lights.push_back({ vec3(0.976f,0.f,0.f), GetAttenuationValuesFromRange(radius), radius , LightType::LightType_Point, TransformPositionScale(vec3(0.75f, 3.f, -2.65f), vec3(radius)), app->sphereModelIdx, 0, 0, 0, 0, vec3(0.75f, 3.f, -2.65f) });
    radius = 49.f;
    app->lights.push_back({ vec3(1.f,1.f,1.f), GetAttenuationValuesFromRange(radius), radius , LightType::LightType_Point, TransformPositionScale(vec3(0.f, 3.f, -0.95f), vec3(radius)), app->sphereModelIdx, 0, 0, 0, 0, vec3(0.f, 3.f, -0.95f) });


    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    app->mode = Mode::Mode_Patrick;
}

void Gui(App* app)
{
    //DOCKING
    InitializeDocking();

    ImGui::Begin("Info");
    ImGui::Separator();
    ImGui::Text("FPS: %f", 1.0f / app->deltaTime);
    ImGui::Separator();
    ImGui::Checkbox("Use normal maps", &app->useNormalMap);
    ImGui::Checkbox("Use relif maps", &app->useRelifMap);
    SelectFrameBufferTexture(app);
    CameraSettings(app);
    LightsSettings(app);
    EntitiesSetings(app);
    ImGui::End();
}

void Update(App* app)
{
    float aspectRario = (float)app->displaySize.x / (float)app->displaySize.y;
    glm::mat4 projection = glm::perspective(glm::radians(60.f), aspectRario, app->zNear, app->zFar);
    // UNRELATED RAMI
    //app->cameraPosition = app->camDist * glm::vec3(cos(app->alpha), app->camHeight, sin(app->alpha));
    //app->cameraDirection = glm::vec3(0) - app->cameraPosition;
    
    //glm::mat4 view = glm::lookAt(app->cameraPos, glm::vec3(0.0f), glm::vec3(0.f, 1.f, 0.f));
    //glm::mat4 view = glm::translate(app->cameraPos);
    //view = glm::rotate(view, app->cameraRot.x * DEGTORAD, glm::vec3(1.f, 0.f, 0.f));
    //view = glm::rotate(view, app->cameraRot.y * DEGTORAD, glm::vec3(0.f, 1.f, 0.f));
    //view = glm::rotate(view, app->cameraRot.z * DEGTORAD, glm::vec3(0.f, 0.f, 1.f));
    glm::mat4 rot = glm::rotate(app->cameraRot.y * DEGTORAD, glm::vec3(0, 1, 0));
    rot = glm::rotate(rot, app->cameraRot.z * DEGTORAD, glm::vec3(0, 0, 1));
    rot = glm::rotate(rot, app->cameraRot.x * DEGTORAD, glm::vec3(1, 0, 0));
    glm::mat4 view = glm::lookAt(app->cameraPos, app->cameraPos + glm::vec3(glm::normalize(rot * glm::vec4(0,0,1,0))), glm::vec3(0, 1, 0));

    glBindBuffer(GL_UNIFORM_BUFFER, app->cbuffer.handle);

    //u8* bufferData = (u8*)glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    //u32 bufferHead = 0;
    app->cbuffer.head = 0;
    app->cbuffer.data = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);

    // -- Global Params
    //TODO: cal pujar els global params cada frame???
    AlignHead(app->cbuffer, app->uniformBlockAlignment);
    app->vpParamsOffset = app->cbuffer.head;
    app->vpMatrix = projection * view;
    PushMat4(app->cbuffer, app->vpMatrix);
    app->vpParamsSize = app->cbuffer.head - app->vpParamsOffset;

    AlignHead(app->cbuffer, app->uniformBlockAlignment);
    app->cameraParamsOffset = app->cbuffer.head;
    PushVec3(app->cbuffer, app->cameraPos);
    PushFloat(app->cbuffer, app->zNear);
    PushFloat(app->cbuffer, app->zFar);
    //PushUInt(app->cbuffer, app->lights.size());
    app->cameraParamsSize = app->cbuffer.head - app->cameraParamsOffset;
    
    for (u32 i = 0; i < app->lights.size(); ++i)
    {
        //AlignHead(app->cbuffer, sizeof(vec4));
        AlignHead(app->cbuffer, app->uniformBlockAlignment);
        app->lights[i].lightParamsOffset = app->cbuffer.head;
        
        Light& light = app->lights[i];
        PushVec3(app->cbuffer, light.color);
        PushVec3(app->cbuffer, light.direction);
        //PushVec3(app->cbuffer, light.position);
        //PushUInt(app->cbuffer, light.type);
        //PushFloat(app->cbuffer, light.radius);
        PushVec3(app->cbuffer, light.pos);
        light.lightParamsSize = app->cbuffer.head - light.lightParamsOffset;
    }

    // -- Local Params
    for (int i = 0; i<app->entities.size(); ++i)
    {

        AlignHead(app->cbuffer, app->uniformBlockAlignment);
        app->entities[i].localParamsOffset = app->cbuffer.head;

        PushMat4(app->cbuffer, app->entities[i].worldMatrix);

        app->entities[i].localParamsSize = app->cbuffer.head - app->entities[i].localParamsOffset;
    }
    //pushing light matrices
    for (u32 i = 0; i < app->lights.size(); ++i)
    {
        AlignHead(app->cbuffer, app->uniformBlockAlignment);
        app->lights[i].localParamsOffset = app->cbuffer.head;

        PushMat4(app->cbuffer, app->lights[i].worldMatrix);

        app->lights[i].localParamsSize = app->cbuffer.head - app->lights[i].localParamsOffset;
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

void RenderEntities(App* app)
{
    Program* textureMeshProgram = &app->programs[app->geometryPassIdx];
    glUseProgram(textureMeshProgram->handle);
    for (Entity entity : app->entities)
    {
        //binding local uniform buffer params
        glBindBufferRange(GL_UNIFORM_BUFFER, 1, app->cbuffer.handle, entity.localParamsOffset, entity.localParamsSize);
        Model& model = app->models[entity.modelIndex];
        Mesh& mesh = app->meshes[model.meshIdx];
        for (u32 i = 0; i < mesh.submeshes.size(); ++i) {
            u32 submeshMaterialIdx = model.materialIdx[i];
            Material& submeshMaterial = app->materials[submeshMaterialIdx];
            if (submeshMaterial.normalsTextureIdx != 0 && app->useNormalMap) {
                if(submeshMaterial.bumpTextureIdx != 0 && app->useRelifMap)
                    textureMeshProgram = &app->programs[app->relGeoPassIdx];
                else
                    textureMeshProgram = &app->programs[app->normGeoPassIdx];
                glUseProgram(textureMeshProgram->handle);
                glUniform1i(glGetUniformLocation(textureMeshProgram->handle, "normalMap"), 1);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.normalsTextureIdx].handle);
                if (submeshMaterial.bumpTextureIdx != 0 && app->useRelifMap)
                {
                    glUniform1i(glGetUniformLocation(textureMeshProgram->handle, "heightMap"), 2);
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.bumpTextureIdx].handle);
                }
            }
            glUniform1i(glGetUniformLocation(textureMeshProgram->handle, "uTexture"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);

            GLuint vao = FindVAO(mesh, i, *textureMeshProgram);
            glBindVertexArray(vao);

            Submesh& submesh = mesh.submeshes[i];
            glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
            //TODO: arreglar aquest canvi de shader cada cop
            textureMeshProgram = &app->programs[app->geometryPassIdx];
            glUseProgram(textureMeshProgram->handle);
        }
    }
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

                //glViewport(0,0,app->displaySize.x,app->displaySize.y);

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
                GLenum buffers[5];
                for (unsigned int i = 0; i < 5; ++i)
                    buffers[i] = GL_COLOR_ATTACHMENT0 + i;
                glDrawBuffers(5, buffers);
                
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glStencilMask(0xff);
                glDepthMask(0xff);
                glEnable(GL_DEPTH_TEST);
                glDepthMask(GL_TRUE);
                glDisable(GL_BLEND);
                //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
                
                glBindBufferRange(GL_UNIFORM_BUFFER, 2, app->cbuffer.handle, app->cameraParamsOffset, app->cameraParamsSize);
                glBindBufferRange(GL_UNIFORM_BUFFER, 3, app->cbuffer.handle, app->vpParamsOffset, app->vpParamsSize);
                //Geometry pass
                RenderEntities(app);

                //Lighting pass
                glBindVertexArray(0);
                glUseProgram(0);
                glDrawBuffer(GL_COLOR_ATTACHMENT0);
                glDepthMask(GL_FALSE);
                glDisable(GL_DEPTH_TEST);
                glEnable(GL_BLEND);
                glBlendEquation(GL_FUNC_ADD);
                glBlendFunc(GL_ONE, GL_ONE);

                glClear(GL_COLOR_BUFFER_BIT |GL_STENCIL_BUFFER_BIT);

                for (int i = 0; i < app->lights.size(); ++i)
                {
                    glBindBufferRange(GL_UNIFORM_BUFFER, 0, app->cbuffer.handle, app->lights[i].lightParamsOffset, app->lights[i].lightParamsSize);
                    glBindBufferRange(GL_UNIFORM_BUFFER, 1, app->cbuffer.handle, app->lights[i].localParamsOffset, app->lights[i].localParamsSize);

                    Program* currProgram = &app->programs[app->directionalLightIdx];
                    switch (app->lights[i].type)
                    {
                    case LightType::LightType_Directional: currProgram = &app->programs[app->directionalLightIdx]; break;
                    case LightType::LightType_Point:currProgram = &app->programs[app->pointLightIdx]; break;
                    default: ELOG("Light type unknown: BAD SHADER PROGRAM FOR LIGHT")break;
                    }

                    //render from light point of view to create shadowMap
                    glm::mat4 lightSpaceMatrix;
                    glm::mat4 lightProjection;
                    Program* shadowProgram = &app->programs[app->noFragmentIdx];
                    //if (app->lights[i].type == LightType::LightType_Directional)
                    //{
                        if (app->lights[i].type == LightType::LightType_Directional)
                        {
                            //glm::mat4 lightProjection = glm::ortho(-35.0f, 35.0f, -35.0f, 35.0f, app->zNear, app->zFar);
                            lightProjection = glm::ortho(-35.0f, 35.0f, -35.0f, 35.0f, 0.1f, 75.f);
                            //glm::vec3 zDir = glm::normalize(app->lights[i].direction);
                            //glm::vec3 xDir = glm::cross(zDir, glm::vec3(0, 1, 0));
                            //glm::vec3 up = glm::cross(xDir, zDir);
                            ////glm::mat4 lightView = glm::lookAt(app->zFar * zDir, glm::vec3(0.0f, 0.0f, 0.0f), up);
                            //glm::mat4 lightView = glm::lookAt(app->zFar/5.f * zDir, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0, 1, 0));
                            glm::mat4 lightView = glm::lookAt(20.f * app->lights[i].direction, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0, 1, 0));
                            lightSpaceMatrix = lightProjection * lightView;

                            glBindFramebuffer(GL_FRAMEBUFFER, app->shadowFramebufferHandle);
                            shadowProgram = &app->programs[app->noFragmentIdx];
                            glUseProgram(shadowProgram->handle);
                        }
                        else if (app->lights[i].type == LightType::LightType_Point)
                        {
                            lightProjection = glm::perspective(glm::radians(90.f), 1.0f, 0.1f, app->zFar);
                            glm::mat4 lightSpaceMatrices[] = {
                                lightProjection * glm::lookAt(app->lights[i].pos, app->lights[i].pos + glm::vec3(1.0,0.0,0.0), glm::vec3(0.0,-1.0, 0.0)),
                                lightProjection * glm::lookAt(app->lights[i].pos, app->lights[i].pos + glm::vec3(-1.0,0.0,0.0), glm::vec3(0.0,-1.0, 0.0)),
                                lightProjection * glm::lookAt(app->lights[i].pos, app->lights[i].pos + glm::vec3(0.0,1.0,0.0), glm::vec3(0.0,0.0, 1.0)),
                                lightProjection * glm::lookAt(app->lights[i].pos, app->lights[i].pos + glm::vec3(0.0,-1.0,0.0), glm::vec3(0.0,0.0, -1.0)),
                                lightProjection * glm::lookAt(app->lights[i].pos, app->lights[i].pos + glm::vec3(0.0,0.0,1.0), glm::vec3(0.0,-1.0, 0.0)),
                                lightProjection * glm::lookAt(app->lights[i].pos, app->lights[i].pos + glm::vec3(0.0,0.0,-1.0), glm::vec3(0.0,-1.0, 0.0))
                            };
                        
                            glBindFramebuffer(GL_FRAMEBUFFER, app->shadowPointFramebufferHandle);
                            shadowProgram = &app->programs[app->shadowCubemapIdx];
                            glUseProgram(shadowProgram->handle);
                            glUniformMatrix4fv(glGetUniformLocation(shadowProgram->handle, "shadowMatrices[0]"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrices[0]));
                            glUniformMatrix4fv(glGetUniformLocation(shadowProgram->handle, "shadowMatrices[1]"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrices[1]));
                            glUniformMatrix4fv(glGetUniformLocation(shadowProgram->handle, "shadowMatrices[2]"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrices[2]));
                            glUniformMatrix4fv(glGetUniformLocation(shadowProgram->handle, "shadowMatrices[3]"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrices[3]));
                            glUniformMatrix4fv(glGetUniformLocation(shadowProgram->handle, "shadowMatrices[4]"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrices[4]));
                            glUniformMatrix4fv(glGetUniformLocation(shadowProgram->handle, "shadowMatrices[5]"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrices[5]));
                            glUniform3f(glGetUniformLocation(shadowProgram->handle, "lightPos"), app->lights[i].pos.x, app->lights[i].pos.y, app->lights[i].pos.z);
                            glUniform1f(glGetUniformLocation(shadowProgram->handle, "farPlane"), app->zFar);
                        
                        }

                        glViewport(0, 0, app->shadowMapWidth, app->shadowMapHeight);
                        glEnable(GL_DEPTH_TEST);
                        glDepthMask(0xff);
                        //glDisable(GL_CULL_FACE);
                        glClear(GL_DEPTH_BUFFER_BIT);
                        if (app->lights[i].type == LightType::LightType_Directional)
                        {
                            glBindBuffer(GL_UNIFORM_BUFFER, app->cbuffer.handle);
                            glBufferSubData(GL_UNIFORM_BUFFER, app->vpParamsOffset, app->vpParamsSize, glm::value_ptr(lightSpaceMatrix));
                            glBindBuffer(GL_UNIFORM_BUFFER, 0);
                        }
                        for (Entity entity : app->entities)
                        {
                            //binding local uniform buffer params
                            glBindBufferRange(GL_UNIFORM_BUFFER, 1, app->cbuffer.handle, entity.localParamsOffset, entity.localParamsSize);
                            Model& model = app->models[entity.modelIndex];
                            Mesh& mesh = app->meshes[model.meshIdx];
                            for (u32 i = 0; i < mesh.submeshes.size(); ++i) {
                                GLuint vao = FindVAO(mesh, i, *shadowProgram);
                                glBindVertexArray(vao);
                                //u32 submeshMaterialIdx = model.materialIdx[i];
                                //Material& submeshMaterial = app->materials[submeshMaterialIdx];
                                //glActiveTexture(GL_TEXTURE0);
                                //glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);

                                Submesh& submesh = mesh.submeshes[i];
                                glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
                            }
                        }
                        if (app->lights[i].type == LightType::LightType_Directional)
                        {
                            glBindBuffer(GL_UNIFORM_BUFFER, app->cbuffer.handle);
                            glBufferSubData(GL_UNIFORM_BUFFER, app->vpParamsOffset, app->vpParamsSize, glm::value_ptr(app->vpMatrix));
                            glBindBuffer(GL_UNIFORM_BUFFER, 0);
                        }

                        glBindFramebuffer(GL_FRAMEBUFFER, app->framebufferHandle);
                        glDisable(GL_DEPTH_TEST);
                        glDepthMask(0x00);
                        glViewport(0, 0, app->displaySize.x, app->displaySize.y);
                    //}
                    //End render shadowmaps ---------------------------------------------------------------------------------------------------------   

                    glUseProgram(currProgram->handle);
                    GLint loc = glGetUniformLocation(currProgram->handle, "uTextureAlb");
                    glUniform1i(loc, 0);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, app->ColorAttachmentHandles[1]);
                    loc = glGetUniformLocation(currProgram->handle, "uTextureNorm");
                    glUniform1i(loc, 1);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, app->ColorAttachmentHandles[2]);
                    //loc = glGetUniformLocation(currProgram->handle, "uTextureDepth");
                    //glUniform1i(loc, 2);
                    loc = glGetUniformLocation(currProgram->handle, "uTexturePos");
                    glUniform1i(loc, 2);
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, app->ColorAttachmentHandles[4]);

                    //unsigned int idx = 1;
                    //for (; idx < app->ColorAttachmentHandles.size(); ++idx)
                    //{
                    //    glActiveTexture(GL_TEXTURE0 + (idx - 1));
                    //    glBindTexture(GL_TEXTURE_2D, app->ColorAttachmentHandles[idx]);
                    //}

                    if (app->lights[i].type == LightType::LightType_Directional) {
                        loc = glGetUniformLocation(currProgram->handle, "shadowMap");
                        glUniform1i(loc, 3);
                        loc = glGetUniformLocation(currProgram->handle, "lightSpaceMatrix");
                        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
                        glActiveTexture(GL_TEXTURE3);
                        glBindTexture(GL_TEXTURE_2D, app->shadowDepthAttachmentHandle);
                    }
                    else if (app->lights[i].type == LightType::LightType_Point)
                    {
                        loc = glGetUniformLocation(currProgram->handle, "shadowCubeMap");
                        glUniform1i(loc, 3);
                        glActiveTexture(GL_TEXTURE3);
                        glBindTexture(GL_TEXTURE_CUBE_MAP, app->shadowPointDepthAttachmentHandle);
                    }

                    if (app->lights[i].type == LightType::LightType_Directional) 
                    {
                        glBindVertexArray(app->vao);
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
                    }
                    else
                    {
                        glBindBufferRange(GL_UNIFORM_BUFFER, 1, app->cbuffer.handle, app->lights[i].localParamsOffset, app->lights[i].localParamsSize);

                        glDisable(GL_CULL_FACE);
                        glEnable(GL_DEPTH_TEST); //glDepthMask(GL_FALSE);
                        glEnable(GL_STENCIL_TEST);
                        glStencilMask(GL_TRUE);
                        glStencilFunc(GL_ALWAYS, 0, 0);
                        glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
                        glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);
                        //glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                        glDrawBuffer(GL_NONE);
                        
                        glUseProgram(app->programs[app->noFragmentIdx].handle);
                        Model& model = app->models[app->lights[i].modelIndex];
                        Mesh& mesh = app->meshes[model.meshIdx];
                        for (u32 j = 0; j < mesh.submeshes.size(); ++j) {
                            GLuint vao = FindVAO(mesh, j, *currProgram);
                            glBindVertexArray(vao);
                        
                            Submesh& submesh = mesh.submeshes[j];
                            glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
                        }

                        //glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                        glDrawBuffer(GL_COLOR_ATTACHMENT0);
                        glStencilFunc(GL_NOTEQUAL, 0, 0xFF);
                        glStencilMask(GL_FALSE);
                        glEnable(GL_CULL_FACE);
                        glCullFace(GL_FRONT);
                        glDisable(GL_DEPTH_TEST);

                        glUseProgram(currProgram->handle);
                        model = app->models[app->lights[i].modelIndex];
                        mesh = app->meshes[model.meshIdx];
                        for (u32 j = 0; j < mesh.submeshes.size(); ++j) {
                            GLuint vao = FindVAO(mesh, j, *currProgram);
                            glBindVertexArray(vao);

                            Submesh& submesh = mesh.submeshes[j];
                            glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
                        }

                        glCullFace(GL_BACK);
                        glDisable(GL_STENCIL_TEST);
                        glStencilMask(GL_TRUE);
                        glClear(GL_STENCIL_BUFFER_BIT);
                    }
                }
                glBindVertexArray(0);
                glUseProgram(0);
                
                //screen render pass
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glEnable(GL_BLEND);
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                //glDisable(GL_DEPTH_TEST);
                //glDepthMask(GL_TRUE);
                //glDepthMask(GL_TRUE);
                glDisable(GL_BLEND);
                
                glUseProgram(app->programs[app->texturedGeometryProgramIdx].handle);
                glBindVertexArray(app->vao);               
                
                //glUniform1i(app->programUniformTexture, 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, app->currentAttachmentTextureHandle);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
                glBindTexture(GL_TEXTURE_2D, 0);
                glBindVertexArray(0);
                glUseProgram(0);

            }
            break;

        default:;
    }
}