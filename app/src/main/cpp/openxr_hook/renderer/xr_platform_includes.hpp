#pragma once

// Только Android (Meta Quest): игра рисует через Vulkan или OpenGL ES.
#define XR_USE_GRAPHICS_API_VULKAN
#define XR_USE_GRAPHICS_API_OPENGL_ES

#include <vulkan/vulkan.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include "../openxr_sdk/include/openxr/openxr.h"
#include "../openxr_sdk/include/openxr/openxr_platform.h"
