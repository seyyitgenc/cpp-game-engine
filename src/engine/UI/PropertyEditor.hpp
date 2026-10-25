#pragma once

#include <vector>

namespace GNC {

class Component;
class ResourceManager;
struct Property;

// Draws a widget per visible property, chosen by the value's type and hint,
// and writes edits straight to the component. Colors are shown in sRGB, a
// rotation as Euler degrees. properties must belong to the component's type.
// Returns whether anything changed.
bool EditProperties(Component& component, const std::vector<Property>& properties, ResourceManager& resources);

} // namespace GNC
