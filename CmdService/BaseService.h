#ifndef BASE_SERVICE_H
#define BASE_SERVICE_H

class BaseService {
public:
	BaseService();
	~BaseService();
	virtual void Init();
    bool IsInstalled();
    bool Install(TCHAR szFilePath[]);
    bool Uninstall();
    bool Start();

public:
	TCHAR m_szServiceName[MAX_PATH];
	DWORD m_dwServiceType;
	DWORD m_dwStartType;

private:
    virtual void Run() = 0;
	virtual bool OnInitialize() { return true; }
	virtual void OnStop() {}
	virtual void OnPause() {}
	virtual void OnContinue() {}
	virtual void OnInterrogate() {}
	virtual void OnShutdown() {}
	virtual void OnUserControl(DWORD dwControl) {}
    
private:
	void SetStatus(DWORD dwCurrentState,
			DWORD dwCheckPoint = 0,
			DWORD dwWaitHint = 2000,
			DWORD dwWin32ExitCode = NO_ERROR,
			DWORD dwServiceSpecificExitCode = 0);

    // static member functions
    static void WINAPI ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);
    static void WINAPI Handler(DWORD dwOpcode);

	// static data
	static BaseService* m_pThis; // nasty hack to get object ptr

    // data members
    SERVICE_STATUS_HANDLE m_hServiceStatus;
    SERVICE_STATUS m_Status;
};

#endif/*BASE_SERVICE_H*/
