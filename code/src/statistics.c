#include "statistics.h"

static struct timeval start;
static struct timeval start_frame;
static long received_bits;
static long last_received_bytes;
static long bad_frames;
static long good_frames;
static double sum_efficiency;

void stat_start_timer() {
  gettimeofday(&start,NULL);
}

void stat_start_frame_timer() {
  gettimeofday(&start_frame,NULL);
}

double stat_get_t_total() {
  struct timeval finish_frame;
  gettimeofday(&finish_frame,NULL);

  return (finish_frame.tv_sec - start_frame.tv_sec) + (finish_frame.tv_usec - start_frame.tv_usec) / 1000000.0;
}

void stat_add_total_efficiency(double frame_efficiency) {
  sum_efficiency = sum_efficiency + frame_efficiency;
}

double stat_get_average_effiency() {
  return sum_efficiency / (good_frames + bad_frames);
}

double stat_get_average_a() {
  double average_efficiency = stat_get_average_effiency();
  return ((1/average_efficiency) - 1)/2;
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