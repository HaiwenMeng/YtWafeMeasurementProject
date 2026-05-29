#ifndef __ERROR_H__
#define __ERROR_H__

///////////////////////////////////////////////////////////////////////////////
//通用错误
#define LYF_ERR_GNL_SUCCESS				0x20000000			// 成功
#define	LYF_ERR_GNL_UNKNOW				0x20000001			// 未知错误
#define LYF_ERR_GNL_PARAM				0x20000002			// 参数不正确
#define LYF_ERR_GNL_PSD					0x20000004			// 密码错误
#define LYF_ERR_GNL_CET_THREAD			0x20000005			// 创建线程失败
#define LYF_ERR_GNL_INV_HANDLE			0x20000006			// 无效的句柄
#define LYF_ERR_GNL__TIME				0x20000007			// 时间转换出错
#define LYF_ERR_GNL_NOINIT				0x20000008			// 未初始化
#define LYF_ERR_GNL_HASHCOUNT			0x20000009			// HASH错误超次

///////////////////////////////////////////////////////////////////////////////
//加密锁错误
#define LYF_ERR_DOG_PSD_USR_PSD			0x20000103			// 用户密码或管理密码错误
#define LYF_ERR_DOG_USR_PSD				0x20000104			// 用户密码错误
#define LYF_ERR_DOG_LMT					0x20000105			// 期限密码错误
#define LYF_ERR_DOG_NODOG				0x20000106			// 找不到加密狗 
#define LYF_ERR_DOG_PSD					0x20000107			// 管理密码错误
#define LYF_ERR_DOG_AXPSD				0x20000108			// 动态密码错误
#define LYF_ERR_DOG_HW					0x20000109			// 底层硬件操作失败
#define LYF_ERR_DOG_ADDR				0x2000010A			// 地址校验错误

///////////////////////////////////////////////////////////////////////////////
//时钟锁错误
#define LYF_ERR_8K_RESETTIME_ONCE_IN_ONE_DAY		0x20000201			// 每天只能重设一次时间
#define LYF_ERR_8K_RESETTIME_3MIN_ONE_DAY		0x20000202			// 每天允许最大时间误差为3分钟
#define LYF_ERR_8K_HW_SETTIME				0x20000203			// 设置时间出错
#define LYF_ERR_8K_HW_READTIME				0x20000204			// 读取时间出错
#define LYF_ERR_8K_HW_UNKNOW_LMT_TYPE			0x20000205			// 未知的计时计次类型

///////////////////////////////////////////////////////////////////////////////
#define LYF_BUFFER_TOO_SMALL				0x20000301			//GetSn中用到
#endif	//endif __ERROR_H__