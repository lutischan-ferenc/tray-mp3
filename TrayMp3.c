#define _WIN32_WINNT 0x0600
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commdlg.h>
#include <stdio.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <mmreg.h>
#include <mmsystem.h>

// Minimap3 integration
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ONLY_MP3
#include "minimp3.h"
#include "minimp3_ex.h"

#define MAX_TRACKS 100
#define MAX_STRING_LEN 64
#define MAX_TOOLTIP_LEN 256

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "winmm.lib")

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
    IDM_POSITION = 1014,
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
    IDT_DECODE_TIMER = 2002,
    IDT_SEEK_TIMER = 2003,
    IDC_VOLUME_SLIDER = 3000,
    IDC_VOLUME_LABEL = 3001,
    IDC_SEEK_SLIDER = 4001,
    IDC_TIME_LABEL_CURRENT = 4002,
    IDC_TIME_LABEL_TOTAL = 4003,
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
    WCHAR menu_autostart[MAX_STRING_LEN];
    WCHAR menu_position[MAX_STRING_LEN];
    WCHAR tooltip_stopped[MAX_STRING_LEN];
    WCHAR about_website[MAX_STRING_LEN];
    WCHAR about_coffee[MAX_STRING_LEN];
} LANG;

static LANG lang_en = { L"Play / Pause", L"Open", L"Replay", L"About", L"Exit", L"Next", L"Previous", L"Stop", L"Language", L"Volume", L"Auto Start", L"Position", L"Stopped", L"Visit our website", L"Support us with a coffee" };
static LANG lang_hu = { L"Lejátszás / Szüneteltetés", L"Megnyitás", L"Ismétlés", L"Névjegy", L"Kilépés", L"Következő", L"Előző", L"Leállítás", L"Nyelv", L"Hangerő", L"Automatikus indítás", L"Pozíció", L"Leállítva", L"Weboldal", L"Támogass egy kávéval" };
static LANG lang_de = { L"Wiedergabe / Pause", L"Öffnen", L"Wiederholen", L"Über", L"Beenden", L"Nächster", L"Vorheriger", L"Stopp", L"Sprache", L"Lautstärke", L"Autostart", L"Position", L"Gestoppt", L"Besuchen Sie unsere Webseite", L"Unterstützen Sie uns mit einem Kaffee" };
static LANG lang_it = { L"Riproduci / Pausa", L"Apri", L"Ripeti", L"Informazioni", L"Esci", L"Successivo", L"Precedente", L"Arresta", L"Lingua", L"Volume", L"Avvio automatico", L"Posizione", L"Fermato", L"Visita il nostro sito web", L"Supportaci con un caffè" };
static LANG lang_es = { L"Reproducir / Pausa", L"Abrir", L"Repetir", L"Acerca de", L"Salir", L"Siguiente", L"Anterior", L"Detener", L"Idioma", L"Volumen", L"Inicio automático", L"Posición", L"Detenido", L"Visita nuestro sitio web", L"Apóyanos con un café" };
static LANG lang_fr = { L"Lecture / Pause", L"Ouvrir", L"Répéter", L"À propos", L"Quitter", L"Suivant", L"Précédent", L"Arrêter", L"Langue", L"Volume", L"Démarrage automatique", L"Position", L"Arrêté", L"Visitez notre site web", L"Soutenez-nous avec un café" };
static LANG lang_ru = { L"Воспроизвести / Пауза", L"Открыть", L"Повтор", L"О программе", L"Выход", L"Следующий", L"Предыдущий", L"Остановить", L"Язык", L"Громкость", L"Автозапуск", L"Положение", L"Остановлено", L"Посетите наш сайт", L"Поддержите нас кофе" };

static LANG *g_lang = &lang_en;
static HMENU g_hMenu = NULL;
static HMENU g_hLangMenu = NULL;

NOTIFYICONDATA nid;
HICON hPlayIcon, hPauseIcon;

int isPlaying = 0;
int isReplay = 0;
char mp3File[MAX_PATH] = "";
WCHAR szClassName[] = L"MP3PlayerWndClass";
WCHAR szVolumeClassName[] = L"VolumePopup";
WCHAR szSeekClassName[] = L"SeekPopup";

// Playlist variables
char playlist[MAX_PATH] = "";
char playlistFiles[MAX_TRACKS][MAX_PATH];
int currentTrack = 0;
int trackCount = 0;

