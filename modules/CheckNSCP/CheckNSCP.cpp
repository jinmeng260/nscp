/**************************************************************************
*   Copyright (C) 2004-2007 by Michael Medin <michael@medin.name>         *
*                                                                         *
*   This code is part of NSClient++ - http://trac.nakednuns.org/nscp      *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#include "stdafx.h"


#include <utils.h>
#include <file_helpers.hpp>
#include <Mutex.h>

#include "CheckNSCP.h"
CheckNSCP gCheckNSCP;

namespace sh = nscapi::settings_helper;

CheckNSCP::CheckNSCP() {
}
CheckNSCP::~CheckNSCP() {
}

bool CheckNSCP::loadModule() {
	return false;
}
bool CheckNSCP::loadModuleEx(std::wstring alias, NSCAPI::moduleLoadMode mode) {
	//std::wstring appRoot = file_helpers::folders::get_local_appdata_folder(SZAPPNAME);
	try {

		sh::settings_registry settings(nscapi::plugin_singleton->get_core());
		settings.set_alias(_T("crash"), alias);

		settings.alias().add_path_to_settings()
			(_T("CRASH SECTION"), _T("Configure crash handling properties."))
			;

		settings.alias().add_key_to_settings()
			(_T("archive"), sh::wstring_key(&crashFolder ),
			_T("ARCHIVE FOLDER"), _T("The archive folder for crash dunpes."))
			;

		settings.register_all();
		settings.notify();

		get_core()->registerCommand(_T("check_nscp"), _T("Check the internal healt of NSClient++."));
		//crashFolder = NSCModuleHelper::getSettingsString(CRASH_SECTION_TITLE, CRASH_ARCHIVE_FOLDER, file_helpers::folders::get_subfolder(appRoot, _T("crash dumps")));


	} catch (nscapi::nscapi_exception &e) {
		NSC_LOG_ERROR_STD(_T("Failed to register command: ") + e.msg_);
	} catch (...) {
		NSC_LOG_ERROR_STD(_T("Failed to register command."));
	}

	return true;
}
bool CheckNSCP::unloadModule() {
	return true;
}
bool CheckNSCP::hasCommandHandler() {
	return true;
}
bool CheckNSCP::hasMessageHandler() {
	return true;
}
std::string CheckNSCP::render(int msgType, const std::string file, int line, std::string message) {
	return message;
}
void CheckNSCP::handleMessage(int msgType, const std::string file, int line, std::string message) {
	if (msgType != NSCAPI::error||msgType != NSCAPI::critical)
		return;
	std::string err = render(msgType, file, line, message);
	{
		MutexLock lock(mutex_);
		if (!lock.hasMutex())
			return;
		errors_.push_back(err);
	}
}


int CheckNSCP::get_crashes(std::wstring &last_crash) {
	if (!file_helpers::checks::is_directory(crashFolder)) {
		return 0;
	}
	WIN32_FIND_DATA wfd;
	FILETIME previous;
	previous.dwHighDateTime = 0;
	previous.dwLowDateTime = 0;
	int count = 0;
	std::wstring find = crashFolder + _T("\\*.txt");
	HANDLE hFind = FindFirstFile(find.c_str(), &wfd);
	std::wstring last_file;
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (CompareFileTime(&wfd.ftCreationTime, &previous) == 1) {
				previous = wfd.ftCreationTime;
				last_file = wfd.cFileName;
			}
			count++;
		} while (FindNextFile(hFind, &wfd));
		FindClose(hFind);
	}
	if (count > 0)
		last_crash = last_file;
	return count;
}

int CheckNSCP::get_errors(std::wstring &last_error) {
	MutexLock lock(mutex_);
	if (!lock.hasMutex()) {
		last_error = _T("Failed to get mutex.");
		return 1;
	}
	if (errors_.empty())
		return 0;
	last_error = to_wstring(errors_.front());
	return errors_.size();
}

NSCAPI::nagiosReturn CheckNSCP::check_nscp( std::list<std::wstring> arguments, std::wstring & msg, std::wstring & perf )
{
	std::wstring last_crash;
	int crash_count = get_crashes(last_crash);
	if (crash_count > 0)
		strEx::append_list(msg, strEx::itos(crash_count) + _T(" crash(es), last crash: ") + last_crash, _T(", "));

	std::wstring last_error;
	int err_count = get_errors(last_error);
	if (err_count > 0)
		strEx::append_list(msg, strEx::itos(err_count) + _T(" error(s), last error: ") + last_error, _T(", "));

	if (msg.empty())
		msg = _T("OK: 0 crash(es), 0 error(s)");
	else
		msg = _T("ERROR: ") + msg;

	return (err_count > 0 || crash_count > 0) ? NSCAPI::returnCRIT:NSCAPI::returnOK;
}

NSCAPI::nagiosReturn CheckNSCP::handleCommand(const strEx::wci_string command, std::list<std::wstring> arguments, std::wstring &message, std::wstring &perf) {
	if (command == _T("check_nscp")) {
		return check_nscp(arguments, message, perf);
	}
	return NSCAPI::returnIgnored;
}

NSC_WRAP_DLL();
NSC_WRAPPERS_MAIN_DEF(gCheckNSCP);
NSC_WRAPPERS_HANDLE_MSG_DEF(gCheckNSCP);
NSC_WRAPPERS_HANDLE_CMD_DEF(gCheckNSCP);