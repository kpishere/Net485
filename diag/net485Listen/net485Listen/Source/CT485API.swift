//
//  CT485API.swift
//  net485Listen
//
//  Created by Kevin Peck on 2020-03-11.
//  Copyright © 2020 Kevin Peck. All rights reserved.
//

import Foundation

//
// Application data structures
//
struct ctCIMFault {
    let faultMajor : UInt8
    let faultMinor : UInt8
    let dspMsgLen : UInt8
    let dspMsg : String
    var count: Int { return Int(self.dspMsgLen) + 3 }

    init(_ data: Data, _ start: Int) {
        self.faultMajor = data[start]
        self.faultMinor = data[start+1]
        self.dspMsgLen = data[start+2]
        // Assuming here that String will only be valid upto the null -- TODO veryfiy this
        self.dspMsg = String(data: Data(bytes: data.subdata(in:(start+3..<Int(self.dspMsgLen+3)))), encoding: .ascii) ?? ""
    }
}
enum MDI: UInt8 {
    case configuration = 0x00
    case status = 0x02
    case sensor = 0x07
    case ident = 0x0E
}
enum PASS: UInt8 {
    case ACK = 0x06
    case NAK = 0x15
}
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
        let mirror = self.members()
        mirror.forEach { (key,value) in
            print("\t\(String(describing: key)): \(value)")
        }
        //dump(self) // For debugginh=g
    }
    func members() -> [String:Any] { return [:] }
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
    static func getMsgTypes() -> NSArray { return [MsgType.NWKSTREQ,MsgType.GCONF,MsgType.GSTAT,MsgType.GSENSR,MsgType.GIDENT,MsgType.GMFGDAT, MsgType.GAPPDATA, MsgType.GETNDID] }
}
class CT485Message_R2RMacSession :CT485Message, CT485DeclareMessageTypes
{
    var r2rCode : R2RCode? { return R2RCode(rawValue:self.data!.packet.data[0]) }
    var macAddr : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(1..<9)).reversed()) }
    var subscriptionId : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(9..<17)).reversed()) }
    
    override func members() -> [String:Any] { return [
            "r2rCode": r2rCode ?? R2RCode.REQ
                      , "macAddr": macAddr ?? []
                      , "subscriptionId":subscriptionId ?? []]
    }
    static func getMsgTypes() -> NSArray { return [MsgType.R2R] }
}
class CT485Message_NodeMacSession :CT485Message, CT485DeclareMessageTypes
{
    var nodeType : UInt8? { return self.data!.packet.data[0] }
    var macAddr : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(1..<9)).reversed()) }
    var subscriptionId : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(9..<17)).reversed()) }
    
    override func members() -> [String:Any] { return [
            "nodeType": nodeType ?? 0x00
                      , "macAddr": macAddr ?? []
                      , "subscriptionId":subscriptionId ?? []]
    }
    static func getMsgTypes() -> NSArray { return [MsgType.GETNDIDACK] }
}
class CT485Message_SubNetMacSession :CT485Message, CT485DeclareMessageTypes
{
    var nodeId : UInt8? { return UInt8(self.data!.packet.data[0]) }
    var subNet : UInt8? { return UInt8(self.data!.packet.data[1]) }
    var macAddr : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(2..<10)).reversed()) }
    var subscriptionId : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(10..<18)).reversed()) }
    
    override func members() -> [String:Any] { return [
                      "nodeId": nodeId ?? 0x00
                      , "subNet": subNet ?? 0x00
                      , "macAddr": macAddr ?? []
                      , "subscriptionId":subscriptionId ?? []]
    }
    static func getMsgTypes() -> NSArray { return [MsgType.TKNOFRACK, MsgType.NDDSCVRACK] }
}
class CT485Message_SetAddres :CT485Message, CT485DeclareMessageTypes
{
    var nodeId : UInt8? { return UInt8(self.data!.packet.data[0]) }
    var subNet : UInt8? { return UInt8(self.data!.packet.data[1]) }
    var macAddr : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(2..<10)).reversed()) }
    var subscriptionId : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(10..<18)).reversed()) }
    var reserved : UInt8? { return UInt8(self.data!.packet.data[18]) }

    override func members() -> [String:Any] { return [
                      "nodeId": nodeId ?? 0x00
                      , "subNet": subNet ?? 0x00
                      , "macAddr": macAddr ?? []
                      , "subscriptionId": subscriptionId ?? [] ]
    }
    static func getMsgTypes() -> NSArray { return [MsgType.SETADDR, MsgType.SETADDRACK] }
}
class CT485Message_VerAnounce :CT485Message, CT485DeclareMessageTypes
{
    var ver : UInt16? { return UInt16(self.data!.packet.data[0] + (0xFF * self.data!.packet.data[1])) }
    var rev : UInt16? { return UInt16(self.data!.packet.data[2] + (0xFF * self.data!.packet.data[3])) }
    var FFD : UInt8? { return self.data!.packet.data[4] }
    
