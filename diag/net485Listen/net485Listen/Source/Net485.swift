//
//  Net485.swift
//  net485Listen
//
//  Created by Kevin Peck on 2020-03-11.
//  Copyright © 2020 Kevin Peck. All rights reserved.
//

import Foundation
import ORSSerial

enum MsgType:UInt8 {
    //
    // CT-CIM Application message types
    //
    case CCMD = 0x03 //Control Command 
    case CCMDACK = 0x83
    case DMAR = 0x1D //Direct Memory Access Read
    case DMARACK = 0x9D
    case DMAW = 0x1E //Direct Memory Access Write
    case DMAWACK = 0x9E
    case ECHO = 0x5A //Echo
    case ECHOACK = 0xDA
    case GAPPDATA = 0x44 //Get Shared Data from Application
    case GAPPDATAACK = 0xC4
    case GCONF = 0x01 //Get Configuration
    case GCONFACK = 0x81
    case GDEVDATA = 0x11 //Application Get Shared Device Data
    case GDEVDATAACK = 0x91
    case GDIAG = 0x06 //Get Diagnostics
    case GDIAGACK = 0x86
    case GIDENT = 0x0E //Get Identification Data
    case GIDENTACK = 0x8E
    case GMFGDAT = 0x13 //Get Manufacturer Device Data 
    case GMFGDATACK = 0x93
    case GMSGGDAT = 0x20 //Get Manufacturer Generic Data
    case GMSGGDATACK = 0xA0
    case GSENSR = 0x07 //Get Sensor Data
    case GSENSRACK = 0x87
    case GSTAT = 0x02 //Get Status
    case GSTATACK = 0x82
    case GUMENU = 0x41 //Get User Menu
    case GUMENUACK = 0xC1
    case SDEVDATA = 0x10 //Application Set Network Shared Data
    case SDEVDATAACK = 0x90
    case SDIAG = 0x05 //Set Diagnostics 
    case SDIAGACK = 0x85
    case SDMSG = 0x04 //Set Display Message 
    case SDMSGACK = 0x84
    case SFAPPDATA = 0x43 //Factory Set Application Shared Data
    case SFAPPDATAACK = 0xC3
    case SIDENT = 0x0D //Set Identification Data 
    case SIDENTACK = 0x8D
    case SMFGDAT = 0x12 //Set Manufacturer Device Data
    case SMFGDATACK = 0x92
    case SMFGGDAT = 0x1F //Set Manufacturer Generic Data
    case SMFGGDATACK = 0x9F
    case SNETLIST = 0x14 //Set Network Node List
    case SNETLISTACK = 0x94
    case UUMENU = 0x42 //Update User Menu
    case UUMENUACK = 0xC2
    //
    // CT-485 Dataflow message types
    //
    case ADDRCNF = 0x76 //Address Confirmation
    case ADDRCNFACK = 0xF6
    case GETNDID = 0x7B //Get Node ID
    case GETNDIDACK = 0xFB
    case NDDSCVR = 0x79 //Node Discovery
    case NDDSCVRACK = 0xF9
    case NWKECREQ = 0x7E //Network Encapsulation Request
    case NWKECREQACK = 0xFE
    case NWKSDSI = 0x7D //Network Shared Data Sector Image
    case NWKSDSIACK = 0xFD
    case NWKSTREQ = 0x75 //Network State Request
    case NWKSTREQACK = 0xF5
    case R2R = 0x00 //Request to Receive (R2R)
    case RSVD_7C = 0x7C //Reserved
    case RSVD_7CACK = 0xFC
    case RSVD_DEFAULT = 0x7F //Reserved
    case RSVD_DEFAULTACK = 0xFF
    case SETADDR = 0x7A //Set Address
    case SETADDRACK = 0xFA
    case TKNOFR = 0x77 //Token Offer
    case TKNOFRACK = 0xF7
    case VRANNC = 0x78 //Version Announcement
    case VRANNCACK = 0xF8
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
enum HeaderStructure : Int {
    case HeaderDestAddr = 0x00
    case HeaderSrcAddr = 0x01
    case HeaderSubnet = 0x02
    case HeaderSndMethd = 0x03
    case HeaderSndParam = 0x04
    case HeaderSndParam1 = 0x05 /* Msg from coordinator = Nth node in all nodes of same type, Msg from subordinate == Zero */
    case HeaderSrcNodeType = 0x06
    case PacketMsgType = 0x07
    case PacketNumber = 0x08
    case PacketLength = 0x09
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
        self.addrDestination = data[HeaderStructure.HeaderDestAddr.rawValue]
        self.addrSource = data[HeaderStructure.HeaderSrcAddr.rawValue]
        self.subnet = data[HeaderStructure.HeaderSubnet.rawValue]
        self.sendMethod = SendMethod( rawValue:data[HeaderStructure.HeaderSndMethd.rawValue] ) ?? .NODETYPROUTING
        self.sendParameter.0 = data[HeaderStructure.HeaderSndParam.rawValue]
        self.sendParameter.1 = data[HeaderStructure.HeaderSndParam1.rawValue]
        self.nodeTypeSource = data[HeaderStructure.HeaderSrcNodeType.rawValue]
        self.packet = net485Packet.init(data)
    }
    var description: String {
        return "Dest:\(self.addrDestination) Src:\(self.addrSource) Sub:\(self.subnet) SendMethd:\(self.sendMethod) Params:\(self.sendParameter) SrcNodeType:\(self.nodeTypeSource) :: \(self.packet.description)"
    }
}


