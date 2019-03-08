/*
 Copyright (C) 2018 Apple Inc. All Rights Reserved.
 See LICENSE.txt for this sample’s licensing information
 
 Abstract:
 Implementatin of Health Thermometer Client app using Bluetooth Low Energy (LE) Health Thermometer Service. This app demonstrats the use of CoreBluetooth APIs for LE devices.
 */

#import "HealthThermometerClientAppDelegate.h"


@implementation HealthThermometerClientAppDelegate

@synthesize window;
@synthesize scanSheet;
@synthesize deviceName;
@synthesize manufactureName;
@synthesize tempType;
@synthesize tempString;
@synthesize timeStampString;
@synthesize connectStatus;
@synthesize mesurementType;
@synthesize temperatureMeasurementChar;
@synthesize intermediateTempChar;
@synthesize thermometers;
@synthesize arrayController;


- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{    
    self.thermometers = [NSMutableArray array];
    autoConnect = FALSE;   /* uncomment this line if you want to automatically connect to previosly known peripheral */
    
    
    manager = [[CBCentralManager alloc] initWithDelegate:self queue:nil];
    
}

- (void) dealloc
{
    [self stopScan];
    
    [testPeripheral setDelegate:nil];
    [testPeripheral release];
    
    [thermometers release];
        
    [manager release];
    [temperatureMeasurementChar release];
    [intermediateTempChar release];
    
    [super dealloc];
}

/* 
 Disconnect peripheral when application terminate 
 */
- (void) applicationWillTerminate:(NSNotification *)notification
{
    if(testPeripheral)
    {
        [manager cancelPeripheralConnection:testPeripheral];
        [self.file closeFile];
    }
}

#pragma mark - Scan sheet methods
/* 
 Open scan sheet to discover  peripherals if it is LE capable hardware
 */
- (IBAction)openScanSheet:(id)sender 
{
    if( [self isLECapableHardware] )
    {
        autoConnect = FALSE;
        [arrayController removeObjects:thermometers];
        [window beginSheet:self.scanSheet completionHandler:^(NSModalResponse returnCode) {
            [self sheetDidEnd:self.scanSheet returnCode:returnCode contextInfo:nil];
        } ];
        [self startScan];
    }
}

/*
 Close scan sheet once device is selected
 */
- (IBAction)closeScanSheet:(id)sender 
{
    [window endSheet:self.scanSheet returnCode:NSAlertFirstButtonReturn];
}

/*
 Close scan sheet without choosing any device
 */
- (IBAction)cancelScanSheet:(id)sender
{
    [window endSheet:self.scanSheet returnCode:NSAlertSecondButtonReturn];
}

/* 
 This method is called when Scan sheet is closed. Initiate connection to selected thermometer peripheral
 */
- (void)sheetDidEnd:(NSWindow *)sheet returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo 
{    
    [self stopScan];
    if(returnCode == NSAlertFirstButtonReturn)
    {            
        NSIndexSet *indexes = [self.arrayController selectionIndexes];
        if ([indexes count] != 0) 
        {
            NSUInteger anIndex = [indexes firstIndex];
            testPeripheral = [self.thermometers objectAtIndex:anIndex];
            [testPeripheral retain];
            [progressIndicator setHidden:FALSE];
            [progressIndicator startAnimation:self];
            [connectButton setTitle:@"Cancel"];
            [manager connectPeripheral:testPeripheral options:nil];
        }
    }
}

#pragma mark - Connect Button
/*
 This method is called when connect button pressed and it takes appropriate actions depending on device connection state
 */
- (IBAction)connectButtonPressed:(id)sender
{
    if(testPeripheral && (testPeripheral.state == CBPeripheralStateConnected))
    {
        /* Disconnect peripheral if its already connected */
        [manager cancelPeripheralConnection:testPeripheral];
    }
    else if (testPeripheral)
    {
        /* Device is not connected, cancel pending connection */
        [progressIndicator setHidden:TRUE];
        [progressIndicator stopAnimation:self];
        [connectButton setTitle:@"Connect"];
        [manager cancelPeripheralConnection:testPeripheral];
        [self openScanSheet:nil];
    }
    else{
        
        [self startScan];
    }
        
        
    
}

