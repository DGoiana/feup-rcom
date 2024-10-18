// Write to serial port in non-canonical mode
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include "alarm.c"

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256

#define FLAG 0x7E
#define A_WRITE 0x03
#define C_WRITE 0x03

volatile int STOP = FALSE;

int next_step(int state, unsigned char *buffer);

void print_buffer(unsigned char *buf_read)
{
    for (int i = 0; buf_read[i] != '\0'; i++)
    {
        printf("var = 0x%02X\n", buf_read[i]);
    }
}

int main(int argc, char *argv[])
{
    // Program usage: Uses either COM1 or COM2
    const char *serialPortName = argv[1];

    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(serialPortName, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0.1; // Inter-character timer unused
    newtio.c_cc[VMIN] = 5;    // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    // Create string to send
    unsigned char buf[BUF_SIZE] = {0};
    unsigned char buf_read[BUF_SIZE] = {0};
    unsigned char flag_checker[1 + 1] = {0};
    unsigned char byte_checker[1 + 1] = {0};

    // In non-canonical mode, '\n' does not end the writing.
    // Test this condition by placing a '\n' in the middle of the buffer.
    // The whole buffer must be sent even with the '\n'.
    int state = 0;
    int bytes = 0;
    int flag = 0;

    buf[0] = FLAG;
    buf[1] = A_WRITE;
    buf[2] = C_WRITE;
    buf[3] = A_WRITE ^ C_WRITE;
    buf[4] = 0x01;
    buf[5] = 0x02;
    buf[6] = 0x00;
    buf[7] = FLAG;

    while (alarmCount < 4)
    {

        if (alarmEnabled == FALSE)
        {
            alarm(3); // Set alarm to be triggered in 3s
            alarmEnabled = TRUE;
            bytes = write(fd, buf, 8);
            printf("sent bytes\n");
        }

        flag = read(fd, flag_checker, 1);

        if (flag_checker[0] == FLAG)
        {
            buf_read[0] = FLAG;
            for (int i = 1; i < 5; i++)
            {
                flag = read(fd, byte_checker, 1);
                buf_read[i] = byte_checker[0];
                // printf("var = 0x%02X\n", buf_read[i]);
            }
            for (int i = 0; i < 5; i++)
            {
                // print_buffer(buf_read);
                state = next_step(state, buf_read);
            }

            buf_read[bytes] = '\0';

            if (state == 5)
            {
                for (int i = 0; i < 5; i++)
                {
                    printf("var = 0x%02X\n", buf_read[i]);
                }
                return 0;
            }
        }

        state = 0;
        sleep(1);
    }

    printf("timed out\n");

    // Wait until all bytes have been written to the serial port
    sleep(1);

    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}

#define START 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_OK 4
#define STOP 5

int next_step(int state, unsigned char *buffer)
{
    switch (state)
    {
    case START:
        if (buffer[0] == FLAG)
            return FLAG_RCV;
        return state;
    case FLAG_RCV:
        if (buffer[1] == FLAG)
            return state;
        else if (buffer[1] == 0x01)
            return A_RCV;
        return START;
    case A_RCV:
        if (buffer[2] == 0x07)
            return C_RCV;
        else if (buffer[2] == FLAG)
            return FLAG_RCV;
        return START;
    case C_RCV:
        if (buffer[3] == (buffer[1] ^ buffer[2]))
            return BCC_OK;
        else if (buffer[3] == FLAG)
            return FLAG_RCV;
        return START;
    case BCC_OK:
        if (buffer[4] == FLAG)
            return STOP;
        return START;
    default:
        break;
    }
    return START;
}
