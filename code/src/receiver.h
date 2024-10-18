#include <stdbool.h>

int receiveMessage(unsigned char *buf, int fd);
bool checkMessage(unsigned char *buf);
int sendResponse(int fd);