#pragma mark - Start/Stop Scan methods
/*
 Request CBCentralManager to scan for health thermometer peripherals using service UUID 0x1809
 */
- (void)startScan 
{    
    //NSDictionary * options = [NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithBool:FALSE], nil];
    
    [manager scanForPeripheralsWithServices:nil options:nil];
}

/*
 Request CBCentralManager to stop scanning for health thermometer peripherals
 */
- (void)stopScan
{
    [manager stopScan];
}


#pragma mark - Start/Stop notification/indication
/*
 Start or stop receiving notification or indication on interested characteristics
 */
- (IBAction)startButtonPressed:(id)sender
{
    BOOL notify;
    
    if([[startStopButton title] isEqualToString:@"Start"])
    {
        notify = TRUE;
    }
    else
    {
        notify = FALSE;
    }

    if(self.intermediateTempChar)
    {
        /* Set notification on intermediate temperature measurement characteristic */
        NSLog(@"inter");
        [testPeripheral setNotifyValue:notify forCharacteristic:self.intermediateTempChar];
    }
    else if( self.temperatureMeasurementChar)
    {
        /* Set indication on temperature measurement characteristic */
        NSLog(@"measure");
        [testPeripheral setNotifyValue:notify forCharacteristic:self.temperatureMeasurementChar];
    }
    else if( self.temperatureMeasurementChar)
    {
        /* Set indication on temperature measurement characteristic */
        NSLog(@"temp");
        [testPeripheral setNotifyValue:notify forCharacteristic:self.temperatureMeasurementChar];
    }
    else if(self.transparentServiceChar)
    {
        NSLog(@"transparent");
        [testPeripheral setNotifyValue:notify forCharacteristic:self.transparentServiceChar];
        
    }
    
}

#pragma mark - LE Capable Platform/Hardware check
/*
 Uses CBCentralManager to check whether the current platform/hardware supports Bluetooth LE. An alert is raised if Bluetooth LE is not enabled or is not supported.
 */
- (BOOL) isLECapableHardware
{
    NSString * state = nil;
    
    switch ([manager state]) 
    {
        case CBManagerStateUnsupported:
            state = @"The platform/hardware doesn't support Bluetooth Low Energy.";
            break;
        case CBManagerStateUnauthorized:
            state = @"The app is not authorized to use Bluetooth Low Energy.";
            break;
        case CBManagerStatePoweredOff:
            state = @"Bluetooth is currently powered off.";
            break;
        case CBManagerStatePoweredOn:
            return TRUE;
        case CBManagerStateUnknown:
        default:
            return FALSE;
            
    }
    
    NSLog(@"Central manager state: %@", state);
    
    [self cancelScanSheet:nil];
    
    NSAlert *alert = [[[NSAlert alloc] init] autorelease];
    [alert setMessageText:state];
    [alert addButtonWithTitle:@"OK"];
    [alert setIcon:[[[NSImage alloc] initWithContentsOfFile:@"Thermometer"] autorelease]];
    [alert beginSheetModalForWindow:[self window] completionHandler:^(NSModalResponse returnCode) {
        return;
    }];
    return FALSE;
}

#pragma mark - CBManagerDelegate methods
/*
 Invoked whenever the central manager's state is updated.
 */
- (void)centralManagerDidUpdateState:(CBCentralManager *)central 
{
    [self isLECapableHardware];
}

/*
 Invoked when the central discovers thermometer peripheral while scanning.
 */
