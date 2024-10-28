#include <time.h>
#include <sys/time.h>

void stat_start_timer();

double stat_get_time();

void stat_set_bits_received(int bytes);

long stat_get_bits_received();

double stat_get_bitrate(double time);

void stat_add_bad_frame();
long stat_get_bad_frames();

void stat_add_good_frame();
long stat_get_good_frames();

double stat_get_fer();