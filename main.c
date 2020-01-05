/****************************************************************************
 * aCID-tOP by Sascha 'SieGeL' Pfalz of (tRSi-iNNOVATiONs)
 *
 * Weektop Tool for FAME with 64-Bit support for upload counting - based
 * on dR.dRE's weekTop Tool
 *
 * (c) 2003-2004 by (tRSi-iNNOVATiONs)
 ****************************************************************************/

#include <exec/exec.h>
#include <Fame/fameDoorProto.h>
#include <pragmas/utility_pragmas.h>
#include <pragmas/date_pragmas.h>
#include <proto/locale.h>
#include <proto/date.h>
#include <libraries/date.h>
#include <utility/date.h>
#include <utility/tagitem.h>
#include "version.h"
#include "struct.h"
#include "proto.h"

extern char *_ProgramName;
const char *ver="$VER: aCID-tOP "COMPILE_VERSION" ("COMPILE_DATE") ["CPU_TYPE"]\0";

LONG __stack = 32768L;

/****************************************************************************
 * Prototypes
 ****************************************************************************/

void main(void);
void __autoopenfail(void) { _XCEXIT(0);}	// Dummy function for SAS
void CloseLibs(void);
void ShutDown(long error);
void wb(char *err);
long CheckCall(void);

/****************************************************************************
 * Main entry point:
 ****************************************************************************/

void main(void)
	{
	struct 	RDArgs *rda=NULL;
	long    ArgArray[1]={0L};

	if(rda=ReadArgs("NODE-NR./A/N",ArgArray,rda))
		{
		node=*(LONG *)ArgArray[0];
		SPrintf((STRPTR)FAMEDoorPort,"FAMEDoorPort%ld",node);
		FreeArgs(rda);
		}
	else
		{
		FreeArgs(rda);
		PrintFault(IoErr(),(FilePart(_ProgramName)));
		Printf("\n%s is a FAME BBS-Door and is only usable via the BBS !\n\n",(FilePart(_ProgramName)));
		exit(0);
		}
	if(!(UtilityBase=(struct Library *) 	OpenLibrary("utility.library",37L))) { SetIoErr(ERROR_INVALID_RESIDENT_LIBRARY);exit(20);}
	if(!(FAMEBase=(struct FAMELibrary *) 	OpenLibrary(FAMENAME,5L))) { SetIoErr(ERROR_INVALID_RESIDENT_LIBRARY);CloseLibs();}
	if(!(LocaleBase=(struct LocaleBase *)	OpenLibrary("locale.library",38L))) { SetIoErr(ERROR_INVALID_RESIDENT_LIBRARY);CloseLibs();}
	myloc=OpenLocale(NULL);
	if(!(mem_pool = CreatePool(MEMF_CLEAR|MEMF_PUBLIC,10240L,10240L))) { SetIoErr(ERROR_NO_FREE_STORE);CloseLibs();}
	if(FIMStart(0,0UL)) CloseLibs();
	if(!(DateBase=OpenLibrary(DATE_NAME,33L)))
		{
    wb("\n\r[37mCannot open date.library V33+!!!\n\r");
		}
	if(!(WTR 		= AllocPooled(mem_pool,sizeof(struct WeekTopRecords)))) wb(NO_MEM);
	if(!(WTD 		= AllocPooled(mem_pool,sizeof(struct WeekTopData)))) wb(NO_MEM);
	if(!(config = AllocPooled(mem_pool,sizeof(struct Config)))) wb(NO_MEM);
  if(!(dinfo  = AllocPooled(mem_pool,sizeof(struct DateInfo)))) wb(NO_MEM);
	if(!(ulstats= AllocPooled(mem_pool,sizeof(struct ULStats)))) wb(NO_MEM);
	if(ReadConfig(config)<=0)
		{
    wb("");	// Either an error or conf should not be used, skipping
		}
  GetWeekNum();
	switch(CheckCall())
		{
    case	0:  ShowStats();
							break;
		case	1:  UpdateStats();
							break;
		}
	wb("");
	}

