/* libnotify.c
 * Artha - Free cross-platform open thesaurus
 * Copyright (C) 2009, 2010 Sundaram Ramaswamy, legends2k@yahoo.com
 *
 * Artha is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Artha is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Artha; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


/*
 * libnotify (win32) implementation ~ defines the DLL's exported functions
 */


/*

Notification window geometry
________________________________________________
________________________________________________
| |@@@@@@@@@@| |Summary                      | |
| |@@      @@| |Body                         | |
| |@  ICON  @| |-----------------------------| |
| |@@      @@| |-----------------------------| |
| |@@@@@@@@@@| |-----------------------------| |
________________________________________________
________________________________________________

| |,
--- and __
---     __ all 3 are equal => icon_padding white space

*/


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Exclude rarely-used stuff from Windows headers */
#define WIN32_LEAN_AND_MEAN

/* Windows Header Files: */
#include <windows.h>

/* include libnotify functions to be implemented */
#include "libnotify.h"
#include <stdlib.h>

/* MSVC below 2010 don't have stdint.h */
#if (defined(_MSC_VER) && (_MSC_VER < 1600))
typedef UINT8 uint8_t;
typedef UINT16 uint16_t;
#else
#include <stdint.h>
#endif

/* from MSDN LoadImage page's comments section */
#define OIC_BANG 32515
#define OIC_NOTE 32516

/* user defined messages */
#define WM_LIBNOTIFYSHOW	(WM_USER + 98)
#define WM_LIBNOTIFYCLOSE	(WM_USER + 99)
#define WM_LIBNOTIFYEXIT	(WM_USER + 100)

#define MAX_SUMMARY_LENGTH	100
#define MAX_BODY_LENGTH		MAX_PATH

/* timers used & their IDs */
enum TIMER_IDS
{
	TIMER_ANIMATION = 1,
	TIMER_NOTIFICATION
};

enum ICON_IDS
{
	ICON_INFO,
	ICON_WARNING,
	ICON_COUNT
};

struct _NotifyNotification
{
	wchar_t summary[MAX_SUMMARY_LENGTH];
	wchar_t body[MAX_BODY_LENGTH];
	enum ICON_IDS icon_req;
};

struct _NotificationWindow
{
	HWND notification_window;
	uint8_t notification_window_alpha;
	RECT notification_window_rect;
	LONG notification_window_width;
	LONG notification_window_height;
	uint16_t summary_body_divider;
};
typedef struct _NotificationWindow NotificationWindow;

/* constants */
static const wchar_t*	wstrLibInitEvent = L"LibNotifyInitializedEvent";
static const uint8_t	rounded_rect_edge = 15;
static const uint8_t	mouse_over_alpha = (0xFF / 4);
static const uint8_t	fade_duration = 15;
static const uint16_t	min_notification_timeout = 3000;
static const uint16_t	window_offset_factor = 10;
static const DWORD		thread_wait_timeout = 2000;
static const uint16_t	milliseconds_per_word = 750;

/* globals */
// set only once by init and not modified else where
// hence thread safe as far as no edits are made other than in init
static HICON			notification_icons[ICON_COUNT] = {NULL};
static HFONT			font_summary = NULL;
static HFONT			font_body = NULL;
static LONG				notification_window_width_max = 0;
static uint16_t			icon_size = 0;
static uint16_t			icon_padding = 0;

// used only by the interface functions
// hence thread safe w.r.t to the window thread
static HANDLE notification_thread = NULL;
static DWORD  notification_thread_id = 0;

// event to prevent multiple initializations
static HANDLE event_lib_inited = NULL;

// CS to guard notification_data from the readers-writers issue
static CRITICAL_SECTION thread_guard = {0};

static NotificationWindow notify_wnd = {0};

