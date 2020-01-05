/****************************************************************************
 *    FILE: WriteData.c
 * PURPOSE: Contains data saving functions (Records, Weekly data etc.)
 ****************************************************************************/

#include <proto/exec.h>
#include <proto/fame.h>
#include <proto/dos.h>
#include <proto/locale.h>
#include <proto/date.h>
#include <exec/exec.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <utility/date.h>
#include <utility/tagitem.h>
#include <dos/stdio.h>
#include <dos/dosextens.h>
#include <Fame/fameDoorProto.h>
#include <pragmas/date_pragmas.h>
#include <libraries/date.h>
#include "proto.h"
#include "struct_ex.h"
#include "version.h"

#define HIGH	0
#define LOW		1

static char wdatname[] = "conf_%ld_weekly.dat",
						wrecname[] = "conf_%ld_records.dat",
						infoline[] = "[36mConference[m: [33m%-42s [36mDay[m: [33m([32m%ld[34m/[32m7[33m) [32m%ld [36m%s left[m\n\r\n\r",
            theader[]	 = "[32mNo# [36mUsername [m([36mHandle[m)        [33mLocation [32m([33mGroup[32m)            [32mFiles [33mUploaded Bytes[m\n\r",
						separator[]= "[34m===-========================-===========================-=====-===============[m\n",
						nodatfile[]= "[37mFAIL!\n\r\n\r[37mCannot save datafile %s (%ld)!\n\r";

extern char *_ProgramName;

struct WeekTopData *tempbuf = NULL;
struct WeekTopData *list    = NULL;
struct WeekTopData zwi;

struct  WeeklyTotals
	{
	ULONG	Files;
	ULONG	BytesHi;
	ULONG BytesLo;
  }wtotals;

/**************************************************************************************************
 * Prototype declarations:
 **************************************************************************************************/

void ShowStats(void);
void UpdateStats(void);
void AddFTPUploads(void);							// V1.1: Checks for datafiles to be added from FTPd

static void ShowEmptyStats(BPTR);
static void CheckForNewWeek(BOOL);
static void SaveUserInfo(void);
static void myquicksort(long start,long end);
static void ShowEntries(long maxentries,BPTR fh);
static void ShowFooter(BPTR);
static void WriteStatsToMail(void);

/**************************************************************************************************
 *  FUNCTION: ShowStats()
 *   PURPOSE: Reads data for current conference and displays them sorted.
 * PARAMETER: none
 *   RETURNS: none
 **************************************************************************************************/

void ShowStats(void)
	{
	BPTR  	weekfile;
  char		fname[256],confname[128];
	long		entries,alloc, i = 0;
	struct	FileInfoBlock __aligned myfib;

	FAMEFillMem(&wtotals,0,sizeof(struct WeeklyTotals));
	CheckForNewWeek(FALSE);
	AddFTPUploads();
	SPrintf(confname,wdatname,config->ConfNum);
  FAMEStrCopy(config->DataPath,fname,255);
	AddPart(fname,confname,255);
	ShowHeader(TRUE);
	GetCommand(confname,0,0,0,SR_ConfName);
	PutStringFormat(infoline,confname,dinfo->dayofweek,dinfo->daysleft,(dinfo->daysleft==1) ? "day" : "days");
	PutString(theader,0);
	PutStringFormat("%s\r",separator);
	if(!(weekfile = Open(fname,MODE_OLDFILE)))
    {
		ShowEmptyStats(NULL);
		ShowFooter(NULL);
		return;
		}
	ExamineFH(weekfile,&myfib);
	entries = myfib.fib_Size / sizeof(struct WeekTopData);
  if(!entries)
  	{
		Close(weekfile);
		ShowEmptyStats(NULL);
		ShowFooter(NULL);
		return;
    }
	if(entries < config->MaxUsers) alloc = config->MaxUsers;
	else alloc = entries;
	if(!(list=AllocPooled(mem_pool,sizeof(struct WeekTopData)*alloc)))
		{
		Close(weekfile);
		wb(NO_MEM);
		}
	Seek(weekfile,0L,OFFSET_BEGINNING);
  while(i < entries)
		{
    if(Read(weekfile,WTD,sizeof(struct WeekTopData))<=0) break;
		CopyMemQuick(WTD,&list[i],sizeof(struct WeekTopData));
		FAMEAdd64(WTD->BytesHi,WTD->BytesLow,&wtotals.BytesHi);
		wtotals.Files+=WTD->Files;
		i++;
		}
  Close(weekfile);
	tempbuf = list;
	myquicksort(0,alloc-1);
	ShowEntries(alloc,NULL);
	ShowFooter(NULL);
	}