// Volume control variables
static HWND g_hVolumeWnd = NULL;
static HWND g_hVolumeSlider = NULL;
static HWND g_hVolumeLabel = NULL;

static HWND g_hSeekWnd = NULL;
static HWND g_hSeekSlider = NULL;
static HWND g_hTimeLabelCurrent = NULL;
static HWND g_hTimeLabelTotal = NULL;
static int isSeeking = 0;

static mp3dec_ex_t mp3d;
static HWAVEOUT hWaveOut = NULL;
static WAVEHDR waveHeaders[4];
static int currentHeader = 0;
static DWORD totalDuration = 0;
static DWORD currentPosition = 0;
static DWORD lastUpdateTime = 0;

// Function declarations
void StopMP3(HWND hwnd);
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
void SaveAutoStartStateToRegistry(void);
BOOL LoadAutoStartStateFromRegistry(void);
void ShowVolumeControl(HWND hwndOwner);
void UpdateVolumeFromSlider(void);
LRESULT CALLBACK VolumeWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void CleanupMP3(void);
BOOL InitMP3Decoder(const char* filename, HWND hwnd);
void FillAudioBuffers(HWND hwnd);
void CALLBACK WaveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
void ShowSeekControl(HWND hwndOwner);
LRESULT CALLBACK SeekWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

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
    HFONT hFont = CreateFontW(27, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
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

void CleanupMP3() {
    if (hWaveOut) {
        waveOutReset(hWaveOut);
        for (int i = 0; i < 4; i++) {
            if (waveHeaders[i].lpData) {
                waveOutUnprepareHeader(hWaveOut, &waveHeaders[i], sizeof(WAVEHDR));
                free(waveHeaders[i].lpData);
                waveHeaders[i].lpData = NULL;
            }
        }
        waveOutClose(hWaveOut);
        hWaveOut = NULL;
    }

    if (mp3d.buffer) {
        mp3dec_ex_close(&mp3d);
    }

    isPlaying = 0;
    currentPosition = 0;
    totalDuration = 0;
    RefreshMenuText();
}

WCHAR* GetMP3SongTitle(const WCHAR* path) {
    // Simple implementation - just return the filename
    static WCHAR title[MAX_PATH];
    wcscpy(title, PathFindFileNameW(path));

    // Remove extension
    WCHAR* ext = wcsrchr(title, L'.');
    if (ext) *ext = L'\0';

    return title;
}

void UpdateTooltip(HWND hwnd) {
    if (strlen(mp3File) == 0 || totalDuration == 0) {
        SetTrayIcon(hwnd, hPlayIcon, g_lang->tooltip_stopped);
        return;
    }

    WCHAR tooltip[MAX_TOOLTIP_LEN] = {0};
    WCHAR wMp3File[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, mp3File, -1, wMp3File, MAX_PATH);
    WCHAR* songTitle = GetMP3SongTitle(wMp3File);

    // Calculate remaining time
    DWORD remaining = (totalDuration - currentPosition) / 1000;
    int min = remaining / 60;
    int sec = remaining % 60;

    DWORD currentSec = currentPosition / 1000;
    int curMin = currentSec / 60;
    int curS = currentSec % 60;

    if (trackCount > 1) {
        _snwprintf(tooltip, sizeof(tooltip)/sizeof(tooltip[0]), L"%s (%d/%d) [%d:%02d]", songTitle, currentTrack + 1, trackCount, curMin, curS);
    } else {
        _snwprintf(tooltip, sizeof(tooltip)/sizeof(tooltip[0]), L"%s [%d:%02d]", songTitle, curMin, curS);
    }

    SetTrayIcon(hwnd, isPlaying ? hPauseIcon : hPlayIcon, tooltip);
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
        CleanupMP3();
        isPlaying = 0;
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

BOOL InitMP3Decoder(const char* filename, HWND hwnd) {
    if (mp3d.buffer) {
        mp3dec_ex_close(&mp3d);
    }

    if (mp3dec_ex_open(&mp3d, filename, MP3D_SEEK_TO_SAMPLE)) {
        return FALSE;
    }

    totalDuration = (DWORD)(mp3d.samples * 1000.0 / (mp3d.info.channels * mp3d.info.hz));

    // Set up wave format
    WAVEFORMATEX wfx = {0};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = mp3d.info.channels;
    wfx.nSamplesPerSec = mp3d.info.hz;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, (DWORD_PTR)WaveOutProc, (DWORD_PTR)hwnd, CALLBACK_FUNCTION) != MMSYSERR_NOERROR) {
        return FALSE;
    }

    // Set volume
    DWORD volume = (currentVolume * 0xFFFF) / 1000;
    waveOutSetVolume(hWaveOut, volume | (volume << 16));

    // Prepare buffers
    int bufferSize = 8192 * wfx.nBlockAlign;
    for (int i = 0; i < 4; i++) {
        waveHeaders[i].lpData = (LPSTR)malloc(bufferSize);
        waveHeaders[i].dwBufferLength = bufferSize;
        waveHeaders[i].dwFlags = 0;
        waveOutPrepareHeader(hWaveOut, &waveHeaders[i], sizeof(WAVEHDR));
    }
    RefreshMenuText();
    return TRUE;
}

