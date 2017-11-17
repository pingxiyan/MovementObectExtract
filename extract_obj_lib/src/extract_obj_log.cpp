#include "extract_obj_log.h"
#include "extract_obj_lock.h"

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <string.h>

#ifdef _WIN32
#include <Windows.h>
#include <time.h>
#include <io.h>
#else
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#endif


#define SHOW_LOG_CONSOLE 1	// 
#define LOG_NAME	"mylog"	// 

static bool g_bWriteFile = false;
static std::string g_LogFileName = LOG_NAME; //getLogFileName(std::string());
static CThreadLock g_threadLock("log_mutex");

/**
* @param log need to support multi-thread.
*/


#ifdef _WIN32
std::string getExePath()
{
	char szFilePath[MAX_PATH + 1] = { 0 };
	GetModuleFileNameA(NULL, szFilePath, MAX_PATH);
	(strrchr(szFilePath, '\\'))[0] = 0; //   
	std::string path = szFilePath;
	return path;
}
#else
std::string getExePath()
{
	char exec_name[1024] = { 0 };
	readlink("/proc/self/exe", exec_name, 1024);
	return std::string(exec_name);
}
#endif

static std::string get_fn_from_fullfn(std::string fullfn)
{
#ifdef _WIN32
	int pos = fullfn.rfind("\\");
#else
	int pos = fullfn.rfind("/");
#endif
	std::string fn = fullfn.substr(pos + 1, fullfn.length());
	return fn;
}

static void judgeTimeAndDel(std::string strFullFn)
{
	std::string fn = get_fn_from_fullfn(strFullFn);
	int curY = 0;
	int curM = 0;
	int curD = 0;
	int oldY = 0;
	int oldM = 0;
	int oldD = 0;
	std::string strTemp = fn.substr(fn.find("_") + 1, fn.length() - fn.find("_") - 1);
	sscanf(strTemp.c_str(), "%d-%d-%d", &oldY, &oldM, &oldD);

	time_t curTm = time(0);
	char tmp[64] = { 0 };
	strftime(tmp, sizeof(tmp), "%Y-%m-%d-%H-%M-%S", localtime(&curTm));
	sscanf(tmp, "%d-%d-%d", &curY, &curM, &curD);

	std::cout << strFullFn << std::endl;

	if (oldY == 0 || oldM == 0 || oldD == 0) {
		return;
	}

	if (curY * 12 * 30 + curM * 30 + curD - oldY * 12 * 30 - oldM * 30 - oldD >= 28) {
		remove(strFullFn.c_str());
		std::cout << " had deleted! " << std::endl;
	}
}

/**
* @brief Delete log.txt before more one month, or all '*.log' files
* @param bOneMonth:
*	true: delete *.log created before one month.
*	false: delete all *.log
*/
static void delOneMonthAgoLog(bool bOneMonth)
{
	std::string strRootPath = getExePath();

#ifdef _WIN32
	std::string strLogFullName = strRootPath + "\\" + "*.log";

	intptr_t Handle;
	struct _finddata_t FileInfo;
	if ((Handle = _findfirst(strLogFullName.c_str(), &FileInfo)) == -1L) {
		// printf("Can' find '*.log' file\n");
		return;
	}
	else {
		std::string curFullFn = strRootPath + "\\" + FileInfo.name;
		if (bOneMonth) {
			judgeTimeAndDel(curFullFn);
		}
		else {	// Directly delete.
			std::cout << curFullFn;
			remove(curFullFn.c_str());
			std::cout << " had deleted" << std::endl;
		}

		while (_findnext(Handle, &FileInfo) == 0) {
			curFullFn = strRootPath + "\\" + FileInfo.name;
			if (bOneMonth) {
				judgeTimeAndDel(curFullFn);
			}
			else { // Directly delete.
				std::cout << curFullFn;
				remove(curFullFn.c_str());
				std::cout << " had deleted" << std::endl;
			}
		}

		_findclose(Handle);
	}
#else
	unsigned int count = 0;		//临时计数，[0，SINGLENUM]  
	char txtname[128];			//存放文本文件名  
	DIR *dp;
	struct dirent *dirp;

	// opendir, readdir, closedir
	dp = opendir(strRootPath.c_str());
	if (dp == NULL)
	{
		perror("opendir");
	}

	// loop all files.  
	while ((dirp = readdir(dp)) != NULL)
	{
		if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0)
			continue;

		int size = strlen(dirp->d_name);

		// file length < 5, can't is *.log file.  
		if (size<5)
			continue;

		// find all *.log file
		if (strcmp((dirp->d_name + (size - 4)), ".log") != 0)
			continue;

		std::cout << dirp->d_name << std::endl;
	}

	closedir(dp);
#endif
}

static std::string getLogFileName(std::string strLogFn)
{
	time_t t = time(0);
	char tmp[64];
	strftime(tmp, sizeof(tmp), "%Y-%m-%d-%H-%M-%S", localtime(&t));

	strLogFn = strLogFn.empty() ? LOG_NAME : strLogFn;

	strLogFn = strLogFn + "_" + std::string(tmp) + ".log";

	return strLogFn;
}

std::string getCurStrTime()
{
	time_t t = time(0);
	char tmp[64];
	strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&t));

	return std::string(tmp);
}

void printfLog(const char* pLogText)
{
	g_threadLock.lock();

	std::string strLogFullName = getExePath() + "\\" + g_LogFileName;

	static FILE* g_pFOut = NULL;
	if (g_pFOut == NULL)
	{
		g_pFOut = fopen(strLogFullName.c_str(), "a+");	// 如果文件不存在，则创建
		if (NULL == g_pFOut)
		{
			printf("can't write log\n");
			return;
		}
	}

	time_t t = time(0);
	char tmp[64];
	strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&t));

	fprintf(g_pFOut, "%s %s", tmp, pLogText);
	fflush(g_pFOut);

	printf("%s %s\n", tmp, pLogText);

	g_threadLock.unlock();
}

bool InitialLog(const char* pLogFn, bool bLogNameAddTime, bool bDelOldLog, bool bWriteFile)
{
	std::cout << "Start init log ..." << std::endl;

	g_bWriteFile = bWriteFile;
	delOneMonthAgoLog(bDelOldLog ? false : true);

	g_LogFileName = bLogNameAddTime ? getLogFileName(pLogFn) : (std::string(pLogFn) + ".log");

	std::cout << "Init log finish, log name = " << g_LogFileName << std::endl
		<< "*********************************************************"
		<< std::endl << std::endl;
	return true;
}