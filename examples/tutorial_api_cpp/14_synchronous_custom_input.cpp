// ------------------------- OpenPose C++ API Tutorial - Example 13 - Custom Input -------------------------
// Synchronous mode: ideal for production integration. It provides the fastest results with respect to runtime
// performance.
// In this function, the user can implement its own way to create frames (e.g., reading his own folder of images).

// Third-party dependencies
#include <opencv2/opencv.hpp>
// Command-line user interface
#define OPENPOSE_FLAGS_DISABLE_PRODUCER
#include <openpose/flags.hpp>
// OpenPose dependencies
#include <openpose/headers.hpp>

// Custom OpenPose flags
// Producer
DEFINE_string(image_dir,                "examples/media/",
    "Process a directory of images. Read all standard formats (jpg, png, bmp, etc.).");
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

// This worker will just read and return all the basic image file formats in a directory
class WUserInput : public op::WorkerProducer<std::shared_ptr<std::vector<std::shared_ptr<op::Datum>>>>
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

    std::shared_ptr<std::vector<std::shared_ptr<op::Datum>>> workProducer()
    {
		if(worked)
		{ 
			this->stop();
			return nullptr;
		}
		else
		{
			// Create new datum
			auto datumsPtr = std::make_shared<std::vector<std::shared_ptr<op::Datum>>>();
			datumsPtr->emplace_back();
			auto& datumPtr = datumsPtr->at(0);
			datumPtr = std::make_shared<op::Datum>();

			// Fill datum
			//const cv::Mat cvInputData = cv::imread(mImageFiles.at(mCounter++));
			Screenshot sc;

			const cv::Mat cvInputData = sc.getScreenshot();
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
    //    try
    //    {
    //        // Close program when empty frame
    //        if (mImageFiles.size() <= mCounter)
    //        {
    //            op::opLog("Last frame read and added to queue. Closing program after it is processed.",
    //                    op::Priority::High);
    //            // This funtion stops this worker, which will eventually stop the whole thread system once all the
    //            // frames have been processed
    //            this->stop();
    //            return nullptr;
    //        }
    //        else
    //        {
    //            // Create new datum
    //            auto datumsPtr = std::make_shared<std::vector<std::shared_ptr<op::Datum>>>();
    //            datumsPtr->emplace_back();
    //            auto& datumPtr = datumsPtr->at(0);
    //            datumPtr = std::make_shared<op::Datum>();

    //            // Fill datum
    //            //const cv::Mat cvInputData = cv::imread(mImageFiles.at(mCounter++));
				//Screenshot sc;
				//
				//const cv::Mat cvInputData = sc.getScreenshot();
    //            datumPtr->cvInputData = OP_CV2OPCONSTMAT(cvInputData);

    //            // If empty frame -> return nullptr
    //            if (datumPtr->cvInputData.empty())
    //            {
    //                op::opLog("Empty frame detected on path: " + mImageFiles.at(mCounter-1) + ". Closing program.",
    //                    op::Priority::High);
    //                this->stop();
    //                datumsPtr = nullptr;
    //            }

    //            return datumsPtr;
    //        }
    //    }
    //    catch (const std::exception& e)
    //    {
    //        this->stop();
    //        op::error(e.what(), __LINE__, __FUNCTION__, __FILE__);
    //        return nullptr;
    //    }
    }

private:
    const std::vector<std::string> mImageFiles;
    unsigned long long mCounter;
};