void CALLBACK WaveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    HWND hwnd = (HWND)dwInstance;

    if (uMsg == WOM_DONE) {
        // A buffer has finished playing, fill it again
        PostMessage(hwnd, WM_USER + 1, 0, 0);
    }
}

void FillAudioBuffers(HWND hwnd) {
    if (!isPlaying || !hWaveOut) return;

    for (int i = 0; i < 4; i++) {
        if (!(waveHeaders[i].dwFlags & WHDR_INQUEUE)) {
            size_t samplesRead = mp3dec_ex_read(&mp3d, (mp3d_sample_t*)waveHeaders[i].lpData, waveHeaders[i].dwBufferLength / sizeof(mp3d_sample_t));
            if (samplesRead > 0) {
                waveHeaders[i].dwBufferLength = samplesRead * sizeof(mp3d_sample_t);
                waveOutWrite(hWaveOut, &waveHeaders[i], sizeof(WAVEHDR));

                // Update position
                currentPosition = (DWORD)(mp3d.cur_sample * 1000.0 / (mp3d.info.channels * mp3d.info.hz));
                lastUpdateTime = GetTickCount();
            } else {
                // End of file
                if (trackCount > 1) {
                    LoadNextTrack(hwnd);
                } else if (isReplay) {
                    mp3dec_ex_seek(&mp3d, 0);
                    FillAudioBuffers(hwnd);
                } else {
                    StopMP3(hwnd);
                }
                break;
            }
        }
    }
}

void PlayMP3(HWND hwnd) {
    if (strlen(mp3File) == 0) {
        SelectMP3File(hwnd);
        return;
    }

    if (!isPlaying) {
        if (!hWaveOut) {
            if (!InitMP3Decoder(mp3File, hwnd)) {
                MessageBoxW(hwnd, L"Failed to open MP3 file", L"Error", MB_OK | MB_ICONERROR);
                return;
            }
        }

        if (waveOutRestart(hWaveOut) == MMSYSERR_NOERROR) {
            isPlaying = 1;
            FillAudioBuffers(hwnd);
            UpdateTooltip(hwnd);
            SetTimer(hwnd, IDT_TIMER, 1000, NULL);
            RefreshMenuText();
        }
    }
}

void PauseMP3(HWND hwnd) {
    if (isPlaying && hWaveOut) {
        if (waveOutPause(hWaveOut) == MMSYSERR_NOERROR) {
            isPlaying = 0;
            KillTimer(hwnd, IDT_TIMER);

            WCHAR tooltip[MAX_TOOLTIP_LEN];
            WCHAR wMp3File[MAX_PATH];
            MultiByteToWideChar(CP_ACP, 0, mp3File, -1, wMp3File, MAX_PATH);
            WCHAR* songTitle = GetMP3SongTitle(wMp3File);

            if (trackCount > 1) {
                _snwprintf(tooltip, sizeof(tooltip) / sizeof(tooltip[0]), L"%s (%d/%d) (Paused)", songTitle, currentTrack + 1, trackCount);
            } else {
                _snwprintf(tooltip, sizeof(tooltip) / sizeof(tooltip[0]), L"%s (Paused)", songTitle);
            }

            SetTrayIcon(hwnd, hPlayIcon, tooltip);
            RefreshMenuText();
        }
    }
}

void StopMP3(HWND hwnd) {
    if (isPlaying || hWaveOut) {
        CleanupMP3();
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
            StopMP3(hwnd);
            return;
        }
    }

    strncpy(mp3File, playlistFiles[currentTrack], MAX_PATH);
    CleanupMP3();
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
            StopMP3(hwnd);
            return;
        }
    }

    strncpy(mp3File, playlistFiles[currentTrack], MAX_PATH);
    CleanupMP3();
    PlayMP3(hwnd);
}

