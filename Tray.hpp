#pragma once

//
// System tray icon helper (GUI/Windows-subsystem build only).
//
// Self-contained: owns a hidden top-level window, a notify icon, and a
// right-click "Exit" menu. A normal (not message-only) top-level window is
// used so the icon is re-added when Explorer restarts (TaskbarCreated).
//
// No networking, no files, no registry — pure Win32 UI. Pumped by the
// existing message loop on the thread that calls Create().
//

#include <windows.h>
#include <shellapi.h>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")

class TrayIcon
{
public:

	using ExitCallback = void (*)(void* context);

	// Creates the hidden window and adds the tray icon. Returns false on failure.
	bool Create(HINSTANCE hinstance, int icon_id, const wchar_t* tooltip, ExitCallback on_exit, void* context)
	{
		m_on_exit = on_exit;
		m_context = context;
		m_taskbar_created = RegisterWindowMessageW(L"TaskbarCreated");

		WNDCLASSEXW wc = { sizeof(wc) };
		wc.lpfnWndProc = &TrayIcon::WndProc;
		wc.hInstance = hinstance;
		wc.lpszClassName = L"SoundKeeperTrayWindow";
		RegisterClassExW(&wc); // Ignore failure if already registered.

		// Normal top-level window, never shown -> invisible but receives broadcasts.
		m_hwnd = CreateWindowExW(0, L"SoundKeeperTrayWindow", L"Sound Keeper",
			WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL, hinstance, this);
		if (!m_hwnd) { return false; }

		m_icon = static_cast<HICON>(LoadImageW(hinstance, MAKEINTRESOURCEW(icon_id), IMAGE_ICON,
			GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));
		if (!m_icon) { m_icon = LoadIconW(NULL, IDI_APPLICATION); m_owns_icon = false; }

		m_nid.cbSize = sizeof(m_nid);
		m_nid.hWnd = m_hwnd;
		m_nid.uID = 1;
		m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		m_nid.uCallbackMessage = kCallbackMsg;
		m_nid.hIcon = m_icon;
		lstrcpynW(m_nid.szTip, tooltip, ARRAYSIZE(m_nid.szTip));

		return Shell_NotifyIconW(NIM_ADD, &m_nid) != FALSE;
	}

	// Removes the tray icon and destroys the window. Safe to call multiple times.
	void Destroy()
	{
		if (m_hwnd)
		{
			Shell_NotifyIconW(NIM_DELETE, &m_nid);
			DestroyWindow(m_hwnd);
			m_hwnd = NULL;
		}
		if (m_icon && m_owns_icon)
		{
			DestroyIcon(m_icon);
		}
		m_icon = NULL;
	}

private:

	static constexpr UINT kCallbackMsg = WM_APP + 1;

	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		if (msg == WM_NCCREATE)
		{
			auto* cs = reinterpret_cast<CREATESTRUCTW*>(lparam);
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
			return DefWindowProcW(hwnd, msg, wparam, lparam);
		}

		auto* self = reinterpret_cast<TrayIcon*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
		return self ? self->Handle(hwnd, msg, wparam, lparam) : DefWindowProcW(hwnd, msg, wparam, lparam);
	}

	LRESULT Handle(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		if (msg == kCallbackMsg)
		{
			const UINT event = LOWORD(lparam);
			if (event == WM_RBUTTONUP || event == WM_CONTEXTMENU || event == WM_LBUTTONUP)
			{
				ShowMenu(hwnd);
			}
			return 0;
		}

		if (m_taskbar_created != 0 && msg == m_taskbar_created)
		{
			// Explorer restarted: re-add the icon.
			Shell_NotifyIconW(NIM_ADD, &m_nid);
			return 0;
		}

		return DefWindowProcW(hwnd, msg, wparam, lparam);
	}

	void ShowMenu(HWND hwnd)
	{
		POINT pt;
		GetCursorPos(&pt);

		HMENU menu = CreatePopupMenu();
		AppendMenuW(menu, MF_STRING, 1, L"Exit Sound Keeper");

		// Required so the menu dismisses correctly for a tray window.
		SetForegroundWindow(hwnd);
		const int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
			pt.x, pt.y, 0, hwnd, NULL);
		DestroyMenu(menu);
		PostMessageW(hwnd, WM_NULL, 0, 0);

		if (cmd == 1 && m_on_exit)
		{
			m_on_exit(m_context);
		}
	}

	ExitCallback     m_on_exit = nullptr;
	void*            m_context = nullptr;
	HWND             m_hwnd = NULL;
	HICON            m_icon = NULL;
	bool             m_owns_icon = true;
	UINT             m_taskbar_created = 0;
	NOTIFYICONDATAW  m_nid = {};
};
