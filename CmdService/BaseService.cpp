#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include "BaseService.h"

// static variables
BaseService* BaseService::m_pThis = NULL;

BaseService::BaseService() {
    _tcsncpy_s(m_szServiceName, "cmdService", MAX_PATH - 1);
	m_dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	m_dwStartType = SERVICE_AUTO_START;
}
BaseService::~BaseService() {
}
void BaseService::Init() {

    // copy the address of the current object so we can access it from
    // the static member callback functions. 
    // WARNING: This limits the application to only one BaseService object. 
    m_pThis = this;

    // set up the initial service status 
    m_hServiceStatus = NULL;
	m_Status.dwServiceType = m_dwServiceType;
	m_Status.dwCurrentState = SERVICE_STOPPED;
	m_Status.dwControlsAccepted = 0;
	m_Status.dwWin32ExitCode = 0;
    m_Status.dwServiceSpecificExitCode = 0;
    m_Status.dwCheckPoint = 0;
    m_Status.dwWaitHint = 0;

	// set up the service start type
	m_dwStartType = m_dwStartType;
}


////////////////////////////////////////////////////////////////////////////////////////
// Install/uninstall routines

// Test if the service is currently installed
bool BaseService::IsInstalled() {

    bool bResult = false;

    // Open the Service Control Manager
    SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (hSCM) {

        // Try to open the service
        SC_HANDLE hService = OpenService(hSCM, m_szServiceName, SERVICE_QUERY_CONFIG);
        if (hService) {
            bResult = true;
            CloseServiceHandle(hService);
        }

        CloseServiceHandle(hSCM);
    }
    
    return bResult;
}

bool BaseService::Install(TCHAR szFilePath[]) {

    // Open the Service Control Manager
    SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);

    if (!hSCM) return false;

    // Create the service
    SC_HANDLE hService = CreateService(hSCM,
									m_szServiceName,
									m_szServiceName,
									SERVICE_ALL_ACCESS,
									m_Status.dwServiceType,
									m_dwStartType,
									SERVICE_ERROR_NORMAL,
									szFilePath,
									NULL,
									NULL,
									NULL,
									NULL,
									NULL);
    if (!hService) {
        CloseServiceHandle(hSCM);
        return false;
    }

	const char *szDescribe="Backend Command Service Tools.";

	ChangeServiceConfig2(hService,1,(LPVOID) &szDescribe);

    // tidy up
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCM);
    return true;
}

bool BaseService::Uninstall() {

    // Open the Service Control Manager
    SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hSCM) return false;

    bool bResult = false;
    SC_HANDLE hService = OpenService(hSCM, m_szServiceName, SERVICE_ALL_ACCESS | DELETE);
	SERVICE_STATUS serviceStatus;
	if (!QueryServiceStatus(hService, &serviceStatus))
		return false;

	if (serviceStatus.dwCurrentState != SERVICE_STOPPED) {
		_tprintf(_T("Stopping service...\n"));
		if (!ControlService(hService, SERVICE_CONTROL_STOP, &serviceStatus))
			return false;
	}

    if (hService) {
        if (DeleteService(hService))
            bResult = true;
        CloseServiceHandle(hService);
    }
    
    CloseServiceHandle(hSCM);
    return bResult;
}


//////////////////////////////////////////////////////////////////////////////////////////////
// Service startup and registration

bool BaseService::Start() {

	SERVICE_TABLE_ENTRY serviceTable[] = {
		{ m_szServiceName, ServiceMain },
		{ NULL, NULL }
	};

	return StartServiceCtrlDispatcher(serviceTable) ? true : false;
}

// static member function (callback)
void BaseService::ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv) {

    // Get a pointer to the C++ object
    BaseService* pService = m_pThis;
    
    // Register the control request handler
    pService->m_hServiceStatus =
		RegisterServiceCtrlHandler(pService->m_szServiceName, Handler);

    if (pService->m_hServiceStatus == NULL)
        return;

	// Start the initialization
	pService->SetStatus(SERVICE_START_PENDING);

	if (pService->OnInitialize()) {
		pService->SetStatus(SERVICE_RUNNING);
        pService->Run();
	}
    // Tell the service manager we are stopped
    pService->SetStatus(SERVICE_STOPPED);

}

///////////////////////////////////////////////////////////////////////////////////////////
// This function consolidates the activities of
// updating the service status with
// SetServiceStatus
void BaseService::SetStatus(DWORD dwCurrentState,
					  DWORD dwCheckPoint,
					  DWORD dwWaitHint,
					  DWORD dwWin32ExitCode,
					  DWORD dwServiceSpecificExitCode) {

	SERVICE_STATUS serviceStatus = m_Status;

	// Fill in all of the SERVICE_STATUS fields, except dwServiceType

	serviceStatus.dwCurrentState = dwCurrentState;
	serviceStatus.dwCheckPoint = dwCheckPoint;
	serviceStatus.dwWaitHint = dwWaitHint;

	// If in the process of something, then accept
	// no control events, else accept anything
	if (dwCurrentState == SERVICE_START_PENDING)
		serviceStatus.dwControlsAccepted = 0;
	else
		serviceStatus.dwControlsAccepted =
						SERVICE_ACCEPT_STOP |
						SERVICE_ACCEPT_PAUSE_CONTINUE |
						SERVICE_ACCEPT_SHUTDOWN;

	// if a specific exit code is defined, set up
	// the win32 exit code properly
	if (dwServiceSpecificExitCode == 0)
		serviceStatus.dwWin32ExitCode = dwWin32ExitCode;
	else
		serviceStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
	serviceStatus.dwServiceSpecificExitCode = dwServiceSpecificExitCode;

	SetServiceStatus(m_hServiceStatus, &serviceStatus);
}

//////////////////////////////////////////////////////////////////////////////////////
// Control request handlers

// static member function (callback) to handle commands from the
// service control manager
void BaseService::Handler(DWORD dwControl) {

    // Get a pointer to the object
    BaseService* pService = m_pThis;
    DWORD currentState = pService->m_Status.dwCurrentState;

    switch (dwControl) {
    case SERVICE_CONTROL_STOP:
        pService->SetStatus(SERVICE_STOP_PENDING);
        pService->OnStop();
		currentState = SERVICE_STOPPED;
        break;

    case SERVICE_CONTROL_PAUSE:
		pService->SetStatus(SERVICE_PAUSE_PENDING);
        pService->OnPause();
		currentState = SERVICE_PAUSED;
        break;

    case SERVICE_CONTROL_CONTINUE:
		pService->SetStatus(SERVICE_CONTINUE_PENDING);
        pService->OnContinue();
		currentState = SERVICE_RUNNING;
        break;

    case SERVICE_CONTROL_INTERROGATE:
        pService->OnInterrogate();
        break;

    case SERVICE_CONTROL_SHUTDOWN:
        pService->OnShutdown();
        return;

    default:
        if (dwControl >= 128 && dwControl <= 255)
            pService->OnUserControl(dwControl);
        break;
    }

    // Report current status
	pService->SetStatus(currentState);
}