- (void)centralManager:(CBCentralManager *)central didDiscoverPeripheral:(CBPeripheral *)peripheral advertisementData:(NSDictionary *)advertisementData RSSI:(NSNumber *)RSSI
{
    
    NSLog(@"Did discover peripheral. peripheral: %@ rssi: %@, UUID: %@ advertisementData: %@ ", peripheral, RSSI, peripheral.identifier, advertisementData);
   
    NSMutableArray *peripherals = [self mutableArrayValueForKey:@"thermometers"];
    if( ![self.thermometers containsObject:peripheral] )
        [peripherals addObject:peripheral];
     
    /* Retreive already known devices */
    
    if([peripheral.name isEqual:@"DroneNode_CA48"]||[peripheral.name isEqual:@"DroneNode_0000"]||[peripheral.name isEqual:@"DroneNode_8FEC"]||[peripheral.name isEqual:@"DroneNode_39FD"]||[peripheral.name isEqual:@"DroneNode"]){
        
        [self stopScan];
        
      
        [progressIndicator setHidden:FALSE];
        [progressIndicator startAnimation:self];
        testPeripheral = peripheral;
        [testPeripheral retain];
        [connectButton setTitle:@"Cancel"];
        [manager connectPeripheral:testPeripheral options:[NSDictionary dictionaryWithObject:[NSNumber numberWithBool:YES] forKey:CBConnectPeripheralOptionNotifyOnDisconnectionKey]];
        //[manager retrievePeripheralsWithIdentifiers:[NSArray arrayWithObject:(id)peripheral.identifier]];
    }
    
}

/*
 Invoked when the central manager retrieves the list of known peripherals.
 Automatically connect to first known peripheral
 */
- (void)centralManager:(CBCentralManager *)central didRetrievePeripherals:(NSArray *)peripherals
{
    NSLog(@"Retrieved peripheral: %lu - %@", [peripherals count], peripherals);
    
    [self stopScan];
    
    /* If there are any known devices, automatically connect to it.*/
    if([peripherals count] >=1)
    {
        [progressIndicator setHidden:FALSE];
        [progressIndicator startAnimation:self];
        testPeripheral = [peripherals objectAtIndex:0];
        [testPeripheral retain];
        [connectButton setTitle:@"Cancel"];
        [manager connectPeripheral:testPeripheral options:[NSDictionary dictionaryWithObject:[NSNumber numberWithBool:YES] forKey:CBConnectPeripheralOptionNotifyOnDisconnectionKey]];
    }
}

/*
 Invoked whenever a connection is succesfully created with the peripheral. 
 Discover available services on the peripheral
 */
- (void)centralManager:(CBCentralManager *)central didConnectPeripheral:(CBPeripheral *)peripheral
{
    NSLog(@"Did connect to peripheral: %@", peripheral);
        
    self.connectStatus = @"Connected";
    [connectButton setTitle:@"Disconnect"];
    [progressIndicator setHidden:TRUE];
    [progressIndicator stopAnimation:self];
    [peripheral setDelegate:self];
    [peripheral discoverServices:nil];
    
}

/*
 Invoked whenever an existing connection with the peripheral is torn down. 
 Reset local variables
 */
