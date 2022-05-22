//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#include "platform.h"
#include <glad/glad.h>

typedef glm::vec2  vec2;
typedef glm::vec3  vec3;
typedef glm::vec4  vec4;
typedef glm::ivec2 ivec2;
typedef glm::ivec3 ivec3;
typedef glm::ivec4 ivec4;

struct Image
{
    void* pixels;
    ivec2 size;
    i32   nchannels;
    i32   stride;
};

struct Texture
{
    GLuint      handle;
    std::string filepath;
};

struct VertexBufferAttribute
{
    u8 location;
    u8 componentCount;
    u8 offset;
};

struct VertexBufferLayout
{
    std::vector<VertexBufferAttribute> attributes;
    u8 stride;
};

struct VertexShaderAttribute
{
    u8 location;
    u8 componentCount;
};

struct VertexShaderLayout
{
    std::vector<VertexShaderAttribute> attributes;
};

struct Vao
{
    GLuint handle;
    GLuint programHandle;
};

struct Program
{
    GLuint             handle;
    std::string        filepath;
    std::string        programName;
    u64                lastWriteTimestamp; // What is this for?
    VertexShaderLayout vertexInputLayout;
};

struct Model {
    u32 meshIdx;
    std::vector<u32> materialIdx;
};

struct Submesh {
    VertexBufferLayout vertexBufferLayout;
    std::vector<float> vertices;
    std::vector<u32> indices;
    u32 vertexOffset;
    u32 indexOffset;

    std::vector<Vao> vaos;
};

struct Mesh {
    std::vector<Submesh> submeshes;
    GLuint vertexBufferHandle;
    GLuint indexBufferHandle;
};

struct Entity {
    glm::mat4 worldMatrix;
    glm::vec3 pos;
    glm::vec3 rot;
    glm::vec3 scale;
    u32 modelIndex;
    u32 localParamsOffset;
    u32 localParamsSize;
    std::string name;
};

struct Material
{
    std::string name;
    vec3 albedo;
    vec3 emissive;
    f32 smoothness;
    u32 albedoTextureIdx;
    u32 emissiveTextureIdx;
    u32 specularTextureIdx;
    u32 normalsTextureIdx;
    u32 bumpTextureIdx;
};

enum LightType
{
    LightType_Directional,
    LightType_Point
};

struct Light 
{
    vec3 color;
    vec3 direction;
    //vec3 position;
    float radius;

    //CPU side
    LightType type;
    glm::mat4 worldMatrix;
    u32 modelIndex;
    u32 localParamsOffset;
    u32 localParamsSize;
    u32 lightParamsOffset;
    u32 lightParamsSize;
};

struct Buffer
{
    GLuint handle;
    GLenum type;
    u32 size;
    u32 head;
    void* data; //mapped data
};

enum Mode
{
    Mode_TexturedQuad,
    Mode_Patrick,
    Mode_Count
};

enum AttachmentOutputs {
    SCENE,
    ALBEDO,
    NORMALS,
    DEPTH,
    POSITION
};

struct App
{
    // Loop
    f32  deltaTime;
    bool isRunning;

    // Input
    Input input;

    // Graphics
    char gpuName[64];
    char openGlVersion[64];

    ivec2 displaySize;

    std::vector<Texture>  textures;
    std::vector<Program>  programs;
    std::vector<Material> materials;
    std::vector<Mesh> meshes;
    std::vector<Model> models;
    std::vector<Entity> entities;
    std::vector<Light> lights;

    //Model indices
    u32 sphereModelIdx;
    u32 planeModelIdx;
    u32 patrickModelIdx;

    // program indices
    u32 texturedGeometryProgramIdx;
    u32 patrickProgramIdx;
    u32 geometryPassIdx;
    u32 directionalLightIdx;
    u32 pointLightIdx;
    u32 noFragmentIdx;
    
    // texture indices
    u32 diceTexIdx;
    u32 whiteTexIdx;
    u32 blackTexIdx;
    u32 normalTexIdx;
    u32 magentaTexIdx;

    // Mode
    Mode mode;

    // Embedded geometry (in-editor simple meshes such as
    // a screen filling quad, a cube, a sphere...)
    GLuint embeddedVertices;
    GLuint embeddedElements;

    // Location of the texture uniform in the textured quad shader
    GLuint programUniformTexture;
    GLuint patrickProgramUniform;
    int uniformBlockAlignment;
    u32 cameraParamsOffset;
    u32 cameraParamsSize;
    Buffer cbuffer;

    // VAO object to link our screen filling quad with our textured quad shader
    GLuint vao;

    AttachmentOutputs currentAttachmentType = AttachmentOutputs::SCENE;
    GLuint currentAttachmentTextureHandle = 0;
    GLuint framebufferHandle = 0;
    std::vector<GLuint> ColorAttachmentHandles;
    GLuint depthAttachmentHandle = 0;

    //Camera Settings
    glm::vec3 cameraPos = glm::vec3(0.f, -1.f, -4.f);
    glm::vec3 cameraRot = glm::vec3(0.f);
    float zNear = 0.1f;
    float zFar = 100.f;
};

void Init(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);

u32 LoadTexture2D(App* app, const char* filepath);
glm::mat4 TransformScale(const glm::vec3& scaleFactors);
glm::mat4 TransformPositionScale(const glm::vec3& pos, const glm::vec3& scaleFactor);
constexpr vec3 GetAttenuationValuesFromRange(unsigned int range);