#define _WIN32_WINNT 0x0600
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commdlg.h>
#include <stdio.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <initguid.h>
#include <propkey.h>
#include <shlobj.h>
#include <propsys.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#define MAX_TRACKS 100
#define MAX_STRING_LEN 64
#define MAX_TOOLTIP_LEN 256

// Function declarations
void StopMP3(HWND hwnd);

DEFINE_PROPERTYKEY(PKEY_Media_Title, 0x64440492, 0x4C8B, 0x11D1, 0x8B, 0x70, 0x08, 0x00, 0x36, 0xB1, 0x1A, 0x03, 13);

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "propsys.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")

enum {
    IDM_EXIT = 1001,
    IDM_TRAY,
    IDM_ABOUT,
    IDM_PLAYPAUSE,
    IDM_OPEN = 1007,
    IDM_REPLAY,
    IDM_NEXT,
    IDM_PREV,
    IDM_STOP,
    IDM_AUTOSTART = 1012,
    IDM_VOLUME = 1013,
    IDD_ABOUT = 101,
    IDC_WEBSITE = 108,
    IDC_COFFEE,
    ID_MENU_LANGUAGE = 201,
    ID_MENU_LANG_EN,
    ID_MENU_LANG_HU,
    ID_MENU_LANG_DE = 204,
    ID_MENU_LANG_IT = 205,
    ID_MENU_LANG_ES = 206,
    ID_MENU_LANG_FR = 207,
    ID_MENU_LANG_RU = 208,
    IDT_TIMER = 2001,
    IDC_VOLUME_SLIDER = 3000
};

static int autoStart = 0;
static int currentVolume = 500;
typedef struct {
    WCHAR menu_playpause[MAX_STRING_LEN];
    WCHAR menu_open[MAX_STRING_LEN];
    WCHAR menu_replay[MAX_STRING_LEN];
    WCHAR menu_about[MAX_STRING_LEN];
    WCHAR menu_exit[MAX_STRING_LEN];
    WCHAR menu_next[MAX_STRING_LEN];
    WCHAR menu_prev[MAX_STRING_LEN];
    WCHAR menu_stop[MAX_STRING_LEN];
    WCHAR menu_language[MAX_STRING_LEN];
    WCHAR menu_volume[MAX_STRING_LEN];
    WCHAR tooltip_stopped[MAX_STRING_LEN];
    WCHAR about_website[MAX_STRING_LEN];
    WCHAR about_coffee[MAX_STRING_LEN];
} LANG;

static LANG lang_en = {
    L"Play / Pause",
    L"Open",
    L"Replay",
    L"About",
    L"Exit",
    L"Next",
    L"Previous",
    L"Stop",
    L"Language",
    L"Volume",
    L"Stopped",
    L"Visit our website",
    L"Support us with a coffee"
};

static LANG lang_hu = {
    L"Lejátszás / Szüneteltetés",
    L"Megnyitás",
    L"Ismétlés",
    L"Névjegy",
    L"Kilépés",
    L"Következő",
    L"Előző",
    L"Leállítás",
    L"Nyelv",
    L"Hangerő",
    L"Leállítva",
    L"Weboldal",
    L"Támogass egy kávéval"
};

static LANG lang_de = {
    L"Wiedergabe / Pause",
    L"Öffnen",
    L"Wiederholen",
    L"Über",
    L"Beenden",
    L"Nächster",
    L"Vorheriger",
    L"Stopp",
    L"Sprache",
    L"Lautstärke",
    L"Gestoppt",
    L"Besuchen Sie unsere Webseite",
    L"Unterstützen Sie uns mit einem Kaffee"
};

static LANG lang_it = {
    L"Riproduci / Pausa",
    L"Apri",
    L"Ripeti",
    L"Informazioni",
    L"Esci",
    L"Successivo",
    L"Precedente",
    L"Arresta",
    L"Lingua",
    L"Volume",
    L"Fermato",
    L"Visita il nostro sito web",
    L"Supportaci con un caffè"
};

static LANG lang_es = {
    L"Reproducir / Pausa",
    L"Abrir",
    L"Repetir",
    L"Acerca de",
    L"Salir",
    L"Siguiente",
    L"Anterior",
    L"Detener",
    L"Idioma",
    L"Volumen",
    L"Detenido",
    L"Visita nuestro sitio web",
    L"Apóyanos con un café"
};

static LANG lang_fr = {
    L"Lecture / Pause",
    L"Ouvrir",
    L"Répéter",
    L"À propos",
    L"Quitter",
    L"Suivant",
    L"Précédent",
    L"Arrêter",
    L"Langue",
    L"Volume",
    L"Arrêté",
    L"Visitez notre site web",
    L"Soutenez-nous avec un café"
};

static LANG lang_ru = {
    L"Воспроизвести / Пауза",
    L"Открыть",
    L"Повтор",
    L"О программе",
    L"Выход",
    L"Следующий",
    L"Предыдущий",
    L"Остановить",
    L"Язык",
    L"Громкость",
    L"Остановлено",
    L"Посетите наш сайт",
    L"Поддержите нас кофе"
};

static LANG *g_lang = &lang_en;
static HMENU g_hMenu = NULL;
static HMENU g_hLangMenu = NULL;

NOTIFYICONDATA nid;
HICON hPlayIcon, hPauseIcon;
HRESULT hr = 0;
int isPlaying = 0;
int isReplay = 0;
char mp3File[MAX_PATH] = "";
WCHAR szClassName[] = L"MP3PlayerWndClass";
WCHAR szVolumeClassName[] = L"VolumePopup";
DWORD pausedPosition = 0;

