#pragma once

#include <stdint.h>
#include <string>

#include "common/optional.h"

namespace vkb
{
// --- Combine ---
template <typename T>
inline Optional<T> combine(const Optional<T> &first, const Optional<T> &second)
{
	return second.has_value() ? second : first;
}

#define COMBINE(Type, ...)                                                                   \
	template <>                                                                              \
	inline Optional<Type> combine(const Optional<Type> &first, const Optional<Type> &second) \
	{                                                                                        \
		if (!first.has_value() && !second.has_value())                                       \
		{                                                                                    \
			return Optional<Type>();                                                         \
		}                                                                                    \
                                                                                             \
		Type type;                                                                           \
		__VA_ARGS__                                                                          \
		return type;                                                                         \
	}

#define C_ATTR(name) type.name = combine(first.value_or().name, second.value_or().name);

// --- Window Properties ---

struct Extent
{
	uint32_t width;
	uint32_t height;
};

struct OptionalTargetExtent
{
	Optional<uint32_t> width;
	Optional<uint32_t> height;
};
COMBINE(OptionalTargetExtent, C_ATTR(width) C_ATTR(height));

enum class WindowMode
{
	Default,
	Headless,
	FullscreenBorderless,
	Fullscreen
};

struct WindowProperties
{
	std::string title;
	bool        resizable;
	WindowMode  mode;
};

struct OptionalWindowProperties
{
	Optional<std::string> title;
	Optional<bool>        resizable;
	Optional<WindowMode>  mode;
};
COMBINE(OptionalWindowProperties, C_ATTR(title) C_ATTR(resizable) C_ATTR(mode));

// --- Renderer Properties ---

enum class VsyncMode
{
	On,
	Off,
	Default        // Use the mode requested by the application
};

struct RenderProperties
{
	bool      use_fixed_simulation_fps;
	float     fixed_simulation_fps;
	VsyncMode vsync;
};

struct OptionalRenderProperties
{
	Optional<float>     fixed_simulation_fps;
	Optional<VsyncMode> vsync;
};
COMBINE(OptionalRenderProperties, C_ATTR(fixed_simulation_fps) C_ATTR(vsync));

// --- Platform Properties ---

struct PlatformProperties
{
	bool process_input_events;
};

struct OptionalPlatformProperties
{
	Optional<bool> process_input_events;
};
COMBINE(OptionalPlatformProperties, C_ATTR(process_input_events));

// --- All Properties ---

struct OptionalProperties
{
	Optional<OptionalTargetExtent>       target_extent;
	Optional<OptionalWindowProperties>   window_properties;
	Optional<OptionalRenderProperties>   render_properties;
	Optional<OptionalPlatformProperties> platform_properties;
};

inline OptionalProperties combine(const OptionalProperties &first, const OptionalProperties &second)
{
	OptionalProperties properties;
	properties.target_extent       = combine(first.target_extent, second.target_extent);
	properties.window_properties   = combine(first.window_properties, second.window_properties);
	properties.render_properties   = combine(first.render_properties, second.render_properties);
	properties.platform_properties = combine(first.platform_properties, second.platform_properties);
	return properties;
}

#undef COMBINE
#undef C_ATTR
}        // namespace vkb