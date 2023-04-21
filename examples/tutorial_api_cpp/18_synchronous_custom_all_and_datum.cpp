// --- OpenPose C++ API Tutorial - Example 17 - Custom Input, Pre-processing, Post-processing, Output, and Datum ---
// Synchronous mode: ideal for production integration. It provides the fastest results with respect to runtime
// performance.
// In this function, the user can implement its own way to read frames, implement its own post-processing (i.e., his
// function will be called after OpenPose has processed the frames but before saving), visualizing any result
// render/display/storage the results, and use their custom Datum structure

// Third-party dependencies
#include <opencv2/opencv.hpp>
// Command-line user interface
#define OPENPOSE_FLAGS_DISABLE_PRODUCER
#define OPENPOSE_FLAGS_DISABLE_DISPLAY
#include <openpose/flags.hpp>
// OpenPose dependencies
#include <openpose/headers.hpp>

// Custom OpenPose flags
// Producer
DEFINE_string(image_dir,                "examples/media/",
    "Process a directory of images. Read all standard formats (jpg, png, bmp, etc.).");
// Display
DEFINE_bool(no_display,                 false,
    "Enable to disable the visual display.");

#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <conio.h>
#include<thread>

#include <facedetectcnn.h>
#define DETECT_BUFFER_SIZE 0x20000
using cv::Mat;
using namespace std;
int startx = 640, starty = 440 , width = 640, height = 300;
int count_frame = 0;
int count_shot = 0;

