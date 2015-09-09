#ifndef CMD_SERVICE_H
#define CMD_SERVICE_H

#include "BaseService.h"

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
};

extern CmdService cmdService;

#endif/*CMD_SERVICE_H*/