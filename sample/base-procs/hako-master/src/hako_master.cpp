#include <hako.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

static bool hako_master_is_end = false;

static void hako_master_signal_handler(int sig)
{
    //hako::logger::get("master")->info("SIGNAL RECV: {0}", sig);
    printf("SIGNAL RECV: %d\n", sig);
    hako_master_is_end = true;
}

int main(int argc, const char* argv[])
{
    if (argc != 3) {
        printf("Usage: %s <delta_msec> <max_delay_msec>\n", argv[0]);
        return 1;
    }
    printf("START\n");
    signal(SIGINT, hako_master_signal_handler);
    signal(SIGTERM, hako_master_signal_handler);

    HakoTimeType delta_usec = strtol(argv[1], NULL, 10) * 1000;
    HakoTimeType max_delay_usec = strtol(argv[2], NULL, 10) * 1000;

    hako::init();


    std::shared_ptr<hako::IHakoMasterController> hako_master = hako::create_master();
    hako_master->set_config_simtime(max_delay_usec, delta_usec);

    while (hako_master_is_end == false) {
        hako_master->execute();
        usleep(delta_usec);
    }

    hako::destroy();
    printf("EXIT\n");
    return 0;
}