/* function declarations */
static DWORD notification_daemon_main(LPVOID lpdwThreadParam);
static LRESULT CALLBACK notificationWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK mouse_over_hook_proc(int nCode, WPARAM wParam, LPARAM lParam);
static uint16_t word_count(const wchar_t *str);
static gboolean byte_to_wide_string(const gchar *byte_string, wchar_t *wide_string, gint max_buffer);


/* dll exported functions */
LIBNOTIFY_API gboolean notify_init(const char *app_name)
{
	DWORD thread_ret_code = 0;
	HANDLE wait_handles[2] = {NULL};

	UNREFERENCED_PARAMETER(app_name);

	// forbid multiple calls, by checking if the event is already created
	HANDLE evtHandle = OpenEvent(EVENT_MODIFY_STATE, FALSE, wstrLibInitEvent);
	if(evtHandle)
	{
		CloseHandle(evtHandle);
		evtHandle = NULL;
		return FALSE;	// currently only one app. can use the library
	}
	else		// library already initialized
	{
		event_lib_inited = CreateEvent(NULL, TRUE, FALSE, wstrLibInitEvent);
		if(!event_lib_inited) goto cleanup_and_exit;
		InitializeCriticalSection(&thread_guard);
	}

	font_summary = CreateFont(0,
							  0,
							  0,
							  0,
							  FW_BOLD,
							  FALSE,
							  FALSE,
							  FALSE,
							  ANSI_CHARSET,
							  OUT_DEFAULT_PRECIS,
							  CLIP_DEFAULT_PRECIS,
							  ANTIALIASED_QUALITY,
							  DEFAULT_PITCH | FF_ROMAN,
							  NULL);
	if(!font_summary) goto cleanup_and_exit;

	font_body = CreateFont(0,
						   0,
						   0,
						   0,
						   FW_NORMAL,
						   FALSE,
						   FALSE,
						   FALSE,
						   ANSI_CHARSET,
						   OUT_DEFAULT_PRECIS,
						   CLIP_DEFAULT_PRECIS,
						   ANTIALIASED_QUALITY,
						   DEFAULT_PITCH | FF_ROMAN,
						   NULL);
	if(!font_body) goto cleanup_and_exit;

	// these needn't be released, since they're OS level shared resources
	notification_icons[0] = LoadImage(NULL,
									  MAKEINTRESOURCE(OIC_NOTE),
									  IMAGE_ICON,
									  0,
									  0,
									  LR_DEFAULTSIZE | LR_SHARED);
	notification_icons[1] = LoadImage(NULL,
									  MAKEINTRESOURCE(OIC_BANG),
									  IMAGE_ICON,
									  0,
									  0,
									  LR_DEFAULTSIZE | LR_SHARED);
	if(!notification_icons[0] || !notification_icons[1])
		goto cleanup_and_exit;

	// set globals which are set only once and read then on multiple times
	/* screen width / 3.5 is the maximum allowed notification width */
	notification_window_width_max = (GetSystemMetrics(SM_CXSCREEN) * 2) / 7;
	icon_size = (uint16_t) GetSystemMetrics(SM_CXICON);
	icon_padding = icon_size / 3;		/* icon spacing is a third of the icon width */

	notification_thread = CreateThread(NULL,
									   0,
									   (LPTHREAD_START_ROUTINE) &notification_daemon_main,
									   event_lib_inited,
									   0,
									   &notification_thread_id);
	if(!notification_thread) goto cleanup_and_exit;

	// block and confirm all things are right and then return success
	wait_handles[0] = event_lib_inited, wait_handles[1] = notification_thread;
	if((WAIT_OBJECT_0 + ARRAYSIZE(wait_handles)) > WaitForMultipleObjects(ARRAYSIZE(wait_handles),
																		  wait_handles,
																		  FALSE,
																		  thread_wait_timeout))
	{
		if(GetExitCodeThread(notification_thread, &thread_ret_code))
		{
			if(STILL_ACTIVE == thread_ret_code)
			{
				return TRUE;
			}
		}
	}

/* order is similar to stack unwind in C++ */
cleanup_and_exit:
	if(notification_thread)
	{
		/* terminate and free the thread */
		TerminateThread(notification_thread, (DWORD) -1);
		CloseHandle(notification_thread);
		notification_thread = NULL;
	}

	if(font_body)
	{
		DeleteObject(font_body);
		font_body = NULL;
	}

	if(font_summary)
	{
		DeleteObject(font_summary);
		font_summary = NULL;
	}

	if(event_lib_inited)
	{
		DeleteCriticalSection(&thread_guard);
		CloseHandle(event_lib_inited);
		event_lib_inited = NULL;
	}

	return FALSE;
}

