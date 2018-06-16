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

#include "../CmnDlg.hpp"
using namespace COMMONDIALOG;

#include "../Logger.hpp"

static const TCHAR szAppNameEng[] = TEXT("MulticastChat");
static const TCHAR szAppNameChn[] = TEXT("MulticastChat�ಥ������");

static const TCHAR szUserLicense[] =
	TEXT("MulticastChat�ಥ������ʹ��Э�鼰���˵����\r\n");

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
			TEXT("���棺Windows�汾���ϻ򸸴��ڲ�����Ч�����ܻᵼ�´���\r\n�Ƿ�������У�"),
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
			SendDlgItemMessage(hDlg, IDC_IPADDRESS, IPM_SETADDRESS, 0, MAKEIPADDRESS(224, 0, 0, 0));

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
					MessageBox(hDlg, TEXT("�ö˿ں�Ϊ�����˿ںţ��˿ںŷ�Χ��1024-65535��"),
						szAppNameChn, MB_ICONERROR | MB_OK);
					break;
				}
				else if (uiPort > 65535)
				{
					MessageBox(hDlg, TEXT("�ö˿ں���Ч���˿ںŷ�Χ��1024-65535��"),
						szAppNameChn, MB_ICONERROR | MB_OK);
					break;
				}

				if (uiPackageSize > 65536)
				{
					MessageBox(hDlg, TEXT("�������ݰ���С��Ч������Ϊ64KB��65536Bytes����"),
						szAppNameChn, MB_ICONERROR | MB_OK);
					break;
				}

				if (uiTTL > 128)
				{
					MessageBox(hDlg, TEXT("TTLֵ��Ч������Ϊ128��"),
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
				TEXT("���ִ����볢��������������ϵ����Ա��"),
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
	//static COSDlg cOpenDlg(hDlg);
	//static COSDlg cSaveDlg(hDlg);
	//cOpenDlg.SetFilter(2, TEXT("�ı��ļ���*.txt��"), TEXT("*.txt"), TEXT("�����ļ���*.*��"), TEXT("*.*"));
	//cOpenDlg.SetDefExt(TEXT("txt"));
	//cSaveDlg.SetFilter(2, TEXT("�ı��ļ���*.txt��"), TEXT("*.txt"), TEXT("�����ļ���*.*��"), TEXT("*.*"));
	//cSaveDlg.SetDefExt(TEXT("txt"));

	switch (message)
	{
	case WM_INITDIALOG:
		{
			cTrans.SetHwnd(hDlg);

			TString szWindowsName(TEXT("MulticastChat�ಥ������"));
			szWindowsName += TEXT(" - ");
			szWindowsName += cTrans.GetMulticastIP();
			szWindowsName += TEXT(" - ");
			szWindowsName += cTrans.GetMulticastPortString();
			SetWindowText(hDlg, szWindowsName.c_str());

			if (!cTrans.BeginChat())
			{
				MessageBox(hDlg,
					TEXT("����������ʧ�ܣ���������״̬����ϵ����Ա��"),
					szAppNameChn, MB_ICONERROR | MB_OK);
				EndDialog(hDlg, 100);
			}
		}
		return TRUE;

	case WM_SHOWWINDOW:
		{
			TString szSend(TEXT("[ϵͳ��Ϣ]\r\n"));
			szSend += cTrans.GetName();
			szSend += TEXT(" ���뵱ǰ�ಥ�������ڡ�");

			cTrans.m_tPackage.byStyle     = TRN_PKG_STYLE_MESSAGE;
			cTrans.m_tPackage.byMode      = 0;
			
			cTrans.m_tPackage.dwDataSize = (szSend.length() + 1) * sizeof(TCHAR);
			cTrans.m_tPackage.dwDataCRC32 = 0;
			
			cTrans.m_tPackage.dwMsgNumber = 0;
			cTrans.m_tPackage.dwMsgTotal  = 1;
			
			memcpy(cTrans.m_tPackage.byData, szSend.data(), cTrans.m_tPackage.dwDataSize);

			cTrans.Send();
		}
		return TRUE;

	case WM_SOCKRECV:
		{
			SOCKADDR_IN tSockAddrFrom;
			cTrans.Recv(&tSockAddrFrom);


		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			{
			}
			break;

		case IDCANCEL:
			SendMessage(hDlg, WM_CLOSE, 0, 0);
			break;
		}
		return TRUE;

	case WM_CLOSE:
		if (IDOK == MessageBox(hDlg,
			TEXT("ȷ�ϼ����˳������ң������ڴ����ļ������񽫱�����ֹ��"),
			szAppNameChn, MB_ICONWARNING | MB_OKCANCEL))
		{
			cTrans.EndChat();
			EndDialog(hDlg, 1);
		}
		return TRUE;
	}

	return FALSE;
}
