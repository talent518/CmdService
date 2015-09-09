#include "ServiceCtrl.h"
#include "ServiceException.h"
#include <tchar.h>

ServiceCtrl::ServiceCtrl() {
}
ServiceCtrl::~ServiceCtrl() {
	if (m_hService != NULL)
		CloseServiceHandle(m_hService);
	if (m_hSCManager != NULL)
		CloseServiceHandle(m_hSCManager);
}

void ServiceCtrl::Init(LPCTSTR szServiceName) {

	m_hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (m_hSCManager == NULL) {
		throw ServiceException("Can not open the Service Control Manager.");
	}

	m_hService = OpenService(m_hSCManager, szServiceName, SERVICE_ALL_ACCESS);
	if (m_hService == NULL) {
		CloseServiceHandle(m_hSCManager);
		throw ServiceException("The service was not installed.");
	}
}


bool ServiceCtrl::Start() {
	
	if (m_hService == NULL || !StartService(m_hService, 0, NULL))
		return false;

	SERVICE_STATUS ssStatus; 
	if (!QueryServiceStatus(m_hService, &ssStatus))
		return false;

	DWORD dwOldCheckPoint; 
	DWORD dwStartTickCount;
	DWORD dwWaitTime;
	// Save the tick count and initial checkpoint.

	dwStartTickCount = GetTickCount();
	dwOldCheckPoint = ssStatus.dwCheckPoint;

	while (ssStatus.dwCurrentState == SERVICE_START_PENDING) {

		// Do not wait longer than the wait hint. A good interval is 
		// one tenth the wait hint, but no less than 1 second and no 
		// more than 10 seconds. 
		dwWaitTime = ssStatus.dwWaitHint / 10;

		if(dwWaitTime < 1000)
			dwWaitTime = 1000;
		else if (dwWaitTime > 10000)
			dwWaitTime = 10000;

		Sleep(dwWaitTime);

		// Check the status again. 

		if (!QueryServiceStatus(m_hService, &ssStatus))
			break;

		if (ssStatus.dwCheckPoint > dwOldCheckPoint) {
			// The service is making progress.
			dwStartTickCount = GetTickCount();
			dwOldCheckPoint = ssStatus.dwCheckPoint;
		} else {
			if(GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint) {
				// No progress made within the wait hint
				break;
			}
		}
	}

	return (ssStatus.dwCurrentState == SERVICE_RUNNING);
}

bool ServiceCtrl::Pause() {
	return Control(SERVICE_CONTROL_PAUSE);
}

bool ServiceCtrl::Continue() {
	return Control(SERVICE_CONTROL_CONTINUE);
}

bool ServiceCtrl::Stop() {
	return Control(SERVICE_CONTROL_STOP);
}

bool ServiceCtrl::Interrogate() {
	return Control(SERVICE_CONTROL_INTERROGATE);
};

bool ServiceCtrl::UserControl(DWORD dwControl) {
	if (dwControl < 128 || dwControl > 255)
		return false;
	return Control(dwControl);
}

DWORD ServiceCtrl::State() const {
	SERVICE_STATUS status;
	if (!QueryServiceStatus(m_hService, &status))
		return 0;
	return status.dwCurrentState;
}

bool ServiceCtrl::Control(DWORD dwControl) {
	SERVICE_STATUS status;
	return ControlService(m_hService, dwControl, &status) ? true : false;
}