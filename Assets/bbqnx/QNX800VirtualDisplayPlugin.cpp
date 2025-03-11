
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

struct clean_up
{
    ~clean_up() {}
} clean_up;

extern "C" void UIX //
UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
    unity_itx = unityInterfaces;
    logger = unity_itx->Get<IUnityLog>();

}

