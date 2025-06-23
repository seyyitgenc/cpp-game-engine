#pragma once

#include <string>

class ImGuiLayer {
private:
public:
    explicit ImGuiLayer(const std::string &name) :
            m_name(name) {
    }
    ~ImGuiLayer() = default;

    std::string name() {
        return m_name;
    }

    void setName(const std::string &name) {
        m_name = name;
    }

    virtual void setVisible(bool visible) { m_visible = visible; }

    virtual bool *visible() { return &m_visible; }
    virtual bool isVisible() { return m_visible; }

    virtual void draw() = 0;

protected:
    std::string m_name = "";
    bool m_visible = false;
};
