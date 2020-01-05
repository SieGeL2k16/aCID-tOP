/*
 *   aCID-eDIT - Small editor for aCID-tOP Data files.
 *   (c) 2003 by Sascha 'SieGeL' Pfalz
 */

#include <proto/fame.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <pragmas/locale_pragmas.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <stdlib.h>
#include <strings.h>
#include <clib/utility_protos.h>
#include "/proto.h"
#include "/struct.h"
#include "version.h"

extern struct	 ExecBase *SysBase;
extern char *_ProgramName;

const 	char *ver="$VER: aCID-eDIT "COMPILE_VERSION" ("COMPILE_DATE") ["CPU_TYPE"]\0";
static 	char wrecname[] = "conf_%ld_records.dat";

/******************************************************************************
 * PROTOTYPES
 ******************************************************************************/

void 	main(void);
void CloseLibs(long err);

static void MainMenu(char *dp);
static void EditRecords(char *dp);
VOID __stdargs SPrintf(STRPTR buffer, char *ctl, ...);
BOOL AskUser(char *question, char buffer[], char *defstr,long buflen);

/****************************************************************************
 *  FUNCTION: main()
 *   PURPOSE: Main entry point
 * PARAMETER: none
 *   RETURNS: none
 ****************************************************************************/

void main(void)
	{
	BPTR	prefs;
	static char pfile[] = "FAME:ExternEnv/Doors/aCID-tOP.cfg";
	char	buffer[202],datapath[256];

  if(SysBase->LibNode.lib_Version<39)
		{
		Printf("\n\n%s requires Kickstart V3.x to run !\n\n",FilePart(_ProgramName));
		return;
		}
	if(!(FAMEBase=(struct FAMELibrary *) 	OpenLibrary(FAMENAME,0))) 		{ SetIoErr(ERROR_INVALID_RESIDENT_LIBRARY);CloseLibs(IoErr());}
	if(!(UtilityBase=(struct Library *) 	OpenLibrary("utility.library",37L))) { SetIoErr(ERROR_INVALID_RESIDENT_LIBRARY);CloseLibs(IoErr());}
	if(!(mem_pool = CreatePool(MEMF_CLEAR|MEMF_PUBLIC,10240L,10240L)))  { SetIoErr(ERROR_NO_FREE_STORE);CloseLibs(IoErr());}
	if(!(prefs = Open(pfile,MODE_OLDFILE)))
		{
		Printf("\n\nERROR: Cannot open preferences file %s !!!\n\n",pfile);
		CloseLibs(0);
    }
	FAMEMemSet(datapath,'\0',255);
	while(FGets(prefs,buffer,200))
		{
		if(FAMEStrStr(buffer,"DATA_PATH="))
			{
			FAMEStrMid(buffer,datapath,11,-1);
			}
		}
	Close(prefs);
	if(!*datapath)
		{
		Printf("\n\nERROR: No DATA_PATH tag found in preferences, aborting!\n\n");
		CloseLibs(0);
		}
	datapath[strlen(datapath)-1] = '\0';	// Cut off trailing '\n'
	if(!(WTR = AllocPooled(mem_pool,sizeof(struct WeekTopRecords))))
		{
    Printf("Cannot allocate memory for Records struct??\n\n");
		CloseLibs(IoErr());
		}
	MainMenu(datapath);
	Printf("\n");
	CloseLibs(0);
	}

/****************************************************************************
 *  FUNCTION: CloseLibs()
 *   PURPOSE: Closes all libs and frees all resources
 * PARAMETER: long err => IoErr() code to print or 0 to not print anything
 *   RETURNS: none
 ****************************************************************************/

void CloseLibs(long err)
	{
	if(err) PrintFault(err,FilePart(_ProgramName));
	if(FAMEBase) 			CloseLibrary((struct Library *)FAMEBase); FAMEBase=NULL;
	if(UtilityBase)		CloseLibrary(UtilityBase); UtilityBase=NULL;
	if(mem_pool)      DeletePool(mem_pool); mem_pool = NULL;
	exit(0);
	}

