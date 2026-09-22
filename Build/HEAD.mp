Project:      HEAD
Build Number: 1610
Date:         Mon Sep 21 16:06:01 2026
Compiler:     ICC12
Version:      7.50


Area                               Addr   Size   Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                            text   4000   3A6B =  14955. bytes (rel,con,rom)

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
       4580  _Menuc
       5C95  _NullFunction
       5C99  _BlowerOn
       5CA3  _CamIndex
       5CAD  _StdVarFunction
       5D97  _Advance
       5DBE  _ZoomInFunct1
       5DDC  _ZoomOutFunct1
       5E07  _FocusFarFunct1
       5E25  _FocusNearFunct1
       5E50  _ZoomInFunct2
       5E6E  _ZoomOutFunct2
       5E99  _FocusNearFunct2
       5EC4  _FocusFarFunct2
       5EE2  _ExtendLA
       5F0D  _RetractLA
       5F38  _startGhost
       5F4F  _ExitMenu
       5F57  _RestoreDefaults
       5F7E  _ArrayVarFunction
       5FA6  _Save_Camera_Add1
       5FC0  _Save_Camera_Add2
       5FDA  _Save_Serial_Num
       5FF4  _MoveLA
       6218  _ATDGetLevel
       6236  _SecondCoatSeq
       6555  _FirstCoatSeq
       6848  _throwGhost
       6C3B  _token_chars
       6EA1  _strcpy
       6EB0  lsl16
       6EBA  fp2int
       6ECD  fp2long
       6F4C  int2fp
       6F6A  uint2fp
       6F7D  ulong2fp
       6F90  long2fp
       6FEF  cmp4
       700E  add4
       7023  sub4
       7038  and4
       704F  or4
       7066  xor4
       707D  com4
       7086  lsl4
       7086  asl4
       7098  asr4
       70AA  lsr4
       70BC  mul4
       70F5  div4s
       7102  div4u
       710A  mod4s
       7110  mod4u
       7122  divmod4s
       712A  divmod4u
       7284  int2long
       7297  lret
       72A2  lret_paged
       72B5  subf4
       72BC  addf4
       736A  cmpf4
       73A3  divf4
       746D  mulf4
       753F  __fp_is_zero
       7546  __unpack_u
       756A  __unpack_s
       7583  __pack
       75AB  __normalize
       75F0  __fp_neg
       75FC  negf4
       7603  __HC12Setup
       7608  __ctype
       7709  __sdepi
       7779  _atof
       79CA  _powi
       7A09  _isdigit
       7A21  _islower
       7A39  _isspace
       7A51  _toupper
       7A6B  __text_end

Area                               Addr   Size   Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                           idata   D000   125A =   4698. bytes (rel,con,rom)

       Addr  Global Symbol
      -----  --------------------------------
       D000  __idata_start
       E25A  __idata_end

Area                               Addr   Size   Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                            data   1000   125A =   4698. bytes (rel,con,rom)

       Addr  Global Symbol
      -----  --------------------------------
       1000  _No2WireCnt
       1000  __data_start
       1002  _gTPDONr
       1003  _clearProc
       1004  _gTimCnt
       1006  _gCANFilter
       1007  _setFilters
       100F  _Seconds
       1010  _StateTime
       1014  _Tick
       1016  _MenuTimer
       1018  _fast_inc
       1019  _test
       101A  _save_serial_flag
       101B  _CurrentLight
       101C  _State
       101D  _CameraSpeed
       101E  _Store_Flag
       101F  _LightLevel1
       1043  _LightLevel2
       1067  _PumpSpd
       108B  _HeadSpd
       10AF  _CameraSpd
       10D3  _CameraTrip
       10F7  _CameraTurnTime
       111B  _MaxLADist
       113F  _MotorPol
       1163  _TYPE
       1187  _LA_TYPE
       11AB  _SecondStrokes
       11CF  _FirstStrokes
       11F3  _CleanOutStrokes
       1217  _RetractTime
       123B  _HtrISOOnOff
       125F  _HtrISOSetPnt
       1283  _HtrBaseOnOff
       12A7  _HtrBaseSetPnt
       12CB  _HtrHoseOnOff
       12EF  _HtrHoseSetPnt
       1313  _PGainISO
       1337  _IGainISO
       135B  _IMaxISO
       137F  _PGainBase
       13A3  _IGainBase
       13C7  _IMaxBase
       13EB  _MachineSize
       140F  _FirstStrokePmpSpd
       1577  _FirstStrokeLASpd
       16DF  _FirstStrokeLen
       1847  _FirstStrokeCtr
       19AF  _SecondStrokePmpSpd
       1B17  _SecondStrokeLASpd
       1C7F  _SecondStrokeLen
       1DE7  _SecondStrokeCtr
       1F4F  _disp_add1
       1F73  _disp_add2
       1F97  _ZoomSpeed1
       1FBB  _FocusSpeed1
       1FDF  _ZoomSpeed2
       2003  _FocusSpeed2
       2027  _AdvanceTime
       204B  _CamTag1
       206F  _CamTag2
       2093  _SerialNum
       20B7  _PumpOnOff
       20DB  _HeadOnOff
       20FF  _Rev
       2123  _HtrISOTemp
       2147  _HtrBaseTemp
       216B  _MenuStackc
       2189  _Stop_flag
       218A  _inspectPIDChange
       218C  _usePID
       218D  _PrevCamPos
       218F  _inspect_cam_PID
       21AB  _min_dt
       21AF  _inspectPIDStrength
       21B3  _inspectCamDirection
       21B4  _total_raw_bytes_sent_by_sendPackets
       21B8  MenuSerialize.c:current_buffer_fill
       21BA  _handle_buffer_fail
       21BC  _enum_dict
       21DA  _enum_dict_counter
       21DC  _menu_context
       21E1  MenuSerialize.c:int_to_str_0_50
       2247  _token_dictionary_ptr
       2249  _myTimer
       224B  _rxbuffer_len
       224D  _receive_error
       224F  Packets.c:receiving
       2250  Packets.c:nextSeq
       2251  Packets.c:totalBytes
       2253  Packets.c:prev_ls_bits
       2254  _last_sent_lsb
       2256  _delayFlag
       225A  __data_end

