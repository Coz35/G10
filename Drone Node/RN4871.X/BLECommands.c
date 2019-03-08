//Created By G10 with the assistance of external online sources



#include "BLECommands.h"
#include "ff.h" 
#include <string.h>


#if defined(__dsPIC33E__)
	#include <p33exxxx.h>
#elif defined(__dsPIC33F__)
	#include <p33fxxxx.h>
#elif defined(__dsPIC30F__)
	#include <p30fxxxx.h>
#elif defined(__PIC24E__)
	#include <p24exxxx.h>
#elif defined(__PIC24H__)
	#include <p24hxxxx.h>
#elif defined(__PIC24F__) || defined(__PIC24FK__)

#endif



#include "AES.h"

FATFS FatFs2; // FatFs work area needed for each volume 
FIL Fil2; // The structure pointing to the SD file used for reading and writing  
char data = false;


//Bluetooth command constants. some are received some are sent
const uint8_t CMD_mode_enter[] = {'$','$','$'}; ///$$$ puts BT in command mode
const uint8_t CMD_mode_response[] = {'C','M','D','>',' '}; ///$$$ puts BT in command mode
const uint8_t CMD_mode_exit[] = {'-','-','-','\r'}; ///--- exits command mode
const uint8_t CMD_exit_response[] = {'E','N','D','\r','\n'}; ///--- exits command mode response
const uint8_t Connection_Ground[] = {'C',',','1',',','D','C','7','C','C','A','9','C','0','3','3','5','\r'};
const uint8_t Connect_ground_response[] = {'T','r','y','i','n','g','\r','\n','%','C','O','N','N','E','C','T',',','1','D','C','7','C','C','A','9','C','0','3','3','5','%'};
const uint8_t CMD_Client_mode[] = {'C','I','\r'};
const uint8_t Notify[] = {'C','H','W',',','0','0','2','A',',','0','1','0','0','\r'};
const uint8_t ListClientServices[] = {'L','C','\r'};
const uint8_t WriteFF[] = {'C','H','W',',','0','0','2','C',',','F','F','\r'};
const uint8_t WriteF9[]= {'C','H','W',',','0','0','2','C',',','F','9','\r'};
const uint8_t WriteEE[]= {'C','H','W',',','0','0','2','C',',','E','E','\r'};
const uint8_t AOK[] = "AOK\r\nCMD>\\";//{'A','O','K','\r','\n','C','M','D','>','\'\''};
const uint8_t reboot[] = {'%','R','E','B','O','O','T','%',};
const uint8_t setbaud[] ={'S','B',',','0','5','\r'};
const uint8_t Transparent[]= {'S','S',',','C','0','\r'};
const uint8_t Reboot1[] = {'R',',','1','\r'};
const uint8_t ServerConnect[] = {'%','C','O','N','N','E','C','T'};
const uint8_t KillConnection[] ={'K',',','1','\r'};
const uint8_t SetCMDpin[] = {'S','W',',','0','D',',','0','C','\r'};
const uint8_t setBaud[] ={'S','B',',','0','5','\r'};
const uint8_t Stat1PinSet[] = {'S','W',',','0','B',',','0','7','\r'};
const uint8_t Stat2PinSet[] = {'S','W',',','0','A',',','0','8','\r'};
const uint8_t FF[] ={0xFF};


//Buffers. All sized 256 which is max for UART on the PIC
uint8_t BT_WriteBuffer[256];
uint8_t BT_ReadBuffer[256];
uint8_t sd_read_buffer[256];

//received by pins RB2 and RB3 respectively
//used to determine if BLE module is connected, transparent UART mode/idle or off
uint8_t status1; 
uint8_t status2 ;

//Used constantly to put the bluetooth module into command mode where it accepts changes to it's configuration
// command mode is negative edge triggered
bool EnterCommandMode()
{  
    
    
    mode_SetHigh();
    __delay_ms(500);
    mode_SetLow();
    __delay_ms(500);
    
    return true;
}

//used constantly to exit command mode on the BLE module. 
//// command mode is Positive edge triggered
bool ExitCommandMode()
{
    mode_SetLow();
    __delay_ms(500);
    mode_SetHigh();
    __delay_ms(500);
    
    return true;
}

//used to connect to ground node. Called in BLE Run 
bool ConnectToGroundNode()
{
    status1 = stat1_GetValue(); //read from BLE module
    status2 = stat2_GetValue(); //read from BLE module
    __delay_ms(200); 
    
    UART1_WriteBuffer(Connection_Ground, sizeof(Connection_Ground)); 
    __delay_ms(3000);
    status1 = stat1_GetValue();
    status2 = stat2_GetValue();
    __delay_ms(200);
    
    // when status2 is high we're connected
    while(status2)
    {
        UART1_WriteBuffer(Connection_Ground, sizeof(Connection_Ground));
        __delay_ms(3000);
        
        status2 = stat2_GetValue();
        __delay_ms(200);
    }
    
    //clear the buffer
    UART1_ReadBuffer(BT_ReadBuffer, sizeof(BT_ReadBuffer));
  
    return true;
}

