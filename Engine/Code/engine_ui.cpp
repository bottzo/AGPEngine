#include "engine.h"
#include <imgui.h>

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
    ImGui::Text("FPS: %f", 1.0f / app->deltaTime);
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
    ImGui::DragFloat3("Camera Position", (float*)&app->cameraPos, 0.05f, 0.0f, 0.0f, "%.3f", NULL);
    ImGui::DragFloat3("Camera Rotation", (float*)&app->cameraRot, 0.1f, -360.0f, 360.0f, "%.3f", NULL);
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
                    ImGui::Separator();
                    ImGui::Text(strLightName.c_str());
                    ImGui::ColorEdit3(("color " + strLightName).c_str(), (float*)&light.color, ImGuiColorEditFlags_NoAlpha);
                    ImGui::DragFloat3(("dir " + strLightName).c_str(), (float*)&light.direction, 0.05f, 0.0f, 0.0f, "%.3f", NULL);
                    if (ImGui::Button(("Remove " + strLightName).c_str()))
                        app->lights.erase(app->lights.begin() + i);
                    ImGui::Separator();
                }
            }
            if (ImGui::Button("Add directional light"))
                app->lights.push_back({ vec3(1,1,1), vec3(0,1,-1), 0 , LightType::LightType_Directional, TransformScale(vec3(1.f)), app->sphereModelIdx, 0, 0, 0, 0 });
            
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
                    ImGui::Separator();
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