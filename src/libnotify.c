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
| |@@      @@| |-----------------------------| |
| |@  ICON  @| |-----------------------------| |
| |@@      @@| |Body                         | |
| |@@@@@@@@@@| |                             | |
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


/* user defined messages */
#define WM_LIBNOTIFYSHOW	(WM_USER + 98)
#define WM_LIBNOTIFYCLOSE	(WM_USER + 99)
#define WM_LIBNOTIFYEXIT	(WM_USER + 100)

/* timers used & their IDs */
enum TIMER_IDS
{
	TIMER_ANIMATION = 1,
	TIMER_NOTIFICATION
};


/* constants */
#define max_summary_length	100
#define max_body_length		300
/* lookup 'MAK' for setting 300 */

const uint8_t	rounded_rect_edge = 15;
const uint8_t	mouse_over_alpha = (0xFF / 4);
const uint8_t	fade_duration = 15;
const uint16_t	min_notification_timeout = 3000;
const uint16_t	window_offset_factor = 10;
const DWORD		thread_wait_timeout = 2000;
/* notification timout is calculated based on the word count */
const uint16_t	milliseconds_per_word = 750;


struct _NotifyNotification
{
	wchar_t summary[max_summary_length];
	wchar_t body[max_body_length];
	HICON icon;
};


/* globals */
HANDLE				notification_thread = NULL;
DWORD				notification_thread_id = 0;
HWND				notification_window = NULL;
uint8_t				notification_window_alpha = 0;
RECT				notification_window_rect = {0};
LONG				notification_window_width_max = 0;
LONG				notification_window_width = 0;
LONG				notification_window_height = 0;
NotifyNotification* notification_data = NULL;
BOOL				is_fading_out = FALSE;
HHOOK				hook_mouse_over = NULL;
HFONT				font_summary = NULL;
HFONT				font_body = NULL;
uint16_t			icon_size = 0;
uint16_t			icon_padding = 0;
uint16_t			summary_body_divider = 0;


/* function declarations */
DWORD notification_daemon_main(LPVOID lpdwThreadParam);
LRESULT CALLBACK thread_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK mouse_over_hook_proc(int nCode, WPARAM wParam, LPARAM lParam);
uint16_t word_count(const wchar_t *str);


/* dll exported functions */
LIBNOTIFY_API gboolean notify_init(const char *app_name)
{
	BOOL ret_val = FALSE;
	HANDLE event_wnd_created = CreateEvent(NULL, TRUE, FALSE, TEXT("NotifyWindowCreatedEvent"));
	DWORD thread_ret_code = 0;
	HANDLE wait_handles[2] = {NULL};

	UNREFERENCED_PARAMETER(app_name);
	
	if(event_wnd_created)
	{
		notification_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) &notification_daemon_main, event_wnd_created, 0, &notification_thread_id);

		if(notification_thread)
		{
			wait_handles[0] = event_wnd_created; wait_handles[1] = notification_thread;

			if((WAIT_OBJECT_0 + 2) > WaitForMultipleObjects(2, wait_handles, FALSE, thread_wait_timeout))
			{
				if(GetExitCodeThread(notification_thread, &thread_ret_code))
				{
					if(STILL_ACTIVE == thread_ret_code)
					{
						notification_data = (NotifyNotification*) malloc(sizeof(NotifyNotification));
						if(notification_data)
							ret_val = TRUE;
					}
				}
			}

			if(!notification_data)
			{
				/* terminate and free the thread */
				TerminateThread(notification_thread, (DWORD) -1);
				CloseHandle(notification_thread);
			}
		}

		CloseHandle(event_wnd_created);
	}

	return ret_val;
}

LIBNOTIFY_API void notify_uninit(void)
{
	if(notification_data)
	{
		PostThreadMessage(notification_thread_id, WM_LIBNOTIFYEXIT, 0, 0);

		free(notification_data);
		notification_data = NULL;

		/* if the wait fails, preempt the thread */
		if(WAIT_OBJECT_0 != WaitForSingleObject(notification_thread, thread_wait_timeout))
			TerminateThread(notification_thread, (DWORD) -1);

		CloseHandle(notification_thread);
	}
}