    override func members() -> [String:Any] { return [
                      "ver": ver ?? 0x00
                      , "rev": rev ?? 0x00
                      , "FFD": FFD ?? 0x00]
    }
    static func getMsgTypes() -> NSArray { return [MsgType.VRANNC] }
}
class CT485Message_NodeId :CT485Message, CT485DeclareMessageTypes
{
    var nodeId : UInt8? { return UInt8(self.data!.packet.data[0]) }
    
    override func members() -> [String:Any] { return ["nodeId": nodeId ?? 0x00]
    }
    static func getMsgTypes() -> NSArray { return [MsgType.NDDSCVR,MsgType.TKNOFR] }
}
class CT485Message_NodeList :CT485Message, CT485DeclareMessageTypes
{
    var virtSubordinate : UInt8? { return UInt8(self.data!.packet.data[0]) }
    var nodeList : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(1..<self.data!.packet.data.count-1))) }
    
    override func members() -> [String:Any] { return ["virtSubordinate": virtSubordinate ?? 0x00, "nodeList": nodeList ?? [0x00] ]
    }
    static func getMsgTypes() -> NSArray{ return [MsgType.NWKSTREQACK, MsgType.ADDRCNF, MsgType.ADDRCNFACK] }
}
class CT485Message_SetNodeList :CT485Message, CT485DeclareMessageTypes
{
    var nodeType : UInt8? { return UInt8(self.data!.packet.data[0]) }
    var nodeList : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(1..<self.data!.packet.data.count-1))) }
    
    override func members() -> [String:Any] { return ["nodeType": nodeType ?? 0x00, "nodeList": nodeList ?? [0x00] ]
    }
    static func getMsgTypes() -> NSArray{ return [MsgType.SNETLIST, MsgType.SNETLISTACK] }
}

class CT485Message_NetDataSect :CT485Message, CT485DeclareMessageTypes
{
    var isRead : Bool? { return UInt8(self.data!.packet.data[0] & 0x80) > 0  }
    var nodeType : UInt8? { return UInt8(self.data!.packet.data[0]) & 0x7F }
    var nodeNetData : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(1..<self.data!.packet.data.count-1))) }
    
    override func members() -> [String:Any] { return [
                        "isRead": isRead ?? true
                      , "nodeType": nodeType ?? 0x00
                      , "nodeNetData":nodeNetData ?? []]
    }
    static func getMsgTypes() -> NSArray { return [MsgType.NWKSDSI, MsgType.NWKSDSIACK] }
}
class CT485Message_NetEncap :CT485Message, CT485DeclareMessageTypes
{
    var netEncapData : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(0..<self.data!.packet.data.count))) }
    
    override func members() -> [String:Any] { return ["netEncapData":netEncapData ?? []]
    }
    static func getMsgTypes() -> NSArray { return [MsgType.NWKECREQ, MsgType.NWKECREQACK] }
}
class CT485Message_Data :CT485Message, CT485DeclareMessageTypes
{
    var msgData : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(0..<self.data!.packet.data.count))) }
    
    override func members() -> [String:Any] { return ["msgData":msgData ?? []]
    }
    static func getMsgTypes() -> NSArray { return [MsgType.ECHO, MsgType.ECHOACK, MsgType.GCONFACK, MsgType.GSTATACK, MsgType.GSENSRACK, MsgType.SIDENT, MsgType.GIDENTACK, MsgType.SMFGDAT, MsgType.SMFGDATACK, MsgType.GMFGDATACK, MsgType.DMARACK, MsgType.DMAW, MsgType.DMAWACK] }
}
class CT485Message_Command :CT485Message, CT485DeclareMessageTypes
{
    var commandCode : UInt16? { return UInt16(self.data!.packet.data[0] + (0xFF * self.data!.packet.data[1])) }
    var commandData : UInt16? { return UInt16(self.data!.packet.data[2] + (0xFF * self.data!.packet.data[3])) }
    
