//
//  CTCommand.swift
//  net485Furnace
//
//  Created by Kevin Peck on 2020-05-04.
//  Copyright © 2020 Kevin Peck. All rights reserved.
//

import Foundation

enum CTSysType_ID0B0 : UInt8 {
    case UNKN = 0 // Unkonown or to be determined
    case CONV = 1 // Conventional
    case HP = 2 // Heatpump
    case DF = 3 // Dual fuel
    case AC = 4 // Cooling
    case GASH = 5 // Gas heat
    case EH = 6 // Electric heat
    case ES = 7 // Electric only
    case FN = 8 // Fan only
    case GH = 9 // Geothermal heat pump
    case GF = 10 // Geothermal dual fuel
    case BC = 11 // Boiler
    case BH = 12 // Boiler heat pump
    case UNUSD = 127 // Unused
    case OTHER = 255 // Something different than above
}
enum CTConfig_Opt_ID0B10 : UInt8 {
    case EMR = 0x80 // EMR Enable/Disable
    case KeypadLock = 0x40 // Keypack lockout Enable/Disable
    case TempIsFarheight = 0x20 // Temp in F == 1, C == 0
    case FastStage2 = 0x10 // Fast 2nd Stage Cool/Heat/Aux ON/OFF
    case DispForceOn = 0x08 // Continuous Display Enabled/Disabled
    case CompLockOut = 0x04 // Compressor lockout Enabled/Disabled
    case AdjHeatCycleRate = 0x02 // 1 == Fast, 0 == Slow (Unused)
    case AdjCoolCycleRate = 0x01 // 1 == Fast, 0 == Slow (Unused)
}
enum CTSensorWeight : UInt8 {
    case NONE = 0
    case Low = 1
    case Med = 2
    case High = 3
}
enum CTProgInterval : UInt8 {
    case Steps4 = 0
    case Steps2 = 1
    case NonProg = 2
    case Reserved = 3
}
enum CTProgProfile : UInt8 {
    case NonProg = 0
    case P5Day11 = 1
    case P7Day = 2
    case P5Day2 = 3
    
}
enum CTUse : UInt8 {
    case Residential = 0 // Default
    case Commercial = 1
}
struct CTConfig_Thermostat_ID0 {
    var SystemType : CTSysType_ID0B0
    var _StagesHeatCool : UInt8
    var BalancePointSetTemp : UInt8 // 0-127 valid (Units?), 0xFF === Default (Unused)
    var FilterTimeHrs : UInt16 // 0 === Disabled, 0xFFFF === Default (Unused)
    var TempDispAdjFctr : Int8 // 0 === Unused or zero offset
    var ProgHoldTimeHrs : UInt16 // 0 === Disabled, 0xFFFF === Default (Unused)
    var RangeHeatLimitTemp : UInt8 // 0xFF === Default (Unused)
    var RangeCoolLimitTemp : UInt8 // 0xFF === Default (Unused)
    var OptionFlags : CTConfig_Opt_ID0B10 // see enum CTConfig_Opt_ID0B10 above
    var _RmtSensWeight : UInt8 // [A, B, C, D], 4 weight options, 0 == Default/None/NA, 1 == Low, 2 == med, 3 == Hi
    var _ThermConfig : UInt8 // [CTProgInterval, CTProgProfile, CTUse, CTSensorWeight]
    var AirHandleLockoutTemp : UInt8 // 0-127 valid (Units?), 0xFF === Default (Unused)
    var UVLampDays : UInt16 // 0 = Disabled; 0xFFFF = Default/Unused value indicating that it is not being used
    var HumidPadHours : UInt16 // 0 = Disabled; 0xFFFF = Default/Unused value indicating that it is not being used.
    var _StagesAuxFan : UInt8
    
    var StagesHeatCool : [UInt8] {
        get { return [_StagesHeatCool >> 4,  _StagesHeatCool & 0x0F] }
        set { if( newValue.count > 1 ) {
                _StagesHeatCool = (newValue[0] & 0x0F) << 4 + (newValue[1] & 0x0F)
            }
        }
    }
    var RmtSensWeight : [UInt8]  {
        get { return [ _RmtSensWeight & 0x03, (_RmtSensWeight & 0x0C) >> 2
                , (_RmtSensWeight & 0x30) >> 4, (_RmtSensWeight & 0xC0) >> 6 ] }
        set { if( newValue.count > 3 ) {
                _RmtSensWeight = newValue[0] & 0x03 + (newValue[1] & 0x03) << 2
                + (newValue[2] & 0x03) << 4  + (newValue[0] & 0x03) << 6
            }
        }
    }
    var ThermConfig : [UInt8] {
        get { return [ _ThermConfig & 0x03, (_ThermConfig & 0x0C) >> 2, (_ThermConfig & 0x10) >> 4
            , _ThermConfig & 0xC0 >> 6 ]}
        set { if( newValue.count > 3 ) {
                _ThermConfig = newValue[0] & 0x03 + (newValue[1] & 0x03) << 2
                + (newValue[2] & 0x01) << 4  + (newValue[3] & 0x03) << 6
            }
        }
    }
    var StagesAuxFan : [UInt8] {
        get { return [_StagesAuxFan >> 4,  _StagesAuxFan & 0x0F] }
        set { if( newValue.count > 1 ) {
                _StagesAuxFan = (newValue[0] & 0x0F) << 4 + (newValue[1] & 0x0F)
            }
        }
    }
}

class CTCommand {
    // 5.1 Get Configuration
    func GetConfiguration( _ route: net485Routing,_ callback:(_ data: net485Packet) -> Void) -> Void {
    
    }
    
}
