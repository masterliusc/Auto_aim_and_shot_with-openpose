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



HHOOK keyboardHook = 0;		// ���Ӿ����
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

	// ��ȡ��Ļ DC
	m_screenDC = GetDC(NULL);
	m_compatibleDC = CreateCompatibleDC(m_screenDC);

	// ����λͼ
	m_hBitmap = CreateCompatibleBitmap(m_screenDC, m_width, m_height);
	SelectObject(m_compatibleDC, m_hBitmap);
}

/* ��ȡ������Ļ�Ľ�ͼ */
Mat Screenshot::getScreenshot()
{
	// �õ�λͼ������
	BitBlt(m_compatibleDC, 0, 0, m_width, m_height, m_screenDC, 0, 0, SRCCOPY);
	GetBitmapBits(m_hBitmap, m_width * m_height * 4, m_screenshotData);

	// ����ͼ��
	Mat screenshot_(m_height, m_width, CV_8UC4, m_screenshotData);
	Mat screenshot;
	cv::cvtColor(screenshot_, screenshot, cv::COLOR_RGBA2RGB);
	return screenshot;
}

/** @brief ��ȡָ����Χ����Ļ��ͼ
* @param x ͼ�����Ͻǵ� X ����
* @param y ͼ�����Ͻǵ� Y ����
* @param width ͼ����
* @param height ͼ��߶�
*/
Mat Screenshot::getScreenshot(int x, int y, int width, int height)
{
	Mat screenshot = getScreenshot();
	return screenshot(cv::Rect(x, y, width, height));
}

/* ��ȡ��Ļ����ֵ */
double Screenshot::getZoom()
{
	// ��ȡ���ڵ�ǰ��ʾ�ļ�����
	HWND hWnd = GetDesktopWindow();
	HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);

	// ��ȡ�������߼����
	MONITORINFOEX monitorInfo;
	monitorInfo.cbSize = sizeof(monitorInfo);
	GetMonitorInfo(hMonitor, &monitorInfo);
	int cxLogical = (monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left);

	// ��ȡ������������
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
	_In_ int nCode,		// �涨������δ�����Ϣ��С�� 0 ��ֱ�� CallNextHookEx
	_In_ WPARAM wParam,	// ��Ϣ����
	_In_ LPARAM lParam	// ָ��ĳ���ṹ���ָ�룬������ KBDLLHOOKSTRUCT���ͼ����������¼���
	) {
	KBDLLHOOKSTRUCT *ks = (KBDLLHOOKSTRUCT*)lParam;		// �����ͼ����������¼���Ϣ
														/*
														typedef struct tagKBDLLHOOKSTRUCT {
														DWORD     vkCode;		// ��������
														DWORD     scanCode;		// Ӳ��ɨ����ţ�ͬ vkCode Ҳ������Ϊ�����Ĵ��š�
														DWORD     flags;		// �¼����ͣ�һ�㰴������Ϊ 0 ̧��Ϊ 128��
														DWORD     time;			// ��Ϣʱ���
														ULONG_PTR dwExtraInfo;	// ��Ϣ������Ϣ��һ��Ϊ 0��
														}KBDLLHOOKSTRUCT,*LPKBDLLHOOKSTRUCT,*PKBDLLHOOKSTRUCT;
														*/
	if (ks->flags == 128 || ks->flags == 129)
	{
		// ��ؼ���
		switch (ks->vkCode) {
		case 0x30: case 0x60:
			//cout << "��⵽������" << "0" << endl;
			break;
		case 0x31: case 0x61:
			//cout << "��⵽������" << "1" << endl;
			worked = false;
			openPoseTutorialThread2();
			break;
		}

		//return 1;		// ʹ����ʧЧ
	}

	// ����Ϣ���ݸ��������е���һ������
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}
int main(int argc, char *argv[])
{
    // Parsing command line flags
    gflags::ParseCommandLineFlags(&argc, &argv, true);
	int ch;
	SetConsoleOutputCP(65001);// ����cmd����Ϊutf8
							  // ��װ����
	keyboardHook = SetWindowsHookEx(
		WH_KEYBOARD_LL,			// �������ͣ�WH_KEYBOARD_LL Ϊ���̹���
		LowLevelKeyboardProc,	// ָ���Ӻ�����ָ��
		GetModuleHandleA(NULL),	// Dll ���
		NULL
		);
	if (keyboardHook == 0) { cout << "�ҹ�����ʧ��" << endl; return -1; }

	//����©����Ϣ������Ȼ����Ῠ��
	MSG msg;
	while (1)
	{
		// �����Ϣ����������Ϣ
		if (PeekMessageA(
			&msg,		// MSG ���������Ϣ
			NULL,		// �����Ϣ�Ĵ��ھ����NULL��������ǰ�߳����д�����Ϣ
			NULL,		// �����Ϣ��Χ�е�һ����Ϣ��ֵ��NULL�����������Ϣ������������ͬʱΪNULL��
			NULL,		// �����Ϣ��Χ�����һ����Ϣ��ֵ��NULL�����������Ϣ������������ͬʱΪNULL��
			PM_REMOVE	// ������Ϣ�ķ�ʽ��PM_REMOVE���������Ϣ�Ӷ�����ɾ��
			)) {
			// �Ѱ�����Ϣ���ݸ��ַ���Ϣ
			TranslateMessage(&msg);

			// ����Ϣ���ɸ����ڳ���
			DispatchMessageW(&msg);
		}
		else
			Sleep(0);    //����CPUȫ��������
	}
    // Running openPoseTutorialThread2
    //return openPoseTutorialThread2();
	return 1;
}