/**************************************************************************************************
 *  FUNCTION: myquicksort()
 *   PURPOSE: Sorts the WeekTop Data by Bytes with FAMENum64Comp().
 * PARAMETER: start => Offset to start sort, usually 0
 *            end   => Offset to end sort, usually max entries
 *   RETURNS: none
 *      NOTE: This function is recursive, make sure that you have enough stack space defined!!!
 **************************************************************************************************/

static void myquicksort(long start,long end)
	{
	ULONG			num64[2];
	register 	long	i,j,s;

	i=start;
	j=end;
	s=(start+end)/2;
	num64[HIGH]	= tempbuf[s].BytesHi;
	num64[LOW]	= tempbuf[s].BytesLow;
	do
		{
		while(FAMENum64Comp(tempbuf[i].BytesHi,tempbuf[i].BytesLow,num64[HIGH],num64[LOW])==1) i++;
		while(FAMENum64Comp(tempbuf[j].BytesHi,tempbuf[j].BytesLow,num64[HIGH],num64[LOW])==-1) j--;
		if (i <= j)
			{
			zwi	= tempbuf[i];
			tempbuf[i]	= tempbuf[j];
			tempbuf[j]  = zwi;
			i++;
			j--;
			}
		}while (i <= j);
	if (start < j) myquicksort(start, j);
	if (end   > i) myquicksort(i,end);
	}

/**************************************************************************************************
 *  FUNCTION: ShowEntries()
 *   PURPOSE: Displays sorted Highscore list
 * PARAMETER: long maxentries => Amount of entries to display
 *            BPTR fh         => NULL to print on screen, else print on given filehandle
 *   RETURNS: none
 **************************************************************************************************/

static void ShowEntries(long maxentries,BPTR fh)
	{
	long	i;
	char  fbuf[40],numbuf[40];

  for ( i = 0; i < maxentries; i++)
		{
		FAMENumToStr(list[i].Files,FNSF_GROUPING|FNSF_NUMLOCALE,40,fbuf);
		FAMENum64ToStr(list[i].BytesHi,list[i].BytesLow,FNSF_GROUPING|FNSF_NUMLOCALE,40,numbuf);
		if(fh)
			{
			FPrintf(fh,"[32m%2ld[33m. [36m%-24s [33m%-27s [32m%5s [33m%14s\n\r",(i+1),list[i].UserName,list[i].Location,fbuf,numbuf);
      }
		else
			{
	    PutStringFormat("[32m%2ld[33m. [36m%-24s [33m%-27s [32m%5s [33m%14s\n\r",(i+1),list[i].UserName,list[i].Location,fbuf,numbuf);
      }
		}
	}

/**************************************************************************************************
 *  FUNCTION: ShowFooter()
 *   PURPOSE: Displays weekly totals and records
 * PARAMETER: BPTR fh => NULL to print on screen, else print to given filehandle
 *   RETURNS: none
 **************************************************************************************************/

