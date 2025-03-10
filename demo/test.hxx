#ifndef DEMO_TEST_HXX
#define DEMO_TEST_HXX

#include <iostream>
#include <atomic>

#include <pthread.h>
#include <screen/screen.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>

namespace
{
    inline auto // Check error of a call
    i_chk_error(const char* file, int line, int err) -> int
    {
        if (err != 0)
        {
            std::cout << "ERROR: " << file << ":" << line << ": " << strerror(errno) << std::endl;
        }
        return err;
    }

    template <typename R>
    inline auto // Check error of a call
    i_chk_egl_error(R&& r, const char* file, int line) -> R
    {
        EGLint error = eglGetError();
        auto get_erro_str = [&]()
        {
            switch (error)
            {
                case EGL_NOT_INITIALIZED:
                    return "EGL is not initialized, or could not be initialized, for the specified EGL display "
                           "connection.";
                case EGL_BAD_ACCESS:
                    return "EGL cannot access a requested resource (for example a context is bound in another thread).";
                case EGL_BAD_ALLOC:
                    return "EGL failed to allocate resources for the requested operation.";
                case EGL_BAD_ATTRIBUTE:
                    return "An unrecognized attribute or attribute value was passed in the attribute list.";
                case EGL_BAD_CONTEXT:
                    return "An EGLContext argument does not name a valid EGL rendering context.";
                case EGL_BAD_CONFIG:
                    return "An EGLConfig argument does not name a valid EGL frame buffer configuration.";
                case EGL_BAD_CURRENT_SURFACE:
                    return "The current surface of the calling thread is a window, pixel buffer or pixmap that is no "
                           "longer valid.";
                case EGL_BAD_DISPLAY:
                    return "An EGLDisplay argument does not name a valid EGL display connection.";
                case EGL_BAD_SURFACE:
                    return "An EGLSurface argument does not name a valid surface (window, pixel buffer or pixmap) "
                           "configured for GL rendering.";
                case EGL_BAD_MATCH:
                    return "Arguments are inconsistent (for example, a valid context requires buffers not supplied by "
                           "a "
                           "valid surface).";
                case EGL_BAD_PARAMETER:
                    return "One or more argument values are invalid.";
                case EGL_BAD_NATIVE_PIXMAP:
                    return "A NativePixmapType argument does not refer to a valid native pixmap.";
                case EGL_BAD_NATIVE_WINDOW:
                    return "A NativeWindowType argument does not refer to a valid native window.";
                case EGL_CONTEXT_LOST:
                    return "A power management event has occurred. The application must destroy all contexts and "
                           "reinitialise OpenGL ES state and objects to continue rendering.";
                default:
                    return "Undefined EGL error";
            }
        };

        if (error != EGL_SUCCESS)
        {
            std::cout << "EGL ERROR: " << file << ":" << line << ": " << get_erro_str() << std::endl;
        }

        return std::forward<R>(r);
    }
} // namespace

#ifndef NDEBUG
#define chk(call)     ::i_chk_error(__FILE__, __LINE__, call)
#define chk_egl(call) ::i_chk_egl_error(call, __FILE__, __LINE__)
#else
#define chk(call)     call
#define chk_egl(call) call
#endif

static const char id_str[] = "test_unity_plugin";

struct test_window_thr
{
    screen_context_t ctx_ = nullptr;
    screen_window_t win_ = nullptr;
    screen_buffer_t win_buf_[2] = {};

    auto //
    run(std::atomic_bool& running, const char* gid) -> void
    {
        int usage = SCREEN_USAGE_NATIVE | SCREEN_USAGE_READ | SCREEN_USAGE_WRITE,
            format = SCREEN_FORMAT_RGBA8888, //
            interval = 1,                    //
            nbuffers = 2,                    //
            size[2] = {1920, 1080},          //
            pos[2] = {0, 0};
        const char test_id_str[] = "test_window";

        chk(screen_create_context(&ctx_, SCREEN_APPLICATION_CONTEXT));
        chk(screen_create_window_type(&win_, ctx_, SCREEN_APPLICATION_WINDOW));
        chk(screen_join_window_group(win_, gid));

        chk(screen_set_window_property_iv(win_, SCREEN_PROPERTY_USAGE, &usage));
        chk(screen_set_window_property_iv(win_, SCREEN_PROPERTY_FORMAT, &format));
        chk(screen_set_window_property_iv(win_, SCREEN_PROPERTY_SWAP_INTERVAL, &interval));
        chk(screen_set_window_property_iv(win_, SCREEN_PROPERTY_SIZE, size));
        chk(screen_set_window_property_iv(win_, SCREEN_PROPERTY_POSITION, pos));
        chk(screen_set_window_property_cv(win_, SCREEN_PROPERTY_ID_STRING, sizeof(test_id_str), test_id_str));
        chk(screen_create_window_buffers(win_, nbuffers));

        int changing_color = 0;
        while (running)
        {
            int win_background[] = {SCREEN_BLIT_COLOR, static_cast<int>(0xff000000) | (changing_color & 0x00ffffff),
                                    SCREEN_BLIT_END};
            changing_color++;

            chk(screen_get_window_property_pv(win_, SCREEN_PROPERTY_BUFFERS, (void**)win_buf_));
            chk(screen_fill(ctx_, win_buf_[0], win_background));
            chk(screen_post_window(win_, win_buf_[0], 0, nullptr, SCREEN_WAIT_IDLE));
        }

        chk(screen_leave_window_group(win_));
        chk(screen_destroy_window_buffers(win_));
        chk(screen_destroy_window(win_));
        chk(screen_destroy_context(ctx_));
    }
};

#endif // DEMO_TEST_HXX
