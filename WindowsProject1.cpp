/*
    项目名称：高级任务调度器 (最终版 - 界面底色修复)

    修复详情：
    1. [UI 修复] System Log & Data Visualization 底色变白:
       - 问题原因: Read-Only Edit 控件默认走 WM_CTLCOLORSTATIC 消息，此前被统一设为了灰色。
       - 解决方案: 在 WM_CTLCOLORSTATIC 中增加 ID 判断，专门为这两个控件返回白色画刷 (WHITE_BRUSH)。
    2. [功能保持]
       - 所有的对齐、加粗、边框、真实备份、死锁演示功能均保持不变。
*/

// ==========================================
// 1. 链接库与 Manifest 配置
// ==========================================
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(linker, "/SUBSYSTEM:WINDOWS")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")

#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h> 
#include <string>
#include <vector>
#include <list>
#include <mutex>
#include <thread>
#include <fstream>
#include <memory>
#include <chrono>
#include <ctime>
#include <sstream>
#include <random>
#include <iomanip>
#include <algorithm>
#include <stdexcept> 

using namespace std;

// ==========================================
// 全局常量与 ID
// ==========================================
#define WM_UPDATE_LOG (WM_USER + 1)
#define WM_UPDATE_LIST (WM_USER + 2)
#define WM_UPDATE_DATA (WM_USER + 3)

enum {
    ID_BTN_A = 101, ID_BTN_B, ID_BTN_C, ID_BTN_D, ID_BTN_E,
    ID_BTN_F,
    ID_BTN_RESET,
    ID_BTN_REVOKE,
    ID_BTN_CLEAR_ALL,
    ID_BTN_CLEAR_LOG,
    ID_BTN_CLEAR_DATA,
    ID_EDIT_LOG,
    ID_EDIT_DATA,
    ID_LIST_TASKS
};

HWND hGlobalWnd = NULL;
HFONT hFontUI = NULL;
HFONT hFontBold = NULL;
HFONT hFontLog = NULL;
HBRUSH hBrushSys = NULL;

// ==========================================
// 日志与数据系统
// ==========================================
class LogWriter {
    ofstream logFile;
    mutex logMutex;
    LogWriter() {
        logFile.open("scheduler.log", ios::app);
    }
public:
    static LogWriter& Instance() { static LogWriter i; return i; }
    ~LogWriter() { if (logFile.is_open()) logFile.close(); }

    void Write(const string& msg) {
        lock_guard<mutex> lock(logMutex);
        auto now = chrono::system_clock::to_time_t(chrono::system_clock::now());
        struct tm t; localtime_s(&t, &now);
        if (logFile.is_open()) {
            logFile << put_time(&t, "[%Y-%m-%d %H:%M:%S] ") << msg << endl;
        }
    }
};

void Log(const string& msg) {
    LogWriter::Instance().Write(msg);
    string* pMsg = new string(msg);
    if (hGlobalWnd) PostMessageA(hGlobalWnd, WM_UPDATE_LOG, 0, (LPARAM)pMsg);
}

void LogData(const string& data) {
    string* pMsg = new string(data);
    if (hGlobalWnd) PostMessageA(hGlobalWnd, WM_UPDATE_DATA, 0, (LPARAM)pMsg);
}

// ==========================================
// 任务系统接口
// ==========================================
class ITask {
public:
    virtual string GetName() const = 0;
    virtual void Execute() = 0;
    virtual ~ITask() = default;
};

