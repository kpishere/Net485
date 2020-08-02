//
//  main.swift
//  net485Listen
//
//  Created by Kevin Peck on 2020-03-07.
//  Copyright © 2020 Kevin Peck. All rights reserved.
//
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
