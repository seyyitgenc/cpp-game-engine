#include "framebuffer.h"

// TODO: i may use GLenum for texture type spec it can make my code simpler

// Configures FrameBufferTexture Depending on it's own values
void FrameBufferTexture::Configure(){
    glGenTextures(1, &_texture);
    glBindTexture(GL_TEXTURE_2D, _texture);
    // NOTE: for now this is commong spec
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // NOTE: temp variable
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};

    switch (_type)
    {
    // - position color buffer
    case FBTT::POSITION:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, _width, _height, 0, GL_RGBA, GL_FLOAT, NULL);
        glFramebufferTexture2D(GL_FRAMEBUFFER, _attachment, GL_TEXTURE_2D, _texture, 0);
        break;
    // - normal color buffer
    case FBTT::NORMAL:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, _width, _height, 0, GL_RGBA, GL_FLOAT, NULL);
        glFramebufferTexture2D(GL_FRAMEBUFFER, _attachment, GL_TEXTURE_2D, _texture, 0);
        break;
    // - color buffer
    case FBTT::ALBEDO:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glFramebufferTexture2D(GL_FRAMEBUFFER, _attachment, GL_TEXTURE_2D, _texture, 0);
        break;
    case FBTT::SHADOW:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, _width, _height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, _attachment, GL_TEXTURE_2D, _texture, 0);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        break;
    case FBTT::SPECULAR:
        Log::write(Log::Fatal, RED_TEXT("FRAMEBUFFERTEXTURE::CONFIGURE::FBTT::SPECULAR is not implemented\n"));
        break;
    case FBTT::DISPLACEMENT:
        Log::write(Log::Fatal, RED_TEXT("FRAMEBUFFERTEXTURE::CONFIGURE::FBTT::DEPTH is not implemented\n"));
        break;
    case FBTT::ROUGHNESS:
        Log::write(Log::Fatal, RED_TEXT("FRAMEBUFFERTEXTURE::CONFIGURE::FBTT::ROUGHNESS is not implemented\n"));
        break;
    default:
        Log::write(Log::Warning, RED_TEXT("Given Type Is Not Valid!\n"));
        break;
    }
}

// FrameBuffer constructor
FrameBuffer::FrameBuffer(){
    glGenFramebuffers(1, &_framebuffer);
}

// Sets current frame buffer to itself
void FrameBuffer::bind(){
    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
}

// Sets current frame buffer to default
void FrameBuffer::unbind(){
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Attaches render buffer and configures it
void FrameBuffer::attachRenderBuffer(){
    bind();
    glGenRenderbuffers(1,&_renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, _renderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCREEN_WIDTH,SCREEN_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _renderbuffer);
}

// FIXME: make this bind textures useful for multiple frame buffers.
// Binds all used textures used by this framebuffer. Starting from GL_TEXTURE_0.
void FrameBuffer::bindTextures(){
    for (size_t i = 0; i < _boundTextures.size(); i++){
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, _boundTextures[i]._texture);
    }
}

// Attaches a texture to it's framebuffer
void FrameBuffer::attachTexture(int width, int height, FBTT type,GLenum attachment){
    bind();
    FrameBufferTexture bufferTexture(width, height, type, attachment);
    _boundTextures.push_back(bufferTexture);
    std::vector<GLenum> attachments;

    for (auto &&i : _boundTextures)
        attachments.push_back(i._attachment);

    glDrawBuffers(attachments.size(), attachments.data());
}

// Checks if generated framebuffer is complete or not.
void FrameBuffer::checkCompleteness(){
    // tell opengl which color attachments we will use (of this framebuffer) for rendering
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        Log::write(Log::Fatal, LIGHT_RED_TEXT("FATAL::FRAMEBUFFER::CHECK_COMPLETENESS Framebuffer not complete! Application may crush or doesn't show expected results"));
}