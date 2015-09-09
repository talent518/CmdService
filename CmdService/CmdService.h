#ifndef CMD_SERVICE_H
#define CMD_SERVICE_H

#include "BaseService.h"

typedef struct _execParam{
	TCHAR keyName[MAX_PATH];
	TCHAR curDirectory[MAX_PATH];
	TCHAR cmdLine[MAX_PATH];
	TCHAR logFile[MAX_PATH];

	HANDLE processId;
	HANDLE hToken;

	struct _execParam *next;
} EXECPARAM, *PEXECPARAM;

class CmdService : public BaseService {
public:
	CmdService();
	virtual void OnStop();
	virtual void OnPause();
	virtual void OnContinue();

	virtual void Run();

	TCHAR m_szConfigFile[MAX_PATH];
	TCHAR m_szLogFile[MAX_PATH];
private:
	volatile bool m_bPaused;
	volatile bool m_bRunning;

	PEXECPARAM pHeadExecParam;
	PDWORD dwThreadId;
	PHANDLE hThread;
	int nCmdSize;
};

extern CmdService cmdService;

#endif/*CMD_SERVICE_H*/