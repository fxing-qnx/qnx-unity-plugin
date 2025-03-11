#include "test.hxx"

int //
main(int argc, char** argv)
{
    test_window_thr test_win;
    std::atomic_bool running(true);
    char group_name[] = "QNX800WindowMapperGroup";
    test_win.run(running, group_name);

    return 0;
}