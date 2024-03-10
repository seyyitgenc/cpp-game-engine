#pragma once
#include "../globals.h"

inline unsigned int guiFrameBuffer;
inline unsigned int sceneFrameBuffer;
inline unsigned int guiBuffer;
inline unsigned int sceneBuffer;
inline unsigned int guiRbo;
inline unsigned int sceneRbo;
// note: temp solution
inline void prepareRenderBuffer(){
    // frame buffer
    glGenFramebuffers(1, &guiFrameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, guiFrameBuffer);

    // gui color buffer
    glGenTextures(1, &guiBuffer);
    glBindTexture(GL_TEXTURE_2D, guiBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, guiBuffer, 0);
    
    // guiRbo
    glGenRenderbuffers(1, &guiRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, guiRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCREEN_WIDTH, SCREEN_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, guiRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
   
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

    // frame buffer
    glGenFramebuffers(1, &sceneFrameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFrameBuffer);

    // scene color buffer
    glGenTextures(1, &sceneBuffer);
    glBindTexture(GL_TEXTURE_2D, sceneBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneBuffer, 0);
    
    // sceneRbo
    glGenRenderbuffers(1, &sceneRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, sceneRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCREEN_WIDTH, SCREEN_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, sceneRbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// framebuffer must be differantiable
// for ex. bloom framebuffer, deferred shading framebuffer, shadowmap framebuffer

class FrameBuffer
{
public:
    FrameBuffer() = default;
    ~FrameBuffer() = default;

private:
    GLuint texture;
    GLuint buffer;
    GLuint rbo;

};

class Renderer
{
    Renderer() = default;
    ~Renderer() = default;
    // void addToRenderQueue(const Model& test){
    //     modelList.push_back(std::make_unique<Model>(test));
    // }
    // void draw(){
    //     for (auto &&i : modelList)
    //     {
    //         // i->Draw();
    //     }
        
    // }
    // std::vector<std::unique_ptr<Model>> modelList;
};