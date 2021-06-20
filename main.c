/**
  * @file main.c
  * Open serial port, send AT commands and receive modem response.
*/
#include "main.h"
/**
 * Opens Serial port and sets the correct settings
 * Error on opening serial Port, returns -1
 * Settings:
 * No parity
 * 1 stop bits
 * 8 byte size
 * Baudrate:115200
 * 
*/
int OpenSerial()
{
    int serial_port = open("/dev/ttyUSB0", O_RDWR);

    if (serial_port == -1) /* Error Checking */
    {
#if DEBUG == 1
        printf("\n  Error! in Opening ttyUSB0  ");
#endif
        return -1;
    }
    else
    {
#if DEBUG == 1
        printf("\n  ttyUSB0 Opened Successfully ");
#endif
    }
    // Create new termios struc, we call it 'tty' for convention
    struct termios tty;

    // Read in existing settings, and handle any error
    if (tcgetattr(serial_port, &tty) != 0)
    {
#if DEBUG == 1
        printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
#endif
        return -1;
    }

    tty.c_cflag &= ~PARENB;        // Clear parity bit, disabling parity
    tty.c_cflag &= ~CSTOPB;        // Clear stop field, only one stop bit used in communication
    tty.c_cflag &= ~CSIZE;         // Clear all bits that set the data size
    tty.c_cflag |= CS8;            // 8 bits per byte
    tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;                                                        // Disable echo
    tty.c_lflag &= ~ECHOE;                                                       // Disable erasure
    tty.c_lflag &= ~ECHONL;                                                      // Disable new-line echo
    tty.c_lflag &= ~ISIG;                                                        // Disable interpretation of INTR, QUIT and SUSP
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);                                      // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL); // Disable any special handling of received bytes

    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

    tty.c_cc[VTIME] = 10; // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
    tty.c_cc[VMIN] = 0;

    // Set in/out baud rate to be 115200
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    // Save tty settings, also checking for error
    if (tcsetattr(serial_port, TCSANOW, &tty) != 0)
    {
#if DEBUG == 1
        printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
#endif
        return -1;
    }

    return serial_port;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
