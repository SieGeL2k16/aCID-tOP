/****************************************************************************
 *    FILE: support.c
 * PURPOSE: Contains support functions required for aCID-tOP
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

static char *cfgstrings[] = {	"DATA_PATH",
															"CONF_%ld_MAXUSERS",
															"CONF_%ld_HEADER",
															"CONF_%ld_WRITEMAIL",
															"CONF_%ld_SYSOPCOUNT",
															"CONF_%ld_BULLPATH"
														};

#define MAX_CHECKS sizeof(cfgstrings) / sizeof(char *)

extern char *_ProgramName;

enum { CFG_DATAPATH=0, CFG_MAXUSERS, CFG_HEADER, CFG_WRITEMAIL,CFG_SYSOPCOUNT, CFG_BULLPATH };

/**************************************************************************************************
 * Prototype declarations:
 **************************************************************************************************/

int 		GetWeekNum(void);
int			ReadConfig(struct Config *);
void 		ShowHeader(BOOL);

static 	long CheckTheLine(char *testline);

/**************************************************************************************************
 *  FUNCTION: GetWeekNum()
 *   PURPOSE: Determines current week number and current weekdate via date.library
 * PARAMETER: none
 *   RETURNS: The weeknumber or 0 to indicate an error
 **************************************************************************************************/

int GetWeekNum(void)
	{
	struct 	DateStamp *myds;
	struct 	DateTime  *mydt;
	char		currdate[LEN_DATSTRING];
	char		dbuf[4],mbuf[4],ybuf[6];
	long		i;

	myds = AllocPooled(mem_pool,sizeof(struct DateStamp));
	mydt = AllocPooled(mem_pool,sizeof(struct DateTime));
	if(!myds || !mydt) return(0);
  myds = DateStamp(myds);
  mydt->dat_Stamp 	= *myds;
  mydt->dat_Format 	= FORMAT_USA;
	mydt->dat_StrDate	= currdate;
	DateToStr(mydt);
	FreePooled(mem_pool,myds,sizeof(struct DateStamp));
	FreePooled(mem_pool,mydt,sizeof(struct DateTime));
	FAMEStrMid(currdate,mbuf,1,2);
	FAMEStrMid(currdate,dbuf,4,2);
	FAMEStrMid(currdate,ybuf,7,-1);
	dinfo->day 				= FAMEAtol(dbuf);
	dinfo->month 			= FAMEAtol(mbuf);
	dinfo->year  		 	= FAMEAtol(ybuf);
	if(dinfo->year < 70) dinfo->year+= 2000;
	else dinfo->year+=1900;
	date_FormatDate("%Dwn",dinfo->day,dinfo->month,dinfo->year,date_Locale,currdate,date_Gregorian);
	dinfo->dayofweek = FAMEAtol(currdate);
	dinfo->daysleft  = 7-dinfo->dayofweek;
	dinfo->weeknum   = date_Week(dinfo->day,dinfo->month,dinfo->year,date_Gregorian);
	for(i = 31; i > 27; i--)
		{
		dinfo->maxweeks  = date_Week(i,12,dinfo->year,date_Gregorian);
		if(dinfo->maxweeks!=1) break;
    }
	}

/**************************************************************************************************
 *  FUNCTION: ReadConfig()
 *   PURPOSE: Tries to read the config from FAME:ExternEnv/Doors/
 * PARAMETER: none
 *   RETURNS: TRUE 	=> Config OK
 *            FALSE => Error in Config
 *      NOTE: Uses FAME Commands! So has to be called AFTER FAMEStart() !
 **************************************************************************************************/