LIBNOTIFY_API void notify_uninit(void)
{
	if(event_lib_inited)
	{
		// it should be next to delete CS but it's up here to guard multiple
		// uninit calls
		CloseHandle(event_lib_inited);
		event_lib_inited = NULL;

		if(notification_thread)
		{
			PostThreadMessage(notification_thread_id,
							  WM_LIBNOTIFYEXIT,
							  0,
							  0);

			/* if the wait fails, preempt the thread */
			if(WAIT_OBJECT_0 != WaitForSingleObject(notification_thread, thread_wait_timeout))
				TerminateThread(notification_thread, (DWORD) -1);
			CloseHandle(notification_thread);
			notification_thread = NULL;
			notification_thread_id = 0;
		}

		if(font_summary)
		{
			DeleteObject(font_summary);
			font_summary = NULL;
		}

		if(font_body)
		{
			DeleteObject(font_body);
			font_body = NULL;
		}

		DeleteCriticalSection(&thread_guard);
	}
}

LIBNOTIFY_API NotifyNotification* notify_notification_new(
	const gchar *summary, const gchar *body,
	const gchar *icon)
{
	if(!notification_thread) return NULL;

	NotifyNotification *notification_data = (NotifyNotification*) malloc(sizeof(NotifyNotification));
	if(notification_data)
	{
		memset(notification_data, 0, sizeof(NotifyNotification));
		notify_notification_update(notification_data, summary, body, icon);
	}

	return notification_data;
}

LIBNOTIFY_API gboolean notify_notification_update(
	NotifyNotification *notification, const gchar *summary,
	const gchar *body, const gchar *icon)
{
	gboolean ret = FALSE;

	EnterCriticalSection(&thread_guard);
	if(notification_thread && notification && summary && body && icon)
	{
		if(byte_to_wide_string(summary, notification->summary, MAX_SUMMARY_LENGTH))
		{
			if(byte_to_wide_string(body, notification->body, MAX_BODY_LENGTH))
			{
				notification->icon_req = 
					(0 == strcmp(icon, "gtk-dialog-warning")) ? ICON_WARNING : ICON_INFO;
				ret = TRUE;
			}
		}
	}
	LeaveCriticalSection(&thread_guard);

	return ret;
}

LIBNOTIFY_API gboolean notify_notification_show(
	NotifyNotification *notification, GError **error)
{
	UNREFERENCED_PARAMETER(error);

	if(notification_thread && notification)
		return PostThreadMessage(notification_thread_id,
								 WM_LIBNOTIFYSHOW,
								 0,
								 (LPARAM) notification);

	return FALSE;
}

LIBNOTIFY_API gboolean notify_notification_close(
	NotifyNotification *notification, GError **error)
{
	UNREFERENCED_PARAMETER(error);

	if(notification_thread && notification)
		return PostThreadMessage(notification_thread_id,
								 WM_LIBNOTIFYCLOSE,
								 0,
								 (LPARAM) notification);

	return FALSE;
}