    override func members() -> [String:Any] { return [
                      "commandCode": commandCode ?? 0x00
                      , "commandData": commandData ?? 0x00]
    }
    static func getMsgTypes() -> NSArray { return [MsgType.VRANNC,MsgType.CCMD,MsgType.CCMDACK] }
}
class CT485Message_SetDisplay :CT485Message, CT485DeclareMessageTypes
{
    var nodeType : UInt8? { return self.data!.packet.data[0] }
    var dspMsgLen : UInt8? { return self.data!.packet.data[1] }
    var dspMsg : String? { return String(data: Data(bytes: self.data!.packet.data.subdata(in:(2..<self.data!.packet.data.count))), encoding: .ascii) }
    
    override func members() -> [String:Any] { return [
            "nodeType": nodeType ?? 0x00
            , "dspMsgLen": dspMsgLen ?? 0x00
            , "dspMsg": dspMsg ?? ""]
    }
    static func getMsgTypes() -> NSArray { return [MsgType.SDMSG] }
}
class CT485Message_SetDiagnostics :CT485Message, CT485DeclareMessageTypes
{
    var nodeType : UInt8? { return self.data!.packet.data[0] }
    var faultMajor : UInt8? { return self.data!.packet.data[1] }
    var faultMinor : UInt8? { return self.data!.packet.data[2] }
    var dspMsgLen : UInt8? { return self.data!.packet.data[3] }
    var dspMsg : String? { return String(data: Data(bytes: self.data!.packet.data.subdata(in:(4..<self.data!.packet.data.count))), encoding: .ascii) }

    override func members() -> [String:Any] { return [
            "nodeType": nodeType ?? 0x00
            , "faultMajor": faultMajor ?? 0x00
            , "faultMinor": faultMinor ?? 0x00
            , "dspMsgLen": dspMsgLen ?? 0x00
            , "dspMsg": dspMsg ?? ""]
    }
    static func getMsgTypes() -> NSArray { return [MsgType.SDIAG] }
}
class CT485Message_ConfirmCode :CT485Message, CT485DeclareMessageTypes
{
    var confirmCode : UInt16? { return UInt16(self.data!.packet.data[0] + (0xFF * self.data!.packet.data[1])) }

    override func members() -> [String:Any] { return ["confirmCode": confirmCode ?? 0x00]
    }
    static func getMsgTypes() -> NSArray { return [MsgType.SDMSGACK, MsgType.SDIAGACK,MsgType.SIDENTACK] }
}
class CT485Message_GetDiagnostics :CT485Message, CT485DeclareMessageTypes
{
    var faultType : UInt8? { return self.data!.packet.data[0] }
    var faultIndex : UInt8? { return self.data!.packet.data[1] }
    
    override func members() -> [String:Any] { return [
            "faultType": faultType ?? 0x00
            , "faultIndex": faultIndex ?? 0x00
            ]
    }
    static func getMsgTypes() -> NSArray { return [MsgType.GDIAG] }
}
class CT485Message_GetDiagnosticsResp :CT485Message, CT485DeclareMessageTypes
{
    var faults : [ctCIMFault]? {
        var plp = 0
        var faults : [ctCIMFault] = []
        while plp < self.data!.packet.data.count {
            let fault = ctCIMFault.init(self.data!.packet.data, plp)
            faults.append(fault)
            plp += fault.count
        }
        return faults
    }
    
