/* 
 * File:   main.c
 * Author: alpsayin
 *
 * Created on December 2, 2011, 12:00 AM
 */
#include <termios.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>

#define DEFAULT_ADDRESS 0
#define DEFAULT_COMMAND COMMAND_CONFIGURE

#define BAUDRATE B57600
#define MODEMDEVICE "/dev/ttyUSB0"
#define _POSIX_SOURCE 1         //POSIX compliant source
#define FALSE 0
#define TRUE 1

#include "packet_types.h"
#include "main.h"

volatile int listen = 0;
volatile int wait_flag = 0;

char devicename[80] = "/dev/ttyUSB0";
long BAUD = B57600; // derived baud rate from command line
long DATABITS = CS8; //8 data bits
long STOPBITS = 0; //1 stop bit
long PARITYON = 0; //no parity
long PARITY = 0; //no parity

int fd, tty;
FILE *input;
FILE *output;
int status;

command_packet_t commandPacket;
command_packet_t receivedCommandPacket;


struct termios oldkey, newkey; //place tor old and new port settings for keyboard teletype
struct termios newtio, oldtio; //place for old and new port settings for serial port
struct sigaction saio; //definition of signal action

int main(int argc, char** argv)
{
    char buf[256];
    char* ptr;
    int i = 0, val = 0, no_command = 0;
    char Key;
    uint8_t confirmation = 1, verbose = 0;

    input = stdin;
    output = stdout;

    const char instr[] = "\nsendKthWsnCommand command utility\n\
----------------------------------------------------------\
\nSends a command through serial port to the root mote to be dispatched by radio\n\
If no command is specified by entering either -t or -r then -e is implied,\
 but if -t is entered with -e then an echo command with a value will be sent, no interval will be set.\
 If -e is entered, it overrides -t and -r, else if -t is entered it overrides -r. \n\
    Options:\n\
    -hE set history enable (0/1) \n\
    -bE burst enable (0/1) (if this is not 1 -h is not effective) \n\
    -wE write enable (0/1) (if this is not 1 -h -b are not effective) \n\
    -tTimeU set measurement interval in seconds, minutes or hours (U=s/m/h) \n\
    -rType request data of Type (data/history/status)   \n\
    -aAddress the address that issue the command (address in radix 16)\n\
    -f no confirmation \n\
    -e send echo command, expecting the same command to return\n\
    -l enables listening after command issue, if used no command has to be entered\n\
\n\
Example: \n\
sendKthWsnCommand -h1 -b1 -w1 -t1h -rd -aFFFF -f -l \n\
    history enabled \n\
    burst enabled \n\
    write enabled \n\
    interval=1 hour \n\
    start listening the network after command\n\
    read data from all nodes (broadcast) \n";

    signal(SIGINT, sigint_handler);


    input = fopen("/dev/tty", "r"); //open the terminal keyboard
    output = fopen("/dev/tty", "w"); //open the terminal screen

    if (!input || !output)
    {
        fprintf(stderr, "Unable to open /dev/tty\n");
        restoreDefaults();
        return EXIT_FAILURE;
    }

    tty = open("/dev/tty", O_RDWR | O_NOCTTY | O_NONBLOCK); //set the user console port up
    tcgetattr(tty, &oldkey); // save current port settings   //so commands are interpreted right for this program

    cfsetispeed(&newkey, BAUDRATE);
    cfsetospeed(&newkey, BAUDRATE);

    // set new port settings for non-canonical input processing  //must be NOCTTY
    newkey.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
    newkey.c_iflag = IGNPAR;
    newkey.c_oflag = OPOST | ONLCR;
    newkey.c_lflag = ~(ICANON | ECHO | ECHOE | ISIG); //ICANON;
    newkey.c_cc[VMIN] = 1;
    newkey.c_cc[VTIME] = 0;
    tcflush(tty, TCIFLUSH);
    tcsetattr(tty, TCSANOW, &newkey);

    commandPacket.address = DEFAULT_ADDRESS; //default address is broadcast
    commandPacket.opcode = DEFAULT_COMMAND;

    for (i = 0; i < argc; i++)
    {
        if (!strncmp(argv[i], "-h", 2))
        {
            if (strlen(argv[i]) == 2)
            {
                fputs(instr, output);
                fputs("-h has too few parameters\n", output);
                restoreDefaults();
                return EXIT_FAILURE;
            }
            val = strtol(argv[i] + 2, &ptr, 2);
            if (ptr == argv[i] + 2)
            {
                fputs(instr, output);
                fputs("-h has faulty parameter\n", output);
                restoreDefaults();
                return EXIT_FAILURE;
            }
            commandPacket.HE = val > 0;
        }
        else if (!strncmp(argv[i], "-b", 2))
        {
            if (strlen(argv[i]) == 2)
            {
                fputs(instr, output);
                fputs("-b has too few parameters\n", output);
                restoreDefaults();
                return EXIT_FAILURE;
            }
            val = strtol(argv[i] + 2, &ptr, 2);
            if (ptr == argv[i] + 2)
            {
                fputs(instr, output);
                fputs("-b has faulty parameter\n", output);
                restoreDefaults();
                return EXIT_FAILURE;
            }
            commandPacket.BE = val > 0;
        }
        else if (!strncmp(argv[i], "-w", 2))
        {
            if (strlen(argv[i]) == 2)
            {
                fputs(instr, output);
                fputs("-w has too few parameters\n", output);
                restoreDefaults();
                return EXIT_FAILURE;
            }
            val = strtol(argv[i] + 2, &ptr, 2);
            if (ptr == argv[i] + 2)
            {
                fputs(instr, output);
                fputs("-w has faulty parameter\n", output);
                restoreDefaults();
                return EXIT_FAILURE;
            }
            commandPacket.WE = val > 0;
        }
        else if (!strncmp(argv[i], "-r", 2))
        {
            if (strlen(argv[i]) == 2)
            {
                fputs(instr, output);
                fputs("-r has too few parameters\n", output);
                restoreDefaults();
                return EXIT_FAILURE;
            }
            val = argv[i][2];
            if (val == 'd')
                commandPacket.opcode = COMMAND_READ_DATA;
            else if (val == 'h')
                commandPacket.opcode = COMMAND_READ_HISTORY;
            else if (val == 's')
                commandPacket.opcode = COMMAND_READ_STATUS;
            else
            {
                fputs(instr, output);
                fputs("-r has faulty parameter\n", output);
                restoreDefaults();
                return EXIT_FAILURE;
            }
        }
        else if (!strncmp(argv[i], "-t", 2))
        {
            if (strlen(argv[i]) == 2)
            {
                fputs(instr, output);
                fputs("-t has too few parameters\n", output);
                restoreDefaults();
                return EXIT_FAILURE;
            }
            val = strtol(argv[i] + 2, &ptr, 10);
            if (ptr == argv[i] + 2)
            {
                fputs(instr, output);
                fputs("-t has faulty parameter\n", output);
                restoreDefaults();
                return EXIT_FAILURE;
            }
            if (*ptr == 's')
                commandPacket.opcode = COMMAND_INTERVAL_SECONDS;
            else if (*ptr == 'm')
                commandPacket.opcode = COMMAND_INTERVAL_MINUTES;
            else if (*ptr == 'h')
                commandPacket.opcode = COMMAND_INTERVAL_HOURS;
            else
            {
                fputs(instr, output);
                fputs("unknown time unit for -t\n", output);
                restoreDefaults();
                return EXIT_FAILURE;
            }
            commandPacket.value = val;
        }
        else if (!strncmp(argv[i], "-a", 2))
        {
            if (strlen(argv[i]) == 2)
            {
                fputs(instr, output);
                fputs("-a has too few parameters\n", output);
                restoreDefaults();
                return EXIT_FAILURE;
            }
            val = strtol(argv[i] + 2, &ptr, 16);
            if (ptr == argv[i] + 2)
            {
                fputs(instr, output);
                fputs("-a has faulty parameter\n", output);
                restoreDefaults();
                return EXIT_FAILURE;
            }
            commandPacket.address = val;
        }
        else if (!strncmp(argv[i], "-e", 2))
        {
            if (strlen(argv[i]) != 2)
            {
                fputs(instr, output);
                fputs("-e has too many parameters\n", output);
                restoreDefaults();
                return EXIT_FAILURE;
            }
            commandPacket.opcode = COMMAND_ECHO;
        }
        else if (!strncmp(argv[i], "-f", 2))
        {
            if (strlen(argv[i]) != 2)
            {
                fputs(instr, output);
                fputs("-f has too many parameters\n", output);
                restoreDefaults();
                return EXIT_FAILURE;
            }
            confirmation = 0;
        }
        else if (!strncmp(argv[i], "-v", 2))
        {
            if (strlen(argv[i]) != 2)
            {
                fputs(instr, output);
                fputs("-v has too many parameters\n", output);
                restoreDefaults();
                return EXIT_FAILURE;
            }
            verbose = 1;
        }
        else if (!strncmp(argv[i], "-l", 2))
        {
            if (strlen(argv[i]) != 2)
            {
                fputs("listen", output);
                fputs(instr, output);
                fputs("-l has too many parameters\n", output);
                restoreDefaults();
                return EXIT_FAILURE;
            }
            listen = 1;
        }
    }

    commandPacket.HE &= commandPacket.WE; //write must be enabled for history enable
    commandPacket.BE &= commandPacket.HE; //history must be enabled for burst enable

    printf("%d %d\n", commandPacket.WE, listen);
    if (!commandPacket.WE)
    {
        if (commandPacket.opcode == COMMAND_CONFIGURE)
        {
            if (!listen)
            {
                fputs(instr, output);
                fputs("no command is entered\n", output);
                restoreDefaults();
                return EXIT_FAILURE;
            }
            no_command = 1;
        }
    }
    if (!no_command)
    {
        if (verbose)
        {
            fputs("\n", output);
            fputs("Command Packet to be Sent\n", output);
            sprintf(buf, "Write Enable\t:\t%d\n", commandPacket.WE);
            fputs(buf, output);
            sprintf(buf, "History Enable\t:\t%d\n", commandPacket.HE);
            fputs(buf, output);
            sprintf(buf, "Burst Enable\t:\t%d\n", commandPacket.BE);
            fputs(buf, output);
            switch (commandPacket.opcode)
            {
            case COMMAND_ECHO:
                sprintf(buf, "Opcode\t\t:\tCommand Echo\n", commandPacket.opcode);
                fputs(buf, output);
                break;
            case COMMAND_READ_DATA:
                sprintf(buf, "Opcode\t\t:\tRead Data\n", commandPacket.opcode);
                fputs(buf, output);
                break;
            case COMMAND_READ_HISTORY:
                sprintf(buf, "Opcode\t\t:\tRead History\n", commandPacket.opcode);
                fputs(buf, output);
                break;
            case COMMAND_READ_STATUS:
                sprintf(buf, "Opcode\t\t:\tRead Status\n", commandPacket.opcode);
                fputs(buf, output);
                break;
            case COMMAND_INTERVAL_SECONDS:
                sprintf(buf, "Opcode\t\t:\tSet Interval Seconds\n", commandPacket.opcode);
                fputs(buf, output);
                break;
            case COMMAND_INTERVAL_MINUTES:
                sprintf(buf, "Opcode\t\t:\tSet Interval Minutes\n", commandPacket.opcode);
                fputs(buf, output);
                break;
            case COMMAND_INTERVAL_HOURS:
                sprintf(buf, "Opcode\t\t:\tSet Interval Hours\n", commandPacket.opcode);
                fputs(buf, output);
                break;
            case COMMAND_CONFIGURE:
                sprintf(buf, "Opcode\t\t:\tConfigure Settings\n", commandPacket.opcode);
                fputs(buf, output);
                break;
            default:
                sprintf(buf, "Opcode\t\t:\tUNKNOWN COMMAND\n", output);
                fputs(buf, output);
            }
            sprintf(buf, "Value\t\t:\t%d\n", commandPacket.value);
            fputs(buf, output);
            if (commandPacket.address == 0xFFFF)
            {
                fputs("Address\t\t:\tAll\n", output);
            }
            else
            {
                sprintf(buf, "Address\t\t:\t0x%x\n\n", commandPacket.address);
                fputs(buf, output);
            }
            fflush(output);
        }
        else
        {
            commandPacketToStr(&commandPacket, buf);
            fputs(buf, output);
            fputs("\n", output);
        }
    }

    if (confirmation)
    {
        fputs("Do you want to continue?[y/N]\n", output);
        val = getchar();
    }

    if (val != 'y' && confirmation == 1)
    {
        fputs("command sending CANCELED...\n", output);
        restoreDefaults();
        return EXIT_FAILURE;
    }

    sprintf(buf, "trying to open %s for read/write\n", devicename);
    fputs(buf, output);
    if (openComPort(O_RDWR))
    {
        restoreDefaults();
        return EXIT_FAILURE;
    }

    if (!no_command)
    {

        fputs("sending command to the root mote...\n", output);
        //TODO feed the command packet byte by byte using a

        val = write(fd, &commandPacket, sizeof (command_packet_t));
        sprintf(buf, "%d bytes written\n", val);
        fputs(buf, output);
        commandPacketToStr(&commandPacket, buf);
        fputs(buf, output);
        fputs("\n", output);
    }

    if (listen)
        fputs("started listening...\n", output);

    while (listen)
    {
        status = fread(&Key, 1, 1, input);
        //usleep(200);
        if (status == 1) //if a key was hit
        {
            switch (Key)
            { /* branch to appropiate key handler */
            case 0x1b: /* Esc */
                listen = 0;
                break;
            default:
                fputc((int) Key, output);
                break;
            } //end of switch key
        } //end if a key was hit
        if (!wait_flag)
        {
            val = read(fd, buf, 256);
            if (val > 0)
            {
                for (i = 0; i < val; i++)
                {
                    fputc((int) buf[i], stdout);
                }
                wait_flag = 1;
            }
            fflush(stdout);
        }
    }

    restoreDefaults();

    return (EXIT_SUCCESS);
}

