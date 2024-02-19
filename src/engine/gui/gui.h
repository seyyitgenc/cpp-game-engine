#pragma once
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"
#include "../texture.h"

// todo: check out dependency injection
namespace Gui{
    void Init(){
        // todo : implement imgui::layer kind of thingy
        // start dear ImGui frame
        static float f0 = 0,f1 = 0,f2 = 0;
        static float colboxSizex = 0,colboxSizey = 0,colboxSizez = 0;
        static float colbox0 = 0, colbox1 = 0, colbox2 = 1;
        Texture* test = TextureManager::getInstance()->getTexture("doesn'txexsit");


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

                        ImGui::Image((void*)(intptr_t)TextureManager::getInstance()->getTextureId("file-icon"), {THUMBNAIL_SIZE, THUMBNAIL_SIZE});

                        ImGui::SetCursorPos({pos.x,pos.y + cellSize + 2});
                        ImGui::TextWrapped("%s",it.first.c_str());
                        index++;
                        ImGui::TableNextColumn();
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
                    // todo: enable this after adding description property to the shader
                    // if (ImGui::BeginTabItem("Description"))
                    // {
                    //     ImGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. ");
                    //     ImGui::EndTabItem();
                    // }
                    if (ImGui::BeginTabItem("Details"))
                    {
                        ShaderInfo info = selectedShader->getShaderInfo(); 
                        ImGui::Text("Shader ID : %d", info.ID);
                        ImGui::Text("Shader Vertex Path   : %s", info.vertexPath.c_str());
                        ImGui::Text("Shader Fragment Path : %s", info.fragmentPath.c_str());
                        ImGui::Text("Shader Geometry Path : %s", info.geometryPath.c_str());
                        ImGui::EndTabItem();
                    }
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
        // // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        // static float f = 0.0f;
        // static int counter = 0;
        // ImGui::Begin("Debug Window");                           // Create a window called "Debug Window" and append into it.

        // ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

        // ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
        // ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        // ImGui::Text("Object Location");               // for now this represents the cyborgs location
        // ImGui::PushItemWidth(120);
        // // note: imagine you have lots of objects you must list all of them
        // // note: and can edit their location in the world.
        // ImGui::SliderFloat("X", &colbox0, -10.0f, 10.0f); ImGui::SameLine();
        // ImGui::SliderFloat("Y", &colbox1, -10.0f, 10.0f); ImGui::SameLine();
        // ImGui::SliderFloat("Z", &colbox2, -10.0f, 10.0f);
        
        // ImGui::Text("Object Size");
        // ImGui::SliderFloat("w", &colboxSizex, -10.0f, 10.0f); ImGui::SameLine();
        // ImGui::SliderFloat("h", &colboxSizey, -10.0f, 10.0f); ImGui::SameLine();
        // ImGui::SliderFloat("d", &colboxSizez, -10.0f, 10.0f);

        // ImGui::Text("Point Pos x: %.6f y: %.6f z: %.6f",picking_pos.x, picking_pos.y, picking_pos.z);
        // ImGui::End();
        // glDeleteTextures(0,&my_img_texture);
    }
}