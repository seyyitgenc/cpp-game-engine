#pragma once
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"
#include "../texture.h"

// todo: check out dependency injection
namespace Gui{
    void Init(){
        // todo : implement imgui::layer kind of thingy

        ImGui::ShowDemoWindow();
        ImGui::SetNextWindowSize(ImVec2(800, 440), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Shaders"))
        {
            // Left
            static int selected = 0;
            static Shader *selectedShader = nullptr; // note: don't forget to free this pointer
            static std::string shaderName;
            {
                float windowWidth = ImGui::GetContentRegionAvail().x;
                ImGui::BeginChild("left pane", ImVec2(windowWidth / 2, 0), true);
                
                float tblWidth = ImGui::GetContentRegionAvail().x;
                float padding = 5;
                float cellSize = THUMBNAIL_SIZE + padding;
                int columnCount = tblWidth / cellSize;
                if(tblWidth < cellSize)
                    columnCount = 1;
                
                if (ImGui::BeginTable("tbl_shaders", columnCount))
                {
                    for (int index = 0; auto &&it : gShaderManager->get_shader_list())
                    {
                        ImGui::TableNextColumn();
                        ImVec2 pos = ImGui::GetCursorPos();
                        std::string itemid = "##" + it.first;
                        // ImGui::PushID(itemid.c_str());
                        ImGui::SetCursorPos({pos.x,pos.y});
                        if (ImGui::Selectable(itemid.c_str(), index == selected,0 ,{cellSize,cellSize}))
                        {
                            selected = index;
                            selectedShader = it.second.get();
                            shaderName = it.first;
                        }
                        // note: temporary solution
                        ImGui::SetCursorPos({pos.x + padding / 2,pos.y + padding / 2});

                        ImGui::Image((void*)(intptr_t)gTextureManager->getTextureId("file-icon"), {THUMBNAIL_SIZE, THUMBNAIL_SIZE});

                        ImGui::SetCursorPos({pos.x,pos.y + cellSize + 2});
                        ImGui::TextWrapped("%s",it.first.c_str());
                        index++;
                    }
                    ImGui::EndTable();
                }
                ImGui::EndChild();
            }
            ImGui::SameLine();
            ImGui::BeginGroup();
            // Right
            if(selectedShader != nullptr){
                ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us
                ImGui::Separator();
                if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
                {
                    ShaderInfo info = selectedShader->getShaderInfo(); 
                    // todo: enable this after adding description property to the shader
                    if (ImGui::BeginTabItem("Attributes")){
                        ImGui::Text("-------------------- WIP --------------------");
                        // static glm::vec3 pos;
                        // pos =  selectedShader->getVec3("light.direction");
                        // static float test[3] = {pos.x,pos.y,pos.z};
                        // ImGui::SliderFloat3("direction",test,-1,1);
                        // auto zibap = App::getInstance().dLight->getProperties<DirectionalLightProperties>();
                        // zibap->direction = glm::make_vec3(test);
                        ImGui::EndTabItem();
                    }
                    selectedShader->getShaderInfo().drawInfo();
                    ImGui::EndTabBar();
                }
                ImGui::EndChild();
            }
            if (ImGui::Button("Reload")) {
                gShaderManager->reloadShader(shaderName);
            }
            ImGui::SameLine();
            if (ImGui::Button("Reload All Shaders")) {
                gShaderManager->reloadAllShaders();
            }
            
            ImGui::EndGroup();
        }
        
        ImGui::End();
    }
}