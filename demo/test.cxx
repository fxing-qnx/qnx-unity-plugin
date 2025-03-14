#include "test.hxx"

#include <thread>
#include <atomic>
#include <functional>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl32.h>

static screen_context_t ctx = nullptr;
static screen_event_t ev = nullptr;
static screen_group_t group = nullptr;

static char cwin_id[64] = {};
static int cwin_size[2] = {200, 200};
static screen_window_t cwin = nullptr;

static const int buf_size = 2;
static int pix_size[buf_size][2] = {};
static screen_pixmap_t pixs[buf_size] = {};
static screen_buffer_t bufs[buf_size] = {};
static std::atomic_bool avls[buf_size] = {};
static std::function<void()> resets[buf_size] = {};

// GL and EGL
static EGLDisplay egl_dpy = EGL_NO_DISPLAY;
static EGLContext egl_ctx = EGL_NO_CONTEXT;
static GLuint texs[buf_size] = {std::numeric_limits<GLuint>::max(), std::numeric_limits<GLuint>::max()};
static EGLImage images[buf_size] = {EGL_NO_IMAGE, EGL_NO_IMAGE};
static PFNEGLCREATEIMAGEKHRPROC feglCreateImage = nullptr;
static PFNEGLDESTROYIMAGEKHRPROC feglDestroyImage = nullptr;
static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC fglEGLImageTargetTexture2D = nullptr;

static int pix_usage = SCREEN_USAGE_OPENGL_ES3 | //
                       SCREEN_USAGE_OPENGL_ES2 | //
                       SCREEN_USAGE_NATIVE |     //
                       SCREEN_USAGE_READ |       //
                       SCREEN_USAGE_WRITE,       //
    format = SCREEN_FORMAT_RGBA8888;

