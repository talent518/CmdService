#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <Shlwapi.h>
#include "CmdService.h"
#include "Log.h"
#include "LogManager.h"

CmdService::CmdService() {
	GetModuleFileName(NULL, m_szConfigFile, MAX_PATH);
	GetModuleFileName(NULL, m_szLogFile, MAX_PATH);

	strcpy(strrchr(m_szConfigFile,'.')+1,"ini");
	strcpy(strrchr(m_szLogFile,'.')+1,"log");
}

void CmdService::OnStop() {
	m_bRunning = false;
}
void CmdService::OnPause() {
	m_bPaused = true;
}
void CmdService::OnContinue() {
	m_bPaused = false;
}

typedef struct _execParam{
	TCHAR keyName[MAX_PATH];
	TCHAR curDirectory[MAX_PATH];
	TCHAR cmdLine[MAX_PATH];
	TCHAR logFile[MAX_PATH];

	struct _execParam *next;
} EXECPARAM, *PEXECPARAM;

int ExecCommand(TCHAR *cmdLine, TCHAR *logFile, TCHAR *curDirectory)
{
	PROCESS_INFORMATION pi; 
	STARTUPINFO si;
	BOOL ret = FALSE; 
	DWORD flags = CREATE_NO_WINDOW;
	HANDLE h;

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	h = CreateFile(logFile,
		FILE_APPEND_DATA,
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		&sa,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL );

	ZeroMemory( &pi, sizeof(PROCESS_INFORMATION) );
	ZeroMemory( &si, sizeof(STARTUPINFO) );
	si.cb = sizeof(STARTUPINFO); 
	si.dwFlags |= STARTF_USESTDHANDLES;
	si.wShowWindow =SW_HIDE;
	si.hStdInput = NULL;
	si.hStdError = NULL;
	si.hStdOutput = NULL;

	ret = CreateProcess(NULL, cmdLine, NULL, NULL, TRUE, flags, NULL, curDirectory, &si, &pi);

	WaitForSingleObject(pi.hProcess, INFINITE );

	CloseHandle(h);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return ret;
}

DWORD WINAPI ThreadProc(LPVOID lpParam)
{
	PEXECPARAM pData;

	//Casttheparametertothecorrectdatatype.
	pData=(PEXECPARAM)lpParam;

	ExecCommand(pData->cmdLine, pData->logFile, pData->curDirectory);

	return 0;
}

void CmdService::Run() {
	m_bRunning = true;
	CLog* log=LogManager::OpenLog(m_szLogFile,CLog::LL_INFO);

	DWORD nBufferSize=32767;

	WIN32_FIND_DATA fileInfo;
	HANDLE hFind= FindFirstFile(m_szConfigFile ,&fileInfo);
	if(hFind != INVALID_HANDLE_VALUE && fileInfo.nFileSizeLow > 0)
		nBufferSize = fileInfo.nFileSizeLow;
	FindClose(hFind);

	PTCHAR pBuffer= (PTCHAR)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, sizeof(TCHAR)*nBufferSize),ptrBuffer,tmpBuffer;
	DWORD ret = GetPrivateProfileSection("Command", pBuffer, nBufferSize, m_szConfigFile);

	PEXECPARAM pHeadExecParam=NULL,ptrExecParam;
	int nCmdSize = 0;

	if(ret>0) {
		TCHAR *p;
		TCHAR tmpDirectory[MAX_PATH]={0};
		TCHAR configFileDirectory[MAX_PATH]={0};
		strncpy(configFileDirectory,m_szConfigFile,strrchr(m_szConfigFile,'\\')-m_szConfigFile+1);
		for(ptrBuffer=pBuffer;*ptrBuffer;ptrBuffer+=strlen(ptrBuffer)+1) {
			ptrExecParam = (PEXECPARAM)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, sizeof(EXECPARAM));
			tmpBuffer=strchr(ptrBuffer,'=');
			strcpy(ptrExecParam->cmdLine,tmpBuffer+1);

			*tmpBuffer='\0';
			strcpy(ptrExecParam->keyName,ptrBuffer);

			sprintf(ptrExecParam->logFile, "%s-%s", m_szLogFile, ptrExecParam->keyName);

			GetPrivateProfileString("Directory", ptrExecParam->keyName, ".\\", tmpDirectory, MAX_PATH-1,m_szConfigFile);
			p=tmpDirectory;
			while(*p) {
				if(*p == '/') {
					*p='\\';
				}
				p++;
			}
			PathCombine(ptrExecParam->curDirectory,configFileDirectory, tmpDirectory);
#ifdef _DEBUG
			log->WriteLog(CLog::LL_INFO,"keyName(%s), curDirectory(%s), cmdLine(%s), logFile(%s)", ptrExecParam->keyName, ptrExecParam->curDirectory, ptrExecParam->cmdLine, ptrExecParam->logFile);
#endif
			ptrExecParam->next = NULL;
			if(pHeadExecParam) {
				pHeadExecParam->next = ptrExecParam;
			} else {
				pHeadExecParam = ptrExecParam;
			}

			nCmdSize++;

			ptrBuffer=strchr(tmpBuffer+1,'\0');
		}
	} else {
		log->WriteLog("Not found [Command] section in config file.");
		log->CloseLogFile();
		return;
	}

	PDWORD dwThreadId = (PDWORD)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, sizeof(DWORD)*nCmdSize);
	PHANDLE hThread = (PHANDLE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, sizeof(HANDLE)*nCmdSize);
	int i;
	while (m_bRunning) {
		if (m_bPaused) {
			Sleep(1000);
			continue;
		}

		ptrExecParam=pHeadExecParam;
		i=0;
		while(ptrExecParam) {
			hThread[i]=CreateThread(NULL, 0, ThreadProc, ptrExecParam, 0, &dwThreadId[i]);
			if(hThread[i]==NULL) {
				log->WriteLog(CLog::LL_ERROR, "[%s] create thread fail", ptrExecParam->keyName);
			} else {
				i++;
			}
			ptrExecParam = ptrExecParam->next;
		}

		WaitForMultipleObjects(i, hThread, TRUE,INFINITE);

		ptrExecParam=pHeadExecParam;
		i = 0;
		while(ptrExecParam) {
			CloseHandle(hThread[i]);
			i++;
			ptrExecParam = ptrExecParam->next;
		}
		Sleep(10000);
	}
	log->CloseLogFile();
}
