/*
 Copyright (C) 2018 Apple Inc. All Rights Reserved.
 See LICENSE.txt for this sample’s licensing information
 
 Abstract:
 Interface file for Health Thermometer Client app using Bluetooth Low Energy (LE) Health Thermometer Service. This app demonstrats the use of CoreBluetooth APIs for LE devices.
 */

#import <Cocoa/Cocoa.h>
#import <CoreBluetooth/CoreBluetooth.h>

@interface HealthThermometerClientAppDelegate : NSObject <NSApplicationDelegate,CBCentralManagerDelegate, CBPeripheralDelegate> 
{
    NSWindow *window;
    NSWindow *scanSheet;
    
    NSString * deviceName;
    NSString * manufactureName;
    NSString * tempType;
    NSString * tempString;
    NSString * timeStampString;
    NSString * connectStatus;
    NSString * mesurementType;
    
    CBCentralManager *manager;
    CBPeripheral *testPeripheral;
    CBCharacteristic * temperatureMeasurementChar;
    CBCharacteristic * intermediateTempChar;
    
    NSMutableArray *thermometers;
    NSArrayController *arrayController;   
    BOOL autoConnect;
    
    IBOutlet NSButton * connectButton;
    IBOutlet NSProgressIndicator *progressIndicator;
    IBOutlet NSButton * startStopButton;
}

@property (assign) IBOutlet NSWindow *window;
@property (assign) IBOutlet NSWindow *scanSheet;
@property (copy) NSString* deviceName;
@property (copy) NSString * manufactureName;
@property (copy) NSString* tempType;
@property (copy) NSString* tempString;
@property (copy) NSString* timeStampString;
@property (copy) NSString * connectStatus;
@property (copy) NSString * mesurementType;
@property (retain) CBCharacteristic * temperatureMeasurementChar;
@property (retain) CBCharacteristic * transparentServiceChar;
@property (retain) CBCharacteristic * intermediateTempChar;
@property (retain) NSMutableArray *thermometers;

@property (retain) NSFileHandle *file;
@property (retain) NSString * filepath;
@property (retain) NSString * dateString;
//object for File Handle
//NSError *error;
//crearing error object for string with file contents format
//@property (copy) NSMutableData *writingdatatofile;
@property (assign) IBOutlet NSArrayController *arrayController;

- (IBAction) openScanSheet:(id) sender;
- (IBAction) closeScanSheet:(id)sender;
- (IBAction) cancelScanSheet:(id)sender;
- (IBAction) connectButtonPressed:(id)sender;
- (IBAction) startButtonPressed:(id)sender;

- (void) startScan;
- (void) stopScan;
- (BOOL) isLECapableHardware;

@end


@interface ThermometerView : NSView
{
}

@end