/* helper functions */
DWORD notification_daemon_main(LPVOID lpdwThreadParam)
{
	HINSTANCE hDLLInstance = (HINSTANCE) GetModuleHandle(NULL);
	WNDCLASSEX wcex = {sizeof(WNDCLASSEX)};

	wchar_t szTitle[] = L"libnotify_notification";
	wchar_t szWindowClass[] = L"libnotify";
	HDC hdc = NULL;
	HGDIOBJ hOldFont = NULL;
	RECT rc = {0};
	MSG msg = {0};

	/* register window class */
	wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW;
	wcex.lpfnWndProc	= notificationWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hDLLInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH) GetStockObject(BLACK_BRUSH);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= NULL;

	if(0 == RegisterClassEx(&wcex)) return ((DWORD) -1);

	/* create the notification window */
	notify_wnd.notification_window = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT,
													szWindowClass,
													szTitle,
													WS_OVERLAPPED | WS_POPUP,
													CW_USEDEFAULT,
													0,
													CW_USEDEFAULT,
													0,
													NULL,
													NULL,
													hDLLInstance,
													NULL);

	if(!notify_wnd.notification_window) return ((DWORD) -1);

	/* height and width set here are dummy, they will later be reset based on DrawText */
	notify_wnd.notification_window_width = notification_window_width_max;
	notify_wnd.notification_window_height = notify_wnd.notification_window_width / 4;

	/*
	   setting wnd's alpha should be done before hiding the window (SetWindowPos); calling
	   it after hiding leads to high CPU usage due to continous WM_PAINT messages there by
	   wasting CPU cycles with transparent draw (BlendFunction) calls; this stops only after
	   the first time the window is hidden i.e. after a notify_notification_close is called
	   (which would be after a show request; refer Bug 1010930 & Question 184541
	*/
	if(!SetLayeredWindowAttributes(notify_wnd.notification_window, 0, 0, LWA_ALPHA))
		return ((DWORD) -1);
	if(!SetWindowPos(notify_wnd.notification_window,
					 HWND_TOPMOST,
					 0,
					 0,
					 notification_window_width_max,
					 notification_window_width_max / 4,
					 SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOREPOSITION))
		return ((DWORD) -1);

	hdc = GetDC(notify_wnd.notification_window);
	if(hdc)
	{
		hOldFont = SelectObject(hdc, (HGDIOBJ) font_summary);
		if(hOldFont)
		{
			/* set the width and get the height for a single line (summary) from DrawText;
			   for rational on width calc., see the above geometry */
			rc.right = notify_wnd.notification_window_width - (icon_size + (icon_padding * 3));

			DrawText(hdc,
					 L"placeholder",
					 -1,
					 &rc,
					 DT_CALCRECT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
			notify_wnd.summary_body_divider = (uint16_t) rc.bottom;

			SelectObject(hdc, hOldFont);
		}

		ReleaseDC(notify_wnd.notification_window, hdc);
		if(!hOldFont) return ((DWORD) -1);
	}
	else
		return ((DWORD) -1);

	// signal that the library initialization was successful before entering in to the message loop
	SetEvent((HANDLE) lpdwThreadParam);

	BOOL bRet = FALSE;
	while((bRet = GetMessage(&msg, NULL, 0, 0)))
	{
		if(-1 != bRet)
		{
			// these are sent as thread messages and not window messages, hence route it to the 
			// window proc., passing the right HWND, since msg's HWND will be NULL
			if((msg.message >= WM_LIBNOTIFYSHOW) && (msg.message <= WM_LIBNOTIFYEXIT))
			{
				notificationWndProc(notify_wnd.notification_window, msg.message, msg.wParam, msg.lParam);
			}
			else
			{
				DispatchMessage(&msg);
			}
		}
		else
		{
			return GetLastError();
		}
	}

	return msg.wParam;
}

