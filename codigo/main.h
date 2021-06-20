/**
  * @file main.h
*/

// C library headers
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Linux headers
#include <fcntl.h>      // Contains file controls like O_RDWR
#include <errno.h>      // Error integer and strerror() function
#include <termios.h>    // Contains POSIX terminal control definitions
#include <unistd.h>     // write(), read(), close()
#include <sys/ioctl.h>  // Control dtr line 
#include <signal.h>

#define DEBUG 1 // 1 -> Show errors and modem response
 


int serial_port;       //Serial Port
int FLAG_DTR;          //FLAG==1, dtr is on and modem is free to comunicate
                       //FLAG==0, Modem is in PSM


int OpenSerial();
void InitialSetup();
void OpenSocketServer();
void CloseSocketServer();
int SendData(char *);
int ReadData();
void SetDTR();
void ClearDTR();
void CheckCFUN(char *);
void ReadWrite(char *);
void SendCommand();
void Onetime();
void Restart();
void EnterPSM();
void ExitPSM();
char menu();
void SystemPause();

#define APN "internet"         ///< ISP APN to use. Altice -> internet / Vodafone ->internet.vodafone.pt
#define IPadd "161.230.159.26" ///< IP address to connect to server
#define PORT "5000"            ///< Server PORT
#define PIN "0000"             ///< SIM card PIN


//Restart
#define Reboot "AT#ENHRST=1,0\r"

//Initial Setup
#define AT "AT\r"
#define CMEE "AT+CMEE=2\r"          /// error report
#define CPIN "AT+CPIN="             /// SIM card pin
#define PSMRI "AT#PSMRI=500\r"      ///  
#define CFUN "AT+CFUN="             /// PSM mode
#define CFUN5 "AT+CFUN=5\r"         /// Turn on PSM
#define CFUN1 "AT+CFUN=1\r"         /// Turn off PSM

//SocketServer
#define SGACT "AT#SGACT="           /// Context management
#define SGACT_CONTEXT "2"
#define SD "AT#SD="                 /// Start TCP Connection
#define SH "AT#SH="                 /// Close TCP connection
#define Socket_TCP "5"              /// Modem socket to open TCP connection

//OneTime                           ///// Socket management 
#define CGDCONT "AT+CGDCONT="
#define Context "2"
#define SCFGEXT "AT#SCFGEXT="
#define SCFGEXT_N ",1,0,0,0,0\r"
#define SCFGEXT2 "AT#SCFGEXT2="
#define SCFGEXT2_N ",0,0,0,0,2\r"
#define SCFG "AT#SCFG="
#define SCFG_N ",2,1500,0,600,0\r"