LIBNOTIFY_API NotifyNotification* notify_notification_new(
	const gchar *summary, const gchar *body,
	const gchar *icon)
{
	if(notification_data)
		notify_notification_update(notification_data, summary, body, icon);

	return notification_data;
}

LIBNOTIFY_API gboolean notify_notification_update(
	NotifyNotification *notification, const gchar *summary,
	const gchar *body, const gchar *icon)
{
	if(notification)
		if(summary && (0 != MultiByteToWideChar(CP_ACP, 0, summary, -1, notification_data->summary, max_summary_length)))
			if(body && (0 != MultiByteToWideChar(CP_ACP, 0, body, -1, notification_data->body, max_body_length)))
				if (NULL != (notification_data->icon = LoadIcon(NULL, (0 == strcmp(icon, "gtk-dialog-warning")) ? IDI_WARNING : IDI_INFORMATION)))
					return TRUE;

	return FALSE;
}

LIBNOTIFY_API gboolean notify_notification_show(
	NotifyNotification *notification, GError **error)
{
	UNREFERENCED_PARAMETER(error);

	if(notification && notification_window)
		return PostMessage(notification_window, WM_LIBNOTIFYSHOW, 0, 0);

	return FALSE;
}

LIBNOTIFY_API gboolean notify_notification_close(
	NotifyNotification *notification, GError **error)
{
	UNREFERENCED_PARAMETER(error);

	if(notification && notification_window)
		return PostMessage(notification_window, WM_LIBNOTIFYCLOSE, 0, 0);

	return FALSE;
}


