#include "constants.h"
#include <stdbool.h>

void state_machine_sendSET(unsigned char byte, int *state, bool tx);
void state_machine_control_packet(int *state, unsigned char byte, int frame_nr);
void state_machine_writes(int *state, unsigned char byte, bool bcc2_checked,int frame_ns);
void state_machine_close(int *state, unsigned char byte);