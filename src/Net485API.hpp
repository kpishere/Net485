
#ifndef Net485API_hpp
#define Net485API_hpp

// Node addresses
#define NODEADDR_BCAST 0x00
#define NODEADDR_PRIMY 0x01
#define NODEADDR_NARB 0xFE
#define NODEADDR_COORD 0xFF

// Node address ranges
#define NODEADDR_V1LO 0x01
#define NODEADDR_V1HI 0x0E

#define NODEADDR_V2LO 0x10
#define NODEADDR_V2HI 0x3E

#define NODEADDR_DIAGLO 0x55
#define NODEADDR_DIAGHI 0x5A

/* // Other addresses
 {0x0F, Address485Purpose.Reserved}, // Overhead validation
 {0x3F, Address485Purpose.Reserved}, // Overhead validation, Future Internet access
 {0x5B, Address485Purpose.Reserved}, // Future Wireless
 {0xC0, Address485Purpose.Restricted}, // Network Analysis
 {0xC1, Address485Purpose.Reserved}, // Future system Authentication
 */

// Subnet adresses
#define SUBNET_BCAST 0x00
#define SUBNET_MAINT 0x01
#define SUBNET_V1SPEC 0x02
#define SUBNET_V2SPEC 0x03

// Send methods
#define SNDMTHD_NOROUTE 0x00
#define SNDMTHD_PRIORITY 0x01
#define SNDMTHD_NTYPE 0x02
#define SNDMTHD_BYSCKT 0x03

// Parameters
#define PARMBYTEHI(x) ((x)>>8)
#define PARMBYTELO(x) ((x)&0xff)

// Node Type codes
#define NTC_ANY         0x00 /*Wild card for any Node Type*/
#define NTC_THERM       0x01 /*Thermostat*/
#define NTC_FURNGAS     0x02 /*IFC Gas Furnace*/
#define NTC_AHNDLR      0x03 /*Air Handler*/
#define NTC_ACOND       0x04 /*Air Conditioner*/
#define NTC_HPUMP       0x05 /*Heat Pump*/
#define NTC_FURNELEC    0x06 /*IFC Electric Furnace*/
#define NTC_PKGGAS      0x07 /*IFC Package System - Gas*/
#define NTC_PKGELEC     0x08 /*IFC Package System - Electric*/
#define NTC_XOVER       0x09 /*Crossover (aka OBBI)*/
#define NTC_COMPSEC     0x0A /*Secondary Compressor*/
#define NTC_AIREXCH     0x0B /*Air Exchanger*/
#define NTC_UNICTL      0x0C /*Unitary Control*/
#define NTC_DEHUM       0x0D /*Dehumidifier*/
#define NTC_ACLEANE     0x0E /*Electronic Air Cleaner*/
#define NTC_ERV         0x0F /*ERV*/
#define NTC_HUMEV       0x10 /*Humidifier (Evaporative)*/
#define NTC_HUMSTM      0x11 /*Humidifier (Steam)*/
#define NTC_HRV         0x12 /*HRV*/
#define NTC_IAQANZR     0x13 /*IAQ Analyzer*/
#define NTC_MEDACLN     0x14 /*Media Air Cleaner*/
#define NTC_ZCTRL       0x15 /*Zone Control*/
#define NTC_ZUI         0x16 /*Zone User Interface*/
#define NTC_BOILER      0x17 /*Boiler*/
#define NTC_WHGAS       0x18 /*Water Heater – Gas*/
#define NTC_WHELEC      0x19 /*Water Heater – Electric*/
#define NTC_WHCMTL      0x1A /*Water Heater - Commercial*/
#define NTC_POOLH       0x1B /*Pool Heater*/
#define NTC_CFAN        0x1C /*Ceiling Fan*/
#define NTC_GW          0x1D /*Gateway*/
#define NTC_DIAG        0x1E /*Diagnostic Device*/
#define NTC_LTCTRL      0x1F /*Lighting Control*/
#define NTC_SCURT       0x20 /*Security System*/
#define NTC_UVLT        0x21 /*UV Light*/
#define NTC_WDSV        0x22 /*Weather Data Device*/
#define NTC_WHSEFAN     0x23 /*Whole House Fan*/
#define NTC_SLRINV      0x24 /*Solar Inverter*/
#define NTC_ZDAMP       0x25 /*Zone Damper*/
#define NTC_ZTC         0x26 /*Zone Temperature Control (ZTC)*/
#define NTC_TSENSE      0x27 /*Temperature Sensor*/
#define NTC_OCC         0x28 /*Occupancy Sensor*/
#define NTC_NETCTRL     0xA6 /*Network Coordinator*/