// --- Task A: 真实文件备份 ---
class TaskBackup : public ITask {
public:
    string GetName() const override { return "Task A: File Backup"; }
    void Execute() override {
        auto now = chrono::system_clock::to_time_t(chrono::system_clock::now());
        struct tm t; localtime_s(&t, &now);
        stringstream ss; ss << "backup_" << put_time(&t, "%Y%m%d_%H%M%S") << ".zip";
        string zipName = ss.str();

        string srcDir = "C:\\Data";
        string destBase = "D:\\Backup";
        string destFile = destBase + "\\" + zipName;

        Log("A: Init backup sequence...");

        if (GetFileAttributesA(srcDir.c_str()) == INVALID_FILE_ATTRIBUTES) {
            Log("A: Creating C:\\Data source...");
            CreateDirectoryA(srcDir.c_str(), NULL);
            ofstream ofs(srcDir + "\\test_file.txt");
            ofs << "Test file for Project 3." << endl;
            ofs.close();
        }

        if (GetFileAttributesA(destBase.c_str()) == INVALID_FILE_ATTRIBUTES) {
            if (!CreateDirectoryA(destBase.c_str(), NULL)) {
                Log("A: D:\\Backup failed. Using C:\\Backup...");
                destBase = "C:\\Backup";
                destFile = destBase + "\\" + zipName;
                CreateDirectoryA(destBase.c_str(), NULL);
            }
        }

        Log("A: Zip -> " + destFile);

        string cmd = "powershell -WindowStyle Hidden -Command \"Compress-Archive -Path '" + srcDir + "\\*' -DestinationPath '" + destFile + "' -Force\"";
        UINT res = WinExec(cmd.c_str(), SW_HIDE);

        if (res > 31) {
            this_thread::sleep_for(chrono::seconds(2));
            if (GetFileAttributesA(destFile.c_str()) != INVALID_FILE_ATTRIBUTES) {
                Log("A: Success! File saved.");
            }
            else {
                Log("A: Cmd sent, verify manually.");
            }
        }
        else {
            Log("A: Failed to launch PowerShell.");
        }
    }
};

// --- Task B: Matrix Calc ---
class TaskMatrix : public ITask {
    int runCount = 0;
public:
    string GetName() const override { return "Task B: Matrix Calc (200x200)"; }
    void Execute() override {
        Log("B: Generating Matrix...");

        auto start = chrono::high_resolution_clock::now();
        int N = 200;
        vector<vector<double>> A(N, vector<double>(N));

        for (int i = 0; i < N; ++i)
            for (int j = 0; j < N; ++j) {
                A[i][j] = (double)(rand() % 100) / 10.0;
            }

        // --- 数据大屏输出 ---
        stringstream ss;
        ss << "\r\n";

        string sep(67, '=');
        ss << sep << "\r\n";
        ss << " TASK B: MATRIX PREVIEW (Top-Left 10x10 Block) - Iteration: " << runCount++ << "\r\n";
        ss << sep << "\r\n";

        ss << "      ";
        for (int j = 0; j < 10; ++j) ss << "+-----";
        ss << "+\r\n";

        for (int i = 0; i < 10; ++i) {
            ss << " R" << setw(2) << i << "  ";
            for (int j = 0; j < 10; ++j) {
                ss << "|" << fixed << setprecision(1) << setw(5) << A[i][j];
            }
            ss << "|\r\n";
            ss << "      ";
            for (int j = 0; j < 10; ++j) ss << "+-----";
            ss << "+\r\n";
        }
        ss << " (... 200x200 Full Data Hidden ...)\r\n";
        LogData(ss.str());

        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> elapsed = end - start;

        stringstream logMsg;
        logMsg << "B: Calc finished in " << fixed << setprecision(2) << elapsed.count() << " ms.";
        Log(logMsg.str());
    }
};

// --- Task C: HTTP ---
class TaskHttp : public ITask {
public:
    string GetName() const override { return "Task C: HTTP GET"; }
    void Execute() override {
        Log("C: Requesting data...");
        this_thread::sleep_for(chrono::milliseconds(800));
        Log("C: Data received.");
    }
};

