Project:      HEAD
Build Number: 1603
Date:         Tue Sep 22 15:03:52 2026
Compiler:     ICC12
Version:      7.50


Area                               Addr   Size   Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                            text   4000   219E =   8606. bytes (rel,con,rom)

       Addr  Global Symbol
      -----  --------------------------------
       4000  __start
       402B  _exit
       402D  __text_start
       402D  _main
       406F  _SDOResponseTable
       40EF  _DUMMY_ENTRY
       40F0  _CANRxISR
       4196  _IRQ_Int_Handler
       4197  _TC0_Int_Handler
       41BE  _TC3_Int_Handler
       41C9  _TC4_Int_Handler
       4239  _TC5_Int_Handler
       424C  _TC7_Int_Handler
       4277  _RTI_Int_Handler
       44B0  _enum_NULL_str
       44B1  _enum_off_on_str
       44B9  _enum_12_24_str
       44BF  _enum_stop_retract_str
       44D7  _enum_base_iso_str
       44E6  _enum_lin_act_str
       44F6  _enum_type_str
       4504  _enum_neg_pos_str
       450C  _enum_alpha_str
       456A  _enum_number_str
       4580  _NullFunction
       4584  _BlowerOn
       458E  _CamIndex
       4598  _StdVarFunction
       4682  _Advance
       46A9  _ZoomInFunct1
       46C7  _ZoomOutFunct1
       46F2  _FocusFarFunct1
       4710  _FocusNearFunct1
       473B  _ZoomInFunct2
       4759  _ZoomOutFunct2
       4784  _FocusNearFunct2
       47AF  _FocusFarFunct2
       47CD  _ExtendLA
       47F8  _RetractLA
       4823  _startGhost
       483A  _ExitMenu
       4842  _RestoreDefaults
       4869  _ArrayVarFunction
       4891  _Save_Camera_Add1
       48AB  _Save_Camera_Add2
       48C5  _Save_Serial_Num
       48DF  _MoveLA
       4B03  _ATDGetLevel
       4B21  _SecondCoatSeq
       4E6E  _FirstCoatSeq
       5191  _throwGhost
       55D4  _strcpy
       55E3  lsl16
       55ED  fp2int
       5600  fp2long
       567F  int2fp
       569D  uint2fp
       56B0  ulong2fp
       56C3  long2fp
       5722  cmp4
       5741  add4
       5756  sub4
       576B  and4
       5782  or4
       5799  xor4
       57B0  com4
       57B9  lsl4
       57B9  asl4
       57CB  asr4
       57DD  lsr4
       57EF  mul4
       5828  div4s
       5835  div4u
       583D  mod4s
       5843  mod4u
       5855  divmod4s
       585D  divmod4u
       59B7  int2long
       59CA  lret
       59D5  lret_paged
       59E8  subf4
       59EF  addf4
       5A9D  cmpf4
       5AD6  divf4
       5BA0  mulf4
       5C72  __fp_is_zero
       5C79  __unpack_u
       5C9D  __unpack_s
       5CB6  __pack
       5CDE  __normalize
       5D23  __fp_neg
       5D2F  negf4
       5D36  __HC12Setup
       5D3B  __ctype
       5E3C  __sdepi
       5EAC  _atof
       60FD  _powi
       613C  _isdigit
       6154  _islower
       616C  _isspace
       6184  _toupper
       619E  __text_end

Area                               Addr   Size   Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                           idata   D000   10C0 =   4288. bytes (rel,con,rom)

       Addr  Global Symbol
      -----  --------------------------------
       D000  __idata_start
       E0C0  __idata_end

Area                               Addr   Size   Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                            data   2000   10C0 =   4288. bytes (rel,con,rom)

       Addr  Global Symbol
      -----  --------------------------------
       2000  _No2WireCnt
       2000  __data_start
       2002  _gTPDONr
       2003  _clearProc
       2004  _gTimCnt
       2006  _gCANFilter
       2007  _setFilters
       200F  _Seconds
       2010  _StateTime
       2014  _Tick
       2016  _MenuTimer
       2018  _fast_inc
       2019  _test
       201A  _save_serial_flag
       201B  _CurrentLight
       201C  _State
       201D  _CameraSpeed
       201E  _Store_Flag
       201F  _LightLevel1
       2041  _LightLevel2
       2063  _PumpSpd
       2085  _HeadSpd
       20A7  _CameraSpd
       20C9  _CameraTrip
       20EB  _CameraTurnTime
       210D  _MaxLADist
       212F  _MotorPol
       2151  _TYPE
       2173  _LA_TYPE
       2195  _SecondStrokes
       21B7  _FirstStrokes
       21D9  _CleanOutStrokes
       21FB  _RetractTime
       221D  _HtrISOOnOff
       223F  _HtrISOSetPnt
       2261  _HtrBaseOnOff
       2283  _HtrBaseSetPnt
       22A5  _HtrHoseOnOff
       22C7  _HtrHoseSetPnt
       22E9  _PGainISO
       230B  _IGainISO
       232D  _IMaxISO
       234F  _PGainBase
       2371  _IGainBase
       2393  _IMaxBase
       23B5  _MachineSize
       23D7  _FirstStrokePmpSpd
       252B  _FirstStrokeLASpd
       267F  _FirstStrokeLen
       27D3  _FirstStrokeCtr
       2927  _SecondStrokePmpSpd
       2A7B  _SecondStrokeLASpd
       2BCF  _SecondStrokeLen
       2D23  _SecondStrokeCtr
       2E77  _disp_add1
       2E99  _disp_add2
       2EBB  _ZoomSpeed1
       2EDD  _FocusSpeed1
       2EFF  _ZoomSpeed2
       2F21  _FocusSpeed2
       2F43  _AdvanceTime
       2F65  _CamTag1
       2F87  _CamTag2
       2FA9  _SerialNum
       2FCB  _PumpOnOff
       2FED  _HeadOnOff
       300F  _Rev
       3031  _HtrISOTemp
       3053  _HtrBaseTemp
       3075  _MenuStackc
       3093  _Stop_flag
       3094  _inspectPIDChange
       3096  _usePID
       3097  _PrevCamPos
       3099  _inspect_cam_PID
       30B5  _min_dt
       30B9  _inspectPIDStrength
       30BD  _inspectCamDirection
       30C0  __data_end

