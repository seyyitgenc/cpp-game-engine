#pragma once

#include "Component.hpp"

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <variant>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace GNC {

class Resource;
struct Material;

// Properties
// A component type's properties as data, so code that does not know the type
// (the Inspector, scene files) can read and write them. A type lists them in
// a static Reflect(PropertyList<T>&), which ComponentRegistry calls.

// ------
// Values
// ------
// An enum is held as its int value. A Resource* is null or a resource of
// PropertyInfo::resourceType. A Material* is one of the scene's.
using PropertyValue = std::variant<bool, int, float, glm::vec3, glm::quat, Resource*, Material*>;

enum class PropertyHint {
    None,
    Color, // vec3: linear RGB, edited as sRGB
    Angle, // float: radians (min and max too), edited in degrees
    Enum,  // int: an index into PropertyInfo::enumNames
};

// ----
// Info
// ----
struct PropertyInfo {
    std::string name {};
    // The scene file's name for it ("innerConeAngle"), apart from the label so
    // renaming one does not break saved scenes. Empty: not saved, for a
    // property that shows what is saved elsewhere (a mesh's material values).
    std::string key {};
    // Shown as a heading above a run of properties with the same category.
    std::string category {};
    PropertyHint hint = PropertyHint::None;
    // Limits while editing; setters still enforce their own.
    std::optional<float> min {};
    std::optional<float> max {};
    // Drag step per pixel, in the unit shown. 0: the editor's default.
    float step = 0.0f;
    // The step grows with the value, for quantities that span orders of
    // magnitude (light intensities).
    bool proportional = false;
    // May carry a unit ("%.2f cd"). Empty: the editor's default.
    std::string format {};
    std::vector<std::string> enumNames {};
    // The scene file's name for each enum value, in enumNames' order; written
    // instead of the int, so reordering the enum keeps old files' meaning.
    std::vector<std::string> enumKeys {};
    // A Resource* property's type, e.g. &typeid(Mesh).
    const std::type_info* resourceType = nullptr;
    bool visible = true;
};

// --------
// Property
// --------
// get and set must only be given components of the type the property was
// described for. The size_t is the element's index for an element property,
// and ignored by the others.
struct Property {
    PropertyInfo info;
    // A default value of the property's type, for code that needs the type
    // without a component to get from (reading a scene file).
    PropertyValue prototype;
    std::function<PropertyValue(const Component&, std::size_t)> get;
    std::function<void(Component&, std::size_t, const PropertyValue&)> set;
    // Optional: changes a copy of info for one component before it is drawn
    // (hides a property that does not apply, switches a unit).
    std::function<void(const Component&, std::size_t, PropertyInfo&)> adjust;
    // Element properties only: how many elements the component has now.
    std::function<std::size_t(const Component&)> count;
};

template<typename Value, typename Variant>
struct IsAlternativeOf : std::false_type {};

template<typename Value, typename... Alternatives>
struct IsAlternativeOf<Value, std::variant<Alternatives...>> : std::bool_constant<(std::is_same_v<Value, Alternatives> || ...)> {};

// -------------
// Property List
// -------------
// What T::Reflect() fills in, in the order the editor shows them.
template<typename T>
class PropertyList {
private:
    std::vector<Property> properties;

public:
    template<typename Value, typename Get, typename Set>
    void Add(PropertyInfo info, Get get, Set set) {
        static_assert(IsAlternativeOf<Value, PropertyValue>::value, "Value must be one of PropertyValue's types");

        properties.push_back(Property {
            .info = std::move(info),
            .prototype = PropertyValue(std::in_place_type<Value>),
            .get = [get](const Component& component, std::size_t) -> PropertyValue {
                return Value(get(static_cast<const T&>(component)));
            },
            .set = [set](Component& component, std::size_t, const PropertyValue& value) {
                set(static_cast<T&>(component), std::get<Value>(value));
            },
            .adjust = {},
            .count = {}
        });
    }

    template<typename Value, typename Get, typename Set, typename Adjust>
    void Add(PropertyInfo info, Get get, Set set, Adjust adjust) {
        Add<Value>(std::move(info), get, set);
        properties.back().adjust = [adjust](const Component& component, std::size_t, PropertyInfo& adjusted) {
            adjust(static_cast<const T&>(component), adjusted);
        };
    }

    // A value per element of a list the component holds (a mesh's material
    // slots). get, set and adjust take the element's index after the
    // component. The editor shows a run of them with one category element
    // by element.
    template<typename Value, typename Count, typename Get, typename Set, typename Adjust>
    void AddElement(PropertyInfo info, Count count, Get get, Set set, Adjust adjust) {
        static_assert(IsAlternativeOf<Value, PropertyValue>::value, "Value must be one of PropertyValue's types");

        properties.push_back(Property {
            .info = std::move(info),
            .prototype = PropertyValue(std::in_place_type<Value>),
            .get = [get](const Component& component, std::size_t element) -> PropertyValue {
                return Value(get(static_cast<const T&>(component), element));
            },
            .set = [set](Component& component, std::size_t element, const PropertyValue& value) {
                set(static_cast<T&>(component), element, std::get<Value>(value));
            },
            .adjust = [adjust](const Component& component, std::size_t element, PropertyInfo& adjusted) {
                adjust(static_cast<const T&>(component), element, adjusted);
            },
            .count = [count](const Component& component) -> std::size_t {
                return count(static_cast<const T&>(component));
            }
        });
    }

    std::vector<Property> Release() && { return std::move(properties); }
};

} // namespace GNC