Area                               Addr   Size   Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                             bss   225A   0391 =    913. bytes (rel,con,rom)

       Addr  Global Symbol
      -----  --------------------------------
       225A  _gTxMonitor
       225A  __bss_start
       2266  _gTxNMT
       2272  _gTxSDO
       227E  _gRxCAN
       228A  _gRPDOConfig
       22AA  _gTPDOConfig
       235A  _gMCOConfig
       236F  _RamCkSum
       2370  _RamData
       2371  _RamAddress
       2373  _StartAddress
       2375  _gProcImg
       23B4  _IncSpeedUpCntr
       23B5  _IncSpeedUpTimer
       23B7  _Temp_LAMovingTimer
       23BB  _LAMovingTimer
       23BF  _LAMoveTimer
       23C1  _HeadSpeed
       23C5  _OldHeadTime
       23C7  _HeadTime
       23C9  _CamTurnTimer
       23CB  _CamHome
       23CC  _CamPosition
       23CE  _CamPIDTimer
       23D0  _CamStopTimer
       23D2  _BlowerTimer
       23D4  _CamIndexTimer
       23D6  _HeadRampTimer
       23D8  _Update_Menu_Timer
       23DA  _HdSpeedTimer
       23DC  _AdvanceTimer
       23DE  _FocusZoomTimer
       23E0  _Timer2
       23E2  _Timer1
       23E4  _Gen_Flags
       23E5  _negTick
       23E7  _posTick
       23E9  _TC0_RCVD_Data
       23EB  _currentDir
       23EC  _prevDir
       23ED  _StrokePos
       23EE  _StrokeFlag
       23EF  _VariableFlag
       23F0  _Menu
       24EC  _StackPointer
       24ED  _cam_addx2
       24EF  _cam_addx1
       24F1  _Stop_Detected
       24F2  _Sense_Detected
       24F3  _Sense_Direction
       24F5  _Rotate
       24F7  _ghostState
       24F8  _RDR_Ratio
       24FC  _CamAddressXmitd
       24FD  _SelectFlag
       24FE  _CursorUpFlag
       24FF  _CursorDownFlag
       2500  _ran_num
       2502  _cam_add2
       2504  _cam_add1
       2506  _StartPumpFlag
       2508  _StrokeNum
       250A  _HeaterTimer
       250C  _lightval
       250D  _iHeadSpd
       250E  _HeadSpdOut
       250F  _MoveCmdXmtd
       2510  _CamDegTimer
       2512  _CamDegXmtd
       2514  _OldPumpSpeed
       2516  _DispDegree
       2517  _CamDegree
       2519  _LAPos
       251D  _Cycle_Complete
       251E  _gTxMsg
       252A  _UpdateArrayVar
       252B  _Multi_Var_ptr
       252C  _String_Var_ptr
       252D  _Variable_flag
       252E  _NullVar
       2552  _UpdateMenu
       2553  _Cam_speed
       2557  _pressed_first
       2558  _odd_even_counter
       2559  _OddStroke
       255A  _LAError
       255B  _PrevCamPWM
       255C  _CurrentCamPWM
       255D  _min_effective_pwm
       255E  _curr_PWM
       255F  _PrevCamPIDTime
       2561  _SenseLevel
       2563  MenuSerialize.c:G_packet_buffer
       259F  _rxbuffer
       25EB  __bss_end