void sigint_handler(int sig)
{
    signal(sig, SIG_IGN);
    restoreDefaults();
    listen = 0;
    exit(0);
}

/***************************************************************************
 * signal handler. sets wait_flag to FALSE, to indicate above loop that     *
 * characters have been received.                                           *
 ***************************************************************************/

void signal_handler_IO(int status)
{
    /*
        fputs("sigio\n", output);
     */
    wait_flag = 0;
}

int openComPort(long rw)
{
    char errBuf[80];

    //open the device(com port) to be non-blocking (read will return immediately)

    fd = open(devicename, rw | O_NOCTTY | O_NONBLOCK);
    if (fd < 0)
    {
        sprintf(errBuf, "cannot open %s", devicename);
        fputs(errBuf, output);
        return 1;
    }


    //wait before flush because of a kernel bug
    usleep(200000);
    //discard the stupid data, which may cause overflow in the tasks
    tcflush(fd, TCIOFLUSH);
    //install the serial handler before making the device asynchronous
    saio.sa_handler = signal_handler_IO;
    sigemptyset(&saio.sa_mask); //saio.sa_mask = 0;
    saio.sa_flags = 0;
    saio.sa_restorer = NULL;
    sigaction(SIGIO, &saio, NULL);

    // allow the process to receive SIGIO
    fcntl(fd, F_SETOWN, getpid());
    // Make the file descriptor asynchronous (the manual page says only
    // O_APPEND and O_NONBLOCK, will work with F_SETFL...)
    fcntl(fd, F_SETFL, O_ASYNC);

    tcgetattr(fd, &oldtio); // save current port settings

    cfsetispeed(&newtio, BAUD);
    cfsetospeed(&newtio, BAUD);

    // set new port settings for canonical input processing
    newtio.c_cflag |= BAUD | DATABITS | STOPBITS | PARITYON | PARITY | CLOCAL | CREAD;
    newtio.c_cflag &= ~CRTSCTS;
    newtio.c_iflag |= IGNPAR;
    newtio.c_iflag &= ~(IXON | IXOFF | IXANY);
    newtio.c_oflag &= ~OPOST;
    newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); //ICANON;
    newtio.c_cc[VMIN] = 1;
    newtio.c_cc[VTIME] = 0;
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &newtio);

    usleep(200000);
    //now that the attributes are actually set, flush the previously received data just to make sure
    tcflush(fd, TCIOFLUSH); //what made the difference was the first tcflush call, but still keeping this, can't hurt

    return 0;
}

void restoreDefaults()
{
    fputs("exiting...\n", output);
    tcsetattr(tty, TCSANOW, &oldkey);
    close(tty);
    close(fd);
}
