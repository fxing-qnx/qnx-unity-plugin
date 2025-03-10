#include "test.hxx"

#include <thread>

auto //
main(int argc, char** argv) -> int
{
    screen_context_t screen_ctx = nullptr;
    screen_event_t screen_ev = nullptr;
    screen_window_t root_win = nullptr;
    screen_group_t group = nullptr;
    std::atomic_bool running(true);
    int usage = SCREEN_USAGE_NATIVE;

    PFNEGLCREATEIMAGEKHRPROC feglCreateImage = nullptr;
    PFNEGLDESTROYIMAGEKHRPROC feglDestroyImage = nullptr;
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC fglEGLImageTargetTexture2D = nullptr;

    chk(screen_create_context(&screen_ctx, SCREEN_BUFFER_PROVIDER_CONTEXT));
    chk(screen_create_window_type(&root_win, screen_ctx, SCREEN_ROOT_WINDOW));
    chk(screen_set_window_property_cv(root_win, SCREEN_PROPERTY_ID_STRING, sizeof(id_str), id_str));
    chk(screen_set_window_property_iv(root_win, SCREEN_PROPERTY_USAGE, &usage));
    chk(screen_flush_context(screen_ctx, SCREEN_WAIT_IDLE));
    chk(screen_create_event(&screen_ev));

    const char gid[] = "unity_plugin_group";
    chk(screen_create_group(&group, screen_ctx));
    chk(screen_set_group_property_cv(group, SCREEN_PROPERTY_NAME, sizeof(gid), gid));

    // launch the test appliation
    test_window_thr test_win;
    std::thread test_thr([&]() { test_win.run(running, gid); });

    EGLDisplay dpy = chk_egl(eglGetDisplay(EGL_DEFAULT_DISPLAY));
    EGLint egl_ctx_attr[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    EGLContext egl_ctx = EGL_NO_CONTEXT;
    EGLint egl_configs_size = 0;
    std::vector<EGLConfig> egl_configs = {};

    chk_egl(eglInitialize(dpy, nullptr, nullptr));
    chk_egl(eglGetConfigs(dpy, nullptr, 0, &egl_configs_size));
    std::clog << egl_configs_size << " EGL configs are avaibale\n";

    if (egl_configs_size == 0)
    {
        std::cerr << "No EGL Configs are present\n";
    }

    egl_configs.resize(egl_configs_size);
    chk_egl(eglGetConfigs(dpy, egl_configs.data(), egl_configs_size, &egl_configs_size));

    size_t config_idx = 0;
    while (config_idx < egl_configs_size && egl_ctx == EGL_NO_CONTEXT)
    {
        std::clog << "Config " << config_idx << "\n";
        egl_ctx = chk_egl(eglCreateContext(dpy, egl_configs[config_idx], EGL_NO_CONTEXT, egl_ctx_attr));
        config_idx++;
    }

    feglCreateImage = (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
    feglDestroyImage = (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
    fglEGLImageTargetTexture2D = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");

    screen_window_t cwin = nullptr;
    char cwin_id[64] = {};

    std::string line = "";
    while (std::getline(std::cin, line), line != "exit")
    {
        while (!chk(screen_get_event(screen_ctx, screen_ev, 0)))
        {
            int ev_type = SCREEN_EVENT_NONE, //
                object_type = -1;
            chk(screen_get_event_property_iv(screen_ev, SCREEN_PROPERTY_TYPE, &ev_type));
            chk(screen_get_event_property_iv(screen_ev, SCREEN_PROPERTY_OBJECT_TYPE, &object_type));

            if (ev_type == SCREEN_EVENT_NONE)
            {
                std::clog << "No event\n";
                break;
            }

            switch (ev_type)
            {
                case SCREEN_EVENT_CREATE:
                {
                    if (object_type == SCREEN_OBJECT_TYPE_WINDOW)
                    {
                        chk(screen_get_event_property_pv(screen_ev, SCREEN_PROPERTY_WINDOW, (void**)&cwin));
                        chk(screen_get_window_property_cv(cwin, SCREEN_PROPERTY_ID, sizeof(cwin_id), cwin_id));
                        chk(screen_share_window_buffers(root_win, cwin));
                        std::clog << "Window crated " << cwin_id << "\n";
                    }
                    break;
                }
                default:
                {
                    std::clog << "Unhandled event: " << ev_type << "\n";
                    break;
                }
            }
        }

        chk(screen_flush_context(screen_ctx, SCREEN_WAIT_IDLE));
    }

    running = false;
    test_thr.join();
    chk_egl(eglDestroyContext(dpy, egl_ctx));
    chk_egl(eglTerminate(dpy));
    chk_egl(eglReleaseThread());
    chk(screen_destroy_window_buffers(root_win));
    chk(screen_destroy_window(root_win));
    chk(screen_destroy_event(screen_ev));
    chk(screen_destroy_context(screen_ctx));

    return 0;
}