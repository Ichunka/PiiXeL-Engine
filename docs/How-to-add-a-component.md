# How to Add a Component

## Quick Start

Create **2 files**: `ComponentName.hpp` and `ComponentName.cpp`

### 1. Header (`include/Components/ComponentName.hpp`)

```cpp
#ifndef PIIXELENGINE_COMPONENTNAME_HPP
#define PIIXELENGINE_COMPONENTNAME_HPP

#include <raylib.h>

namespace PiiXeL {

struct ComponentName {
    float value{0.0f};
    Vector2 position{0.0f, 0.0f};
    bool enabled{true};
};

}

#endif
```

### 2. Implementation (`src/Components/ComponentName.cpp`)

```cpp
#include "Components/ComponentName.hpp"
#include "Components/ComponentModuleMacros.hpp"

#ifdef BUILD_WITH_EDITOR
#include <imgui.h>
#endif

namespace PiiXeL {

BEGIN_COMPONENT_MODULE(ComponentName)
    // 1. Register reflection fields
    REFLECT_FIELDS()
        reflectionBuilder.Field("value", &ReflectedType::value,
            ::PiiXeL::Reflection::FieldFlags::Public | ::PiiXeL::Reflection::FieldFlags::Serializable,
            ::PiiXeL::Reflection::FieldMetadata{.rangeMin = 0.0f, .rangeMax = 100.0f, .dragSpeed = 0.5f});
        reflectionBuilder.Field("position", &ReflectedType::position);
        reflectionBuilder.Field("enabled", &ReflectedType::enabled);
    END_REFLECT_MODULE()

    // 2. Auto-serialize (or custom if needed)
    AUTO_SERIALIZATION()

    #ifdef BUILD_WITH_EDITOR
    // 3. Display order in inspector (0=first, 100=last)
    EDITOR_DISPLAY_ORDER(50)

    // 4. Custom editor UI (optional - auto-generated if omitted)
    EDITOR_UI() {
        // Use built-in property rendering
        ::PiiXeL::Reflection::ImGuiRenderer::RenderProperties(component, entityPicker, assetPicker);

        // Add custom UI
        if (ImGui::Button("Reset")) {
            component.value = 0.0f;
        }
    }
    EDITOR_UI_END()

    // 5. Default values when component is added
    EDITOR_CREATE_DEFAULT() {
        ReflectedType comp{};
        comp.value = 42.0f;
        comp.enabled = true;
        return comp;
    }
    EDITOR_CREATE_DEFAULT_END()

    // 6. Custom duplication logic
    EDITOR_DUPLICATE() {
        ReflectedType copy = original;
        copy.value = 0.0f;  // Reset value on duplicate
        return copy;
    }
    EDITOR_DUPLICATE_END()
    #endif
END_COMPONENT_MODULE(ComponentName)

}
```

### 3. Register in ReflectionInit

Add to `src/Reflection/ReflectionInit.cpp`:

```cpp
void __force_link_ComponentName();

void InitializeReflection() {
    // ... other components
    __force_link_ComponentName();
}
```

## That's It!

- **Automatic serialization** ✓
- **Automatic inspector UI** ✓
- **Automatic add/remove** ✓
- **Automatic undo/redo** ✓
- **Automatic duplication** ✓

## Advanced: Custom Serialization

```cpp
module->SetSerializer([](const ReflectedType& comp) -> nlohmann::json {
    return nlohmann::json{
        {"value", comp.value},
        {"custom", "data"}
    };
});

module->SetDeserializer([](ReflectedType& comp, const nlohmann::json& data) {
    comp.value = data.value("value", 0.0f);
});
```

## Advanced: Manual Rendering (Skip Registry)

For components managed manually (like Tag/Transform):

```cpp
SKIP_REGISTRY_RENDER()  // Won't appear in automatic inspector
```
