#include "dummy.hpp"

#include <glm/gtc/type_ptr.hpp>

namespace GNC {

Dummy::Dummy(const std::string& modelPath)
    : m_model(std::make_unique<Model>(modelPath))
{
}

void Dummy::event(float deltaTime) { }

void Dummy::update(float deltaTime) { }

void Dummy::draw(const Shader& shader)
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, m_scale);
    model = glm::translate(model, m_position);

    // TODO: later on abstract this into uniform class.
    shader.setMat4("model", model);
    m_model->Draw(shader);
}

} // namespace GNC
