// ------------------------- OpenPose Library Tutorial - Thread - Example 2 - User Input Processing And Output -------------------------
// This directory only provides examples for the basic OpenPose thread mechanism API, and it is only meant for people
// interested in the multi-thread architecture without been interested in the OpenPose pose estimation algorithm.
// You are most probably looking for the [examples/tutorial_api_cpp/](../tutorial_api_cpp/) or
// [examples/tutorial_api_python/](../tutorial_api_python/), which provide examples of the thread API already applied
// to body pose estimation.

// Third-party dependencies
#include <opencv2/opencv.hpp>
// GFlags: DEFINE_bool, _int32, _int64, _uint64, _double, _string
#include <gflags/gflags.h>
// Allow Google Flags in Ubuntu 14
#ifndef GFLAGS_GFLAGS_H_
    namespace gflags = google;
#endif
// OpenPose dependencies
#include <openpose/headers.hpp>

// See all the available parameter options withe the `--help` flag. E.g., `build/examples/openpose/openpose.bin --help`
// Note: This command will show you flags for other unnecessary 3rdparty files. Check only the flags for the OpenPose
// executable. E.g., for `openpose.bin`, look for `Flags from examples/openpose/openpose.cpp:`.
// Debugging/Other
DEFINE_int32(logging_level,             3,              "The logging level. Integer in the range [0, 255]. 0 will output any opLog() message,"
                                                        " while 255 will not output any. Current OpenPose library messages are in the range 0-4:"
                                                        " 1 for low priority messages and 4 for important ones.");
// Producer
DEFINE_string(image_dir,                "examples/media/",      "Process a directory of images. Read all standard formats (jpg, png, bmp, etc.).");
// Consumer
DEFINE_bool(fullscreen,                 false,          "Run in full-screen mode (press f during runtime to toggle).");


#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <conio.h>

using cv::Mat;
using namespace std;



HHOOK keyboardHook = 0;		// 钩子句柄、
bool worked = false;



class Screenshot
{
public:
	Screenshot();
	double static getZoom();
	cv::Mat getScreenshot();
	cv::Mat getScreenshot(int x, int y, int width, int height);

private:
	int m_width;
	int m_height;
	HDC m_screenDC;
	HDC m_compatibleDC;
	HBITMAP m_hBitmap;
	LPVOID m_screenshotData = nullptr;
};

Screenshot::Screenshot()
{
	double zoom = getZoom();
	m_width = GetSystemMetrics(SM_CXSCREEN) * zoom;
	m_height = GetSystemMetrics(SM_CYSCREEN) * zoom;
	m_screenshotData = new char[m_width * m_height * 4];
	memset(m_screenshotData, 0, m_width);

	// 获取屏幕 DC
	m_screenDC = GetDC(NULL);
	m_compatibleDC = CreateCompatibleDC(m_screenDC);

	// 创建位图
	m_hBitmap = CreateCompatibleBitmap(m_screenDC, m_width, m_height);
	SelectObject(m_compatibleDC, m_hBitmap);
}

/* 获取整个屏幕的截图 */
Mat Screenshot::getScreenshot()
{
	// 得到位图的数据
	BitBlt(m_compatibleDC, 0, 0, m_width, m_height, m_screenDC, 0, 0, SRCCOPY);
	GetBitmapBits(m_hBitmap, m_width * m_height * 4, m_screenshotData);

	// 创建图像
	Mat screenshot_(m_height, m_width, CV_8UC4, m_screenshotData);
	Mat screenshot;
	cv::cvtColor(screenshot_, screenshot, cv::COLOR_RGBA2RGB);
	return screenshot;
}

/** @brief 获取指定范围的屏幕截图
* @param x 图像左上角的 X 坐标
* @param y 图像左上角的 Y 坐标
* @param width 图像宽度
* @param height 图像高度
*/
Mat Screenshot::getScreenshot(int x, int y, int width, int height)
{
	Mat screenshot = getScreenshot();
	return screenshot(cv::Rect(x, y, width, height));
}

/* 获取屏幕缩放值 */
double Screenshot::getZoom()
{
	// 获取窗口当前显示的监视器
	HWND hWnd = GetDesktopWindow();
	HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);

	// 获取监视器逻辑宽度
	MONITORINFOEX monitorInfo;
	monitorInfo.cbSize = sizeof(monitorInfo);
	GetMonitorInfo(hMonitor, &monitorInfo);
	int cxLogical = (monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left);

	// 获取监视器物理宽度
	DEVMODE dm;
	dm.dmSize = sizeof(dm);
	dm.dmDriverExtra = 0;
	EnumDisplaySettings(monitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &dm);
	int cxPhysical = dm.dmPelsWidth;

	return cxPhysical * 1.0 / cxLogical;
}

