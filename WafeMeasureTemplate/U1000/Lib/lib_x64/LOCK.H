#ifndef __LOCK_H__/*LYF_2014_01_15*/
#define __LOCK_H__


//////////////////////////////////////////////////////////////////////////////
//如果打算使用静态库，请需要添加预编译处理定义 __LOCK_STATIC 
//#define __LOCK_STATIC

#ifdef __LOCK_STATIC

#define LYFEXP
#define In			 	// 输入参数
#define Out			 	// 输出参数

#else

#ifdef CDLL8_EXPORTS
#define LYFEXP		__declspec(dllexport)
#else
#define LYFEXP		__declspec(dllimport)
#endif // CDLL8_EXPORTS

#define In			 	// 输入参数
#define Out			 	// 输出参数

#endif

#define CALLAPI __stdcall

#ifdef __cplusplus
extern "C"{
#endif

#ifndef LIMIT_TYPE_COUNT
#define LIMIT_TYPE_COUNT				1			// 期限类型为计次
#endif
#ifndef LIMIT_TYPE_LENGTH
#define LIMIT_TYPE_LENGTH				2			// 期限类型时间长度
#endif
#ifndef LIMIT_TYPE_QUANTUM
#define LIMIT_TYPE_QUANTUM				3			// 期限类型为时间段
#endif
	///////////////////////////////////////////////////////////////////////////////



	///////////////////////////////////////////////////////////////////////////////
	//LOCK.H
	///////////////////////////////////////////////////////////////////////////////
	/************************************************************************/
	/* Led_On,设置一个Led的亮灭周期中，Led灯点亮的时间，单位为50ms				*/
	/* Led_Time, 设置一个Led灯的亮灭周期，单位为50ms							*/
	/* 说明：Led_On不能大于Led_Time,否则会返回参数错误							*/
	/* Led_On可以等于Led_Time,此时，灯常亮										*/
	/* flag为0时，忽略密码，此时password参数必须为空，							*/
	/*            函数调用成功会临时改变Led的亮灭周期							*/
	/* flag为1时，会验证密码，													*/
	/*            若密码错误，会设置失败										*/
	/*			  若密码验证完成，会永久改变Led灯的亮灭周期						*/
	/************************************************************************/
	LYFEXP BOOL CALLAPI SetLedTime(In unsigned short Led_on , 
		In unsigned short Led_Time, 
		In unsigned char flag, 
		In const char* password, 
		In unsigned long uSerial,
		In unsigned long uMini);


	LYFEXP	BOOL CALLAPI GetSn(unsigned long* pSn,unsigned long* pulCount);
	/************************************************************************
	***功能说明：
	***		此函数用于获得机器上的加密锁的序列号,可在InitiateLock之前使用
	***参数说明：
	***		pSn：指向获取序列号的ULONG数组;
	***		pulCount：pSn数组的大小;
	***		当pSn == NULL时, 如果函数成功,pulCount返回存放序列号时，pSn数组需要的大小;
	***		当pSn != NULL时, 如果函数失败，且返回 LYF_BUFFER_TOO_SMALL,表示pSn过小，pulCount返回pSn需要的数组大小;
	***						 如果函数成功，pSn获得机器上存在的加密锁序列号的数组;
	***返回值：
	***		返回值为TRUE时表示函数执行成功，否则参考错误列表。使用LYFGetLastError()获取错误号
	************************************************************************/

	LYFEXP  BOOL CALLAPI InitiateLock(unsigned long uSerial);
	/*
	*** 功能说明：
	***		此函数是加密锁的初始化函数，调用任何其他函数前请务必先调用此函数进行初始化工作。
	***
	*** 参数说明：
	***		uSerial	：加密锁的序列号，标示一个特定的加密锁；如果为0，则表示连接到本机的第一个类似的设备。
	***
	*** 返回值：
	***		该函数的返回值如果为TRUE则函数成功执行，如果为FALSE，则参考错误号列表。使用LYFGetLastError()获取错误号。
	*/


	///////////////////////////////////////////////////////////////////////////////
	LYFEXP  BOOL CALLAPI TerminateLock(unsigned long uSerial);
	/*
	*** 功能说明：
	***		此函数对是加密锁不再使用时调用的终止函数，对结束加密锁使用进行必要的操作。
	***		这个函数调用后，任何其他相关函数将不能正常使用，除非再次调用InitiateLock()函数。
	***
	*** 参数说明：
	***		uSerial	：加密锁的序列号，标示一个特定的加密锁；如果为0，则表示连接到本机的第一个类似的设备。
	***
	*** 返回值：
	***		该函数的返回值如果为TRUE则函数成功执行，如果为FALSE，则参考错误号列表。使用LYFGetLastError()获取错误号。
	*/


	//////////////////////////////////////////////////////////////////////////////
	LYFEXP  BOOL CALLAPI Lock32_Function(In long nRandom,
		Out long *puRet,
		In unsigned long uSerial);
	/*
	*** 功能说明：此函数根据输入的参数，通过加密锁的运算，返回一个有符号的32位整数。
	*** 与ShieldPC()的返回值相比较，如果两个返回值相等则说明加密锁能够正常工作。
	***
	*** 参数说明：
	*** uRandomNum	：	随机数,32位整数。
	*** puRet		：	获取到一个32位整形值
	*** uSerial		：	加密锁的序列号，标示一个特定的加密锁；如果为0，则表示连接到本机的第一个类似加密锁的设备。
	*** 
	*** 返回值：
	*** 该函数的返回值如果为TRUE则函数成功执行，如果为FALSE，则参考错误号列表。使用LYFGetLastError()获取错误号。
	*/


	//////////////////////////////////////////////////////////////////////////////
	LYFEXP  BOOL CALLAPI ReadTime(Out time_t *ptTime,
		In const char *pszPsd,
		In unsigned long uSerial,
		In unsigned long uMini);
	/*
	*** 功能说明：此函数取得加密锁的当前时间。
	***
	*** 参数说明：
	*** ptTime	：该参数以time_t的形式表示；其值是从UTC时间1970年1月1日00:00:00到当前时刻的秒数。
	*** pszPsd	：管理密码或动态密码，当其值为动态密码时，uMini是与之对应的一个值。
	*** uSerial	：加密锁的序列号，标示一个特定的加密锁；如果为0，则表示连接到本机的第一个类似设备。
	*** uMini	：动态密码的Mini值，如果pszPsd是管理密码，该值可为任意值。
	***
	*** 返回值：
	*** 该函数的返回值如果为TRUE则函数成功执行，如果为FALSE，则参考错误号列表。使用LYFGetLastError()获取错误号。
	*/


	//////////////////////////////////////////////////////////////////////////////
	LYFEXP  BOOL CALLAPI ResetTime(In const char* pszPsd,
		In unsigned long uSerial,
		In unsigned long uMini);
	/*
	*** 功能说明：此函数如果成功完成，将把加密锁的当前时间设置为本机当前时间。
	*** 只有动态密码才有权限重新设置时间。重设时间功能每天只能使用一次，每次最多矫正上下5分钟。
	***
	*** 参数说明：
	*** pszPsd	：动态密码，uMini是与之对应的一个值。
	*** uSerial	：加密锁的序列号，标示一个特定的加密锁；如果为0，则表示连接到本机的第一个类似设备。
	*** uMini	：动态密码的Mini值。
	***
	*** 返回值：
	*** 该函数的返回值如果为TRUE则函数成功执行，如果为FALSE，则参考错误号列表。使用LYFGetLastError()获取错误号。
	*/

	//////////////////////////////////////////////////////////////////////////////
	LYFEXP  BOOL CALLAPI WriteLock(In unsigned long uAddr,
		In const void *Buffer,
		In const char *pszPsd,
		In unsigned long uSerial,
		In unsigned long uMini);
	/*
	*** 功能说明：向加密锁的存储器中写入数据，该函数只写入从缓冲区指针指向地址开始的4字节数据。
	***
	*** 参数说明：
	*** uAddr	：要写入的起始地址，必须为从1开始到1151
	*** Buffer	：要写入的缓冲区指针。
	*** pszPsd	：管理密码或动态密码，当其值为动态密码时，uMini是与之对应的一个值。
	*** uSerial	：加密锁的序列号，标示一个特定的加密锁；如果为0，则表示连接到本机的第一个类似的设备。
	*** uMini	：动态密码的Mini值，如果pszPsd是管理密码，该值可为任意值。
	***
	*** 返回值：
	*** 该函数的返回值如果为TRUE则函数成功执行，如果为FALSE，则参考错误号列表。使用LYFGetLastError()获取错误号。
	*/

	//////////////////////////////////////////////////////////////////////////////

	LYFEXP  BOOL CALLAPI WriteLockEx(In unsigned long uAddr,
		In const void *Buffer,
		In unsigned long iBufLen,
		In const char *pszPsd,
		In unsigned long uSerial,
		In unsigned long uMini);
	/*
	*** 功能说明：向加密锁的存储器中写入整段数据。
	***
	*** 参数说明：
	*** uAddr	：要写入的起始地址，必须为从1开始到1151。
	*** Buffer	：要写入的缓冲区指针。
	*** iBufLen	：写入缓冲去的长度。
	*** pszPsd	：管理密码或动态密码，当其值为动态密码时，uMini是与之对应的一个值。
	*** uSerial	：加密锁的序列号，标示一个特定的加密锁；如果为0，则表示连接到本机的第一个类似加密锁的设备。
	*** uMini	：动态密码的Mini值，如果pszPsd是管理密码，该值可为任意值。
	***
	*** 返回值：
	*** 该函数的返回值如果为TRUE则函数成功执行，如果为FALSE，则参考错误号列表。使用LYFGetLastError()获取错误号。
	*/

	//////////////////////////////////////////////////////////////////////////////
	LYFEXP  BOOL CALLAPI ReadLock(In unsigned long uAddr,
		In void *pBuffer,
		In const char *pszPsd,
		In unsigned long uSerial,
		In unsigned long uMini);
	/*
	*** 功能说明：从加密锁的存储器中读出数据，该函数读取从指定地址开始的4字节数据。
	***
	*** 参数说明：
	*** uAddr	：要读取的起始地址，必须为从1开始到1151
	*** Buffer	：要读取的缓冲区指针。
	*** pszPsd	：管理密码或动态密码，当其值为动态密码时，uMini是与之对应的一个值。
	*** uSerial	：加密锁的序列号，标示一个特定的加密锁；如果为0，则表示连接到本机的第一个类似加密锁的设备。
	*** uMini	：动态密码的Mini值，如果pszPsd是管理密码，该值可为任意值。
	***
	*** 返回值：
	*** 该函数的返回值如果为TRUE则函数成功执行，如果为FALSE，则参考错误号列表。使用LYFGetLastError()获取错误号。
	*/

	//////////////////////////////////////////////////////////////////////////////
	LYFEXP  BOOL CALLAPI ReadLockEx(In unsigned long uAddr,
		In void *pBuffer,
		In unsigned long iReadLen,
		In const char *pszPsd,
		In unsigned long uSerial,
		In unsigned long uMini);
	/*
	*** 功能说明：从加密锁的存储器中读出整段数据。
	***
	*** 参数说明：
	*** uAddr	：要读取的起始地址，必须为从1开始到1279
	*** Buffer	：要读取的缓冲区指针。
	*** iReadLen：读取的数据长度
	*** pszPsd	：管理密码或动态密码，当其值为动态密码时，uMini是与之对应的一个值。
	*** uSerial	：加密锁的序列号，标示一个特定的加密锁；如果为0，则表示连接到本机的第一个类似加密锁的设备。
	*** uMini	：动态密码的Mini值，如果pszPsd是管理密码，该值可为任意值。
	***
	*** 返回值：
	*** 该函数的返回值如果为TRUE则函数成功执行，如果为FALSE，则参考错误号列表。使用LYFGetLastError()获取错误号。
	*/

	//////////////////////////////////////////////////////////////////////////////
	LYFEXP  BOOL CALLAPI Counter(In const char *pszPsd,
		In unsigned long uSerial,
		In unsigned long uMini,
		In unsigned long uSign,
		Out unsigned long *uCount);
	/*
	*** 功能说明：获取加密锁还要多少次到期。
	***
	*** 参数说明：
	*** pszPsd	：管理密码或读写密码，如果是读写密码时，每次调用使用次数（或者时间）会减1。
	*** uSerial	：加密锁的序列号，标示一个特定的加密锁；如果为0，则表示连接到本机的第一个类似加密锁的设备。
	*** uMini	：动态密码的Mini值，如果pszPsd是管理密码，该值可为任意值。
	*** uSign	：到期时是否弹出窗口提示，1为弹出，0为不弹。
	***	uCount	：出参，用来获取加密锁还有多少次到期。
	***
	*** 返回值：
	*** 该函数的返回值如果为TRUE则函数成功执行，如果为FALSE，则参考错误号列表。使用LYFGetLastError()获取错误号。
	*/

	//////////////////////////////////////////////////////////////////////////////
	LYFEXP  BOOL CALLAPI SetLock(In unsigned long uFunction,
		In Out unsigned long *puParam,
		In unsigned long uParam,
		In const char *pszParam,
		In const char *pszPsd,
		In unsigned long uSerial,
		In unsigned long uMini);
	/*
	*** 功能说明：该函数的功能是设置加密锁。
	***
	*** 参数说明：
	*** uFun	：功能号
	*** puParam	：与功能号对应的参数
	*** uParam	：与功能号对应的32位整数
	*** pszParam：与功能号对应的参数
	*** pszPsd	：管理密码或动态密码，当其值为动态密码时，uMini是与之对应的一个值。
	*** uSerial	：加密锁的序列号，标示一个特定的加密锁；如果为0，则表示连接到本机的第一个类似加密锁的设备。
	*** uMini	：动态密码的Mini值，如果pszPsd是管理密码，该值可为任意值。

	***----------------------------------------------------------------------------
	*** 功能号:uFunction = 0 时：函数返回加密锁剩余的使用次数/时间
	*** uFun	：= 0
	*** puParam	：返回剩余次数/分钟
	*** uParam	：忽略
	*** pszParam：忽略
	*** pszPsd	：管理密码或动态密码，当其值为动态密码时，uMini是与之对应的一个值。
	*** uSerial	：加密锁的序列号，标示一个特定的加密锁；如果为0，则表示连接到本机的第一个类似加密锁的设备。
	*** uMini	：动态密码的Mini值；如果pszPsd是管理密码，该值忽略。
	***----------------------------------------------------------------------------

	***----------------------------------------------------------------------------
	*** 功能号:uFunction = 1 时：设置加密锁期限，类型为计次
	*** uFun	：= 1
	*** puParam	：要设置期限的次数
	*** uParam	：忽略
	*** pszParam：期限密码
	*** pszPsd	：管理密码或动态密码，当其值为动态密码时，uMini是与之对应的一个值。
	*** uSerial	：加密锁的序列号，标示一个特定的加密锁；如果为0，则表示连接到本机的第一个类似加密锁的设备。
	*** uMini	：动态密码的Mini值；如果pszPsd是管理密码，该值忽略。
	***----------------------------------------------------------------------------

	***----------------------------------------------------------------------------
	*** 功能号:uFunction = 2 时：设置加密锁期限，类型为计时类型，加密锁可以有效使用时间为特定的分钟数
	*** uFun	：= 2
	*** puParam	：要设置期限的分钟数
	*** uParam	：忽略
	*** pszParam：期限密码
	*** pszPsd	：管理密码或动态密码，当其值为动态密码时，uMini是与之对应的一个值。
	*** uSerial	：加密锁的序列号，标示一个特定的加密锁；如果为0，则表示连接到本机的第一个类似加密锁的设备。
	*** uMini	：动态密码的Mini值；如果pszPsd是管理密码，该值忽略。
	***----------------------------------------------------------------------------

	***----------------------------------------------------------------------------
	*** 功能号:uFunction = 3 时：设置加密锁期限，类型为计时类型，加密锁可以有效使用至固定的某个时间
	*** uFun	：= 3
	*** puParam	：要设置期限的起始时间
	*** uParam	：要设置期限的结束时间
	*** pszParam：期限密码
	*** pszPsd	：管理密码或动态密码，当其值为动态密码时，uMini是与之对应的一个值。
	*** uSerial	：加密锁的序列号，标示一个特定的加密锁；如果为0，则表示连接到本机的第一个类似加密锁的设备。
	*** uMini	：动态密码的Mini值；如果pszPsd是管理密码，该值忽略。
	***----------------------------------------------------------------------------

	***----------------------------------------------------------------------------
	*** 功能号:uFunction = 4 时：永久解除时间限制，或重新重置加密锁期限
	*** uFun	：= 4
	*** puParam	：忽略
	*** uParam	：当为动态密码时输入需要重新设定的期限（必须与生成动态密码输入的相同）
	*** pszParam：期限密码。注，当期限密码正确传入时，将永久解除时间限制。如要重置时间限制，请忽略此参数。
	*** pszPsd	：动态密码，uMini是与之对应的一个值。
	*** uSerial	：加密锁的序列号，标示一个特定的加密锁；如果为0，则表示连接到本机的第一个类似加密锁的设备。
	*** uMini	：动态密码的Mini值；如果pszParam已经输入正确的期限密码，该值忽略。
	*** 注意	输入错误，需要为加密锁断电（插拔），才可以正常解除或者重置期限
	***----------------------------------------------------------------------------

	***----------------------------------------------------------------------------
	*** 功能号:uFunction = 5 时：设置限制类型
	*** uFun	：= 5
	*** puParam	：指向的值为LIMIT_TYPE_COUNT表示计次。LIMIT_TYPE_LENGTH表示计时。LIMIT_TYPE_QUANTUM标示特定时间段。
	*** uParam	：忽略
	*** pszParam：忽略
	*** pszPsd	：管理密码或动态密码，当其值为动态密码时，uMini是与之对应的一个值。
	*** uSerial	：加密锁的序列号，标示一个特定的加密锁；如果为0，则表示连接到本机的第一个类似加密锁的设备。
	*** uMini	：动态密码的Mini值；如果pszPsd是管理密码，该值忽略。
	***----------------------------------------------------------------------------

	***----------------------------------------------------------------------------
	*** 功能号:uFunction = 6 时：获取限制类型
	*** uFun	：= 6
	*** puParam	：返回加密锁当前期限类型 与设置时的该值对应，如果不在这3个当中，标示加密锁还未设置期限。
	*** uParam	：忽略
	*** pszParam：忽略
	*** pszPsd	：管理密码或动态密码，当其值为动态密码时，uMini是与之对应的一个值。
	*** uSerial	：加密锁的序列号，标示一个特定的加密锁；如果为0，则表示连接到本机的第一个类似加密锁的设备。
	*** uMini	：动态密码的Mini值；如果pszPsd是管理密码，该值忽略。
	***----------------------------------------------------------------------------

	***----------------------------------------------------------------------------
	*** 功能号:uFunction = 7 时：设置管理密码
	*** uFun	：= 7
	*** puParam	：忽略
	*** uParam	：忽略
	*** pszParam：指向要设置的密码字符串的指针，密码长度最长为8个英文字母、数字、特殊符号、等字符。
	*** pszPsd	：管理密码或动态密码，当其值为动态密码时，uMini是与之对应的一个值。
	*** uSerial	：加密锁的序列号，标示一个特定的加密锁；如果为0，则表示连接到本机的第一个类似加密锁的设备。
	*** uMini	：动态密码的Mini值；如果pszPsd是管理密码，该值忽略。
	***----------------------------------------------------------------------------

	***----------------------------------------------------------------------------
	*** 功能号:uFunction = 8 时：获取加密锁的序列号
	*** uFun	：= 8
	*** puParam	：返回加密锁的序列号
	*** uParam	：忽略
	*** pszParam：忽略
	*** pszPsd	：忽略
	*** uSerial	：加密锁的序列号，标示一个特定的加密锁；如果为0，则表示连接到本机的第一个类似加密锁的设备。
	*** uMini	：动态密码的Mini值；如果pszPsd是管理密码，该值忽略。
	***----------------------------------------------------------------------------

	*** 返回值：
	*** 该函数的返回值如果为非0则函数成功执行，如果为0，则参考错误号列表。LYFGetLastError()返回错误号。

	注意：调用此函数使用其不同功能时，忽略的参数不能传入0，NULL等值,其他任意值都可以。
	*/

	///////////////////////////////////////////////////////////////////////////////
	LYFEXP  unsigned long CALLAPI LYFGetLastErr();
	/*
	*** 功能说明：该函数的功能是返回当前线程最后一次错误的错误号，与windows的GetLastError()函数返回的值兼容
	***			  函数返回的错误号可在错误列表中查找，如果不在错误表中，则在windows API的错误列表中查找。
	***
	*** 返回值：
	*** 该函数的返回值是函数的错误号。
	*/
	///////////////////////////////////////////////////////////////////////////////

	// CPU SerialNumber
	// comprise 6 WORD nibble, fommatting xxxx-xxxx-xxxx-xxxx-xxxx-xxxx

	struct SerialNumber
	{
		WORD nibble[6];			// 6 WORD nibble;

		SerialNumber()			//constructor
		{
			memset(nibble, 0, sizeof(nibble));
		}
	};

	//以下四个函数。当执行成功时返回TRUE,否则返回FALSE;例程为cdll8test
	LYFEXP BOOL	CALLAPI GetCpuId(SerialNumber &serial);
	/*
	*** 功能说明：获取电脑的cpuid，cpuid是个六个字，96位长度的结构，类似，xxxx-xxxx-xxxx-xxxx-xxxx-xxxx。
	***
	*** 参数说明：SerialNumber : serial;
	是个结构体，定义如下：
	struct SerialNumber
	{
	WORD nibble[6];			// 6 WORD nibble;

	SerialNumber()			//constructor
	{
	memset(nibble, 0, sizeof(nibble));
	}
	};
	用于保存获得的cpuid;

	*** 返回值：
	*** 该函数的返回值如果为非0则函数成功执行，如果为0，则机器不支持cpuid。
	*/
	LYFEXP BOOL	CALLAPI GetMacAddr(BYTE* mac);
	/*
	*** 功能说明：获取电脑的第一个网卡的mac地址。
	***
	*** 参数说明：BYTE* mac:是一个6字节的数组mac[6]; 如果函数执行成功，mac会保存六个字节的mac地址
	*** 返回值：
	*** 该函数的返回值如果为非0则函数成功执行，如果为0，则参考错误号列表。LYFGetLastError()返回错误号。
	*/
	LYFEXP BOOL	CALLAPI GetHDSerial(char* HardDriveModelNumber);
	/*
	***功能：获取硬盘序列号
	***参数：可以设为char HardDriveModelNumber[128];
	***返回值：如果返回值为非0则函数执行成功，否则参考错误号列表。LYFGetLastError()返回错误号。
	*/
	LYFEXP BOOL	CALLAPI GetVolumNumber(const char* lpRootPathName,DWORD* lpVolumeSerialNumber);
	/*
	***功能：获取逻辑磁盘序列号
	***参数：lpRootPathName，要获取的逻辑盘符，如C:\;
	***	 lpVolumeSerialNumber，DWORD类型指针，保存序列号
	***返回值：如果返回值为非0则函数执行成功，否则参考错误号列表。LYFGetLastError()返回错误号。
	*/

#ifdef __cplusplus
}
#endif



