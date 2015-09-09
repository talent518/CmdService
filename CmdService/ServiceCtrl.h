#ifndef SERVICE_CTRL_H
#define SERVICE_CTRL_H

#include <windows.h>

class ServiceCtrl {
public:
	ServiceCtrl();
	~ServiceCtrl();

	void Init(LPCTSTR szServiceName);

	bool Start();
	bool Pause();
	bool Continue();
	bool Stop();
	bool Interrogate();
	bool UserControl(DWORD dwControl);
	DWORD State() const;
private:
	bool Control(DWORD dwControl);
private:
	SC_HANDLE m_hSCManager;
	SC_HANDLE m_hService;
};

#endif/*SERVICE_CTRL_H*/
