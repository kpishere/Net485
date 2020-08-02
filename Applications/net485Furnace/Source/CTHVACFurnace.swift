//
//  CTHVACFurnace.swift
//  net485Furnace
//
//  Created by Kevin Peck on 2020-05-15.
//  Copyright © 2020 Kevin Peck. All rights reserved.
//

import Foundation

// States for a two stage natural gas furnace
enum CTFurnaceState {
    case offlineDefault
    case onlineIdle
    case modeTest
    case modeRunInduce
    case modeRunHi
    case modeRunLow
    case modeRunFan
}

class Stack {
    var stackArray = [String]()
    
    func push(stringToPush: String){
        self.stackArray.append(stringToPush)
    }
    func pop() -> String? {
        if self.stackArray.last != nil {
            let stringToReturn = self.stackArray.last
            self.stackArray.removeLast()
            return stringToReturn!
        } else {
            return nil
        }
    }
}

/*
    Specific Device (furnace) :

    A collection of configuration files and/or class extension to the generic device class.

 */
class CTHVACFurnace : PacketProcessor {
    var state : CTFurnaceState = CTFurnaceState.offlineDefault
    var work : Stack = Stack()
    var system : CTDevice = CTDevice()
        
    override func serialPort(_ serialPort: POSIXSerialPort, didReceive data: Data) {
        let currentDateTime = Date();
        
        // check packet is complete
        let fmdTimediffSec = String(format: "%.3f", currentDateTime.timeIntervalSince(self.dateTimePrior))
        print("\n\(currentDateTime) \(fmdTimediffSec) Bytes:\(data.count) Pkt:\(data.asHexString)")

        let msg = net485MsgHeader.init(data)
        print(msg.description)
        let msgOfType = CT485MessageCreator.shared.create(msg)
        msgOfType?.description()

        self.processEvent(msgOfType!);

        self.dateTimePrior = currentDateTime
    }
    
    func sendMessage(_ msg: CT485Message) {
        let toSend = msg.data?.bytes()
        self.serialPort?.send(toSend);
    }
    
    func processEvent(_ message: CT485Message) {
        switch(self.state) {
            case .offlineDefault :
//            5.1.3 Cold Start Procedure -- Wait to receive a network node list.
//                  On receiving the node list, do Warm Start Procedure
                if(message is CT485Message_NodeList) {
                    self.state = CTFurnaceState.onlineIdle
                    self.system.nodeList = (message as! CT485Message_NodeList)
                    self.system.ConfirmNetworkNodeList(self.sendMessage)
                    self.processEvent(message)
                }
            case .onlineIdle :
                NSLog("new state: \(self.state)");
            case .modeTest :
                NSLog("new state: \(self.state)");
            case .modeRunFan :
                NSLog("new state: \(self.state)");
            case .modeRunInduce :
                NSLog("new state: \(self.state)");
            case .modeRunLow :
                NSLog("new state: \(self.state)");
            case .modeRunHi :
                NSLog("new state: \(self.state)");
        }
    }
}