LRESULT CALLBACK notificationWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	RECT rc = {0};
	UINT notify_duration = 0;
	static BOOL is_fading_out = FALSE;
	static HHOOK hook_mouse_over = NULL;
	static NotifyNotification notification_data_copy = {L"", L"", ICON_INFO};

	switch (message)
	{
	case WM_LIBNOTIFYSHOW:
		/* close if already running */
		if(IsWindowVisible(hWnd))
		{
			SendMessage(hWnd, WM_LIBNOTIFYCLOSE, 0, 0);
		}
		/* guarded by CS to make sure notification_data doesn't get corrupted
		   when this code and notify_notification_update is running in parallel */
		{
			EnterCriticalSection(&thread_guard);
			NotifyNotification *notification_data = (NotifyNotification*) lParam;
			if(notification_data &&
			   notification_data->body &&
			   notification_data->summary)
			{
				notification_data_copy = *notification_data;
			}
			else
			{
				LeaveCriticalSection(&thread_guard);
				break;
			}
			LeaveCriticalSection(&thread_guard);
		}

		/* deduce the allowed text width from the max width; see geometry for rationale */
		rc.right = notification_window_width_max - (icon_size + (icon_padding * 3));

		hdc = GetDC(hWnd);
		if(hdc)
		{
			HRGN hRgn = NULL;
			HGDIOBJ hOldFont = SelectObject(hdc, (HGDIOBJ) font_body);
			if(hOldFont)
			{
				DrawText(hdc,
						 notification_data_copy.body,
						 -1,
						 &rc,
						 DT_CALCRECT | DT_WORDBREAK |
						 DT_EDITCONTROL | DT_NOCLIP |
						 DT_NOPREFIX | DT_EXTERNALLEADING);

				SelectObject(hdc, hOldFont);
			}

			ReleaseDC(hWnd, hdc);
			if(!hOldFont) return 0;	/* exit if font selection failed */

			/* calculate the actual bounding rectangle from the DrawText output */
			notify_wnd.notification_window_height = notify_wnd.summary_body_divider +
													rc.bottom +
													(icon_padding * 3);
			notify_wnd.notification_window_width = rc.right + icon_size + (icon_padding * 3);

			/* word count * milliseconds per word */
			notify_duration = word_count(notification_data_copy.body) * milliseconds_per_word;

			/* in case the calculation renders too low a value, replace it with a de facto minimum */
			notify_duration = MAX(notify_duration, min_notification_timeout);

			/* get the screen area uncluttered by the taskbar */
			if(SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0))
			{
				LONG window_x = 0, window_y = 0;

				/* system tray @ right bottom */
				if((rc.bottom != GetSystemMetrics(SM_CYSCREEN)) ||
					(rc.right != GetSystemMetrics(SM_CXSCREEN)))
				{
					window_x = rc.right -
							   (GetSystemMetrics(SM_CXSCREEN) / window_offset_factor) -
							   notify_wnd.notification_window_width;
					window_y = rc.bottom -
							   (GetSystemMetrics(SM_CYSCREEN) / window_offset_factor) -
							   notify_wnd.notification_window_height;
				}
				else if(rc.left != 0)	/* left bottom */
				{
					window_x = rc.left +
							   (GetSystemMetrics(SM_CXSCREEN) / window_offset_factor);
					window_y = rc.bottom -
							   (GetSystemMetrics(SM_CYSCREEN) / window_offset_factor) -
							   notify_wnd.notification_window_height;
				}
				else					/* right top */
				{
					window_x = rc.right -
							   (GetSystemMetrics(SM_CXSCREEN) / window_offset_factor) -
							   notify_wnd.notification_window_width;
					window_y = rc.top +
							   (GetSystemMetrics(SM_CYSCREEN) / window_offset_factor);
				}

				/* resize and reposition the window */
				MoveWindow(hWnd,
						   window_x,
						   window_y,
						   notify_wnd.notification_window_width,
						   notify_wnd.notification_window_height,
						   TRUE);

				/* set the new positions to be used by the mouse over hook */
				notify_wnd.notification_window_rect.left = window_x;
				notify_wnd.notification_window_rect.top = window_y;
				notify_wnd.notification_window_rect.right = window_x + notify_wnd.notification_window_width;
				notify_wnd.notification_window_rect.bottom = window_y + notify_wnd.notification_window_height;

				/* make it as a rounded rect. */
				hRgn = CreateRoundRectRgn(0,
										  0,
										  notify_wnd.notification_window_width,
										  notify_wnd.notification_window_height,
										  rounded_rect_edge,
										  rounded_rect_edge);
				SetWindowRgn(hWnd, hRgn, TRUE);

				/* since bRedraw is set to TRUE in SetWindowRgn invalidation isn't required */
				/*InvalidateRect(hWnd, NULL, TRUE);*/

				/* show the window and set the timers for animation and overall visibility */
				ShowWindow(hWnd, SW_SHOWNOACTIVATE);

				SetTimer(hWnd, TIMER_ANIMATION, fade_duration, NULL);
				SetTimer(hWnd, TIMER_NOTIFICATION, notify_duration, NULL);
			}
		}
		break;
	case WM_LIBNOTIFYCLOSE:
		/* clean up and reset flags */
		{
			if(hook_mouse_over)
			{
				UnhookWindowsHookEx(hook_mouse_over);
				hook_mouse_over = NULL;
			}

			KillTimer(hWnd, TIMER_ANIMATION);
			KillTimer(hWnd, TIMER_NOTIFICATION);
			is_fading_out = FALSE;

			ShowWindow(hWnd, SW_HIDE);
		}
		break;
	case WM_PAINT:
		if((L'\0' != notification_data_copy.body[0]) &&
		   (L'\0' != notification_data_copy.summary[0]))
		{
			hdc = BeginPaint(hWnd, &ps);

			SetTextColor(hdc, RGB(255, 255, 255));
			SetBkMode(hdc, TRANSPARENT);

			HGDIOBJ hOldFont = SelectObject(hdc, (HGDIOBJ) font_summary);

			if(hOldFont)
			{
				/* set the padding as left offset and center the icon horizontally */
				DrawIcon(hdc,
						 icon_padding,
						 (notify_wnd.notification_window_height / 2) - (icon_size / 2),
						 notification_icons[notification_data_copy.icon_req]);

				/* calculate and DrawText for both summary and body
				   based on the geometry given above */
				rc.left = icon_size + (icon_padding * 2);
				rc.right = notify_wnd.notification_window_width - icon_padding;
				rc.top = icon_padding;
				rc.bottom = notify_wnd.summary_body_divider + (icon_padding * 2);

				DrawText(hdc,
						 notification_data_copy.summary,
						 -1,
						 &rc,
						 DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);

				if(SelectObject(hdc, (HGDIOBJ) font_body))
				{
					rc.top = rc.bottom;
					rc.bottom = notify_wnd.notification_window_height - icon_padding;

					DrawText(hdc,
							 notification_data_copy.body,
							 -1,
							 &rc,
							 DT_WORDBREAK | DT_EDITCONTROL |
							 DT_NOCLIP | DT_NOPREFIX |
							 DT_EXTERNALLEADING);
				}

				SelectObject(hdc, hOldFont);
			}

			EndPaint(hWnd, &ps);
		}
		break;
	case WM_LIBNOTIFYEXIT:
		if(hook_mouse_over)
		{
			UnhookWindowsHookEx(hook_mouse_over);
			hook_mouse_over = NULL;
		}

		KillTimer(notify_wnd.notification_window, TIMER_ANIMATION);
		KillTimer(notify_wnd.notification_window, TIMER_NOTIFICATION);
		PostQuitMessage(0);
		break;
	case WM_TIMER:
		if(IsWindowVisible(hWnd))
		{
			if(wParam == TIMER_ANIMATION)	/* notification animation timer */
			{
				if(is_fading_out)
				{
					if(notify_wnd.notification_window_alpha > 5)
					{
						notify_wnd.notification_window_alpha -= 25;
					}
					else
					{
						/* once fully faded out, self destroy and reset the flags */
						KillTimer(hWnd, TIMER_ANIMATION);
						is_fading_out = FALSE;
						notify_wnd.notification_window_alpha = 0;
						SendMessage(hWnd, WM_LIBNOTIFYCLOSE, 0, 0);
					}
				}
				else
				{
					if(notify_wnd.notification_window_alpha < 250)
					{
						notify_wnd.notification_window_alpha += 25;
					}
					else
					{
						/* self destory as alpha reaches the maximum */
						KillTimer(hWnd, TIMER_ANIMATION);
						notify_wnd.notification_window_alpha = 255;

						/* set the mouse over hook once the window is fully visible */
						hook_mouse_over = SetWindowsHookEx(WH_MOUSE_LL,
														   mouse_over_hook_proc,
														   (HINSTANCE) GetModuleHandle(NULL),
														   0);
					}
				}
				/* for all the above cases set the newly calculated alpha */
				SetLayeredWindowAttributes(notify_wnd.notification_window,
										   0,
										   notify_wnd.notification_window_alpha,
										   LWA_ALPHA);
			}
			else	/* notification duration timer */
			{
				/* self destruct once timed out */
				KillTimer(hWnd, TIMER_NOTIFICATION);

				/* kill the hook set by animation timer */
				if(hook_mouse_over)
				{
					UnhookWindowsHookEx(hook_mouse_over);
					hook_mouse_over = NULL;
				}

				/* start fading out sequence */
				is_fading_out = TRUE;
				SetTimer(hWnd, TIMER_ANIMATION, fade_duration, NULL);
			}
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

LRESULT CALLBACK mouse_over_hook_proc(int nCode, WPARAM wParam, LPARAM lParam)
{
	/*
	   don't use the direct mouse position in MSLLHOOKSTRUCT, it'll be in absolute coords;
	   while cursor pos will be relative (based on any DPI change) coords. E.g. on a
	   1680 x 1050 monitor, where the DPI is set to 150% larger, the mouse pointer pos.
	   will still be in 1680 x 1050 coordinates, while all the window coordinates will based
	   on virtual screen coords I.e. SM_CXSCREEN x SM_CYSCREEN will be 1120 x 700
     */
	POINT pt = {0};
	GetCursorPos(&pt);

	if(pt.x >= notify_wnd.notification_window_rect.left && pt.x <= notify_wnd.notification_window_rect.right &&
		pt.y >= notify_wnd.notification_window_rect.top && pt.y <= notify_wnd.notification_window_rect.bottom)
	{
		SetLayeredWindowAttributes(notify_wnd.notification_window, 0, mouse_over_alpha, LWA_ALPHA);
	}
	else
		SetLayeredWindowAttributes(notify_wnd.notification_window, 0, notify_wnd.notification_window_alpha, LWA_ALPHA);

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

uint16_t word_count(const wchar_t *str)
{
	uint16_t count = 0;

	while(*str)
	{
		if(' ' == *str || '-' == *str)
		{
			++count;
			/* skip further word delimiters chars */
			while(*++str && (' ' == *str || '-' == *str));
		}
		else
			++str;
	}

	return count;
}

static gboolean byte_to_wide_string(const gchar *byte_string, wchar_t *wide_string, gint max_buffer)
{
	static const wchar_t ellipses[] = L"...";
	const guint16 conversion_limit = max_buffer - ARRAYSIZE(ellipses);
	gint conv_len = ((gint)strlen(byte_string) >= max_buffer) ? conversion_limit : -1;
	if(0 != MultiByteToWideChar(CP_ACP, 0, byte_string, conv_len, wide_string, max_buffer))
	{
	    if(-1 != conv_len)
		{
			guint16 i = conversion_limit - 1;
			for(; wide_string[i] != L' '; --i);
			wcscpy_s(&wide_string[++i], max_buffer - conversion_limit, ellipses);
		}
		return TRUE;
	}
	return FALSE;
}
