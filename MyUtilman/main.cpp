#include "./thirdpart/utils.h"
#include <tchar.h>
#include <cassert>

/************************************************************************/
/* 
	ʹ�õ� power sync link ���������⣬���ػ�(��ֱ�����̣����Ļ���)������
	������꣬�����ò���
	
	żȻ���֣�ֻ��Ҫ���� LinkEngKM.exe ������̾Ϳ�����.

	ʵ���ϣ�power sync link ����������: power sync link �� UI ���̣�MacKMLink ����
	�������߼�������̣�LinkEngKM ������̨����֮������ݴ���.���ڹ�˾���������
	��������� LinkEngKm ���̹ҵ�.����Ҳ����������Ľ��˼·��
	ɱ�� LinkEngKM ���̺�ֻҪ�������������¼������ͻᱻ MacKMLink �������������
	����ֻ��Ҫɱ�����Ϳ����ˡ�

	����½�����֣�MacMLink ����Ҳ���ܱ������򵥵Ĵ����ǣ�ɱ��������̣���UI ����ȥ����
	������̡�

	��������߼���: 
		1.���ó�ʱʱ��(1����)����û�������¼���ɱ�� LinkEngKM ����(�������Ҷ��ڴ���)��[��ȥ�����߼�]
		2.��������������(�������������������ӿ�)��

	���淢��һ�������Ż�������: 
		�ڸ��ػ���ɱ�� LinkEngKM ���̺������ػ����뵽���ػ�ʱ������һ�㲻��������Ϊ��
		���ڽ��յ������¼�ʱ�Ż�ȥ���� LinkEngKm �������״ν���ʱ�ͻ��п��������Ż��ķ����ǣ�ÿ
		��ɱ�� LinkEngKM ����һ��ģ�����룬ʹ LinkEngKM ����.
*/
/************************************************************************/

//û�м�������¼��೤ʱ����
DWORD loseInputWithSecs()
{
	LASTINPUTINFO lii;
	
	lii.cbSize = sizeof(LASTINPUTINFO);
	//ȡ������һ�η��� input �¼�ʱ��ϵͳ�����е�ʱ���. lii.dwTime=GetTickCount. ��λ����
	GetLastInputInfo(&lii);
	DWORD lastInputTickcount = lii.dwTime;
	return (GetTickCount() - lastInputTickcount)/1000;
}

//1 ��ǰû�����������ϴμ�⵽Ҳ��û����
//2 ��ǰû�����������ϴμ�⵽����
//3 ��ǰ���������ϴμ�⵽δ����
//4 ��ǰ���������ϴμ�⵽Ҳ������
//����, ������ 2 �� 3 ���ʾ�в���
int screenLockState()
{
	static BOOL lastScreenLocked = FALSE;
	int retkd = 0;
	BOOL curLocked = hasScreenLocked();
	if(!curLocked)
	{
		if(!lastScreenLocked)
		{
			retkd = 1;
		}
		else
		{
			retkd = 2;
		}
	}
	else
	{
		if(!lastScreenLocked)
		{
			retkd = 3;
		}
		else
		{
			retkd = 4;
		}
	}
	lastScreenLocked = curLocked;
	return retkd;
}


bool killLinkEngineIfCrash()
{
	set<DWORD> pids;
	if(!getProcessInfoByName(_T("WerFault.exe"), pids))
	{
		return false;
	}

	HWND faultWnd = NULL;//GetForegroundWindow();
	faultWnd = ::FindWindow(0,_T("Link Engine"));
	DWORD wndProcessId = 0;
	GetWindowThreadProcessId(faultWnd,&wndProcessId);
	if(pids.end() != pids.find(wndProcessId))
	{
		LOG_INFO("Link Engine has crashed");
		KillProcess(wndProcessId,0);
		KillProcessByName(_T("LinkEngKM.exe"));
		return true;
	}
	return false;
}

bool killMacKMLinkIfCrash()
{
	set<DWORD> pids;
	if(!getProcessInfoByName(_T("WerFault.exe"), pids))
	{
		return false;
	}

	HWND faultWnd = NULL;//GetForegroundWindow();
	faultWnd = ::FindWindow(0,_T("Mac KM Link"));
	DWORD wndProcessId = 0;
	GetWindowThreadProcessId(faultWnd,&wndProcessId);
	if(pids.end() != pids.find(wndProcessId))
	{
		LOG_INFO("Mac KM Link has crashed");
		KillProcess(wndProcessId,0);
		KillProcessByName(_T("MacKMLink.exe"));
		return true;
	}
	return false;

}