// If the user needs his own variables, he can inherit the op::Datum struct and add them in there.
// UserDatum can be directly used by the OpenPose wrapper because it inherits from op::Datum, just define
// WrapperT<std::vector<std::shared_ptr<UserDatum>>> instead of Wrapper
// (or equivalently WrapperT<std::vector<std::shared_ptr<UserDatum>>>)
struct UserDatum : public op::Datum
{
    bool boolThatUserNeedsForSomeReason;

    UserDatum(const bool boolThatUserNeedsForSomeReason_ = false) :
        boolThatUserNeedsForSomeReason{boolThatUserNeedsForSomeReason_}
    {}
};

// The W-classes can be implemented either as a template or as simple classes given
// that the user usually knows which kind of data he will move between the queues,
// in this case we assume a std::shared_ptr of a std::vector of UserDatum

// This worker will just read and return all the basic image file formats in a directory
class WUserInput : public op::WorkerProducer<std::shared_ptr<std::vector<std::shared_ptr<UserDatum>>>>
{
public:
    WUserInput(const std::string& directoryPath) :
        mImageFiles{op::getFilesOnDirectory(directoryPath, op::Extensions::Images)}, // For all basic image formats
        // If we want only e.g., "jpg" + "png" images
        // mImageFiles{op::getFilesOnDirectory(directoryPath, std::vector<std::string>{"jpg", "png"})},
        mCounter{0}
    {
        if (mImageFiles.empty())
            op::error("No images found on: " + directoryPath, __LINE__, __FUNCTION__, __FILE__);
    }

    void initializationOnThread() {}

    std::shared_ptr<std::vector<std::shared_ptr<UserDatum>>> workProducer()
    {

		if (worked)
		{
			this->stop();
			return nullptr;
		}
		else
		{
			// Create new datum
			auto datumsPtr = std::make_shared<std::vector<std::shared_ptr<UserDatum>>>();
			datumsPtr->emplace_back();
			auto& datumPtr = datumsPtr->at(0);
			datumPtr = std::make_shared<UserDatum>();

			// Fill datum
			//const cv::Mat cvInputData = cv::imread(mImageFiles.at(mCounter++));
			Screenshot sc;

			const cv::Mat cvInputData = sc.getScreenshot(480,270,960,540);
			datumPtr->cvInputData = OP_CV2OPCONSTMAT(cvInputData);

			// If empty frame -> return nullptr
			if (datumPtr->cvInputData.empty())
			{
				op::opLog("Empty frame detected on path: " + mImageFiles.at(mCounter - 1) + ". Closing program.",
					op::Priority::High);
				this->stop();
				datumsPtr = nullptr;
			}
			worked = true;
			return datumsPtr;
			

		}
        //try
        //{
        //    // Close program when empty frame
        //    if (mImageFiles.size() <= mCounter)
        //    {
        //        op::opLog(
        //            "Last frame read and added to queue. Closing program after it is processed.", op::Priority::High);
        //        // This funtion stops this worker, which will eventually stop the whole thread system once all the
        //        // frames have been processed
        //        this->stop();
        //        return nullptr;
        //    }
        //    else
        //    {
        //        // Create new datum
        //        auto datumsPtr = std::make_shared<std::vector<std::shared_ptr<UserDatum>>>();
        //        datumsPtr->emplace_back();
        //        auto& datumPtr = datumsPtr->at(0);
        //        datumPtr = std::make_shared<UserDatum>();

        //        // Fill datum
        //        const cv::Mat cvInputData = cv::imread(mImageFiles.at(mCounter++));
        //        datumPtr->cvInputData = OP_CV2OPCONSTMAT(cvInputData);

        //        // If empty frame -> return nullptr
        //        if (datumPtr->cvInputData.empty())
        //        {
        //            op::opLog("Empty frame detected on path: " + mImageFiles.at(mCounter-1) + ". Closing program.",
        //                op::Priority::High);
        //            this->stop();
        //            datumsPtr = nullptr;
        //        }

        //        return datumsPtr;
        //    }
        //}
        //catch (const std::exception& e)
        //{
        //    op::opLog("Some kind of unexpected error happened.");
        //    this->stop();
        //    op::error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        //    return nullptr;
        //}
    }

private:
    const std::vector<std::string> mImageFiles;
    unsigned long long mCounter;
};

