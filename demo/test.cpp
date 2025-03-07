#include <iostream>
#include <vector>

#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <screen/screen.h>

inline int chk_error(const char* file, int line, int err) noexcept
{
    if (err != EOK)
    {
        std::cout << "ERROR:" << file << ":" << line << ": " << strerror(errno) << "\n";
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

    // create result pixmap
    int pixmap_size[] = {resolutions[0], resolutions[1]};
    int pixmap_format = SCREEN_FORMAT_RGBA8888, //
        pixmap_usage = SCREEN_USAGE_READ | SCREEN_USAGE_NATIVE;

    screen_pixmap_t pixmap = nullptr;
    chk(screen_create_pixmap(&pixmap, ctx));
    chk(screen_set_pixmap_property_iv(pixmap, SCREEN_PROPERTY_USAGE, &pixmap_usage));
    chk(screen_set_pixmap_property_iv(pixmap, SCREEN_PROPERTY_FORMAT, &pixmap_format));
    chk(screen_set_pixmap_property_iv(pixmap, SCREEN_PROPERTY_BUFFER_SIZE, pixmap_size));
    chk(screen_create_pixmap_buffer(pixmap));

    // get pixmap buffer
    screen_buffer_t pixmap_buf = nullptr;
    int stride = 0;
    uint32_t* mapping = nullptr;
    chk(screen_get_pixmap_property_pv(pixmap, SCREEN_PROPERTY_RENDER_BUFFERS, (void**)&pixmap_buf));
    chk(screen_get_buffer_property_pv(pixmap_buf, SCREEN_PROPERTY_POINTER, (void**)&mapping));
    chk(screen_get_buffer_property_iv(pixmap_buf, SCREEN_PROPERTY_STRIDE, &stride));

    std::string line = "";
    while (std::getline(std::cin, line))
    {
        if (line == "exit")
        {
            break;
        }

        if (line == "capture")
        {
            size_t selected_display = 0;
            std::cout << "Selection display: ";
            std::cin >> selected_display;
            if (selected_display > displays.size())
            {
                selected_display = 0;
            }
            chk(screen_read_display(displays[0], pixmap_buf, 0, nullptr, 0));
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
    chk(screen_destroy_event(event));
    chk(screen_destroy_context(ctx));

    return 0;
}