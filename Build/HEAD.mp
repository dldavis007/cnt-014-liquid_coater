Project:      HEAD
Build Number: 1604
Date:         Mon Oct 05 11:43:03 2026
Compiler:     ICC12
Version:      7.50


Area                               Addr   Size   Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                            text   4000   392A =  14634. bytes (rel,con,rom)

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
       4580  _enum_internal_external_cameras
       4592  _enum_enabled_disabled_cameras
       45A4  _Menuc
       5CB9  _NullFunction
       5CBD  _BlowerOn
       5CC7  _CamIndex
       5CD1  _StdVarFunction
       5DBB  _Advance
       5DE2  _ZoomInFunct1
       5E00  _ZoomOutFunct1
       5E2B  _FocusFarFunct1
       5E49  _FocusNearFunct1
       5E74  _ZoomInFunct2
       5E92  _ZoomOutFunct2
       5EBD  _FocusNearFunct2
       5EE8  _FocusFarFunct2
       5F06  _ExtendLA
       5F31  _RetractLA
       5F5C  _startGhost
       5F73  _ExitMenu
       5F7B  _RestoreDefaults
       5FA2  _ArrayVarFunction
       5FCA  _Save_Camera_Add1
       5FE4  _Save_Camera_Add2
       5FFE  _Save_Serial_Num
       6018  _MoveLA
       623C  _ATDGetLevel
       625A  _SecondCoatSeq
       65A7  _FirstCoatSeq
       68CA  _throwGhost
       6D60  _strcpy
       6D6F  lsl16
       6D79  fp2int
       6D8C  fp2long
       6E0B  int2fp
       6E29  uint2fp
       6E3C  ulong2fp
       6E4F  long2fp
       6EAE  cmp4
       6ECD  add4
       6EE2  sub4
       6EF7  and4
       6F0E  or4
       6F25  xor4
       6F3C  com4
       6F45  asl4
       6F45  lsl4
       6F57  asr4
       6F69  lsr4
       6F7B  mul4
       6FB4  div4s
       6FC1  div4u
       6FC9  mod4s
       6FCF  mod4u
       6FE1  divmod4s
       6FE9  divmod4u
       7143  int2long
       7156  lret
       7161  lret_paged
       7174  subf4
       717B  addf4
       7229  cmpf4
       7262  divf4
       732C  mulf4
       73FE  __fp_is_zero
       7405  __unpack_u
       7429  __unpack_s
       7442  __pack
       746A  __normalize
       74AF  __fp_neg
       74BB  negf4
       74C2  __HC12Setup
       74C7  __ctype
       75C8  __sdepi
       7638  _atof
       7889  _powi
       78C8  _isdigit
       78E0  _islower
       78F8  _isspace
       7910  _toupper
       792A  __text_end

Area                               Addr   Size   Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                           idata   D000   112B =   4395. bytes (rel,con,rom)

       Addr  Global Symbol
      -----  --------------------------------
       D000  __idata_start
       E12B  __idata_end

Area                               Addr   Size   Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                            data   2000   112B =   4395. bytes (rel,con,rom)

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
       2FA9  _InternalExternalCameraSetting
       2FCB  _Cam1Enable
       2FED  _Cam2Enable
       300F  _SerialNum
       3031  _PumpOnOff
       3053  _HeadOnOff
       3075  _Rev
       3097  _HtrISOTemp
       30B9  _HtrBaseTemp
       30DB  _MenuStackc
       30F9  _Stop_flag
       30FA  _inspectPIDChange
       30FC  _usePID
       30FD  _PrevCamPos
       30FF  _trig_query_sent
       3100  _trig_query_timer
       3102  _inspect_cam_PID
       311E  _min_dt
       3122  _inspectPIDStrength
       3126  _inspectCamDirection
       3127  Subroutines1.c:addr_tx_pending
       3128  Subroutines1.c:addr_tx_pending:1
       312B  __data_end

Area                               Addr   Size   Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                             bss   312B   033B =    827. bytes (rel,con,rom)

       Addr  Global Symbol
      -----  --------------------------------
       312B  __bss_start
       312B  _gTxMonitor
       3137  _gTxNMT
       3143  _gTxSDO
       314F  _gRxCAN
       315B  _gRPDOConfig
       317B  _gTPDOConfig
       322B  _gMCOConfig
       3240  _RamCkSum
       3241  _RamData
       3242  _RamAddress
       3244  _StartAddress
       3246  _gProcImg
       3285  _IncSpeedUpCntr
       3286  _IncSpeedUpTimer
       3288  _Temp_LAMovingTimer
       328C  _LAMovingTimer
       3290  _LAMoveTimer
       3292  _HeadSpeed
       3296  _OldHeadTime
       3298  _HeadTime
       329A  _CamHome
       329B  _CamPosition
       329D  _CamPIDTimer
       329F  _CamStopTimer
       32A1  _CamTurnTimer
       32A3  _BlowerTimer
       32A5  _CamIndexTimer
       32A7  _HeadRampTimer
       32A9  _Update_Menu_Timer
       32AB  _HdSpeedTimer
       32AD  _AdvanceTimer
       32AF  _FocusZoomTimer
       32B1  _Timer2
       32B3  _Timer1
       32B5  _Gen_Flags
       32B6  _negTick
       32B8  _posTick
       32BA  _TC0_RCVD_Data
       32BC  _currentDir
       32BD  _prevDir
       32BE  _StrokePos
       32BF  _StrokeFlag
       32C0  _VariableFlag
       32C1  _Menu
       33BD  _StackPointer
       33BE  _cam_addx2
       33C0  _cam_addx1
       33C2  _NullVar
       33E4  _Stop_Detected
       33E5  _Sense_Detected
       33E6  _Sense_Direction
       33E8  _Rotate
       33EA  _ghostState
       33EB  _RDR_Ratio
       33EF  _CamAddressXmitd
       33F0  _SelectFlag
       33F1  _CursorUpFlag
       33F2  _CursorDownFlag
       33F3  _ran_num
       33F5  _cam_add2
       33F7  _cam_add1
       33F9  _StartPumpFlag
       33FB  _StrokeNum
       33FD  _HeaterTimer
       33FF  _lightval
       3400  _iHeadSpd
       3401  _HeadSpdOut
       3402  _MoveCmdXmtd
       3403  _CamDegTimer
       3405  _CamDegXmtd
       3407  _OldPumpSpeed
       3409  _DispDegree
       340A  _CamDegree
       340C  _LAPos
       3410  _Cycle_Complete
       3411  _UpdateMenu
       3412  _gTxMsg
       341E  _UpdateArrayVar
       341F  _Multi_Var_ptr
       3420  _String_Var_ptr
       3421  _Variable_flag
       3422  Subroutines1.c:addr_tx_time
       3424  Subroutines1.c:addr_tx_time:1
       3426  _Cam_speed
       342A  _pressed_first
       342B  _odd_even_counter
       342C  _OddStroke
       342D  _LAError
       342E  _paired_camera_address
       3430  _PrevCamPWM
       3431  _CurrentCamPWM
       3432  _min_effective_pwm
       3433  _curr_PWM
       3434  _PrevCamPIDTime
       3436  _activeCamAddress
       3438  _SenseLevel
       3466  __bss_end

