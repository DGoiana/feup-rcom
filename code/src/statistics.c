#include "statistics.h"

static struct timeval start;
long received_bits;
long last_received_bytes;
long bad_frames;
long good_frames;

void stat_start_timer() {
gettimeofday(&start,NULL);
return;
}

double stat_get_time() {
struct timeval finish;
gettimeofday(&finish,NULL);

return (finish.tv_sec - start.tv_sec) + (finish.tv_usec - start.tv_usec) / 1000000.0;
}

void stat_set_bits_received(int bytes) {
received_bits = bytes * 8;
}

long stat_get_bits_received() {
return received_bits;
}

double stat_get_bitrate(double time) {
return received_bits / time;
}

void stat_add_bad_frame() {
bad_frames++;
}

long stat_get_bad_frames() {
return bad_frames;
}

void stat_add_good_frame() {
good_frames++;
}

long stat_get_good_frames() {
return good_frames;
}

double stat_get_fer() {
return bad_frames / ((double) (bad_frames + good_frames));
}