//empty the software buffer
void ClearReadBuffer()
{
    uint16_t i;
    for(i=0;i<sizeof(BT_ReadBuffer);i++)
    {
        BT_ReadBuffer[i] = NULL;
        
    }
    
}

//clears the storage buffer (not the actual UART buffer)
void ClearWriteBuffer()
{
    uint8_t i;
    for(i=0;i<sizeof(BT_WriteBuffer);i++)
    {
        BT_WriteBuffer[i] = NULL;
        
    }
    
    
}


//Puts BLE module into client mode. Required for Ground Node connection
bool EnterClientMode()
{

    //Clear read buffer
    UART1_ReadBuffer(BT_ReadBuffer, sizeof(BT_ReadBuffer));
    //send enter command mode command
    UART1_WriteBuffer(CMD_Client_mode, sizeof(CMD_Client_mode));
    __delay_ms(50);
   
    //read response
    UART1_ReadBuffer(BT_ReadBuffer, sizeof(BT_ReadBuffer));
    
    //check that the response contains AOK
    if (strstr((char*)BT_ReadBuffer,"AOK") !=NULL)
    {
        
        __delay_ms(1000); //Ensure client mode is set. Very important that we are in client mode.
        return true;
    }
    
    return false; //returns false if we didn't get the AOK response
       
}

//Tells ground we wish to notify on the custom defined transmit characteristic
bool WriteNotify()
{   
    //clear read buffer
    UART1_ReadBuffer(BT_ReadBuffer, sizeof(BT_ReadBuffer));
    
    //write notify command to BLE module
    UART1_WriteBuffer(Notify, sizeof(Notify));
    __delay_ms(50);
    
    //read BLE UART buffer
    UART1_ReadBuffer(BT_ReadBuffer, sizeof(BT_ReadBuffer));
    
    //check for AOK response 
    if (strstr((char*)BT_ReadBuffer,"AOK") !=NULL)
    {
        __delay_ms(1000);
        return true;
    }
    
    return false; //return false if we didn't set notify correctly
    
    
}


//Main function run during 
bool BLErun(){

    
    uint8_t count = 0;
    bool done = true;

    //the counts in the following code prevent too many attemps incase a step fails.
    //ultimately this will force the code to restart
    while(!EnterCommandMode() && count <5)
    {
        
        count++;
        if(count == 5)
        {
            done = false;
            
        }

    }
    
    if(count <5){
        
        count =0;
    
    }
    
    //Added for demonstration. This guarantees the server node is not connected and allows the ground node the chance
    UART1_WriteBuffer(KillConnection, sizeof(KillConnection));
    __delay_ms(100);
    
    
    while(!ConnectToGroundNode()&& count <5)
    {
        __delay_ms(3000);
        
        count++;
        if(count == 5)
        {
            done = false;
            
        }
       

    }

    if(count <5){
        
        count =0;
    
    }
    while(!EnterClientMode()&& count <5)
    {
        count++;
        if(count == 5)
        {
            done = false;
            
        }

    }
    
    __delay_ms(1000);
    
    if(count <5)
    {
        
        count = 0;
    
    }
    
    while(!WriteNotify() && count < 5)
    {
        count++;
        if(count == 5)
        {
            done = false;
            
        }
        
    }   
    
    __delay_ms(2000);
    if(count < 5)
    {
        
        count = 0;
    
    }
    
    while(!StartGroundNodeDataTransmission() && count < 5)
    {
      
    }
    
    
    if(!done)
    {
        ExitCommandMode();
    }
     
    
    
    
    return done;
}

//Write 0xFF to ground node Rx characteristic
//Used in custom protocol for interfacing with Ground node
bool StartGroundNodeDataTransmission()
{
    
    UART1_ReadBuffer(BT_ReadBuffer, sizeof(BT_ReadBuffer));
    ClearReadBuffer();
    
    UART1_WriteBuffer(WriteFF, sizeof(WriteFF));
    
    
    return true;
}


//Required for communication with server node
bool EnterTransparent()
{
    
    
    EnterCommandMode();
        
    
    UART1_ReadBuffer(BT_ReadBuffer, sizeof(BT_ReadBuffer));
    ClearReadBuffer();
    __delay_ms(500);
    UART1_WriteBuffer(Transparent, sizeof(Transparent));
    __delay_ms(500);
    

     UART1_WriteBuffer(Reboot1, sizeof(Reboot1));
     __delay_ms(4000);
     
    
     return true;             
    }

//wait for server connection
bool WaitForServer()
{  
        
   
   //Use BLE status pins to determine if connection has been formed 
    status1 = stat1_GetValue();
    status2 = stat2_GetValue();
    __delay_ms(50);
        
    while(status2)
    {
    status1 = stat1_GetValue();
    status2 = stat2_GetValue();
    __delay_ms(50);
    }
          
    return true;
    }


