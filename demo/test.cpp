#include <iostream>
#include <vector>

#include <pthread.h>
#include <screen/screen.h>

inline int chk_error(const char* file, int line, int err) noexcept
{
    if (err != EOK)
    {
        std::cout << file << ":" << line << ": " << strerror(errno) << "\n";
    }
    return err;
}

#ifndef NDEBUG
thread_local int tmp_err = EOK;
#define chk(call) (tmp_err = call, chk_error(__FILE__, __LINE__, tmp_err), tmp_err)
#else
#define chk(call) call
#endif

int main(int argc, char** argv)
{
    screen_context_t ctx = nullptr;
    screen_event_t event = nullptr;

    int ctx_type = SCREEN_DISPLAY_MANAGER_CONTEXT | SCREEN_BUFFER_PROVIDER_CONTEXT;
    chk(screen_create_context(&ctx, ctx_type));
    chk(screen_create_event(&event));

    // get all available displays
    int ndisplays = 0;
    std::vector<screen_display_t> displays;
    std::vector<int> resolutions = {};
    chk(screen_get_context_property_iv(ctx, SCREEN_PROPERTY_DISPLAY_COUNT, &ndisplays));
    displays.resize(ndisplays);
    resolutions.resize(ndisplays * 2);
    chk(screen_get_context_property_pv(ctx, SCREEN_PROPERTY_DISPLAYS, (void**)displays.data()));

    for (size_t d = 0; d < displays.size(); d++)
    {
        char id_str[64] = {};
        chk(screen_get_display_property_cv(displays[d], SCREEN_PROPERTY_ID_STRING, sizeof(id_str), id_str));
        chk(screen_get_display_property_iv(displays[d], SCREEN_PROPERTY_SIZE, &resolutions[d * 2]));
        std::clog << "Display id string: " << id_str                            //
                  << "@" << resolutions[d * 2] << "x" << resolutions[d * 2 + 1] //
                  << "\n";
    }

    // create result buffer
    screen_buffer_t buf = nullptr;
    std::vector<unsigned char> pixels(resolutions[0] * resolutions[1] * 4);
    void* pixels_ptr = pixels.data();
    int buf_res[] = {resolutions[0], resolutions[1]};
    int buf_format = SCREEN_FORMAT_RGBA8888, //
        buf_stride = resolutions[0] * 4,     //
        buf_size = pixels.size();

    chk(screen_create_buffer(&buf));
    chk(screen_set_buffer_property_iv(buf, SCREEN_PROPERTY_FORMAT, &buf_format));
    chk(screen_set_buffer_property_iv(buf, SCREEN_PROPERTY_BUFFER_SIZE, buf_res));
    chk(screen_set_buffer_property_iv(buf, SCREEN_PROPERTY_SIZE, &buf_size));
    chk(screen_set_buffer_property_iv(buf, SCREEN_PROPERTY_STRIDE, &buf_stride));
    chk(screen_set_buffer_property_pv(buf, SCREEN_PROPERTY_POINTER, (void**)&pixels_ptr));

    screen_pixmap_t pixmap = nullptr;
    int pixmap_usage = SCREEN_USAGE_READ | SCREEN_USAGE_WRITE | SCREEN_USAGE_NATIVE;
    chk(screen_create_pixmap(&pixmap, ctx));
    chk(screen_set_pixmap_property_iv(pixmap, SCREEN_PROPERTY_USAGE, &pixmap_usage));
    chk(screen_attach_pixmap_buffer(pixmap, buf));

    std::string line = "";
    while (std::getline(std::cin, line))
    {
        if (line == "exit")
        {
            break;
        }

        if (line == "capture")
        {
        }

        while (!chk(screen_get_event(ctx, event, 0)))
        {
            int val = SCREEN_EVENT_NONE, //
                object_type = -1;
            chk(screen_get_event_property_iv(event, SCREEN_PROPERTY_TYPE, &val));
            chk(screen_get_event_property_iv(event, SCREEN_PROPERTY_OBJECT_TYPE, &object_type));

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

        chk(screen_flush_context(ctx, SCREEN_WAIT_IDLE));
    }

    chk(screen_destroy_pixmap(pixmap));
    chk(screen_destroy_buffer(buf));
    chk(screen_destroy_event(event));
    chk(screen_destroy_context(ctx));

    return 0;
}