HHOOK keyboardHook = 0;		// 钩子句柄、
bool worked = true,open=false;
std::chrono::time_point<std::chrono::high_resolution_clock> opTimer;
cv::VideoCapture capture;


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
Screenshot sc;

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
	//cout << " screenshot: " << screenshot.cols << " " << screenshot.rows << endl;
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
	bool finished = false;
};

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
		if (worked||!open)
		{
			//mClosed = true;
			//this->stop();
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
			
			
			const cv::Mat cvInputData = sc.getScreenshot(startx, starty, width, height);
			
			/*cv::Mat image2;
			capture.set(cv::CAP_PROP_POS_FRAMES, count_frame);
			capture.read(image2);
			image2 = image2(cv::Rect(startx, starty, width, height));
			count_frame += 4;*/
			//normalize(image2, image2,255,0,cv::NORM_MINMAX, CV_8UC3);
			/*cv::Mat image= sc.getScreenshot(640, 300, 640, 480);
			int * pResults = NULL;
			unsigned char * pBuffer = (unsigned char *)malloc(DETECT_BUFFER_SIZE);
			pResults = facedetect_cnn(pBuffer, (unsigned char*)(image.ptr(0)), image.cols, image.rows, (int)image.step);
			cout << pResults << " " << pResults[1] << " " << pResults[2] << " " << pResults[3] << endl;*/
			//std::cout << "img size: " << cvInputData.cols << " " << cvInputData.rows<<std::endl;
			op::printTime(opTimer, "OpenPose demo successfully finished. Total time: ", " seconds.", op::Priority::High);
			opTimer = op::getTimerInit();
			datumPtr->cvInputData = OP_CV2OPCONSTMAT(cvInputData);
			count_frame++;
			//datumPtr->cvInputData = OP_CV2OPCONSTMAT(image2);
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			// If empty frame -> return nullptr
			if (datumPtr->cvInputData.empty())
			{
				//mClosed = true;
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
        //            op::opLog(
        //                "Empty frame detected on path: " + mImageFiles.at(mCounter-1) + ". Closing program.",
        //                op::Priority::High);
        //            this->stop();
        //            datumsPtr = nullptr;
        //        }

        //        return datumsPtr;
        //    }
        //}
        //catch (const std::exception& e)
        //{
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
                    //cv::bitwise_not(cvOutputData, cvOutputData);
					
                }

            }
        }
        catch (const std::exception& e)
        {
            //this->stop();
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
			
            if (datumsPtr != nullptr && !datumsPtr->empty()&&!datumsPtr->at(0)->finished)
            {
				count_shot++;
				cout << "count: " << count_frame << " shot: " << count_shot << endl;
				/*op::printTime(opTimer, "OpenPose demo successfully finished. Total time: ", " seconds.", op::Priority::High);
				opTimer=op::getTimerInit();*/
				POINT p;
				POINT last_p;
				GetCursorPos(&last_p);//获取鼠标坐标
				float dx = 0, dy = 0, tx = last_p.x, ty = last_p.y;
				const auto& poseKeypoints = datumsPtr->at(0)->poseKeypoints;
				if (poseKeypoints.getSize(0) == 1)
				{
					float x = poseKeypoints[{0, 0, 0}];
					float y = poseKeypoints[{0, 0, 1}];
					float s = poseKeypoints[{0, 0, 2}];
					if (s == 0 && poseKeypoints[{0, 1, 2}]>0)
					{
						x = poseKeypoints[{0, 1, 0}];
						y = poseKeypoints[{0, 1, 1}];
					}
					dx = (x + startx) - 1920/2;
					dy = (y + starty) - 1080/2;
					tx = (x + startx);
					ty = (y + starty);
				}
				else if (poseKeypoints.getSize(0)>1)
				{
					float maxs = 0;
					int best = 0;
					for (int i = 0; i < poseKeypoints.getSize(0); i++)
					{
						if (poseKeypoints[{i, 0, 2}] > maxs)
						{
							maxs = poseKeypoints[{i, 0, 2}];
							best = i;
						}
					}
					float x = poseKeypoints[{best, 0, 0}];
					float y = poseKeypoints[{best, 0, 1}];
					float s = poseKeypoints[{best, 0, 2}];
					dx = (x + startx) - 1920 / 2;
					dy = (y + starty) - 1080 / 2;
					tx = (x + startx);
					ty = (y + starty);
				}
				//dy = dy + 20;
			
				//if (poseKeypoints.getSize(0) > 0)
				if(poseKeypoints.getSize(0) >0)
				{
					
					cout << " tx " << dx << " ty " << dy<<endl;
					INPUT inputm;
					//SetCursorPos(last_p.x+ dx, last_p.y + dy);
					float sign_y = (dy / abs(dy)) *(abs(dy) / abs(dx));
					int sign_x = dx / abs(dx);
					LARGE_INTEGER t1, t2, tc;
					QueryPerformanceFrequency(&tc);
					QueryPerformanceCounter(&t1);
					//for (int i = 0; i <= abs(dx); i++)
					//{
					//	inputm.mi.dx = (0 +  sign_x*4.);// *(65536.0f / GetSystemMetrics(SM_CXSCREEN));//x being coord in pixels
					//	inputm.mi.dy =  (0 + 4.*sign_y);// *(65536.0f / GetSystemMetrics(SM_CYSCREEN));//y being coord in pixels
					//	inputm.mi.dwFlags = MOUSEEVENTF_MOVE;
					//	inputm.type = INPUT_MOUSE;
					//	SendInput(1, &inputm, sizeof(inputm));
					//	//Sleep(0);
					//}
					inputm.mi.dx = (dx);// *(65536.0f / GetSystemMetrics(SM_CXSCREEN));//x being coord in pixels
					inputm.mi.dy = (dy);// *(65536.0f / GetSystemMetrics(SM_CYSCREEN));//y being coord in pixels
					inputm.mi.dwFlags = MOUSEEVENTF_MOVE;
					inputm.type = INPUT_MOUSE;
					SendInput(1, &inputm, sizeof(inputm));
					QueryPerformanceCounter(&t2);
					double time = (double)(t2.QuadPart - t1.QuadPart) / (double)tc.QuadPart;
					std::cout << "time = " << time * 1000 << std::endl;  //输出时间（单位：ｓ）
					//for (int i = 0; i <= abs(dy); i++)
					//{
					//	inputm.mi.dx = 0;// (0 + 2 * sign_x);// *(65536.0f / GetSystemMetrics(SM_CXSCREEN));//x being coord in pixels
					//	inputm.mi.dy = (0 +  sign_y*2);// *(65536.0f / GetSystemMetrics(SM_CYSCREEN));//y being coord in pixels
					//	inputm.mi.dwFlags = MOUSEEVENTF_MOVE;
					//	inputm.type = INPUT_MOUSE;
					//	SendInput(1, &inputm, sizeof(inputm));
					//	//Sleep(1);
					//}
					//inputm.mi.dx = dx;//(last_p.x+ dx) *(65536.0f / GetSystemMetrics(SM_CXSCREEN));//x being coord in pixels
					//inputm.mi.dy = dy;// (last_p.y + dy) *(65536.0f / GetSystemMetrics(SM_CYSCREEN));//y being coord in pixels
					//inputm.mi.dwFlags = MOUSEEVENTF_MOVE ;
					//inputm.type = INPUT_MOUSE; 
					//SendInput(1, &inputm, sizeof(inputm));
					/*GetCursorPos(&p);
					cout << p.x << " "<<p.y << endl;*/
					
				    mouse_event(MOUSEEVENTF_LEFTDOWN   , 0, 0, 0, 0);
					
					
					/*std::this_thread::sleep_for(std::chrono::milliseconds(100));
					mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
					mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
					mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);*/
					datumsPtr->at(0)->finished = true;
					
					//std::this_thread::sleep_for(std::chrono::milliseconds(300));
					mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
					//const cv::Mat cvMat = OP_OP2CVCONSTMAT(datumsPtr->at(0)->cvOutputData);
					//if (!cvMat.empty())


					//{
					//	cv::imshow("aim", cvMat);
			 	//		// Display image and sleeps at least 1 ms (it usually sleeps ~5-10 msec to display the image)
					//	const char key = (char)cv::waitKey(0);
					//	/*if (key == 27)
					//		this->stop();*/
					//}
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					//Sleep(300);
				}
				else
				{
					
					datumsPtr->at(0)->finished = true;
					mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
				}
				
				// Show in command line the resulting pose keypoints for body, face and hands
                //op::opLog("\nKeypoints:");
                //// Accesing each element of the keypoints
                //const auto& poseKeypoints = datumsPtr->at(0)->poseKeypoints;
                //op::opLog("Person pose keypoints:");
                //for (auto person = 0 ; person < poseKeypoints.getSize(0) ; person++)
                //{
                //    op::opLog("Person " + std::to_string(person) + " (x, y, score):");
                //    for (auto bodyPart = 0 ; bodyPart < poseKeypoints.getSize(1) ; bodyPart++)
                //    {
                //        std::string valueToPrint;
                //        for (auto xyscore = 0 ; xyscore < poseKeypoints.getSize(2) ; xyscore++)
                //        {
                //            valueToPrint += std::to_string(   poseKeypoints[{person, bodyPart, xyscore}]   ) + " ";
                //        }
                //        op::opLog(valueToPrint);
                //    }
                //}

                //op::opLog(" ");
                //// Alternative: just getting std::string equivalent
                //op::opLog("Face keypoints: " + datumsPtr->at(0)->faceKeypoints.toString());
                //op::opLog("Left hand keypoints: " + datumsPtr->at(0)->handKeypoints[0].toString());
                //op::opLog("Right hand keypoints: " + datumsPtr->at(0)->handKeypoints[1].toString());
                // Heatmaps
                //const auto& poseHeatMaps = datumsPtr->at(0)->poseHeatMaps;
                /*if (!poseHeatMaps.empty())
                {
                    op::opLog("Pose heatmaps size: [" + std::to_string(poseHeatMaps.getSize(0)) + ", "
                            + std::to_string(poseHeatMaps.getSize(1)) + ", "
                            + std::to_string(poseHeatMaps.getSize(2)) + "]");
                    const auto& faceHeatMaps = datumsPtr->at(0)->faceHeatMaps;
                    op::opLog("Face heatmaps size: [" + std::to_string(faceHeatMaps.getSize(0)) + ", "
                            + std::to_string(faceHeatMaps.getSize(1)) + ", "
                            + std::to_string(faceHeatMaps.getSize(2)) + ", "
                            + std::to_string(faceHeatMaps.getSize(3)) + "]");
                    const auto& handHeatMaps = datumsPtr->at(0)->handHeatMaps;
                    op::opLog("Left hand heatmaps size: [" + std::to_string(handHeatMaps[0].getSize(0)) + ", "
                            + std::to_string(handHeatMaps[0].getSize(1)) + ", "
                            + std::to_string(handHeatMaps[0].getSize(2)) + ", "
                            + std::to_string(handHeatMaps[0].getSize(3)) + "]");
                    op::opLog("Right hand heatmaps size: [" + std::to_string(handHeatMaps[1].getSize(0)) + ", "
                            + std::to_string(handHeatMaps[1].getSize(1)) + ", "
                            + std::to_string(handHeatMaps[1].getSize(2)) + ", "
                            + std::to_string(handHeatMaps[1].getSize(3)) + "]");
                }
*/

				/*datumsPtr->resize(0);
				datumsPtr->shrink_to_fit();*/
                 //Display results (if enabled)
                if (0)//!FLAGS_no_display)
                {
                    // Display rendered output image
                    const cv::Mat cvMat = OP_OP2CVCONSTMAT(datumsPtr->at(0)->cvOutputData);
                    if (!cvMat.empty())


                    {
                        cv::imshow(OPEN_POSE_NAME_AND_VERSION + " - Tutorial C++ API", cvMat);
                        // Display image and sleeps at least 1 ms (it usually sleeps ~5-10 msec to display the image)
                        const char key = (char)cv::waitKey(1);
                        if (key == 27)
                            this->stop();
                    }
                    else
                        op::opLog("Empty cv::Mat as output.", op::Priority::High, __LINE__, __FUNCTION__, __FILE__);
                }
            }
			worked = false;
        }
        catch (const std::exception& e)
        {
            //this->stop();
            op::error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }
};