//Used to send SD card data Over UART to BLE module and to Server Node
bool SendData1()
{
    __delay_ms(750);
    FRESULT open_file_status; //Status of opening the file for IO
    open_file_status = f_open(&Fil2, "GNDS1.txt", FA_OPEN_ALWAYS | FA_WRITE | FA_READ); //Note the file pointer is zeroed on a file open
    
    FRESULT sd_seek_status;
    sd_seek_status = f_lseek(&Fil2, 0); //Move file pointer to the start of the file (which is zero)
    
    /* Variables associated with an SD read */
    FRESULT sd_read_status; // Status of the read operation
     // The buffer holding information read from the SD card (filled by reference via f_read method) - Can be any size 100 is arbritrary
   
    UINT sd_num_bytes_read; // Number of bytes read (should match the above, with the exception of when the EOF has been reached)
    
    uint16_t i;
    for(i=0;i<sizeof(sd_read_buffer);i++)
    {
        sd_read_buffer[i] = ' ';
        
    }
    
    //Pad the start of the file in case the first buffer is lost in time delays
    uint8_t aoks[] = "AOK\nAOK\nAOK\nAOK\nAOK\nAOK\nAOK\nAOK\nAOK\nAOK\nAOK\nAOK\nAOK\nAOK\nAOK\nAOK\nAOK\nAOK\nAOK\nAOK\nAOK\nAOK\nAOK\nAOK\n";
    UART1_WriteBuffer(aoks, sizeof(aoks));
    __delay_ms(100);
    
        while (f_eof(&Fil2) == 0) //The f_eof(&Fil) method checks whether the read pointer is at the end of the file
        {            
            
            sd_read_status = f_read(&Fil2, sd_read_buffer, sizeof(sd_read_buffer), &sd_num_bytes_read);   
            __delay_ms(50);
            UART1_WriteBuffer(sd_read_buffer, sizeof(sd_read_buffer));
            __delay_ms(50);
            for(i=0;i<sizeof(sd_read_buffer);i++)
            {
                sd_read_buffer[i] = ' ';

            }
        }
   
   while(!EnterCommandMode())
    {
       
    }
            
    __delay_ms(50);
    UART1_WriteBuffer(KillConnection, sizeof(KillConnection));
    __delay_ms(200);
    ExitCommandMode();
    __delay_ms(50);
    UART1_ReadBuffer(BT_ReadBuffer, sizeof(BT_ReadBuffer));
    ClearReadBuffer();
   
     return true;             
    }

//Main function for entering server mode. Called form main.c
bool ServerMode()
{
         
        EnterTransparent();
        WaitForServer();       
        SendData1();
        return false;
}


//Main function for entering Ground Mode. Called from Main.c
bool GroundMode()
{
    FRESULT open_file_status; //Status of opening the file for IO
    open_file_status = f_open(&Fil2, "gnds1.txt", FA_OPEN_ALWAYS | FA_WRITE | FA_READ);
    FRESULT sd_seek_status;
    sd_seek_status = f_lseek(&Fil2, 0); // puts file cursor to beginning of file
    bool quit = false; //ensures to stay in ground mode until data transfer is complete
    
    FRESULT sd_write_status;
    UINT sd_num_bytes_written; // Number of bytes written to the SD card (should match sd_num_bytes_to_write  
   
    __delay_ms(50);                  
    while(!BLErun()) // connection protocol to connect to ground node BLE MAC address, and start notification on custom service
    {
    }
    
    
    int counter = 0;
    while(!quit)
    {

        __delay_ms(500); // need more than 300ms to ensure no overlap between data sent from ground node and command response

        UART1_SDReadBuffer((char*)BT_ReadBuffer, sizeof(BT_ReadBuffer)); // reads 80 bytes of the UART buffer (ground only sends 80)
        if (strstr((char*)BT_ReadBuffer,"FFFFFF") !=NULL) // looks for stop code sent from ground node
            {
            __delay_ms(1000);
            quit = true; // exits ground mode
            }
        sd_write_status = f_write(&Fil2, BT_ReadBuffer, sizeof(BT_ReadBuffer), &sd_num_bytes_written); // write 80 bytes to the open file in the SD card

        UART1_SDReadBuffer((char*)BT_ReadBuffer, sizeof(BT_ReadBuffer)); // clears SD buffer
        UART1_WriteBuffer(WriteEE, sizeof(WriteEE));//Sends "EE" handshaking back to ground node to request further data

        counter++;
    }
    
    FRESULT sd_close_status;
    sd_close_status = f_close(&Fil2); //closes file on SD card
    
    
    UART1_WriteBuffer(KillConnection, sizeof(KillConnection)); // kills connection with ground node
       
    __delay_ms(100);
    
    ExitCommandMode(); // exits command mode
    __delay_ms(50);
    UART1_ReadBuffer(BT_ReadBuffer, sizeof(BT_ReadBuffer)); // clears UART buffer (safety clear)
    
    
    return true; // allows main protocol to determine that the files have been collected
    
}
