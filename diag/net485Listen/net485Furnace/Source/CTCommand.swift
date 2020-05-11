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
    case DisableDegCOffSlow = 0x00
}
enum CTSensorWeight : UInt8 {
    case NONE = 0
    case Low = 1
    case Med = 2
    case High = 3
}
enum CTConfig_ID0B12 : UInt8 {
    case LocalSensWtHi = 0xC0 // Default
    case LocalSensWtMed = 0x80
    case LocalSensWtLo = 0x40

    case Commercial = 0x10 // Unset is Default (Residential)

    case ProgProf_5Day11 = 0x04
    case ProgProf_7Day = 0x08
    case ProgProf_5Day2 = 0x0C

    case ProgInerval_Steps2 = 0x01
    case ProgInerval_NonProg = 0x02
    case ProgInerval_Reserved = 0x03

    case LocalSensWtNone_Residential_NonProg_Steps4 = 0x00
}
enum CTConfig_ID0B22 : UInt8 {
    case DST = 0x01
    case KeypadLockTotal = 0x02
    case KeypadLockPartial = 0x04
    case BeeperEnabled = 0x20
    case BModeEnabled = 0x40
    case RTClockChangeLockout = 0x80
    case SDT_KeypadEnabled_BeeperDisabled_OMode_RTClockChange = 0x00
}
enum CTConfig_ID0B27 : UInt8 {
    case PhoneNumDisplayOnCommsFault = 0x01
    case None = 0x00
}
enum CTConfig_ID0B30 : UInt8 {
    case HUMCapable = 0x08
    case DHMCapable = 0x04
    case IndepHUMCapable = 0x02
    case IndepDHMCapable = 0x01
    case None = 0x00
}
enum CTConfig_ID0B31 : UInt8 {
    case ProgProf5Day2Capable = 0x08
    case ProgProf7DayCapable = 0x04
    case ProgProf5Day11Capable = 0x02
    case NonProgProfCapable = 0x01
    case NotCapable = 0x00
}
enum CTConfig_ID0B32 : UInt8 {
    case Steps2Capable = 0x04
    case NonProgCapableSteps1 = 0x02
    case Steps4Capable = 0x01
    case NotCapable = 0x00
}
struct CTConfig_Thermostat_ID0 {
    var SystemType_0 : CTSysType_ID0B0
    var _StagesHeatCool_1 : UInt8
    var StagesHeatCool_1 : [UInt8] {
        get { return [_StagesHeatCool_1 >> 4,  _StagesHeatCool_1 & 0x0F] }
        set { if( newValue.count > 1 ) {
                _StagesHeatCool_1 = (newValue[0] & 0x0F) << 4 + (newValue[1] & 0x0F)
            }
        }
    }
    var BalancePointSetTemp_2 : UInt8 // 0-127 valid (Units?), 0xFF === Default (Unused)
    var FilterTimeHrs_3_4 : UInt16 // 0 === Disabled, 0xFFFF === Default (Unused)
    var TempDispAdjFctr_5 : Int8 // 0 === Unused or zero offset
    var ProgHoldTimeHrs_6_7 : UInt16 // 0 === Disabled, 0xFFFF === Default (Unused)
    var RangeHeatLimitTemp_8 : UInt8 // 0xFF === Default (Unused)
    var RangeCoolLimitTemp_9 : UInt8 // 0xFF === Default (Unused)
    var _OptionFlags_10 : UInt8 // see enum CTConfig_Opt_ID0B10 above
    var OptionFlags_10 : [CTConfig_Opt_ID0B10] {
        get { return [CTConfig_Opt_ID0B10(rawValue: _OptionFlags_10 & CTConfig_Opt_ID0B10.AdjCoolCycleRate.rawValue) ?? .DisableDegCOffSlow
            , CTConfig_Opt_ID0B10(rawValue: _OptionFlags_10 & CTConfig_Opt_ID0B10.AdjHeatCycleRate.rawValue) ?? .DisableDegCOffSlow
            , CTConfig_Opt_ID0B10(rawValue: _OptionFlags_10 & CTConfig_Opt_ID0B10.CompLockOut.rawValue) ?? .DisableDegCOffSlow
            , CTConfig_Opt_ID0B10(rawValue: _OptionFlags_10 & CTConfig_Opt_ID0B10.DispForceOn.rawValue) ?? .DisableDegCOffSlow
            , CTConfig_Opt_ID0B10(rawValue: _OptionFlags_10 & CTConfig_Opt_ID0B10.FastStage2.rawValue) ?? .DisableDegCOffSlow
            , CTConfig_Opt_ID0B10(rawValue: _OptionFlags_10 & CTConfig_Opt_ID0B10.TempIsFarheight.rawValue) ?? .DisableDegCOffSlow
            , CTConfig_Opt_ID0B10(rawValue: _OptionFlags_10 & CTConfig_Opt_ID0B10.KeypadLock.rawValue) ?? .DisableDegCOffSlow
            , CTConfig_Opt_ID0B10(rawValue: _OptionFlags_10 & CTConfig_Opt_ID0B10.EMR.rawValue) ?? .DisableDegCOffSlow
           ] }
        set { _OptionFlags_10 = newValue.reduce(0, {x,y in x + y.rawValue }) }
    }
    var _RmtSensWeight_11 : UInt8 // [A, B, C, D], 4 weight options, 0 == Default/None/NA, 1 == Low, 2 == med, 3 == Hi
    var RmtSensWeight_11 : [CTSensorWeight]  {
        get { return [ CTSensorWeight( rawValue: _RmtSensWeight_11 & 0x03) ?? .NONE
            , CTSensorWeight( rawValue: (_RmtSensWeight_11 & 0x0C) >> 2) ?? .NONE
            , CTSensorWeight( rawValue: (_RmtSensWeight_11 & 0x30) >> 4) ?? .NONE
            , CTSensorWeight( rawValue: (_RmtSensWeight_11 & 0xC0) >> 6) ?? .NONE ] }
        set { if(newValue.count > 3) {
                _RmtSensWeight_11 = newValue[0].rawValue + newValue[1].rawValue << 2
                    + newValue[2].rawValue << 4  + newValue[3].rawValue << 6
            }
        }
    }
    // Depending on the profile and Step type, DBID 1 shall be populated
    var _ThermConfig_12 : UInt8 // [CTProgInterval, CTProgProfile, CTUse, CTSensorWeight]
    var ThermConfig_12 : [CTConfig_ID0B12] {
        get { return [CTConfig_ID0B12(rawValue: _ThermConfig_12 & CTConfig_ID0B12.ProgInerval_Reserved.rawValue) ?? .LocalSensWtNone_Residential_NonProg_Steps4
        , CTConfig_ID0B12(rawValue: _ThermConfig_12 & CTConfig_ID0B12.ProgProf_5Day2.rawValue) ?? .LocalSensWtNone_Residential_NonProg_Steps4
        , CTConfig_ID0B12(rawValue: _ThermConfig_12 & CTConfig_ID0B12.Commercial.rawValue) ?? .LocalSensWtNone_Residential_NonProg_Steps4
        , CTConfig_ID0B12(rawValue: _ThermConfig_12 & CTConfig_ID0B12.LocalSensWtHi.rawValue) ?? .LocalSensWtNone_Residential_NonProg_Steps4
           ] }
        set { _ThermConfig_12 = newValue.reduce(0, {x,y in x + y.rawValue }) }
    }
    var AirHandleLockoutTemp_13 : UInt8 // 0-127 valid (Units?), 0xFF === Default (Unused)
    var UVLampDays_14_15 : UInt16 // 0 = Disabled; 0xFFFF = Default/Unused value indicating that it is not being used
    var HumidPadHours_16_17 : UInt16 // 0 = Disabled; 0xFFFF = Default/Unused value indicating that it is not being used.
    var _StagesAuxFan_18 : UInt8
    var StagesAuxFan_18 : [UInt8] {
        get { return [_StagesAuxFan_18 >> 4,  _StagesAuxFan_18 & 0x0F] }
        set { if( newValue.count > 1 ) {
                _StagesAuxFan_18 = (newValue[0] & 0x0F) << 4 + (newValue[1] & 0x0F)
            }
        }
    }
    var AdjAuxCycleRates_19 : UInt8 // Default/Unused is 0; Percentage - 0.5% Increments. (Refer to Control Command 0x48 for details).
    var AdjHeatCycleRates_20 : UInt8 // IF > 0, DB ID 0 Byte 10 Bit 1 Ignored; Percentage - 0.5% Increments. (Refer to Control Command 0x48 for details).
    var AdjCoolCycleRates_21 : UInt8 // IF > 0, DB ID 0 Byte 10 Bit 0 Ignored; Percentage - 0.5% Increments. (Refer to Control Command 0x48 for details).
    var _ThermConfig_22 : UInt8
    var ThermConfig_22 : [CTConfig_ID0B22] {
        get {  return [CTConfig_ID0B22(rawValue: _ThermConfig_22 & CTConfig_ID0B22.DST.rawValue) ?? .SDT_KeypadEnabled_BeeperDisabled_OMode_RTClockChange
        , CTConfig_ID0B22(rawValue: _ThermConfig_22 & CTConfig_ID0B22.KeypadLockTotal.rawValue) ?? .SDT_KeypadEnabled_BeeperDisabled_OMode_RTClockChange
        , CTConfig_ID0B22(rawValue: _ThermConfig_22 & CTConfig_ID0B22.KeypadLockPartial.rawValue) ?? .SDT_KeypadEnabled_BeeperDisabled_OMode_RTClockChange
        , CTConfig_ID0B22(rawValue: _ThermConfig_22 & CTConfig_ID0B22.BeeperEnabled.rawValue) ?? .SDT_KeypadEnabled_BeeperDisabled_OMode_RTClockChange
        , CTConfig_ID0B22(rawValue: _ThermConfig_22 & CTConfig_ID0B22.BModeEnabled.rawValue) ?? .SDT_KeypadEnabled_BeeperDisabled_OMode_RTClockChange
        , CTConfig_ID0B22(rawValue: _ThermConfig_22 & CTConfig_ID0B22.RTClockChangeLockout.rawValue) ?? .SDT_KeypadEnabled_BeeperDisabled_OMode_RTClockChange] }
        set { _ThermConfig_22 = newValue.reduce(0, {x,y in x + y.rawValue }) }
    }
    var GMTOffset_23 : Int8 // Signed Byte (Scale 4) (15min increments)
    var DisplayContrast_24 : UInt8 // 0% (Lowest) to 100% (Highest)
    var CommsFaultTimer_25_26 : UInt16 // 30 Seconds to 900 Seconds (15Minutes). This indicates the time that the controls shall wait before reacting to a communication fault.
    var _ThermConfig_27 : UInt8
    var ThermConfig_27 : [CTConfig_ID0B27] {
        get { return [CTConfig_ID0B27(rawValue: _ThermConfig_27 & CTConfig_ID0B27.PhoneNumDisplayOnCommsFault.rawValue) ?? .None] }
        set { _ThermConfig_27 = newValue.reduce(0, {x,y in x + y.rawValue }) }
    }
    var NodeTypeIndoorUnit_28 : UInt8 // 0xFF = Default/Unused value indicating that it is not being used
    var NodeTypeOutdoorUnit_29 : UInt8 // 0xFF = Default/Unused value indicating that it is not being used
    var _ThermConfig_30 : UInt8
    var Thermconfig_30 : [CTConfig_ID0B30] {
        get {return [CTConfig_ID0B30(rawValue: _ThermConfig_30 & CTConfig_ID0B30.HUMCapable.rawValue) ?? .None
        , CTConfig_ID0B30(rawValue: _ThermConfig_30 & CTConfig_ID0B30.DHMCapable.rawValue) ?? .None
        , CTConfig_ID0B30(rawValue: _ThermConfig_30 & CTConfig_ID0B30.IndepHUMCapable.rawValue) ?? .None
        , CTConfig_ID0B30(rawValue: _ThermConfig_30 & CTConfig_ID0B30.IndepDHMCapable.rawValue) ?? .None] }
        set { _ThermConfig_30 = newValue.reduce(0, {x,y in x + y.rawValue }) }
    }
    var _ThermConfig_31 : UInt8
    var Thermconfig_31 : [CTConfig_ID0B31] {
        get {return [CTConfig_ID0B31(rawValue: _ThermConfig_31 & CTConfig_ID0B31.ProgProf5Day2Capable.rawValue) ?? .NotCapable
        , CTConfig_ID0B31(rawValue: _ThermConfig_31 & CTConfig_ID0B31.ProgProf7DayCapable.rawValue) ?? .NotCapable
        , CTConfig_ID0B31(rawValue: _ThermConfig_31 & CTConfig_ID0B31.ProgProf5Day11Capable.rawValue) ?? .NotCapable
        , CTConfig_ID0B31(rawValue: _ThermConfig_31 & CTConfig_ID0B31.NonProgProfCapable.rawValue) ?? .NotCapable] }
        set { _ThermConfig_31 = newValue.reduce(0, {x,y in x + y.rawValue }) }
    }
    var _ThermConfig_32 : UInt8
    var Thermconfig_32 : [CTConfig_ID0B32] {
        get {return [CTConfig_ID0B32(rawValue: _ThermConfig_32 & CTConfig_ID0B32.Steps2Capable.rawValue) ?? .NotCapable
        , CTConfig_ID0B32(rawValue: _ThermConfig_32 & CTConfig_ID0B32.NonProgCapableSteps1.rawValue) ?? .NotCapable
        , CTConfig_ID0B32(rawValue: _ThermConfig_32 & CTConfig_ID0B32.Steps4Capable.rawValue) ?? .NotCapable] }
        set { _ThermConfig_32 = newValue.reduce(0, {x,y in x + y.rawValue }) }
    }
}
enum CTProgTime : UInt8 {
    case H00M00 = 0
    case H00M15 = 1
    case H00M30 = 2
    case H00M45 = 3
    case H01M00 = 4
    case H01M15 = 5
    case H01M30 = 6
    case H01M45 = 7
    case H02M00 = 8
    case H02M15 = 9
    case H02M30 = 10
    case H02M45 = 11
    case H03M00 = 12
    case H03M15 = 13
    case H03M30 = 14
    case H03M45 = 15
    case H04M00 = 16
    case H04M15 = 17
    case H04M30 = 18
    case H04M45 = 19
    case H05M00 = 20
    case H05M15 = 21
    case H05M30 = 22
    case H05M45 = 23
    case H06M00 = 24
    case H06M15 = 25
    case H06M30 = 26
    case H06M45 = 27
    case H07M00 = 28
    case H07M15 = 29
    case H07M30 = 30
    case H07M45 = 31
    case H08M00 = 32
    case H08M15 = 33
    case H08M30 = 34
    case H08M45 = 35
    case H09M00 = 36
    case H09M15 = 37
    case H09M30 = 38
    case H09M45 = 39
    case H10M00 = 40
    case H10M15 = 41
    case H10M30 = 42
    case H10M45 = 43
    case H11M00 = 44
    case H11M15 = 45
    case H11M30 = 46
    case H11M45 = 47
    case H12M00 = 48
    case H12M15 = 49
    case H12M30 = 50
    case H12M45 = 51
    case H13M00 = 52
    case H13M15 = 53
    case H13M30 = 54
    case H13M45 = 55
    case H14M00 = 56
    case H14M15 = 57
    case H14M30 = 58
    case H14M45 = 59
    case H15M00 = 60
    case H15M15 = 61
    case H15M30 = 62
    case H15M45 = 63
    case H16M00 = 64
    case H16M15 = 65
    case H16M30 = 66
    case H16M45 = 67
    case H17M00 = 68
    case H17M15 = 69
    case H17M30 = 70
    case H17M45 = 71
    case H18M00 = 72
    case H18M15 = 73
    case H18M30 = 74
    case H18M45 = 75
    case H19M00 = 76
    case H19M15 = 77
    case H19M30 = 78
    case H19M45 = 79
    case H20M00 = 80
    case H20M15 = 81
    case H20M30 = 82
    case H20M45 = 83
    case H21M00 = 84
    case H21M15 = 85
    case H21M30 = 86
    case H21M45 = 87
    case H22M00 = 88
    case H22M15 = 89
    case H22M30 = 90
    case H22M45 = 91
    case H23M00 = 92
    case H23M15 = 93
    case H23M30 = 94
    case H23M45 = 95
}
struct CTProgSetting {
    var _dampTime : UInt8 // Heat Profile b7 Damper bit (Commercial Only), b6-b2 Hour b1,b0 Min
    var Damper : Bool {
        get { return (_dampTime & 0x80) > 0 }
        set { _fanSetPt = _dampTime & ( newValue ? 0x80 : 0x00 ) }
    }
    var Time : CTProgTime {
        get { return CTProgTime( rawValue: _dampTime & 0x7F) ?? .H00M00 }
        set { _fanSetPt = _dampTime & ( newValue.rawValue & 0x7F )}
    }
    var _fanSetPt : UInt8 // Heat Profile b7 Prg Fan b6-b0 Set Point
    var ProgFan : Bool {
        get { return (_fanSetPt & 0x80) > 0 }
        set { _fanSetPt = _fanSetPt & ( newValue ? 0x80 : 0x00 ) }
    }
    var SetPointTemp : UInt8 {
        get { return _fanSetPt & 0x7F }
        set { _fanSetPt = _fanSetPt & ( newValue & 0x7F )}
    }
}
struct CTConfig_ProgProfile {
    var Mon_MOR_OCC1 : CTProgSetting
    var Mon_DAY_UNO1 : CTProgSetting
    var Mon_EVE_OCC2 : CTProgSetting
    var Mon_NHT_UNO2 : CTProgSetting

