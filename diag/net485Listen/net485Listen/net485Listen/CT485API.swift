//
//  CT485API.swift
//  net485Listen
//
//  Created by Kevin Peck on 2020-03-11.
//  Copyright © 2020 Kevin Peck. All rights reserved.
//

import Foundation

//
// Base class of all message data types
//
@objc protocol CT485DeclareMessageTypes {
    static func getMsgTypes() ->  NSArray
}
class CT485Message : NSObject {
    var data : net485MsgHeader?
    
    required init(data: net485MsgHeader) {
        self.data = data
    }
    func description() {
        let mirror = Mirror(reflecting: self)
        mirror.children.forEach { (child) in
            print("\t\(String(describing: child.label)): \(child.value)")
        }
        //dump(self) // For debugginh=g
    }
}
//
// Singleton Entry point for resolving package to data structure
//
class CT485MessageCreator {
    static let shared = CT485MessageCreator()
    var messages: [MsgType: AnyClass?] = [:]

    // Message Types register themselves with factory
    private init() {
        // Register sub-cleasses in hash table dictionary
        let answer = subclasses(of: CT485Message.self)
        answer.forEach { (msgClass) in
            if let classWithMsgTypes = msgClass as? CT485DeclareMessageTypes.Type
            {
                for msgType in classWithMsgTypes.getMsgTypes() {
                    self.messages[msgType as! MsgType] = msgClass
                }
            }
        }
    }
    // Calling method returns object of associated type
    func create(_ pkt: net485MsgHeader  ) -> CT485Message? {
        // instantiate class and return
        if let finalClass = self.messages[pkt.packet.msgType] as? CT485Message.Type {
            return finalClass.init(data:pkt)
        } else {
            return nil
        }
    }
}
//
// The message mappings of the Packet Payload
//
class CT485Message_Empty :CT485Message, CT485DeclareMessageTypes
{
    static func getMsgTypes() -> NSArray { return [MsgType.NWKSTREQ] }
}
class CT485Message_R2RMacSession :CT485Message, CT485DeclareMessageTypes, CustomLeafReflectable
{
    var r2rCode : R2RCode? { return R2RCode(rawValue:self.data!.packet.data[0]) }
    var macAddr : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(1..<9)).reversed()) }
    var subscriptionId : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(9..<17)).reversed()) }
    
    var customMirror: Mirror {
        return Mirror(self, children: [
            "r2rCode": r2rCode ?? R2RCode.REQ
                      , "macAddr": macAddr ?? []
                      , "subscriptionId":subscriptionId ?? []])
    }
    static func getMsgTypes() -> NSArray { return [MsgType.R2R] }
}
class CT485Message_NodeMacSession :CT485Message, CT485DeclareMessageTypes, CustomLeafReflectable
{
    var nodeType : UInt8? { return self.data!.packet.data[0] }
    var macAddr : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(1..<9)).reversed()) }
    var subscriptionId : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(9..<17)).reversed()) }
    
    var customMirror: Mirror {
        return Mirror(self, children: [
            "nodeType": nodeType ?? 0x00
                      , "macAddr": macAddr ?? []
                      , "subscriptionId":subscriptionId ?? []])
    }
    static func getMsgTypes() -> NSArray { return [MsgType.GETNDIDACK] }
}
class CT485Message_SubNetMacSession :CT485Message, CT485DeclareMessageTypes, CustomLeafReflectable
{
    var nodeId : UInt8? { return UInt8(self.data!.packet.data[0]) }
    var subNet : UInt8? { return UInt8(self.data!.packet.data[1]) }
    var macAddr : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(2..<10)).reversed()) }
    var subscriptionId : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(10..<18)).reversed()) }
    
    var customMirror: Mirror {
        return Mirror(self,  children: [
                      "nodeId": nodeId ?? 0x00
                      , "subNet": subNet ?? 0x00
                      , "macAddr": macAddr ?? []
                      , "subscriptionId":subscriptionId ?? []])
    }
    static func getMsgTypes() -> NSArray { return [MsgType.TKNOFRACK, MsgType.NDDSCVRACK] }
}
class CT485Message_SetAddres :CT485Message, CT485DeclareMessageTypes, CustomLeafReflectable
{
    var nodeId : UInt8? { return UInt8(self.data!.packet.data[0]) }
    var subNet : UInt8? { return UInt8(self.data!.packet.data[1]) }
    var macAddr : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(2..<10)).reversed()) }
    var subscriptionId : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(10..<18)).reversed()) }
    var reserved : UInt8? { return UInt8(self.data!.packet.data[18]) }

    var customMirror: Mirror {
        return Mirror(self,  children: [
                      "nodeId": nodeId ?? 0x00
                      , "subNet": subNet ?? 0x00
                      , "macAddr": macAddr ?? []
                      , "subscriptionId": subscriptionId ?? [] ])
    }
    static func getMsgTypes() -> NSArray { return [MsgType.SETADDR, MsgType.SETADDRACK] }
}
class CT485Message_VerAnounce :CT485Message, CT485DeclareMessageTypes, CustomLeafReflectable
{
    var ver : UInt16? { return UInt16(self.data!.packet.data[0] + (0xFF * self.data!.packet.data[1])) }
    var rev : UInt16? { return UInt16(self.data!.packet.data[2] + (0xFF * self.data!.packet.data[3])) }
    var FFD : UInt8? { return self.data!.packet.data[4] }
    
