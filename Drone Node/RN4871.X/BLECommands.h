/* 
 * File:   BLECommands.h
 * Author: Corey
 *
 * Created on January 26, 2019, 6:29 PM
 */

#ifndef BLECOMMANDS_H
#define	BLECOMMANDS_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "GlobalConstants.h"
    

void EncryptMode();
bool EnterCommandMode(); //Sends '$$$' to enter command mode
bool ExitCommandMode();
bool ConnectToGroundNode();
bool EnterClientMode();
bool StartGroundNodeDataTransmission(); //Write 0xFF to ground node Rx characteristic
bool BLErun();
void CheckReboot();
void ClearReadBuffer(); // Fills BT_ReadBuffer with NULL
bool ServerMode();
bool GroundMode();

#ifdef	__cplusplus
}
#endif

#endif	/* BLECOMMANDS_H */