// Message Types
//
// CT-CIM Application message types
//
#define MSGTYP_GCONF    0x01 /*Get Configuration*/
#define MSGTYP_GSTAT    0x02 /*Get Status*/
#define MSGTYP_CCMD     0x03 /*Control Command*/
#define MSGTYP_SDMSG    0x04 /*Set Display Message*/
#define MSGTYP_SDIAG    0x05 /*Set Diagnostics*/
#define MSGTYP_GDIAG    0x06 /*Get Diagnostics*/
#define MSGTYP_GSENSR   0x07 /*Get Sensor Data*/
#define MSGTYP_SIDENT   0x0D /*Set Identification Data*/
#define MSGTYP_GIDENT   0x0E /*Get Identification Data*/
#define MSGTYP_SDEVDATA 0x10 /*Application Set Network Shared Data*/
#define MSGTYP_GDEVDATA 0x11 /*Application Get Shared Device Data*/
#define MSGTYP_SMFGDAT  0x12 /*Set Manufacturer Device Data*/
#define MSGTYP_GMFGDAT  0x13 /*Get Manufacturer Device Data*/
#define MSGTYP_SNETLIST 0x14 /*Set Network Node List */
#define MSGTYP_DMAR     0x1D /*Direct Memory Access Read*/
#define MSGTYP_DMAW     0x1E /*Direct Memory Access Write*/
#define MSGTYP_SMFGGDAT 0x1F /*Set Manufacturer Generic Data*/
#define MSGTYP_GMSGGDAT 0x20 /*Get Manufacturer Generic Data*/
#define MSGTYP_GUMENU   0x41 /*Get User Menu*/
#define MSGTYP_UUMENU   0x42 /*Update User Menu*/
#define MSGTYP_SFAPPDATA 0x43 /*Factory Set Application Shared Data*/
#define MSGTYP_GAPPDATA 0x44 /*Get Shared Data from Application*/
#define MSGTYP_ECHO     0x5A /*Echo*/
//
// CT-485 Dataflow message types
//
#define MSGTYP_R2R      0x00 /*Request to Receive (R2R)*/
#define MSGTYP_NETSTATE 0x75 /*Network State Request*/
#define MSGTYP_ADDRCNFM 0x76 /*Address Confirmation*/ 
#define MSGTYP_TOKEN    0x77 /*Token Offer*/ //TODO
#define MSGTYP_ANUCVER  0x78 /*Version Announcement */
#define MSGTYP_NDSCVRY  0x79 /*Node Discovery*/
#define MSGTYP_SADDR    0x7A /*Set Address*/ //TODO
#define MSGTYP_GNODEID  0x7B /*Get Node ID*/
#define MSGTYP_NSDSI    0x7D /*Network Shared Data Sector Image*/ //TODO
#define MSGTYP_NENCREQ  0x7E /*Network Encapsulation Request*/ //TODO
#define MSGRESP(msg) ((msg) | 0x80) /*Convert # to response code*/
//
// Properties specific to each message type
//
#define R2R_CODE 0x00
#define R2R_ACK_CODE 0x06


// Packet number
/* Bit 7, the Dataflow bit, is a ‘1’ (one) on all Dataflow packets i.e. all R2Rs and ACKs.
   Bit 6, reserved
   Bit 5, the Version bit, was a ‘1’ in CT-485 V 1.0 to indicate all CT-485 1.0 devices. CT-485 V 2.0 shall use a value of ‘0’ (zero) for the Version. A CT2.0 coordinator shall set the Version bit to a ‘1’ when sending Node Discovery Request messages. A CT2.0 subordinate shall reply with its version bit clear, ‘0’, indicating it is a CT2.0 device. A CT1.0 device shall reply with its version bit set indicating it is a CT1.0 device. The setting of the version bit by the coordinator is required to obtain the expected node discovery response from some older CT1.0 devices which echoed the version bit during the addressing sequence. A coordinator shall ignore this bit except for CT version determination during a node discovery response.
   Bits 4-0, chunk number: CT-485 V 2.0 does not provide streaming or chunking as a service, so the chunk number shall also be ‘0’ (zero) on all packets. */
#define NETV1 1
#define NETV2 2
#define PKTNUMBER(isDataFlow,isVer1) ( 0x00 | (isDataFlow ? 0x80 : 0x00) | (isVer1 ? 0x20 : 0x00) )
#define PKTVER(pkt) (pkt->header()[HeaderStructureE::PacketNumber]&0x20>0?NETV1:NETV2)
#define PKTISDATA(pkt) (pkt->header()[HeaderStructureE::PacketNumber]&0x80>0)