void configureWrapper(op::WrapperT<UserDatum>& opWrapperT)
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
        const auto netInputSize = op::flagsToPoint(op::String(FLAGS_net_resolution), "320x176");
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
        const auto multipleView = (FLAGS_3d || FLAGS_3d_views > 1);
        // Face and hand detectors
        const auto faceDetector = op::flagsToDetector(FLAGS_face_detector);
        const auto handDetector = op::flagsToDetector(FLAGS_hand_detector);
        // Enabling Google Logging
        const bool enableGoogleLogging = true;

        // Initializing the user custom classes
        // Frames producer (e.g., video, webcam, ...)
        auto wUserInput = std::make_shared<WUserInput>(FLAGS_image_dir);
        // Processing
        auto wUserPostProcessing = std::make_shared<WUserPostProcessing>();
        // GUI (Display)
        auto wUserOutput = std::make_shared<WUserOutput>();

        // Add custom input
        const auto workerInputOnNewThread = false;
        opWrapperT.setWorker(op::WorkerType::Input, wUserInput, workerInputOnNewThread);
        // Add custom processing
        const auto workerProcessingOnNewThread = false;
        opWrapperT.setWorker(op::WorkerType::PostProcessing, wUserPostProcessing, workerProcessingOnNewThread);
        // Add custom output
        const auto workerOutputOnNewThread = true;
        opWrapperT.setWorker(op::WorkerType::Output, wUserOutput, workerOutputOnNewThread);

        // Pose configuration (use WrapperStructPose{} for default and recommended configuration)
        const op::WrapperStructPose wrapperStructPose{
            poseMode, netInputSize, FLAGS_net_resolution_dynamic, outputSize, keypointScaleMode, FLAGS_num_gpu,
            FLAGS_num_gpu_start, FLAGS_scale_number, (float)FLAGS_scale_gap,
            op::flagsToRenderMode(FLAGS_render_pose, multipleView), poseModel, !FLAGS_disable_blending,
            (float)FLAGS_alpha_pose, (float)FLAGS_alpha_heatmap, FLAGS_part_to_show, op::String(FLAGS_model_folder),
            heatMapTypes, heatMapScaleMode, FLAGS_part_candidates, (float)FLAGS_render_threshold,
            FLAGS_number_people_max, FLAGS_maximize_positives, FLAGS_fps_max, op::String(FLAGS_prototxt_path),
            op::String(FLAGS_caffemodel_path), (float)FLAGS_upsampling_ratio, enableGoogleLogging};
        opWrapperT.configure(wrapperStructPose);
        // Face configuration (use op::WrapperStructFace{} to disable it)
        const op::WrapperStructFace wrapperStructFace{
            FLAGS_face, faceDetector, faceNetInputSize,
            op::flagsToRenderMode(FLAGS_face_render, multipleView, FLAGS_render_pose),
            (float)FLAGS_face_alpha_pose, (float)FLAGS_face_alpha_heatmap, (float)FLAGS_face_render_threshold};
        opWrapperT.configure(wrapperStructFace);
        // Hand configuration (use op::WrapperStructHand{} to disable it)
        const op::WrapperStructHand wrapperStructHand{
            FLAGS_hand, handDetector, handNetInputSize, FLAGS_hand_scale_number, (float)FLAGS_hand_scale_range,
            op::flagsToRenderMode(FLAGS_hand_render, multipleView, FLAGS_render_pose), (float)FLAGS_hand_alpha_pose,
            (float)FLAGS_hand_alpha_heatmap, (float)FLAGS_hand_render_threshold};
        opWrapperT.configure(wrapperStructHand);
        // Extra functionality configuration (use op::WrapperStructExtra{} to disable it)
        const op::WrapperStructExtra wrapperStructExtra{
            FLAGS_3d, FLAGS_3d_min_views, FLAGS_identification, FLAGS_tracking, FLAGS_ik_threads};
        opWrapperT.configure(wrapperStructExtra);
        // Output (comment or use default argument to disable any output)
        const op::WrapperStructOutput wrapperStructOutput{
            FLAGS_cli_verbose, op::String(FLAGS_write_keypoint), op::stringToDataFormat(FLAGS_write_keypoint_format),
            op::String(FLAGS_write_json), op::String(FLAGS_write_coco_json), FLAGS_write_coco_json_variants,
            FLAGS_write_coco_json_variant, op::String(FLAGS_write_images), op::String(FLAGS_write_images_format),
            op::String(FLAGS_write_video), FLAGS_write_video_fps, FLAGS_write_video_with_audio,
            op::String(FLAGS_write_heatmaps), op::String(FLAGS_write_heatmaps_format), op::String(FLAGS_write_video_3d),
            op::String(FLAGS_write_video_adam), op::String(FLAGS_write_bvh), op::String(FLAGS_udp_host),
            op::String(FLAGS_udp_port)};
        opWrapperT.configure(wrapperStructOutput);
        // No GUI. Equivalent to: opWrapper.configure(op::WrapperStructGui{});
        // Set to single-thread (for sequential processing and/or debugging and/or reducing latency)
        if (FLAGS_disable_multi_thread)
            opWrapperT.disableMultiThreading();
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
        opTimer = op::getTimerInit();

        // OpenPose wrapper
        op::opLog("Configuring OpenPose...", op::Priority::High);
        op::WrapperT<UserDatum> opWrapperT;
        configureWrapper(opWrapperT);

        // Start, run, and stop processing - exec() blocks this thread until OpenPose wrapper has finished
        op::opLog("Starting thread(s)...", op::Priority::High);
        opWrapperT.exec();
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

	if (ks->flags == 0 && ks->vkCode == 0x4c)//|| ks->flags == 129)
	{
		if (open)
		{
			open = false;
		}
		else if (!open)
		{
			open = true;
		}
		//tutorialApiCpp();
		// 监控键盘
		//cout << "vkCode:  " << ks->vkCode << endl;
		//switch (ks->vkCode) {
		//case 0x30: case 0x60:
		//	//cout << "检测到按键：" << "0" << endl;
		//	break;
		//case 0x31: case 0x61:
		//	//cout << "检测到按键：" << "1" << endl;
		//	/*worked = false;
		//	tutorialApiCpp();*/
		//	break;
		//case 0x4c:
		/*while (1)
		{
		worked = false;ll
		l
		tutorialApiCpp();
		Sleep(30);
		}*/

		//int x = 0;
		//int y = 0;
		//INPUT inputm;
		//int count = 100;
		//for (int i = 0; i <= count; i++) {
		//	inputm.mi.dx = (x - i) *(65535.0f / (GetSystemMetrics(SM_CXSCREEN) - 1));//x being coord in pixels
		//	inputm.mi.dy = (y)*(65535.0f / (GetSystemMetrics(SM_CXSCREEN) - 1));//y being coord in pixels
		//	inputm.mi.dwFlags = MOUSEEVENTF_MOVE;
		//	inputm.type = INPUT_MOUSE;
		//	SendInput(1, &inputm, sizeof(inputm));
		//	Sleep(1);
		//}
		//break;
		//}

		return 1;		// 使按键失效
	}

	// 将消息传递给钩子链中的下一个钩子
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}
void hook() {
	int ch;
	SetConsoleOutputCP(65001);// 更改cmd编码为utf8
							  // 安装钩子
	keyboardHook = SetWindowsHookEx(
		WH_KEYBOARD_LL,			// 钩子类型，WH_KEYBOARD_LL 为键盘钩子
		LowLevelKeyboardProc,	// 指向钩子函数的指针
		GetModuleHandleA(NULL),	// Dll 句柄
		NULL
		);
	if (keyboardHook == 0) { cout << "挂钩键盘失败" << endl; return ; }

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
}
int main(int argc, char *argv[])
{
	SetProcessAffinityMask(GetCurrentProcess(), 3);
	/*capture.open("D:/AI/libfacedetection-master/testcs.mp4");
	if (!capture.isOpened()) {
		printf("could not read this video file...\n");
		return -1;
	}*/
    // Parsing command line flags
    gflags::ParseCommandLineFlags(&argc, &argv, true);
	//cv::namedWindow("aim");
	//cv::resizeWindow("aim", cv::Size(width, height));
	//cv::moveWindow("aim", startx, starty);
	thread p1(tutorialApiCpp);
	thread p2(hook);
	
	p2.join();
    // Running tutorialApiCpp
	return 0;// tutorialApiCpp();
}