Area   [ Extended Code Page ]      Addr   Size   Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                         extcode   E0000   742A =  29738. bytes (rel,con,rom)

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
      38:9029 $_set_screener_std
      38:910D $_MCOHW_PullMessage
      38:91C1 $_MCOHW_PushMessage
      38:9287 $_MCOHW_GetTime
      38:9297 $_MCOHW_IsTimeExpired
      38:92D5 $_MCOHW_TimerISR
      38:92DD $_MCOHW_Init
      38:9370 $_MCOHW_SetCANFilter
      38:93EF $_InitPorts
      38:945A $_InitInterrupts
      38:948D $_InitPLL
      38:94A1 $_PWMInit
      38:9500 $_AtoDInit
      38:9518 $_getVariable
      38:959C $_StdVarFunc_String
      38:9616 $_Next_Char_In_String_Var
      38:9653 $_FindMenu
      38:970F $_next_variable
      38:97A1 $_getstrval
      38:9A39 $_getvalue
      38:9C05 $_incvar
      38:9CF1 $_decvar
      38:9DDD $_CursorUp
      38:9E4A $_CursorDown
      38:9EB7 $_IncVariable
      38:9EE1 $_DecVariable
      38:9F0B $_CkCntrLength
      38:A052 $_Select
      38:A375 $_DeSelect
      38:A456 $_LoadMenu
      38:AC52 $_InsertCursor
      38:AC89 $_ClearTitler
      38:ACDE $_DisplayTitler
      38:AE14 $_Display
      38:AF1B $_PositionDisplay
      38:AF6E $_Load_Variables
      38:B0D1 $_Save_Variables
      38:B1EC $_Load_Camera_Add
      38:B237 $_UpdateArrayVariables
      38:B2CE $_ResetProc
      38:B2D6 $_InitCANOpen
      38:B2DF $__print
      38:BA7C $_atoi
      38:BB06 $_itoa
      38:BC42 $_ltoa
      38:BDE0 $_ultoa
      38:BEC4 $_utoa
      38:BF7D $_isdigit
      38:BF95 $_islower
      38:BFAD $_isspace
      38:BFC5 $_toupper
      38:BFE0 $_abs
      39:8000 $_Load_Serial_Num
      39:8071 $_CleanCoatSeq
      39:82DA $_internalExternal
      39:82F6 $_TrigRequest
      39:839D $_doevents
      39:9A48 $_update_active_cam_address
      39:9A7E $_poll_paired_camera_address
      39:9AE2 $_CameraMain1
      39:9E3C $_CameraMain2
      39:A196 $_clear_camera_commands
      39:A19A $_GetRotationalSpeed
      39:A1DA $_PID_Loop
      39:A31F $_ftoa
      39:A658 $_fabsf
      39:A694 $_sprintf
      39:A6E7 $__FormatFP_1
      39:B108 $_srand
      39:B111 $_rand
      39:B12A $_memcmp
      39:B187 $_memcpy
      39:B1CB $_memset
      39:B1FD $_strcmp
      39:B24E $_strcspn
      39:B299 $_strlen
      39:B2BE $_strncpy
      39:B329 $_strpbrk
      39:B352 $_strspn
      39:B3B6 $_strtok
      39:B42D __extcode_end

Area                               Start  End    Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                          memory   FF80   10000 =  65536. bytes (abs,ovr,rom)

       Addr  Global Symbol
      -----  --------------------------------
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
[ formatfp1.s, abs.s, rand.s, memcmp.s, memcpy.s, memset.s, strcmp.s, strcspn.s, strlen.s, strncpy.s ]
[ strpbrk.s, strspn.s, strtok.s, atof.s, isdigit.s, islower.s, isspace.s, toupper.s ]

User Global Definitions

init_sp = 0x4000
initrm = 0x21

Area Definitions

-btext:0x4000.0x7fff:0xc000.0xffff
-bdata:0x2000.0xffff
-bextcode:0xe0000.0xfffff
-bidata:0xd000.0xffff

