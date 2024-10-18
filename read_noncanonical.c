// Read from serial port in non-canonical mode
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
#include <stdbool.h>
#include "constants.h"

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256

#define FLAG 0x7E
#define A_READ 0x01
#define C_READ 0x07

int create_response(unsigned char response_buf[], int state, bool iterator, bool is_initialized, bool to_finish)
{
    response_buf[0] = F_FLAG;
    response_buf[1] = A_RC;
    if (is_initialized == FALSE)
        response_buf[2] = C_UA;
    else if (to_finish == TRUE)
        response_buf[2] = C_DISC;
    else if (state == RESEND)
        response_buf[2] = iterator == FALSE ? C_RR0 : C_RR1;
    else
        response_buf[2] = iterator == FALSE ? C_REJ0 : C_REJ1;
    response_buf[3] = BCC1(response_buf[1], response_buf[2]);
    response_buf[4] = F_FLAG;
    return 0;
}

int next_step_stop_wait(int state, char byte, char bcc)
{
    switch (state)
    {
    case START:
        if (byte == FLAG)
            return FLAG_RCV;
        return state;
    case FLAG_RCV:
        if (byte == FLAG)
            return state;
        else if (byte == A_TX)
            return A_RCV;
        return START;
    case A_RCV:
        if (byte == A_TX)
            return C_RCV;
        else if (byte == FLAG)
            return FLAG_RCV;
        return START;
    case C_RCV:
        if (byte == bcc)
            return BCC_OK;
        else if (byte == FLAG)
            return FLAG_RCV;
        return START;
    case BCC_OK:
        if (byte != FLAG)
            return DATA_RCV;
        return START;
    case DATA_RCV:
        if (byte != FLAG)
            return DATA_RCV;
        return FINAL_FLAG_RCV;
    case FINAL_FLAG_RCV:
        if (byte == bcc)
            return STOP;
        return RESEND;

    default:
        break;
    }
    return START;
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

    // Open serial port device for reading and writing and not as controlling tty
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
    newtio.c_cc[VMIN] = 1;    // Blocking read until 5 chars received

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

    // Loop for input
    unsigned char buf[BUF_SIZE + 1] = {0}; // +1: Save space for the final '\0' char
    unsigned char read_buf[1 + 1] = {0};
    int state = 0;
    int index = 0;
    int current = 0;
    int bytes = 0;

    // Returns after 5 chars have been input

    while (1)
    {
        while (current < 8)
        {
            printf("reading byte:\n");
            bytes = read(fd, read_buf, 1);
            buf[current] = read_buf[0];
            current++;
        }
        current = 0;

        char bcc1 = buf[1] ^ buf[2];
        char bcc2 = 0x0;

        printf("[");
        for (int i = 0; i < 8; i++)
        {
            printf("0x%02X, ", buf[i]);
        }
        printf("]\n");

        while (state != STOP && index < 8 + 1)
        {
            if (state == C_RCV)
            {
                state = next_step_stop_wait(state, buf[index], bcc1);
                printf("here - ");
            }
            else if (state == DATA_RCV)
            {
                bcc2 = bcc2 ^ buf[index - 1];
                state = next_step_stop_wait(state, buf[index], 0x00);
                printf("buf[index]: 0x%02X\n", buf[index]);
                printf("here2 | current bcc2 0x%02X - ", bcc2);
            }
            else if (state == FINAL_FLAG_RCV)
            {
                state = next_step_stop_wait(state, buf[index - 2], bcc2);
                printf("here3 - ");
            }
            else
                state = next_step_stop_wait(state, buf[index], 0x00);

            printf("current state: %d\n", state);
            printf("var = 0x%02X\n", buf[index]);
            index++;
        }
        printf("final state: %d\n", state);

        /*if (state == STOP || state == RESEND)
        {
            // create_response(buf, state, FALSE, FALSE, FALSE);
            buf[0] = F_FLAG;
            buf[1] = A_RC;
            buf[2] = C_UA;
            buf[3] = buf[1] ^ buf[2];
            buf[4] = F_FLAG;
            bytes = write(fd, buf, 5);
            printf("%d bytes written\n", bytes);
            state = 0;
            break;
        }
    */
    }

    // The while() cycle should be changed in order to respect the specifications
    // of the protocol indicated in the Lab guide

    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}