- (void)centralManager:(CBCentralManager *)central didDisconnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error
{
    NSLog(@"Did Disconnect to peripheral: %@ with error = %@", peripheral, [error localizedDescription]);
    self.connectStatus = @"Not Connected";
    self.deviceName = @"";
    self.timeStampString = @"";
    self.tempType = @"";
    self.tempString = @"";
    self.mesurementType = @"";
    self.manufactureName = @"";
    [connectButton setTitle:@"Connect"];
    [startStopButton setTitle:@"Start"];
    if( testPeripheral )
    {
        [testPeripheral setDelegate:nil];
        [testPeripheral release];
        testPeripheral = nil;
    }
    
    [self.file closeFile];
   
    
    @try{
        NSTask *t = [[[NSTask alloc] init] autorelease];
        [t setLaunchPath:@"/usr/bin/scp"];
        NSString * newfilename = [[@"/Users/Corey/Documents/NodeData/NodeData" stringByAppendingString:self.dateString] stringByAppendingString:@".txt"];
        NSLog(@"New File Path: %@", newfilename);
        [t setArguments:[NSArray arrayWithObjects:@"-i", @"/Users/Corey/Documents/NodeData/ws-keypair.pem",newfilename, @"ec2-user@35.182.28.73:/var/NodeData/", nil]];
        
        // Set the pipe to the standard output and error to get the results of the command
        NSPipe *p = [[[NSPipe alloc] init] autorelease];
        [t setStandardOutput:p];
        [t setStandardError:p];
        
        // Launch (forks) the process
        [t launch]; // raises an exception if something went wrong
        
        // Prepare to read
        NSFileHandle *readHandle = [p fileHandleForReading];
        NSData *inData = nil;
        NSMutableData *totalData = [[[NSMutableData alloc] init] autorelease];
        
        while ((inData = [readHandle availableData]) &&
               [inData length]) {
            [totalData appendData:inData];
        }
        
        // Polls the runloop until its finished
        [t waitUntilExit];
        
        NSLog(@"Terminationstatus: %d", [t terminationStatus]);
        NSLog(@"Data recovered: %@", totalData);
  
    
    
        [t launch];
    }
    @catch (NSException *e)
    {
        NSLog(@"Expection occurred %@", [e reason]);
        
    }
   
  
   
    
    
    
    
    double delayInSeconds = 10.0;
    [NSThread sleepForTimeInterval: delayInSeconds];
    NSLog(@"Delay");
    
    [self startScan];
}

/*
 Invoked whenever the central manager fails to create a connection with the peripheral.
 */
- (void)centralManager:(CBCentralManager *)central didFailToConnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error
{
    NSLog(@"Fail to connect to peripheral: %@ with error = %@", peripheral, [error localizedDescription]);
    [connectButton setTitle:@"Connect"];
    if( testPeripheral )
    {
        [testPeripheral setDelegate:nil];
        [testPeripheral release];
        testPeripheral = nil;
    }
}

#pragma mark - CBPeripheralDelegate methods
/*
 Invoked upon completion of a -[discoverServices:] request.
 */
- (void)peripheral:(CBPeripheral *)peripheral didDiscoverServices:(NSError *)error
{
    if (error) 
    {
        NSLog(@"Discovered services for %@ with error: %@", peripheral.name, [error localizedDescription]);
        return;
    }
    for (CBService * service in peripheral.services)
    {
        NSLog(@"Service found with UUID: %@", service.UUID);
        
        if([service.UUID isEqual:[CBUUID UUIDWithString:@"1809"]])
        {
            /* Thermometer Service - discover termperature measurement, intermediate temperature measturement and measurement interval characteristics */
            [testPeripheral discoverCharacteristics:[NSArray arrayWithObjects:[CBUUID UUIDWithString:@"2A1E"], [CBUUID UUIDWithString:@"2A1C"], [CBUUID UUIDWithString:@"2A21"], nil] forService:service];
        }
        else if([service.UUID isEqual:[CBUUID UUIDWithString:@"180A"]])
        {
            /* Device Information Service - discover manufacture name characteristic */
            [testPeripheral discoverCharacteristics:[NSArray arrayWithObject:[CBUUID UUIDWithString:@"2A29"]] forService:service];
        }
        else if ( [service.UUID isEqual:[CBUUID UUIDWithString:@"1800"]] )
        {
            /* GAP (Generic Access Profile) - discover device name characteristic */
            [testPeripheral discoverCharacteristics:[NSArray arrayWithObject:[CBUUID UUIDWithString:@"2A00"]]  forService:service];
        }
        else if ( [service.UUID isEqual:[CBUUID UUIDWithString:@"49535343-FE7D-4AE5-8FA9-9FAFD205E455"]] )
        {
            
            /* GAP (Generic Access Profile) - discover device name characteristic */
            [testPeripheral discoverCharacteristics:[NSArray arrayWithObject:[CBUUID UUIDWithString:@"49535343-1E4D-4BD9-BA61-23C647249616"]]  forService:service];
        }
    }
}

/*
 Invoked upon completion of a -[discoverCharacteristics:forService:] request.
 */
