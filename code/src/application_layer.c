// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <string.h>
#include <stdio.h>
#include "constants.h"
#include <limits.h>

unsigned char sequence_number = 0;

struct ctrlPacket {
    unsigned char type;
    unsigned char file_name[UCHAR_MAX + 1];
    size_t file_size;
};

struct dataPacket {
    int sequence_number;
    size_t packet_size;
    unsigned char
}

size_t calculate_file_size(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    fseek(fp, 0L, SEEK_END);
    size_t res = ftell(fp);
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

size_t build_control_packet(unsigned char *packet, ctrlPacket ctrl_packet_info)
{
    buf[0] = ctrl_packet_info.type;

    buf[1] = PACKET_CONTROL_FILESIZE;

    size_t filesize = ctrl_packet_info.file_size;

    buf[2] = sizeof(filesize);
    buf[3] = filesize;
    buf[4] = filesize >> 8;
    buf[5] = filesize >> 16;
    
    buf[6] = PACKET_CONTROL_FILENAME;

    size_t filename_length = strlen(ctrl_packet_info.file_name);

    buf[7] = filename_length;

    memcpy(buf + 8, ctrl_packet_info.file_name, filename_length + 1);
    
    return filename_length + 8;
}

void application_layer_tx_protocol(const char* filename){
    unsigned char initial_ctrl_packet[MAX_PAYLOAD_SIZE];

    ctrlPacket ctrl_packet_info = {
        .type = CTRL_START,
        .file_size = calculate_file_size(filename)
    };

    strcpy(ctrlPacket.file_name, filename);

    size_t ctrl_packet_size = buildControlPacket(initial_ctrl_packet, ctrl_packet_info);

    if(llwrite(initial_ctrl_packet, ctrl_packet_size) != 0)){
        printf("Error writing control packet.\n");
        exit(1);
    }

    FILE* ptr = fopen(filename, "rb");
    

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

    llopen(linkLayer);
    // TODO: error handlingshed changes

    if (linkLayer.role == LlTx)
    {
        unsigned char part_penguin[1004] = {0};
        int num_bytes_read;

        while ((num_bytes_read = fread(part_penguin, sizeof(char), 1000, ptr)) > 0)
        {
            unsigned char data_packet[1004] = {0};
            buildDataPacket(data_packet, num_bytes_read, part_penguin);

            if (llwrite(data_packet, num_bytes_read + 4) != 0)
            {
                printf("File corrupted. Exiting\n");
                return;
            }
            printf("Sent part of penguin!\n");
        }

        unsigned char final_ctrl_packet[1000] = {0};
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
        unsigned char initial_ctrl_packet[1000] = {0};
        int k = 0;

        printf("waiting for control packet:\n");
        llread(initial_ctrl_packet);

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
            printf("copied %d bytes from received to write \n", size);

            fwrite(to_write_penguin, 1, size, ptr);
            printf("Writing part of penguin...\n");

            printf("Trying to read penguin...\n");
            llread(part_penguin);
            printf("frame %d\n", k);
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