// --- Task D: Reminder ---
class TaskReminder : public ITask {
public:
    string GetName() const override { return "Task D: Reminder"; }
    void Execute() override {
        thread([]() {
            MessageBoxA(hGlobalWnd, "休息 5 分钟", "课堂提醒", MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
            }).detach();
        Log("D: Popup displayed.");
    }
};

// --- Task E: Stats ---
class TaskStats : public ITask {
public:
    string GetName() const override { return "Task E: Random Stats"; }
    void Execute() override {
        Log("E: Generating 1000 numbers...");

        random_device rd; mt19937 gen(rd());
        uniform_int_distribution<> dis(0, 100);
        vector<int> nums;
        double sum = 0;

        stringstream ss;
        ss << "\r\n";

        string sep(84, '=');
        ss << sep << "\r\n";
        ss << " TASK E: DATA MATRIX (20 Columns x 50 Rows)\r\n";
        ss << sep << "\r\n";

        string lineBorder = "+" + string(82, '-') + "+";
        ss << lineBorder << "\r\n";

        for (int i = 0; i < 1000; ++i) {
            int n = dis(gen);
            nums.push_back(n);
            sum += n;

            if (i % 20 == 0) ss << "| ";
            ss << setw(3) << n << " ";
            if ((i + 1) % 20 == 0) ss << " |\r\n";
        }
        ss << lineBorder << "\r\n";

        double mean = sum / 1000.0;
        double variance = 0;
        for (int n : nums) variance += (n - mean) * (n - mean);
        variance /= 1000.0;

        ss << "  [STATISTICS REPORT]\r\n";
        ss << "  > Count:    1000\r\n";
        ss << "  > Mean:     " << fixed << setprecision(2) << mean << "\r\n";
        ss << "  > Variance: " << variance << "\r\n";

        LogData(ss.str());
        Log("E: Stats computed (See Data Board).");
    }
};

// --- Task F: Chaos ---
class TaskChaos : public ITask {
public:
    string GetName() const override { return "Task F: CHAOS (FREEZE)"; }
    void Execute() override {
        Log("F: Critical Error! Throwing Exception...");
        throw runtime_error("CORE DUMP: MEMORY VIOLATION");
    }
};

// ==========================================
// 调度器
// ==========================================
struct ScheduledTask {
    int id = 0;
    shared_ptr<ITask> task = nullptr;
    chrono::system_clock::time_point runTime = {};
    bool isPeriodic = false;
    chrono::milliseconds interval = chrono::milliseconds(0);

    string GetTimeStr() {
        auto t = chrono::system_clock::to_time_t(runTime);
        struct tm tmInfo; localtime_s(&tmInfo, &t);
        stringstream ss; ss << put_time(&tmInfo, "%H:%M:%S");
        return ss.str();
    }
};

class TaskScheduler {
    list<shared_ptr<ScheduledTask>> taskList;
    mutex listMutex;
    condition_variable cv;
    bool running = true;
    thread workerThread;
    int nextId = 1;
    bool isFrozen = false;

    TaskScheduler() { workerThread = thread(&TaskScheduler::WorkerLoop, this); }

    void RefreshUI() { if (hGlobalWnd) PostMessageA(hGlobalWnd, WM_UPDATE_LIST, 0, 0); }

    void WorkerLoop() {
        while (running) {
            shared_ptr<ScheduledTask> currentTask = nullptr;

            {
                unique_lock<mutex> lock(listMutex);
                cv.wait(lock, [this] { return (!taskList.empty() && !isFrozen) || !running; });

                if (!running) { lock.unlock(); break; }
                if (isFrozen) continue;

                auto it = min_element(taskList.begin(), taskList.end(),
                    [](const auto& a, const auto& b) { return a->runTime < b->runTime; });

                if (it != taskList.end()) {
                    auto now = chrono::system_clock::now();
                    if ((*it)->runTime <= now) {
                        currentTask = *it;
                        taskList.erase(it);
                    }
                    else {
                        cv.wait_until(lock, (*it)->runTime);
                        continue;
                    }
                }
            }

            if (currentTask) {
                RefreshUI();
                try {
                    currentTask->task->Execute();
                }
                catch (const std::exception& e) {
                    string err = "SYSTEM FAILURE: " + string(e.what());
                    Log(err);
                    Log("!!! SYSTEM FROZEN !!!");

                    { lock_guard<mutex> lock(listMutex); isFrozen = true; }
                    {
                        unique_lock<mutex> lock(listMutex);
                        cv.wait(lock, [this] { return !isFrozen || !running; });
                    }
                    Log(">>> SYSTEM RECOVERED. <<<");
                }

                if (currentTask->isPeriodic && running && !isFrozen) {
                    currentTask->runTime = chrono::system_clock::now() + currentTask->interval;
                    { lock_guard<mutex> lock(listMutex); taskList.push_back(currentTask); }
                    Log("Rescheduled: " + currentTask->task->GetName());
                    cv.notify_one();
                    RefreshUI();
                }
            }
        }
    }

public:
    static TaskScheduler& Instance() { static TaskScheduler i; return i; }
    ~TaskScheduler() { Stop(); }

