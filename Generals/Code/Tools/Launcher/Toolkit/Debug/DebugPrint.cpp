/*
**	Command & Conquer Generals(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/******************************************************************************
*
* FILE
*     $Archive:  $
*
* DESCRIPTION
*     Debug printing mechanism
*
* PROGRAMMER
*     Denzil E. Long, Jr.
*     $Author:  $
*
* VERSION INFO
*     $Modtime:  $
*     $Revision:  $
*
******************************************************************************/

#ifdef _DEBUG

#include "DebugPrint.h"
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

char debugLogName[_MAX_PATH];

/******************************************************************************
*
* NAME
*     DebugPrint(String, ArgList...)
*
* DESCRIPTION
*     Ouput debug print messages to the debugger and log file.
*
* INPUTS
*     String  - String to output.
*     ArgList - Argument list
*
* RESULT
*     NONE
*
******************************************************************************/

void __cdecl DebugPrint(const char* string, ...)
	{
	static char _buffer[1024];
	static char _filename[512] = "";

	if (string != NULL)
		{
		// Format string
		va_list	va;
		va_start(va, string);
		//Check the return value of vsprintf_s
		int result = vsprintf_s(_buffer, sizeof(_buffer), string, va);
		if (result < 0 || result >= sizeof(_buffer)) {
			OutputDebugString("Error: vsprintf_s failed or buffer overflow!\n");
		}
		va_end(va);

		// Open log file
		HANDLE file = INVALID_HANDLE_VALUE;

		if (strlen(_filename) == 0)
			{
			char path[_MAX_PATH];
			char drive[_MAX_DRIVE];
			char dir[_MAX_DIR];

			GetModuleFileName(GetModuleHandle(NULL), &path[0], sizeof(path));
			// Correctly using _splitpath_s with buffer size specifications
			errno_t err = _splitpath_s(path, drive, _MAX_DRIVE, dir, _MAX_DIR, nullptr, 0, nullptr, 0);
			if (err != 0) {
				OutputDebugString("Error: _splitpath_s failed!\n");
				return; // Or handle the error appropriately
			}
			//Correctly using _makepath_s
			err = _makepath_s(_filename, sizeof(_filename), drive, dir, debugLogName, "txt");
			if (err != 0) {
				OutputDebugString("Error: _makepath_s failed!\n");
				return;
			}

			OutputDebugString("Creating ");
			OutputDebugString(_filename);
			OutputDebugString("\n");

			file = CreateFile(_filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL, NULL);
			}
		else
			{
			file = CreateFile(_filename, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
				FILE_ATTRIBUTE_NORMAL, NULL);
			}
		
		// Send string to debugger
		OutputDebugString(_buffer);

		// Insert carriage return after newlines
		int i = 0;

		while (_buffer[i] != '\0')
			{
			if (_buffer[i] == '\n')
				{
				int end = strlen(_buffer);
				assert((end + 1) <= sizeof(_buffer));

				while (end >= i)
					{
					_buffer[end + 1] = _buffer[end];
					end--;
					}

				_buffer[i] = '\r';
				i++;
				}

			i++;
			}

		// Send string to log file
		assert(file != INVALID_HANDLE_VALUE);

		if (file != INVALID_HANDLE_VALUE)
			{
			SetFilePointer(file, 0, NULL, FILE_END);

			DWORD written;
			WriteFile(file, &_buffer[0], strlen(_buffer), &written, NULL);
			
			CloseHandle(file);
			}
		}
	}


/******************************************************************************
*
* NAME
*     PrintWin32Error
*
* DESCRIPTION
*     Display Win32 error message (Error retrieved from GetLastError())
*
* INPUTS
*     Message string
*
* RESULT
*     NONE
*
******************************************************************************/

void __cdecl PrintWin32Error(const char* string, ...)
	{
	static char _buffer[1024];

	if (string != NULL)
		{
		// Format string
		va_list	va;
		va_start(va, string);
		int result = vsprintf_s(_buffer, sizeof(_buffer), string, va);
		if (result < 0 || result >= sizeof(_buffer)) {
			OutputDebugString("Error: vsprintf_s failed or buffer overflow!\n");
		}
		va_end(va);

		LPVOID lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

		DebugPrint("***** Win32 Error: %s\n", _buffer);
		DebugPrint("      Reason: %s\n", (char*)lpMsgBuf);

		LocalFree(lpMsgBuf);
		}
	}

#endif // _DEBUG
