// TODO: CHANGE THIS (AVOID USING THE HEAP FOR THIS - IT WILL BRING PROBLEMS)
#define BUF_SIZE 1024

/* F values */
#define F_FLAG 0x7E

/*  A values */
// Address field in frames that are commands sent by the Transmitter or replies sent by the Receiver
#define A_TX 0x03
// Address field in frames that are commands sent by the Receiver or replies sent by the Transmitter
#define A_RC 0x01

#define ESC 0x07D
#define REPLACED (F_FLAG ^ 0x20)

/* C values */
#define C_SET 0x03
#define C_UA 0x07
#define C_RR0 0xAA
#define C_RR1 0xAB
#define C_REJ0 0x54
#define C_REJ1 0x55
#define C_DISC 0x0B
#define C_FRAME0 0x00
#define C_FRAME1 0x80

#define BCC(A, C) (A ^ C)

/* STATE MACHINE STATES */
#define START 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_OK 4
#define DATA_RCV 5
#define DATA_STUFFED 6
#define STUFFED_RECEIVED 7
#define FINAL_FLAG_RCV 8
#define STOP 9
#define RESEND 10

#define BCC2_CHECK 5

/* PACKETS */

#define CTRL_START 0x01
#define CTRL_FINISH 0x03
#define DATA 0x02
#define PACKET_CONTROL_FILESIZE 0x00
#define PACKET_CONTROL_FILENAME 0x01