#ifndef DATA_DEFINE_H
#define DATA_DEFINE_H

enum AxisID
{
    X = 4,
    Y = 2,
    Z = 0
};

enum AxisPresetPos
{
    Standard_1 = 0,
    Standard_2,
    Standard_3,
    Standard_4,
    Loading,
    Waitting,
    Wid
};

enum CommandID
{
    CMD_SVON_ID = 6001,
    CMD_HOME_ID,
    CMD_SMCHOME_ID,
    CMD_ABS_ID,
    CMD_REL_ID,
    CMD_VEL_ID,
    CMD_LINEXY_ID,
    CMD_HALT_ID,
    CMD_HALTLINEXY_ID,
    CMD_CLR_ID,
    CMD_SSP_ID,
    CMD_SAD_ID,
    CMD_SDL_ID,
    CMD_OUTP_ID,
    CMD_GETINP_ID,
    CMD_HOMESTATUS_X_ID,
    CMD_HOMESTATUS_Y_ID,
    CMD_HOMESTATUS_Z_ID,
    CMD_GETOUTP_ID,
    CMD_RCP_X_ID,
    CMD_RCP_Y_ID,
    CMD_RCP_Z_ID,
    CMD_RDP_ID,
    CMD_RSP_ID,
    CMD_RAD_ID,
    CMD_RDL_ID,
    CMD_STATUS_X_ID,
    CMD_STATUS_Y_ID,
    CMD_STATUS_Z_ID,
    CMD_ERRSTATUS_X_ID,
    CMD_ERRSTATUS_Y_ID,
    CMD_ERRSTATUS_Z_ID,
    CMD_SETPCOM1PARAM_ID,
    CMD_CTRLPCOM1_ID,
    CMD_CTRLAUTOSND_START_ID,
    CMD_CTRLAUTOSND_STOP_ID
};

#define Template_File_Head_Row_Count    14       // 模板文件模板头行数

// 指令宏定义（格式字符串）
#define CMD_SVON       "#svon %1,%2,%3"       // 使能
#define CMD_HOME       "#home %1,%2"          // 驱动器回零
#define CMD_SMCHOME    "#smchome %1,%2,%3,%4,%5" // SMC回零
#define CMD_ABS        "#abs %1,%2,%3"         // 绝对运动
#define CMD_REL        "#rel %1,%2,%3"         // 相对运动
#define CMD_VEL        "#vel %1,%2,%3"         // 连续运动
#define CMD_LINEXY     "#linexy %1,%2,%3,%4,%5,%6"      // 插补XY轴
#define CMD_HALT       "#halt %1,%2"           // 停止运动
#define CMD_HALTLINEXY "#haltlinexy %1"        //停止线性运动
#define CMD_CLR        "#clr %1,%2"            // 清除报警
#define CMD_SSP        "#ssp %1,%2,%3"         // 设置速度
#define CMD_SAD        "#sad %1,%2,%3"         // 设置加速度
#define CMD_SDL        "#sdl %1,%2,%3"         // 设置减速度
#define CMD_OUTP       "#outp %1,%2,%3"        // 设置输出信号
#define CMD_GETINP     "#getinp %1"            // 获取输入信号
#define CMD_HOMESTATUS "#homestatus %1,%2"     // 获取回零状态
#define CMD_GETOUTP    "#getoutp %1"           // 获取输出信号
#define CMD_RCP        "#rcp %1,%2"            // 获取当前位置
#define CMD_RDP        "#rdp %1,%2"            // 获取命令位置
#define CMD_RSP        "#rsp %1,%2"            // 获取速度
#define CMD_RAD        "#rad %1,%2"            // 获取加速度
#define CMD_RDL        "#rdl %1,%2"            // 获取减速度
#define CMD_STATUS     "#status %1,%2"         // 获取轴状态
#define CMD_ERRSTATUS  "#errstatus %1,%2"      // 获取轴错误状态
#define CMD_SETPCOM1PARAM  "#setpcom1param %1,%2,%3,%4,%5,%6,%7"      // 设置PCOM1参数
#define CMD_CTRLPCOM1   "#ctrlpcom1 %1,%2,%3"   //控制PCOM1
#define CMD_CTRLAUTOSND   "#ctrlAutoSnd %1,%2"   //控制自动上传

#define TW_CircleCenter_X  122.25
#define TW_CircleCenter_Y  143.97
#define NJ_CircleCenter_X  142.322
#define NJ_CircleCenter_Y  151.896