Area                               Addr   Size   Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                             bss   30C0   032B =    811. bytes (rel,con,rom)

       Addr  Global Symbol
      -----  --------------------------------
       30C0  _gTxMonitor
       30C0  __bss_start
       30CC  _gTxNMT
       30D8  _gTxSDO
       30E4  _gRxCAN
       30F0  _gRPDOConfig
       3110  _gTPDOConfig
       31C0  _gMCOConfig
       31D5  _RamCkSum
       31D6  _RamData
       31D7  _RamAddress
       31D9  _StartAddress
       31DB  _gProcImg
       3212  _IncSpeedUpCntr
       3213  _IncSpeedUpTimer
       3215  _Temp_LAMovingTimer
       3219  _LAMovingTimer
       321D  _LAMoveTimer
       321F  _HeadSpeed
       3223  _OldHeadTime
       3225  _HeadTime
       3227  _CamHome
       3228  _CamPosition
       322A  _CamPIDTimer
       322C  _CamStopTimer
       322E  _CamTurnTimer
       3230  _BlowerTimer
       3232  _CamIndexTimer
       3234  _HeadRampTimer
       3236  _Update_Menu_Timer
       3238  _HdSpeedTimer
       323A  _AdvanceTimer
       323C  _FocusZoomTimer
       323E  _Timer2
       3240  _Timer1
       3242  _Gen_Flags
       3243  _negTick
       3245  _posTick
       3247  _TC0_RCVD_Data
       3249  _currentDir
       324A  _prevDir
       324B  _StrokePos
       324C  _StrokeFlag
       324D  _VariableFlag
       324E  _Menu
       334A  _StackPointer
       334B  _cam_addx2
       334D  _cam_addx1
       334F  _NullVar
       3371  _Stop_Detected
       3372  _Sense_Detected
       3373  _Sense_Direction
       3375  _Rotate
       3377  _ghostState
       3378  _RDR_Ratio
       337C  _CamAddressXmitd
       337D  _SelectFlag
       337E  _CursorUpFlag
       337F  _CursorDownFlag
       3380  _ran_num
       3382  _cam_add2
       3384  _cam_add1
       3386  _StartPumpFlag
       3388  _StrokeNum
       338A  _HeaterTimer
       338C  _lightval
       338D  _iHeadSpd
       338E  _HeadSpdOut
       338F  _MoveCmdXmtd
       3390  _CamDegTimer
       3392  _CamDegXmtd
       3394  _OldPumpSpeed
       3396  _DispDegree
       3397  _CamDegree
       3399  _LAPos
       339D  _Cycle_Complete
       339E  _UpdateMenu
       339F  _gTxMsg
       33AB  _UpdateArrayVar
       33AC  _Multi_Var_ptr
       33AD  _String_Var_ptr
       33AE  _Variable_flag
       33AF  _Cam_speed
       33B3  _pressed_first
       33B4  _odd_even_counter
       33B5  _OddStroke
       33B6  _LAError
       33B7  _PrevCamPWM
       33B8  _CurrentCamPWM
       33B9  _min_effective_pwm
       33BA  _curr_PWM
       33BB  _PrevCamPIDTime
       33BD  _SenseLevel
       33EB  __bss_end

Area   [ Extended Code Page ]      Addr   Size   Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                         extcode   E0000   71AA =  29098. bytes (rel,con,rom)

