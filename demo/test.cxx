#include "test.hxx"

auto //
main(int argc, char** argv) -> int
{
    const char id_str[] = "test_unity_plugin";
    const char gid_str[] = "test_unity_plugin_group";
    screen_context_t screen_ctx = nullptr;
    screen_event_t screen_ev = nullptr;
    screen_window_t root_win = nullptr;
    screen_window_t test_win = nullptr;

    chk(screen_create_context(&screen_ctx, SCREEN_BUFFER_PROVIDER_CONTEXT));
    chk(screen_create_event(&screen_ev));
    chk(screen_create_window_type(&root_win, screen_ctx, SCREEN_ROOT_WINDOW));
    chk(screen_set_window_property_cv(root_win, SCREEN_PROPERTY_ID_STRING, sizeof(id_str), id_str));
    chk(screen_set_window_property_iv(root_win, SCREEN_PROPERTY_USAGE, (const int[]){SCREEN_USAGE_NATIVE}));
    chk(screen_create_window_group(root_win, gid_str));

    {                                                                  // create test window
        int usage = SCREEN_USAGE_OPENGL_ES2 | SCREEN_USAGE_OPENGL_ES3, //
            format = SCREEN_FORMAT_RGBA8888,                           //
            interval = 1,                                              //
            nbuffers = 2,                                              //
            size[2] = {200, 200},                                      //
            pos[2] = {0, 0};

        chk(screen_create_window(&test_win, screen_ctx));
        chk(screen_set_window_property_iv(test_win, SCREEN_PROPERTY_USAGE, &usage));
        chk(screen_set_window_property_iv(test_win, SCREEN_PROPERTY_FORMAT, &format));
        chk(screen_set_window_property_iv(test_win, SCREEN_PROPERTY_SWAP_INTERVAL, &interval));
        chk(screen_set_window_property_iv(test_win, SCREEN_PROPERTY_SIZE, size));
        chk(screen_set_window_property_iv(test_win, SCREEN_PROPERTY_POSITION, pos));
        chk(screen_create_window_buffers(test_win, nbuffers));
        chk(screen_join_window_group(test_win, gid_str));
    }

    EGLDisplay dpy = chk_egl(eglGetDisplay(EGL_DEFAULT_DISPLAY));
    EGLint egl_ctx_attr[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    chk_egl(eglInitialize(dpy, nullptr, nullptr));

    EGLint egl_configs_size = 0;
    std::vector<EGLConfig> egl_configs = {};
    chk_egl(eglGetConfigs(dpy, nullptr, 0, &egl_configs_size));
    std::clog << egl_configs_size << " EGL configs are avaibale\n";

    if (egl_configs_size == 0)
    {
        std::cerr << "No EGL Configs are present\n";
    }

    egl_configs.resize(egl_configs_size);
    chk_egl(eglGetConfigs(dpy, egl_configs.data(), egl_configs_size, &egl_configs_size));

    EGLContext egl_ctx = EGL_NO_CONTEXT;
    for (size_t c = 0; c < egl_configs_size && egl_ctx == EGL_NO_CONTEXT; c++)
    {
        std::clog << "Config " << c << "\n";
        egl_ctx = chk_egl(eglCreateContext(dpy, egl_configs[c], EGL_NO_CONTEXT, egl_ctx_attr));
    }

    std::string line = "";
    while (std::getline(std::cin, line), line != "exit")
    {
        while (!chk(screen_get_event(screen_ctx, screen_ev, 0)))
        {
            int val = SCREEN_EVENT_NONE, //
                object_type = -1;
            chk(screen_get_event_property_iv(screen_ev, SCREEN_PROPERTY_TYPE, &val));
            chk(screen_get_event_property_iv(screen_ev, SCREEN_PROPERTY_OBJECT_TYPE, &object_type));

            if (val == SCREEN_EVENT_NONE)
            {
                std::cout << "No event\n";
                break;
            }

            switch (val)
            {
                default:
                {
                    std::cout << "Unhandled event: " << val << "\n";
                    break;
                }
            }
        }
    }

    chk_egl(eglDestroyContext(dpy, egl_ctx));
    chk_egl(eglTerminate(dpy));
    chk_egl(eglReleaseThread());
    chk(screen_destroy_window_buffers(test_win));
    chk(screen_destroy_window(test_win));
    chk(screen_destroy_window(root_win));
    chk(screen_destroy_event(screen_ev));
    chk(screen_destroy_context(screen_ctx));

    return 0;
}