/****************************************************************************
 *  FUNCTION: MainMenu()
 *   PURPOSE: Shows the main menu of aCID-eDIT, main loop of program
 * PARAMETER: char *dp => Path where aCID-tOP datafiles are stored.
 *   RETURNS: none
 ****************************************************************************/

static void MainMenu(char *dp)
	{
	long	result;

  while(1)
  	{
    if(CheckSignal(SIGBREAKF_CTRL_C)) return;
		Printf("\f\naCID-eDIT v%s by SieGeL/tRSi - Editor for aCID-tOP data files\n\n",COMPILE_VERSION);
		Printf("Using Datapath: %s\n\n",dp);
  	Printf("Choose action:\n\n[1] = Edit Records\n[0] = Quit aCID-eDIT\n\n");
		Printf("Please make your choice :> ");
		Flush(Output());
		result = FGetC(Input());
    Flush(Input());
		switch((char)result)
			{
			case	'0': 	return;
			case	'1':	EditRecords(dp);
			}
		}
	}

/****************************************************************************
 *  FUNCTION: EditRecords()
 *   PURPOSE: Shows the main menu of aCID-eDIT, main loop of program
 * PARAMETER: char *dp => Path where aCID-tOP datafiles are stored.
 *   RETURNS: none
 ****************************************************************************/

static void EditRecords(char *dp)
	{
	char	buf[80],fname[256],confname[128],dummy[80];
	long	confnum;
	BPTR	rfile;

	Printf("\nEnter Conf number for which to edit the records (0 = back) :> ");
	Flush(Output());
	FGets(Input(),buf,3);
	confnum = FAMEAtol(buf);
	if(!confnum) return;
	FAMEFillMem(WTR,0,sizeof(struct WeekTopRecords));
	SPrintf(confname,wrecname,confnum);
  FAMEStrCopy(dp,fname,255);
	AddPart(fname,confname,255);
  if(rfile = Open(fname,MODE_OLDFILE))
		{
		Read(rfile,WTR,sizeof(struct WeekTopRecords));
		Close(rfile);
		FPuts(Output(),"\nUsing existing record file.\n\n");
		}
	else FPrintf(Output(),"\nNo record data found for Conf %ld, using empty record file.\n\n",confnum);

	SPrintf(dummy,"%ld",WTR->ActWeekNumber);
	if(AskUser("       Week Number",buf,dummy,4)==FALSE) CloseLibs(IoErr());
	WTR->ActWeekNumber = FAMEAtol(buf);

	if(AskUser("Username last week",WTR->LastUserName,WTR->LastUserName,32)==FALSE) CloseLibs(IoErr());

  FAMENum64ToStr(WTR->LastBytesHi,WTR->LastBytesLow,NULL,40,dummy);
	if(AskUser("   Bytes last week",buf,dummy,80)==FALSE) CloseLibs(IoErr());
	FAMEStrTo64(buf,&WTR->LastBytesHi,&WTR->LastBytesLow);

  SPrintf(dummy,"%ld",WTR->LastFiles);
	if(AskUser("   Files last week",buf,dummy,80)==FALSE) CloseLibs(IoErr());
	WTR->LastFiles = FAMEAtol(buf);

	if(AskUser(" Username all time",WTR->BestUserName,WTR->BestUserName,32)==FALSE) CloseLibs(IoErr());

  FAMENum64ToStr(WTR->BestBytesHi,WTR->BestBytesLow,NULL,40,dummy);
	if(AskUser("    Bytes all time",buf,dummy,80)==FALSE) CloseLibs(IoErr());
	FAMEStrTo64(buf,&WTR->BestBytesHi,&WTR->BestBytesLow);

  SPrintf(dummy,"%ld",WTR->BestFiles);
	if(AskUser("    Files all time",buf,dummy,80)==FALSE) CloseLibs(IoErr());
	WTR->BestFiles = FAMEAtol(buf);

	// Dumping data and ask if we should save now the contents:


  FPrintf(Output(),"\n\nDumping new values for Records in Conference %ld:\n\n",confnum);
	FPrintf(Output(),"       Week Number: %ld\n",WTR->ActWeekNumber);
	FPrintf(Output(),"Username last week: %s\n",WTR->LastUserName);
	FAMENum64ToStr(WTR->LastBytesHi,WTR->LastBytesLow,FNSF_GROUPING|FNSF_NUMLOCALE,80,dummy);
  FPrintf(Output(),"   Bytes last week: %s\n",dummy);
	FAMENumToStr(WTR->LastFiles,FNSF_GROUPING|FNSF_NUMLOCALE,80,dummy);
	FPrintf(Output(),"   Files last week: %s\n",dummy);

	FPrintf(Output()," Username all time: %s\n",WTR->BestUserName);
	FAMENum64ToStr(WTR->BestBytesHi,WTR->BestBytesLow,FNSF_GROUPING|FNSF_NUMLOCALE,80,dummy);
  FPrintf(Output(),"    Bytes all time: %s\n",dummy);
	FAMENumToStr(WTR->BestFiles,FNSF_GROUPING|FNSF_NUMLOCALE,80,dummy);
	FPrintf(Output(),"    Files all time: %s\n",dummy);

  FPrintf(Output(),"\nOkay to save new values (y/n) ? ");
	Flush(Output());
  if(ToUpper((char)FGetC(Input()))=='N') return;
	Flush(Input());

	if(!(rfile = Open(fname,MODE_NEWFILE)))
		{
		PrintFault(IoErr(),"Open recordfile");
		CloseLibs(0);
		}
	if(Write(rfile,WTR,sizeof(struct WeekTopRecords)) != sizeof(struct WeekTopRecords))
		{
    PrintFault(IoErr(),"Save recordfile");
		Close(rfile);
		CloseLibs(0);
		}
	Close(rfile);
  FPuts(Output(),"\nSuccessfully saved records.\n\n");
	Delay(100);
	}

