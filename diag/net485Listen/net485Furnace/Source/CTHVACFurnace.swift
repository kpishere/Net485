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
    case off
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
            var stringToReturn = self.stackArray.last
            self.stackArray.removeLast()
            return stringToReturn!
        } else {
            return nil
        }
    }
}

class CTHVACFurnace : NSObject {
    var network : PacketProcessor
    var state : CTFurnaceState
    var work : Stack net485MsgHeader
    
    required init(_ net: PacketProcessor) {
        self.network = net;
        self.state = CTFurnaceState.off
    }
}
