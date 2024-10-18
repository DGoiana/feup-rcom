int createSET(unsigned char *set);
int sendWithTimeout(unsigned char *buffer_receive, unsigned char *buffer_send, int timeout, int fd, int *current_alarm_count);
void alarmHandler(int signal);
bool checkResponse(unsigned char *buffer_received);