    var Tue_MOR_OCC1 : CTProgSetting
    var Tue_DAY_UNO1 : CTProgSetting
    var Tue_EVE_OCC2 : CTProgSetting
    var Tue_NHT_UNO2 : CTProgSetting

    var Wed_MOR_OCC1 : CTProgSetting
    var Wed_DAY_UNO1 : CTProgSetting
    var Wed_EVE_OCC2 : CTProgSetting
    var Wed_NHT_UNO2 : CTProgSetting

    var Thr_MOR_OCC1 : CTProgSetting
    var Thr_DAY_UNO1 : CTProgSetting
    var Thr_EVE_OCC2 : CTProgSetting
    var Thr_NHT_UNO2 : CTProgSetting

    var Fri_MOR_OCC1 : CTProgSetting
    var Fri_DAY_UNO1 : CTProgSetting
    var Fri_EVE_OCC2 : CTProgSetting
    var Fri_NHT_UNO2 : CTProgSetting

    var WE1_MOR_OCC1 : CTProgSetting
    var WE1_DAY_UNO1 : CTProgSetting
    var WE1_EVE_OCC2 : CTProgSetting
    var WE1_NHT_UNO2 : CTProgSetting

    var WE2_MOR_OCC1 : CTProgSetting
    var WE2_DAY_UNO1 : CTProgSetting
    var WE2_EVE_OCC2 : CTProgSetting
    var WE2_NHT_UNO2 : CTProgSetting
}
struct CTConfig_Thermostat_ID1 {
    var Heat : CTConfig_ProgProfile
    var Cool : CTConfig_ProgProfile
}
enum CTCtlCmdCode : UInt8 {
// 6.2.1 HVAC Profile Control Command Codes & 6.2.2 Zone Profile Control Command Codes
    case HeatSetPointTemp = 0x01 // 1 Heat Set Point Temperature Modify
    case CoolSetPointTemp = 0x02 // 2 Cool Set Point Temperature Modify
    case HeatProfileChange = 0x03 // 3 Heat Profile Change
    case CoolProfileChange = 0x04 // 4 Cool Profile Change
    case SystemSwitchModify = 0x05 // 5 System Switch Modify
    case PermSetPointTemp = 0x06 // 6 Permanent Set Point Temp & Hold Modify
    case HoldOverride = 0x08 // 8 Hold Override
    case RealTimeDayOverride = 0x0F // 15 Real Time/Day Override
    case RestoreFactoryDefaults = 0x45 // 69 Restore Factory Defaults
    case TestMode = 0x50 // 80 Test Mode
    case SubsysInstallTest = 0x51 // 81 Subsystem Installation Test
    case SubsysBusyStatus = 0x61 // 97 Subsystem Busy Status
    case DemandDehumid = 0x62 // 98 Dehumidification Demand
    case DemandHumid = 0x63 // 99 Humidification Demand
    case DemandHeat = 0x64 // 100 Heat Demand
    case DemandCool = 0x65 // 101 Cool Demand
    case DemandFan = 0x66 // 102 Fan Demand
    case DemandBackupHeat = 0x67 // 103 Back-Up Heat Demand
    case DemandDefrost = 0x68 // 104 Defrost Demand
    case DemandAuxHeat = 0x69 // 105 Aux Heat Demand
    case PublishPrice = 0xE0 // 224 Publish Price
    // 6.2.2 Zone Profile Control Command Codes
    case AutoPairReq = 0x52 // 82 Auto-Pairing Request
    case PairOwnerReq = 0x53 // 83 Pair Ownership Request
    case DemandDamperPos = 0x60 // 96 Damper Position Demand

// 6.2.3 Motor Profile Control Command Codes
    case SetMotorSpeed = 0x6A // 106 Set Motor Speed
    case SetMotorTorque = 0x6B // 107 Set Motor Torque
    case SetAirflowDemand = 0x6C // 108 Set Airflow Demand
    case SetCtrlMode = 0x6D // 109 Set Control Mode
    case SetDemandRampRate = 0x6E // 110 Set Demand Ramp Rate
    case SetMotorDirection = 0x6F // 111 Set Motor Direction
    case SetMotorTorquePerc = 0x70 // 112 Set Motor Torque Percent
    case SetMotorPosDemand = 0x71 // 113 Set Motor Position Demand
    case SetBlowerCoeff_1 = 0x72 // 114 Set Blower Coefficient 1
    case SetBlowerCoeff_2 = 0x73 // 115 Set Blower Coefficient 2
    case SetBlowerCoeff_3 = 0x74 // 116 Set Blower Coefficient 3
    case SetBlowerCoeff_4 = 0x75 // 117 Set Blower Coefficient 4
    case SetBlowerCoeff_5 = 0x76 // 118 Set Blower Coefficient 5
    case SetBlowerId_0 = 0x77 // 119 Set Blower Identification 0
    case SetBlowerId_1 = 0x78 // 120 Set Blower Identification 1
    case SetBlowerId_2 = 0x79 // 121 Set Blower Identification 2
    case SetBlowerId_3 = 0x7A // 122 Set Blower Identification 3
    case SetBlowerId_4 = 0x7B // 123 Set Blower Identification 4
    case SetBlowerId_5 = 0x7C // 124 Set Blower Identification 5
    case SetSpeedLimit = 0x7F // 127 Set Speed Limit
    case SetTorqueLimit = 0x80 // 128 Set Torque Limit
    case SetAirflowLimit = 0x81 // 129 Set Airflow Limit
    case SetPowerOutputLimit = 0x82 // 130 Set Power Output Limit
    case SetDeviceTempLimit = 0x83 // 131 Set Device Temperature Limit
    case StopMotorByBrake = 0x85 // 133 STOP Motor by Braking
    case RunStopMotor = 0x86 // 134 RUN/STOP Motor
    case SetDemandRampTime = 0x88 // 136 Set Demand Ramp Time
    case SetInducerRampTime = 0x89 // 137 Set Inducer Ramp Rate
    case SetBlowerCoeff_6 = 0x8A // 138 Set Blower Coefficient 6
    case SetBlowerCoeff_7 = 0x8B // 139 Set Blower Coefficient 7
    case SetBlowerCoeff_8 = 0x8C // 140 Set Blower Coefficient 8
    case SetBlowerCoeff_9 = 0x8D // 141 Set Blower Coefficient 9
    case SetBlowerCoeff_10 = 0x8E // 142 Set Blower Coefficient 10
}
class CTCommand {
    // 5.1 Get Configuration
    func GetConfiguration( _ route: net485Routing,_ callback:(_ data: CT485Message) -> Void) -> Void {
    }
    func GiveConfiguration(_ callback:(_ data: CT485Message) -> Void) -> CT485Message_Data {
        return CT485Message_Data.init(data: net485MsgHeader.init(Data.init()))
    }
    // 5.2 Get Status
    func GetStatus( _ route: net485Routing,_ callback:(_ data: CT485Message) -> Void) -> Void {
    }
    func GiveStatus(_ callback:(_ data: CT485Message) -> Void) -> CT485Message_Data {
        return CT485Message_Data.init(data: net485MsgHeader.init(Data.init()))
    }
    // 5.3 Set Control Command
    func SetControlCommand(_ route: net485Routing,_ callback:(_ data: CT485Message) -> Void
    , _ command : CT485Message_Command ) -> Void {
    }
    func ConfirmControlCommand(_ callback:(_ data: CT485Message) -> Void) -> CT485Message_Command {
        return CT485Message_Command.init(data: net485MsgHeader.init(Data.init()))
    }
    // 5.4 Set Display Message
    func SetDisplayMessage(_ route: net485Routing,_ callback:(_ data: CT485Message) -> Void
    , _ message : CT485Message_SetDisplay) -> Void {
    }
    func ConfirmDisplayMessage(_ callback:(_ data: CT485Message) -> Void) -> CT485Message_SetDisplay {
        return CT485Message_SetDisplay.init(data: net485MsgHeader.init(Data.init()))
    }
    // 5.5 Set Diagnostics
    func SetDiagnosticMessage(_ route: net485Routing,_ callback:(_ data: CT485Message) -> Void
    , _ faultMessage : CT485Message_SetDiagnostics) -> Void {
    }
    func ConfirmDiagnosticMessage(_ callback:(_ data: CT485Message) -> Void) -> CT485Message_SetDiagnostics {
        return CT485Message_SetDiagnostics.init(data: net485MsgHeader.init(Data.init()))
    }
    // 5.6 Get Diagnostics
    func GetDiagnostics(_ route: net485Routing,_ callback:(_ data: CT485Message) -> Void, _ faultIndex : UInt8) -> Void {
    }
    func GiveDiagnostics(_ callback:(_ data: CT485Message) -> Void) -> CT485Message_SetDiagnostics {
        return CT485Message_SetDiagnostics.init(data: net485MsgHeader.init(Data.init()))
    }
    // 5.7 Get Sensor Data
    func GetSensor(_ route: net485Routing,_ callback:(_ data: CT485Message) -> Void) -> Void {
    }
    func GiveSensor(_ callback:(_ data: CT485Message) -> Void) -> CT485Message_Data {
        return CT485Message_Data.init(data: net485MsgHeader.init(Data.init()))
    }
    // 5.8 Set Identification
    func SetIdent(_ route: net485Routing,_ callback:(_ data: CT485Message) -> Void, optionalIdent: CT485Message_SetIdentification) -> Void {
    }
    func ConfirmIdent(_ callback:(_ data: CT485Message) -> Void) ->CT485Message_SetIdentification {
        return CT485Message_SetIdentification.init(data: net485MsgHeader.init(Data.init()))
    }
    // 5.9 Get Identification
    func GetIdent(_ route: net485Routing,_ callback:(_ data: CT485Message) -> Void) -> Void {
    }
    func GiveIdent(_ callback:(_ data: CT485Message) -> Void) -> CT485Message_GetIdentification {
        return CT485Message_GetIdentification.init(data: net485MsgHeader.init(Data.init()))
    }
    // 5.10 Set Application Shared Data to Network
    func SetAppNetSharedData(_ route: net485Routing,_ callback:(_ data: CT485Message) -> Void, appSD: CT485Message_SetDevNetData ) -> Void {
    }
    func ConfirmAppNetSharedData(_ callback:(_ data: CT485Message) -> Void) -> CT485Message_SetDevNetData {
        return CT485Message_SetDevNetData.init(data: net485MsgHeader.init(Data.init()))
    }
    // 5.11 Get Application Shared Data from Network
    func GetAppNetSD(_ route: net485Routing,_ callback:(_ data: CT485Message) -> Void, sectorNodeType: CT485Message_GetDevNetData) -> Void {
    }
    func GiveAppNetSD(_ callback:(_ data: CT485Message) -> Void) -> CT485Message_SetDevNetData {
        return CT485Message_SetDevNetData.init(data: net485MsgHeader.init(Data.init()))
    }
    // 5.12 Set Manufacturer Device Data
    func SetMfgDeviceData (_ route: net485Routing,_ callback:(_ data: CT485Message) -> Void, mfgData : CT485Message_Data) -> Void {
    }
    func ConfirmMfgDeviceData(_ callback:(_ data: CT485Message) -> Void) -> CT485Message_Data {
        return CT485Message_Data.init(data: net485MsgHeader.init(Data.init()))
    }
    // 5.13 Get Manufacturer Device Data
    func GetMfgDeviceData(_ route: net485Routing,_ callback:(_ data: CT485Message) -> Void) {
    }
    func GiveMfgDeviceData(_ callback:(_ data: CT485Message) -> Void) -> CT485Message_Data {
        return CT485Message_Data.init(data: net485MsgHeader.init(Data.init()))
    }
    // 5.14 Set Network Node List
    func SetNetworkNodeList(_ route: net485Routing,_ callback:(_ data: CT485Message) -> Void, nodeList : CT485Message_NodeList) -> Void {
    }
    func ConfirmNetworkNodeList(_ callback:(_ data: CT485Message) -> Void) -> CT485Message_NodeList {
        return CT485Message_NodeList.init(data: net485MsgHeader.init(Data.init()))
    }
    // 5.15 Get Direct Memory Access (DMA) Read
    //void GetDMARead(Routing, RequestFunctPtr callback, MessageReadCode, MDIPacketID, DBID, Range)
}
