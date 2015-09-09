#include <ctime>
#include <string>
#include <cstdarg>
#include <Windows.h>
#include <Shlwapi.h>
#include "api.h"

DWORD GetFileSize(const char *filePath) {
	DWORD fileSize=0;

	// 获取配置文件大小
	WIN32_FIND_DATA fileInfo;
	HANDLE hFind= FindFirstFile(filePath ,&fileInfo);
	if(hFind != INVALID_HANDLE_VALUE && fileInfo.nFileSizeLow > 0)
		fileSize = fileInfo.nFileSizeLow;
	FindClose(hFind);

	return fileSize;
}

bool FileBackupCount(const char *filePath) {
	char szNewFilePath[MAX_PATH],szIndexFilePath[MAX_PATH];

	sprintf(szIndexFilePath, "%s-index", filePath);

	int fileCount;
	FILE *file=fopen(szIndexFilePath,"rw");

	if(!file) {
		return false;
	}

	fscanf(file,"%d",&fileCount);

	sprintf(szNewFilePath,"%s-%06d",filePath);
	if(rename(filePath, szNewFilePath)) {
		fclose(file);
		return false;
	} else {
		fseek(file, 0, SEEK_SET);
		fprintf(file,"%d", fileCount+1);
		fclose(file);
		return true;
	}
}

LPCTSTR GetFileVersion(LPCTSTR lpszFilePath)
{
	TCHAR *szResult="0.0.0.0";

	if (PathFileExists(lpszFilePath))
	{
		VS_FIXEDFILEINFO *pVerInfo = NULL;
		DWORD dwTemp, dwSize;
		BYTE *pData = NULL;
		UINT uLen;

		dwSize = GetFileVersionInfoSize(lpszFilePath, &dwTemp);
		if (dwSize == 0)
		{
			return szResult;
		}

		pData = new BYTE[dwSize+1];
		if (pData == NULL)
		{
			return szResult;
		}

		if (!GetFileVersionInfo(lpszFilePath, 0, dwSize, pData))
		{
			delete[] pData;
			return szResult;
		}

		if (!VerQueryValue(pData, TEXT("\\"), (void **)&pVerInfo, &uLen)) 
		{
			delete[] pData;
			return szResult;
		}

		DWORD verMS = pVerInfo->dwFileVersionMS;
		DWORD verLS = pVerInfo->dwFileVersionLS;
		DWORD major = HIWORD(verMS);
		DWORD minor = LOWORD(verMS);
		DWORD build = HIWORD(verLS);
		DWORD revision = LOWORD(verLS);
		delete[] pData;
		delete pVerInfo;

		szResult=new TCHAR[20];
		sprintf(szResult,"%d.%d.%d.%d", major, minor, build, revision);
	}

	return szResult;
}

int ExecCommand(TCHAR *cmdLine, TCHAR *logFile, TCHAR *curDirectory) {
	return ExecCommand(cmdLine, logFile, curDirectory, NULL, NULL);
}

int ExecCommand(TCHAR *cmdLine, TCHAR *logFile, TCHAR *curDirectory, LPDWORD exitCode) {
	return ExecCommand(cmdLine, logFile, curDirectory, exitCode, NULL);
}

int ExecCommand(TCHAR *cmdLine, TCHAR *logFile, TCHAR *curDirectory, LPDWORD exitCode, PHANDLE processId)
{
	PROCESS_INFORMATION pi; 
	STARTUPINFO si;
	SECURITY_ATTRIBUTES sa;
	BOOL ret = FALSE; 
	DWORD flags = NORMAL_PRIORITY_CLASS;
	HANDLE logStdOut;

#ifndef _DEBUG
	flags |= CREATE_NO_WINDOW;
#endif

	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	if(GetFileSize(logFile) > 10*1024*1024) {
		FileBackupCount(logFile);
	}

	logStdOut = CreateFile(logFile, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

	DWORD dwHigh;
	DWORD dwPos = GetFileSize(logStdOut, &dwHigh);
	SetFilePointer(logStdOut,dwPos,0,FILE_BEGIN);

	ZeroMemory( &pi, sizeof(PROCESS_INFORMATION) );
	ZeroMemory( &si, sizeof(STARTUPINFO) );
	si.cb = sizeof(STARTUPINFO); 
	si.dwFlags =  STARTF_USESTDHANDLES;
	si.wShowWindow =SW_HIDE;
	si.hStdInput = NULL;
	si.hStdError = logStdOut;
	si.hStdOutput = logStdOut;

	ret = CreateProcess(NULL, cmdLine, &sa, &sa, sa.bInheritHandle, flags, NULL, curDirectory, &si, &pi);
	if(ret) {
		if(processId)
			*processId = pi.hProcess;

		WaitForSingleObject(pi.hProcess, INFINITE );

		if(exitCode) {
			*exitCode = GetExitCodeProcess(pi.hProcess, exitCode);
		}

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	CloseHandle(logStdOut);

	return ret;
}
