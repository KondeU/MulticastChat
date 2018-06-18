#define _WINSOCK_DEPRECATED_NO_WARNINGS ////////
#define _CRT_SECURE_NO_WARNINGS //////


#include "../CTransfer.hpp" // A strange bug, the WinSock header must be before Windows.h,
CTransfer cTrans;           // or will cause redefinition error.

#include <Windows.h>
#pragma comment(linker,                                      \
				"/manifestdependency:\""                     \
				"type='win32' "                              \
				"name='Microsoft.Windows.Common-Controls' "  \
				"version='6.0.0.0' "                         \
				"processorArchitecture='*' "                 \
				"publicKeyToken='6595b64144ccf1df' "         \
				"language='*'\"")
#include "resource.h"

#include <process.h>

#include <Shlwapi.h>
#pragma comment(lib,"Shlwapi.lib")

#include "../CmnDlg.hpp"
using namespace COMMONDIALOG;

#include "../Logger.hpp"

static const TCHAR szAppNameEng[] = TEXT("MulticastChat");
static const TCHAR szAppNameChn[] = TEXT("MulticastChat多播聊天室");

static const TCHAR szUserLicense[] =
	TEXT("MulticastChat多播聊天室使用协议及软件说明：\r\n");

void AppendEdit(HWND hWnd, INT nEditItemID, LPCTSTR szOutputString, ...)
{
	HWND hEdit = GetDlgItem(hWnd, nEditItemID);

	TCHAR szBuffer[4096] = { 0 }; // Max buffer size is 4096

	va_list vlArgs;
	va_start(vlArgs, szBuffer);
	#ifdef UNICODE
	_vsnwprintf_s(szBuffer, sizeof(szBuffer), szOutputString, vlArgs);
	#else
	_vsnprintf_s(szBuffer, sizeof(szBuffer), szOutputString, vlArgs);
	#endif
	va_end(vlArgs);

	SendMessage(hEdit, EM_SETSEL, -2, -1);
	SendMessage(hEdit, EM_REPLACESEL, TRUE, (LPARAM)szBuffer);

	SendMessage(hEdit, WM_VSCROLL, SB_BOTTOM, 0);
}

BOOL CALLBACK DlgProcAdd(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcLogo(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcChat(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, INT iCmdShow)
{
	INT_PTR iRet = DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG_ADD), GetDesktopWindow(), DlgProcAdd);
	
	switch (iRet)
	{
	case 1:
		if (1 ==
			DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG_LOGO), GetDesktopWindow(), DlgProcLogo))
		{
			break;
		}
		else // return == 100, error and close execute
		{
			return 1;
		}

	case 100: // User close the execute
		return 0;

	case -1:
		ErrMsgBox(GetDesktopWindow(), szAppNameChn);
		return 1;

	case 0:
		if (IDNO == MessageBox(GetDesktopWindow(),
			TEXT("警告：Windows版本过老或父窗口参数无效，可能会导致错误。\r\n是否继续运行？"),
			szAppNameChn, MB_ICONWARNING | MB_YESNO))
		{
			return 0;
		}
		break;
	}

	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG_CHAT), GetDesktopWindow(), DlgProcChat);

	return 0;
}