void ToggleReplay(HWND hwnd) {
    isReplay = !isReplay;
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
        DWORD langId = g_lang == &lang_en ? ID_MENU_LANG_EN :
                       g_lang == &lang_hu ? ID_MENU_LANG_HU :
                       g_lang == &lang_de ? ID_MENU_LANG_DE :
                       g_lang == &lang_it ? ID_MENU_LANG_IT :
                       g_lang == &lang_es ? ID_MENU_LANG_ES :
                       g_lang == &lang_fr ? ID_MENU_LANG_FR :
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
    ModifyMenuW(g_hMenu, IDM_POSITION, MF_BYCOMMAND | MF_STRING | (mp3File[0] == '\0' || totalDuration == 0 ? MF_GRAYED : MF_ENABLED), IDM_POSITION, g_lang->menu_position);
    ModifyMenuW(g_hMenu, IDM_REPLAY, MF_BYCOMMAND | MF_STRING | (isReplay ? MF_CHECKED : 0), IDM_REPLAY, g_lang->menu_replay);
    ModifyMenuW(g_hMenu, IDM_AUTOSTART, MF_BYCOMMAND | MF_STRING | (autoStart ? MF_CHECKED : 0), IDM_AUTOSTART, g_lang->menu_autostart);
    ModifyMenuW(g_hMenu, IDM_ABOUT, MF_BYCOMMAND | MF_STRING, IDM_ABOUT, g_lang->menu_about);
    ModifyMenuW(g_hMenu, IDM_EXIT, MF_BYCOMMAND | MF_STRING, IDM_EXIT, g_lang->menu_exit);

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
                if (hLinkFont) { DeleteObject(hLinkFont); hLinkFont = NULL; }
                if (hBrush) { DeleteObject(hBrush); hBrush = NULL; }
                EndDialog(hwndDlg, LOWORD(wParam));
                return TRUE;
            }
            break;
        case WM_DESTROY:
            if (hLinkFont) { DeleteObject(hLinkFont); hLinkFont = NULL; }
            if (hBrush) { DeleteObject(hBrush); hBrush = NULL; }
            break;
    }
    return FALSE;
}

void UpdateVolumeFromSlider(void) {
    if (g_hVolumeSlider) {
        int pos = (int)SendMessage(g_hVolumeSlider, TBM_GETPOS, 0, 0);
        currentVolume = pos;
        if (hWaveOut) {
            DWORD volume = (currentVolume * 0xFFFF) / 1000;
            waveOutSetVolume(hWaveOut, volume | (volume << 16));
        }
        SaveVolumeToRegistry();
        if (g_hVolumeLabel) {
            WCHAR volStr[16];
            swprintf(volStr, 16, L"%d%%", (currentVolume * 100) / 1000);
            SetWindowTextW(g_hVolumeLabel, volStr);
        }
    }
}