*	Set of commands to be run on startup of the modem
*   Enable error report
*   Set RI line pulse duration to 500ms
*   Unlocks the sim card
*/
void InitialSetup()
{
    char msg[5][50];
    strcpy(msg[0], AT);
    strcpy(msg[1], CFUN "1\r");    //PSM OFF
    strcpy(msg[2], CMEE);          //Enables the report of the error result code in verbose format.
    strcpy(msg[3], PSMRI);         //Enables the ring indicator line (RI) response to an event while the modem is in power saving mode. Sets the pulse duration to 500ms.
    strcpy(msg[4], CPIN PIN "\r"); //SIM card PIN =0000

    for (int i = 0; i < 5; i++)
        ReadWrite(msg[i]);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
*   Start a TCP connection
*   Opens context 2
*   Opens the modem socket Nº5 and connects to a TCP server 
*/
void OpenSocketServer()
{
    char msg[2][50];

    strcpy(msg[0], SGACT SGACT_CONTEXT ",1\r");
    strcpy(msg[1], SD Socket_TCP ",0," PORT ",\"" IPadd "\",0,0,1\r");

    ReadWrite(msg[0]);
    ReadWrite(msg[1]);

    SendCommand();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
*   Terminates the TCP connection by closing the socket and deactivating the context
*/
void CloseSocketServer()
{
    char msg[2][50];

    strcpy(msg[0], SH Socket_TCP "\r");
    strcpy(msg[1], SGACT SGACT_CONTEXT ",0\r");

    ReadWrite(msg[0]);
    ReadWrite(msg[1]);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * To send data to the modem we have to write in the file ttyUSB0 
 */
int SendData(char msg[])
{
    int i;

    i = write(serial_port, msg, strlen(msg));

    if (i <= 0) /* Error Checking */
    {
#if DEBUG == 1
        printf("\n  Error! Sending DATA  ");
#endif
        return -1;
    }
    else
    {
        return i;
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * To read the modem response, read the file ttyUSB0
 */
int ReadData()
{
    char read_buf[256];

    // set everything to 0 so we can
    // call printf() easily.
    memset(&read_buf, '\0', sizeof(read_buf));

    // Read bytes.
    int num_bytes = read(serial_port, &read_buf, sizeof(read_buf)); ///get modem response

    if (num_bytes == -1)
    {
#if DEBUG == 1
        printf("\n  Error! Reading DATA  ");
#endif
        return -1
    }
    else
    {

#if DEBUG == 1
        printf("%s\n", read_buf); ///print modem response
#endif
        return num_bytes;
        fflush(stdin);
        fflush(stdout);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * set the Data Terminal Ready(DTR) to 1 to leave Power Save Mode(PSM) and communicate with the modem 
 */
void SetDTR()
{
    int status;
    ioctl(serial_port, TIOCMGET, &status); //GET DTR PIN status

    status |= TIOCM_DTR;
    ioctl(serial_port, TIOCMSET, status); //SET DTR PIN to 1 if to comunicate, if the modem is in power save
#if DEBUG == 1
    printf("\nDTR=1\n");
#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * set the DTR to 0 to enter PSM 
 */
void ClearDTR()
{
    int status;
    ioctl(serial_port, TIOCMGET, &status); //GET DTR PIN status

    status &= ~TIOCM_DTR;
    ioctl(serial_port, TIOCMSET, status); //SET DTR PIN t0 0, if the modem is in power save mode
#if DEBUG == 1
    printf("\nDTR=0\n");
#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *  Checks the at command sent:
 *  if AT+CFUN=5, then sets the DTR line to 0 to enter PSM
 *  if AT+CFUN=1, then sets the DTR line to 1 to leave PSM
 */
void CheckCFUN(char msg[])
{
    if (strcmp(msg, "at+cfun=5\r") == 0 || strcmp(msg, "AT+CFUN=5\r") == 0)
    {
        FLAG_DTR = 0;
        ClearDTR();
    }

    if (strcmp(msg, "at+cfun=1\r") == 0 || strcmp(msg, "AT+CFUN=1\r") == 0)
    {
        FLAG_DTR = 1;
        SetDTR();
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Checks if the FLAG_DTR    
 * @details If FLAG_DTR==0, the modem is in PSM, Sets the DTR line to 1     
 *          Sends Data    
 *          Receives the modem Response    
 *          If FLAG_DTR==0, the modem must turn back to PSM, Sets the DTR line to 0
 */
void ReadWrite(char msg[])
{
    int i, o = 0;

    if (FLAG_DTR == 0)
    {
        SetDTR();
    }

    CheckCFUN(msg);
    i = SendData(msg);
    o = ReadData(msg);

    if (FLAG_DTR == 0)
    {
        ClearDTR();
    }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Scans user inputted commands and sends to modem
 */
void SendCommand()
{
    char msg[80];
#if DEBUG == 1
    printf("\nGo Back to Menu Send: back");
#endif
    while (strcmp(msg, "back\r") != 0 && strcmp(msg, "BACK\r") != 0)
    {
        printf("\nCODE:");
        scanf("%s", msg);
        strncat(msg, "\r", 2);
        ReadWrite(msg);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
*	Run only the first time the modem is started.
*	Socket Configuration commands.
*	Selects PDP context identifier 2, selects �IP� data protocol, sets �Internet" APN.
*	Selects the socket 5, sets SRING URC mode to display the connIdand the amount of data received, disables TCP keepalive timer timeout, sets the socket initial connection timeout to 60s, disables the max packet timeout(not used in command mode).
*	Selects the socket 5, enables the indication of connId socket and closure cause when a NO CARRIER indication occurs.
*	Selects the socket 5, the PDP context 2, sets the max packet size for the TCP/IP stack to 1500 (not used in command mode), disables the socket exchange inactivity timeout, sets the socket initial connection timeout to 60s, disables the max packet timeout (not used in command mode).
*/
void OneTime()
{
    char msg[10][50];

    strcpy(msg[0], CGDCONT Context ", \"IP\" ,\" " APN " \", \"\",0,0,0,0\r"); //Selects PDP context identifier 2, selects �IP� data protocol, sets �Internet" APN.
    strcpy(msg[1], SCFGEXT Socket_TCP SCFGEXT_N);                              //Selects the socket 5, sets SRING URC mode to display the connIdand the amount of data received, disables TCP keepalive timer timeout, sets the socket initial connection timeout to 60s, disables the max packet timeout(not used in command mode).
    strcpy(msg[2], SCFGEXT2 Socket_TCP SCFGEXT2_N);                            //Selects the socket 5, enables the indication of connId socket and closure cause when a NO CARRIER indication occurs.
    strcpy(msg[3], SCFG Socket_TCP SCFG_N);                                    //Selects the socket 5, the PDP context 2, sets the max packet size for the TCP/IP stack to 1500 (not used in command mode), disables the socket exchange inactivity timeout, sets the socket initial connection timeout to 60s, disables the max packet timeout (not used in command mode).

    for (int i = 0; i < 4; i++)
    {
        ReadWrite(msg[i]);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
*	Reboot Modem instantly:AT#ENHRST=1,0
*	
*/
void Restart()
{
    char msg[50];
    strcpy(msg, Reboot); //Restart Modem
#if DEBUG == 1
    printf("\nRestarting...\n");
#endif
    ReadWrite(msg);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Enter PSM Mode: AT+CFUN=5
 */
void EnterPSM()
{
    char msg[50];

    strcpy(msg, CFUN5);
    ReadWrite(msg);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Exit PSM Mode: AT+CFUN=1
 */
void ExitPSM()
{
    char msg[50];

    strcpy(msg, CFUN1);
    ReadWrite(msg);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// Menu of options to interact with the user
char menu()
{
    char choice = ' ';
    do
    {
        system("clear");
        printf("flag dtr = %d\n", FLAG_DTR);
        printf("------------------------------------\n");
        printf("1- Initial Setup\n");        //activates error report and ri response time 500ms
        printf("2- Start TCP Connection\n"); //activates context and starts a TCP connection
        printf("3- Send Commands\n");        //user freely sends commands
        printf("4- Enter PSM Mode\n");
        printf(".\n");
        printf("5- Close TCP Connection\n");
        printf("6- Exit PSM Mode\n");
        printf(".\n");
        printf("8- Restart Modem\n");  //turn the modem off or restart
        printf("9- One Time Setup\n"); //defines the settings for the socket 5 and turns the leds off
        printf("s/S- Exit.\n");
        printf("------------------------------------\n\n");
        scanf("%c", &choice);
    } while (choice != 's' && choice != 'S' && choice != '1' && choice != '2' && choice != '3' && choice != '4' && choice != '5' && choice != '6' && choice != '7' && choice != '8' && choice != '9');

    return choice;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main()
{
    system("clear");

    char choice = ' ';

    serial_port = OpenSerial();

    if (serial_port == -1)
    {

#if DEBUG == 1
        printf("Error In Serial Port");
#endif
        return -1;
    }
#if DEBUG == 1
    SystemPause();
#endif

    pid_t pid = fork();

    if (pid == 0)
    {
        while (1)
        {
            ReadData();
        }
    }

    do
    {
        choice = menu();

        switch (choice)
        {
        case '1':
        {
            system("clear");

            InitialSetup();
#if DEBUG == 1
            SystemPause();
            SystemPause();
#endif
            break;
        }

        case '2':
        {
            system("clear");

            OpenSocketServer();
#if DEBUG == 1
            SystemPause();
            SystemPause();
#endif
            break;
        }

        case '3':
        {
            system("clear");
            SendCommand(); //Write commands
#if DEBUG == 1
            SystemPause();
            SystemPause();
#endif
            break;
        }

        case '4':
        {
            system("clear");

            EnterPSM();
#if DEBUG == 1
            SystemPause();
            SystemPause();
#endif
            break;
        }

        case '5':
        {
            system("clear");

            CloseSocketServer();
#if DEBUG == 1
            SystemPause();
            SystemPause();
#endif
            break;
        }

        case '6':
        {
            system("clear");

            ExitPSM();
#if DEBUG == 1
            SystemPause();
            SystemPause();
#endif
            break;
        }

        case '8':
        {
            system("clear");
            Restart(); //Reboot Modem
#if DEBUG == 1
            SystemPause();
            SystemPause();
#endif
            break;
        }
        case '9':
        {
            system("clear");
            OneTime();
            SystemPause();
            break;
        }
        }
    } while (choice != 's' && choice != 'S');

    CloseSocketServer();
    ExitPSM();
    SystemPause();
    close(serial_port);
    kill(pid, SIGTERM);

#if DEBUG == 1
    printf("\nSerial Port Closed.\n");
#endif
    return 0;
}

void SystemPause()
{
    printf("\nSystem Pause, Press Enter to continue.\n");
    int c = getchar();
    c++;
}
