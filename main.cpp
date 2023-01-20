// Copyright (c) 2022. Cyan_Changes.
// @Author Cyan_Changes(mailto:lc_cyan@outlook.com)
// @License https://www.apache.org/licenses/LICENSE-2.0.html
// @Do not upload to CSDN or other site!

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <thread>
#include <codecvt>
#include <locale>
#include <exception>
#include <sstream>

int ProcessControl();
int FSClose();
void MySettings();
int stealThreadId(const std::thread::id&);

bool G_whileBack = true;                                // 自动返回主页
std::string G_targetProcessName = "StudentMain.exe";    // 默认目标程序

std::thread myth;

std::vector<std::wstring> G_targetTitle{L"BlackScreen Window", L"屏幕广播", L"共享屏幕"};

auto shouldExit = false;

int main() {
    // setlocale(LC_ALL, "zh_CN.UTF-8"); // Legacy C style setlocale
    std::locale::global(std::locale("zh_CN.utf8"));
    std::ios::sync_with_stdio(false);
    std::cout << "Initialize Locale... \nCheckout locale settings...\n";
    std::locale::global(std::locale("zh_CN.UTF-8"));
    std::cout << "std::locale " << std::locale().name() << "\n";
    std::cout << "std::cin/cout " << std::cin.getloc().name() << " / " << std::cin.getloc().name() << "\n";
    std::cout << "std::wcin/wcout " << std::wcin.getloc().name() << " / " << std::wcin.getloc().name() << "\n";
    menu:
    std::cout << "Welcome to the " << G_targetProcessName <<" Killer\n"; // Print the Menu
    // std::cout << "\t\t - Made in China!(Too lazy to write any Chinese(Abs not a gcc charset bug, MSVC yyds)^v^)\n";
    std::cout << "1.Process Control\n";
    std::cout << "2.Switch AWC(Auto Windows Close)\n";
    std::cout << "3.Settings\n";
    std::cout << "4.Exit\n";
    std::cout << "[Id] " << std::flush;
    short code;
    std::cin >> code; // 获取用户输入
    switch(code){
        case 1: // 进程 暂停 继续 终止
            ProcessControl();
            break;
        case 2: // 自动关闭全屏(测试)
            FSClose();
            break;
        case 3: // 设定页面
            MySettings();
            break;
        case 4: // 退出
            shouldExit = true;
            try{ // 尝试释放资源
                myth.detach(); // Release
            } catch(std::exception& e){ // 出现错误
                std::cout << "thread may not exist or it hang on: " << e.what();
                if (myth.joinable()){ // 如果可释放
                    std::cout << "Terminating...";
                    // 获取TERMINATE权限
                    HANDLE tHnd = OpenThread(THREAD_TERMINATE, FALSE, stealThreadId(myth.get_id()));
                    TerminateThread(tHnd, 0); // Terminate
                }

            }
            return EXIT_SUCCESS;
        default: // 选项不存在
            std::cout << "Wrong chose " << code << "! Please chose again: ";
            goto menu;

    }
    if (G_whileBack){ // 返回菜单
        std::cout << "Back to menu..." << std::endl;
        goto menu;
    } 
    return EXIT_SUCCESS;
}

// Convert string to wstring
inline std::wstring to_wide_string(const std::string& input)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(input);
}
// Convert wstring to string
inline std::string to_byte_string(const std::wstring& input)
{
    //std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(input);
}

int stealThreadId(const std::thread::id & id)
{
    std::stringstream sin;
    sin << id;
    return std::stoi(sin.str());
}

// 获取所有相同 进程名 的 PID
std::vector<DWORD> GetPIDsByName(LPCSTR ProcessName)
{
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    std::vector<DWORD> PIDs;

    // Take a snapshot of all processes in the system.
    // 获取当前系统所有进程的快照
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) // 检测句柄
    {
        return PIDs;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hProcessSnap, &pe32))
    {
        CloseHandle(hProcessSnap);              // clean the snapshot object 释放快照句柄
        return PIDs; // 返回空vec
    }

    do
    {
        if (!strcmp(ProcessName, pe32.szExeFile)) // 如果映像是目标进程
        {
            // 添加进程ID到vector
            PIDs.push_back(pe32.th32ProcessID);
        }

    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap); // 释放快照
    return PIDs;
}