- (void)peripheral:(CBPeripheral *)peripheral didDiscoverCharacteristicsForService:(CBService *)service error:(NSError *)error 
{
    if (error) 
    {
        NSLog(@"Discovered characteristics for %@ with error: %@", service.UUID, [error localizedDescription]);
        return;
    }
    
    
    if([service.UUID isEqual:[CBUUID UUIDWithString:@"1809"]])
    {
        for (CBCharacteristic * characteristic in service.characteristics)
        {
            /* Set indication on temperature measurement */
            if([characteristic.UUID isEqual:[CBUUID UUIDWithString:@"2A1C"]])
            {
                self.temperatureMeasurementChar = characteristic;
                NSLog(@"Found a Temperature Measurement Characteristic");
            } 
            /* Set notification on intermediate temperature measurement */
            if([characteristic.UUID isEqual:[CBUUID UUIDWithString:@"2A1E"]])
            {
                self.intermediateTempChar = characteristic;
                NSLog(@"Found a Intermediate Temperature Measurement Characteristic");
            }            
            /* Write value to measurement interval characteristic */
            if( [characteristic.UUID isEqual:[CBUUID UUIDWithString:@"2A21"]])
            {                
                uint16_t val = 2;
                NSData * valData = [NSData dataWithBytes:(void*)&val length:sizeof(val)];
                [testPeripheral writeValue:valData forCharacteristic:characteristic type:CBCharacteristicWriteWithResponse];  
                NSLog(@"Found a Temperature Measurement Interval Characteristic - Write interval value");
            }
        }
    }
    
    
    
    if([service.UUID isEqual:[CBUUID UUIDWithString:@"49535343-FE7D-4AE5-8FA9-9FAFD205E455"]])
    {
        for (CBCharacteristic * characteristic in service.characteristics)
        {
            
            if([characteristic.UUID isEqual:[CBUUID UUIDWithString:@"49535343-1E4D-4BD9-BA61-23C647249616"]])
            {
                self.transparentServiceChar = characteristic;
                NSLog(@"Found a Transparent UART Service Characteristic");
                NSDate *currentDate = [NSDate date];
                 NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
                 [dateFormatter setDateFormat:@"yyyyMMddHHmmss"];
                self.dateString = [dateFormatter stringFromDate:currentDate];
                self.filepath = [@"/Users/Corey/Documents/NodeData/NodeData" stringByAppendingString: self.dateString];
                self.filepath = [self.filepath stringByAppendingString:@".txt"];
                self.file = [NSFileHandle fileHandleForUpdatingAtPath: self.filepath];
                NSString *content = @"";
                NSData *fileContents = [content dataUsingEncoding:NSUTF8StringEncoding];
                //set writing path to file
                if (self.file == nil){ //check file present or not in file
                    [[NSFileManager defaultManager] createFileAtPath:self.filepath contents:fileContents attributes:nil];
                    self.file = [NSFileHandle fileHandleForUpdatingAtPath: self.filepath];
                    NSLog(@"File Created");
                    NSLog(@"%@",self.filepath);
                }
                else{
                    
                    NSLog(@"File Found");
                    
                }
                
                
                [testPeripheral setNotifyValue:true forCharacteristic:self.transparentServiceChar];
                NSLog(@"Set Notification for transparent Service true");
                /*
                NSString * val = @"ready";
                NSData * valData = [NSData dataWithBytes:(void*)&val length:sizeof(val)];
                [testPeripheral writeValue:valData forCharacteristic:characteristic type:CBCharacteristicWriteWithResponse];
                 */
                
                
            }
        }
    }
    if([service.UUID isEqual:[CBUUID UUIDWithString:@"180A"]])
    {
        for (CBCharacteristic * characteristic in service.characteristics)
        {
            /* Read manufacturer name */
            if([characteristic.UUID isEqual:[CBUUID UUIDWithString:@"2A29"]])
            {                
                [testPeripheral readValueForCharacteristic:characteristic];
                NSLog(@"Found a Device Manufacturer Name Characteristic - Read manufacturer name");
            }           
        } 
    }
    
    if ( [service.UUID isEqual:[CBUUID UUIDWithString:@"1800"]] )
    {
        for (CBCharacteristic *characteristic in service.characteristics) 
        {
            /* Read device name */
            if([characteristic.UUID isEqual:[CBUUID UUIDWithString:@"2A00"]])
            {                
                [testPeripheral readValueForCharacteristic:characteristic];
                NSLog(@"Found a Device Name Characteristic - Read device name");
            }
        }
    }
}

