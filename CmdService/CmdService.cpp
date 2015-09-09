#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <Shlwapi.h>
#include "CmdService.h"
#include "Log.h"
#include "LogManager.h"
#include "api.h"

CmdService::CmdService() {
	pHeadExecParam=NULL;
	nCmdSize = 0;

	GetModuleFileName(NULL, m_szConfigFile, MAX_PATH);
	GetModuleFileName(NULL, m_szLogFile, MAX_PATH);

	strcpy(strrchr(m_szConfigFile,'.')+1,"ini");
	strcpy(strrchr(m_szLogFile,'.')+1,"log");
}

void CmdService::OnStop() {
	m_bPaused = false;

	PEXECPARAM ptrExecParam=pHeadExecParam;
	while(ptrExecParam) {
		TerminateProcess(ptrExecParam->processId, 4);
		ptrExecParam = ptrExecParam->next;
	}

	m_bRunning = false;
}
void CmdService::OnPause() {
	m_bPaused = true;

	PEXECPARAM ptrExecParam=pHeadExecParam;
	while(ptrExecParam) {
		TerminateProcess(ptrExecParam->processId, 4);
		ptrExecParam = ptrExecParam->next;
	}
}
void CmdService::OnContinue() {
	m_bPaused = false;
}

DWORD WINAPI ThreadProc(LPVOID lpParam)
{
	PEXECPARAM pData;

	pData=(PEXECPARAM)lpParam;

	pData->processId = NULL;
	ExecCommand(pData->cmdLine, pData->logFile, pData->curDirectory, NULL, &pData->processId);
	pData->processId = NULL;

	return 0;
}

void CmdService::Run() {
	CLog* log=LogManager::OpenLog(m_szLogFile,CLog::LL_INFO);
	int i;

	DWORD nBufferSize=GetFileSize(m_szConfigFile);

	if(!nBufferSize) {
		nBufferSize = 32767;
	}

	PEXECPARAM tmpExecParam=NULL,ptrExecParam;
	PTCHAR pBuffer= (PTCHAR)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, sizeof(TCHAR)*nBufferSize),ptrBuffer,tmpBuffer;
	DWORD ret = GetPrivateProfileSection("Command", pBuffer, nBufferSize, m_szConfigFile);

	// 读取所有配置的命令
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
			if(tmpExecParam) {
				tmpExecParam->next = ptrExecParam;
			} else {
				pHeadExecParam = ptrExecParam;
			}
			tmpExecParam = ptrExecParam;

			nCmdSize++;

			ptrBuffer=strchr(tmpBuffer+1,'\0');
		}
	} else {
		log->WriteLog("Not found [Command] section in config file.");
		log->CloseLogFile();
		return;
	}

	dwThreadId = (PDWORD)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, sizeof(DWORD)*nCmdSize);
	hThread = (PHANDLE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, sizeof(HANDLE)*nCmdSize);
	m_bRunning = true;
	while (m_bRunning) {
		if (m_bPaused) {
			Sleep(200);
			continue;
		}

		ptrExecParam=pHeadExecParam;
		i=0;
		while(ptrExecParam) {
			hThread[i]=CreateThread(NULL, 0, ThreadProc, ptrExecParam, 0, &dwThreadId[i]);
			if(hThread[i]==NULL) {
				log->WriteLog(CLog::LL_ERROR, "[%s] create thread fail", ptrExecParam->keyName);
			} else {
#ifdef _DEBUG
				log->WriteLog(CLog::LL_INFO, "[%d] = %s", i, ptrExecParam->keyName);
#endif
				i++;
			}
			ptrExecParam = ptrExecParam->next;
		}

		WaitForMultipleObjects(i, hThread, TRUE, INFINITE);

		ptrExecParam=pHeadExecParam;
		i = 0;
		while(ptrExecParam) {
			CloseHandle(hThread[i]);
			i++;
			ptrExecParam = ptrExecParam->next;
		}

		Sleep(1000);
	}
	log->CloseLogFile();
}