#define TW_SOFEDOG_KEY1		28475
#define TW_SOFEDOG_KEY2		19174
#define TW_SOFEDOG_KEY3		20890
#define TW_SOFEDOG_KEY4		9809
#define NJ_SOFEDOG_KEY1		5772
#define NJ_SOFEDOG_KEY2		26935
#define NJ_SOFEDOG_KEY3		6176
#define NJ_SOFEDOG_KEY4		21399

#define ERROR_LIST \
    X(0, "No error")\
    X(1, "Communication error. The fieldbus slave is no longer in state operational.")\
    X(2, "Axis error")\
    X(3, "Fieldbus has lost synchronicity")\
    X(10, "Position outside of permissible range of SWLimit")\
    X(11, "Hardware end switch is active")\
    X(12, "This error occurs if a linear axis has more than 2^15 32-bit overflows of the position in increments")\
    X(13, "Drive status Halt or Quickstop is not supported")\
    X(14, "Drive has no power")\
    X(15, "This error is no longer used")\
    X(16, "Position lag error. Difference between set and current position exceeds the given limit")\
    X(17, "Homing error reported by axis")\
    X(18, "A problem with the license occurred")\
    X(20, "Controller enable not done or brake applied")\
    X(21, "Axis in wrong controller mode")\
    X(25, "Invalid action at logical axis")\
    X(27, "Axis is not operational wCommunicationState <> 100")\
    X(30, "Motion creating module has not been called again before end of the motion")\
    X(31, "Type of given AXIS_REF variable is not AXIS_REF")\
    X(32, "AXIS_REF variable has been exchanged while the module was active.")\
    X(33, "Axis disabled while being moved. MC_Power.bRegulatorOn")\
    X(34, "Axis in its current state cannot execute a motion command, because the axis doesn’t signal currently that it follows the target values.")\
    X(35, "The drive reported an error during an ongoing movement.")\
    X(40, "Maximum velocity fMaxVelocity exceeded")\
    X(41, "Maximum acceleration fMaxAcceleration exceeded")\
    X(42, "Maximum deceleration fMaxDeceleration exceeded")\
    X(50, "Invalid velocity or acceleration values")\
    X(51, "Mode requests for safety reasons use of end switches.")\
    X(60, "No free handle has been sent to open file.")\
    X(65, "Initialization of SMC_MultiAcyclicCommunicator not successful")\
    X(66, "The axis has got an invalid handle")\
    X(67, "Too many tasks with SDO-producing axes")\
    X(68, "Atomic add failed")\
    X(69, "SDO-read lead to invalid datalength (> 4)")\
    X(70, "Mode not supported")\
    X(71, "In current mode, controller mode cannot be changed.")\
    X(72, "SMC_SetControllerMode has been interrupted by MC_Stop or errorstop.")\
    X(75, "Axis not in correct controller mode. Deprecated, no longer returned by SMC_SetTorque.")\
    X(80, "Error at startup of the axis group")\
    X(81, "The axis is not yet in the required state.")\
    X(85, "The function block does not support virtual or logical axes.")\
    X(86, "The number of absolute bits is invalid, must be in the range 8 .. 32.")\
    X(87, "The modulo period is negative, this is not supported.")\
    X(90, "Invalid values")\
    X(91, "Gearing parameters must not be changed as long as the drive is under control.")\
    X(92, "Invalid modulo period (<= 0 or greater than half the bus bandwidth)")\
    X(93, "The modulo period in increments is not an integer, but the modulo-handling is done by the drive.")\
    X(110, "Axis contain no information on cycle time (fTaskCycle = 0).")\
    X(120, "Axis without error")\
    X(121, "Axis does not perform error-reset.")\
    X(122, "Error could not be reset.")\
    X(123, "Communication with the axis did not work.")\
    X(124, "If there is communication error, it cannot be reset")\
    X(130, "Parameter number unknown")\
    X(131, "Error during transmission to the drives. See error number in function block instance ReadDriveParameter.")\
    X(132, "No assignment for drive parameters available")\
    X(133, "Conversion of the value to / from the drive parameters failed.")\
    X(140, "Parameter number unknown or writing not allowed")\
    X(141, "See error number in function block instance WriteDriveParameter")\
    X(142, "No assignment for drive parameters available")\
    X(143, "Conversion of the value to / from the drive parameters failed")\
    X(170, "Axis was not in standstill state.")\
    X(171, "Drive did not start homing.")\
    X(172, "Drive did not reply, when homing was done.")\
    X(173, "Error at stop after homing. Deceleration may not be set.")\
    X(174, "Drive is in errorstop status. Homing cannot be executed.")\
    X(180, "Unknown error at stop")\
    X(181, "Invalid velocity or acceleration values")\
    X(182, "Direction = shortest not applicable")\
    X(183, "Drive is in errorstop status. Stop cannot be executed")\
    X(184, "Instance of MC_Stop blocking the axis by Execute = TRUE has not been called yet. MC_Stop (Execute = FALSE) has to be called.")\
    X(185, "Cannot interrupt an ongoing MC_Stop")\
    X(200, "The task interval of the bus task could not be determined.")\
    X(201, "Invalid velocity or acceleration values")\
    X(202, "Direction error")\
    X(226, "Invalid velocity or acceleration values")\
    X(227, "Direction error")\
    X(251, "Invalid velocity or acceleration values")\
    X(252, "Direction error")\
    X(276, "Invalid velocity or acceleration values")\
    X(277, "Direction error")\
    X(278, "Except from aborting, don’t call a major mvt-FB after SMC_MoveSuperimposed")\
    X(300, "No longer used; only for compatibility")\
    X(301, "Invalid velocity or acceleration values")\
    X(302, "Direction = shortest/fastest not applicable")\
    X(325, "Erroneous array size")\
    X(326, "Step time = t#0s")\
    X(350, "Erroneous array size")\
    X(351, "Step time = t#0s")\
    X(375, "Erroneous array size")\
    X(376, "Step time = t#0s")\
    X(400, "Trigger already active")\
    X(401, "DriveInterface does not support the window function.")\
    X(402, "Communication error")\
    X(410, "Trigger already de-allocated")\
    X(426, "Invalid velocity or acceleration values")\
    X(427, "Invalid direction")\
    X(451, "Invalid velocity or acceleration values")\
    X(452, "Invalid direction")\
    X(453, "Direction = fastest not applicable")\
    X(475, "SMC_ChangeDynamicLimits may only be called in state standstill. or power_off.")\
    X(476, "Invalid velocity, acceleration, deceleration or jerk values")\
    X(600, "Cam does not contain any tappets.")\
    X(601, "Tappet group ID exceeds MAX_NUM_TAPPETS")\
    X(602, "More than 32 accesses on one CAM_REF")\
    X(625, "No cam selected")\
    X(626, "Master axis out of valid range")\
    X(627, "Velocity and acceleration values must be specified for ramp_in function")\
    X(628, "Scaling variables fEditor/TableMasterMin/Max are not correct")\
    X(629, "Too many tappets became active during one cycle")\
    X(640, "Function block for the given cam format is not implemented")\
    X(675, "RatioDenominator = 0")\
    X(676, "Acceleration invalid")\
    X(677, "Deceleration invalid")\
    X(678, "Master changed regulator state (on -> off or off -> on) while coupling with slave was active.")\
    X(679, "Jerk invalid")\
    X(725, "Velocity and acceleration/deceleration values invalid")\
    X(726, "Rotation axis with fPositionPeriod = 0")\
    X(750, "Type of given cam is not MC_CAM_REF.")\
    X(751, "Master area, xStart and xEnd, from CamTable is not covered by curve data.")\
    X(752, "Cam data table has empty master range.")\
    X(753, "Cam data master has invalid max-, min-values.")\
    X(754, "Cam data slave has invalid max-, min-values.")\
    X(775, "During coupling of slave axis, master axis has changed direction of rotation.")\
    X(776, "Input AvoidReversal is set, but slave reversal cannot be avoided.")\
    X(777, "Input AvoidReversal must not be set for finite slave axes.")\
    X(778, "If BufferMode is not Aborting, then MasterStartDistance must not be positive.")\
    X(779, "The synchronisation cannot be started.")\
    X(800, "Gear backlash fBacklash too large (> position periode/2)")\
    X(825, "Internal error: computation of quadratic trajectory failed")\
    X(826, "Internal error: computation of quadratic trajectory failed")\
    X(827, "Internal error: computation of quadratic trajectory failed")\
    X(828, "Internal error: computation of quadratic trajectory failed")\
    X(829, "Internal error: computation of quadratic trajectory failed")\
    X(830, "Internal error: computation of quadratic trajectory failed")\
    X(831, "Internal error: computation of quadratic trajectory failed")\
    X(832, "Internal error: computation of quadratic trajectory failed")\
    X(850, "Action only permitted in standstill or power_off.")\
    X(851, "Invalid ramp type")\
    X(852, "Action only permitted in standstill or power_off.")\
    X(853, "Invalid motion type or position period")\
    X(854, "Function block only applicable to virtual axis.")\
    X(1000, "arget is not licensed for CNC.")\
    X(1001, "ath cannot be processed because set velocity = 0.")\
    X(1002, "ast object of path has end velocity <> 0.")\
    X(1003, "arning: GEOINFO-List processed in DataIn but end of list not reached.")\
    X(1004, "elocity at Stop > 0")\
    X(1005, "oo much SMC_Interpolator recursions. SoftMotion error")\
    X(1006, "MC_CheckVelocities is not the last processed function block, that accesses to the OutQueue-data by poqDataIn.")\
    X(1007, "nternal or numeric error")\
    X(1008, "elocity and acceleration / deceleration is null or to low")\
    X(1009, "B called with dwIpoTime = 0")\
    X(1010, "erk invalid because jerk must be positive")\
    X(1011, "nternal error. Computation of the quadratic velocity profile does not converge.")\
    X(1012, "nvalid velocity mode")\
    X(1013, "ore than the allowed number of axes has been interpolated. You are using a restricted version.")\
    X(1014, "he segment is numerically degenerate: It is very short and at the end of the queue. It should be ignored.")\
    X(1015, "omputation of interpolation point failed because the curvature of the spline is too high. Try changing the path to avoid sharp corners.")\
    X(1050, "arning: poqDataIn of OutQueue is created too small. Adherence of stops not guaranteed.")\
    X(1051, "ath does not go completely in queue.")\
    X(1070, "irection input has an invalid value.")\
    X(1071, "ould not determine position on CNC path for the given x-position.")\
    X(1080, "arning: poqDataIn of OutQueue is created too small. Adherence of stops not guaranteed")\
    X(1081, "arning: Final velocities inconsistent")\
    X(1100, "elocity and acceleration/deceleration values non-positive")\
    X(1120, "alues of fGapVelocity / fGapAcceleration / fGapDeceleration non-positive")\
    X(1130, "he input has been exhausted, but there is still an unfinished token")\
    X(1131, "he input does not match any token type")\
    X(1132, "he input is ambiguous, it could be more than one token type")\
    X(1133, "he string is too long to be stored in a token. (Too long string literal, variable name or identifier?)")\
    X(1134, "nvalid number literal.")\
    X(1150, "oo less arguments provided for function inside G-Code")\
    X(1151, "oo many arguments provided for function inside G-Code")\
    X(1152, "heck of argument types matching function declaration returned error")\
    X(1153, "ould not find local variable on stack")\
    X(1154, "ould not read string from token")\
    X(1155, "ot more closing than opening brackets")\
    X(1156, "ot more opening than closing brackets")\
    X(1157, "id not find infix-operator")\
    X(1158, "id not find prefix-operator")\
    X(1159, "ot two operators with equal precedence but non-equal associativity")\
    X(1160, "xpected a valid term")\
    X(1161, "ound invalid sequence of tokens inside expression")\
    X(1162, "ot more terms than expected")\
    X(1163, "ould not parse an expression because the stack is too small")\
    X(1164, "he name of a subprogram parameter or variable is already used for a different subprogram parameter or variable.")\
    X(1165, "he input token queue of the g-code parser does not hold a complete g-code sentence.")\
    X(1167, "he subprogram declaration could not be stored because the symbol table capacity has been exceeded.")\
    X(1168, "he maximum number of subprogram parameters has been exceeded.")\
    X(1169, "he lookup of the subprogram declaration failed. Maybe the subprogram name is incorrect, or the search path for subprograms is incomplete.")\
    X(1170, "id not find variable in symbol table")\
    X(1171, "oken type unknown")\
    X(1172, "o term after G-address")\
    X(1173, "nknown variable type (not LREAL, BOOL, or STRING).")\
    X(1174, " token of a different type (e.g. an operator or identifier) was expected here.")\
    X(1175, "n identifier with length three or longer was expected here.")\
    X(1176, "he identifier is too long (more than 80 characters).")\
    X(1177, " G-address (such as ‘G’, ‘F’, ‘X’) was expected here.")\
    X(1178, "n N-word (such as ‘N10’) was expected here.")\
    X(1179, "he sentence number is not in the range [0, ..., 16#FFFFFFFF].")\
    X(1180, "he sentence number must be a number literal. (For example, expressions are not allowed.)")\
    X(1181, "he identifier is a reserved keyword that cannot be used here.")\
    X(1182, "he subprogram declaration has already been read with a different signature or an error.")\
    X(1183, "he initial value has the wrong type for this local variable")\
    X(1184, "he maximum number of subprogram parameters has been exceeded.")\
    X(1185, " subprogram declaration must be the first sentence in a g-code file.")\
    X(1186, "nly one subprogram is allowed in a g-code file.")\
    X(1187, " LET declaration is not allowed after a regular g-code sentence.")\
    X(1188, "he END_SUBPROGRAM sentence is not matched by a subprogram declaration.")\
    X(1189, "fter END_SUBPROGRAM, no more tokens are allowed.")\
    X(1190, " subprogram is not terminated by END_SUBPROGRAM.")\
    X(1200, "cceleration value impermissible")\
    X(1201, "eceleration value impermissible")\
    X(1202, "ata underrun. Queue has been read and is empty.")\
    X(1203, "ump to line cannot be executed because line number is unknown.")\
    X(1204, "yntax invalid")\
    X(1205, "bjects are not supported in 3D mode.")\
    X(1206, "egative value as a period of additional axes invalid")\
    X(1207, "ot both axes A and U are interpolated. PA and PU are mutually exclusive.")\
    X(1208, "ot both axes B and V are interpolated. PB and PV are mutually exclusive.")\
    X(1209, "ot both axes C and W are interpolated. PC and PW are mutually exclusive.")\
    X(1210, "or G54/G55/G56, either all of A,B,C or none must be given if orientation mode is not equal to SMC_ORI_CONVENTION.ADDAXES.")\
    X(1211, "he z-axis of the decoder CS must be equal to the z-axis of the machine CS.")\
    X(1212, "caling would distort commanded circle or ellipse.")\
    X(1213, "ew relative rotation of the DCS (G55) would affect the scaling")\
    X(1214, "or G54/G55/G56, either all of I,J,K or none must be given.")\
    X(1250, "ocal variable cannot be read due to invalid type")\
    X(1251, "he variable that’s supposed to be written on does not have the correct type.")\
    X(1252, "ould not evaluate an expression because the evaluation stack is too small")\
    X(1253, "umeric term became NaN")\
    X(1254, "ivision by zero")\
    X(1255, "nvalid scaling factors")\
    X(1256, "n subprogram return, the interpreter stack ran empty. Internal error.")\
    X(1257, "he interpreter stack is too small. Too many local variables in the g-code or too many subprograms active.")\
    X(1258, "he given buffer for the interpreter stack is either 0 or smaller than 1024 bytes.")\
    X(1259, "he buffer for the outqueue is too small.")\
    X(1280, "o data available at all")\
    X(1281, "oo many different callstacks between current state of interpreter and iObjNo")\
    X(1282, "nvalid program index")\
    X(1283, "oo many different programs")\
    X(1284, "B called in wrong task")\
    X(1300, "uffer too small")\
    X(1301, "uffer elements have wrong type.")\
    X(1302, "urrent line of the Interpolator could not be found.")\
    X(1400, "nternal error in CNC")\
    X(1401, "he path element cannot hold H Switch points more than MAX_IPOSWITCHES.")\
    X(1410, "G75 not allowed during tool radius correction")\
    X(1411, "Queue full, but no further cartesian element")\
    X(1412, "Tool radius correction does not support movement type SPLINE3D_5")\
    X(1414, "Plane change not allowed during tool radius correction")\
    X(1450, "The maximum nesting level for subprogram calls has been exceeded.")\
    X(1451, "A return statement from the main g-code program is not allowed.")\
    X(1452, "The declaration of a subprogram has not been found in the subprogram CNC file.")\
    X(1453, "The next g-code sentence could not be added to the output sentence queue because there was not enough space left.")\
    X(1500, "Given CNC program is not of type SMC_CNC_REF")\
    X(1501, "Given OutQueue is not of type SMC_OUTQUEUE")\
    X(1502, "4-byte aligned buffer part is not used in pbyBuffer")\
    X(1600, "Function block only works with 2D paths.")\
    X(1700, "Range for smoothing too large")\
    X(1701, "Invalid input dSmoothingPart ]0..1]")\
    X(1800, "SMC_SegmentAnalyzer detects that OutQueue buffer is full but not completed. The function block can only operate when OutQueue fits the buffer completely.")\
    X(1801, "OutQueue buffer changed while the function block is operating on them.")\
    X(1820, "Invalid input value in dSplittingParameter")\
    X(1830, "Position cannot be saved. Interpolator is inactive")\
    X(1831, "Saved position has not been found so far. It is probably on a different path.")\
    X(1832, "Structure passed in ePos contains no saved position. SMC_BlockSearchSavePos is not executed or in a wrong way.")\
    X(1900, "Feature flag must be in the range {1, .., 31}.")\
    X(1901, "Function block does not support h-functions.")\
    X(1902, "Function block only works in 3D-Mode.")\
    X(1903, "Internal error computing the spline.")\
    X(1910, "wAdditionalParameter is too big")\
    X(1950, "The value of one of the inputs is invalid.")\
    X(1951, "The previous movement does not support blending.")\
    X(1952, "The previous movement does not support buffered movements.")\
    X(1953, "Cannot inherit owner because the active movement has not been not called yet in this cycle. Internal error.")\
    X(1954, "The axis is moving but there is no active movement function block. Missing follow-up movement after movement that does not end in standstill?")\
    X(1955, "The configured BufferMode is not supported by the commanded movement.")\
    X(1956, "An error occured in a previous movement.")\
    X(1957, "Only one busy buffered/blending movement per FB instance is allowed.")\
    X(2000, "File does not exist")\
    X(2001, "No buffer allocated")\
    X(2002, "Buffer too small")\
    X(2003, "Data underrun. Buffer has been read, is empty.")\
    X(2004, "Placeholder variable could not be replaced.")\
    X(2005, "Input pvl does not point to a SMC_VARLIST object.")\
    X(2006, "Input pStringBuffer is not used or does not point to a variable of type SMC_StringBuffer")\
    X(2007, "In the CNC program more different strings are used than the string buffer can hold.")\
    X(2008, "The file for a subprogram could not be found.")\
    X(2050, "File could not be opened.")\
    X(2051, "No buffer defined")\
    X(2052, "Buffer too small")\
    X(2053, "Unexpected end of file")\
    X(2100, "File could not be opened")\
    X(2101, "Buffer overrun. WriteToFile must be called more frequently.")\
    X(2200, "File could not be opened.")\
    X(2201, "Saved cam too big")\
    X(2202, "Wrong compilation mode")\
    X(2203, "File has wrong version")\
    X(2204, "Unexpected end of file")\
    X(3001, "This error is obsolete and is maintained only for compatibility. SMC_WDPF_TIMEOUT_PREPARING_LIST")\
    X(3002, "This error is obsolete and is maintained only for compatibility. File could not be created.")\
    X(3003, "This error is obsolete and is maintained only for compatibility. Error at reading the parameters.")\
    X(3004, "This error is obsolete and is maintained only for compatibility. Timeout during preparing the parameter list.")\
    X(5000, "Nominator of the conversion factor dwRatioTechUnitsDenom of the Encoder reference is 0")\
    X(5001, "Other module trying to process motion on the Encoder axis")\
    X(5002, "Filter depth is invalid")\
    X(6000, "An internal error occurred during initialization of a new movement.")\
    X(6001, "Invalid axis type, not finite or modulo (internal error)")\
    X(10000, "Buffer pBuffer too small")\
    X(10001, "The function block must be applied to a path that fits completely in the buffer.")\
    X(11000, "The axis group is in the wrong state for the requested operation.")\
    X(11001, "More than the maximum allowed number of axes has been added to an axis group.")\
    X(11002, "The dynamic limits of a single axis are invalid. (fSWMaxVelocity/Acceleration/Deceleration/Jerk)")\
    X(11004, "The given coordinate system is invalid for the requested operation")\
    X(11005, "An axis of the axis group is in state error")\
    X(11006, "The given buffer mode is not supported")\
    X(11007, "The dynamic factor is not in the range ]0 .. 1]")\
    X(11008, "The dynamic limits for the movement are invalid.")\
    X(11009, "The given axis is not part of the axis group.")\
    X(11010, "The requested operation is not supported")\
    X(11011, "No kinematic configuration has been configured.")\
    X(11012, "The number of axes is not equal to the number of axes needed by the kinematic transformation")\
    X(11013, "A coordinated movement has been interrupted by a single axis movement.")\
    X(11014, "An error occurred while following the computed set values.")\
    X(11015, "An axis group cannot depend on more than one master axis group.")\
    X(11016, "An axis group A may not depend on another axis group that depends on A.")\
    X(11017, "A dependant axis group must be executed in the same task.")\
    X(11018, "An axis belonging to an axis group must be executed in the same task.")\
    X(11019, "A second activation of the function block occurred while the PCS was still used by buffered motion commands.")\
    X(11020, "An error ocurred in a previous motion command.")\
    X(11021, "A parameter of an administrative function block is invalid.")\
    X(11022, "One of the axes of the group uses an unsupported ramp type. Only trapez and quadratic are supported.")\
    X(11023, "invalid transition parameter")\
    X(11025, "CP-Tracking: Cannot keep the path; try to reduce the dynamics on the path and / or the dynamics when entering CP-Tracking.")\
    X(11026, "Current position does not allow continuation.")\
    X(11027, "Buffer in continueData is too small; big external movement-queue-buffer in use.")\
    X(11028, "The checksum stored in the continue data is wrong. The continue data either was not written by MC_GroupInterrupt, or was modified afterwards.")\
    X(11029, "Commanded SMC_GroupWait while axes were moving independently from the axis group (single axis movement or similar).")\
    X(11030, "Inverse transformation lead to axis values exceeding the configured limits.")\
    X(11031, "Prohibit adding the same axis to an axis group twice")\
    X(11032, "No transformation is set.")\
    X(11033, "The path cannot be resumed after the current axis group error.")\
    X(11034, "The continue data for MC_GroupContinue has not been written correctly. (MC_GroupInterrupt not called / non-resumable axis group error)")\
    X(11035, "The configured kinematics does not support ABC_As_ACS")\
    X(11036, "The commanded interrupt position is invalid.")\
    X(11037, "The commanded relative movement is not supported.")\
    X(11038, "The commanded relative movement is not supported.")\
    X(11039, "The commanded circular movement is not supported.")\
    X(11040, "The commanded aborting circular movement is not supported.")\
    X(11041, "The checksum of the continue position is wrong.")\
    X(11042, "A circular movement with a tool change is only supported in a static coordinate system.")\
    X(11043, "Continue data could not be written. This can happen when trying to write continue data while moving to another continue position.")\
    X(11100, "Internal error in the kernel")\
    X(11101, "Task cycle time not positive")\
    X(12000, "The parameter values of the transformation are invalid.")\
    X(12001, "The requested position is outside of the working area of the transformation.")\
    X(12002, "The tool cannot work together with the positioning-part of the machine.")\
    X(12003, "The FB implements |ISMKinematicWithInitialization|, but hasn’t been Initialized yet.")\
    X(12004, "Special error for positioning part of Kin_4Axis_Palletizer, which implements ISMPositionKinematics_Offset2, but cannot actually cope with offsets <> 0.")\
    X(12005, "The configured tool offset is not compatible with the kinematics used")\
    X(12006, "The configured position limits of one or more axes are not valid for the kinematics.")\
    X(12007, "The path deviation is higher than the allowed tolerance.")\
    X(12008, "The given axis position exceeds the position limits of an axis.")\
    X(12100, "The cache of the CP planner is too small.")\
    X(12101, "The evaluation of the position on a path element failed (internal error).")\
    X(12102, "Non-continuable state reached (internal error).")\
    X(12103, "Maximum trajectory length exceeded (internal error).")\
    X(12104, "Path acceleration too high (invalid state, internal error).")\
    X(12105, "Maximum number of iterations exceeded (internal error).")\
    X(12106, "No trajectory could be computed (internal error).")\
    X(12107, "The out-queue is full (internal error).")\
    X(12108, "Queue underrun: no element left in queue.")\
    X(12109, "Invalid queue (invalid size or pointer)")\
    X(12110, "An internal error occurred while blending two CP movements")\
    X(12111, "The three points that define the circle are collinear.")\
    X(12112, "The center point is not on the perpendicular bisector of the start and end point.")\
    X(12113, "The radius is zero")\
    X(12114, "The distance between start and end point is larger than the diameter. (When trying to create a circle using SMC_Circ_Mode.Radius.)")\
    X(12115, "The start and end point are equal (SMC_Circ_Mode.Radius).")\
    X(12116, "The blending spline is degenerate (too short or non-regular)")\
    X(12117, "The path element cannot be created because it is too short.")\
    X(12118, "The path could not be cut for aborting movement (internal error).")\
    X(12119, "The given angular velocity or acceleration is invalid (quaternion with non-zero real part, internal error).")\
    X(12120, "The given orientation is invalid (non-orthonormal matrix or non-unit quaternion, internal error).")\
    X(12121, "The given time budget for the computation is exceeded (internal error).")\
    X(12122, "A limit of an axis has been violated. This can happen if a CP movement is done too close to a singularity or if the path has a very high curvature.")\
    X(12125, "The kinematic configuration of the start position differs from the end position. The CP movement would pass a singularity.")\
    X(12126, "This error is obsolete and is maintained only for compatibility.")\
    X(12127, "The stop trajectory could not be found in the interval. (internal error)")\
    X(12128, "The kinematics does not support the orientation mode “Axis” for continuous path movements.")\
    X(12129, "The axis orientation interpolation mode is not supported for CP movements with a dynamic PCS (tracking).")\
    X(12130, "An invalid path element has been created (internal error)")\
    X(12131, "The transition between two path elements is not G2 continuous, and there is no stop (internal error)")\
    X(12132, "Axis orientation interpolation mode is used, but between the start end end position, there is a singularity of the position kinematics.")\
    X(12133, "The position kinematics has configurations but does not implement the interface ISMPositionKinematics_Offset2.")\
    X(12134, "The target position of a rotary axis will not be reached in the commanded period.")\
    X(12135, "A rotary axis has violated the allowed axis range during a CP movement. The CP movment is not in the working area.")\
    X(12136, "The target trajectory could not be computed due to a discontinuity of the end position (internal error).")\
    X(12137, "The trajectory is not smooth, the phase end states do not equal the the current state (internal error).")\
    X(12138, "A parameter transformation could not be created (internal error).")\
    X(12139, "Error generating path-invariant PTP element (internal error).")\
    X(12140, "Error while computing a new trajectory: the path velocity after phase 1 (positive jerk of length dTau1) is negative (internal error).")\
    X(12141, "Transitioning from a single axis movement to a continuous path movement is not supported.")\
    X(12142, "The CP-planner did not make any progress.")\
    X(12143, "Internal error in the evaluation cache of the CP-planner.")\
    X(12200, "Either MaxLinearDistance or MaxAngularDistance of SMC_GroupJog2 is non-positive.")\
    X(13001, "Internal error in a movement.")\
    X(13002, "Internal error while trying to get a position in a movement.")\
    X(13003, "Internal error in the CPTR kernel.")\
    X(13004, "Internal error while interpolating in the CP kernel.")\
    X(13005, "Internal error while adding a new movement to the CP kernel.")\
    X(13006, "Internal error while coordinating kernels.")\
    X(13007, "Internal error while adding movements to the axis group.")\
    X(13008, "Internal error while reading the state of one of the group’s axes.")\
    X(13009, "Internal error while trying to read the velocity.")\
    X(13010, "Internal error while trying to read the acceleration.")\
    X(13011, "Internal error: The axis group is in an invalid state.")\
    X(13012, "Internal error while handling a PCS.")\
    X(13013, "An internal error occured during an interrupt.")\
    X(13014, "Internal error while reading continue data.")\
    X(13015, "Internal error while writing continue data.")\
    X(13016, "Internal error in a path element.")\
    X(13017, "Internal error while estimating the limits of a path element.")\
    X(13018, "Internal error in a zero-length path element.")\
    X(13019, "Internal error in a blending element.")\
    X(13020, "Internal error in the geometric part of a path element.")\
    X(13021, "Internal error in the orientation part of a path element.")\
    X(13022, "Internal error in an axis space element.")\
    X(13023, "Internal error in an element function.")\
    X(13024, "Internal error during cp halt or stop.")\
    X(13025, "Internal error while initializing trajectory planning.")\
    X(13026, "Internal error while generating samples.")\
    X(13027, "Internal error while creating the output path.")\
    X(13028, "Internal error while evaluating axis values.")\
    X(13029, "Internal error in planning lookahead.")\
    X(13030, "Internal error while trying to read the jerk.")\
    X(13031, "Internal error while writing the continue position.")\
    X(13032, "Internal error while checking whether the path is continuous.")\
    X(13033, "Internal error while aborting.")\
    X(13034, "Internal error while creating a line to relative standstill.")\
    X(13035, "Internal error while evaluating a pose on a path element.")

#endif // DATA_DEFINE_H