/*
 Invoked upon completion of a -[readValueForCharacteristic:] request or on the reception of a notification/indication.
 */
- (void)peripheral:(CBPeripheral *)peripheral didUpdateValueForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error
{
    if (error) 
    {
        NSLog(@"Error updating value for characteristic %@ error: %@", characteristic.UUID, [error localizedDescription]);
        return;
    }
        
    /* Updated value for temperature measurement received */
    if(([characteristic.UUID isEqual:[CBUUID UUIDWithString:@"49535343-FE7D-4AE5-8FA9-9FAFD205E455"]] || [characteristic.UUID isEqual:[CBUUID UUIDWithString:@"49535343-1E4D-4BD9-BA61-23C647249616"]]) && characteristic.value)
    {
        
        
        
        //[testPeripheral writeValue:characteristic.value forCharacteristic:characteristic type:CBCharacteristicWriteWithResponse];
        
        NSData * updatedValue = characteristic.value;
        //NSLog(@"%@", updatedValue);
        if(updatedValue.length == 0)
            return;
        //uint8_t* datap[] = [updatedValue bytes];
        
        NSString* myString;
        myString = [[NSString alloc] initWithData:updatedValue encoding:NSASCIIStringEncoding];
        
        
        
        char* dataPointer = (char*)[updatedValue bytes];
        NSMutableData *writingdatatofile;
       
        const char *c = [myString UTF8String];
        NSLog(@"%s",c);
        //const char *writingchar = dataPointer;
        writingdatatofile = [NSMutableData dataWithBytes:c length:strlen(c)];
        //convert string format into ns mutable data format
        
        
        //[file seekToFileOffset: 6];
        //object pointer initialy points the offset as 6 position in file
        [self.file writeData: writingdatatofile];
        
        //writing data to new file
        
        
        
        uint8_t flags = dataPointer[0]; dataPointer++;
        int32_t tempData = (int32_t)CFSwapInt32LittleToHost(*(uint32_t*)dataPointer); dataPointer += 4;
        int8_t exponent = (int8_t)(tempData >> 24);
        int32_t mantissa = (int32_t)(tempData & 0x00FFFFFF);
        
        if( tempData == 0x007FFFFF )
        {
            NSLog(@"Invalid temperature value received");
            return;
        }
            
        float tempValue = (float)(mantissa*pow(10, exponent));
        self.tempString = [NSString stringWithFormat:@"%.1f", tempValue];
                
        /* measurement type */
        if(flags & 0x01)
        {
            self.mesurementType = @"ºF";
        }
        else
        {
            self.mesurementType = @"ºC";
        }
        
        /* timestamp */
        if( flags & 0x02 )
        {
            uint16_t year = CFSwapInt16LittleToHost(*(uint16_t*)dataPointer); dataPointer += 2;
            uint8_t month = *(uint8_t*)dataPointer; dataPointer++;
            uint8_t day = *(uint8_t*)dataPointer; dataPointer++;
            uint8_t hour = *(uint8_t*)dataPointer; dataPointer++;
            uint8_t min = *(uint8_t*)dataPointer; dataPointer++;
            uint8_t sec = *(uint8_t*)dataPointer; dataPointer++;
            
            NSString * dateString = [NSString stringWithFormat:@"%d %d %d %d %d %d", year, month, day, hour, min, sec];
            
            NSDateFormatter *dateFormat = [[NSDateFormatter alloc] init];
            [dateFormat setDateFormat: @"yyyy MM dd HH mm ss"];
            NSDate* date = [dateFormat dateFromString:dateString];
            
            [dateFormat setDateFormat:@"EEE MMM dd, yyyy"];
            NSString* dateFormattedString = [dateFormat stringFromDate:date];
            
            [dateFormat setDateFormat:@"h:mm a"];
            NSString* timeFormattedString = [dateFormat stringFromDate:date];
            
            [dateFormat release];
            
            if( dateFormattedString && timeFormattedString )
            {
                self.timeStampString = [NSString stringWithFormat:@"%@ at %@", dateFormattedString, timeFormattedString];
            }
        }
        
        /* temperature type */
        if( flags & 0x04 )
        {
            uint8_t type = *(uint8_t*)dataPointer;
            NSString* location = nil;
            
            switch (type)
            {
                case 0x01:
                    location = @"Armpit";
                    break;
                case 0x02:
                    location = @"Body - general";
                    break;
                case 0x03:
                    location = @"Ear";
                    break;
                case 0x04:
                    location = @"Finger";
                    break;
                case 0x05:
                    location = @"Gastro-intenstinal Tract";
                    break;
                case 0x06:
                    location = @"Mouth";
                    break;
                case 0x07:
                    location = @"Rectum";
                    break;
                case 0x08:
                    location = @"Toe";
                    break;
                case 0x09:
                    location = @"Tympanum - ear drum";
                    break;
                default:
                    break;
            }
            if (location) 
            {
                self.tempType = [NSString stringWithFormat:@"Body location: %@", location];
            }
        }
    }
    
    /* Value for device name received */
    if([characteristic.UUID isEqual:[CBUUID UUIDWithString:@"2A00"]])
    {
        self.deviceName = [[[NSString alloc] initWithData:characteristic.value encoding:NSUTF8StringEncoding] autorelease];
        NSLog(@"Device Name = %@", self.deviceName);
    }
    
    /* Value for manufacturer name received */
    if([characteristic.UUID isEqual:[CBUUID UUIDWithString:@"2A29"]]) 
    {
        self.manufactureName = [[[NSString alloc] initWithData:characteristic.value encoding:NSUTF8StringEncoding] autorelease];
        NSLog(@"Manufacturer Name = %@", self.manufactureName);
    }
}