BOOL CALLBACK DlgProcAdd(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			cTrans.SetHwnd(hDlg);
			cTrans.SetCaption(szAppNameChn);

			SendDlgItemMessage(hDlg, IDC_IPADDRESS, IPM_SETRANGE, 0, MAKEIPRANGE(224, 239)); // First byte limit
			SendDlgItemMessage(hDlg, IDC_IPADDRESS, IPM_SETADDRESS, 0, MAKEIPADDRESS(224, 0, 2, 0));

			SetDlgItemInt(hDlg, IDC_EDIT_PORT, 5500, FALSE);

			SendDlgItemMessage(hDlg, IDC_COMBO_NET, CB_ADDSTRING, 0, LPARAM(TEXT("INADDR_ANY")));
			SendDlgItemMessage(hDlg, IDC_COMBO_NET, CB_SETCURSEL, 0, 0); // wParam is index

			SetDlgItemText(hDlg, IDC_EDIT_NAME, TEXT("YourName"));

			SetDlgItemInt(hDlg, IDC_EDIT_PACKSIZE, 4096, FALSE); // Default package size is 4KB

			SetDlgItemInt(hDlg, IDC_EDIT_TTL, 1, FALSE); // TTL is 1

			SetDlgItemText(hDlg, IDC_EDIT_LICENSE, szUserLicense);

			bool bGetValidIP = false;
			vector<TString> vValidIP = cTrans.GetValidIP(&bGetValidIP);
			for (size_t n = 0; n < vValidIP.size(); n++)
			{
				SendDlgItemMessage(hDlg, IDC_COMBO_NET, CB_ADDSTRING,
					n + 1, LPARAM(vValidIP[n].c_str()));
			}
		}
		return TRUE;
	
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			{
				TCHAR szMulticastIP[16];
				TCHAR szLocalIP[16];
				unsigned int uiPort;
				TCHAR szName[32];
				
				GetDlgItemText(hDlg, IDC_IPADDRESS, szMulticastIP, 16);
				GetDlgItemText(hDlg, IDC_COMBO_NET, szLocalIP, 16);
				uiPort = GetDlgItemInt(hDlg, IDC_EDIT_PORT, nullptr, FALSE);
				GetDlgItemText(hDlg, IDC_EDIT_NAME, szName, 32);

				unsigned int uiPackageSize;
				unsigned int uiTTL;

				uiPackageSize = GetDlgItemInt(hDlg, IDC_EDIT_PACKSIZE, nullptr, FALSE);
				uiTTL = GetDlgItemInt(hDlg, IDC_EDIT_TTL, nullptr, FALSE);

				if (uiPort < 1024)
				{
					MessageBox(hDlg, TEXT("该端口号为保留端口号！端口号范围：1024-65535。"),
						szAppNameChn, MB_ICONERROR | MB_OK);
					break;
				}
				else if (uiPort > 65535)
				{
					MessageBox(hDlg, TEXT("该端口号无效！端口号范围：1024-65535。"),
						szAppNameChn, MB_ICONERROR | MB_OK);
					break;
				}

				if (uiPackageSize > 65536)
				{
					MessageBox(hDlg, TEXT("传输数据包大小无效！上限为64KB（65536Bytes）。"),
						szAppNameChn, MB_ICONERROR | MB_OK);
					break;
				}

				if (uiTTL > 128)
				{
					MessageBox(hDlg, TEXT("TTL值无效！上限为128。"),
						szAppNameChn, MB_ICONERROR | MB_OK);
					break;
				}

				cTrans.InitParam(szMulticastIP, szLocalIP, uiPort, szName, uiPackageSize, uiTTL);

				EndDialog(hDlg, 1);
			}
			break;

		case IDCANCEL:
			EndDialog(hDlg, 100);
			break;
		}
		return TRUE;

	case WM_CLOSE:
		EndDialog(hDlg, 100);
		return TRUE;
	}

	return FALSE;
}

BOOL CALLBACK DlgProcLogo(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static bool bSuccess = false;

	switch (message)
	{
	case WM_INITDIALOG:
		cTrans.SetHwnd(hDlg);
		SetTimer(hDlg, 0, 1000, NULL); // Delay at least 1000ms to display the logo
		return TRUE;

	case WM_SHOWWINDOW:
		PostMessage(hDlg, (WM_APP + 0x100), 0, 0);
		return TRUE;

	case (WM_APP + 0x100):
		if (!cTrans.EnterChat())
		{
			MessageBox(hDlg,
				TEXT("出现错误，请尝试重新启动或联系管理员。"),
				szAppNameChn, MB_ICONERROR | MB_OK);
			EndDialog(hDlg, 100);
		}
		else
		{
			bSuccess = true;
		}
		return TRUE;

	case WM_TIMER:
		if (bSuccess)
			EndDialog(hDlg, 1);
		return TRUE;
	}

	return FALSE;
}

