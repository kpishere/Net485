//
//  main.swift
//  net485Listen
//
//  Created by Kevin Peck on 2020-03-07.
//  Copyright © 2020 Kevin Peck. All rights reserved.
//

import Foundation
import ORSSerial

// Helper extensions/functions
//
extension String {
    var isInt: Bool {
        return Int(self) != nil
    }
}
extension Data {
    var asHexString: String {
        return reduce("") {$0 + String(format: " %02x", $1 as CVarArg)}
    }
}
func printUsage() {
    let msg: Array<String> = ["Usage of '\(CommandLine.arguments[0])' <serial device path>"]
    
    print(msg.joined(separator:"\n"))
}
func subclasses<T>(of theClass: T) -> [T] {
    var count: UInt32 = 0, result: [T] = []
    let allClasses = objc_copyClassList(&count)!

    for n in 0 ..< count {
        let someClass: AnyClass = allClasses[Int(n)]
        guard let someSuperClass = class_getSuperclass(someClass), String(describing: someSuperClass) == String(describing: theClass) else { continue }
        result.append(someClass as! T)
    }

    return result
}
//
// Helper extensions/functions

//
// MAIN
//
let baudRate = 9600
let timeoutSerialSeconds:Double = 0.000200 // 200 micro-second minimum packet hold time
let maxResponseLength:UInt = 252  // Maximum packet size with 2-byte checksum

setbuf(stdout, nil)

if CommandLine.argc < 2 {
    printUsage();
    exit(1);
}

let serialDevice = CommandLine.arguments[1]

let port = ORSSerialPort(path: serialDevice)
port?.baudRate = NSNumber(value: baudRate)
port?.parity = .none
port?.numberOfStopBits = 1
port?.numberOfDataBits = 8
port?.usesDTRDSRFlowControl = false
port?.usesRTSCTSFlowControl = false

var packet = PacketProcessor.init(port!);
port?.open()

RunLoop.current.run() // loop