    void Stop() {
        { lock_guard<mutex> lock(listMutex); running = false; isFrozen = false; }
        cv.notify_all();
        if (workerThread.joinable()) workerThread.join();
    }

    void UnfreezeSystem() {
        {
            lock_guard<mutex> lock(listMutex);
            if (!isFrozen) { Log("System normal."); return; }
            isFrozen = false;
        }
        Log("-> RESET Signal Received.");
        cv.notify_all();
    }

    void AddTask(shared_ptr<ITask> task, int delayMs, int intervalMs = 0) {
        auto st = make_shared<ScheduledTask>();
        st->id = nextId++;
        st->task = task;
        st->runTime = chrono::system_clock::now() + chrono::milliseconds(delayMs);
        st->interval = chrono::milliseconds(intervalMs);
        st->isPeriodic = (intervalMs > 0);
        { lock_guard<mutex> lock(listMutex); taskList.push_back(st); }
        cv.notify_one();
        Log("Added: " + task->GetName());
        RefreshUI();
    }

    void RevokeTask(int taskId) {
        lock_guard<mutex> lock(listMutex);
        auto it = find_if(taskList.begin(), taskList.end(), [taskId](const auto& t) { return t->id == taskId; });
        if (it != taskList.end()) {
            string name = (*it)->task->GetName();
            taskList.erase(it);
            Log("Revoked: " + name);
            RefreshUI();
        }
    }

    void ClearAllTasks() {
        {
            lock_guard<mutex> lock(listMutex);
            taskList.clear();
        }
        Log("Queue cleared (All pending tasks removed).");
        RefreshUI();
    }

    vector<pair<int, string>> GetPendingTasks() {
        lock_guard<mutex> lock(listMutex);
        vector<pair<int, string>> res;
        for (const auto& t : taskList) {
            stringstream ss;
            ss << "[" << t->GetTimeStr() << "] " << t->task->GetName();
            if (t->isPeriodic) ss << " (Loop)";
            res.push_back({ t->id, ss.str() });
        }
        return res;
    }
};

class TaskFactory {
public:
    static shared_ptr<ITask> CreateTask(int id) {
        switch (id) {
        case ID_BTN_A: return make_shared<TaskBackup>();
        case ID_BTN_B: return make_shared<TaskMatrix>();
        case ID_BTN_C: return make_shared<TaskHttp>();
        case ID_BTN_D: return make_shared<TaskReminder>();
        case ID_BTN_E: return make_shared<TaskStats>();
        case ID_BTN_F: return make_shared<TaskChaos>();
        default: return nullptr;
        }
    }
};

// ==========================================
// UI Logic
// ==========================================
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static HWND hListTasks, hEditLog, hEditData;