// Playlist variables
char playlist[MAX_PATH] = "";
char playlistFiles[MAX_TRACKS][MAX_PATH];
int currentTrack = 0;
int trackCount = 0;
// Volume control variables
static HWND g_hVolumeWnd = NULL;
static HWND g_hVolumeSlider = NULL;
void SetLanguage(LANG *newLang);
void SaveLanguageSelectionToRegistry(void);
BOOL LoadLanguageSelectionFromRegistry(void);
void SaveReplayStateToRegistry(void);
BOOL LoadReplayStateFromRegistry(void);
void SaveVolumeToRegistry(void);
BOOL LoadVolumeFromRegistry(void);
void RefreshMenuText(void);
INT_PTR CALLBACK AboutDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void ShowAboutDialog(HWND hwndParent);
BOOL ParseM3UFile(const WCHAR* m3uPath);
void SelectMP3File(HWND hwnd);
void PlayMP3(HWND hwnd);
void PauseMP3(HWND hwnd);
void LoadNextTrack(HWND hwnd);
void LoadPrevTrack(HWND hwnd);
void UpdateTooltip(HWND hwnd);
void ToggleReplay(HWND hwnd);
HICON CreateIconFromText(const WCHAR* pid, COLORREF color);
WCHAR* GetMP3SongTitle(const WCHAR* path);
HRESULT GetMP3Duration(const WCHAR* pPath, DWORD* durationMs);
void SaveAutoStartStateToRegistry(void);
BOOL LoadAutoStartStateFromRegistry(void);
void ShowVolumeControl(HWND hwndOwner);
void UpdateVolumeFromSlider(void);
LRESULT CALLBACK VolumeWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
HICON CreateIconFromText(const WCHAR* pid, COLORREF color) {
    HDC hdc = CreateCompatibleDC(NULL);
    BITMAPINFO bi = {0};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = 32;
    bi.bmiHeader.biHeight = -32;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void* p;
    HBITMAP hBitmap = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, &p, NULL, 0);
    SelectObject(hdc, hBitmap);

    HBRUSH h = CreateSolidBrush(color);
    RECT rect = {0, 0, 32, 32};
    FillRect(hdc, &rect, h);
    DeleteObject(h);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    HFONT hFont = CreateFontW(27, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                             DEFAULT_PITCH | FF_SWISS, L"Arial");
    SelectObject(hdc, hFont);

    SIZE textSize;
    GetTextExtentPoint32W(hdc, pid, wcslen(pid), &textSize);
    int textX = (32 - textSize.cx) / 2;
    int textY = (32 - textSize.cy) / 2 - 2;

    if (wcscmp(pid, L"\u25BA") == 0) {
        textX += 2;
    }

    TextOutW(hdc, textX, textY, pid, wcslen(pid));

    HBITMAP hMask = CreateCompatibleBitmap(hdc, 32, 32);
    HDC hdcMask = CreateCompatibleDC(hdc);
    SelectObject(hdcMask, hMask);
    SetBkColor(hdc, color);
    BitBlt(hdcMask, 0, 0, 32, 32, hdc, 0, 0, SRCCOPY);

    ICONINFO ii = {0};
    ii.fIcon = TRUE;
    ii.hbmColor = hBitmap;
    ii.hbmMask = hMask;
    HICON hIcon = CreateIconIndirect(&ii);

    DeleteObject(hFont);
    DeleteObject(hBitmap);
    DeleteObject(hMask);
    DeleteDC(hdcMask);
    DeleteDC(hdc);

    return hIcon;
}