CPU ExtAddr  Global Symbol
      -----  --------------------------------
      38:8000 $_EEInit
      38:800B $_EEWrite
      38:81B8 $_MCO_Search_OD
      38:822D $_MCO_Send_SDO_Abort
      38:827F $_MCO_Handle_SDO_Request
      38:8579 $_MCO_Prepare_TPDOs
      38:865A $_MCO_TransmitPDO
      38:8725 $_MCO_Init
      38:87E8 $_MCO_InitRPDO
      38:88AD $_MCO_InitTPDO
      38:89C7 $_MCO_ProcessStack
      38:8DD5 $_Reset_Max33011
      38:8E4B $_MCOUSER_FatalError
      38:8E53 $_MCOUSER_ResetApplication
      38:8E5B $_MCOUSER_ResetCommunication
      38:9013 $_set_screener_std
      38:90F7 $_MCOHW_PullMessage
      38:91AB $_MCOHW_PushMessage
      38:9271 $_MCOHW_GetTime
      38:9281 $_MCOHW_IsTimeExpired
      38:92BF $_MCOHW_TimerISR
      38:92C7 $_MCOHW_Init
      38:935A $_MCOHW_SetCANFilter
      38:93D9 $_InitPorts
      38:9444 $_InitInterrupts
      38:9477 $_InitPLL
      38:948B $_PWMInit
      38:94EA $_AtoDInit
      38:9502 $_getVariable
      38:9586 $_StdVarFunc_String
      38:9600 $_Next_Char_In_String_Var
      38:963D $_FindMenu
      38:96F9 $_next_variable
      38:978B $_getstrval
      38:9A23 $_getvalue
      38:9BEF $_incvar
      38:9CDB $_decvar
      38:9DC7 $_CursorUp
      38:9E34 $_CursorDown
      38:9EA1 $_IncVariable
      38:9ECB $_DecVariable
      38:9EF5 $_CkCntrLength
      38:A03C $_Select
      38:A35F $_DeSelect
      38:A440 $_LoadMenu
      38:AC3C $_InsertCursor
      38:AC73 $_ClearTitler
      38:ACC8 $_DisplayTitler
      38:ADFE $_Display
      38:AF05 $_PositionDisplay
      38:AF58 $_Load_Variables
      38:B0BB $_Save_Variables
      38:B1D6 $_Load_Camera_Add
      38:B221 $_UpdateArrayVariables
      38:B2B8 $_ResetProc
      38:B2C0 $_InitCANOpen
      38:B2C9 $__print
      38:BA66 $_atoi
      38:BAF0 $_itoa
      38:BC2C $_ltoa
      38:BDCA $_ultoa
      38:BEAE $_utoa
      38:BF67 $_isdigit
      38:BF7F $_islower
      38:BF97 $_isspace
      38:BFAF $_toupper
      38:BFCA $_sprintf
      39:8000 $_Load_Serial_Num
      39:8071 $_CleanCoatSeq
      39:82DA $_doevents
      39:9925 $_CameraMain1
      39:9C3F $_CameraMain2
      39:9F62 $_GetRotationalSpeed
      39:9FA2 $_PID_Loop
      39:A0E7 $_ftoa
      39:A420 $_fabsf
      39:A485 $__FormatFP_1
      39:AEA6 $_abs
      39:AEC3 $_srand
      39:AECC $_rand
      39:AEE5 $_memcmp
      39:AF42 $_memcpy
      39:AF86 $_strcmp
      39:AFD7 $_strcspn
      39:B022 $_strlen
      39:B047 $_strncpy
      39:B0B2 $_strpbrk
      39:B0DB $_strspn
      39:B13F $_strtok
      39:B1B6 __extcode_end

Area                               Start  End    Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                          memory   6200   10000 =  65536. bytes (abs,ovr,rom)

       Addr  Global Symbol
      -----  --------------------------------
       6200  _Menuc
       FF80  _interrupt_vectors

Files Linked      [ module(s) ]

C:\iccv712\lib\crt12initrm.o	[ crt12initrm.s ]
Controller.o	[ Controller.c ]
EEProm.o	[ EEProm.c ]
mco.o	[ mco.c ]
user.o	[ user.c ]
mcohw.o	[ mcohw.c ]
Interrupts.o	[ Interrupts.c ]
Subroutines.o	[ Subroutines.c ]
Subroutines1.o	[ Subroutines1.c ]
<library>	[ _printf.s, strcpy.s, lsl16.s, fp2long.s, long2fp.s, longcmp.s, ladd.s, lbits.s, lshift.s, lmul.s, ldiv.s ]
[ int2long.s, lret.s, lret_paged.s, fpadd.s, fpcmp.s, fpdiv.s, fpmul.s, fputil.s, setup.s, atoi.s, itoa.s ]
[ ltoa.s, ultoa.s, utoa.s, ftoa.s, isdigit.s, islower.s, isspace.s, ctype.s, toupper.s, fabsf.s, sprintf.s ]
[ formatfp1.s, abs.s, rand.s, memcmp.s, memcpy.s, strcmp.s, strcspn.s, strlen.s, strncpy.s, strpbrk.s ]
[ strspn.s, strtok.s, atof.s, isdigit.s, islower.s, isspace.s, toupper.s ]

User Global Definitions

init_sp = 0x4000
initrm = 0x21

Area Definitions

-btext:0x4000.0x7fff:0xc000.0xffff
-bdata:0x2000.0xffff
-bextcode:0xe0000.0xfffff
-bidata:0xd000.0xffff

