#include "engine.h"
#include <imgui.h>
#include "engine_ui.h"

void InitializeDocking()
{
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    static bool docking = true;
    if (ImGui::Begin("DockSpace", &docking, window_flags)) {
        // DockSpace
        ImGui::PopStyleVar(3);
        if (docking)
        {
            ImGuiID dockspace_id = ImGui::GetID("DockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
        }

        ImGui::End();
    }
}

void SelectFrameBufferTexture(App*app)
{
    ImGui::Text("Framebuffer output");
    char strMem[8];
    char* currentValue = strMem;
    switch (app->currentAttachmentType)
    {
    case AttachmentOutputs::SCENE: currentValue = (char*)"SCENE"; break;
    case AttachmentOutputs::ALBEDO: currentValue = (char*)"ALBEDO"; break;
    case AttachmentOutputs::NORMALS: currentValue = (char*)"NORMALS"; break;
    case AttachmentOutputs::DEPTH: currentValue = (char*)"DEPTH"; break;
    case AttachmentOutputs::POSITION: currentValue = (char*)"POSITION"; break;
    default:
        break;
    }
    if (ImGui::BeginCombo("##Screen Output", currentValue, ImGuiComboFlags_PopupAlignLeft))
    {
        if (ImGui::Selectable("SCENE")) {
            app->currentAttachmentTextureHandle = app->ColorAttachmentHandles[0];
            app->currentAttachmentType = AttachmentOutputs::SCENE;
        }
        if (ImGui::Selectable("ALBEDO")) {
            app->currentAttachmentTextureHandle = app->ColorAttachmentHandles[1];
            app->currentAttachmentType = AttachmentOutputs::ALBEDO;
        }
        if (ImGui::Selectable("NORMALS")) {
            app->currentAttachmentTextureHandle = app->ColorAttachmentHandles[2];
            app->currentAttachmentType = AttachmentOutputs::NORMALS;
        }
        if (ImGui::Selectable("DEPTH")) {
            app->currentAttachmentTextureHandle = app->ColorAttachmentHandles[3];
            app->currentAttachmentType = AttachmentOutputs::DEPTH;
        }
        if (ImGui::Selectable("POSITION")) {
            app->currentAttachmentTextureHandle = app->ColorAttachmentHandles[4];
            app->currentAttachmentType = AttachmentOutputs::POSITION;
        }
        ImGui::EndCombo();
    }
    ImGui::Separator();
}

void CameraSettings(App* app)
{
    ImGui::Text("Camera Settings");
    ImGui::DragFloat3("Camera Position", (float*)&app->cameraPos, 0.05f, 0.0f, 0.0f, "%.3f", NULL);
    ImGui::DragFloat3("Camera Rotation", (float*)&app->cameraRot, 0.1f, -360.0f, 360.0f, "%.3f", NULL);
    ImGui::DragFloat("Camera Near", (float*)&app->zNear, 0.1f, 0.2f, 4000.0f, "%.3f", NULL);
    ImGui::DragFloat("Camera Far", (float*)&app->zFar, 0.1f, 0.05f, 3999.0f, "%.3f", NULL);
    ImGui::Separator();
}

void LightsSettings(App* app)
{
    if (ImGui::TreeNodeEx("Lights")) 
    {
        if (ImGui::TreeNodeEx("Directional Lights"))
        {
            for (unsigned int i = 0; i < app->lights.size(); ++i)
            {
                Light& light = app->lights[i];
                if (light.type == LightType::LightType_Directional)
                {
                    std::string strLightIndex = std::to_string(i);
                    std::string strLightName = "Light " + strLightIndex;
                    ImGui::Spacing();
                    ImGui::Text(strLightName.c_str());
                    ImGui::ColorEdit3(("color " + strLightName).c_str(), (float*)&light.color, ImGuiColorEditFlags_NoAlpha);
                    ImGui::DragFloat3(("dir " + strLightName).c_str(), (float*)&light.direction, 0.05f, 0.0f, 0.0f, "%.3f", NULL);
                    if (ImGui::Button(("Remove " + strLightName).c_str()))
                        app->lights.erase(app->lights.begin() + i);
                    ImGui::Separator();
                }
            }
            if (ImGui::Button("Add directional light"))
                app->lights.push_back({ vec3(1,1,1), vec3(1,1,1), 0 , LightType::LightType_Directional, TransformScale(vec3(1.f)), app->sphereModelIdx, 0, 0, 0, 0 });
            
            ImGui::TreePop();
        }
        if (ImGui::TreeNodeEx("Point Lights"))
        {
            for (unsigned int i = 0; i < app->lights.size(); ++i)
            {
                Light& light = app->lights[i];
                if (light.type == LightType::LightType_Point)
                {
                    std::string strLightIndex = std::to_string(i);
                    std::string strLightName = "Light " + strLightIndex;
                    ImGui::Spacing();
                    ImGui::Text(strLightName.c_str());
                    ImGui::ColorEdit3(("color " + strLightName).c_str(), (float*)&light.color, ImGuiColorEditFlags_NoAlpha);
                    glm::vec3 pos = light.worldMatrix[3];
                    if (ImGui::DragFloat3(("pos " + strLightName).c_str(), (float*)&pos, 0.05f, 0.0f, 0.0f, "%.3f", NULL))
                        light.worldMatrix = TransformPositionScale(pos, glm::vec3(light.radius));
                    float prevRadius = light.radius;
                    if (ImGui::DragFloat(("radius " + strLightName).c_str(), &light.radius, .2f, 0.f, 3250.f))
                    {
                        light.worldMatrix = glm::scale(light.worldMatrix, glm::vec3(1.f/prevRadius));
                        light.worldMatrix = glm::scale(light.worldMatrix, glm::vec3(light.radius));
                        light.direction = GetAttenuationValuesFromRange(light.radius);
                    }

                    if (ImGui::Button(("Remove " + strLightName).c_str()))
                        app->lights.erase(app->lights.begin() + i);

                    ImGui::Separator();
                }
            }
            if (ImGui::Button("Add point light"))
            {
                float radius = 13.f;
                app->lights.push_back({ vec3(1,1,1), GetAttenuationValuesFromRange(radius), radius , LightType::LightType_Point, TransformPositionScale(vec3(0.f, 0.f, 0.f), vec3(radius)), app->sphereModelIdx, 0, 0, 0, 0 });
            }
            ImGui::TreePop();
        }
        ImGui::TreePop();
    }
    ImGui::Separator();
}

#define DEGTORAD 0.0174533f
void EntitiesSetings(App* app)
{
    if (ImGui::TreeNodeEx("Entities"))
    {
        for (int i = 0; i < app->entities.size(); ++i)
        {
            Entity& entity = app->entities[i];
            ImGui::Spacing();
            ImGui::Text(entity.name.c_str());
            char buffer[128];
            strcpy_s(buffer, entity.name.c_str());
            if (ImGui::InputText((entity.name + std::to_string(i)).c_str(), buffer, (int)(sizeof(buffer) / sizeof(char)), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
                entity.name = buffer;
            glm::vec3 prevPos = entity.pos;
            if (ImGui::DragFloat3(("pos " + entity.name).c_str(), (float*)&entity.pos, 0.05f, 0.0f, 0.0f, "%.3f", NULL))
                entity.worldMatrix = glm::translate(entity.worldMatrix, entity.pos - prevPos);
            glm::vec3 prevRot = entity.rot;
            if (ImGui::DragFloat3(("rot " + entity.name).c_str(), (float*)&entity.rot, 0.3f, -360.f, 360.0f, "%.3f", NULL))
            {
                glm::vec3 rotation = (entity.rot - prevRot) * DEGTORAD;
                entity.worldMatrix = glm::rotate(entity.worldMatrix, rotation.x, glm::vec3(1.f, 0.f, 0.f));
                entity.worldMatrix = glm::rotate(entity.worldMatrix, rotation.y, glm::vec3(0.f, 1.f, 0.f));
                entity.worldMatrix = glm::rotate(entity.worldMatrix, rotation.z, glm::vec3(0.f, 0.f, 1.f));
            }
            glm::vec3 prevScale = entity.scale;
            if (ImGui::DragFloat3(("scale " + entity.name).c_str(), (float*)&entity.scale, 0.02f, 0.0f, 0.0f, "%.3f", NULL))
            {
                entity.worldMatrix = glm::scale(entity.worldMatrix, glm::vec3(1.f / prevScale));
                entity.worldMatrix = glm::scale(entity.worldMatrix, entity.scale);
            }
            if (ImGui::Button(("Remove " + entity.name).c_str()))
                app->entities.erase(app->entities.begin() + i);
            ImGui::Separator();
        }
        if (ImGui::Button("Add Sphere"))
        {
            Entity sphere = {};
            sphere.worldMatrix = TransformPositionScale(vec3(0.f), vec3(1.f));
            sphere.rot = vec3(0.f);
            sphere.scale = vec3(1.f);
            sphere.worldMatrix = TransformPositionScale(sphere.pos, sphere.scale);
            sphere.modelIndex = app->sphereModelIdx;
            sphere.localParamsOffset = 0;
            sphere.localParamsSize = 0;
            sphere.name = "Sphere " + std::to_string(app->entities.size());
            app->entities.push_back(sphere);
        }
        if (ImGui::Button("Add Patrick"))
        {
            Entity patrick = {};
            patrick.pos = vec3(0.f);
            patrick.rot = vec3(0.f);
            patrick.scale = vec3(0.45f);
            patrick.worldMatrix = TransformPositionScale(patrick.pos, patrick.scale);
            patrick.modelIndex = app->patrickModelIdx;
            patrick.localParamsOffset = 0;
            patrick.localParamsSize = 0;
            patrick.name = "Patrick " + std::to_string(app->entities.size());
            app->entities.push_back(patrick);
        }
        if (ImGui::Button("Add Plane"))
        {
            Entity plane = {};
            plane.pos = vec3(0.f, 0.f, 0.f);
            plane.rot = vec3(0.f);
            plane.scale = vec3(1.f);
            plane.worldMatrix = TransformPositionScale(plane.pos, plane.scale);
            plane.modelIndex = app->planeModelIdx;
            plane.localParamsOffset = 0;
            plane.localParamsSize = 0;
            plane.name = "Plane " + std::to_string(app->entities.size());
            app->entities.push_back(plane);
        }
        ImGui::TreePop();
    }
    ImGui::Separator();
}