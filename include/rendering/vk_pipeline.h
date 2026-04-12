/**
 * @file vk_pipeline.h
 * @brief Vulkan rendering pipeline helpers (requires HAS_VULKAN).
 *
 * Provides instance creation, device selection, and pipeline setup.
 * Guarded behind HAS_VULKAN. Without it, serves as a reference for
 * the Vulkan initialization sequence.
 */
#ifndef RENDERING_VK_PIPELINE_H
#define RENDERING_VK_PIPELINE_H

#include "core/error.h"

#ifdef HAS_VULKAN
#include <vulkan/vulkan.h>

/** @brief Vulkan context holding instance, device, and queue handles. */
typedef struct {
    VkInstance       instance;
    VkPhysicalDevice physical_device;
    VkDevice         device;
    VkQueue          graphics_queue;
    uint32_t         queue_family;
} VkContext;

/**
 * @brief Create and initialize a Vulkan context.
 * @param ctx      Context to initialize.
 * @param app_name Application name for VkApplicationInfo.
 * @return ERR_OK on success, or an error code.
 */
ErrorCode vk_context_create(VkContext *ctx, const char *app_name);

/**
 * @brief Destroy a Vulkan context and release all resources.
 * @param ctx Context to destroy.
 */
void      vk_context_destroy(VkContext *ctx);

/**
 * @brief Create a shader module from SPIR-V bytecode.
 * @param device Vulkan logical device.
 * @param code   Pointer to SPIR-V bytecode.
 * @param size   Size of bytecode in bytes.
 * @return Shader module handle, or VK_NULL_HANDLE on failure.
 */
VkShaderModule vk_create_shader(VkDevice device, const uint32_t *code, size_t size);

#else /* !HAS_VULKAN — reference patterns only */

/*
 * Vulkan Initialization Pattern (reference):
 *
 * 1. Create VkInstance:
 *      VkApplicationInfo app = { .apiVersion = VK_API_VERSION_1_3 };
 *      VkInstanceCreateInfo ci = { .pApplicationInfo = &app };
 *      vkCreateInstance(&ci, NULL, &instance);
 *
 * 2. Select physical device:
 *      vkEnumeratePhysicalDevices(instance, &count, devices);
 *      // Pick device with required queue families & features
 *
 * 3. Create logical device + queues:
 *      VkDeviceQueueCreateInfo qci = { .queueFamilyIndex = idx };
 *      VkDeviceCreateInfo dci = { .pQueueCreateInfos = &qci };
 *      vkCreateDevice(physical, &dci, NULL, &device);
 *
 * 4. Create render pass, pipeline layout, graphics pipeline
 * 5. Create framebuffers, command pool, command buffers
 * 6. Render loop: acquire image, record commands, submit, present
 *
 * Key differences from OpenGL:
 * - Explicit memory management (VkDeviceMemory)
 * - Pre-compiled shaders (SPIR-V)
 * - Explicit synchronization (semaphores, fences)
 * - Pipeline state is immutable once created
 */

#endif /* HAS_VULKAN */
#endif /* RENDERING_VK_PIPELINE_H */
