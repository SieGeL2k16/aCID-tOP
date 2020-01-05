/****************************************************************************
 *    FILE: struct_ex.h
 * PURPOSE:  Global variables used by aCID-tOP - extern version
 ****************************************************************************/

extern struct 	FAMELibrary		*FAMEBase;
extern struct 	Library 			*UtilityBase;
extern struct 	LocaleBase 		*LocaleBase;
extern struct		Locale				*myloc;
extern struct  	Library       *DateBase;

extern APTR	mem_pool;

struct ULStats
	{
	long	filecount;
	long	mode;
	long	internal;
	ULONG	BytesHi;
	ULONG	BytesLo;
  long	UserNumber;
	char	UserName[28],
				UserLocation[26];
	};

extern struct	DateInfo
	{
  long		day,
					month,
					year,
					weeknum,
					dayofweek,
					daysleft,
					wnumrecords,
					maxweeks;
	};

extern struct WeekTopData
	{
	long	UserNumber;
	char	UserName[32],
				Location[32];
	ULONG	BytesHi,
				BytesLow;
	ULONG Files;
	};

extern struct WeekTopRecords
	{
  long	ActWeekNumber;
	char	LastUserName[32],
				BestUserName[32];
	ULONG	LastBytesHi,
				LastBytesLow;
	ULONG	BestBytesHi,
				BestBytesLow;
	ULONG LastFiles;
	ULONG BestFiles;
	};

extern struct Config
	{
	char  DataPath[256],
				ConfHeader[256],
				BullPath[256];
	BOOL	SysopCount,
				WriteMail;
  short	MaxUsers;
	long	ConfNum;
	};

extern struct WeekTopData    	*WTD;
extern struct WeekTopRecords 	*WTR;
extern struct Config 				 	*config;
extern struct DateInfo       	*dinfo;
extern struct ULStats					*ulstats;

// Error strings:

extern char NO_MEM[];