    switch (message) {
    case WM_CREATE:
    {
        hBrushSys = GetSysColorBrush(COLOR_BTNFACE);

        hFontUI = CreateFontA(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
        hFontBold = CreateFontA(19, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
        hFontLog = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_MODERN, "Consolas");

        int padding = 15;

        int col1_x = padding;
        int col1_w = 220;

        int col2_x = col1_x + col1_w + padding;
        int col2_w = 400;

        int col3_x = col2_x + col2_w + padding;
        int col3_w = 640;

        // --- 高度计算 ---
        int startY = padding;
        int btnH = 40;
        int gap = 10;
        int titleH = 25;

        int Y_BOTTOM = 640;

        int rightEditTop = startY + titleH + 5;
        int rightEditH = Y_BOTTOM - rightEditTop;

        int midFixedOverhead = (titleH + 5) + (10 + btnH + 15) + (titleH + 5);
        int availableH = Y_BOTTOM - startY - midFixedOverhead;
        int hQueue = availableH / 2;
        int hLog = availableH - hQueue;

        // === 第一列：控制区 ===
        int curY = startY;

        // 1. Control Panel GroupBox
        int cpHeight = 275;
        HWND hGrpCP = CreateWindowA("BUTTON", "Task Control Panel", WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
            col1_x, curY, col1_w, cpHeight, hWnd, NULL, NULL, NULL);
        SendMessage(hGrpCP, WM_SETFONT, (WPARAM)hFontBold, TRUE);

        curY += 25;

        auto Btn = [&](const char* txt, int id, int x, int w) {
            HWND hBtn = CreateWindowA("BUTTON", txt, WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, x, curY, w, btnH, hWnd, (HMENU)(UINT_PTR)id, NULL, NULL);
            SendMessage(hBtn, WM_SETFONT, (WPARAM)hFontUI, TRUE);
            curY += btnH + gap;
            };

        Btn("Task A: Backup (Disk)", ID_BTN_A, col1_x + 10, col1_w - 20);
        Btn("Task B: Matrix (Visual)", ID_BTN_B, col1_x + 10, col1_w - 20);
        Btn("Task C: HTTP GET", ID_BTN_C, col1_x + 10, col1_w - 20);
        Btn("Task D: Reminder", ID_BTN_D, col1_x + 10, col1_w - 20);
        Btn("Task E: Stats (Visual)", ID_BTN_E, col1_x + 10, col1_w - 20);

        curY = startY + cpHeight + 10;

        // System Tools GroupBox
        HWND hGrp = CreateWindowA("BUTTON", "System Tools", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, col1_x, curY, col1_w, 110, hWnd, NULL, NULL, NULL);
        SendMessage(hGrp, WM_SETFONT, (WPARAM)hFontBold, TRUE);

        HWND hBtnChaos = CreateWindowA("BUTTON", "FREEZE", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, col1_x + 10, curY + 25, 90, 60, hWnd, (HMENU)(UINT_PTR)ID_BTN_F, NULL, NULL);
        SendMessage(hBtnChaos, WM_SETFONT, (WPARAM)hFontUI, TRUE);

        HWND hBtnReset = CreateWindowA("BUTTON", "RESET", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, col1_x + 110, curY + 25, 90, 60, hWnd, (HMENU)(UINT_PTR)ID_BTN_RESET, NULL, NULL);
        SendMessage(hBtnReset, WM_SETFONT, (WPARAM)hFontUI, TRUE);

        // === 第二列：中间 ===
        int midY = startY;

        HWND hTitle2 = CreateWindowA("STATIC", "Task Queue", WS_VISIBLE | WS_CHILD | SS_LEFT, col2_x, midY, col2_w, titleH, hWnd, NULL, NULL, NULL);
        SendMessage(hTitle2, WM_SETFONT, (WPARAM)hFontBold, TRUE);
        midY += titleH + 5;

        hListTasks = CreateWindowA("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
            col2_x, midY, col2_w, hQueue, hWnd, (HMENU)(UINT_PTR)ID_LIST_TASKS, NULL, NULL);
        SendMessage(hListTasks, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        midY += hQueue + gap;

        int smallBtnW = (col2_w - 10) / 2;

        HWND hBtnRevoke = CreateWindowA("BUTTON", "Revoke Selected", WS_VISIBLE | WS_CHILD,
            col2_x, midY, smallBtnW, btnH, hWnd, (HMENU)(UINT_PTR)ID_BTN_REVOKE, NULL, NULL);
        SendMessage(hBtnRevoke, WM_SETFONT, (WPARAM)hFontUI, TRUE);

        HWND hBtnClearAll = CreateWindowA("BUTTON", "Clear Queue", WS_VISIBLE | WS_CHILD,
            col2_x + smallBtnW + 10, midY, smallBtnW, btnH, hWnd, (HMENU)(UINT_PTR)ID_BTN_CLEAR_ALL, NULL, NULL);
        SendMessage(hBtnClearAll, WM_SETFONT, (WPARAM)hFontUI, TRUE);

        midY += btnH + 15;

        HWND hTitle3 = CreateWindowA("STATIC", "System Log (Status)", WS_VISIBLE | WS_CHILD | SS_LEFT, col2_x, midY, col2_w - 60, titleH, hWnd, NULL, NULL, NULL);
        SendMessage(hTitle3, WM_SETFONT, (WPARAM)hFontBold, TRUE);

        int clearBtnW = 60;
        int clearBtnH = 22;
        HWND hBtnClrLog = CreateWindowA("BUTTON", "Clear", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            col2_x + col2_w - clearBtnW, midY, clearBtnW, clearBtnH, hWnd, (HMENU)(UINT_PTR)ID_BTN_CLEAR_LOG, NULL, NULL);
        SendMessage(hBtnClrLog, WM_SETFONT, (WPARAM)hFontUI, TRUE);

        midY += titleH + 5;

        hEditLog = CreateWindowA("EDIT", "",
            WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
            col2_x, midY, col2_w, hLog, hWnd, (HMENU)(UINT_PTR)ID_EDIT_LOG, NULL, NULL);
        SendMessage(hEditLog, WM_SETFONT, (WPARAM)hFontLog, TRUE);

        // === 第三列：右侧 ===
        int rightY = startY;

        HWND hTitle4 = CreateWindowA("STATIC", "Data Visualization Board", WS_VISIBLE | WS_CHILD | SS_LEFT, col3_x, rightY, col3_w - 60, titleH, hWnd, NULL, NULL, NULL);
        SendMessage(hTitle4, WM_SETFONT, (WPARAM)hFontBold, TRUE);

        HWND hBtnClrData = CreateWindowA("BUTTON", "Clear", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            col3_x + col3_w - clearBtnW, rightY, clearBtnW, clearBtnH, hWnd, (HMENU)(UINT_PTR)ID_BTN_CLEAR_DATA, NULL, NULL);
        SendMessage(hBtnClrData, WM_SETFONT, (WPARAM)hFontUI, TRUE);

        rightY += titleH + 5;

        hEditData = CreateWindowA("EDIT", "Waiting for data tasks (B or E)...\r\n",
            WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
            col3_x, rightY, col3_w, rightEditH, hWnd, (HMENU)(UINT_PTR)ID_EDIT_DATA, NULL, NULL);
        SendMessage(hEditData, WM_SETFONT, (WPARAM)hFontLog, TRUE);
        SendMessageA(hEditData, EM_SETLIMITTEXT, 0x7FFFFFFF, 0);
    }
    break;

    case WM_CTLCOLORSTATIC:
    {
        HDC hdc = (HDC)wParam;
        int id = GetDlgCtrlID((HWND)lParam);

        // [核心修复] 给只读编辑框 ID_EDIT_LOG 和 ID_EDIT_DATA 返回白色背景
        if (id == ID_EDIT_LOG || id == ID_EDIT_DATA) {
            SetBkColor(hdc, RGB(255, 255, 255));
            return (INT_PTR)GetStockObject(WHITE_BRUSH);
        }

        // Label 和 GroupBox 保持透明
        SetBkMode(hdc, TRANSPARENT);
        return (INT_PTR)hBrushSys;
    }

    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLOREDIT:
    {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, OPAQUE);
        return (INT_PTR)GetStockObject(WHITE_BRUSH);
    }

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);

        if (id == ID_BTN_CLEAR_LOG) {
            SetWindowTextA(hEditLog, "");
            return 0;
        }
        if (id == ID_BTN_CLEAR_DATA) {
            SetWindowTextA(hEditData, "Waiting for data tasks (B or E)...\r\n");
            return 0;
        }

        if (id == ID_BTN_RESET) {
            TaskScheduler::Instance().UnfreezeSystem();
            return 0;
        }

        if (id == ID_BTN_REVOKE) {
            int sel = (int)SendMessageA(hListTasks, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                int tid = (int)SendMessageA(hListTasks, LB_GETITEMDATA, sel, 0);
                TaskScheduler::Instance().RevokeTask(tid);
            }
            return 0;
        }

        if (id == ID_BTN_CLEAR_ALL) {
            TaskScheduler::Instance().ClearAllTasks();
            return 0;
        }

        auto task = TaskFactory::CreateTask(id);
        if (task) {
            int delay = 0, interval = 0;
            switch (id) {
            case ID_BTN_A: delay = 1000; break;
            case ID_BTN_B: interval = 5000; break;
            case ID_BTN_C: delay = 0; break;
            case ID_BTN_D: interval = 60000; break;
            case ID_BTN_E: delay = 5000; break;
            case ID_BTN_F: delay = 500; break;
            }
            TaskScheduler::Instance().AddTask(task, delay, interval);
        }
    }
    break;