BOOL CALLBACK DlgProcChat(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			cTrans.SetHwnd(hDlg);

			TString szWindowsName(TEXT("MulticastChat多播聊天室"));
			szWindowsName += TEXT(" - ");
			szWindowsName += cTrans.GetMulticastIP();
			szWindowsName += TEXT(" - ");
			szWindowsName += cTrans.GetMulticastPortString();
			SetWindowText(hDlg, szWindowsName.c_str());

			if (!cTrans.BeginChat())
			{
				MessageBox(hDlg,
					TEXT("加入聊天室失败！请检查网络状态或联系管理员！"),
					szAppNameChn, MB_ICONERROR | MB_OK);
				EndDialog(hDlg, 100);
			}

			void __cdecl RecvFileThread(void * pArgc);
			_beginthread(RecvFileThread, 0, hDlg); ///////
		}
		return TRUE;

	case WM_SHOWWINDOW:
		{
			TString szSend(TEXT("[系统消息]\r\n"));
			szSend += cTrans.GetName();
			szSend += TEXT(" 加入当前多播聊天室。");

			cTrans.SendMsg(szSend);
		}
		return TRUE;

	case WM_SOCKRECV:
		{
			SOCKADDR_IN tSockAddrFrom;
			if (cTrans.Recv(&tSockAddrFrom))
			{
				switch (cTrans.m_tPackage.byStyle)
				{
				case TRN_PKG_STYLE_MESSAGE:
					{
						TString szRecv((TCHAR *)cTrans.m_tPackage.byData);

						szRecv += TEXT("\r\n--- IP: ");
						TCHAR szIP[16] = { 0 };
						InetNtop(AF_INET, &tSockAddrFrom.sin_addr, szIP, 16);
						szRecv += szIP;
						szRecv += TEXT(" ---\r\n");

						TString & szShow = UpdateChatMsgBuf(szRecv);
						SetDlgItemText(hDlg, IDC_EDIT_RECV, szShow.c_str());
						SendDlgItemMessage(hDlg, IDC_EDIT_RECV, WM_VSCROLL, SB_BOTTOM, 0);
					}
					break;

				case TRN_PKG_STYLE_SETTING:
					{

					}
					break;
				}
			}
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			{
				TString szSend(TString(TEXT("[")) + cTrans.GetName() + TString(TEXT("]\r\n")));

				TCHAR szMsg[2000] = { 0 };
				int iMsgCnt = GetWindowTextLength(GetDlgItem(hDlg, IDC_EDIT_SEND));
				if (iMsgCnt > 2000)
				{
					TStringStream ss;
					ss << TEXT("最大发送字符上限为2000字！当前编辑框字数为");
					ss << iMsgCnt << TEXT("字。");
					TString szMsgOutLmt;
					ss >> szMsgOutLmt;

					MessageBox(hDlg, szMsgOutLmt.c_str(), szAppNameChn, MB_ICONWARNING | MB_OK);
				}
				else
				{
					GetDlgItemText(hDlg, IDC_EDIT_SEND, szMsg, 2000);
					szSend += szMsg;

					if (cTrans.SendMsg(szSend))
					{
						// Send successfully
						SetDlgItemText(hDlg, IDC_EDIT_SEND, TEXT(""));
					}
				}
			}
			break;

		case IDCANCEL:
			SendMessage(hDlg, WM_CLOSE, 0, 0);
			break;

		case IDC_BUTTON_ROOMINFO:
			MessageBox(hDlg, cTrans.GetInfo().c_str(), szAppNameChn, MB_ICONINFORMATION | MB_OK);
			break;

		case IDC_BUTTON_SENDFILE:
			{
				void __cdecl SendFileThread(void * pArgc);
				_beginthread(SendFileThread, 0, hDlg);
			}
			break;
		}
		return TRUE;

	case WM_CLOSE:
		if (IDOK == MessageBox(hDlg,
			TEXT("确认即将退出聊天室？若正在传输文件，任务将被迫终止。"),
			szAppNameChn, MB_ICONWARNING | MB_OKCANCEL))
		{
			TString szSend(TEXT("[系统消息]\r\n"));
			szSend += cTrans.GetName();
			szSend += TEXT(" 离开当前多播聊天室。");

			cTrans.SendMsg(szSend);

			cTrans.EndChat();
			EndDialog(hDlg, 1);
		}
		return TRUE;

	// NM_CLICK (syslink)
	case WM_NOTIFY: // This notification code is sent in the form of a WM_NOTIFY message
		{
			LPNMHDR hdr = (LPNMHDR)lParam;
			if ((NM_CLICK == hdr->code) && (IDC_SYSLINK == hdr->idFrom))
			{
				//PNMLINK pnmLink = (PNMLINK)lParam;
				//ShellExecuteW(hDlg, L"open", pnmLink->item.szUrl, NULL, NULL, SW_SHOWNORMAL);
				ShellExecute(hDlg, TEXT("open"), TEXT("https://github.com/KondeU"), NULL, NULL, SW_SHOWNORMAL);
			}
		}
		return TRUE;
	}

	return FALSE;
}

