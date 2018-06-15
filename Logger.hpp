/************************************************************

BSD 2-Clause License

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Copyright (c) 2018, KondeU, All rights reserved.

Project:     Logger model
File:        Logger.hpp
Description: Could only run on Windows. Use to log run information.
Date:        2018-06-13
Version:     1.1
Authors:     Deyou Kong <370242479@qq.com>
History:     01, 2018-06-13, Deyou Kong, Create file and implement it.

See: http://www.github.com/KondeU

************************************************************/

#pragma once

#include <Windows.h>
#include <sstream>
#include <string>

#ifdef UNICODE
#define TString std::wstring
#define TStringStream std::wstringstream
#else
#define TString std::string
#define TStringStream std::stringstream
#endif

TString ErrorInfo(DWORD * pdwLastError = nullptr)
{
	DWORD dwLastError = GetLastError();
	if (pdwLastError)
	{
		*pdwLastError = dwLastError;
	}

	LPVOID lpMsgBuf = nullptr;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, dwLastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);

	TString szErrInfo((PTCHAR)lpMsgBuf);

	LocalFree(lpMsgBuf);

	return szErrInfo;
}

void ErrMsgBox(HWND hwnd, LPCTSTR szCaption)
{
	DWORD dwLastError = 0;
	TString szErrInfo = ErrorInfo(&dwLastError);

	TStringStream ss;
	ss << TEXT("发生运行错误，系统错误代码：") << dwLastError;
	
	TString szErrShow;
	ss >> szErrShow;
	szErrShow += TEXT("\r\n信息：") + szErrInfo;

	MessageBox(hwnd, szErrShow.c_str(), szCaption, MB_ICONERROR | MB_OK);
}