void SetTrayIcon(HWND hwnd, HICON hIcon, const WCHAR* tip) {
    nid.hIcon = hIcon;
    wcsncpy(nid.szTip, tip, sizeof(nid.szTip) / sizeof(nid.szTip[0]));
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void CleanupMCI() {
    StopMP3(GetForegroundWindow());
}

WCHAR* GetMP3SongTitle(const WCHAR* path) {
    IShellItem* psi = NULL;
    hr = SHCreateItemFromParsingName(path, NULL, &IID_IShellItem, (void**)&psi);
    if (SUCCEEDED(hr)) {
        IPropertyStore* pps = NULL;
        hr = psi->lpVtbl->QueryInterface(psi, &IID_IPropertyStore, (void**)&pps);
        if (SUCCEEDED(hr)) {
            PROPVARIANT var;
            PropVariantInit(&var);
            hr = pps->lpVtbl->GetValue(pps, &PKEY_Media_Title, &var);
            if (SUCCEEDED(hr) && var.vt == VT_LPWSTR && var.pwszVal && wcslen(var.pwszVal) > 0) {
                WCHAR* title = (WCHAR*)CoTaskMemAlloc((wcslen(var.pwszVal) + 1) * sizeof(WCHAR));
                if (title) {
                    wcscpy(title, var.pwszVal);
                    PropVariantClear(&var);
                    pps->lpVtbl->Release(pps);
                    psi->lpVtbl->Release(psi);
                    return title;
                }
            }
            PropVariantClear(&var);
            pps->lpVtbl->Release(pps);
        }
        psi->lpVtbl->Release(psi);
    }

    return NULL;
}

HRESULT GetMP3Duration(const WCHAR* pPath, DWORD* durationMs) {
    IShellItem* psi = NULL;
    hr = SHCreateItemFromParsingName(pPath, NULL, &IID_IShellItem, (void**)&psi);
    if (SUCCEEDED(hr)) {
        IPropertyStore* pps = NULL;
        hr = psi->lpVtbl->QueryInterface(psi, &IID_IPropertyStore, (void**)&pps);
        if (SUCCEEDED(hr)) {
            PROPVARIANT var;
            PropVariantInit(&var);
            hr = pps->lpVtbl->GetValue(pps, &PKEY_Media_Duration, &var);
            if (SUCCEEDED(hr) && var.vt == VT_UI8 && var.uhVal.QuadPart > 0) {
                *durationMs = (DWORD)(var.uhVal.QuadPart / 10000);
                PropVariantClear(&var);
                pps->lpVtbl->Release(pps);
                psi->lpVtbl->Release(psi);
                return S_OK;
            }
            PropVariantClear(&var);
            pps->lpVtbl->Release(pps);
        }
        psi->lpVtbl->Release(psi);
    }

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    if (FAILED(hr)) {
        return hr;
    }

    IMFSourceReader* pReader = NULL;
    hr = MFCreateSourceReaderFromURL(pPath, NULL, &pReader);
    if (SUCCEEDED(hr)) {
        PROPVARIANT var;
        PropVariantInit(&var);
        hr = pReader->lpVtbl->GetPresentationAttribute(pReader, MF_SOURCE_READER_MEDIASOURCE, &MF_PD_DURATION, &var);
        if (SUCCEEDED(hr) && var.vt == VT_UI8) {
            *durationMs = (DWORD)(var.uhVal.QuadPart / 10000);
            PropVariantClear(&var);
            pReader->lpVtbl->Release(pReader);
            MFShutdown();
            return S_OK;
        }
        PropVariantClear(&var);
        pReader->lpVtbl->Release(pReader);
    }

    MFShutdown();
    return hr;
}

void UpdateTooltip(HWND hwnd) {
    if (!isPlaying || strlen(mp3File) == 0) {
        SetTrayIcon(hwnd, hPlayIcon, g_lang->tooltip_stopped);
        return;
    }

    WCHAR tooltip[MAX_TOOLTIP_LEN] = {0};
    WCHAR wMp3File[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, mp3File, -1, wMp3File, MAX_PATH);
    WCHAR* songTitle = GetMP3SongTitle(wMp3File);
    if (!songTitle) {
        songTitle = (WCHAR*)PathFindFileNameW(wMp3File);
    }

    WCHAR status[32];
    DWORD currentPos = 0;
    if (mciSendString(L"status mp3 position", status, sizeof(status)/sizeof(status[0]), NULL) == 0) {
        currentPos = wcstoul(status, NULL, 10);
    }

    DWORD duration = 0;
    if (SUCCEEDED(GetMP3Duration(wMp3File, &duration))) {
        if (currentPos > duration) currentPos = duration;
        int remaining = (duration - currentPos) / 1000;
        int min = remaining / 60;
        int sec = remaining % 60;

        if (trackCount > 1) {
            _snwprintf(tooltip, sizeof(tooltip)/sizeof(tooltip[0]),
                      L"%s (%d/%d) (%d:%02d)", songTitle, currentTrack + 1, trackCount, min, sec);
        } else {
            _snwprintf(tooltip, sizeof(tooltip)/sizeof(tooltip[0]),
                      L"%s (%d:%02d)", songTitle, min, sec);
        }
    } else {
        DWORD mciDuration = 0;
        if (mciSendString(L"status mp3 length", status, sizeof(status)/sizeof(status[0]), NULL) == 0) {
            mciDuration = wcstoul(status, NULL, 10);
            int remaining = (mciDuration - currentPos) / 1000;
            int min = remaining / 60;
            int sec = remaining % 60;

            if (trackCount > 1) {
                _snwprintf(tooltip, sizeof(tooltip)/sizeof(tooltip[0]),
                          L"%s (%d/%d) (%d:%02d)", songTitle, currentTrack + 1, trackCount, min, sec);
            } else {
                _snwprintf(tooltip, sizeof(tooltip)/sizeof(tooltip[0]),
                          L"%s (%d:%02d)", songTitle, min, sec);
            }
        } else {
            if (trackCount > 1) {
                _snwprintf(tooltip, sizeof(tooltip)/sizeof(tooltip[0]),
                          L"%s (%d/%d)", songTitle, currentTrack + 1, trackCount);
            } else {
                wcscpy(tooltip, songTitle);
            }
        }
    }

    SetTrayIcon(hwnd, isPlaying ? hPauseIcon : hPlayIcon, tooltip);
    if (songTitle != PathFindFileNameW(wMp3File)) {
        CoTaskMemFree(songTitle);
    }
}

void SelectMP3File(HWND hwnd) {
    OPENFILENAMEW ofn = {0};
    WCHAR szFile[MAX_PATH * 100] = {0};
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(WCHAR);
    ofn.lpstrFilter = L"Media Files (*.mp3;*.m3u)\0*.mp3;*.m3u\0MP3 Files (*.mp3)\0*.mp3\0Playlist Files (*.m3u)\0*.m3u\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

    if (GetOpenFileNameW(&ofn)) {
        mciSendString(L"close mp3", NULL, 0, NULL);
        isPlaying = 0;
        pausedPosition = 0;
        KillTimer(hwnd, IDT_TIMER);

        trackCount = 0;
        currentTrack = 0;
        mp3File[0] = '\0';
        playlist[0] = '\0';

        WCHAR* p = szFile;
        WCHAR* dir = p;
        p += wcslen(p) + 1;

        if (*p == L'\0') {
            // Single file selected
            if (wcsstr(szFile, L".m3u") || wcsstr(szFile, L".M3U")) {
                // M3U playlist
                if (ParseM3UFile(szFile)) {
                    WideCharToMultiByte(CP_UTF8, 0, szFile, -1, playlist, MAX_PATH, NULL, NULL);
                    strncpy(mp3File, playlistFiles[currentTrack], MAX_PATH);
                }
            } else {
                // Single MP3 file
                WideCharToMultiByte(CP_UTF8, 0, szFile, -1, playlistFiles[trackCount], MAX_PATH, NULL, NULL);
                trackCount++;
                WideCharToMultiByte(CP_UTF8, 0, szFile, -1, mp3File, MAX_PATH, NULL, NULL);
            }
        } else {
            // Multiple files selected
            while (*p && trackCount < MAX_TRACKS) {
                WCHAR fullPath[MAX_PATH];
                _snwprintf(fullPath, MAX_PATH, L"%s\\%s", dir, p);

                if (wcsstr(fullPath, L".m3u") || wcsstr(fullPath, L".M3U")) {
                    // M3U playlist
                    if (ParseM3UFile(fullPath)) {
                        strncpy(mp3File, playlistFiles[currentTrack], MAX_PATH);
                        break;
                    }
                } else {
                    // MP3 file(s)
                    WideCharToMultiByte(CP_UTF8, 0, fullPath, -1, playlistFiles[trackCount], MAX_PATH, NULL, NULL);
                    trackCount++;
                }
                p += wcslen(p) + 1;
            }

            if (trackCount > 0 && mp3File[0] == '\0') {
                strncpy(mp3File, playlistFiles[currentTrack], MAX_PATH);
            }
        }

        if (mp3File[0] != '\0') {
            PlayMP3(hwnd);
        } else {
            SetTrayIcon(hwnd, hPlayIcon, g_lang->tooltip_stopped);
        }
    }
}

void PlayMP3(HWND hwnd) {
    if (strlen(mp3File) == 0) {
        SelectMP3File(hwnd);
        return;
    }

    if (!isPlaying) {
        WCHAR wCommand[512];
        WCHAR wMp3File[MAX_PATH];
        MultiByteToWideChar(CP_UTF8, 0, mp3File, -1, wMp3File, MAX_PATH);

        if (pausedPosition == 0) {
            // Initial play or new track
            mciSendString(L"close mp3", NULL, 0, NULL);
            _snwprintf(wCommand, sizeof(wCommand) / sizeof(wCommand[0]), L"open \"%s\" type mpegvideo alias mp3", wMp3File);
            if (mciSendString(wCommand, NULL, 0, NULL) == 0) {
                mciSendString(L"set mp3 time format ms", NULL, 0, NULL);
                if (isReplay) {
                    mciSendString(L"set mp3 repeat", NULL, 0, NULL);
                }
                WCHAR cmd[64];
                _snwprintf(cmd, 64, L"setaudio mp3 volume to %d", currentVolume);
                mciSendString(cmd, NULL, 0, NULL);
                mciSendString(L"play mp3 notify", NULL, 0, hwnd);
                isPlaying = 1;
                UpdateTooltip(hwnd);
                SetTimer(hwnd, IDT_TIMER, 1000, NULL);
                RefreshMenuText();
            }
        } else {
            // Resume from paused position
            _snwprintf(wCommand, sizeof(wCommand) / sizeof(wCommand[0]), L"play mp3 from %lu notify", pausedPosition);
            if (mciSendString(wCommand, NULL, 0, hwnd) == 0) {
                isPlaying = 1;
                UpdateTooltip(hwnd);
                SetTimer(hwnd, IDT_TIMER, 1000, NULL);
                RefreshMenuText();
            }
            pausedPosition = 0;
        }
    }
}

void PauseMP3(HWND hwnd) {
    if (isPlaying) {
        WCHAR status[32];
        if (mciSendString(L"status mp3 position", status, sizeof(status)/sizeof(status[0]), NULL) == 0) {
            pausedPosition = wcstoul(status, NULL, 10);
        }
        mciSendString(L"pause mp3", NULL, 0, NULL);
        isPlaying = 0;
        KillTimer(hwnd, IDT_TIMER);
        WCHAR tooltip[MAX_TOOLTIP_LEN];
        WCHAR wMp3File[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, mp3File, -1, wMp3File, MAX_PATH);
        WCHAR* songTitle = GetMP3SongTitle(wMp3File);
        if (!songTitle) {
            songTitle = (WCHAR*)PathFindFileNameW(wMp3File);
        }

        if (trackCount > 1) {
            _snwprintf(tooltip, sizeof(tooltip) / sizeof(tooltip[0]),
                      L"%s (%d/%d)", songTitle, currentTrack + 1, trackCount);
        } else {
            _snwprintf(tooltip, sizeof(tooltip) / sizeof(tooltip[0]),
                      L"%s", songTitle);
        }

        SetTrayIcon(hwnd, hPlayIcon, tooltip);
        if (songTitle != PathFindFileNameW(wMp3File)) {
            CoTaskMemFree(songTitle);
        }
        RefreshMenuText();
    }
}

void StopMP3(HWND hwnd) {
    if (isPlaying || pausedPosition > 0) {
        mciSendString(L"close mp3", NULL, 0, NULL);
        isPlaying = 0;
        pausedPosition = 0;
        KillTimer(hwnd, IDT_TIMER);
        SetTrayIcon(hwnd, hPlayIcon, g_lang->tooltip_stopped);
        mp3File[0] = '\0';
        RefreshMenuText();
    }
}

BOOL ParseM3UFile(const WCHAR* m3uPath) {
    FILE* file = _wfopen(m3uPath, L"r");
    if (!file) return FALSE;

    WideCharToMultiByte(CP_ACP, 0, m3uPath, -1, playlist, MAX_PATH, NULL, NULL);
    char line[MAX_PATH] = {0};
    trackCount = 0;

    while (fgets(line, sizeof(line), file) && trackCount < MAX_TRACKS) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        line[strcspn(line, "\r\n")] = 0;
        strncpy(playlistFiles[trackCount], line, MAX_PATH);
        trackCount++;
    }

    fclose(file);
    return trackCount > 0;
}