static void ShowFooter(BPTR fh)
	{
	char	fbuf[10],nbuf[20];

	FAMENumToStr(wtotals.Files,FNSF_GROUPING|FNSF_NUMLOCALE,10,fbuf);
	FAMENum64ToStr(wtotals.BytesHi,wtotals.BytesLo,FNSF_GROUPING|FNSF_NUMLOCALE,20,nbuf);
	if(fh)
		{
    FPrintf(fh,"\n[36mTotal Uploaded Files[m: [33m[[32m%9s[33m] [36mTotal Uploaded Bytes[m: [33m[[32m%19s[33m][m\n",fbuf,nbuf);
		FPuts(fh,separator);
		}
	else
		{
		PutStringFormat("\n\r[36mTotal Uploaded Files[m: [33m[[32m%9s[33m] [36mTotal Uploaded Bytes[m: [33m[[32m%19s[33m][m\n\r",fbuf,nbuf);
		PutStringFormat("%s\r",separator);
    }
	FAMENumToStr(WTR->LastFiles,FNSF_GROUPING|FNSF_NUMLOCALE,10,fbuf);
	FAMENum64ToStr(WTR->LastBytesHi,WTR->LastBytesLow,FNSF_GROUPING|FNSF_NUMLOCALE,20,nbuf);
	if(!*WTR->LastUserName) FAMEStrCopy("n/a",WTR->LastUserName,27);
	if(fh)
		{
		FPrintf(fh,"[36mTop Uploader Last Week[m: [33m%-18s [36mBytes[m: [33m%14s [36mFiles[m: [33m%5s[m\n",WTR->LastUserName,nbuf,fbuf);
    }
	else
		{
		PutStringFormat("[36mTop Uploader Last Week[m: [33m%-18s [36mBytes[m: [33m%14s [36mFiles[m: [33m%5s[m\n\r",WTR->LastUserName,nbuf,fbuf);
    }
	FAMENumToStr(WTR->BestFiles,FNSF_GROUPING|FNSF_NUMLOCALE,10,fbuf);
	FAMENum64ToStr(WTR->BestBytesHi,WTR->BestBytesLow,FNSF_GROUPING|FNSF_NUMLOCALE,20,nbuf);
	if(!*WTR->BestUserName) FAMEStrCopy("n/a",WTR->BestUserName,27);
	if(fh)
		{
		FPrintf(fh,"[36mTop Uploader Record   [m: [33m%-18s [36mBytes[m: [33m%14s [36mFiles[m: [33m%5s[m\n",WTR->BestUserName,nbuf,fbuf);
    }
	else
		{
    PutStringFormat("[36mTop Uploader Record   [m: [33m%-18s [36mBytes[m: [33m%14s [36mFiles[m: [33m%5s[m\n\r",WTR->BestUserName,nbuf,fbuf);
		}
	}

/**************************************************************************************************
 *  FUNCTION: ShowEmptyStats()
 *   PURPOSE: Displays empty bull in case of new week or file not found
 * PARAMETER: BPTR fh => Filehandle to write stats too, else NULL to print on screen
 *   RETURNS: none
 **************************************************************************************************/

static void ShowEmptyStats(BPTR fh)
	{
	long	middle = config->MaxUsers / 2;
  long	i;
	char	noul[]="                No uploaders available";

	for(i = 0; i < config->MaxUsers; i++)
		{
    if((i+1) == middle)
			{
			if(fh) FPrintf(fh,"[32m%2ld[33m. [35m%-52s [32m%5s [33m%14s[m\n\r",(i+1),noul,"0","0");
			else PutStringFormat("[32m%2ld[33m. [35m%-52s [32m%5s [33m%14s[m\n\r",(i+1),noul,"0","0");
			}
		else
			{
			if(fh) FPrintf(fh,"[32m%2ld[33m. [36m%-24s [32m%-27s [32m%5s [33m%14s\n\r",(i+1),"","","0","0");
      else PutStringFormat("[32m%2ld[33m. [36m%-24s [32m%-27s [32m%5s [33m%14s\n\r",(i+1),"","","0","0");
			}
    }
	}

/**************************************************************************************************
 *  FUNCTION: UpdateStats()
 *   PURPOSE: Updates ULSTAT in DataDir for current conference/user
 * PARAMETER: none
 *   RETURNS: none
 **************************************************************************************************/

void UpdateStats(void)
	{
	char tbuf[40],fbuf[40];

	ShowHeader(FALSE);
	FAMENum64ToStr(ulstats->BytesHi,ulstats->BytesLo,FNSF_GROUPING|FNSF_NUMLOCALE,38,tbuf);
	FAMENumToStr(ulstats->filecount,FNSF_GROUPING|FNSF_NUMLOCALE,38,fbuf);
	PutStringFormat("\n\r[36mAdding[m: [32m%s [36m%s with [33m%s [36mbytes[m...",fbuf,(ulstats->filecount==1) ? "file" : "files",tbuf);
	GetCommand(ulstats->UserName,0,0,0,NR_Name);
	GetCommand(ulstats->UserLocation,0,0,0,NR_Location);
	GetCommand("",0,0,0,NR_SlotNumber);
	ulstats->UserNumber=MyFAMEDoorMsg->fdom_Data2;
  CheckForNewWeek(TRUE);
	SaveUserInfo();
	AddFTPUploads();
	PutString("[32mdone.\n\r",1);
	}

