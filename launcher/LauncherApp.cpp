
// DashFactionLauncher.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "LauncherApp.h"
#include "MainDlg.h"
#include "LauncherCommandLineInfo.h"
#include <common/GameConfig.h>
#include <common/version.h>
#include <common/ErrorUtils.h>
#include <launcher_common/PatchedAppLauncher.h>
#include <log/Logger.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// LauncherApp initialization
BOOL LauncherApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
    Win32xx::LoadCommonControls();

    // Command line parsing
    INFO("Parsing command line");
    LauncherCommandLineInfo cmd_line_info;
    cmd_line_info.Parse();

    if (cmd_line_info.HasHelpFlag())
    {
        // Note: we can't use stdio console API in win32 application
        Message(NULL,
            "Usage: DashFactionLauncher [-game] [-level name] [-editor] args...\n"
            "-game        Starts game immediately\n"
            "-level name  Starts game immediately and loads specified level\n"
            "-editor      Starts level editor immediately\n"
            "args...      Additional arguments passed to game or editor\n",
            "Dash Faction Launcher Help", MB_OK | MB_ICONINFORMATION);
        return FALSE;
    }

    // Migrate Dash Faction config from old version
    MigrateConfig();

    // Launch game or editor based on command line flag
    if (cmd_line_info.HasGameFlag())
    {
        LaunchGame(nullptr);
        return FALSE;
    }

    if (cmd_line_info.HasEditorFlag())
    {
        LaunchEditor(nullptr);
        return FALSE;
    }

    // Show main dialog
    INFO("Showing main dialog");
	MainDlg dlg;
	dlg.DoModal();

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
    INFO("Closing the launcher");
	return FALSE;
}

void LauncherApp::MigrateConfig()
{
    try
    {
        GameConfig config;
        if (config.load() && config.dash_faction_version != VERSION_STR)
        {
            INFO("Migrating config");
            if (config.tracker == "rf.thqmultiplay.net" && config.dash_faction_version.empty()) // < 1.1.0
                config.tracker = DEFAULT_RF_TRACKER;
            config.dash_faction_version = VERSION_STR;
            config.save();
        }
    }
    catch (std::exception&)
    {
        // ignore
    }
}

bool LauncherApp::LaunchGame(HWND hwnd, const char* mod_name)
{
    WatchDogTimer::ScopedStartStop wdt_start{m_watch_dog_timer};
    GameLauncher launcher;

    try
    {
        INFO("Checking installation");
        launcher.check_installation();
        INFO("Installation is okay");
    }
    catch (FileNotFoundException &e)
    {
        std::stringstream ss;
        std::string download_url;

        ss << "Game directory validation has failed! File is missing:\n" << e.get_file_name() << "\n"
            << "Please make sure game executable specified in options is located inside a valid Red Faction installation "
            << "root directory.";
        std::string str = ss.str();
        Message(hwnd, str.c_str(), nullptr, MB_OK | MB_ICONWARNING);
        return false;
    }
    catch (FileHashVerificationException &e)
    {
        std::stringstream ss;
        std::string download_url;

        ss << "Game directory validation has failed! File " << e.get_file_name() << " has unrecognized hash sum.\n\n"
            << "SHA1:\n" << e.get_sha1();
        if (e.get_file_name() == "tables.vpp") {
            ss << "\n\nIt can prevent multiplayer functionality or entire game from working properly.\n"
                << "If your game has not been updated to 1.20 please do it first. If this warning still shows up "
                << "replace your tables.vpp file with original 1.20 NA " << e.get_file_name() << " available on FactionFiles.com.\n"
                << "Do you want to open download page?";
            std::string str = ss.str();
            download_url = "https://www.factionfiles.com/ff.php?action=file&id=517871";
            int result = Message(hwnd, str.c_str(), nullptr, MB_YESNOCANCEL | MB_ICONWARNING);
            if (result == IDYES) {
                ShellExecuteA(hwnd, "open", download_url.c_str(), nullptr, nullptr, SW_SHOW);
                return false;
            }
            else if (result == IDCANCEL) {
                return false;
            }
        }
        else {
            std::string str = ss.str();
            if (Message(hwnd, str.c_str(), nullptr, MB_OKCANCEL | MB_ICONWARNING) == IDCANCEL) {
                return false;
            }
        }
    }

    try
    {
        INFO("Launching the game...");
        launcher.launch(mod_name);
        INFO("Game launched!");
        return true;
    }
    catch (PrivilegeElevationRequiredException&)
    {
        Message(hwnd,
            "Privilege elevation is required. Please change RF.exe file properties and disable all "
            "compatibility settings (Run as administrator, Compatibility mode for Windows XX, etc.) or run "
            "Dash Faction launcher as administrator.",
            nullptr, MB_OK | MB_ICONERROR);
    }
    catch (FileNotFoundException& e)
    {
        Message(hwnd, "Game executable has not been found. Please set a proper path in Options.",
                nullptr, MB_OK | MB_ICONERROR);
    }
    catch (FileHashVerificationException &e)
    {
        std::stringstream ss;
        ss << "Unsupported game executable has been detected!\n\n"
            << "SHA1:\n" << e.get_sha1() << "\n\n"
            << "Dash Faction supports only unmodified Red Faction 1.20 NA executable.\n"
            << "If your game has not been updated to 1.20 please do it first. If the error still shows up "
            << "replace your RF.exe file with original 1.20 NA RF.exe available on FactionFiles.com.\n"
            << "Click OK to open download page.";
        std::string str = ss.str();
        if (Message(hwnd, str.c_str(), nullptr, MB_OKCANCEL | MB_ICONERROR) == IDOK)
            ShellExecuteA(hwnd, "open", "https://www.factionfiles.com/ff.php?action=file&id=517545", NULL, NULL, SW_SHOW);
    }
    catch (std::exception &e)
    {
        std::string msg = generate_message_for_exception(e);
        Message(hwnd, msg.c_str(), nullptr, MB_ICONERROR | MB_OK);
    }
    return false;
}

bool LauncherApp::LaunchEditor(HWND hwnd, const char* mod_name)
{
    WatchDogTimer::ScopedStartStop wdt_start{m_watch_dog_timer};
    EditorLauncher launcher;
    try
    {
        INFO("Launching editor...");
        launcher.launch(mod_name);
        INFO("Editor launched!");
        return true;
    }
    catch (std::exception &e)
    {
        std::string msg = generate_message_for_exception(e);
        Message(hwnd, msg.c_str(), nullptr, MB_ICONERROR | MB_OK);
        return false;
    }
}

int LauncherApp::Message(HWND hwnd, const char *pszText, const char *pszTitle, int Flags)
{
    WatchDogTimer::ScopedPause wdt_pause{m_watch_dog_timer};
    INFO("%s: %s", pszTitle ? pszTitle : "Error", pszText);
    if (GetSystemMetrics(SM_CMONITORS) > 0) {
        return MessageBoxA(hwnd, pszText, pszTitle, Flags);
    }
    else
    {
        fprintf(stderr, "%s: %s", pszTitle ? pszTitle : "Error", pszText);
        return -1;
    }
}
