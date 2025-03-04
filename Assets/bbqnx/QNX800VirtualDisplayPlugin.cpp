#include <iostream>
#include <cstdint>
#include <unordered_map>

#include <screen.h>
#include <pthread.h>
#include <qh/error.h>

#include "IUnityInterface.h"
#include "IUnityLog.h"
#define UIX UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API

static IUnityInterfaces* unity_itx = nullptr;
static IUnityLog* logger = nullptr;
static screen_context_t ctx = nullptr;
static screen_display_mode_t mode = {};
static std::unordered_map<screen_display_t, screen_buffer_t> buffers = {};

extern "C"
{
    void UIX UnityPluginLoad(IUnityInterfaces* unityInterfaces)
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

    void UIX UnityPluginUnload(IUnityInterfaces* unityInterfaces)
    {
        if (ctx && screen_destroy_context(ctx))
        {
            UNITY_LOG(logger, qh_strerror(errno));
        }
    }

    std::uint64_t UIX NewQNX800VirtualDisplay(std::uint32_t width, std::uint32_t height)
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

        return reinterpret_cast<std::uint64_t>(v_display);
    }

    void UIX DelQNX800VirtualDisplay(std::uint64_t display)
    {
        if (display && screen_destroy_display(reinterpret_cast<screen_display_t>(logger)))
        {
            UNITY_LOG(logger, qh_strerror(errno));
        }
    }

    std::uint64_t UIX ReadQNX800VirtualDisplay(std::uint64_t screen, std::uint32_t gl_tex)
    {
        return 0;
    }
}