/**************************************************************************************************
 *  FUNCTION: CheckForNewWeek()
 *   PURPOSE: Loads the All-Records, checks if weeknumbers are equal. If not, drops old weekly data
 *            file and updates the records file with actual week number and new records if the
 *            old ones where beaten. Finally the week data file is truncated for new week start.
 * PARAMETER:  TRUE => Show infos on screen
 *            FALSE => Perform everything without displaying infos
 *   RETURNS: none
 **************************************************************************************************/

static void CheckForNewWeek(BOOL display)
	{
	BPTR 		rfile;
  char		fname[256],confname[128],rname[256];
	long  	entries,i = 0;
	struct	FileInfoBlock __aligned myfib;
	BOOL		nofile = FALSE;
	struct	WeekTopData __aligned saft;

	SPrintf(confname,wrecname,config->ConfNum);
  FAMEStrCopy(config->DataPath,rname,255);
	AddPart(rname,confname,255);
	if(rfile = Open(rname,MODE_OLDFILE))
		{
		Read(rfile,WTR,sizeof(struct WeekTopRecords));
		Close(rfile);
		}
  else nofile = TRUE;
	if(WTR->ActWeekNumber==dinfo->weeknum) return;		// We are still in the same week.

	dinfo->wnumrecords = WTR->ActWeekNumber;					// Save week number from old records

	// Re-Init Mode:

	if(display==TRUE) PutString("[36mre-init db[m...",0);
  if(nofile == FALSE) WriteStatsToMail();
	WTR->ActWeekNumber = dinfo->weeknum;							// Assign new weeknum to records table
	SPrintf(confname,wdatname,config->ConfNum);
  FAMEStrCopy(config->DataPath,fname,255);
	AddPart(fname,confname,255);
	FAMEFillMem(&saft,0,sizeof(struct WeekTopData));
	if(rfile = Open(fname,MODE_OLDFILE))
		{
		ExamineFH(rfile,&myfib);
		entries = myfib.fib_Size / sizeof(struct WeekTopData);
 		while(i < entries)
			{
			if(Read(rfile,WTD,sizeof(struct WeekTopData))<=0) break;
			if(FAMEIsHigher64(WTD->BytesHi,WTD->BytesLow,saft.BytesHi,saft.BytesLow)==TRUE)
				{
				CopyMemQuick(WTD,&saft,sizeof(struct WeekTopData));	// New highest score found, save it.
				}
			if(FAMEIsHigher64(WTD->BytesHi,WTD->BytesLow,WTR->BestBytesHi,WTR->BestBytesLow)==TRUE)
				{
        WTR->BestBytesHi  = WTD->BytesHi;
				WTR->BestBytesLow = WTD->BytesLow;
				WTR->BestFiles		= WTD->Files;
				FAMEStrCopy(WTD->UserName,WTR->BestUserName,24);
				}
      i++;
			}
		Close(rfile);
    WTR->LastBytesHi  = saft.BytesHi;
		WTR->LastBytesLow = saft.BytesLow;
		WTR->LastFiles		= saft.Files;
		FAMEStrCopy(saft.UserName,WTR->LastUserName,24);
    }

	// Now update the record file after sending the mail and or wrote the bull

	if(!(rfile = Open(rname,MODE_READWRITE)))		// Save modified record
		{
    PutStringFormat(nodatfile,FilePart(_ProgramName),rname,IoErr());
		wb("");
		}
  if(Write(rfile,WTR,sizeof(struct WeekTopRecords)) != sizeof(struct WeekTopRecords))
		{
		Close(rfile);
    PutStringFormat(nodatfile,rname,IoErr());
		wb("");
		}
	Close(rfile);
	if(!(rfile = Open(fname,MODE_NEWFILE)))		// Truncate old week data
		{
    PutStringFormat(nodatfile,fname,IoErr());
		wb("");
		}
	Close(rfile);
	if(display==TRUE) PutString("[36mok, adding[m...",0);
	}

