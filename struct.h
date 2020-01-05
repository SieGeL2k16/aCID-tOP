/****************************************************************************
 *    FILE: struct.h
 * PURPOSE:  Global variables used by aCID-tOP
 ****************************************************************************/

long node = NULL;

struct 	FAMELibrary		*FAMEBase = NULL;
struct 	Library 			*UtilityBase = NULL;
struct  Library       *DateBase=NULL;
struct 	LocaleBase 		*LocaleBase = NULL;
struct	Locale				*myloc;

APTR		mem_pool = NULL;

char 		NO_MEM[] = "[37mCannot allocate memory!!![m";

// Stores data from current upload session, if called as ULSTAT

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

// Date breakdown, required for data processing

struct	DateInfo
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

// One record of normal weekdata for one user

struct WeekTopData
	{
	long	UserNumber;
	char	UserName[32],
				Location[32];
	ULONG	BytesHi,
				BytesLow;
	ULONG Files;
	};

// All time records

struct WeekTopRecords
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

// Configuration of aCID-tOP

struct Config
	{
	char  DataPath[256],
				ConfHeader[256],
				BullPath[256];
	BOOL	SysopCount,
				WriteMail;
  short	MaxUsers;
	long	ConfNum;
	};

// We allocate these structures:

struct WeekTopData    *WTD = NULL;
struct WeekTopRecords *WTR = NULL;
struct Config 				*config = NULL;
struct DateInfo       *dinfo  = NULL;
struct ULStats				*ulstats = NULL;
