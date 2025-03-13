#include "test.hxx"

#include <thread>
#include <unistd.h>

static screen_context_t ctx = nullptr;
static screen_event_t ev = nullptr;
static screen_group_t group = nullptr;

enum class BUF_STATE : int
{
    READY,
    BUSY,
    EXPIRE
};

static std::atomic_bool reader_paused;
static std::atomic_bool writer_paused;

static char cwin_id[64] = {};
static int cwin_size[2] = {};
static screen_window_t cwin = nullptr;
static screen_pixmap_t pixs[2] = {};
static screen_buffer_t bufs[2] = {};
static std::atomic<BUF_STATE> avls[2] = {};

void //
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
            int pix_usage = SCREEN_USAGE_OPENGL_ES3 | //
                            SCREEN_USAGE_OPENGL_ES2 | //
                            SCREEN_USAGE_NATIVE |     //
                            SCREEN_USAGE_READ,        //
                format = SCREEN_FORMAT_RGBA8888;

            for (int i = 0; i < sizeof(pixs) / sizeof(pixs[0]); i++)
            {
                chk(screen_create_pixmap(&pixs[i], ctx));
                chk(screen_set_pixmap_property_iv(pixs[i], SCREEN_PROPERTY_USAGE, &pix_usage));
                chk(screen_set_pixmap_property_iv(pixs[i], SCREEN_PROPERTY_FORMAT, &format));
                chk(screen_set_pixmap_property_iv(pixs[i], SCREEN_PROPERTY_BUFFER_SIZE, cwin_size));
                chk(screen_create_pixmap_buffer(pixs[i]));
                chk(screen_get_pixmap_property_pv(pixs[i], SCREEN_PROPERTY_BUFFERS, (void**)(bufs + i)));
            }

            std::string joint_msg = "QNX800WindowMapperGroup ";
            joint_msg += cwin_id;
            joint_msg += " joint";
            std::cout << joint_msg << std::endl;
            break;
        }
        case SCREEN_EVENT_CLOSE:
        {
            if (cwin)
            {
                avls[0] = BUF_STATE::EXPIRE;
                avls[1] = BUF_STATE::EXPIRE;
                while (!reader_paused || !writer_paused)
                {
                    // spin lock
                }

                for (int i = 0; i < sizeof(pixs) / sizeof(pixs[0]); i++)
                {
                    chk(screen_destroy_pixmap_buffer(pixs[i]));
                    bufs[i] = nullptr;
                }
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
    BUF_STATE state;
    for (int i = 0; i < 3; i++)
    {
        int idx = i % 2;
        if (state = BUF_STATE::READY, avls[idx].compare_exchange_weak(state, BUF_STATE::BUSY))
        {

            chk(screen_read_window(cwin, bufs[idx], 0, nullptr, 0));
            sleep(1);
            std::cout << "Read to buffer " << idx << std::endl;
            avls[idx] = BUF_STATE::READY;
            break;
        }

        if (state == BUF_STATE::EXPIRE)
        {
            writer_paused = true;
        }
    }
}

void read_buf()
{
    BUF_STATE state;
    for (int i = 0; i < 3; i++)
    {
        int idx = i % 2;
        if (state = BUF_STATE::READY, avls[idx].compare_exchange_weak(state, BUF_STATE::BUSY))
        {
            chk(screen_read_window(cwin, bufs[idx], 0, nullptr, 0));
            sleep(2);
            std::cout << "Read from buffer " << idx << std::endl;
            avls[idx] = BUF_STATE::READY;
            break;
        }

        if (state == BUF_STATE::EXPIRE)
        {
            reader_paused = true;
        }
    }
}

int //
main(int argc, char** argv)
{
    avls[0] = BUF_STATE::EXPIRE;
    avls[1] = BUF_STATE::EXPIRE;
    reader_paused = true;
    writer_paused = true;

    chk(screen_create_context(&ctx, SCREEN_APPLICATION_CONTEXT));
    chk(screen_create_event(&ev));

    char group_name[] = "unity_plugin_group";
    chk(screen_create_group(&group, ctx));
    chk(screen_set_group_property_cv(group, SCREEN_PROPERTY_NAME, sizeof(group_name), group_name));
    chk(screen_flush_context(ctx, SCREEN_WAIT_IDLE));

    std::thread reader_thr(
        []()
        {
            sleep(4);
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

    chk(screen_destroy_group(group));
    chk(screen_destroy_event(ev));
    chk(screen_destroy_context(ctx));

    return 0;
}