    override func members() -> [String:Any] { return ["faults":faults ?? []]
    }
    static func getMsgTypes() -> NSArray { return [MsgType.GDIAGACK] }
}
class CT485Message_SetDevNetData :CT485Message, CT485DeclareMessageTypes
{
    var sectNodeType : UInt8? { return self.data!.packet.data[0] }
    // Shared data section
    var devDataLen : UInt8? { return self.data!.packet.data[1] }
    var controlId : UInt16? { return UInt16(self.data!.packet.data[2] + (0xFF * self.data!.packet.data[3])) }
    var manufId : UInt16? { return UInt16(self.data!.packet.data[4] + (0xFF * self.data!.packet.data[5])) }
    var appNodeType : UInt8? { return self.data!.packet.data[6] }
    var devData : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(7..<Int(self.data!.packet.data[1])))) }
    
    override func members() -> [String:Any] { return [
                      "sectNodeType": sectNodeType ?? 0x00
                      , "devDataLen": devDataLen ?? 0x00
                      , "controlId": controlId ?? 0x00
                      , "manufId": manufId ?? 0x00
                      , "appNodeType": appNodeType ?? 0x00
                      , "devData": devData ?? []
                      ]
    }
    static func getMsgTypes() -> NSArray { return [MsgType.SDEVDATA, MsgType.SDEVDATAACK, MsgType.GDEVDATAACK, MsgType.GAPPDATAACK] }
}
class CT485Message_GetDevNetData :CT485Message, CT485DeclareMessageTypes
{
    var sectNodeType : UInt8? { return self.data!.packet.data[0] }

    override func members() -> [String:Any] { return [
            "sectNodeType": sectNodeType ?? 0x00
            ]
    }
    static func getMsgTypes() -> NSArray { return [MsgType.GDEVDATA] }
}
class CT485Message_DMAReq :CT485Message, CT485DeclareMessageTypes
{
    var mdi : MDI? { return MDI(rawValue: self.data!.packet.data[0]) ?? .configuration }
    var pktNumber : UInt8? { return self.data!.packet.data[1] }
    var startDBId : UInt8? { return self.data!.packet.data[2] }
    var range : UInt8? { return self.data!.packet.data[3] }

    override func members() -> [String:Any] { return [
            "mdi": mdi ?? 0x00
            , "pktNumber": pktNumber ?? 0x00
            , "startDBId": startDBId ?? 0x00
            , "range": range ?? 0x00
            ]
    }
    static func getMsgTypes() -> NSArray { return [MsgType.DMAR] }
}
class CT485Message_MfgData :CT485Message, CT485DeclareMessageTypes
{
    var manufId : UInt16? { return UInt16(self.data!.packet.data[0] + (0xFF * self.data!.packet.data[1])) }
    var mfgData : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(2..<self.data!.packet.data.count))) }
    
    override func members() -> [String:Any] { return [
        "manufId":manufId ?? 0x00
        ,"mfgData":mfgData ?? []
        ]
    }
    static func getMsgTypes() -> NSArray { return [MsgType.SMFGGDAT, MsgType.SMFGGDATACK, MsgType.GMSGGDAT, MsgType.GMSGGDATACK ] }
}
class CT485Message_GetUserMenu :CT485Message, CT485DeclareMessageTypes
{
    var menuFile : UInt8? { return self.data!.packet.data[0] }
    var manuMain : UInt8? { return self.data!.packet.data[1] }
    var manuSub : UInt8? { return self.data!.packet.data[2] }
    var fetchOffset : UInt16? { return UInt16(self.data!.packet.data[3] + (0xFF * self.data!.packet.data[4])) }
    var maxReturnBytes : UInt8? { return self.data!.packet.data[5] }