/*
 Invoked upon completion of a -[writeValue:forCharacteristic:] request.
 */
- (void)peripheral:(CBPeripheral *)peripheral didWriteValueForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error 
{
    if (error) 
    {
        NSLog(@"Error writing value for characteristic %@ error: %@", characteristic.UUID, [error localizedDescription]);
        return;
    }
}

/*
 Invoked upon completion of a -[setNotifyValue:forCharacteristic:] request.
 */
- (void)peripheral:(CBPeripheral *)peripheral didUpdateNotificationStateForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error 
{
    if (error) 
    {
        NSLog(@"Error updating notification state for characteristic %@ error: %@", characteristic.UUID, [error localizedDescription]);
        return;
    }
    
    
    
    NSLog(@"Updated notification state for characteristic %@ (newState:%@)", characteristic.UUID, [characteristic isNotifying] ? @"Notifying" : @"Not Notifying");
    
    if( ([characteristic.UUID isEqual:[CBUUID UUIDWithString:@"2A1C"]]) ||
       ([characteristic.UUID isEqual:[CBUUID UUIDWithString:@"2A1E"]]) )
    {
        /* Set start/stop button depending on characteristic notifcation/indication */
        if( [characteristic isNotifying] )
        {
            [startStopButton setTitle:@"Stop"];
        }
        else
        {
            [startStopButton setTitle:@"Start"];
        }
    }     
}

@end

@implementation ThermometerView

-(void)drawRect:(NSRect)rect
{
    rect = [self bounds];
    [[NSColor blackColor] set];
    NSRectFill(rect);
    
    [self setNeedsDisplay:YES];
}


@end

