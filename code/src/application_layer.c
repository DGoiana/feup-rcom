// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include "constants.h"

typedef struct
{
    unsigned char type;
    unsigned char file_name[UCHAR_MAX + 1];
    size_t file_size;
} ctrlPacket;

typedef struct
{
    int sequence_number;
    size_t payload_length;
    unsigned char packet[MAX_PAYLOAD_SIZE];
} dataPacket;

void formatScreen(bool connectionOpened, double percentage, bool tx)
{
    printf("Opening connection...\n");
    if (connectionOpened)
        printf("Connection opened!\n");

    if (tx)
        printf("%0.0f%% Sent\n", percentage);
    else
        printf("%0.0f%% Received\n", percentage);
}

size_t calculate_file_size(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    fseek(fp, 0L, SEEK_END);
    size_t res = ftell(fp);
    fclose(fp);
    return res;
}

size_t build_data_packet(unsigned char *packet, dataPacket data_packet)
{

    unsigned char l1 = data_packet.payload_length / 256;
    unsigned char l2 = data_packet.payload_length % 256;

    packet[0] = DATA;
    packet[1] = data_packet.sequence_number;
    packet[2] = l1;
    packet[3] = l2;
    memcpy(packet + 4, data_packet.packet, data_packet.payload_length);

    return data_packet.payload_length + 4;
}

size_t build_control_packet(unsigned char *packet, ctrlPacket ctrl_packet_info)
{
    packet[0] = ctrl_packet_info.type;

    packet[1] = PACKET_CONTROL_FILESIZE;

    size_t filesize = ctrl_packet_info.file_size;

    packet[2] = sizeof(filesize);
    packet[3] = filesize;
    packet[4] = filesize >> 8;
    packet[5] = filesize >> 16;

    packet[6] = PACKET_CONTROL_FILENAME;

    size_t filename_length = strlen((const char *)ctrl_packet_info.file_name);

    packet[7] = filename_length;

    memcpy(packet + 8, ctrl_packet_info.file_name, filename_length + 1);

    return filename_length + 8;
}

void application_layer_tx_protocol(const char *filename)
{
    unsigned char initial_ctrl_packet[MAX_PAYLOAD_SIZE];

    ctrlPacket ctrl_packet_info = {
        .type = CTRL_START,
        .file_size = calculate_file_size(filename),
    };

    strcpy((char *)ctrl_packet_info.file_name, filename);

    size_t ctrl_packet_size = build_control_packet(initial_ctrl_packet, ctrl_packet_info);

    if (llwrite(initial_ctrl_packet, ctrl_packet_size) != 0)
    {
        printf("Error writing control packet.\n");
        exit(1);
    }

    FILE *ptr = fopen(filename, "rb");

    dataPacket data_packet;
    int sequence_number = 0;
    unsigned char packet[MAX_PAYLOAD_SIZE + 4];

    size_t num_bytes_read;
    double num_bytes_written;

    while ((num_bytes_read = fread(data_packet.packet, sizeof(char), MAX_PAYLOAD_SIZE, ptr)) > 0)
    {
        data_packet.sequence_number = sequence_number;
        data_packet.payload_length = num_bytes_read;

        size_t packet_size = build_data_packet(packet, data_packet);

        size_t total_file_size = 0;
        total_file_size |= initial_ctrl_packet[3];
        total_file_size |= initial_ctrl_packet[4] << 8;
        total_file_size |= initial_ctrl_packet[5];

        if (llwrite(packet, packet_size) != 0)
        {
            printf("File corrupted. Exiting\n");
            exit(1);
        }

        num_bytes_written += num_bytes_read;

        system("clear");
        formatScreen(true, num_bytes_written / total_file_size * 100, true);
    }

    unsigned char final_ctrl_packet[MAX_PAYLOAD_SIZE];
    ctrl_packet_info.type = CTRL_FINISH;
    ctrl_packet_size = build_control_packet(final_ctrl_packet, ctrl_packet_info);

    if (llwrite(final_ctrl_packet, ctrl_packet_size) != 0)
    {
        printf("Error writing control packet.\n");
        exit(1);
    }
}

void application_layer_rx_protocol(const char *filename)
{
    unsigned char initial_ctrl_packet[MAX_PAYLOAD_SIZE];
    llread(initial_ctrl_packet);

    if (initial_ctrl_packet[0] != CTRL_START)
    {
        printf("Initial control packet not found.\n");
        exit(1);
    }

    size_t total_file_size = 0;
    total_file_size |= initial_ctrl_packet[3];
    total_file_size |= initial_ctrl_packet[4] << 8;
    total_file_size |= initial_ctrl_packet[5];

    FILE *ptr = fopen(filename, "wb+");

    dataPacket data_packet;
    unsigned char packet[MAX_PAYLOAD_SIZE];

    double bytes_written = 0;

    while (TRUE)
    {
        llread(data_packet.packet);

        if (data_packet.packet[0] != DATA && data_packet.packet[0] == CTRL_FINISH)
            return;

        data_packet.payload_length = data_packet.packet[2] * 256 + data_packet.packet[3];

        memcpy(packet, data_packet.packet + 4, data_packet.payload_length);
        fwrite(packet, 1, data_packet.payload_length, ptr);
        bytes_written += data_packet.payload_length;

        system("clear");
        formatScreen(true, bytes_written / total_file_size * 100, false);
    }
}

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{

    LinkLayerRole r = strcmp(role, "tx") == 0 ? LlTx : LlRx;
    LinkLayer linkLayer;

    memset(linkLayer.serialPort, '\0', 50);
    strcpy(linkLayer.serialPort, serialPort);

    linkLayer.role = r;
    linkLayer.baudRate = baudRate;
    linkLayer.nRetransmissions = nTries;
    linkLayer.timeout = timeout;

    llopen(linkLayer);

    if (r == LlTx)
        application_layer_tx_protocol(filename);
    else
        application_layer_rx_protocol(filename);

    printf("Closing...\n");
    llclose(1);
};