void LoadNextTrack(HWND hwnd) {
    if (trackCount == 0) return;

    currentTrack++;
    if (currentTrack >= trackCount) {
        if (isReplay && trackCount > 1) {
            currentTrack = 0;
        } else {
            currentTrack = trackCount - 1;
            mciSendString(L"close mp3", NULL, 0, NULL);
            isPlaying = 0;
            KillTimer(hwnd, IDT_TIMER);
            SetTrayIcon(hwnd, hPlayIcon, g_lang->tooltip_stopped);
            RefreshMenuText();
            return;
        }
    }

    strncpy(mp3File, playlistFiles[currentTrack], MAX_PATH);
    mciSendString(L"close mp3", NULL, 0, NULL);
    isPlaying = 0;
    pausedPosition = 0; // Reset paused position for new track
    PlayMP3(hwnd);
}

void LoadPrevTrack(HWND hwnd) {
    if (trackCount == 0) return;

    currentTrack--;
    if (currentTrack < 0) {
        if (isReplay && trackCount > 1) {
            currentTrack = trackCount - 1;
        } else {
            currentTrack = 0;
            mciSendString(L"close mp3", NULL, 0, NULL);
            isPlaying = 0;
            KillTimer(hwnd, IDT_TIMER);
            SetTrayIcon(hwnd, hPlayIcon, g_lang->tooltip_stopped);
            RefreshMenuText();
            return;
        }
    }

    strncpy(mp3File, playlistFiles[currentTrack], MAX_PATH);
    mciSendString(L"close mp3", NULL, 0, NULL);
    isPlaying = 0;
    pausedPosition = 0; // Reset paused position for new track
    PlayMP3(hwnd);
}

