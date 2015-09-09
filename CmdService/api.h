#ifndef _API_H
#define _API_H

DWORD GetFileSize(const char *filePath);
bool FileBackupCount(const char *filePath);
LPCTSTR GetFileVersion(LPCTSTR lpszFilePath);
int ExecCommand(TCHAR *cmdLine, TCHAR *logFile, TCHAR *curDirectory);
int ExecCommand(TCHAR *cmdLine, TCHAR *logFile, TCHAR *curDirectory, LPDWORD exitCode = NULL);
int ExecCommand(TCHAR *cmdLine, TCHAR *logFile, TCHAR *curDirectory, LPDWORD exitCode = NULL, PHANDLE processId = NULL);

#endif /*_API_H*/