    override func members() -> [String:Any] { return [
            "menuFile": menuFile ?? 0x00
            , "manuMain": manuMain ?? 0x00
            , "manuSub": manuSub ?? 0x00
            , "fetchOffset": fetchOffset ?? 0x00
            , "maxReturnBytes": maxReturnBytes ?? 0x00
            ]
    }
    static func getMsgTypes() -> NSArray { return [MsgType.GUMENU] }
}
class CT485Message_UserMenu :CT485Message, CT485DeclareMessageTypes
{
    var menuFile : UInt8? { return self.data!.packet.data[0] }
    var manuMain : UInt8? { return self.data!.packet.data[1] }
    var manuSub : UInt8? { return self.data!.packet.data[2] }
    var fetchOffset : UInt16? { return UInt16(self.data!.packet.data[3] + (0xFF * self.data!.packet.data[4])) }
    var maxReturnBytes : UInt8? { return self.data!.packet.data[5] }
    var menuData : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(6..<Int(self.data!.packet.data[5])))) }

    override func members() -> [String:Any] { return [
            "menuFile": menuFile ?? 0x00
            , "manuMain": manuMain ?? 0x00
            , "manuSub": manuSub ?? 0x00
            , "fetchOffset": fetchOffset ?? 0x00
            , "maxReturnBytes": maxReturnBytes ?? 0x00
            , "menuData": menuData ?? []
            ]
    }
    static func getMsgTypes() -> NSArray { return [MsgType.GUMENUACK] }
}
class CT485Message_SetUserMenu :CT485Message, CT485DeclareMessageTypes
{
    var menuFile : UInt8? { return self.data!.packet.data[0] }
    var manuMain : UInt8? { return self.data!.packet.data[1] }
    var manuSub : UInt8? { return self.data!.packet.data[2] }
    var fileCode1 : UInt8? { return self.data!.packet.data[3] }
    var fileValue : UInt16? { return UInt16(self.data!.packet.data[4] + (0xFF * self.data!.packet.data[5])) }
    var fileCode2 : UInt8? { return self.data!.packet.data[6] }

    override func members() -> [String:Any] { return [
            "menuFile": menuFile ?? 0x00
            , "manuMain": manuMain ?? 0x00
            , "manuSub": manuSub ?? 0x00
            , "fileCode1": fileCode1 ?? 0x00
            , "fileValue": fileValue ?? 0x00
            , "fileCode2": fileCode2 ?? 0x00
            ]
    }
    static func getMsgTypes() -> NSArray { return [MsgType.GUMENU,MsgType.UUMENU] }
}
class CT485Message_SetUserMenuResp :CT485Message, CT485DeclareMessageTypes
{
    var menuFile : UInt8? { return self.data!.packet.data[0] }
    var manuMain : UInt8? { return self.data!.packet.data[1] }
    var manuSub : UInt8? { return self.data!.packet.data[2] }
    var fileCode1 : UInt8? { return self.data!.packet.data[3] }
    var fileValue : UInt16? { return UInt16(self.data!.packet.data[4] + (0xFF * self.data!.packet.data[5])) }
    var fileCode2 : UInt8? { return self.data!.packet.data[6] }
    var status : PASS? { return PASS(rawValue: self.data!.packet.data[6]) ?? .NAK }

    override func members() -> [String:Any] { return  [
            "menuFile": menuFile ?? 0x00
            , "manuMain": manuMain ?? 0x00
            , "manuSub": manuSub ?? 0x00
            , "fileCode1": fileCode1 ?? 0x00
            , "fileValue": fileValue ?? 0x00
            , "fileCode2": fileCode2 ?? 0x00
            , "status": status ?? PASS.NAK
            ]
    }
    static func getMsgTypes() -> NSArray { return [MsgType.GUMENUACK,MsgType.UUMENUACK] }
}

class CT485Message_SetFacNetData :CT485Message, CT485DeclareMessageTypes
{
    var facDataLen : UInt8? { return self.data!.packet.data[0] }
    var controlId : UInt16? { return UInt16(self.data!.packet.data[1] + (0xFF * self.data!.packet.data[2])) }
    var manufId : UInt16? { return UInt16(self.data!.packet.data[3] + (0xFF * self.data!.packet.data[4])) }
    var appNodeType : UInt8? { return self.data!.packet.data[5] }
    var facData : [UInt8]? { return [UInt8](self.data!.packet.data.subdata(in:(6..<Int(self.data!.packet.data[1])))) }
    
    override func members() -> [String:Any] { return   [
                      "facDataLen": facDataLen ?? 0x00
                      , "controlId": controlId ?? 0x00
                      , "manufId": manufId ?? 0x00
                      , "appNodeType": appNodeType ?? 0x00
                      , "facData": facData ?? []
                      ]
    }
    static func getMsgTypes() -> NSArray { return [MsgType.SFAPPDATA, MsgType.SFAPPDATAACK] }
}