// 进程控制 暂停 继续 终止
int ProcessControl() {
    menuPC:
    std::cout << "Operation 1.Suspend 2.Resume 3.Terminate\n";
    std::cout << "[Id] " << std::flush;
    short code = 0;
    std::cin >> code;
    // 一下为实现
    std::vector<DWORD> pids = GetPIDsByName(G_targetProcessName.data());
    for (auto &pid: pids) { // 枚举所有PID
        THREADENTRY32 th32{};
        HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, pid);
        if (hThreadSnap == INVALID_HANDLE_VALUE) { // 判断句柄无效
            return -1;
        }
        th32.dwSize = sizeof(THREADENTRY32);
        if (!Thread32First(hThreadSnap, &th32)) { // 获取第一个te头失败
            CloseHandle(hThreadSnap);
            return -1;
        }
        HANDLE ProcHnd = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE, FALSE, pid);
        switch (code) { // 分类分别 暂停 继续 终止 进程
            case 1: // 暂停进程
                do {
                    if (th32.th32OwnerProcessID == pid) {
                        HANDLE thhnd = OpenThread(THREAD_SUSPEND_RESUME, FALSE, th32.th32ThreadID);
                        std::cout << "Suspending " << th32.th32ThreadID << " of " << th32.th32OwnerProcessID << "... ";
                        if (SuspendThread(thhnd) == -1UL) {
                            std::cout << "fail!\n" << GetLastError();
                        } else { std::cout << "success!\n"; }
                        CloseHandle(thhnd);
                    }
                } while (Thread32Next(hThreadSnap, &th32));
                std::cout << "All Thread Suspended";
                break;
            case 2: // 继续进程
                do {
                    if (th32.th32OwnerProcessID == pid) {
                        HANDLE thhnd = OpenThread(THREAD_SUSPEND_RESUME, FALSE, th32.th32ThreadID);
                        std::cout << "Resuming " << th32.th32ThreadID << " of " << th32.th32OwnerProcessID << "... ";
                        if (ResumeThread(thhnd) == -1UL) {
                            std::cout << "fail!" << GetLastError() << "\n";
                        } else { std::cout << "success!\n"; }
                        CloseHandle(thhnd);
                    }
                } while (Thread32Next(hThreadSnap, &th32));
                std::cout << "All Thread resumed";
                break;
            case 3: // 终止进程
                do {
                    if (th32.th32OwnerProcessID == pid) {
                        HANDLE thhnd = OpenThread(THREAD_TERMINATE, FALSE, th32.th32ThreadID);
                        std::cout << "Terminating " << th32.th32ThreadID << " of " << th32.th32OwnerProcessID << "... ";
                        if (!TerminateThread(thhnd, EXIT_SUCCESS)) {
                            std::cout << "fail!" << GetLastError() << "\n";
                        } else { std::cout << "success!\n"; }
                        CloseHandle(thhnd);
                    }
                } while (Thread32Next(hThreadSnap, &th32));
                std::cout << "Terminating Process " << pid << "... ";
                if (TerminateProcess(ProcHnd, EXIT_SUCCESS)) {
                    std::cout << "success!";
                } else { std::cout << "fail!" << GetLastError() << "\n"; }
                break;
            default: // 错误的选项
                std::cout << "Invalid value: " << code << "!" << std::endl;
                goto menuPC;
        }
        CloseHandle(ProcHnd);
        CloseHandle(hThreadSnap);
        std::cout << std::endl;
    }
    if (pids.empty()){
        std::cout << "Process may not exist!" << std::endl;
    }
    return 0;
}

#define CHECKOUT_LENGTH 80