void ToggleReplay(HWND hwnd) {
    isReplay = !isReplay;
    if (isPlaying && trackCount <= 1) {
        mciSendString(L"close mp3", NULL, 0, NULL);
        pausedPosition = 0; // Reset paused position when toggling replay
        PlayMP3(hwnd);
    }
    SaveReplayStateToRegistry();
    RefreshMenuText();
}

void SetLanguage(LANG *newLang) {
    g_lang = newLang;
    RefreshMenuText();
    UpdateTooltip(GetForegroundWindow());
}

void SaveLanguageSelectionToRegistry(void) {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\TrayMp3", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        DWORD langId = g_lang == &lang_en ? ID_MENU_LANG_EN : g_lang == &lang_hu ? ID_MENU_LANG_HU :
                        g_lang == &lang_de ? ID_MENU_LANG_DE : g_lang == &lang_it ? ID_MENU_LANG_IT :
                        g_lang == &lang_es ? ID_MENU_LANG_ES : g_lang == &lang_fr ? ID_MENU_LANG_FR :
                        g_lang == &lang_ru ? ID_MENU_LANG_RU : ID_MENU_LANG_EN;
        RegSetValueExW(hKey, L"Language", 0, REG_DWORD, (const BYTE *)&langId, sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

BOOL LoadLanguageSelectionFromRegistry(void) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\TrayMp3", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD langId, dataSize = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"Language", NULL, NULL, (LPBYTE)&langId, &dataSize) == ERROR_SUCCESS) {
            switch (langId) {
                case ID_MENU_LANG_EN: g_lang = &lang_en; break;
                case ID_MENU_LANG_HU: g_lang = &lang_hu; break;
                case ID_MENU_LANG_DE: g_lang = &lang_de; break;
                case ID_MENU_LANG_IT: g_lang = &lang_it; break;
                case ID_MENU_LANG_ES: g_lang = &lang_es; break;
                case ID_MENU_LANG_FR: g_lang = &lang_fr; break;
                case ID_MENU_LANG_RU: g_lang = &lang_ru; break;
                default: g_lang = &lang_en; break;
            }
            RegCloseKey(hKey);
            return TRUE;
        }
        RegCloseKey(hKey);
    }
    return FALSE;
}

void SaveReplayStateToRegistry(void) {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\TrayMp3", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        DWORD replayState = isReplay ? 1 : 0;
        RegSetValueExW(hKey, L"Replay", 0, REG_DWORD, (const BYTE *)&replayState, sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

BOOL LoadReplayStateFromRegistry(void) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\TrayMp3", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD replayState, dataSize = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"Replay", NULL, NULL, (LPBYTE)&replayState, &dataSize) == ERROR_SUCCESS) {
            isReplay = (replayState == 1);
            RegCloseKey(hKey);
            return TRUE;
        }
        RegCloseKey(hKey);
    }
    return FALSE;
}
void SaveVolumeToRegistry(void) {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\TrayMp3", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, L"Volume", 0, REG_DWORD, (const BYTE *)&currentVolume, sizeof(DWORD));
        RegCloseKey(hKey);
    }
}
BOOL LoadVolumeFromRegistry(void) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\TrayMp3", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD vol, dataSize = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"Volume", NULL, NULL, (LPBYTE)&vol, &dataSize) == ERROR_SUCCESS) {
            currentVolume = vol;
            if (currentVolume > 1000) currentVolume = 1000;
            if (currentVolume < 0) currentVolume = 0;
            RegCloseKey(hKey);
            return TRUE;
        }
        RegCloseKey(hKey);
    }
    currentVolume = 500;
    return FALSE;
}
void SaveAutoStartStateToRegistry(void) {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        if (autoStart) {
            WCHAR exePath[MAX_PATH];
            GetModuleFileNameW(NULL, exePath, MAX_PATH);
            RegSetValueExW(hKey, L"TrayMp3", 0, REG_SZ, (const BYTE*)exePath, (wcslen(exePath) + 1) * sizeof(WCHAR));
        } else {
            RegDeleteValueW(hKey, L"TrayMp3");
        }
        RegCloseKey(hKey);
    }
}

BOOL LoadAutoStartStateFromRegistry(void) {
    HKEY hKey;
    autoStart = 0;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        WCHAR exePath[MAX_PATH];
        DWORD type = 0, size = sizeof(exePath);
        if (RegQueryValueExW(hKey, L"TrayMp3", NULL, &type, (LPBYTE)exePath, &size) == ERROR_SUCCESS && type == REG_SZ) {
            WCHAR currentExe[MAX_PATH];
            GetModuleFileNameW(NULL, currentExe, MAX_PATH);
            if (wcscmp(exePath, currentExe) == 0) {
                autoStart = 1;
            }
        }
        RegCloseKey(hKey);
    }
    return autoStart;
}

