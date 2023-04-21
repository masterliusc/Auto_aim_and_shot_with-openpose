// ----------------------------- OpenPose C++ API Tutorial - Example 1 - Body from image -----------------------------
// It reads an image, process it, and displays it with the pose keypoints.

// Third-party dependencies
#include <opencv2/opencv.hpp>
// Command-line user interface
#define OPENPOSE_FLAGS_DISABLE_POSE
#include <openpose/flags.hpp>
// OpenPose dependencies
#include <openpose/headers.hpp>


#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <conio.h>

using cv::Mat;
using namespace std;
// Custom OpenPose flags
// Producer
DEFINE_string(image_path, "examples/media/COCO_val2014_000000000192.jpg",
	"Process an image. Read all standard formats (jpg, png, bmp, etc.).");
// Display
DEFINE_bool(no_display, false,
	"Enable to disable the visual display.");


HHOOK keyboardHook = 0;		// 钩子句柄、




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

// This worker will just read and return all the jpg files in a directory
void display(const std::shared_ptr<std::vector<std::shared_ptr<op::Datum>>>& datumsPtr)
{
    try
    {
        // User's displaying/saving/other processing here
            // datum.cvOutputData: rendered frame with pose or heatmaps
            // datum.poseKeypoints: Array<float> with the estimated pose
        if (datumsPtr != nullptr && !datumsPtr->empty())
        {
            // Display image
            const cv::Mat cvMat = OP_OP2CVCONSTMAT(datumsPtr->at(0)->cvOutputData);
            if (!cvMat.empty())
            {
               /* cv::imshow(OPEN_POSE_NAME_AND_VERSION + " - Tutorial C++ API", cvMat);
                cv::waitKey(0);*/
            }
            else
                op::opLog("Empty cv::Mat as output.", op::Priority::High, __LINE__, __FUNCTION__, __FILE__);
        }
        else
            op::opLog("Nullptr or empty datumsPtr found.", op::Priority::High);
    }
    catch (const std::exception& e)
    {
        op::error(e.what(), __LINE__, __FUNCTION__, __FILE__);
    }
}

void printKeypoints(const std::shared_ptr<std::vector<std::shared_ptr<op::Datum>>>& datumsPtr)
{
    try
    {
        // Example: How to use the pose keypoints
        if (datumsPtr != nullptr && !datumsPtr->empty())
        {
            // Alternative 1
            op::opLog("Body keypoints: " + datumsPtr->at(0)->poseKeypoints.toString(), op::Priority::High);

            // // Alternative 2
            // op::opLog(datumsPtr->at(0)->poseKeypoints, op::Priority::High);

            // // Alternative 3
            // std::cout << datumsPtr->at(0)->poseKeypoints << std::endl;

            // // Alternative 4 - Accesing each element of the keypoints
            // op::opLog("\nKeypoints:", op::Priority::High);
            // const auto& poseKeypoints = datumsPtr->at(0)->poseKeypoints;
            // op::opLog("Person pose keypoints:", op::Priority::High);
            // for (auto person = 0 ; person < poseKeypoints.getSize(0) ; person++)
            // {
            //     op::opLog("Person " + std::to_string(person) + " (x, y, score):", op::Priority::High);
            //     for (auto bodyPart = 0 ; bodyPart < poseKeypoints.getSize(1) ; bodyPart++)
            //     {
            //         std::string valueToPrint;
            //         for (auto xyscore = 0 ; xyscore < poseKeypoints.getSize(2) ; xyscore++)
            //             valueToPrint += std::to_string(   poseKeypoints[{person, bodyPart, xyscore}]   ) + " ";
            //         op::opLog(valueToPrint, op::Priority::High);
            //     }
            // }
            // op::opLog(" ", op::Priority::High);
        }
        else
            op::opLog("Nullptr or empty datumsPtr found.", op::Priority::High);
    }
    catch (const std::exception& e)
    {
        op::error(e.what(), __LINE__, __FUNCTION__, __FILE__);
    }
}

int tutorialApiCpp()
{
    try
    {
        op::opLog("Starting OpenPose demo...", op::Priority::High);
        const auto opTimer = op::getTimerInit();

        // Configuring OpenPose
        op::opLog("Configuring OpenPose...", op::Priority::High);
        op::Wrapper opWrapper{op::ThreadManagerMode::Asynchronous };
        // Set to single-thread (for sequential processing and/or debugging and/or reducing latency)
        /*if (FLAGS_disable_multi_thread)
            opWrapper.disableMultiThreading();*/

        // Starting OpenPose
        op::opLog("Starting thread(s)...", op::Priority::High);
        opWrapper.start();

        // Process and display image
		Screenshot SC;
        //const cv::Mat cvImageToProcess = cv::imread(FLAGS_image_path);
		const cv::Mat cvImageToProcess = SC.getScreenshot(480,270,960,540);
        const op::Matrix imageToProcess = OP_CV2OPCONSTMAT(cvImageToProcess);
        auto datumProcessed = opWrapper.emplaceAndPop(imageToProcess);
        if (datumProcessed != nullptr)
        {
            //printKeypoints(datumProcessed);
            //if (!FLAGS_no_display)
              //display(datumProcessed);
        }
        else
            op::opLog("Image could not be processed.", op::Priority::High);

        // Measuring total time
        op::printTime(opTimer, "OpenPose demo successfully finished. Total time: ", " seconds.", op::Priority::High);

        // Return
        return 0;
    }
    catch (const std::exception&)
    {
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
			tutorialApiCpp();
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
	// 删除钩子
	UnhookWindowsHookEx(keyboardHook);
    // Running tutorialApiCpp
    return tutorialApiCpp();
}