/****************************************************************************
 *  FUNCTION: CheckCall()
 *   PURPOSE: Tries to determine how we are called. If called as normal door
 *            we have to output the statistics.
 *            If called from ULSTAT syscmd, add data to our weekly structs
 * PARAMETER: none
 *   RETURNS: 0 => Called as Door
 *            1 => Called from ULSTATS
 *            2 => ERROR
 ****************************************************************************/

long	CheckCall(void)
	{
	struct 	RDArgs *myargs;
  struct 	CSource mysource;
	long   	myarray[10]={0L,0L,0L,0L,0L,0L,0L,0L,0L,0L};
	char		mline[202],testbuf[202];
	enum 		{ARG_ULSTAT=0,ARG_FILES,ARG_BYTES,ARG_CPS,ARG_EFF,ARG_HOLDS,ARG_HOLDBYTES,ARG_MODE,ARG_INT,ARG_DEL};

	GetCommand(mline,0,0,0,NR_MainLine);
	FAMEStrMid(mline,testbuf,1,6);
	if(Strnicmp(testbuf,"ULSTAT",6)) return(0);
	strcat(mline,"\n");
	if(!(myargs=AllocDosObject(DOS_RDARGS,NULL))) wb(NO_MEM);
	mysource.CS_Buffer=mline;
  mysource.CS_Length=strlen(mline);
	mysource.CS_CurChr=0;
	myargs->RDA_Source=mysource;
	myargs->RDA_Flags=RDAF_NOPROMPT;
	if(!ReadArgs("ULSTAT,FILES/N/A,BYTES/N/A,CPS/N/A,EFF/N/A,HOLDS/N/A,HOLDBYTES/N/A,MODE/N/A,INT/N/A,DEL/N/A",myarray,myargs))
		{
		FreeDosObject(DOS_RDARGS,myargs);
		return(0);
		}
	ulstats->filecount	=*(LONG *)myarray[ARG_FILES];
	ulstats->mode				=*(LONG *)myarray[ARG_MODE];
	ulstats->internal		=*(LONG *)myarray[ARG_INT];
	ulstats->BytesLo		=*(LONG *)myarray[ARG_BYTES];
	FreeDosObject(DOS_RDARGS,myargs);
	if(!ulstats->filecount) return(-1);
	switch(ulstats->mode)
		{
		case ULSTAT_UPLOAD_UL:
		case ULSTAT_UPLOAD_UG:
		case ULSTAT_UPLOAD_U:
		case ULSTAT_UPLOAD_RZ:
		case ULSTAT_UPLOAD_PARTUL_RESUME:
		case NORMAL_DL_XFER:							return(1);
    default:													return(2);
		}
	}

/****************************************************************************
 *  FUNCTION: wb()
 *   PURPOSE: Terminates the FAMEDoor session
 * PARAMETER: none
 *   RETURNS: none
 ****************************************************************************/

void wb(char *err)
	{
	if(*err) NC_PutString(err,1);
	PutStringFormat("[m\n\r");
	FIMEnd(0);
	}

/****************************************************************************
 *   FUNCTION: ShutDown()
 *    PURPOSE: Called by FAME Door header to shutdown door, calls CloseLibs()
 * PARAMETERS: error => Error code from FAME, see FAMEDefines.h
 *    RETURNS: none
 ****************************************************************************/

void ShutDown(long error)
	{
	CloseLibs();
	}

/****************************************************************************
 *  FUNCTION: CloseLibs()
 *   PURPOSE: Closes all libs and frees all resources
 * PARAMETER: none
 *   RETURNS: none
 ****************************************************************************/

void CloseLibs(void)
	{
	if(UtilityBase)		CloseLibrary(UtilityBase); UtilityBase=NULL;
  if(LocaleBase)
		{
		if(myloc) CloseLocale(myloc);myloc=NULL;
		CloseLibrary((struct Library *)LocaleBase);LocaleBase=NULL;
		};
	if(FAMEBase) 			CloseLibrary((struct Library *)FAMEBase); FAMEBase=NULL;
	if(DateBase)			CloseLibrary(DateBase); DateBase=NULL;
	if(mem_pool)      DeletePool(mem_pool); mem_pool = NULL;
	exit(0);
	}