void RefreshMenuText(void) {
    if (!g_hMenu) return;
    ModifyMenuW(g_hMenu, IDM_PLAYPAUSE, MF_BYCOMMAND | MF_STRING, IDM_PLAYPAUSE, g_lang->menu_playpause);
    ModifyMenuW(g_hMenu, IDM_STOP, MF_BYCOMMAND | MF_STRING, IDM_STOP, g_lang->menu_stop);
    ModifyMenuW(g_hMenu, IDM_OPEN, MF_BYCOMMAND | MF_STRING, IDM_OPEN, g_lang->menu_open);
    ModifyMenuW(g_hMenu, IDM_NEXT, MF_BYCOMMAND | MF_STRING, IDM_NEXT, g_lang->menu_next);
    ModifyMenuW(g_hMenu, IDM_PREV, MF_BYCOMMAND | MF_STRING, IDM_PREV, g_lang->menu_prev);
    ModifyMenuW(g_hMenu, IDM_VOLUME, MF_BYCOMMAND | MF_STRING, IDM_VOLUME, g_lang->menu_volume);
    ModifyMenuW(g_hMenu, IDM_REPLAY, MF_BYCOMMAND | MF_STRING | (isReplay ? MF_CHECKED : 0), IDM_REPLAY, g_lang->menu_replay);
    ModifyMenuW(g_hMenu, IDM_AUTOSTART, MF_BYCOMMAND | MF_STRING | (autoStart ? MF_CHECKED : 0), IDM_AUTOSTART, L"Auto Start");
    ModifyMenuW(g_hMenu, IDM_ABOUT, MF_BYCOMMAND | MF_STRING, IDM_ABOUT, g_lang->menu_about);
    ModifyMenuW(g_hMenu, IDM_EXIT, MF_BYCOMMAND | MF_STRING, IDM_EXIT, g_lang->menu_exit);
    // Add all language options to the language menu
    if (g_hLangMenu) {
        ModifyMenuW(g_hLangMenu, 0, MF_BYPOSITION | MF_STRING, ID_MENU_LANG_EN, L"English");
        ModifyMenuW(g_hLangMenu, 1, MF_BYPOSITION | MF_STRING, ID_MENU_LANG_HU, L"Magyar");
        ModifyMenuW(g_hLangMenu, 2, MF_BYPOSITION | MF_STRING, ID_MENU_LANG_DE, L"Deutsch");
        ModifyMenuW(g_hLangMenu, 3, MF_BYPOSITION | MF_STRING, ID_MENU_LANG_IT, L"Italiano");
        ModifyMenuW(g_hLangMenu, 4, MF_BYPOSITION | MF_STRING, ID_MENU_LANG_ES, L"Español");
        ModifyMenuW(g_hLangMenu, 5, MF_BYPOSITION | MF_STRING, ID_MENU_LANG_FR, L"Français");
        ModifyMenuW(g_hLangMenu, 6, MF_BYPOSITION | MF_STRING, ID_MENU_LANG_RU, L"Русский");
    }
    ModifyMenuW(g_hMenu, (UINT_PTR)g_hLangMenu, MF_BYCOMMAND | MF_STRING | MF_POPUP, (UINT_PTR)g_hLangMenu, g_lang->menu_language);
    ModifyMenuW(g_hMenu, IDM_REPLAY, MF_BYCOMMAND | MF_STRING | (isReplay ? MF_CHECKED : 0), IDM_REPLAY, g_lang->menu_replay);
    ModifyMenuW(g_hMenu, IDM_AUTOSTART, MF_BYCOMMAND | MF_STRING | (autoStart ? MF_CHECKED : 0), IDM_AUTOSTART, L"Auto Start");
    ModifyMenuW(g_hMenu, IDM_ABOUT, MF_BYCOMMAND | MF_STRING, IDM_ABOUT, g_lang->menu_about);
    ModifyMenuW(g_hMenu, IDM_EXIT, MF_BYCOMMAND | MF_STRING, IDM_EXIT, g_lang->menu_exit);
    if (g_hLangMenu) {
        CheckMenuItem(g_hLangMenu, ID_MENU_LANG_EN, MF_BYCOMMAND | (g_lang == &lang_en ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(g_hLangMenu, ID_MENU_LANG_HU, MF_BYCOMMAND | (g_lang == &lang_hu ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(g_hLangMenu, ID_MENU_LANG_DE, MF_BYCOMMAND | (g_lang == &lang_de ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(g_hLangMenu, ID_MENU_LANG_IT, MF_BYCOMMAND | (g_lang == &lang_it ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(g_hLangMenu, ID_MENU_LANG_ES, MF_BYCOMMAND | (g_lang == &lang_es ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(g_hLangMenu, ID_MENU_LANG_FR, MF_BYCOMMAND | (g_lang == &lang_fr ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(g_hLangMenu, ID_MENU_LANG_RU, MF_BYCOMMAND | (g_lang == &lang_ru ? MF_CHECKED : MF_UNCHECKED));
    }
}

void ShowAboutDialog(HWND hwndParent) {
    DialogBoxW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDD_ABOUT), hwndParent, AboutDlgProc);
}

INT_PTR CALLBACK AboutDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HFONT hLinkFont = NULL;
    static HBRUSH hBrush = NULL;

    switch (uMsg) {
        case WM_INITDIALOG: {
            RECT rc, rcOwner;
            GetWindowRect(GetDesktopWindow(), &rcOwner);
            GetWindowRect(hwndDlg, &rc);
            int x = (rcOwner.right - (rc.right - rc.left)) / 2;
            int y = (rcOwner.bottom - (rc.bottom - rc.top)) / 2;
            SetWindowPos(hwndDlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

            HFONT hFont = (HFONT)SendDlgItemMessageW(hwndDlg, IDC_WEBSITE, WM_GETFONT, 0, 0);
            LOGFONTW lf;
            GetObjectW(hFont, sizeof(LOGFONTW), &lf);
            lf.lfUnderline = TRUE;
            hLinkFont = CreateFontIndirectW(&lf);
            SendDlgItemMessageW(hwndDlg, IDC_WEBSITE, WM_SETFONT, (WPARAM)hLinkFont, TRUE);
            SendDlgItemMessageW(hwndDlg, IDC_COFFEE, WM_SETFONT, (WPARAM)hLinkFont, TRUE);

            SetDlgItemTextW(hwndDlg, IDC_WEBSITE, g_lang->about_website);
            SetDlgItemTextW(hwndDlg, IDC_COFFEE, g_lang->about_coffee);

            hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
            return TRUE;
        }
        case WM_CTLCOLORDLG:
            return (INT_PTR)hBrush;

        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            HWND hwndCtrl = (HWND)lParam;

            if (GetDlgCtrlID(hwndCtrl) == IDC_WEBSITE || GetDlgCtrlID(hwndCtrl) == IDC_COFFEE) {
                SetTextColor(hdc, RGB(0, 0, 255));
            } else {
                SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
            }

            SetBkMode(hdc, TRANSPARENT);
            return (INT_PTR)hBrush;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_WEBSITE && HIWORD(wParam) == STN_CLICKED) {
                ShellExecuteW(NULL, L"open", L"https://github.com/lutischan-ferenc/tray-mp3", NULL, NULL, SW_SHOWNORMAL);
                return TRUE;
            }
            if (LOWORD(wParam) == IDC_COFFEE && HIWORD(wParam) == STN_CLICKED) {
                ShellExecuteW(NULL, L"open", L"https://coff.ee/lutischanf", NULL, NULL, SW_SHOWNORMAL);
                return TRUE;
            }
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                if (hLinkFont) {
                    DeleteObject(hLinkFont);
                    hLinkFont = NULL;
                }
                if (hBrush) {
                    DeleteObject(hBrush);
                    hBrush = NULL;
                }
                EndDialog(hwndDlg, LOWORD(wParam));
                return TRUE;
            }
            break;

        case WM_DESTROY:
            if (hLinkFont) {
                DeleteObject(hLinkFont);
                hLinkFont = NULL;
            }
            if (hBrush) {
                DeleteObject(hBrush);
                hBrush = NULL;
            }
            break;
    }
    return FALSE;
}
void UpdateVolumeFromSlider(void) {
    if (g_hVolumeSlider) {
        int pos = (int)SendMessage(g_hVolumeSlider, TBM_GETPOS, 0, 0);
        WCHAR cmd[64];
        _snwprintf(cmd, 64, L"setaudio mp3 volume to %d", pos);
        mciSendString(cmd, NULL, 0, NULL);
        currentVolume = pos;
        SaveVolumeToRegistry();
    }
}
LRESULT CALLBACK VolumeWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            g_hVolumeWnd = hwnd;
            g_hVolumeSlider = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
                WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS | TBS_TOOLTIPS,
                10, 10, 180, 40, hwnd, (HMENU)IDC_VOLUME_SLIDER,
                GetModuleHandle(NULL), NULL);
            if (g_hVolumeSlider) {
                SendMessage(g_hVolumeSlider, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 1000));
                SendMessage(g_hVolumeSlider, TBM_SETPAGESIZE, 0, (LPARAM)100);
                SendMessage(g_hVolumeSlider, TBM_SETTICFREQ, 100, 0);
                SendMessage(g_hVolumeSlider, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)currentVolume);
            }
            break;
        }
        case WM_HSCROLL: {
            if ((HWND)lParam == g_hVolumeSlider) {
                switch (LOWORD(wParam)) {
                    case TB_THUMBTRACK:
                    case TB_THUMBPOSITION:
                    case TB_ENDTRACK:
                        UpdateVolumeFromSlider();
                        break;
                }
            }
            break;
        }
        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE) {
                DestroyWindow(hwnd);
            }
            break;
        case WM_DESTROY:
            g_hVolumeWnd = NULL;
            g_hVolumeSlider = NULL;
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}
void ShowVolumeControl(HWND hwndOwner) {
    if (g_hVolumeWnd && IsWindow(g_hVolumeWnd)) {
        SetForegroundWindow(g_hVolumeWnd);
        return;
    }

    POINT pt;
    GetCursorPos(&pt);

    HWND hPopup = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        szVolumeClassName, g_lang->menu_volume,
        WS_POPUP | WS_BORDER | WS_CAPTION,
        pt.x - 100, pt.y - 60, 200, 80,
        hwndOwner, NULL, GetModuleHandle(NULL), NULL);

    if (hPopup) {
        ShowWindow(hPopup, SW_SHOW);
        SetForegroundWindow(hPopup);
    }
}
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            g_hMenu = CreatePopupMenu();
            AppendMenuW(g_hMenu, MF_STRING, IDM_PLAYPAUSE, g_lang->menu_playpause);
            AppendMenuW(g_hMenu, MF_STRING, IDM_STOP, g_lang->menu_stop);
            AppendMenuW(g_hMenu, MF_STRING, IDM_OPEN, g_lang->menu_open);
            AppendMenuW(g_hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(g_hMenu, MF_STRING, IDM_NEXT, g_lang->menu_next);
            AppendMenuW(g_hMenu, MF_STRING, IDM_PREV, g_lang->menu_prev);
            AppendMenuW(g_hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(g_hMenu, MF_STRING, IDM_VOLUME, g_lang->menu_volume);
            AppendMenuW(g_hMenu, MF_SEPARATOR, 0, NULL);
            g_hLangMenu = CreatePopupMenu();
            AppendMenuW(g_hLangMenu, MF_STRING, ID_MENU_LANG_EN, L"English");
            AppendMenuW(g_hLangMenu, MF_STRING, ID_MENU_LANG_HU, L"Magyar");
            AppendMenuW(g_hLangMenu, MF_STRING, ID_MENU_LANG_DE, L"Deutsch");
            AppendMenuW(g_hLangMenu, MF_STRING, ID_MENU_LANG_IT, L"Italiano");
            AppendMenuW(g_hLangMenu, MF_STRING, ID_MENU_LANG_ES, L"Español");
            AppendMenuW(g_hLangMenu, MF_STRING, ID_MENU_LANG_FR, L"Français");
            AppendMenuW(g_hLangMenu, MF_STRING, ID_MENU_LANG_RU, L"Русский");
            AppendMenuW(g_hMenu, MF_POPUP, (UINT_PTR)g_hLangMenu, g_lang->menu_language);
            LoadReplayStateFromRegistry();
            AppendMenuW(g_hMenu, MF_STRING | (isReplay ? MF_CHECKED : 0), IDM_REPLAY, g_lang->menu_replay);
            AppendMenuW(g_hMenu, MF_STRING | (autoStart ? MF_CHECKED : 0), IDM_AUTOSTART, L"Auto Start");
            AppendMenuW(g_hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(g_hMenu, MF_STRING, IDM_ABOUT, g_lang->menu_about);
            AppendMenuW(g_hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(g_hMenu, MF_STRING, IDM_EXIT, g_lang->menu_exit);
            nid.cbSize = sizeof(NOTIFYICONDATA);
            nid.hWnd = hwnd;
            nid.uID = IDM_TRAY;
            nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            nid.uCallbackMessage = IDM_TRAY;
            hPlayIcon = CreateIconFromText(L"\u25BA", RGB(0, 0, 255));
            hPauseIcon = CreateIconFromText(L"\u2759\u2759", RGB(0, 0, 255));
            if (!hPlayIcon || !hPauseIcon) {
                MessageBoxW(hwnd, L"Failed to create icon(s)!", L"Error", MB_OK | MB_ICONERROR);
                PostQuitMessage(1);
                return -1;
            }
            nid.hIcon = hPlayIcon;
            wcsncpy(nid.szTip, g_lang->tooltip_stopped, sizeof(nid.szTip) / sizeof(nid.szTip[0]));
            if (!Shell_NotifyIcon(NIM_ADD, &nid)) {
                MessageBoxW(hwnd, L"Failed to add system tray icon!", L"Error", MB_OK | MB_ICONERROR);
                PostQuitMessage(1);
                return -1;
            }
            SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hPlayIcon);
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hPlayIcon);
            LoadLanguageSelectionFromRegistry();
            LoadVolumeFromRegistry();
            RefreshMenuText();
            UpdateTooltip(GetForegroundWindow());
            break;
        case WM_TIMER:
            if (wParam == IDT_TIMER && isPlaying) {
                UpdateTooltip(hwnd);
            }
            break;
        case MM_MCINOTIFY:
            if (wParam == MCI_NOTIFY_SUCCESSFUL) {
                if (trackCount > 1) {
                    LoadNextTrack(hwnd);
                } else if (isReplay) {
                    mciSendString(L"seek mp3 to start", NULL, 0, NULL);
                    mciSendString(L"play mp3 notify", NULL, 0, hwnd);
                } else {
                    mciSendString(L"close mp3", NULL, 0, NULL);
                    isPlaying = 0;
                    pausedPosition = 0;
                    KillTimer(hwnd, IDT_TIMER);
                    SetTrayIcon(hwnd, hPlayIcon, g_lang->tooltip_stopped);
                    RefreshMenuText();
                }
            }
            break;

        case IDM_TRAY:
            if (lParam == WM_LBUTTONDOWN) {
                if (strlen(mp3File) == 0) {
                    SelectMP3File(hwnd);
                } else {
                    if (isPlaying) {
                        PauseMP3(hwnd);
                    } else {
                        PlayMP3(hwnd);
                    }
                }
            } else if (lParam == WM_RBUTTONDOWN) {
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hwnd);
                int cmd = TrackPopupMenu(g_hMenu, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, NULL);
                switch (cmd) {
                    case IDM_PLAYPAUSE:
                        if (strlen(mp3File) == 0) {
                            SelectMP3File(hwnd);
                        } else if (isPlaying) {
                            PauseMP3(hwnd);
                        } else {
                            PlayMP3(hwnd);
                        }
                        break;
                    case IDM_STOP:
                        StopMP3(hwnd);
                        break;
                    case IDM_OPEN:
                        SelectMP3File(hwnd);
                        break;
                    case IDM_REPLAY:
                        ToggleReplay(hwnd);
                        break;
                    case IDM_NEXT:
                        LoadNextTrack(hwnd);
                        break;
                    case IDM_PREV:
                        LoadPrevTrack(hwnd);
                        break;
                    case IDM_VOLUME:
                        ShowVolumeControl(hwnd);
                        break;
                    case IDM_ABOUT:
                        ShowAboutDialog(hwnd);
                        break;
                    case IDM_EXIT:
                        mciSendString(L"close mp3", NULL, 0, NULL);
                        Shell_NotifyIcon(NIM_DELETE, &nid);
                        DestroyMenu(g_hMenu);
                        DestroyMenu(g_hLangMenu);
                        DestroyIcon(hPlayIcon);
                        DestroyIcon(hPauseIcon);
                        CleanupMCI();
                        PostQuitMessage(0);
                        break;
                    case ID_MENU_LANG_EN:
                        SetLanguage(&lang_en);
                        SaveLanguageSelectionToRegistry();
                        break;
                    case ID_MENU_LANG_HU:
                        SetLanguage(&lang_hu);
                        SaveLanguageSelectionToRegistry();
                        break;
                    case ID_MENU_LANG_DE:
                        SetLanguage(&lang_de);
                        SaveLanguageSelectionToRegistry();
                        break;
                    case ID_MENU_LANG_IT:
                        SetLanguage(&lang_it);
                        SaveLanguageSelectionToRegistry();
                        break;
                    case ID_MENU_LANG_ES:
                        SetLanguage(&lang_es);
                        SaveLanguageSelectionToRegistry();
                        break;
                    case ID_MENU_LANG_FR:
                        SetLanguage(&lang_fr);
                        SaveLanguageSelectionToRegistry();
                        break;
                    case ID_MENU_LANG_RU:
                        SetLanguage(&lang_ru);
                        SaveLanguageSelectionToRegistry();
                        break;
                    case IDM_AUTOSTART:
                        autoStart = !autoStart;
                        SaveAutoStartStateToRegistry();
                        RefreshMenuText();
                        break;
                }
            }
            break;

        case WM_DESTROY:
            mciSendString(L"close mp3", NULL, 0, NULL);
            Shell_NotifyIcon(NIM_DELETE, &nid);
            DestroyMenu(g_hMenu);
            DestroyMenu(g_hLangMenu);
            DestroyIcon(hPlayIcon);
            DestroyIcon(hPauseIcon);
            CleanupMCI();
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        MessageBoxW(NULL, L"COM initialization failed!", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icc);
    WNDCLASSW wcVol = {0};
    wcVol.lpfnWndProc = VolumeWndProc;
    wcVol.hInstance = hInstance;
    wcVol.lpszClassName = szVolumeClassName;
    wcVol.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcVol.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    RegisterClassW(&wcVol);
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = szClassName;
    wc.hIcon = hPlayIcon;
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(szClassName, L"TrayMp3", WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, NULL, NULL, hInstance, NULL);
    if (hwnd == NULL) {
        MessageBoxW(NULL, L"Window creation failed!", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    LoadLanguageSelectionFromRegistry();
    LoadReplayStateFromRegistry();
    LoadAutoStartStateFromRegistry();
    LoadVolumeFromRegistry();
    RefreshMenuText();

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    CoUninitialize();
    DestroyIcon(wc.hIcon);
    return msg.wParam;
}