TCHAR powerKMExePath[MAX_PATH];
TCHAR powerKMWorkDir[MAX_PATH];
bool storePoerKMpath(ProcessInfo* thisInfo=NULL)
{
	set<ProcessInfo> infos;
	ProcessInfo *info = thisInfo;
	if(NULL == info)
	{	
		if(!getProcessInfoByNameEx(_T("PowerSyncKM.exe"),infos) || infos.empty())
		{
			return false;
		}
		info = &(*(infos.begin()));
	}

	size_t len = tstrLen(info->_path);
	memcpy_s(powerKMExePath,sizeof(TCHAR)* MAX_PATH,info->_path,len*sizeof(TCHAR));
	powerKMExePath[len] = 0;

	size_t charLen = 0;
	char * chPath = wideCharSetToDefaultMult(powerKMExePath,len,&charLen);

	string workDir;
	getParentDir(chPath, workDir);

	delete[] chPath;

	size_t whLen;
	TCHAR *chWorkDir = defaultMultCharSetToWide(workDir.c_str(),workDir.length(),&whLen);
	
	memcpy_s(powerKMWorkDir,MAX_PATH*sizeof(TCHAR),chWorkDir,whLen*sizeof(TCHAR));
	powerKMWorkDir[whLen] = 0;
	delete[] chWorkDir;
	return true;
}

bool restartAll()
{
	set<ProcessInfo> infos;
	if(getProcessInfoByNameEx(_T("PowerSyncKM.exe"),infos) && !infos.empty())
	{
		if(1 < infos.size())
		{
			LOG_INFO("PowerSyncKM.exe started more than one time");
		}

		set<ProcessInfo> ::iterator it = infos.begin();
		for(;it != infos.end(); ++it)
		{
			ProcessInfo &info = *it;
			KillProcess(info._pid,0);
		}

		KillProcessByName(_T("LEWD.exe"));
		KillProcessByName(_T("LinkEngKM.exe"));
		KillProcessByName(_T("MacKMLink.exe"));

		storePoerKMpath(&(*infos.begin()));
	}
	
	if(0 != tstrLen(powerKMExePath))
		startProcess(powerKMExePath, NULL, powerKMWorkDir);
	return true;
}

static int timeoutSecs = 60;

bool killDetetor()
{
	bool timeout = loseInputWithSecs()>timeoutSecs;
	if (timeout)
	{
		LOG_INFO("timeout");
	}
	return timeout;
}

/************************************************************************/
/* 
	�������������¼���UI �������� Mac Link ����(���û����)��Mac Link ����
	������ linkeng Km ����(���û����)
*/
/************************************************************************/
void startLinkEngine()
{
	BYTE vks[] = {VK_CONTROL,VK_MULTIPLY};	// ctrl + *
	sendToForegroundWnd(vks, sizeof(vks));
}

void monitorCallback(void *data)
{
	storePoerKMpath();
	if(3 == screenLockState())
	{
		LOG_INFO("screen lock just now");
		restartAll();
		return;
	}
	if(killLinkEngineIfCrash())
	{
		startLinkEngine();
		LOG_INFO("deal success!");
	}
	if(killMacKMLinkIfCrash())
	{
		startLinkEngine();
		LOG_INFO("deal success!");
	}
}

void monitor()
{
	timeRun(monitorCallback, 0, 1);
}

void testModule()
{
	set<ProcessInfo> infos;
	getProcessInfoByNameEx(_T("PowerSyncKM.exe"),infos);
	return;
}



int WINAPI WinMain(
				   HINSTANCE hInstance,       //����ǰʵ���ľ�����Ժ���ʱ������GetModuleHandle(0)�����  
				   HINSTANCE hPrevInstance,   //���������Win32����������0���Ѿ�����������  
				   char * lpCmdLine,          //ָ����/0��β�������У�������EXE������ļ�����������GetCommandLine()����ȡ������������  
				   int nCmdShow               //ָ��Ӧ����ʲô��ʽ��ʾ������  
				   )
{
	if(tryFirstRunModule())
	{
		LOG_INFO("app has been started");
		return -1;
	}
	monitor();
	return 0;
}