// This worker will just invert the image
class WUserPostProcessing : public op::Worker<std::shared_ptr<std::vector<std::shared_ptr<UserDatum>>>>
{
public:
    WUserPostProcessing()
    {
        // User's constructor here
    }

    void initializationOnThread() {}

    void work(std::shared_ptr<std::vector<std::shared_ptr<UserDatum>>>& datumsPtr)
    {
        try
        {
            // User's post-processing (after OpenPose processing & before OpenPose outputs) here
                // datumPtr->cvOutputData: rendered frame with pose or heatmaps
                // datumPtr->poseKeypoints: Array<float> with the estimated pose
            if (datumsPtr != nullptr && !datumsPtr->empty())
            {
                for (auto& datumPtr : *datumsPtr)
                {
                    cv::Mat cvOutputData = OP_OP2CVMAT(datumPtr->cvOutputData);
                    cv::bitwise_not(cvOutputData, cvOutputData);
                }
            }
        }
        catch (const std::exception& e)
        {
            op::opLog("Some kind of unexpected error happened.");
            this->stop();
            op::error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }
};

// This worker will just read and return all the jpg files in a directory
class WUserOutput : public op::WorkerConsumer<std::shared_ptr<std::vector<std::shared_ptr<UserDatum>>>>
{
public:
    void initializationOnThread() {}

