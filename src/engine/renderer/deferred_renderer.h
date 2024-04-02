#pragma once

#include "framebuffer.h"

class DeferredRenderer
{
public:
    DeferredRenderer(int width, int height){};
    void StartRenderToFBO(FrameBuffer buffer){};
    void StopRenderToFBO(){};

    void StartRenderToShadowMap(FrameBuffer buffer){};
    void StopRenderToShadowMap(){};
    // showTexture();
    // showShadowMap();
    void render(){};
private:
    glm::mat4 _worldToLightViewMatrix;
    glm::mat4 _lightViewToProjectionMatrix;
    glm::mat4 _worldToCameraViewMatrix;
};