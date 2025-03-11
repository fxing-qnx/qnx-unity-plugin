#include "test.hxx"

#include <thread>

int //
main(int argc, char** argv)
{
    screen_context_t screen_ctx = nullptr;
    screen_event_t screen_ev = nullptr;
    screen_group_t group = nullptr;
    screen_stream_t stream = nullptr;
    std::atomic_bool running(true);

    chk(screen_create_context(&screen_ctx, SCREEN_BUFFER_PROVIDER_CONTEXT));
    chk(screen_create_event(&screen_ev));
    chk(screen_create_stream(&stream, screen_ctx));

    int stream_usage = SCREEN_USAGE_NATIVE | SCREEN_USAGE_READ;
    chk(screen_set_stream_property_iv(stream, SCREEN_PROPERTY_USAGE, &stream_usage));

    char group_name[] = "unity_plugin_group";
    chk(screen_create_group(&group, screen_ctx));
    chk(screen_set_group_property_cv(group, SCREEN_PROPERTY_NAME, sizeof(group_name), group_name));
    chk(screen_flush_context(screen_ctx, SCREEN_WAIT_IDLE));

    // create egl
    PFNEGLCREATEIMAGEKHRPROC feglCreateImage = nullptr;
    PFNEGLDESTROYIMAGEKHRPROC feglDestroyImage = nullptr;
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC fglEGLImageTargetTexture2D = nullptr;
    EGLDisplay egl_dpy = chk_egl(eglGetDisplay(EGL_DEFAULT_DISPLAY));
    EGLint egl_ctx_attr[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    EGLContext egl_ctx = EGL_NO_CONTEXT;
    EGLint egl_configs_size = 0;
    GLuint gl_tex = 0;
    EGLImageKHR egl_image = EGL_NO_IMAGE;
    std::vector<EGLConfig> egl_configs = {};

    chk_egl(eglInitialize(egl_dpy, nullptr, nullptr));
    chk_egl(eglGetConfigs(egl_dpy, nullptr, 0, &egl_configs_size));
    std::clog << egl_configs_size << " EGL configs are avaibale\n";

    if (egl_configs_size == 0)
    {
        std::cerr << "No EGL Configs are present\n";
    }

    egl_configs.resize(egl_configs_size);
    chk_egl(eglGetConfigs(egl_dpy, egl_configs.data(), egl_configs_size, &egl_configs_size));

    size_t config_idx = 0;
    while (config_idx < egl_configs_size && egl_ctx == EGL_NO_CONTEXT)
    {
        std::clog << "Config " << config_idx << "\n";
        egl_ctx = chk_egl(eglCreateContext(egl_dpy, egl_configs[config_idx], EGL_NO_CONTEXT, egl_ctx_attr));
        config_idx++;
    }
    chk_egl(eglMakeCurrent(egl_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, egl_ctx));

    feglCreateImage = (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
    feglDestroyImage = (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
    fglEGLImageTargetTexture2D = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");

    chk_gl(glGenTextures(1, &gl_tex));
    chk_gl(glBindTexture(GL_TEXTURE_2D, gl_tex));
    chk_gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    chk_gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    chk_gl(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    chk_gl(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

    // launch the test appliation
    test_window_thr test_win;
    std::thread test_thr([&]() { test_win.run(running, group_name); });

    screen_window_t cwin = nullptr;
    screen_pixmap_t cpixmap = nullptr;
    screen_stream_t cstream = nullptr;

    std::string line = "";
    while (std::getline(std::cin, line))
    {
        while (!chk(screen_get_event(screen_ctx, screen_ev, 0)))
        {
            int ev_type = SCREEN_EVENT_NONE, //
                object_type = -1;
            chk(screen_get_event_property_iv(screen_ev, SCREEN_PROPERTY_TYPE, &ev_type));
            chk(screen_get_event_property_iv(screen_ev, SCREEN_PROPERTY_OBJECT_TYPE, &object_type));

            if (ev_type == SCREEN_EVENT_NONE)
            {
                break;
            }

            switch (ev_type)
            {
                case SCREEN_EVENT_CREATE:
                {
                    if (object_type == SCREEN_OBJECT_TYPE_WINDOW)
                    {
                        char cwin_id[64] = {};
                        int pix_usage = SCREEN_USAGE_OPENGL_ES3 | SCREEN_USAGE_NATIVE | SCREEN_USAGE_READ, //
                            format = SCREEN_FORMAT_RGBA8888,                                               //
                            pix_size[2] = {};
                        chk(screen_get_event_property_pv(screen_ev, SCREEN_PROPERTY_WINDOW, (void**)&cwin));
                        chk(screen_get_window_property_cv(cwin, SCREEN_PROPERTY_ID_STRING, sizeof(cwin_id), cwin_id));
                        chk(screen_get_window_property_iv(cwin, SCREEN_PROPERTY_SIZE, pix_size));
                        chk(screen_get_window_property_iv(cwin, SCREEN_PROPERTY_FORMAT, &format));
                        chk(screen_create_pixmap(&cpixmap, screen_ctx));
                        chk(screen_set_pixmap_property_iv(cpixmap, SCREEN_PROPERTY_USAGE, &pix_usage));
                        chk(screen_set_pixmap_property_iv(cpixmap, SCREEN_PROPERTY_FORMAT, &format));
                        chk(screen_set_pixmap_property_iv(cpixmap, SCREEN_PROPERTY_BUFFER_SIZE, pix_size));
                        std::cout << cwin_id << "@" << pix_size[0] << "x" << pix_size[1] << " joint the group\n";

                        chk(screen_get_window_property_pv(cwin, SCREEN_PROPERTY_STREAM, (void**)&cstream));
                        if (cstream)
                        {
                            chk(screen_consume_stream_buffers(stream, 0, cstream));
                            std::cout << "Child window stream acquired\n";
                        }
                    }
                    break;
                }
                default:
                {
                    std::cout << "Unhandled event: " << ev_type << "\n";
                    break;
                }
            }
        }

        if (cwin && cstream)
        {
            if (egl_image != EGL_NO_IMAGE)
            {
                chk_egl(feglDestroyImage(egl_dpy, egl_image));
                egl_image = EGL_NO_IMAGE;
            }

            screen_buffer_t cbuf = nullptr;
            chk(screen_acquire_buffer(&cbuf, stream, nullptr, nullptr, nullptr, //
                                      SCREEN_ACQUIRE_AND_RELEASE_OTHERS | SCREEN_ACQUIRE_DONT_BLOCK));

            if (line == "exit")
            {
                running = false;

                if (cbuf)
                {
                    chk(screen_release_buffer(cbuf));
                }
                break;
            }

            if (cbuf)
            {
                chk(screen_attach_pixmap_buffer(cpixmap, cbuf));
                egl_image = chk_egl(feglCreateImage(egl_dpy, EGL_NO_CONTEXT, EGL_NATIVE_PIXMAP_KHR,
                                                    (EGLNativePixmapType)cpixmap, nullptr));

                chk_gl(glBindTexture(GL_TEXTURE_2D, gl_tex));
                fglEGLImageTargetTexture2D(GL_TEXTURE_2D, egl_image);
                chk_egl(0);
                std::cout << (void*)egl_image << std::endl;
                std::cout << gl_tex << std::endl;

                chk(screen_release_buffer(cbuf));
            }
        }
    }

    running = false;
    test_thr.join();

    chk_gl(glDeleteTextures(1, &gl_tex));
    if (egl_image != EGL_NO_IMAGE)
    {
        chk_egl(feglDestroyImage(egl_dpy, egl_image));
    }
    chk_egl(eglMakeCurrent(egl_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
    chk_egl(eglDestroyContext(egl_dpy, egl_ctx));
    chk_egl(eglTerminate(egl_dpy));
    chk_egl(eglReleaseThread());

    if (cpixmap)
    {
        chk(screen_destroy_pixmap(cpixmap));
    }
    if (cstream)
    {
        chk(screen_destroy_stream(cstream));
    }
    chk(screen_destroy_group(group));
    chk(screen_destroy_stream(stream));
    chk(screen_destroy_event(screen_ev));
    chk(screen_destroy_context(screen_ctx));

    return 0;
}