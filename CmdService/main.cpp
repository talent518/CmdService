#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <Shlwapi.h>
#include "CmdService.h"
#include "ServiceCtrl.h"
#include "getopt.h"
#include "api.h"

CmdService cmdService;

static const opt_struct OPTIONS[] = {
	{'h', 0, "help"},
	{'?', 0, "usage"},/* help alias (both '?' and 'usage') */
	{'v', 0, "version"},

	{'c', 1, "ctrl"},
	{'n', 1, "name"},
	{'f', 1, "file"},
	{'l', 1, "log"},

	{'-', 0, NULL} /* end of args */
};

static void usage(const char *argv0)
{
	const char *prog;

	prog = strrchr(argv0, '/');
	if (prog) {
		prog++;
	} else {
		prog = "php";
	}
	
	printf( "Usage: %s [options]\n"
	    "\n"
		"  confile file format:\n"
		"    [Command]\n"
		"    <log suffix>=<command line>\n"
		"    [Directory]\n"
		"    <log suffix>=<command line current directory>\n"
		"    [Environment]\n"
		"    <log suffix>=<environment variable>\n"
		"  options:\n"
		"    -h,--help,-?,--usage    This help\n"
		"    -v                      Version number\n"
		"\n"
		"    -n <service name>       set service name\n"
		"    -f <config file>        set service config file\n"
		"    -l <log file>           set service log file\n"
		"    -c install              install service\n"
		"    -c unstall              unstall service\n"
		"    -c start                start service\n"
		"    -c stop                 stop service\n"
		"    -c restart              restart service\n"
		"    -c status               service status\n"
		, prog);
}

int main(int argc, char *argv[]) {
	char *optarg = NULL;
	char *servName = NULL;
	char *signalArg = NULL;
	char *configFile = NULL;
	int optind = 1;
	int c;
	char *p;

	ServiceCtrl serviceCtrl;

	TCHAR curPath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH-1,curPath);

	if(argc==1){
		usage(argv[0]);
		exit(0);
	}

	while ((c = getopt(argc, argv, OPTIONS, &optarg, &optind, 0, 2)) != -1) {
		switch (c) {
			case 'h': /* help & quit */
			case '?':
				usage(argv[0]);
				exit(0);
			case 'v':
				printf("CmdService %s", GetFileVersion(argv[0]));
				return 0;
			case 'c': /* service control */
				signalArg = strdup(optarg);
				break;
			case 'n': /* service name*/
			   _tcsncpy_s(cmdService.m_szServiceName, optarg, MAX_PATH - 1);
			   break;
			case 'f': /* config file */
				p=optarg;
				while(*p) {
					if(*p == '/') {
						*p='\\';
					}
					p++;
				}
				PathCombine(cmdService.m_szConfigFile,curPath,optarg);
				break;
			case 'l': /* log file */
				p=optarg;
				while(*p) {
					if(*p == '/') {
						*p='\\';
					}
					p++;
				}
				PathCombine(cmdService.m_szLogFile,curPath,optarg);
				break;
		}
	}

	if(signalArg == NULL) {
		printf("Service control parameter not found, please use -c <install | uninstall | start | stop | restart | status>\n");
		return 2;
	}

	cmdService.Init();
	if(cmdService.IsInstalled()) {
		serviceCtrl.Init(cmdService.m_szServiceName);
	}

	if(!strcmpi(signalArg,"install")) {
		// Get the executable file path
		TCHAR szFilePath[MAX_PATH];
		GetModuleFileName(NULL, szFilePath, MAX_PATH);

		TCHAR binPath[MAX_PATH];

		sprintf(binPath,"\"%s\" -c runservice -n %s -f \"%s\" -l \"%s\"", szFilePath, cmdService.m_szServiceName, cmdService.m_szConfigFile, cmdService.m_szLogFile);

		printf("%s\n",binPath);

		if(cmdService.Install(binPath)) {
			printf("Install service success.\n");
			return 0;
		} else {
			printf("Install service fail.\n");
			return 3;
		}
	} else if(!strcmpi(signalArg,"uninstall")) {
		if(cmdService.Uninstall()) {
			printf("Uninstall service success.\n");
			return 0;
		} else {
			printf("Uninstall service fail.\n");
			return 3;
		}
	} else if(!strcmpi(signalArg,"start")) {
		if(serviceCtrl.Start()) {
			printf("Start service success.\n");
			return 0;
		} else {
			printf("Start service fail.\n");
			return 3;
		}
	} else if(!strcmpi(signalArg,"stop")) {
		if(serviceCtrl.Stop()) {
			printf("Stop service success.\n");
			return 0;
		} else {
			printf("Stop service fail.\n");
			return 3;
		}
	} else if(!strcmpi(signalArg,"restart")) {
		if(serviceCtrl.Stop()) {
			printf("Stop service success.\n");
		} else {
			printf("Stop service fail.\n");
		}
		if(serviceCtrl.Start()) {
			printf("Start service success.\n");
			return 0;
		} else {
			printf("Start service fail.\n");
			return 3;
		}
	} else if(!strcmpi(signalArg,"status")) {
		switch(serviceCtrl.State()) {
		case SERVICE_CONTINUE_PENDING:
			printf("The service continue is pending.\n");
			break;
		case SERVICE_PAUSE_PENDING:
			printf("The service pause is pending.\n");
			break;
		case SERVICE_PAUSED:
			printf("The service is paused.\n");
			break;
		case SERVICE_RUNNING:
			printf("The service is running.\n");
		case SERVICE_START_PENDING:
			printf("The service is starting.\n");
			break;
		case SERVICE_STOP_PENDING:
			printf("The service is stopping.\n");
			break;
		case SERVICE_STOPPED:
			printf("The service is not running.\n");
			break;
		default:
			TCHAR lpMsgBuf[1024];
			FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL
			);
			printf("%s\n",lpMsgBuf);
			return 3;
		}
	} else if(!strcmpi(signalArg,"runservice")) {
		if (cmdService.IsInstalled()) {
			if (!cmdService.Start())
				printf("The service can not run from command line.\n");
		} else {
			printf("The service is not installed.\n");
		}
	} else {
		usage(argv[0]);
		exit(0);
	}

	return 0;
}
