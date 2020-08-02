//
//  main.swift
//  net485Furnace
//
//  Created by Kevin Peck on 2020-05-02.
//  Copyright © 2020 Kevin Peck. All rights reserved.
//

import Foundation
import POSIXSerial

/*
TODO:

5.2.1 Warm Start Procedure – All Subsystems Except Thermostat

1. Gotoanidlestate.
2. Check the network node list. Am I on the list?
a. Ifno,stopandwait.
b. Ifyes,continueonwiththewarmstartprocedure.
3. Has shared data been checked for validity?
a. Ifno,checkshareddataforvalidity.
b. Ifyes,continuewithwarmstartprocedure.
4. Isshareddatavalid?
a. If no, request shared data from the network.
b. If yes, transmit shared data to the network.
5. Check the network node list. Is a thermostat on the list?
a. Ifno,stopandwait.
b. Ifyes,continuewithwarmstartprocedure.
6. Checktoseeiffaultconditionsexist.
a. Ifno,transmitaDiagnosticsSet–Clearmessagetothethermostat.
b. Ifyes,transmitaDiagnosticsSet–Faultmessagetothethermostat.
7. Subsystem is now considered to be a communicating control on the network.
8. If the subsystem is a heat pump or air conditioner, check the network node list to
see if an air handler or furnace is active on the network. a. Ifyes,dothefollowingsteps.
i. Request Configuration from the indoor unit to verify max CFM.
ii. Request Status of the indoor unit to verify fan and defrost control
operation states.
iii. Re-Command the indoor unit if necessary.

5.2.2 Warm Start Procedure - Thermostat

1. Go to an idle state. Prepare to auto-configure the HVAC system.
2. Clearalldiagnosticanddisplaymessagesinmemory.
3. Doesthethermostatuseshareddata?
a. If no, continue with warm start procedure.
b. Ifyes,hasshareddatabeencheckedforvalidity?
i. If no, check shared data for validity.
ii. If yes, continue with warm start procedure.
c. Is shared data valid?
i. If no, request shared data from the network.
ii. If yes, transmit shared data to the network.
4. Foreachsubsystemlistedinthenetworknodelist,dothefollowing:
a. RequestConfigurationdata
b. Request Identification data (optional)
c. Request Status data (optional)
5. Makecontroldecisionsandreissuecommandsasnecessary.


*/

func printUsage() {
    let msg: Array<String> = ["Usage of '\(CommandLine.arguments[0])' <serial device path>"]
    
    print(msg.joined(separator:"\n"))
}
//
// MAIN
//
let baudRate:Int = 9600
let timeoutSerialSeconds:Double = 0.000200 // 200 micro-second minimum packet hold time
let maxResponseLength:UInt = 252  // Maximum packet size with 2-byte checksum

setbuf(stdout, nil)

if CommandLine.argc < 2 {
    printUsage();
    exit(1);
}

let serialDevice :String? = CommandLine.arguments[1]

let port = POSIXSerialPort(path: serialDevice, baudRate: Int32(baudRate), readChunk:Int(maxResponseLength), readBlockMs: Float(timeoutSerialSeconds * 1e6))

var device = CTHVACFurnace.init(port!); // put device in cold start state

port?.open() // enable hardware serial port to receive commands

RunLoop.current.run() // loop