void // executing in the plugin thread
handle_win_ev(int ev_type)
{
    switch (ev_type)
    {
        case SCREEN_EVENT_CREATE:
        {
            screen_window_t tmp_win = nullptr;
            chk(screen_get_event_property_pv(ev, SCREEN_PROPERTY_WINDOW, (void**)&tmp_win));

            if (cwin)
            {
                chk(screen_leave_window_group(tmp_win));
                std::cout << "QNX800WindowMapperGroup Only one window is allowed\n";
                break;
            }

            chk(screen_get_window_property_cv(tmp_win, SCREEN_PROPERTY_ID_STRING, sizeof(cwin_id), cwin_id));
            chk(screen_get_window_property_iv(tmp_win, SCREEN_PROPERTY_SIZE, cwin_size));

            cwin = tmp_win;
            for (int i = 0; i < buf_size; i++)
            {
                resets[i] = [i]()
                {
                    pix_size[i][0] = cwin_size[0];
                    pix_size[i][1] = cwin_size[1];
                    chk_egl(feglDestroyImage(egl_dpy, images[i]));
                    chk(screen_destroy_pixmap_buffer(pixs[i]));

                    chk(screen_set_pixmap_property_iv(pixs[i], SCREEN_PROPERTY_USAGE, &pix_usage));
                    chk(screen_set_pixmap_property_iv(pixs[i], SCREEN_PROPERTY_FORMAT, &format));
                    chk(screen_set_pixmap_property_iv(pixs[i], SCREEN_PROPERTY_BUFFER_SIZE, pix_size[i]));
                    chk(screen_create_pixmap_buffer(pixs[i]));
                    chk(screen_get_pixmap_property_pv(pixs[i], SCREEN_PROPERTY_BUFFERS, (void**)(bufs + i)));

                    images[i] = feglCreateImage(egl_dpy, EGL_NO_CONTEXT, EGL_NATIVE_PIXMAP_KHR, //
                                                (EGLNativePixmapType)pixs[i], nullptr);

                    chk_gl(glBindTexture(GL_TEXTURE_2D, texs[i]));
                    chk_gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
                    chk_gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
                    chk_gl(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
                    chk_gl(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
                    chk_gl(fglEGLImageTargetTexture2D(GL_TEXTURE_2D, images[i]));
                    std::cout << "Reset GL texture" << texs[i] << std::endl;
                };
                std::cout << "Reset dispatched" << i << std::endl;
            }

            std::string joint_msg = "QNX800WindowMapperGroup ";
            joint_msg += cwin_id;
            joint_msg += " joint@";
            joint_msg += std::to_string(cwin_size[0]);
            joint_msg += "x";
            joint_msg += std::to_string(cwin_size[1]);
            std::cout << joint_msg << std::endl;
            break;
        }
        case SCREEN_EVENT_CLOSE:
        {
            if (cwin)
            {
                cwin = nullptr;

                std::string leave_msg = "QNX800WindowMapperGroup ";
                leave_msg += cwin_id;
                leave_msg += " left";
                std::cout << leave_msg << std::endl;
            }
            break;
        }
    }
}

void //
write_buf()
{
    // update at 120 fps
    std::this_thread::sleep_for(std::chrono::microseconds(8333));

    bool ready;
    for (int i = 0; i < buf_size + 1; i++)
    {
        int idx = i % buf_size;
        if (ready = true, avls[idx].compare_exchange_weak(ready, false))
        {
            if (resets[idx])
            {
                resets[idx]();
                resets[idx] = nullptr;
            };

            chk(screen_read_window(cwin, bufs[idx], 0, nullptr, 0)); // error might happen, but expected
            std::cout << "Writing " << idx << std ::endl;
            avls[idx] = true;
            break;
        }
    }
}

int // testing only
read_buf()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    static int holding = 0;
    bool ready;
    for (int i = 0; i < buf_size + 1; i++)
    {
        int idx = i % buf_size;
        if (ready = true, avls[idx].compare_exchange_weak(ready, false))
        {
            std::cout << "Reading " << idx << std ::endl;
            avls[holding] = true;
            holding = idx;
            return texs[idx];
        }
    }

    return -1;
}

int //
main(int argc, char** argv)
{
    chk(screen_create_context(&ctx, SCREEN_APPLICATION_CONTEXT));
    chk(screen_create_event(&ev));

    char group_name[] = "unity_plugin_group";
    chk(screen_create_group(&group, ctx));
    chk(screen_set_group_property_cv(group, SCREEN_PROPERTY_NAME, sizeof(group_name), group_name));
    chk(screen_flush_context(ctx, SCREEN_WAIT_IDLE));

    // init egl
    egl_dpy = chk_egl(eglGetDisplay(EGL_DEFAULT_DISPLAY));
    EGLint egl_configs_size = 0;
    std::vector<EGLConfig> egl_configs = {};
    EGLint egl_ctx_attr[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    chk_egl(eglInitialize(egl_dpy, nullptr, nullptr));
    chk_egl(eglGetConfigs(egl_dpy, nullptr, 0, &egl_configs_size));

    egl_configs.resize(egl_configs_size);
    chk_egl(eglGetConfigs(egl_dpy, egl_configs.data(), egl_configs_size, &egl_configs_size));

    for (size_t c = 0; c < egl_configs_size && egl_ctx == EGL_NO_CONTEXT; c++)
    {
        egl_ctx = chk_egl(eglCreateContext(egl_dpy, egl_configs[c], EGL_NO_CONTEXT, egl_ctx_attr));
    }

    if (egl_ctx == EGL_NO_CONTEXT)
    {
        std::cout << "Can not create egl context\n";
    }
    chk_egl(eglMakeCurrent(egl_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, egl_ctx));

    feglCreateImage = (PFNEGLCREATEIMAGEKHRPROC)chk_egl(eglGetProcAddress("eglCreateImageKHR"));
    feglDestroyImage = (PFNEGLDESTROYIMAGEKHRPROC)chk_egl(eglGetProcAddress("eglDestroyImageKHR"));
    fglEGLImageTargetTexture2D =
        (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)chk_egl(eglGetProcAddress("glEGLImageTargetTexture2DOES"));

    // create empty pixmap
    for (int i = 0; i < buf_size; i++)
    {
        chk(screen_create_pixmap(&pixs[i], ctx));
        chk(screen_set_pixmap_property_iv(pixs[i], SCREEN_PROPERTY_USAGE, &pix_usage));
        chk(screen_set_pixmap_property_iv(pixs[i], SCREEN_PROPERTY_FORMAT, &format));
        chk(screen_set_pixmap_property_iv(pixs[i], SCREEN_PROPERTY_BUFFER_SIZE, cwin_size));
        chk(screen_create_pixmap_buffer(pixs[i]));
        chk(screen_get_pixmap_property_pv(pixs[i], SCREEN_PROPERTY_BUFFERS, (void**)(bufs + i)));

        images[i] = feglCreateImage(egl_dpy, EGL_NO_CONTEXT, EGL_NATIVE_PIXMAP_KHR, //
                                    (EGLNativePixmapType)pixs[i], nullptr);

        pix_size[i][0] = cwin_size[0];
        pix_size[i][1] = cwin_size[1];
    }

    chk_gl(glGenTextures(buf_size, texs));
    for (int t = 0; t < buf_size; t++)
    {
        chk_gl(glBindTexture(GL_TEXTURE_2D, texs[t]));
        chk_gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
        chk_gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
        chk_gl(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        chk_gl(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
        chk_gl(fglEGLImageTargetTexture2D(GL_TEXTURE_2D, images[t]));
    }

    avls[0] = false; // starting at the holding by reader
    for (int i = 1; i < buf_size; i++)
    {
        avls[1] = true;
    }

    std::thread reader_thr(
        []()
        {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            while (true)
            {
                read_buf();
            }
        });

    std::string line = "";
    while (true)
    {
        while (!chk(screen_get_event(ctx, ev, 0)))
        {
            int ev_type = SCREEN_EVENT_NONE, //
                object_type = -1;
            chk(screen_get_event_property_iv(ev, SCREEN_PROPERTY_TYPE, &ev_type));
            chk(screen_get_event_property_iv(ev, SCREEN_PROPERTY_OBJECT_TYPE, &object_type));

            if (ev_type == SCREEN_EVENT_NONE)
            {
                break;
            }

            if (object_type == SCREEN_OBJECT_TYPE_WINDOW)
            {
                handle_win_ev(ev_type);
            }
        }

        if (cwin)
        {
            write_buf();
        }
    }

    for (int i = 0; i < buf_size; i++)
    {
        chk_egl(feglDestroyImage(egl_dpy, images[i]));
        chk(screen_destroy_pixmap_buffer(pixs[i]));
        chk(screen_destroy_pixmap(pixs[i]));
    }

    chk_egl(eglMakeCurrent(egl_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
    chk_egl(eglDestroyContext(egl_dpy, egl_ctx));
    chk_egl(eglTerminate(egl_dpy));
    chk_egl(eglReleaseThread());
    chk(screen_destroy_group(group));
    chk(screen_destroy_event(ev));
    chk(screen_destroy_context(ctx));

    return 0;
}