Area   [ Extended Code Page ]      Addr   Size   Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                         extcode   E0000   9D7E =  40318. bytes (rel,con,rom)

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
      38:95FA $_Next_Char_In_String_Var
      38:9637 $_FindMenu
      38:96F3 $_next_variable
      38:9785 $_getstrval
      38:9A1D $_getvalue
      38:9BE9 $_incvar
      38:9CD5 $_decvar
      38:9DC1 $_CursorUp
      38:9E2E $_CursorDown
      38:9E9B $_IncVariable
      38:9ECD $_DecVariable
      38:9EFF $_CkCntrLength
      38:A046 $_Select
      38:A369 $_DeSelect
      38:A449 $_LoadMenu
      38:AC45 $_InsertCursor
      38:AC7C $_ClearTitler
      38:ACD1 $_DisplayTitler
      38:AE06 $_Display
      38:AF0D $_PositionDisplay
      38:AF60 $_Load_Variables
      38:B0C3 $_Save_Variables
      38:B1DE $_Load_Camera_Add
      38:B229 $_UpdateArrayVariables
      38:B2A4 $_ResetProc
      38:B2AC $_InitCANOpen
      38:B2B5 Packets.c:calculate_xor_checksum
      38:B2DE $_receivePackets
      38:B622 $_sendPacket
      38:B696 $_sendPackets
      38:B7C1 $_MCO_ProcessStack_Menu
      38:B7FA $__print
      38:BF97 $_isdigit
      38:BFAF $_islower
      38:BFC7 $_isspace
      38:BFDF $_toupper
      39:8000 $_Load_Serial_Num
      39:8071 $_CleanCoatSeq
      39:82DA $_doevents
      39:98D4 $_CameraMain1
      39:9BFC $_CameraMain2
      39:9F2D $_GetRotationalSpeed
      39:9F6D $_PID_Loop
      39:A0B2 $_update_menu_var_by_value
      39:A13E $_update_menu_var_by_str
      39:A1A2 $_SkipVarFunction
      39:A21E $_atoi
      39:A2A8 $_itoa
      39:A3E4 $_ltoa
      39:A582 $_ultoa
      39:A666 $_utoa
      39:A71F $_ftoa
      39:AA58 $_fabsf
      39:AA94 $_sprintf
      39:AAE7 $__FormatFP_1
      39:B508 $_abs
      39:B525 $_srand
      39:B52E $_rand
      39:B547 $_memcmp
      39:B5A4 $_memcpy
      39:B5E8 $_memmove
      39:B668 $_strcmp
      39:B6B9 $_strcspn
      39:B704 $_strlen
      39:B729 $_strncmp
      39:B792 $_strncpy
      39:B7FD $_strpbrk
      39:B826 $_strspn
      39:B88A $_strtok
      3A:8000 $_itoa_0_to_50
      3A:8038 $_clear_packet_buffer
      3A:805C $_sendBufferedPackets
      3A:8145 $_trim_spaces
      3A:81D1 $_finalize_menu_text
      3A:8390 $_is_function_ignored
      3A:83CB $_send_section_start
      3A:83E6 $_send_section_end
      3A:83FC $_send_marked_string
      3A:848C $_finalize_and_tokenize_string
      3A:868C $_are_var_archetypes_equal
      3A:875A $_is_plausible_header_line
      3A:87E5 $_get_nth_word
      3A:890D $_get_variable_chain_depth
      3A:8940 $_generate_descriptive_var_name
      3A:8C87 $_process_token_dictionary
      3A:8CBF $_process_menu_titles
      3A:8EB0 $_process_enum_dict
      3A:90AF $_process_func_dict
      3A:9257 $_get_enum_dict_idx
      3A:92D2 $_print_g_value
      3A:930F $_process_var_arch_dict
      3A:9520 $_reset_var_processed_flags
      3A:958F $_process_var_instance_data
      3A:96FF $_assign_archetype_index_to_matching_variables
      3A:9792 $_reset_all_archetype_indices
      3A:9801 $_send_serialized_menu
      3A:9914 $_update_value_gui
      3A:9D9B $_find_menu_variable_index
      3A:9F33 $_scan_menu_variables
      3A:A0CB $_scan_menu_functions
      3A:A1D7 $_handle_function
      3A:A1FB $_handle_str
      3A:A307 $_handle_var
      3A:A3AA $_handle_buffer
      3A:A483 __extcode_end

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
MenuSerialize.o	[ MenuSerialize.c ]
Packets.o	[ Packets.c ]
<library>	[ _printf.s, strcpy.s, lsl16.s, fp2long.s, long2fp.s, longcmp.s, ladd.s, lbits.s, lshift.s, lmul.s, ldiv.s ]
[ int2long.s, lret.s, lret_paged.s, fpadd.s, fpcmp.s, fpdiv.s, fpmul.s, fputil.s, setup.s, atoi.s, itoa.s ]
[ ltoa.s, ultoa.s, utoa.s, ftoa.s, isdigit.s, islower.s, isspace.s, ctype.s, toupper.s, fabsf.s, sprintf.s ]
[ formatfp1.s, abs.s, rand.s, memcmp.s, memcpy.s, memmove.s, strcmp.s, strcspn.s, strlen.s, strncmp.s ]
[ strncpy.s, strpbrk.s, strspn.s, strtok.s, atof.s, isdigit.s, islower.s, isspace.s, toupper.s ]

User Global Definitions

init_sp = 0x4000
initrm = 0x21

Area Definitions

-btext:0x4000.0x7fff:0xc000.0xffff
-bdata:0x1000.0xffff
-bextcode:0xe0000.0xfffff
-bidata:0xd000.0xffff

