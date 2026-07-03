/**
 * @file vk_pipeline.c
 * @brief Vulkan pipeline implementation (requires HAS_VULKAN + Vulkan SDK).
 */
#include "rendering/vk_pipeline.h"

#ifdef HAS_VULKAN
#include <string.h>
#include <stdio.h>

ErrorCode vk_context_create(VkContext *ctx, const char *app_name) {
    if (!ctx || !app_name) return ERR_INVALID_ARG;
    memset(ctx, 0, sizeof(*ctx));

    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = app_name,
        .apiVersion = VK_API_VERSION_1_3,
    };
    VkInstanceCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
    };
    if (vkCreateInstance(&ci, NULL, &ctx->instance) != VK_SUCCESS) return ERR_UNKNOWN;

    /* Pick first physical device */
    uint32_t count = 1;
    vkEnumeratePhysicalDevices(ctx->instance, &count, &ctx->physical_device);
    if (count == 0) {
        vk_context_destroy(ctx);
        return ERR_UNKNOWN;
    }

    /* Find graphics queue family */
    uint32_t qf_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(ctx->physical_device, &qf_count, NULL);
    VkQueueFamilyProperties qf_props[32];
    if (qf_count > 32) qf_count = 32;
    vkGetPhysicalDeviceQueueFamilyProperties(ctx->physical_device, &qf_count, qf_props);

    ctx->queue_family = UINT32_MAX;
    for (uint32_t i = 0; i < qf_count; i++) {
        if (qf_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            ctx->queue_family = i;
            break;
        }
    }
    if (ctx->queue_family == UINT32_MAX) {
        vk_context_destroy(ctx);
        return ERR_UNKNOWN;
    }

    float                   priority = 1.0f;
    VkDeviceQueueCreateInfo qci = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = ctx->queue_family,
        .queueCount = 1,
        .pQueuePriorities = &priority,
    };
    VkDeviceCreateInfo dci = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &qci,
    };
    if (vkCreateDevice(ctx->physical_device, &dci, NULL, &ctx->device) != VK_SUCCESS) {
        vk_context_destroy(ctx);
        return ERR_UNKNOWN;
    }
    vkGetDeviceQueue(ctx->device, ctx->queue_family, 0, &ctx->graphics_queue);
    return ERR_OK;
}

void vk_context_destroy(VkContext *ctx) {
    if (!ctx) return;
    if (ctx->device) vkDestroyDevice(ctx->device, NULL);
    if (ctx->instance) vkDestroyInstance(ctx->instance, NULL);
    memset(ctx, 0, sizeof(*ctx));
}

VkShaderModule vk_create_shader(VkDevice device, const uint32_t *code, size_t size) {
    VkShaderModuleCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode = code,
    };
    VkShaderModule mod;
    if (vkCreateShaderModule(device, &ci, NULL, &mod) != VK_SUCCESS) return VK_NULL_HANDLE;
    return mod;
}

#endif /* HAS_VULKAN */