/****************************************************************************
 *  FUNCTION: AskUser()
 *   PURPOSE: Prompts user to enter some informations
 * PARAMETER: char *question => Question to display
 *            char *buffer   => Buffer to hold entered value
 *            char *defstr   => Default value to use if no data is entered
 *            long buflen    => Maximum size of buffer
 *   RETURNS: TRUE  => Successful
 *            FALSE => Error while reading from Output() - should never happen!
 ****************************************************************************/

BOOL AskUser(char *question, char buffer[], char *defstr,long buflen)
	{
  char	ubuf[256],retcodes[108];
	long	res;

  for(res=0; res < strlen(defstr); res++) retcodes[res] = '\b';
	retcodes[res]='\0';
  FPrintf(Output(),"%s :> %s%s",question,defstr,retcodes);
	Flush(Output());
	Flush(Input());
	SetIoErr(0);
	FAMEMemSet(ubuf,'\0',255);
	FGets(Input(),ubuf,255);
	if(IoErr())
		{
    PrintFault(IoErr(),"\n\nLINE-ERROR:");
		return(FALSE);
		}
  ubuf[strlen(ubuf)-1] = '\0';
	if(*ubuf) FAMEStrCopy(ubuf,buffer,buflen);
	else FAMEStrCopy(defstr,buffer,buflen);
	return(TRUE);
	}

/****************************************************************************
 *  FUNCTION: SPrintf()
 *   PURPOSE: Replacement for sprintf() using exec/RawDoFmt()
 * PARAMETER: See RawDoFmt() for description of Format codes.
 *   RETURNS: none
 ****************************************************************************/

VOID __stdargs SPrintf(STRPTR buffer, char *ctl, ...)
	{
	RawDoFmt(ctl, (long *)(&ctl + 1), (void (*))"\x16\xc0\x4e\x75",buffer);
	}