/**************************************************************************************************
 *  FUNCTION: SaveUserInfo()
 *   PURPOSE: Checks week data if actual user is already stored. When found, the data is updated
 *            else a new data record is append on bottom of file.
 * PARAMETER: none
 *   RETURNS: none
 **************************************************************************************************/

static void SaveUserInfo(void)
	{
	BPTR		datfile;
	char		fname[256],confname[128];
	long  	entries,i = 0;
	struct	FileInfoBlock __aligned myfib;

  if(ulstats->UserNumber == 1 && config->SysopCount==FALSE)
		{
    PutString("[36mSysop UL skipped[m...",0);
		return;
    }
	SPrintf(confname,wdatname,config->ConfNum);
  FAMEStrCopy(config->DataPath,fname,255);
	AddPart(fname,confname,255);
	if(!(datfile = Open(fname,MODE_READWRITE)))
		{
		PutStringFormat("[37mFAIL!!!\n\r\n\r[37mCannot open data file %s (%ld)\n\r",fname,IoErr());
		wb("");
		}
	ExamineFH(datfile,&myfib);
  entries = myfib.fib_Size / sizeof(struct WeekTopData);
	while(i < entries)
		{
		if(Read(datfile,WTD,sizeof(struct WeekTopData))<=0) break;
		if(WTD->UserNumber == ulstats->UserNumber)
			{
			Seek(datfile,i * sizeof(struct WeekTopData),OFFSET_BEGINNING);
			FAMEAdd64(ulstats->BytesHi,ulstats->BytesLo,&WTD->BytesHi);
      WTD->Files+=ulstats->filecount;
      if(Write(datfile,WTD,sizeof(struct WeekTopData)) != sizeof(struct WeekTopData))
				{
				PutStringFormat(nodatfile,fname,IoErr());
				Close(datfile);
				wb("");
				}
			Close(datfile);
			return;
			}
		i++;
		}

	// We did not find the User, so append him at the end of our list

	Seek(datfile,0L,OFFSET_END);
	FAMEMemSet(WTD,0,sizeof(struct WeekTopData));
  WTD->UserNumber = ulstats->UserNumber;
	WTD->BytesHi    = ulstats->BytesHi;
	WTD->BytesLow		= ulstats->BytesLo;
	WTD->Files			= ulstats->filecount;
	FAMEStrCopy(ulstats->UserName,WTD->UserName,24);
	FAMEStrCopy(ulstats->UserLocation,WTD->Location,27);
  if(Write(datfile,WTD,sizeof(struct WeekTopData)) != sizeof(struct WeekTopData))
		{
		PutStringFormat(nodatfile,fname,IoErr());
		i = -100;
		}
	Close(datfile);
	if(i == -100) wb("");
	}

/**************************************************************************************************
 *  FUNCTION: WriteStatsToMail()
 *   PURPOSE: Writes statistics as EALL message for the current conference
 *   RETURNS: none
 **************************************************************************************************/

