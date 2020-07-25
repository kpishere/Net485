//
//  main.swift
//  net485Listen
//
//  Created by Kevin Peck on 2020-03-07.
//  Copyright © 2020 Kevin Peck. All rights reserved.
//

/*
TODO: Remove dependancy on ORSerialPort as it's use of IO kit does not support TTY devices.  Rather, the following
is portable between CU and TTY devices which is important for testing between virtualized and physical environments

import Foundation
import Darwin

print("Started:\n")
var masterFile: FileHandle?

var masterFD: Int32 = 0
masterFD = open("/dev/cu.wchusbserial1460",O_RDWR|O_NONBLOCK)
grantpt(masterFD)
unlockpt(masterFD)
masterFile = FileHandle.init(fileDescriptor: masterFD)

masterFile!.write("tty\u{0D}".data(using: String.Encoding.utf8)!)
sleep(1)

let data1 = masterFile!.availableData
let strData1 = String(data: data1, encoding: String.Encoding.utf8)!
print("Output: "+strData1)

*/

import Foundation
import POSIXSerial

func printUsage() {
    let msg: Array<String> = ["Usage of '\(CommandLine.arguments[0])' <serial device path>"]
    print(msg.joined(separator:"\n"))
}

//
// MAIN
//
let baudRate:Int32 = 9600
let timeoutSerialSeconds:Double = 0.000200 // 200 micro-second minimum packet hold time
let maxResponseLength:UInt = 252  // Maximum packet size with 2-byte checksum

setbuf(stdout, nil)

if CommandLine.argc < 2 {
    printUsage();
    exit(1);
}

let serialDevice = CommandLine.arguments[1]

let port = POSIXSerialPort(path: serialDevice, baudRate: Int32(baudRate))
var packet = PacketProcessor.init(port!);
port?.open()

RunLoop.current.run() // loop