LRESULT CALLBACK VolumeWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            g_hVolumeWnd = hwnd;
            g_hVolumeSlider = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
                                             WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS | TBS_TOOLTIPS,
                                             55, 10, 270, 30,
                                             hwnd, (HMENU)IDC_VOLUME_SLIDER, GetModuleHandle(NULL), NULL);
            g_hVolumeLabel = CreateWindowExW(0, L"STATIC", L"",
                                            WS_CHILD | WS_VISIBLE | SS_CENTER,
                                            160, 40, 80, 20,
                                            hwnd, (HMENU)IDC_VOLUME_LABEL, GetModuleHandle(NULL), NULL);
            if (g_hVolumeSlider) {
                SendMessage(g_hVolumeSlider, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 1000));
                SendMessage(g_hVolumeSlider, TBM_SETPAGESIZE, 0, (LPARAM)100);
                SendMessage(g_hVolumeSlider, TBM_SETTICFREQ, 100, 0);
                SendMessage(g_hVolumeSlider, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)currentVolume);
            }
            if (g_hVolumeLabel) {
                WCHAR volStr[16];
                swprintf(volStr, 16, L"%d%%", (currentVolume * 100) / 1000);
                SetWindowTextW(g_hVolumeLabel, volStr);
            }
            break;
        }
        case WM_HSCROLL: {
            if ((HWND)lParam == g_hVolumeSlider) {
                switch(LOWORD(wParam)) {
                    case TB_THUMBTRACK:
                    case TB_PAGEUP:
                    case TB_PAGEDOWN:
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
            g_hVolumeLabel = NULL;
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
    HWND hPopup = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, szVolumeClassName, g_lang->menu_volume,
                                 WS_POPUP | WS_BORDER | WS_CAPTION,
                                 pt.x - 200, pt.y - 45, 400, 90,
                                 hwndOwner, NULL, GetModuleHandle(NULL), NULL);
    if (hPopup) {
        ShowWindow(hPopup, SW_SHOW);
        SetForegroundWindow(hPopup);
    }
}

LRESULT CALLBACK SeekWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            g_hSeekWnd = hwnd;

            g_hSeekSlider = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ,
                                           55, 10, 270, 30, hwnd, (HMENU)IDC_SEEK_SLIDER, GetModuleHandle(NULL), NULL);

            g_hTimeLabelCurrent = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                                 10, 15, 45, 20, hwnd, (HMENU)IDC_TIME_LABEL_CURRENT, GetModuleHandle(NULL), NULL);

            g_hTimeLabelTotal = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                                               330, 15, 45, 20, hwnd, (HMENU)IDC_TIME_LABEL_TOTAL, GetModuleHandle(NULL), NULL);

            if (g_hSeekSlider) {
                SendMessage(g_hSeekSlider, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, totalDuration / 1000));
                SendMessage(g_hSeekSlider, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(currentPosition / 1000));
            }

            SetTimer(hwnd, IDT_SEEK_TIMER, 500, NULL);
            PostMessage(hwnd, WM_TIMER, IDT_SEEK_TIMER, 0);

            break;
        }

        case WM_TIMER: {
            if (wParam == IDT_SEEK_TIMER && !isSeeking) {
                if (g_hSeekSlider) {
                    SendMessage(g_hSeekSlider, TBM_SETPOS, TRUE, currentPosition / 1000);
                }

                WCHAR timeStr[16];
                DWORD currentSec = currentPosition / 1000;
                swprintf(timeStr, 16, L"%02d:%02d", currentSec / 60, currentSec % 60);
                if (g_hTimeLabelCurrent) SetWindowTextW(g_hTimeLabelCurrent, timeStr);

                DWORD totalSec = totalDuration / 1000;
                swprintf(timeStr, 16, L"%02d:%02d", totalSec / 60, totalSec % 60);
                if (g_hTimeLabelTotal) SetWindowTextW(g_hTimeLabelTotal, timeStr);
            }
            break;
        }

        case WM_HSCROLL:
            if ((HWND)lParam == g_hSeekSlider) {
                switch(LOWORD(wParam)) {
                    case TB_THUMBTRACK:
                    case TB_PAGEUP:
                    case TB_PAGEDOWN: {
                        isSeeking = 1;
                        DWORD newPosSec = SendMessage(g_hSeekSlider, TBM_GETPOS, 0, 0);
                        WCHAR timeStr[16];
                        swprintf(timeStr, 16, L"%02d:%02d", newPosSec / 60, newPosSec % 60);
                        if (g_hTimeLabelCurrent) SetWindowTextW(g_hTimeLabelCurrent, timeStr);
                        break;
                    }
                    case TB_ENDTRACK:
                        isSeeking = 0;
                        if (hWaveOut && totalDuration > 0) {
                            DWORD newPosSec = SendMessage(g_hSeekSlider, TBM_GETPOS, 0, 0);
                            uint64_t target_sample = (uint64_t)newPosSec * mp3d.info.hz * mp3d.info.channels;

                            waveOutReset(hWaveOut);
                            mp3dec_ex_seek(&mp3d, target_sample);
                            currentPosition = newPosSec * 1000;

                            if (isPlaying) {
                                FillAudioBuffers(GetForegroundWindow());
                            }
                        }
                        break;
                }
            }
            break;

        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE) {
                DestroyWindow(hwnd);
            }
            break;

        case WM_DESTROY:
            KillTimer(hwnd, IDT_SEEK_TIMER);
            g_hSeekWnd = NULL;
            g_hSeekSlider = NULL;
            g_hTimeLabelCurrent = NULL;
            g_hTimeLabelTotal = NULL;
            isSeeking = 0;
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void ShowSeekControl(HWND hwndOwner) {
    if (g_hSeekWnd && IsWindow(g_hSeekWnd)) {
        SetForegroundWindow(g_hSeekWnd);
        return;
    }

    if (totalDuration == 0) return;

    POINT pt;
    GetCursorPos(&pt);
    HWND hPopup = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, szSeekClassName, g_lang->menu_position,
                                 WS_POPUP | WS_BORDER | WS_CAPTION,
                                 pt.x - 200, pt.y - 45, 400, 90,
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
            AppendMenuW(g_hMenu, MF_STRING | (mp3File[0] == '\0' || totalDuration == 0 ? MF_GRAYED : MF_ENABLED), IDM_POSITION, g_lang->menu_position);
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
            LoadAutoStartStateFromRegistry();
            RefreshMenuText();
            UpdateTooltip(GetForegroundWindow());

            break;

        case WM_TIMER:
            if (wParam == IDT_TIMER && isPlaying) {
                UpdateTooltip(hwnd);
            }
            break;

        case WM_USER + 1:
            // Buffer finished playing, fill it again
            FillAudioBuffers(hwnd);
            break;

        case IDM_TRAY:
            if (lParam == WM_LBUTTONDOWN) {
                 if (strlen(mp3File) == 0) {
                     SelectMP3File(hwnd);
                 } else {
                     if (isPlaying) PauseMP3(hwnd); else PlayMP3(hwnd);
                 }
            } else if (lParam == WM_RBUTTONDOWN) {
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hwnd);
                int cmd = TrackPopupMenu(g_hMenu, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, NULL);
                switch (cmd) {
                    case IDM_PLAYPAUSE:
                        if (strlen(mp3File) == 0) SelectMP3File(hwnd);
                        else if (isPlaying) PauseMP3(hwnd);
                        else PlayMP3(hwnd);
                        break;
                    case IDM_STOP: StopMP3(hwnd); break;
                    case IDM_OPEN: SelectMP3File(hwnd); break;
                    case IDM_REPLAY: ToggleReplay(hwnd); break;
                    case IDM_NEXT: LoadNextTrack(hwnd); break;
                    case IDM_PREV: LoadPrevTrack(hwnd); break;
                    case IDM_VOLUME: ShowVolumeControl(hwnd); break;
                    case IDM_POSITION: ShowSeekControl(hwnd); break;
                    case IDM_ABOUT: ShowAboutDialog(hwnd); break;
                    case IDM_EXIT:
                        CleanupMP3();
                        Shell_NotifyIcon(NIM_DELETE, &nid);
                        DestroyMenu(g_hMenu);
                        DestroyMenu(g_hLangMenu);
                        DestroyIcon(hPlayIcon);
                        DestroyIcon(hPauseIcon);
                        PostQuitMessage(0);
                        break;
                    case ID_MENU_LANG_EN: SetLanguage(&lang_en); SaveLanguageSelectionToRegistry(); break;
                    case ID_MENU_LANG_HU: SetLanguage(&lang_hu); SaveLanguageSelectionToRegistry(); break;
                    case ID_MENU_LANG_DE: SetLanguage(&lang_de); SaveLanguageSelectionToRegistry(); break;
                    case ID_MENU_LANG_IT: SetLanguage(&lang_it); SaveLanguageSelectionToRegistry(); break;
                    case ID_MENU_LANG_ES: SetLanguage(&lang_es); SaveLanguageSelectionToRegistry(); break;
                    case ID_MENU_LANG_FR: SetLanguage(&lang_fr); SaveLanguageSelectionToRegistry(); break;
                    case ID_MENU_LANG_RU: SetLanguage(&lang_ru); SaveLanguageSelectionToRegistry(); break;
                    case IDM_AUTOSTART:
                        autoStart = !autoStart;
                        SaveAutoStartStateToRegistry();
                        RefreshMenuText();
                        break;
                }
            }
            break;

        case WM_DESTROY:
            CleanupMP3();
            Shell_NotifyIcon(NIM_DELETE, &nid);
            DestroyMenu(g_hMenu);
            DestroyMenu(g_hLangMenu);
            DestroyIcon(hPlayIcon);
            DestroyIcon(hPauseIcon);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
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

    WNDCLASSW wcSeek = {0};
    wcSeek.lpfnWndProc = SeekWndProc;
    wcSeek.hInstance = hInstance;
    wcSeek.lpszClassName = szSeekClassName;
    wcSeek.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcSeek.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    RegisterClassW(&wcSeek);

    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = szClassName;
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(szClassName, L"TrayMp3", WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, NULL, NULL, hInstance, NULL);
    if (hwnd == NULL) {
        MessageBoxW(NULL, L"Window creation failed!", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return msg.wParam;
}