enum HeaderStructureE {
    HeaderDestAddr = 0x00,
    HeaderSrcAddr = 0x01,
    HeaderSubnet = 0x02,
    HeaderSndMethd = 0x03,
    HeaderSndParam = 0x04,
    HeaderSndParam1 = 0x05,
    HeaderSrcNodeType = 0x06,
    PacketMsgType = 0x07,
    PacketNumber = 0x08,
    PacketLength = 0x09
};

#define MTU_HEADER 10UL
#define MTU_DATA 240UL
#define MTU_CHECKSUM 2UL
#define MTU_MAX (MTU_HEADER + MTU_DATA + MTU_CHECKSUM)

typedef struct Net485PacketS {
    uint8_t buffer[MTU_MAX];
    uint8_t dataSize;
    uint8_t* header() {
        return &buffer[0];
    }
    uint8_t* data() {
        return &buffer[MTU_HEADER];
    }
    uint8_t* checksum() {
        return &buffer[MTU_HEADER + dataSize];
    }
    void display() {
#ifdef DEBUG
        Serial.print("{Pkt header: ");
        for(int i=0; i<MTU_HEADER;i++) {
            Serial.print(header()[i],HEX);
            Serial.print(" ");
        }
        Serial.println("}");
#endif
    }
} Net485Packet;

/////

enum Net485MacAddressE {
    Reserved = 0x00, MfgId = 0x01, DeviceId = 0x03, SIZE = 8
};

typedef struct Net485MacAddressS {
    uint8_t mac[Net485MacAddressE::SIZE];
    uint16_t manufacturerId(uint16_t mfgId=0) {
        if(mfgId) {
            memcpy(&mac[Net485MacAddressE::MfgId], &mfgId, sizeof(uint16_t));
        } else {
            memcpy(&mfgId, &mac[Net485MacAddressE::MfgId], sizeof(uint16_t));
        }
        return mfgId;
    };
    uint64_t deviceId(uint64_t devId) { /* only lower N bytes copied! */
        if(devId) {
            memcpy(&mac[Net485MacAddressE::DeviceId], (((char *)&devId)+Net485MacAddressE::DeviceId), Net485MacAddressE::SIZE - Net485MacAddressE::DeviceId);
        } else {
            devId = 0;
            memcpy((((char *)&devId)+Net485MacAddressE::DeviceId), &mac[Net485MacAddressE::DeviceId], Net485MacAddressE::SIZE - Net485MacAddressE::DeviceId);
        }
        return devId;
    }
    bool isSameAs(struct Net485MacAddressS *_mac) {
        bool match = true;
        for(int i=0; i<Net485MacAddressE::SIZE; i++) match &= _mac->mac[i] == mac[i];
        return match;
    }
    void clear() {
        for( int i=0; i<Net485MacAddressE::SIZE; i++ ) mac[i] = 0;
    }
    void setRandom(unsigned long _seed) {
        unsigned long seed = _seed + analogRead(0);
        randomSeed(seed);
        for(int i=Net485MacAddressE::DeviceId; i<Net485MacAddressE::SIZE; i++)
            mac[i] = random(0x00, 0xFF);
        mac[Net485MacAddressE::Reserved] = 0xFF; // Non-zero value flags a random MAC
    }
    void display() {
#ifdef DEBUG
    Serial.print("{mac:");
    for(int i=0; i<Net485MacAddressE::SIZE;i++) {
        Serial.print(mac[i],HEX);
        Serial.print(" ");
    }
    Serial.print("}");
#endif
    }
} Net485MacAddress;

typedef struct Net485DataVersionS {
    uint16_t Version;
    uint16_t Revision;
    bool isFFD;
    struct Net485DataVersionS *init(Net485Packet *pkt) {
        Version = ((uint8_t *)(pkt->data()))[0];
        Revision = ((uint8_t *)(pkt->data()))[sizeof(uint16_t)];
        isFFD = ((uint8_t *)(pkt->data()))[sizeof(uint16_t)*2];
    }
    Net485Packet *write(Net485Packet *pkt) {
        memcpy(pkt->data(), &Version, sizeof(uint16_t));
        memcpy(&(pkt->data()[sizeof(uint16_t)]), &Revision, sizeof(uint16_t));
        pkt->data()[sizeof(uint16_t)*2] = isFFD;
        return pkt;
    }
    uint8_t comp(struct Net485DataVersionS left, struct Net485DataVersionS right) {
        if(left.Version > right.Version) return 1;
        if(left.Version == right.Version && left.Revision > right.Revision) return 1;
        if(left.Version == right.Version && left.Revision == right.Revision) return 0;
        return -1;
    }
} Net485DataVersion;

#endif Net485API_hpp
