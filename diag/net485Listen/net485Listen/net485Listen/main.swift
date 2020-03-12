//
//  main.swift
//  net485Listen
//
//  Created by Kevin Peck on 2020-03-07.
//  Copyright © 2020 Kevin Peck. All rights reserved.
//

import Foundation
import ORSSerial

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

//// CT485API
//
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
        //dump(self)
    }
}
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
///
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
//// CT485API

//// Net485
//
//
enum MsgType:UInt8 {
    case R2R = 0x00 //Request to Receive (R2R)
    // ...
    case NWKSTREQ = 0x75 //Network State Request
    case NWKSTREQACK = 0xF5
    case ADDRCNF = 0x76 //Address Confirmation
    case ADDRCNFACK = 0xF6
    case TKNOFR = 0x77 //Token Offer
    case TKNOFRACK = 0xF7
    case VRANNC = 0x78 //Version Announcement
    case VRANNCACK = 0xF8
    case NDDSCVR = 0x79 //Node Discovery
    case NDDSCVRACK = 0xF9
    case SETADDR = 0x7A //Set Address
    case SETADDRACK = 0xFA
    case GETNDID = 0x7B //Get Node ID
    case GETNDIDACK = 0xFB
    case RSVD_7C = 0x7C //Reserved
    case RSVD_7CACK = 0xFC
    case NWKSDSI = 0x7D //Network Shared Data Sector Image
    case NWKSDSIACK = 0xFD
    case NWKECREQ = 0x7E //Network Encapsulation Request
    case NWKECREQACK = 0xFE
    case RSVD_DEFAULT = 0x7F //Reserved
    case RSVD_DEFAULTACK = 0xFF
}
enum SendMethod: UInt8 {
    case NONROUTED = 0x00 //Sent directly to device, is not routed
    case PRIORITYROUTING = 0x01 //Routing by Priority Control Command Device
    case NODETYPROUTING = 0x02 //Routing by Priority Node Type
    case SOCKETROUTING = 0x03 //Routing by Socket
}
enum R2RCode: UInt8 {
    case REQ = 0x00
    case ACK = 0x06
}
struct net485Packet {
    let msgType: MsgType
    let pktNumber: UInt8
    let pktLength: UInt8
    let data: Data
    
    init(_ data:Data) {
        self.msgType = MsgType(rawValue:data[7]) ?? .RSVD_DEFAULT
        self.pktNumber = data[8]
        self.pktLength = data[9]
        self.data =  data.subdata(in: 10..<data.count-2)
    }
    var isDataFlow: Bool { return self.pktNumber & 0x80 != 0}
    var version: UInt8 { return self.pktNumber & 0x20 == 0 ? 2 : 1 }
    var chunk: UInt8 { return self.pktNumber & 0x1F }
    var description: String {
        return "\(self.msgType) isDataFlow:\(self.isDataFlow) Ver:\(self.version) Chunk:\(self.chunk) len:\(self.pktLength)"
    }
}
struct net485MsgHeader {
    let addrDestination: UInt8
    let addrSource: UInt8
    let subnet: UInt8
    let sendMethod: SendMethod
    let sendParameter: (UInt8, UInt8)
    let nodeTypeSource: UInt8
    let packet: net485Packet
    
    init(_ data:Data) {
        self.addrDestination = data[0]
        self.addrSource = data[1]
        self.subnet = data[2]
        self.sendMethod = SendMethod( rawValue:data[3] ) ?? .NODETYPROUTING
        self.sendParameter.0 = data[4]
        self.sendParameter.1 = data[5]
        self.nodeTypeSource = data[6]
        self.packet = net485Packet.init(data)
    }
    var description: String {
        return "Dest:\(self.addrDestination) Src:\(self.addrSource) Sub:\(self.subnet) SendMethd:\(self.sendMethod) Params:\(self.sendParameter) SrcNodeType:\(self.nodeTypeSource) :: \(self.packet.description)"
    }
}


class PacketProcessor : NSObject, ORSSerialPortDelegate {

    convenience init(_ port: ORSSerialPort) {
        self.init()
        port.delegate = self
    }
    
    func serialPortWasRemovedFromSystem(_ serialPort: ORSSerialPort) {
        exit(2)
    }
    
    func serialPort(_ serialPort: ORSSerialPort, didReceive data: Data) {
        let isOK = self.passCRC(data: data)
        print("\nBytes:\(data.count) Pkt:\(data.asHexString) \(isOK ? "CRCOK":"CRCERR")")
        if isOK {
            let msg = net485MsgHeader.init(data)
            print(msg.description)
            let msgOfType = CT485MessageCreator.shared.create(msg)
            msgOfType?.description()
        }
    }
    
    func passCRC(data: Data) -> Bool {
        var isOK: Bool = true
        var lrc1: Int = 0xAA
        var lrc2: Int = 0
        for b in data {
            lrc1 += Int(b)
            if(lrc1 >= 255) {lrc1 -= 255 }
            lrc2 += lrc1
            if(lrc2 >= 255) {lrc2 -= 255 }
        }
        isOK = (lrc1 == 0)
        return isOK
    }
}
//
//
//// Net485

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