void configureWrapper(op::Wrapper& opWrapper)
{
    try
    {
        // Configuring OpenPose

        // logging_level
        op::checkBool(
            0 <= FLAGS_logging_level && FLAGS_logging_level <= 255, "Wrong logging_level value.",
            __LINE__, __FUNCTION__, __FILE__);
        op::ConfigureLog::setPriorityThreshold((op::Priority)FLAGS_logging_level);
        op::Profiler::setDefaultX(FLAGS_profile_speed);

        // Applying user defined configuration - GFlags to program variables
        // outputSize
        const auto outputSize = op::flagsToPoint(op::String(FLAGS_output_resolution), "-1x-1");
        // netInputSize
        const auto netInputSize = op::flagsToPoint(op::String(FLAGS_net_resolution), "-1x368");
        // faceNetInputSize
        const auto faceNetInputSize = op::flagsToPoint(op::String(FLAGS_face_net_resolution), "368x368 (multiples of 16)");
        // handNetInputSize
        const auto handNetInputSize = op::flagsToPoint(op::String(FLAGS_hand_net_resolution), "368x368 (multiples of 16)");
        // poseMode
        const auto poseMode = op::flagsToPoseMode(FLAGS_body);
        // poseModel
        const auto poseModel = op::flagsToPoseModel(op::String(FLAGS_model_pose));
        // JSON saving
        if (!FLAGS_write_keypoint.empty())
            op::opLog(
                "Flag `write_keypoint` is deprecated and will eventually be removed. Please, use `write_json`"
                " instead.", op::Priority::Max);
        // keypointScaleMode
        const auto keypointScaleMode = op::flagsToScaleMode(FLAGS_keypoint_scale);
        // heatmaps to add
        const auto heatMapTypes = op::flagsToHeatMaps(FLAGS_heatmaps_add_parts, FLAGS_heatmaps_add_bkg,
                                                      FLAGS_heatmaps_add_PAFs);
        const auto heatMapScaleMode = op::flagsToHeatMapScaleMode(FLAGS_heatmaps_scale);
        // >1 camera view?
        // const auto multipleView = (FLAGS_3d || FLAGS_3d_views > 1 || FLAGS_flir_camera);
        const auto multipleView = false;
        // Face and hand detectors
        const auto faceDetector = op::flagsToDetector(FLAGS_face_detector);
        const auto handDetector = op::flagsToDetector(FLAGS_hand_detector);
        // Enabling Google Logging
        const bool enableGoogleLogging = true;

        // Initializing the user custom classes
        // Frames producer (e.g., video, webcam, ...)
        auto wUserInput = std::make_shared<WUserInput>(FLAGS_image_dir);
        // Add custom processing
        const auto workerInputOnNewThread = true;
        opWrapper.setWorker(op::WorkerType::Input, wUserInput, workerInputOnNewThread);

        // Pose configuration (use WrapperStructPose{} for default and recommended configuration)
        const op::WrapperStructPose wrapperStructPose{
            poseMode, netInputSize, FLAGS_net_resolution_dynamic, outputSize, keypointScaleMode, FLAGS_num_gpu,
            FLAGS_num_gpu_start, FLAGS_scale_number, (float)FLAGS_scale_gap,
            op::flagsToRenderMode(FLAGS_render_pose, multipleView), poseModel, !FLAGS_disable_blending,
            (float)FLAGS_alpha_pose, (float)FLAGS_alpha_heatmap, FLAGS_part_to_show, op::String(FLAGS_model_folder),
            heatMapTypes, heatMapScaleMode, FLAGS_part_candidates, (float)FLAGS_render_threshold,
            FLAGS_number_people_max, FLAGS_maximize_positives, FLAGS_fps_max, op::String(FLAGS_prototxt_path),
            op::String(FLAGS_caffemodel_path), (float)FLAGS_upsampling_ratio, enableGoogleLogging};
        opWrapper.configure(wrapperStructPose);
        // Face configuration (use op::WrapperStructFace{} to disable it)
        const op::WrapperStructFace wrapperStructFace{
            FLAGS_face, faceDetector, faceNetInputSize,
            op::flagsToRenderMode(FLAGS_face_render, multipleView, FLAGS_render_pose),
            (float)FLAGS_face_alpha_pose, (float)FLAGS_face_alpha_heatmap, (float)FLAGS_face_render_threshold};
        opWrapper.configure(wrapperStructFace);
        // Hand configuration (use op::WrapperStructHand{} to disable it)
        const op::WrapperStructHand wrapperStructHand{
            FLAGS_hand, handDetector, handNetInputSize, FLAGS_hand_scale_number, (float)FLAGS_hand_scale_range,
            op::flagsToRenderMode(FLAGS_hand_render, multipleView, FLAGS_render_pose), (float)FLAGS_hand_alpha_pose,
            (float)FLAGS_hand_alpha_heatmap, (float)FLAGS_hand_render_threshold};
        opWrapper.configure(wrapperStructHand);
        // Extra functionality configuration (use op::WrapperStructExtra{} to disable it)
        const op::WrapperStructExtra wrapperStructExtra{
            FLAGS_3d, FLAGS_3d_min_views, FLAGS_identification, FLAGS_tracking, FLAGS_ik_threads};
        opWrapper.configure(wrapperStructExtra);
        // Output (comment or use default argument to disable any output)
        const op::WrapperStructOutput wrapperStructOutput{
            FLAGS_cli_verbose, op::String(FLAGS_write_keypoint), op::stringToDataFormat(FLAGS_write_keypoint_format),
            op::String(FLAGS_write_json), op::String(FLAGS_write_coco_json), FLAGS_write_coco_json_variants,
            FLAGS_write_coco_json_variant, op::String(FLAGS_write_images), op::String(FLAGS_write_images_format),
            op::String(FLAGS_write_video), FLAGS_write_video_fps, FLAGS_write_video_with_audio,
            op::String(FLAGS_write_heatmaps), op::String(FLAGS_write_heatmaps_format), op::String(FLAGS_write_video_3d),
            op::String(FLAGS_write_video_adam), op::String(FLAGS_write_bvh), op::String(FLAGS_udp_host),
            op::String(FLAGS_udp_port)};
        opWrapper.configure(wrapperStructOutput);
        // GUI (comment or use default argument to disable any visual output)
        const op::WrapperStructGui wrapperStructGui{
            op::flagsToDisplayMode(FLAGS_display, FLAGS_3d), !FLAGS_no_gui_verbose, FLAGS_fullscreen};
        opWrapper.configure(wrapperStructGui);
        // Set to single-thread (for sequential processing and/or debugging and/or reducing latency)
        if (FLAGS_disable_multi_thread)
            opWrapper.disableMultiThreading();
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

        // OpenPose wrapper
        op::opLog("Configuring OpenPose...", op::Priority::High);
        op::Wrapper opWrapper;
        configureWrapper(opWrapper);

        // Start, run, and stop processing - exec() blocks this thread until OpenPose wrapper has finished
        op::opLog("Starting thread(s)...", op::Priority::High);
        opWrapper.exec();

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
			worked = false;
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
    // Running tutorialApiCpp
    return tutorialApiCpp();
}