static void WriteStatsToMail(void)
	{
	BPTR	 	tfile,rfile;
  struct 	FAMEMailHeader *fmh;
	char	 	tmpname[128],confname[128],fname[256];
	struct  FileInfoBlock __aligned myfib;
	long		entries,alloc,i = 0;

	if(config->WriteMail!=TRUE && !*config->BullPath) return;
	SPrintf(tmpname,"T:msg%lx",FindTask(NULL));
	if(*config->ConfHeader)
		{
		if(FAMEDosMove(config->ConfHeader,tmpname,32768,FDMF_NODELETE)==FALSE)
			{
			Fault(IoErr(),NULL,confname,126);
			PutStringFormat("\n\n\r[36mError copying header: %s\n\r",confname);
			wb("");
			}
    }
	if(!(tfile = Open(tmpname,MODE_READWRITE)))
		{
		wb("[37mCannot create/Open temporary file!!![m\n\r");
		}
 	Seek(tfile,0L,OFFSET_END);
	FPrintf(tfile,"[35maCID-tOP[m [mv%s [36mby SieGeL[m/[36mtRSi [m([36mtRSi[m-[36miNNOVATiONs[m) %6s [36mCurrent week[m: [32m([33m%02ld[34m/[33m%ld[32m)\n\r",COMPILE_VERSION," ",dinfo->weeknum,dinfo->maxweeks);
	SPrintf(confname,wdatname,config->ConfNum);
  FAMEStrCopy(config->DataPath,fname,255);
	AddPart(fname,confname,255);
	GetCommand(confname,0,0,0,SR_ConfName);
	FPrintf(tfile,infoline,confname,7,0,"days");
	FPuts(tfile,theader);
	FPuts(tfile,separator);
	if(!(rfile=Open(fname,MODE_OLDFILE)))
		{
		ShowEmptyStats(tfile);
		ShowFooter(tfile);
    }
	else
		{
		ExamineFH(rfile,&myfib);
		entries = myfib.fib_Size / sizeof(struct WeekTopData);
		if(!entries)
			{
			ShowEmptyStats(tfile);
			ShowFooter(tfile);
			}
		else
			{
			if(entries < config->MaxUsers) alloc = config->MaxUsers;
			else alloc = entries;
			if(!(list=AllocPooled(mem_pool,sizeof(struct WeekTopData)*alloc)))
				{
				Close(tfile);
				Close(rfile);
				DeleteFile(tmpname);
				wb(NO_MEM);
				}
			Seek(rfile,0L,OFFSET_BEGINNING);
  		while(i < entries)
				{
    		if(Read(rfile,WTD,sizeof(struct WeekTopData))<=0) break;
				CopyMemQuick(WTD,&list[i],sizeof(struct WeekTopData));
				FAMEAdd64(WTD->BytesHi,WTD->BytesLow,&wtotals.BytesHi);
				wtotals.Files+=WTD->Files;
				i++;
				}
			tempbuf = list;
			myquicksort(0,alloc-1);
			ShowEntries(alloc,tfile);
			ShowFooter(tfile);
			FreePooled(mem_pool,list,sizeof(struct WeekTopData)*alloc);
			FAMEFillMem(&wtotals,0,sizeof(struct WeeklyTotals));
      }
		}
	Close(rfile);
	Close(tfile);

	// Message is constructed in T:msgXXXXXXXX - Now send it through FAME:

	if(config->WriteMail)
		{
		if(!(fmh = (struct FAMEMailHeader *)AllocPooled(mem_pool,sizeof(struct FAMEMailHeader))))
			{
			DeleteFile(tmpname);
			wb(NO_MEM);
			}
		FAMEStrCopy("EALL",fmh->fmah_ToName,32);
		FAMEStrCopy("SYSOP",fmh->fmah_FromName,32);
	  SPrintf(fname,"UL Stats Week %02ld / %ld",dinfo->wnumrecords,dinfo->year);
	  FAMEStrCopy(fname,fmh->fmah_Subject,32);
		fmh->fmah_MsgStatus = TRUE;
		fmh->fmah_Private = FALSE;
		MyFAMEDoorMsg->fdom_StructDummy1 = fmh;
	  PutCommand(tmpname,config->ConfNum,0,0,RD_SaveMsgFile);
		}
	if(*config->BullPath)
		{
		if(FAMEDosMove(tmpname,config->BullPath,32768L,FDMF_NODELETE)==FALSE)
			{
			Fault(IoErr(),NULL,confname,126);
			PutStringFormat("\n\n\r[36mError copying to bull: %s\n\r",confname);
			wb("");
			}
		}
	DeleteFile(tmpname);
	if(MyFAMEDoorMsg->fdom_Data2)
		{
		PutStringFormat("[36mERROR OCCURED WHILE SENDING MAIL: %ld\n\r",MyFAMEDoorMsg->fdom_Data2);
		}
	}

/**************************************************************************************************
 *  FUNCTION: AddFTPUploads()
 *   PURPOSE: Checks for existence of FAME-FTPd data files and add them, when found. (V1.1+)
 * PARAMETER: none
 *   RETURNS: none
 **************************************************************************************************/

void AddFTPUploads(void)
	{



	}