#ifndef _SHIELDPC_H
#define _SHIELDPC_H

/*
*** 功能说明：
***		此函数根据输入的参数返回一个无符号的32位整数。与Lock32_Function()
***		的返回值相比较，如果两个返回值相等则说明加密锁能够正常工作。
***
*** 参数说明：
***		nRandom	：32位随机数。
***		key1		：16位的Key值，需要与加密锁的对应值相等 0-65535
***		key2		：16位的Key值，需要与加密锁的对应值相等 0-65535
***		key3		：16位的Key值，需要与加密锁的对应值相等 0-65535
***		key4		：16位的Key值，需要与加密锁的对应值相等 0-65535
***
*** 返回值：
***		该函数返回一个32位无符号整形值。
*/
static long ShieldPC( long nRandom, unsigned short key1, unsigned short key2, unsigned short key3, unsigned short key4 )
{
	long ReTurnData;

	unsigned short int y,y1,y2,x1,x2,y11,y22;
	unsigned short int outdata1,outdata2;
	//outdata2 = //保存X的高16位
	outdata2 = (unsigned short int)((nRandom & 0xffff0000) >> 16);
	outdata1 = (unsigned short int)(0xffff & nRandom); //保存X的低16位
	x1=outdata1;
	x2=outdata2;
	y1 = (unsigned short int)(x1 ^ key2);
	y11 = (unsigned short int)(x2 ^ key1);

	y1 = y1 + y11;
	if (y1 > 32767)
		y1 = y1 - 32767;

	y1 = (unsigned short int)(y1 ^ 16) ;
	if (y1 > 32767)
		y1 = y1 - 32767;

	y1 = (unsigned short int)(y1 % key4);
	y = (unsigned short int)(y1 ^ key3);
	if (y > 50000)
		y = y - 50000-1;

	y11 = x1 + key1;
	if (y11 > 32767)
		y11 = y11 - 32767;

	y22 = (unsigned short int)(y11 % key3);
	y11 = (unsigned short int)(key4 ^ x2);
	y2 = y22 ^ y11;

	if (y2 > 50000)
		y2 = y2 - 50000-1;
	y = y ^ y2;

	ReTurnData = y2;

	ReTurnData = ReTurnData << 16; //返回的数据为y2和y拼成的长整型数据
	ReTurnData = ReTurnData ^ y;
	return(ReTurnData);
};

#endif


///////////////////////////////////////////////////////////////////////////////
#endif // __LOCK_H__
