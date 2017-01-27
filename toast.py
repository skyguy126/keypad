from __future__ import (absolute_import, print_function, unicode_literals)

import logging
from os import path
from time import sleep

from win32api import GetModuleHandle, PostQuitMessage
from win32con import CW_USEDEFAULT, IMAGE_ICON, IDI_APPLICATION,\
                     LR_DEFAULTSIZE, LR_LOADFROMFILE,\
                     WM_DESTROY, WS_OVERLAPPED, WS_SYSMENU, WM_USER
from win32gui import CreateWindow, DestroyWindow, LoadIcon, LoadImage,\
                     NIF_ICON, NIF_INFO, NIF_MESSAGE, NIF_TIP,\
                     NIM_ADD, NIM_DELETE, NIM_MODIFY,\
                     RegisterClass, Shell_NotifyIcon, UpdateWindow, WNDCLASS


# Provided by: https://github.com/jithurjacob/Windows-10-Toast-Notification/

class WindowsBalloonTip:
    def __init__(self):
        """Initialize."""
        message_map = {WM_DESTROY: self.on_destroy, }

        # Register the window class.
        wc = WNDCLASS()
        self.hinst = wc.hInstance = GetModuleHandle(None)
        wc.lpszClassName = str("PythonTaskbar")  # must be a string
        wc.lpfnWndProc = message_map  # could also specify a wndproc.
        self.classAtom = RegisterClass(wc)

    def balloon_tip(self, title="Notification", msg="...", duration=5):

        style = WS_OVERLAPPED | WS_SYSMENU
        self.hwnd = CreateWindow(self.classAtom, "Taskbar", style, 0, 0, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, self.hinst, None)
        UpdateWindow(self.hwnd)

        # Taskbar icon
        flags = NIF_ICON | NIF_MESSAGE | NIF_TIP
        nid = (self.hwnd, 0, flags, WM_USER + 20, None, "Tooltip")
        Shell_NotifyIcon(NIM_ADD, nid)
        Shell_NotifyIcon(NIM_MODIFY, (self.hwnd, 0, NIF_INFO, WM_USER + 20, None, "Balloon Tooltip", msg, 200, title))

        # wait then destroy
        sleep(duration)
        DestroyWindow(self.hwnd)
        return None

    def on_destroy(self, hwnd, msg, wparam, lparam):
        nid = (self.hwnd, 0)
        Shell_NotifyIcon(NIM_DELETE, nid)
        PostQuitMessage(0)
        return None

if __name__ == "__main__":
    w = WindowsBalloonTip()
    w.balloon_tip("Keypad", "Connected", duration=5)
