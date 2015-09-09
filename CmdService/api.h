#ifndef _API_H
#define _API_H

DWORD GetFileSize(const char *filePath);
bool FileBackupCount(const char *filePath);
LPCTSTR GetFileVersion(LPCTSTR lpszFilePath);
int ExecCommand(TCHAR *cmdLine, TCHAR *logFile, TCHAR *curDirectory, LPDWORD exitCode = NULL, PHANDLE processId = NULL, HANDLE hToken = NULL);

#endif /*_API_H*/