void LocateAndKillTh(){
    while (!shouldExit) {
        POINT point{};
        GetCursorPos(&point);
        HWND WndHnd = WindowFromPoint(point);
        auto *buffer = new wchar_t[CHECKOUT_LENGTH]{};
        GetWindowTextW(WndHnd, buffer, 20);
        for (const std::wstring &title: G_targetTitle) {
            if (title == buffer) {
                if (title ==L"BlackScreen Window"){
                    SetWindowPos(WndHnd, HWND_DESKTOP, 0, 0, 100, 100, SWP_NOACTIVATE | SWP_NOREDRAW | SWP_HIDEWINDOW);
                } else {
                    SetWindowPos(WndHnd, HWND_DESKTOP, 0, 0, 600, 400, SWP_NOACTIVATE | SWP_NOREDRAW);
                }
                CloseWindow(WndHnd);
            }
        }
    }
}

int FSClose(){
    std::cout << "- Start locating Windows...\n";
    if (!myth.joinable()){
        shouldExit = false;
        std::cout << " - Starting new thread..." << std::endl;
        myth = std::thread(LocateAndKillTh);
        // myth.join(); // enable, if you want hang on this console.
    } else {
        std::cout << " - Thread may already here... \n   Detaching..." << std::endl;
        try{
            shouldExit = true;
            myth.detach();
        } catch (std::exception& e){
            std::cout << " - Something went wrong: " << e.what() << ".\n   Terminating..." << std::endl;
            std::thread::id tid = myth.get_id();
            HANDLE tHnd = OpenThread(THREAD_TERMINATE, FALSE, stealThreadId(tid));
            TerminateThread(tHnd, 0);
        }

    }
    return 0;
}

void MySettings() { // 设定页
    menuMS:
    std::cout << "<== My Settings ==>\n";
    std::cout << "1.should automatic back to menu\n";
    std::cout << "2.the PC target process name\n";
    std::cout << "3.Checkout All Process!\n";
    std::cout << "4.Manager AWC Vector\n";
    std::cout << "5.Back to the Menu\n";
    std::cout << "[Id] ";
    short code;
    std::cin >> code;
    HANDLE th32_snap = nullptr;
    switch (code) {
        case 1:
            std::cout << "Original Value: " << G_whileBack << "\n";
            std::cout << "New Value:      ";
            std::cin >> G_whileBack;
            break;
        case 2:
            std::cout << "Original Value: " << G_targetProcessName << "\n";
            std::cout << "New Value:      ";
            std::cin >> G_targetProcessName;
            break;
        case 3:
            th32_snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            std::cout << "PID \t\t\t PName \t\t\t Ts\n";
            PROCESSENTRY32 pe32;
            Process32First(th32_snap, &pe32);
            do {
                std::cout << pe32.th32ProcessID << "\t\t\t" << pe32.szExeFile << "\t\t\t" << pe32.cntThreads << "\n";
            } while(Process32Next(th32_snap, &pe32));
            break;
        case 4:
            menuModvec:
            std::cout << "Original list: ";
            for (std::wstring &title : G_targetTitle){
                std::wcout << title << " ";
            }
            std::cout << std::endl << "1.Push back 2.Insert 3.Remove\n";
            std::cout << "[Id] ";
            {
                short code1;
                std::cin >> code1;
                std::wstring s;
                unsigned int point = 0;
                switch (code1) {
                    case 1:
                        std::cout << "Enter something in new line\n";
                        std::wcin >> s;
                        G_targetTitle.push_back(s);
                        break;
                    case 2:
                        std::cout << "Enter a position and enter something in new line\n";
                        std::wcin >> point >> s;
                        G_targetTitle.insert(G_targetTitle.begin() + point, s);
                        break;
                    case 3:
                        std::cout << "Enter a position in new line\n";
                        std::cin >> point;
                        if (G_targetTitle.begin() + point < G_targetTitle.end()) {
                            std::wstring tmp = G_targetTitle.at(point);
                            G_targetTitle.erase(G_targetTitle.begin() + point);
                            std::wcout << "Success to erase the " << tmp << " of vector";
                        } else {
                            std::cout << "Position out of vector";
                        }
                        break;
                    default:
                        goto menuModvec;
                }

            }
            break;
        case 5:
            std::cout << "...";
            return;
        default:
            std::cout << "Invalid value: " << code << "!" << std::endl;
            goto menuMS;
    }
    if (th32_snap) CloseHandle(th32_snap);
    std::cout << std::endl;
}