    void workConsumer(const std::shared_ptr<std::vector<std::shared_ptr<UserDatum>>>& datumsPtr)
    {
        try
        {
            // User's displaying/saving/other processing here
                // datumPtr->cvOutputData: rendered frame with pose or heatmaps
                // datumPtr->poseKeypoints: Array<float> with the estimated pose
            if (datumsPtr != nullptr && !datumsPtr->empty())
            {
                const cv::Mat cvMat = OP_OP2CVCONSTMAT(datumsPtr->at(0)->cvOutputData);
                cv::imshow(OPEN_POSE_NAME_AND_VERSION + " - Tutorial Thread API", cvMat);
                // It displays the image and sleeps at least 1 ms (it usually sleeps ~5-10 msec to display the image)
                cv::waitKey(1);
            }
        }
        catch (const std::exception& e)
        {
            op::opLog("Some kind of unexpected error happened.");
            this->stop();
            op::error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }
};

int openPoseTutorialThread2()
{
    try
    {
        op::opLog("Starting OpenPose demo...", op::Priority::High);
        const auto opTimer = op::getTimerInit();

        // ------------------------- INITIALIZATION -------------------------
        // Step 1 - Set logging level
            // - 0 will output all the logging messages
            // - 255 will output nothing
        op::checkBool(
            0 <= FLAGS_logging_level && FLAGS_logging_level <= 255, "Wrong logging_level value.",
            __LINE__, __FUNCTION__, __FILE__);
        op::ConfigureLog::setPriorityThreshold((op::Priority)FLAGS_logging_level);
        // Step 2 - Setting thread workers && manager
        typedef std::shared_ptr<std::vector<std::shared_ptr<UserDatum>>> TypedefDatumsSP;
        op::ThreadManager<TypedefDatumsSP> threadManager;
        // Step 3 - Initializing the worker classes
        // Frames producer (e.g., video, webcam, ...)
        auto wUserInput = std::make_shared<WUserInput>(FLAGS_image_dir);
        // Processing
        auto wUserProcessing = std::make_shared<WUserPostProcessing>();
        // GUI (Display)
        auto wUserOutput = std::make_shared<WUserOutput>();

        // ------------------------- CONFIGURING THREADING -------------------------
        // In this simple multi-thread example, we will do the following:
            // 3 (virtual) queues: 0, 1, 2
            // 1 real queue: 1. The first and last queue ids (in this case 0 and 2) are not actual queues, but the
            // beginning and end of the processing sequence
            // 2 threads: 0, 1
            // wUserInput will generate frames (there is no real queue 0) and push them on queue 1
            // wGui will pop frames from queue 1 and process them (there is no real queue 2)
        auto threadId = 0ull;
        auto queueIn = 0ull;
        auto queueOut = 1ull;
        threadManager.add(threadId++, wUserInput, queueIn++, queueOut++);       // Thread 0, queues 0 -> 1
        threadManager.add(threadId++, wUserProcessing, queueIn++, queueOut++);  // Thread 1, queues 1 -> 2
        threadManager.add(threadId++, wUserOutput, queueIn++, queueOut++);      // Thread 2, queues 2 -> 3

        // ------------------------- STARTING AND STOPPING THREADING -------------------------
        op::opLog("Starting thread(s)...", op::Priority::High);
        // Two different ways of running the program on multithread environment
            // Option a) Using the main thread (this thread) for processing (it saves 1 thread, recommended)
        threadManager.exec();
            // Option b) Giving to the user the control of this thread
        // // VERY IMPORTANT NOTE: if OpenCV is compiled with Qt support, this option will not work. Qt needs the main
        // // thread to plot visual results, so the final GUI (which uses OpenCV) would return an exception similar to:
        // // `QMetaMethod::invoke: Unable to invoke methods with return values in queued connections`
        // // Start threads
        // threadManager.start();
        // // Keep program alive while running threads. Here the user could perform any other desired function
        // while (threadManager.isRunning())
        //     std::this_thread::sleep_for(std::chrono::milliseconds{33});
        // // Stop and join threads
        // op::opLog("Stopping thread(s)", op::Priority::High);
        // threadManager.stop();

        // Measuring total time
        op::printTime(opTimer, "OpenPose demo successfully finished. Total time: ", " seconds.", op::Priority::High);

        // Return
        return 0;
    }
    catch (const std::exception& e)
    {
        op::error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        return -1;
    }
}
LRESULT CALLBACK LowLevelKeyboardProc(
	_In_ int nCode,		// 规定钩子如何处理消息，小于 0 则直接 CallNextHookEx
	_In_ WPARAM wParam,	// 消息类型
	_In_ LPARAM lParam	// 指向某个结构体的指针，这里是 KBDLLHOOKSTRUCT（低级键盘输入事件）
	) {
	KBDLLHOOKSTRUCT *ks = (KBDLLHOOKSTRUCT*)lParam;		// 包含低级键盘输入事件信息
														/*
														typedef struct tagKBDLLHOOKSTRUCT {
														DWORD     vkCode;		// 按键代号
														DWORD     scanCode;		// 硬件扫描代号，同 vkCode 也可以作为按键的代号。
														DWORD     flags;		// 事件类型，一般按键按下为 0 抬起为 128。
														DWORD     time;			// 消息时间戳
														ULONG_PTR dwExtraInfo;	// 消息附加信息，一般为 0。
														}KBDLLHOOKSTRUCT,*LPKBDLLHOOKSTRUCT,*PKBDLLHOOKSTRUCT;
														*/
	if (ks->flags == 128 || ks->flags == 129)
	{
		// 监控键盘
		switch (ks->vkCode) {
		case 0x30: case 0x60:
			//cout << "检测到按键：" << "0" << endl;
			break;
		case 0x31: case 0x61:
			//cout << "检测到按键：" << "1" << endl;
			worked = false;
			openPoseTutorialThread2();
			break;
		}

		//return 1;		// 使按键失效
	}

	// 将消息传递给钩子链中的下一个钩子
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}
int main(int argc, char *argv[])
{
    // Parsing command line flags
    gflags::ParseCommandLineFlags(&argc, &argv, true);
	int ch;
	SetConsoleOutputCP(65001);// 更改cmd编码为utf8
							  // 安装钩子
	keyboardHook = SetWindowsHookEx(
		WH_KEYBOARD_LL,			// 钩子类型，WH_KEYBOARD_LL 为键盘钩子
		LowLevelKeyboardProc,	// 指向钩子函数的指针
		GetModuleHandleA(NULL),	// Dll 句柄
		NULL
		);
	if (keyboardHook == 0) { cout << "挂钩键盘失败" << endl; return -1; }

	//不可漏掉消息处理，不然程序会卡死
	MSG msg;
	while (1)
	{
		// 如果消息队列中有消息
		if (PeekMessageA(
			&msg,		// MSG 接收这个消息
			NULL,		// 检测消息的窗口句柄，NULL：检索当前线程所有窗口消息
			NULL,		// 检查消息范围中第一个消息的值，NULL：检查所有消息（必须和下面的同时为NULL）
			NULL,		// 检查消息范围中最后一个消息的值，NULL：检查所有消息（必须和上面的同时为NULL）
			PM_REMOVE	// 处理消息的方式，PM_REMOVE：处理后将消息从队列中删除
			)) {
			// 把按键消息传递给字符消息
			TranslateMessage(&msg);

			// 将消息分派给窗口程序
			DispatchMessageW(&msg);
		}
		else
			Sleep(0);    //避免CPU全负载运行
	}
    // Running openPoseTutorialThread2
    //return openPoseTutorialThread2();
	return 1;
}
