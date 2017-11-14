/**
* Sample
* 1: Auto get parameter;
* 2: Analysis video, get movement object, through OpenCV decode.
* Sandy Yann, Nov 14, 2017
*/

#include "extract_obj.h"
#include "common.h"

#include <opencv2/opencv.hpp>

/**
* @brief Auto get parameter sample
* @param psrcfn : vedio file name.
* @return void* : auto parameter handle.
*/
void* ExtractObjTestGetAutoParam(const char *psrcfn)
{
	cv::Mat img;
	uint8_t* ptCurYuv = NULL;

	cv::VideoCapture cap(psrcfn);
	if (!cap.isOpened()) {
		printf("can't open: %s\n", psrcfn);
		return 0;
	}
	cap.read(img);

	void* pvAPHandle = NULL;
	pvAPHandle = ccAutoParamOpen(img.cols, img.rows, 300);
	if (NULL == pvAPHandle) {
		printf("ccAutoParamOpen fail\n");
		return NULL;
	}

	int ret = 1;
	while (cap.read(img)) {
		static int pts = 0;
		if (ptCurYuv == NULL) {
			ptCurYuv = new uint8_t[img.cols * img.rows * 3 / 2];
		}
		
		bgr24_to_yuv(ptCurYuv, img.data, img.cols, img.rows, img.step);

		Input input = { 0 };
		input.dataType = YUV;
		input.pData = 0;
		input.width = img.cols;
		input.height = img.rows;
		input.pts = pts += 40;

		ret = ccAutoParamProcess(pvAPHandle, &input);
		if (1 != ret)
		{
			break;
		}

		//cv::namedWindow("s", 0);
		//cv::imshow("s", img);
		//cv::waitKey(1);
		printf("\rprocess idx = %d         ", pts / 40);
	}

	if(ptCurYuv) { 
		delete[] ptCurYuv;
	}

	// Had finished intial parameter.
	if (ret == 2) {
		// Get parameter handle.
		void* pvAParam = ccGetAutoParam(pvAPHandle);
		if (NULL == pvAParam) {
			printf("ccGetAutoParam fail\n");
		}

		ccAutoParamClose(&pvAPHandle);
		return pvAParam;
	}

	ccAutoParamClose(&pvAPHandle);
	return NULL;
}

void realtime_show_detect_result(cv::Mat img, const Output& output);
{
	for (int i = 0; i < output.tDetObj.num; i++)
	{
		cv::Rect rt = cv::Rect(output.tDetObj.atRtRoi[i].x,
			output.tDetObj.atRtRoi[i].y,
			output.tDetObj.atRtRoi[i].w,
			output.tDetObj.atRtRoi[i].h);
		cv::rectangle(rsz, rt, cv::Scalar(255, 0, 0), 1);
	}
	cv::imshow("detect", rsz);
	cv::waitKey(1);
}

void show_report_result(const Output& output)
{
	for (int i = 0; i < output.num; i++)
	{
		printf("report obj pts = %I64d, obj_id = %I64d \n",
			output.aRptObj[i].pts, output.aRptObj[i].u64ObjID);

		cv::Mat rptImg = cv::Mat(output.aRptObj[i].tRoiRt.h, output.aRptObj[i].tRoiRt.w, CV_8UC3, output.aRptObj[i].pBGR24);
		//cv::rectangle(rptImg, rt, cv::Scalar(255, 0, 0), 1);
		cv::imshow("rpt", rptImg);
		cv::waitKey(0);
	}
}

/**
* @brief Analysis vedio, extract movement objects.
* @param psrcfn : vedio file name.
* @param pdstfn : result video file, draw result to this video.
*/
int ExtractObjTest(const char *psrcfn, const char *pdstfn, void *pvAParam/* param handle */)
{
	cv::Mat img;
	cv::Mat rsz;
	uint8_t* pYuv = NULL;
	std::string dstPath = std::string(pdstfn) + "_ReportImg";

	cv::VideoCapture cap(psrcfn);
	if (!cap.isOpened()) {
		printf("can't open: %s\n", psrcfn);
		return 0;
	}
	cap.read(img);
	cv::resize(img, rsz, cv::Size(352, 288));

	void* pvHandle = NULL;

	pvHandle = ccObjExtractOpen(rsz.cols, rsz.rows, pvAParam);

	// Release parameter handle
	ccReleaseAutoParam(&pvAParam);

	while (cap.read(img))
	{
		cv::resize(img, rsz, cv::Size(352, 288));
		static int pts = 0;
		if (pYuv == NULL) {
			pYuv = new uint8_t[rsz.cols * rsz.rows * 3 / 2];
		}
		bgr24_to_yuv(pYuv, rsz.data, rsz.cols, rsz.rows, rsz.step);
		Input input = { 0 };
		input.dataType = YUV;
		input.pData = pYuv;
		input.width = rsz.cols;
		input.height = rsz.rows;
		input.pts = pts += 40;

		Output output = { 0 };
		ccObjExtractProcess(pvHandle, &input, &output);

		// real-time show detect result
#if 1
		realtime_show_detect_result(rsz, output);
#endif
		// show report result
#if 1
		show_report_result(output);
#endif

#if 1	// 保存上报结果
		saveRptResult(output);
#endif
		printf("\rprocess idx = %d         ", pts / 40);

		if (pts > 40 * 1000) {
			break;
		}
	}

	TObjExtractOut output = { 0 };
	ccObjExtractGetRemain(pvHandle, &output);
#if 1	// 保存上报结果
	saveRptResult(output);
#endif
	ccObjExtractClose(&pvHandle);

	//vwrite.release();

	return 1;
}

int main(int argc, char** argv)
{
	const char *psrcfn, *pdstfn, *svfolder;
	if (argc != 4)
	{
		printf("param argv[1] = src video fn\nargv[2] = dst video fn\n");
		psrcfn = "C:\\XipingYan_Code\\OpenSourceCode\\mygithub\\LP-Detection-Recognition\\windows\\192.168.1.45_ch1_20140527_104944_20140527104951_20140527112640.avi";
		pdstfn = "dst.avi";
		svfolder = "default_save_folder";
	}
	else {
		psrcfn = argv[1];
		pdstfn = argv[2];
		svfolder = argv[3];
	}

	// Auto get parameter
	void *pvAParam = ExtractObjTestGetAutoParam(psrcfn);

	// or manual set parameter.

	// Start analysis. pvAParam will be released in 'ExtractObjTest'
	ExtractObjTest(psrcfn, pdstfn, pvAParam);

	return 1;
}
