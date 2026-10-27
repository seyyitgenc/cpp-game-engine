#include "PropertyEditor.hpp"

#include <algorithm>
#include <cfloat>
#include <climits>
#include <cmath>
#include <string>
#include <typeindex>
#include <variant>

#include <glm/glm.hpp>
#include <glm/gtc/color_space.hpp>
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>

#include "ECS/Component/Property.hpp"
#include "Resource/Resource.hpp"
#include "Resource/ResourceManager.hpp"
#include "Utils/Overloaded.hpp"

namespace GNC {

namespace {

struct Field {
    const Component& component;
    const Property& property;
    const PropertyInfo& info;
    ResourceManager& resources;

    const char* Label() const { return info.name.c_str(); }
    const char* Format(const char* fallback) const { return info.format.empty() ? fallback : info.format.c_str(); }
    float Step(float fallback) const { return info.step > 0.0f ? info.step : fallback; }
};

// ------
// Ranges
// ------
// ImGui only clamps when min < max, so a one-sided bound gets the widest
// other end.
struct Range {
    float min = 0.0f;
    float max = 0.0f;
    ImGuiSliderFlags flags = ImGuiSliderFlags_None;
};

Range MakeRange(const PropertyInfo& info, float scale = 1.0f) {
    if (!info.min && !info.max) return {};
    return {
        info.min ? *info.min * scale : -FLT_MAX,
        info.max ? *info.max * scale : FLT_MAX,
        ImGuiSliderFlags_AlwaysClamp
    };
}

// ------------
// Value Fields
// ------------
bool EditEnum(const Field& field, int& value) {
    const std::vector<std::string>& names = field.info.enumNames;
    const bool known = value >= 0 && value < static_cast<int>(names.size());
    if (!ImGui::BeginCombo(field.Label(), known ? names[value].c_str() : "?")) return false;

    bool changed = false;
    for (int i = 0; i < static_cast<int>(names.size()); ++i) {
        if (ImGui::Selectable(names[i].c_str(), i == value)) {
            changed = i != value;
            value = i;
        }
    }
    ImGui::EndCombo();
    return changed;
}

bool EditInt(const Field& field, int& value) {
    if (field.info.hint == PropertyHint::Enum) return EditEnum(field, value);

    // Halved for the open end, so ImGui's max - min cannot overflow.
    int min = 0;
    int max = 0;
    ImGuiSliderFlags flags = ImGuiSliderFlags_None;
    if (field.info.min || field.info.max) {
        min = field.info.min ? static_cast<int>(*field.info.min) : INT_MIN / 2;
        max = field.info.max ? static_cast<int>(*field.info.max) : INT_MAX / 2;
        flags = ImGuiSliderFlags_AlwaysClamp;
    }
    return ImGui::DragInt(field.Label(), &value, field.Step(1.0f), min, max, field.Format("%d"), flags);
}

bool EditFloat(const Field& field, float& value) {
    if (field.info.hint == PropertyHint::Angle) {
        const Range range = MakeRange(field.info, glm::degrees(1.0f));
        float degrees = glm::degrees(value);
        if (!ImGui::DragFloat(field.Label(), &degrees, field.Step(0.2f), range.min, range.max, field.Format("%.1f°"), range.flags)) return false;

        value = glm::radians(degrees);
        return true;
    }

    const Range range = MakeRange(field.info);
    const float step = field.info.proportional ? std::max(std::abs(value) * 0.01f, 0.01f) : field.Step(0.01f);
    return ImGui::DragFloat(field.Label(), &value, step, range.min, range.max, field.Format("%.3f"), range.flags);
}

bool EditVec3(const Field& field, glm::vec3& value) {
    if (field.info.hint == PropertyHint::Color) {
        glm::vec3 srgb = glm::convertLinearToSRGB(value);
        if (!ImGui::ColorEdit3(field.Label(), &srgb.x)) return false;

        value = glm::convertSRGBToLinear(srgb);
        return true;
    }

    const Range range = MakeRange(field.info);
    return ImGui::DragFloat3(field.Label(), &value.x, field.Step(0.01f), range.min, range.max, field.Format("%.3f"), range.flags);
}

// Angles recomputed from the quaternion every frame jump mid-drag near
// +-90° pitch, so the shown angles are kept while the rotation is still the
// one they made.
struct EulerCache {
    const Component* component = nullptr;
    const Property* property = nullptr;
    glm::quat rotation {};
    glm::vec3 degrees {};
};
EulerCache eulerCache;

bool EditRotation(const Field& field, glm::quat& value) {
    if (eulerCache.component != &field.component || eulerCache.property != &field.property || eulerCache.rotation != value) {
        // + 0 turns -0, which would show as "-0.0°", into 0.
        const glm::vec3 degrees = glm::degrees(glm::eulerAngles(value)) + glm::vec3(0.0f);
        eulerCache = { &field.component, &field.property, value, degrees };
    }
    if (!ImGui::DragFloat3(field.Label(), &eulerCache.degrees.x, field.Step(0.5f), 0.0f, 0.0f, field.Format("%.1f°"))) return false;

    value = glm::quat(glm::radians(eulerCache.degrees));
    eulerCache.rotation = value;
    return true;
}

bool EditResource(const Field& field, Resource*& value) {
    const char* current = value ? value->GetId().c_str() : "(none)";
    if (!field.info.resourceType) {
        ImGui::LabelText(field.Label(), "%s", current);
        return false;
    }
    if (!ImGui::BeginCombo(field.Label(), current)) return false;

    const std::type_index type(*field.info.resourceType);
    bool changed = false;
    for (const std::string& id : field.resources.GetResourceIds(type)) {
        Resource* candidate = field.resources.GetResource(type, id);
        if (ImGui::Selectable(id.c_str(), candidate == value)) {
            changed = candidate != value;
            value = candidate;
        }
    }
    ImGui::EndCombo();
    return changed;
}

bool EditValue(const Field& field, PropertyValue& value) {
    return std::visit(Overloaded {
        [&](bool& v) { return ImGui::Checkbox(field.Label(), &v); },
        [&](int& v) { return EditInt(field, v); },
        [&](float& v) { return EditFloat(field, v); },
        [&](glm::vec3& v) { return EditVec3(field, v); },
        [&](glm::quat& v) { return EditRotation(field, v); },
        [&](Resource*& v) { return EditResource(field, v); },
    }, value);
}

} // namespace

bool EditProperties(Component& component, const std::vector<Property>& properties, ResourceManager& resources) {
    bool changed = false;
    // Of the last visible property.
    std::string category;

    for (const Property& property : properties) {
        PropertyInfo info = property.info;
        if (property.adjust) property.adjust(component, info);
        if (!info.visible) continue;

        if (!info.category.empty() && info.category != category) {
            ImGui::SeparatorText(info.category.c_str());
        }
        category = info.category;

        PropertyValue value = property.get(component);
        if (EditValue({ component, property, info, resources }, value)) {
            property.set(component, value);
            changed = true;
        }
    }
    return changed;
}

} // namespace GNC
