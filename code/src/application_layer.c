// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <string.h>
#include <stdio.h>
#include "constants.h"

unsigned char sequence_number = 0;

long int calculate_file_size(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    fseek(fp, 0L, SEEK_END);
    long int res = ftell(fp);
    fclose(fp);
    return res;
}

void buildDataPacket(unsigned char *buf, int num_bytes_read, unsigned char *packet)
{
    if (sequence_number == 99)
        sequence_number = 0;

    unsigned char l1 = num_bytes_read / 256;
    unsigned char l2 = num_bytes_read % 256;

    buf[0] = 2;
    buf[1] = sequence_number;
    sequence_number++;
    buf[2] = l1;
    buf[3] = l2;
    memcpy(buf + 4, packet, num_bytes_read);
}

void buildControlPacket(unsigned char *buf, int start, const char *filename, int *ctrl_packet_size)
{
    int file_size = calculate_file_size(filename);

    printf("filesize :%d\n",file_size);

    buf[0] = start;
    buf[1] = 0;
    buf[2] = sizeof(file_size);
    buf[3] = file_size;
    buf[4] = file_size >> 8;
    buf[5] = file_size >> 16;
    buf[6] = 1;
    buf[7] = strlen(filename);
    for (int i = 0; i < strlen(filename); i++)
    {
        buf[i + 8] = filename[i];
    }
    *ctrl_packet_size = strlen(filename) + 8;
}

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    // Building connection parameters
    LinkLayerRole r = strcmp(role, "tx") == 0 ? LlTx : LlRx;
    LinkLayer linkLayer;

    memset(linkLayer.serialPort, '\0', 50);
    strcpy(linkLayer.serialPort, serialPort);

    linkLayer.role = r;
    linkLayer.baudRate = baudRate;
    linkLayer.nRetransmissions = nTries;
    linkLayer.timeout = timeout;

    // CAREFUL WITH THE BUF_SIZE
    llopen(linkLayer);
    // TODO: error handlingshed changes

/*     unsigned char buf[100] = {0};

    if(linkLayer.role == LlTx) {
        unsigned char send[11] = {0x1,0x2,0x3,0x4,0x7e,0x7e,0x5};
        llwrite(send,11);
    } else {
        llread(buf);
    } */

    if (linkLayer.role == LlTx)
    {
        int ctrl_packet_size = 0;

        unsigned char initial_ctrl_packet[100] = {0};
        buildControlPacket(initial_ctrl_packet, 1, filename, &ctrl_packet_size);
        llwrite(initial_ctrl_packet, ctrl_packet_size);

        printf("Start control packet sent!\n");
        unsigned char part_penguin[1000] = {0};

        FILE *ptr;
        ptr = fopen(filename, "rb");
        printf("File opened to read!\n");

        int num_bytes_read;

        while ((num_bytes_read = fread(part_penguin, sizeof(char), 1000, ptr)) > 0)
        {
            unsigned char data_packet[1004] = {0};
            buildDataPacket(data_packet, num_bytes_read, part_penguin);
            printf("data_packet:");
            for(int i = 0; i < 1004; i++) {
                printf(" 0x%02x ",data_packet[i]);
            }
            printf("\n");

            if (llwrite(data_packet, num_bytes_read + 4) != 0)
            {
                printf("File corrupted. Exiting\n");
                return;
            }
            printf("Sent part of penguin!\n");
        }

        unsigned char final_ctrl_packet[100] = {0};
        ctrl_packet_size = 0;

        printf("sending final control packet...\n");
        buildControlPacket(final_ctrl_packet, 3, filename, &ctrl_packet_size);
        llwrite(final_ctrl_packet, ctrl_packet_size);
    }
    else
    {
        // open file
        FILE *ptr;
        ptr = fopen(filename, "wb+");

        // read initial control packet
        unsigned char initial_ctrl_packet[100] = {0};
        int k = 0;

        llread(initial_ctrl_packet);
        printf("initial_ctrl_packet:");
        for(int i = 0; i < 8; i++) {
            printf(" 0x%02x ",initial_ctrl_packet[i]);
        } 
        printf("\n");

        if (initial_ctrl_packet[0] != 1)
            return;

        printf("Initial control packet received\n");

        // TODO: check other control packet chars

        unsigned char part_penguin[1004] = {0};
        unsigned char to_write_penguin[1000] = {0};

        printf("Trying to read penguin...\n");
        llread(part_penguin);

        while (1)
        {
            if (part_penguin[0] != 2)
            {
                printf("Not penguin by now. Skipping...\n");
                break;
            }

            int size = part_penguin[2] * 256 + part_penguin[3];

            memcpy(to_write_penguin, part_penguin + 4, size);
            printf("copied %d bytes from received to write \n",size);
            printf("data_packet:");
            for(int i = 0; i < 1000; i++) {
                printf(" 0x%02x ",part_penguin[i+4]);
            } 
            printf("\n");

            fwrite(to_write_penguin, 1, size, ptr);
            printf("Writing part of penguin...\n");

            printf("Trying to read penguin...\n");
            llread(part_penguin); 
            printf("frame %d\n",k);
            k++;
        }

        if (part_penguin[0] != 3)
        {
            printf("Control packet was not a finish one\n");
            return;
        }

        printf("Received final control packet. Exiting...\n");
    }

    llclose(0);
}; 