    case WM_UPDATE_LOG:
    {
        string* s = (string*)lParam;
        if (s) {
            int len = GetWindowTextLengthA(hEditLog);
            SendMessageA(hEditLog, EM_SETSEL, (WPARAM)len, (LPARAM)len);
            SendMessageA(hEditLog, EM_REPLACESEL, 0, (LPARAM)s->c_str());
            SendMessageA(hEditLog, EM_REPLACESEL, 0, (LPARAM)"\r\n");
            delete s;
        }
    }
    break;

    case WM_UPDATE_DATA:
    {
        string* s = (string*)lParam;
        if (s) {
            int len = GetWindowTextLengthA(hEditData);
            SendMessageA(hEditData, EM_SETSEL, (WPARAM)len, (LPARAM)len);
            SendMessageA(hEditData, EM_REPLACESEL, 0, (LPARAM)s->c_str());
            SendMessage(hEditData, WM_VSCROLL, SB_BOTTOM, 0);
            delete s;
        }
    }
    break;

    case WM_UPDATE_LIST:
    {
        SendMessageA(hListTasks, LB_RESETCONTENT, 0, 0);
        auto list = TaskScheduler::Instance().GetPendingTasks();
        for (const auto& p : list) {
            int idx = (int)SendMessageA(hListTasks, LB_ADDSTRING, 0, (LPARAM)p.second.c_str());
            SendMessageA(hListTasks, LB_SETITEMDATA, idx, (LPARAM)p.first);
        }
    }
    break;

    case WM_DESTROY:
        DeleteObject(hFontUI);
        DeleteObject(hFontBold);
        DeleteObject(hFontLog);
        PostQuitMessage(0);
        break;
    }
    return DefWindowProcA(hWnd, message, wParam, lParam);
}

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    InitCommonControls();

    WNDCLASSEXA wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0, 0, hInstance, NULL, LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_BTNFACE + 1), NULL, "SchClass", NULL };

    RegisterClassExA(&wc);
    // 高度 760 (底部留空)
    hGlobalWnd = CreateWindowA("SchClass", "Project 3: Scheduler w/ Data Visualization Board", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        50, 50, 1350, 760, NULL, NULL, hInstance, NULL);

    ShowWindow(hGlobalWnd, SW_SHOW);
    UpdateWindow(hGlobalWnd);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return 0;
}