    var customMirror: Mirror {
        return Mirror(self,children: [
                      "ver": ver ?? 0x00
                      , "rev": rev ?? 0x00
                      , "FFD": FFD ?? 0x00])
    }
    static func getMsgTypes() -> NSArray { return [MsgType.VRANNC] }
}
class CT485Message_NodeId :CT485Message, CT485DeclareMessageTypes, CustomLeafReflectable
{
    var nodeId : UInt8? { return UInt8(self.data!.packet.data[0]) }
    
    var customMirror: Mirror {
        return Mirror(self, children: ["nodeId": nodeId ?? 0x00])
    }
    static func getMsgTypes() -> NSArray { return [MsgType.NDDSCVR,MsgType.TKNOFR,MsgType.GETNDID] }
}
class CT485Message_NodeList :CT485Message, CT485DeclareMessageTypes, CustomLeafReflectable
{
    var virtSubordinate : UInt8? { return UInt8(self.data!.packet.data[0]) }
    var nodeList : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(1..<self.data!.packet.data.count-1))) }
    
    var customMirror: Mirror {
        return Mirror(self, children: ["virtSubordinate": virtSubordinate ?? 0x00, "nodeList": nodeList ?? [0x00] ])
    }
    static func getMsgTypes() -> NSArray{ return [MsgType.NWKSTREQACK, MsgType.ADDRCNF, MsgType.ADDRCNFACK] }
}
class CT485Message_NetDataSect :CT485Message, CT485DeclareMessageTypes, CustomLeafReflectable
{
    var isRead : Bool? { return UInt8(self.data!.packet.data[0] & 0x80) > 0  }
    var nodeType : UInt8? { return UInt8(self.data!.packet.data[0]) & 0x7F }
    var nodeNetData : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(1..<self.data!.packet.data.count-1)).reversed()) }
    
    var customMirror: Mirror {
        return Mirror(self, children: [
                        "isRead": isRead ?? true
                      , "nodeType": nodeType ?? 0x00
                      , "nodeNetData":nodeNetData ?? []])
    }
    static func getMsgTypes() -> NSArray { return [MsgType.NWKSDSI, MsgType.NWKSDSIACK] }
}
class CT485Message_NetEncap :CT485Message, CT485DeclareMessageTypes, CustomLeafReflectable
{
    var netEncapData : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(0..<self.data!.packet.data.count)).reversed()) }
    
    var customMirror: Mirror {
        return Mirror(self, children: ["netEncapData":netEncapData ?? []])
    }
    static func getMsgTypes() -> NSArray { return [MsgType.NWKECREQ, MsgType.NWKECREQACK] }
}
class CT485Message_data :CT485Message, CT485DeclareMessageTypes, CustomLeafReflectable
{
    var netEncapData : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(0..<self.data!.packet.data.count)).reversed()) }
    
    var customMirror: Mirror {
        return Mirror(self, children: ["netEncapData":netEncapData ?? []])
    }
    static func getMsgTypes() -> NSArray { return [MsgType.ECHO, MsgType.NWKECREQACK] }
}

