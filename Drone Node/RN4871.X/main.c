/**
  Generated main.c file from MPLAB Code Configurator

  @Company
    Microchip Technology Inc.

  @File Name
    main.c

  @Summary
    This is the generated main.c using PIC24 / dsPIC33 / PIC32MM MCUs.

  @Description
    This source file provides main entry point for system intialization and application code development.
    Generation Information :
        Product Revision  :  PIC24 / dsPIC33 / PIC32MM MCUs - 1.75.1
        Device            :  PIC24FJ256GA702
    The generated drivers are tested against the following:
        Compiler          :  XC16 v1.35
        MPLAB 	          :  MPLAB X v5.05
*/

/*
    (c) 2016 Microchip Technology Inc. and its subsidiaries. You may use this
    software and any derivatives exclusively with Microchip products.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
    WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
    PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION
    WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
    BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
    FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
    ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
    THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.

    MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE
    TERMS.
*/

/**
  Section: Included Files
*/

#include "GlobalConstants.h"
#include "BLECommands.h"
#include "ff.h" /* Reference to the Fat Fs */ 


char BT_WriteBuffer1[256];
char BT_ReadBuffer1[256];
const uint8_t WriteFF2[] = {'C','H','W',',','0','0','2','C',',','F','F','\r'};


/* Global SD variables */
FATFS FatFs; // FatFs work area needed for each volume 
FIL Fil; // The structure pointing to the SD file used for reading and writing  

/*
                         Main application
 */
int main(void)
{
    
    
    SYSTEM_Initialize();// initialize the device
    __delay_ms(2000);
    //RN_Initialize();
    __delay_ms(200);
    FRESULT sd_mount_status = f_mount(&FatFs, "", 1);
    while (sd_mount_status !=FR_OK){sd_mount_status = f_mount(&FatFs, "", 1); __delay_ms(10);}
    
    bool filesOnboard = false; //GroundMode(); should be ground mode (testing comment/ true)
    
 while (1){  
          
        while(!filesOnboard)
        {
        filesOnboard = GroundMode();
        }
        
        //EncryptMode();
        
        while(filesOnboard)
        {
        filesOnboard = ServerMode();
        
        }
        
        
 }
    



    
    return 1;
}
/**
 End of File
*/