/* helper functions */
DWORD notification_daemon_main(LPVOID lpdwThreadParam)
{
	HINSTANCE hDLLInstance = (HINSTANCE) GetModuleHandle(NULL);
	WNDCLASSEX wcex = {sizeof(WNDCLASSEX)};

	wchar_t szTitle[] = TEXT("libnotify_notification");
	wchar_t szWindowClass[] = TEXT("libnotify");
	HDC hdc = NULL;
	HFONT hOldFont = NULL;
	RECT rc = {0};
	MSG msg = {0};

	/* register window class */
	wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW;
	wcex.lpfnWndProc	= thread_proc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hDLLInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH) GetStockObject(BLACK_BRUSH);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= NULL;

	if(0 == RegisterClassEx(&wcex)) goto cleanup_and_exit;

	/* create the notification window */
	notification_window = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT,
		szWindowClass, szTitle, WS_OVERLAPPED | WS_POPUP,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hDLLInstance, NULL);

	if(!notification_window) goto cleanup_and_exit;

	/* screen width / 3.5 is the maximum allowed notification width */
	notification_window_width_max = (GetSystemMetrics(SM_CXSCREEN) * 2) / 7;
	icon_size = (uint16_t) GetSystemMetrics(SM_CXICON);
	icon_padding = icon_size / 3;		/* icon spacing is a third of the icon width */

	/* height and width set here are dummy, they will later be reset based on DrawText */
	notification_window_width = notification_window_width_max;
	notification_window_height = notification_window_width / 4;

	SetWindowPos(notification_window, HWND_TOPMOST, 0, 0, notification_window_width, notification_window_height, SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOREPOSITION);
	SetLayeredWindowAttributes(notification_window, 0, notification_window_alpha, LWA_ALPHA);
	
	font_summary = CreateFont(0, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_ROMAN, NULL);
	if(!font_summary) goto cleanup_and_exit;

	font_body = CreateFont(0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_ROMAN, NULL);
	if(!font_body) goto cleanup_and_exit;

	hdc = GetDC(notification_window);
	if(hdc)
	{
		hOldFont = (HFONT) SelectObject(hdc, (HFONT) font_summary);
		if(hOldFont)
		{
			/* set the width and get the height for a single line (summary) from DrawText;
			   for rational on width calc., see the above geometry */
			rc.right = notification_window_width - (icon_size + (icon_padding * 3));

			DrawText(hdc, TEXT("placeholder"), -1, &rc, DT_CALCRECT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
			summary_body_divider = (uint16_t) rc.bottom;

			SelectObject(hdc, (HFONT) hOldFont);
		}

		ReleaseDC(notification_window, hdc);
		if(!hOldFont) goto cleanup_and_exit;
	}
	else
		goto cleanup_and_exit;

	SetEvent((HANDLE) lpdwThreadParam);

	while(GetMessage(&msg, NULL, 0, 0))
	{
		if((msg.message == WM_LIBNOTIFYEXIT) || (msg.message == WM_QUIT))
		{
			if(hook_mouse_over)
			{
				UnhookWindowsHookEx(hook_mouse_over);
				hook_mouse_over = NULL;
			}

			KillTimer(notification_window, TIMER_ANIMATION);
			KillTimer(notification_window, TIMER_NOTIFICATION);

			if(font_summary)
				DeleteObject(font_summary);
			if(font_body)
				DeleteObject(font_body);

			break;
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return msg.wParam;

/* cleanup and return */
cleanup_and_exit:

	if(font_summary)
		DeleteObject(font_summary);
	if(font_body)
		DeleteObject(font_body);

	return ((DWORD) -1);
}

LRESULT CALLBACK thread_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	RECT rc = {0};
	UINT notify_duration = 0;
	HFONT hOldFont = NULL;

	switch (message)
	{
	case WM_LIBNOTIFYSHOW:
		if(notification_data)
		{
			/* deduce the allowed text width from the max width; see geometry for rationale */
			rc.right = notification_window_width_max - (icon_size + (icon_padding * 3));
			
			hdc = GetDC(hWnd);
			if(hdc)
			{
				HFONT hOldFont = NULL;
				HRGN hRgn = NULL;

				hOldFont = (HFONT) SelectObject(hdc, font_body);
				if(hOldFont)
				{
					DrawText(hdc, notification_data->body, -1, &rc, DT_CALCRECT | DT_WORDBREAK | DT_EDITCONTROL | DT_NOCLIP | DT_NOPREFIX | DT_EXTERNALLEADING);
					SelectObject(hdc, hOldFont);
				}

				ReleaseDC(hWnd, hdc);
				if(!hOldFont) return 0;	/* exit if font selection failed */

				/* calculate the actual bounding rectangle from the DrawText output */
				notification_window_height = summary_body_divider + rc.bottom + (icon_padding * 3);
				notification_window_width = rc.right + icon_size + (icon_padding * 3);

				/* word count * milliseconds per word */
				notify_duration = word_count(notification_data->body) * milliseconds_per_word;

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
						window_x = rc.right - (GetSystemMetrics(SM_CXSCREEN) / window_offset_factor) - notification_window_width;
						window_y = rc.bottom - (GetSystemMetrics(SM_CYSCREEN) / window_offset_factor) - notification_window_height;
					}
					else if(rc.left != 0)	/* left bottom */
					{
						window_x = rc.left + (GetSystemMetrics(SM_CXSCREEN) / window_offset_factor);
						window_y = rc.bottom - (GetSystemMetrics(SM_CYSCREEN) / window_offset_factor) - notification_window_height;
					}
					else					/* right top */
					{
						window_x = rc.right - (GetSystemMetrics(SM_CXSCREEN) / window_offset_factor) - notification_window_width;
						window_y = rc.top + (GetSystemMetrics(SM_CYSCREEN) / window_offset_factor);
					}

					/* resize and reposition the window */
					MoveWindow(hWnd, window_x, window_y, notification_window_width, notification_window_height, TRUE);

					/* set the new positions to be used by the mouse over hook */
					notification_window_rect.left = window_x;
					notification_window_rect.top = window_y;
					notification_window_rect.right = window_x + notification_window_width;
					notification_window_rect.bottom = window_y + notification_window_height;
					
					/* make it as a rounded rect. */
					hRgn = CreateRoundRectRgn(0, 0, notification_window_width, notification_window_height, rounded_rect_edge, rounded_rect_edge);
					SetWindowRgn(notification_window, hRgn, TRUE);

					/* since bRedraw is set to TRUE in SetWindowRgn invalidation isn't required */
					/*InvalidateRect(hWnd, NULL, TRUE);*/

					/* show the window and set the timers for animation and overall visibility */
					ShowWindow(hWnd, SW_SHOWNOACTIVATE);

					SetTimer(notification_window, TIMER_ANIMATION, fade_duration, NULL);
					SetTimer(notification_window, TIMER_NOTIFICATION, notify_duration, NULL);
				}
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
		if(notification_data && notification_data->summary && 
			notification_data->body && notification_data->icon)
		{
			hdc = BeginPaint(hWnd, &ps);

			SetTextColor(hdc, RGB(255, 255, 255));
			SetBkMode(hdc, TRANSPARENT);

			hOldFont = (HFONT) SelectObject(hdc, font_summary);

			if(hOldFont)
			{
				/* set the padding as left offset and center the icon horizontally */
				DrawIcon(hdc, icon_padding, (notification_window_height / 2) - (icon_size / 2), notification_data->icon);

				/* calculate and DrawText for both summary and body based on the geometry given above */
				rc.left = icon_size + (icon_padding * 2);
				rc.right = notification_window_width - icon_padding;
				rc.top = icon_padding;
				rc.bottom = summary_body_divider + (icon_padding * 2);

				DrawText(hdc, notification_data->summary, -1, &rc, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);

				if(SelectObject(hdc, font_body))
				{
					rc.top = rc.bottom;
					rc.bottom = notification_window_height - icon_padding;

					DrawText(hdc, notification_data->body, -1, &rc, DT_WORDBREAK | DT_EDITCONTROL | DT_NOCLIP | DT_NOPREFIX | DT_EXTERNALLEADING);
				}

				SelectObject(hdc, hOldFont);
			}

			EndPaint(hWnd, &ps);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_TIMER:
		if(IsWindowVisible(hWnd))
		{
			if(wParam == TIMER_ANIMATION)	/* notification animation timer */
			{
				if(is_fading_out)
				{
					if(notification_window_alpha > 5)
					{
						notification_window_alpha -= 25;
						SetLayeredWindowAttributes(notification_window, 0, notification_window_alpha, LWA_ALPHA);
					}
					else
					{
						/* once fully faded out, self destroy and reset the flags */
						KillTimer(hWnd, TIMER_ANIMATION);
						is_fading_out = FALSE;
						notification_window_alpha = 0;
						PostMessage(hWnd, WM_LIBNOTIFYCLOSE, 0, 0);
					}
				}
				else
				{
					if(notification_window_alpha < 250)
					{
						notification_window_alpha += 25;
						SetLayeredWindowAttributes(notification_window, 0, notification_window_alpha, LWA_ALPHA);
					}
					else
					{
						/* self destory as alpha reaches the maximum */
						KillTimer(hWnd, TIMER_ANIMATION);
						notification_window_alpha = 255;

						/* set the mouse over hook once the window is fully visible */
						hook_mouse_over = SetWindowsHookEx(WH_MOUSE_LL, mouse_over_hook_proc, (HINSTANCE) GetModuleHandle(NULL), 0);
					}
				}
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
				SetTimer(hWnd, 1, fade_duration, NULL);
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

	if(pt.x >= notification_window_rect.left && pt.x <= notification_window_rect.right && 
		pt.y >= notification_window_rect.top && pt.y <= notification_window_rect.bottom)
	{
		SetLayeredWindowAttributes(notification_window, 0, mouse_over_alpha, LWA_ALPHA);
	}
	else
		SetLayeredWindowAttributes(notification_window, 0, notification_window_alpha, LWA_ALPHA);

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