/*
BOOL CALLBACK DlgProcSender(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void __cdecl SendFileThread(void * pArgc)
{
	HWND hwndParent = (HWND)pArgc;
	//DialogBox((HINSTANCE)GetWindowLong(hwndParent, GWL_HINSTANCE),
	//	MAKEINTRESOURCE(IDD_DIALOG_FILESENDER), hwndParent, DlgProcSender);
	DialogBox((HINSTANCE)GetWindowLong(hwndParent, GWL_HINSTANCE),
		MAKEINTRESOURCE(IDD_DIALOG_FILESENDER), GetDesktopWindow(), DlgProcSender);
	_endthread();
}

BOOL CALLBACK DlgProcSender(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	struct TStaticSender
	{
		HWND hDlg;
		COSDlg cOpenDlg;
		CFileTransfer cFileTrans;
		HANDLE hFile;

		TStaticSender(HWND hwnd) : cOpenDlg(hwnd)
		{
			hDlg = hwnd;
		}
	};
	static vector<TStaticSender> vStatic;
	
	COSDlg * pcOpenDlg = nullptr; // Use to process
	CFileTransfer * pcFileTrans = nullptr; // Use to process
	HANDLE * phFile = nullptr; // Use to process

	bool bGetStatic = false;
	for (size_t n = 0; n < vStatic.size(); n++)
	{
		if (hDlg == vStatic[n].hDlg)
		{
			bGetStatic = true;
			pcOpenDlg = &(vStatic[n].cOpenDlg);
			pcFileTrans = &(vStatic[n].cFileTrans);
			break;
		}
	}
	if (bGetStatic)
	{
		TStaticSender tStaticNew(hDlg);
		vStatic.push_back(tStaticNew);
	}

	switch (message)
	{
	case WM_INITDIALOG:
		pcOpenDlg->SetFilter(1, TEXT("所有文件（*.*）"), TEXT("*.*"));
		CheckDlgButton(hDlg, IDC_CHECK_FILECRC32, TRUE);
		SetDlgItemInt(hDlg, IDC_EDIT_ASKDELAY, 0, FALSE);
		SetDlgItemInt(hDlg, IDC_EDIT_TRANSFERPORT, 5600, FALSE);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK: // Ask transfer file
			{
				EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
				
				cTrans.m_tPackage.byStyle = TRN_PKG_STYLE_SETTING;
				cTrans.m_tPackage.byMode = TRN_PKG_MODE_SETTING_TRANSASK;

				cTrans.m_tPackage.dwMsgNumber = 0;
				cTrans.m_tPackage.dwMsgTotal = 1;

				unsigned int uiPort = GetDlgItemInt(hDlg, IDC_EDIT_TRANSFERPORT, nullptr, FALSE);

				pcFileTrans->GenerateKey();

				TCHAR szFile[MAX_PATH];
				GetDlgItemText(hDlg, IDC_EDIT_SENDFILE, szFile, MAX_PATH);
				TCHAR * szSimpFile = PathFindFileName(szFile);

				cTrans.m_tPackage.dwDataSize = sizeof(uiPort) +
					sizeof(CFileTransfer::TFileData::byKey) + (lstrlen(szSimpFile) + 1) * sizeof(TCHAR);

				memcpy(cTrans.m_tPackage.byData, &uiPort, sizeof(uiPort));
				memcpy(cTrans.m_tPackage.byData + sizeof(uiPort), pcFileTrans->m_tFileData.byKey,
					sizeof(CFileTransfer::TFileData::byKey));
				memcpy(((BYTE *)cTrans.m_tPackage.byData) +
					sizeof(uiPort) + sizeof(CFileTransfer::TFileData::byKey),
					(BYTE *)szSimpFile, (lstrlen(szSimpFile) + 1) * sizeof(TCHAR));

				cTrans.Send();

				pcFileTrans->SetHwnd(hDlg);
				pcFileTrans->SetCaption(szAppNameChn);
				pcFileTrans->InitParam(cTrans.GetMulticastIP().c_str(), cTrans.GetLocalIP().c_str(),
					uiPort, cTrans.GetName().c_str(), cTrans.GetPackageSize(), cTrans.GetTTL());
				pcFileTrans->EnterChat();
				pcFileTrans->BeginChat();
			}
			break;

		case IDC_BUTTON_SEND:
			{
				EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_SEND), FALSE);

				(*phFile) = CreateFile(pcFileTrans->GetFile().c_str(), GENERIC_WRITE,
					0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				
				DWORD dwFileSize = GetFileSize(*phFile, NULL);

				pcFileTrans->m_tPackage.dwMsgTotal = dwFileSize / 2048;
				pcFileTrans->m_tPackage.dwMsgNumber = 0;

				DWORD dwRead = 0;
				ReadFile(*phFile, pcFileTrans->m_tFileData.byData, 2048, &dwRead, NULL);
				pcFileTrans->m_tPackage.dwDataSize = dwRead + sizeof(CFileTransfer::TFileData::byKey);
				memcpy(pcFileTrans->m_tPackage.byData, &pcFileTrans->m_tFileData,
					pcFileTrans->m_tPackage.dwDataSize);
				pcFileTrans->Send();

				SetTimer(hDlg, 0, 1000, NULL);
			}
			break;

		case IDC_BUTTON_SENDFILE:
			{
				TCHAR szCrntDir[MAX_PATH];
				GetCurrentDirectory(MAX_PATH, szCrntDir);
				if (pcOpenDlg->CmnDlgOpenFile())
				{
					SetDlgItemText(hDlg, IDC_EDIT_SENDFILE, pcOpenDlg->GetFilePath());
				}
				SetCurrentDirectory(szCrntDir);
			}
			break;
		}
		return TRUE;

	case WM_TIMER:
		return TRUE;

	case WM_SOCKRECV:
		{
			SOCKADDR_IN tSockAddrFrom;
			if (pcFileTrans->Recv(&tSockAddrFrom))
			{
				if (TRN_PKG_STYLE_FILE == pcFileTrans->m_tPackage.byStyle)
				{

				}
				else if (TRN_PKG_STYLE_SETTING == pcFileTrans->m_tPackage.byStyle)
				{
					if (TRN_PKG_MODE_SETTING_TRANSACCEPT == pcFileTrans->m_tPackage.byMode)
					{

					}
				}
			}

			
		}
		return TRUE;

	case WM_CLOSE:
		// 文件是否传完？退出？
		
		CloseHandle(*phFile);
		pcFileTrans->EndChat();
		for (size_t n = 0; n < vStatic.size(); n++)
		{
			if (hDlg == vStatic[n].hDlg)
			{
				vStatic.erase(vStatic.begin() + n);
				break;
			}
		}
		EndDialog(hDlg, 0);
		return TRUE;
	}

	return FALSE;
}

BOOL CALLBACK DlgProcRecver(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void __cdecl RecvFileThread(void * pArgc)
{
	HWND hwndParent = (HWND)pArgc;
	DialogBox((HINSTANCE)GetWindowLong(hwndParent, GWL_HINSTANCE),
		MAKEINTRESOURCE(IDD_DIALOG_FILESENDER), GetDesktopWindow(), DlgProcRecver);
	_endthread();
}

BOOL CALLBACK DlgProcRecver(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return TRUE;

	case WM_CLOSE:
		EndDialog(hDlg, 0);
		return TRUE;
	}

	return FALSE;
}
*/

int SendFile(LPCTSTR szFile);
void __cdecl SendFileThread(void * pArgc)
{
	TCHAR szFile[MAX_PATH];

	COSDlg cOpenDlg(NULL);
	cOpenDlg.SetFilter(1, TEXT("所有文件（*.*）"), TEXT("*.*"));

	TCHAR szCrntDir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, szCrntDir);
	if (cOpenDlg.CmnDlgOpenFile())
	{
		lstrcpy(szFile, cOpenDlg.GetFilePath());
	}
	SetCurrentDirectory(szCrntDir);

	SendFile(szFile);

	MessageBox(NULL, TEXT("发送完成！"), szAppNameChn, MB_OK);

	_endthread();
}

int RecvFile(LPTSTR szFile);
void __cdecl RecvFileThread(void * pArgc)
{
	TCHAR szFile[MAX_PATH];

	while (true)
	{
		RecvFile(szFile);
		
		TCHAR szInfo[1024];
		wsprintf(szInfo, TEXT("多播网络上接收到文件，保存至\r\n%s"), szFile);

		MessageBox(NULL, szInfo, szAppNameChn, MB_OK);
	}

	_endthread();
}