class PacketProcessor : NSObject, ORSSerialPortDelegate {
    let maxPktSize : UInt = 252
    let pktHeaderSize : UInt = 10
    let pktCrcSize : UInt = 2
    var accumlPacket : Data = Data()
    var dateTimePrior : Date = Date()
    
    convenience init(_ port: ORSSerialPort) {
        self.init()
        port.delegate = self
    }
    func serialPortWasRemovedFromSystem(_ serialPort: ORSSerialPort) {
        exit(2)
    }
    
    func serialPort(_ serialPort: ORSSerialPort, didReceive data: Data) {
        if(data.count > 0) {
            self.accumlPacket += data;
        }
        let pktLen = self.isValid(data: self.accumlPacket)
        print("\nnew:\(data.count) acum:\(self.accumlPacket.count) crc:\(pktLen)");
        if(pktLen>0) {
            let currentDateTime = Date();
            let validPktPart = Range(0...Int(pktLen-1))
            let partialData = self.accumlPacket.subdata(in: validPktPart)
            
            // check packet is complete
            let fmdTimediffSec = String(format: "%.3f", currentDateTime.timeIntervalSince(self.dateTimePrior))
            print("\n\(currentDateTime) \(fmdTimediffSec) Bytes:\(pktLen) Pkt:\(partialData.asHexString)")

            let msg = net485MsgHeader.init(partialData)
            print(msg.description)
            let msgOfType = CT485MessageCreator.shared.create(msg)
            msgOfType?.description()
            
            self.accumlPacket.removeSubrange(validPktPart)

            self.dateTimePrior = currentDateTime
        }
        // Recover from an invalid packet -- note that with one invalid packet, others can be lost too
        // this isn't ideal
        if(self.accumlPacket.count > maxPktSize) {
            print("\n CRCERR Bytes:\(self.accumlPacket.count) Pkt:\(self.accumlPacket.asHexString)")
            self.accumlPacket.removeAll()
        }
    }
    
    // Returns 0 if not valid, calculated length of valid data otherwise
    func isValid(data: Data) -> UInt {
        // Minimum packet size
        if( data.count < (self.pktHeaderSize + self.pktCrcSize)) {
            return 0;
        }
        // Calculated size requirement
        let payloadLen = UInt(data[HeaderStructure.PacketLength.rawValue])
        if( data.count < ( payloadLen + self.pktHeaderSize + self.pktCrcSize)) {
            return 0;
        }
        // CRC is OK
        return (self.passCRC(data: data) ? payloadLen + self.pktHeaderSize + self.pktCrcSize : 0)
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