int ReadConfig(struct Config *c)
	{
	BPTR		prefspointer;
	struct	RDArgs *myargs;
	struct	CSource mysource;
	char		mybuf[256],buf2[256];
	long    retvar, myarray[1]={0L};

	GetCommand("",0,0,0,NR_GetConfNum);
	c->ConfNum=MyFAMEDoorMsg->fdom_Data2;
	if(!(prefspointer=Open("FAME:ExternEnv/Doors/aCID-tOP.cfg",MODE_OLDFILE)))
		{
		PutStringFormat("\n\r[37m%s: Unable to open config file FAME:ExternEnv/Doors/aCID-tOP.cfg![m\n\r",FilePart(_ProgramName));
		return(-1);
		}
	if(!(myargs=AllocDosObject(DOS_RDARGS, NULL)))
		{
		Close(prefspointer);
		PutString(NO_MEM,1);
		return(-1);
		}
	SetVBuf(prefspointer,NULL,BUF_LINE,1024);
  while(FGets(prefspointer,mybuf,255))
		{
		retvar = CheckTheLine(mybuf);
		if(retvar < 0)
			{
			continue;
			}
		SPrintf(buf2,mybuf,c->ConfNum);
		mysource.CS_Buffer=buf2;
		mysource.CS_Length=strlen(buf2);
		mysource.CS_CurChr=0;
		myargs->RDA_Source=mysource;
		myargs->RDA_Flags=RDAF_NOPROMPT;
		switch(retvar)
			{
			case	CFG_MAXUSERS:	FAMEStrCat("/N",buf2);
													break;
			}
		if(!(myargs=ReadArgs(buf2,myarray,myargs)))
			{
			Close(prefspointer);
			FreeDosObject(DOS_RDARGS,myargs);
			PutStringFormat("\n\r[37mCONFIG-ERROR: Entry [35m%s[37m has a wrong value !!!\n\n\rDOS-Error: [32m%ld [37m - ",cfgstrings[retvar],IoErr());
			return(-1);
			}
		FAMEMemSet(mybuf,'\0',255);
		switch(retvar)
			{
			case	CFG_MAXUSERS:		c->MaxUsers=*(LONG *)myarray[0];
                          	break;
			case	CFG_DATAPATH:		SPrintf(c->DataPath,"%s",myarray[0]);
                          	break;
			case	CFG_HEADER:			SPrintf(c->ConfHeader,"%s",myarray[0]);
														break;
     	case	CFG_BULLPATH:		SPrintf(c->BullPath,"%s",myarray[0]);
                            break;
			case	CFG_SYSOPCOUNT: SPrintf(mybuf,"%s",myarray[0]);
														if(!Stricmp(mybuf,"Yes")) c->SysopCount = TRUE;
														else c->SysopCount = FALSE;
														break;
			case	CFG_WRITEMAIL:	SPrintf(mybuf,"%s",myarray[0]);
														if(!Stricmp(mybuf,"Yes")) c->WriteMail = TRUE;
														else c->WriteMail = FALSE;
														break;
			}
		FreeArgs(myargs);
		}
	Close(prefspointer);
	FreeDosObject(DOS_RDARGS,myargs);
	prefspointer = Lock(c->DataPath,ACCESS_READ);
	if(prefspointer)
		{
		UnLock(prefspointer);
		}
  else
		{
		PutStringFormat("\n\r[37m%s: Data dir %s does not exist!!\n\r",FilePart(_ProgramName),c->DataPath);
		return(-1);
		}
	if(!c->MaxUsers)
		{
		return(-2);
    }
	return(1);
	}

/*****************************************************************************
 *  FUNCTION: CheckTheLine()
 *   PURPOSE: Checks *testline for a known keyword, returns offset for
 *            checkarray or -1 to indicate that the current line is no config
 *            line !
 * PARAMETER: current line as returned from FGets()
 *    RETURN: Either array offset or -1 to indicate no data found
 *****************************************************************************/

static long CheckTheLine(char *testline)
	{
	register long	lv;
	char     buf[80];

	for(lv=0;lv<MAX_CHECKS;lv++)
    {
  	SPrintf(buf,cfgstrings[lv],config->ConfNum);
		if(!(Strnicmp(testline,buf,strlen(buf))))
			{
			return(lv);
			}
		}
	return(-1);
	}

/*****************************************************************************
 *  FUNCTION: ShowHeader()
 *   PURPOSE: If a headerfile is defined, it will be displayed
 *            All Error conditions are handled here, just call this function.
 * PARAMETER: TRUE  => Search&Display of Conf header
 *            FALSE => Only the logo is displayed (update mode!)
 *    RETURN: none
 *****************************************************************************/

void ShowHeader(BOOL full)
	{
	BPTR	hfile;
	char	fbuf[256];

	if(full==TRUE)
		{
		if(*config->ConfHeader)
			{
    	if(hfile = Open(config->ConfHeader,MODE_OLDFILE))
				{
				while(FGets(hfile,fbuf,255))
					{
      	  PutStringFormat("%s\r",fbuf);
					}
    	  Close(hfile);
				PutString("",1);
				}
			}
		else PutString("",1);
		}
	else PutString("",1);
	if(full==TRUE)	
		{
		PutStringFormat("[35maCID-tOP[m [mv%s [36mby SieGeL[m/[36mtRSi [m([36mtRSi[m-[36miNNOVATiONs[m) %6s [36mCurrent week[m: [32m([33m%02ld[34m/[33m%ld[32m)\n\r",COMPILE_VERSION," ",dinfo->weeknum,dinfo->maxweeks);
		}
	else
		{
		PutStringFormat("[35maCID-tOP[m [mv%s [36mby SieGeL[m/[36mtRSi [m([36mtRSi[m-[36miNNOVATiONs[m)\n\r",COMPILE_VERSION);
		}
	}

