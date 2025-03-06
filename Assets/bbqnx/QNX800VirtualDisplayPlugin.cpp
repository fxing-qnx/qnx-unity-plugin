#include <iostream>
#include <cstdint>
#include <unordered_map>

#include <pthread.h>
#include <screen/screen.h>
#include <qh/error.h>

#include "IUnityInterface.h"
#include "IUnityLog.h"
#define UIX UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API

static IUnityInterfaces* unity_itx = nullptr;
static IUnityLog* logger = nullptr;
static screen_context_t ctx = nullptr;
static screen_display_mode_t mode = {};
static std::unordered_map<screen_display_t, screen_buffer_t> buffers = {};

struct clean_up
{
    ~clean_up() {}
} clean_up;

extern "C" void UIX UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
    unity_itx = unityInterfaces;
    logger = unity_itx->Get<IUnityLog>();

    // create screen ctx
    if (screen_create_context(&ctx, SCREEN_DISPLAY_MANAGER_CONTEXT))
    {
        UNITY_LOG(logger, qh_strerror(errno));
    }

    mode.refresh = 60;
    mode.index = 0;
    mode.format = SCREEN_FORMAT_RGBA8888;
    mode.flags = SCREEN_DISPLAY_MODE_PREFERRED |    //
                 SCREEN_DISPLAY_MODE_FORMAT_VALID | //
                 SCREEN_DISPLAY_MODE_REFRESH_VALID;
}

extern "C" void UIX UnityPluginUnload(IUnityInterfaces* unityInterfaces)
{
    if (ctx && screen_destroy_context(ctx))
    {
        UNITY_LOG(logger, qh_strerror(errno));
    }
}

extern "C" std::uint64_t UIX NewQNX800VirtualDisplay(std::int32_t width, std::int32_t height)
{
    if (!ctx)
    {
        return reinterpret_cast<std::uint64_t>(nullptr);
    }

    screen_display_t v_display = nullptr;
    mode.width = width;
    mode.height = height;
    mode.aspect_ratio[0] = width;
    mode.aspect_ratio[1] = height;

    if (screen_create_display(&v_display, ctx, &mode))
    {
        UNITY_LOG(logger, qh_strerror(errno));
        return reinterpret_cast<std::uint64_t>(nullptr);
    }

    // create a buffer
    screen_buffer_t& buf = buffers[v_display];
    if (screen_create_buffer(&buf))
    {
        UNITY_LOG(logger, qh_strerror(errno));
        buffers.erase(v_display);
        return reinterpret_cast<std::uint64_t>(nullptr);
    }

    if (screen_set_buffer_property_iv(buf, SCREEN_PROPERTY_FORMAT, //
                                      (const int[]){SCREEN_FORMAT_RGBA8888}))
    {
        UNITY_LOG(logger, qh_strerror(errno));
        buffers.erase(v_display);
        return reinterpret_cast<std::uint64_t>(nullptr);
    }

    return reinterpret_cast<std::uint64_t>(v_display);
}

extern "C" void UIX DelQNX800VirtualDisplay(std::uint64_t display)
{
    auto r = buffers.find(reinterpret_cast<screen_display_t>(display));
    if (r == buffers.end())
    {
        std::string msg = "Screen handle " + std::to_string(display) + " does not exist";
        UNITY_LOG(logger, msg.c_str());
        return;
    }

    if (screen_destroy_buffer(r->second))
    {
        UNITY_LOG(logger, qh_strerror(errno));
    }

    if (display && screen_destroy_display(reinterpret_cast<screen_display_t>(display)))
    {
        UNITY_LOG(logger, qh_strerror(errno));
    }
    buffers.erase(reinterpret_cast<screen_display_t>(display));
}

extern "C" void UIX ReadQNX800VirtualDisplay(std::uint64_t display,
                                             std::uint32_t width,
                                             std::uint32_t height,
                                             std::uint64_t pixels)
{
    auto r = buffers.find(reinterpret_cast<screen_display_t>(display));
    if (r == buffers.end())
    {
        std::string msg = "Screen handle " + std::to_string(display) + " does not exist";
        UNITY_LOG(logger, msg.c_str());
        return;
    }

    void* ptr = reinterpret_cast<screen_display_t>(pixels);
    int err = 0;
    // the proper size is width * height * 4 * sizeof(char), by the standard, the sizeof(char) is always 1
    err |= screen_set_buffer_property_iv(r->second, SCREEN_PROPERTY_BUFFER_SIZE, (const int[]){width, height});
    err |= screen_set_buffer_property_iv(r->second, SCREEN_PROPERTY_SIZE, (const int[]){width * height * 4});
    err |= screen_set_buffer_property_iv(r->second, SCREEN_PROPERTY_STRIDE, (const int[]){width * 4});
    err |= screen_set_buffer_property_pv(r->second, SCREEN_PROPERTY_POINTER, &ptr);

    if (err)
    {
        UNITY_LOG(logger, qh_strerror(errno));
        return;
    }
}