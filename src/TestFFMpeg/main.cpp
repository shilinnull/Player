#if 0
#include <iostream>
extern "C" 
{
#include <libavcodec/avcodec.h>
}
#pragma comment(lib,"avcodec.lib")
using namespace std;
int main(int argc, char *argv[])
{
	cout << "Test FFmpeg " << endl;
#ifdef WIN32
	cout << "32 位程序" << endl;
#else
	cout << "64 位程序" << endl;
#endif
	cout << avcodec_configuration() << endl;
	cout << avcodec_version() << endl;
	cout << avcodec_license() << endl;
	return 0;
}
#endif

extern"C" {
#include "libavformat/avformat.h"
#include "libavutil/error.h"
}
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avutil.lib")

#include <string>
#include <iostream>

using namespace std;

int main() {
	const string path = "111.mp4";
	// 初始化封装库
	av_register_all();
	// 初始化网络库
	avformat_network_init();
	// 添加参数
	AVDictionary* opts = NULL;
	av_dict_set(&opts, "rtsp_transport", "tcp", 0);
	av_dict_set(&opts, "max_delay", "500", 0);
	// 解封装上下文
	AVFormatContext* ic = NULL;
	int ret = avformat_open_input(&ic, path.c_str(), 0, &opts);
	if (ret != 0)
	{
		char buf[1024]{ 0 };
		av_strerror(ret, buf, sizeof buf - 1);
		cout << "open " << path << " failed! :" << buf << endl;
		return -1;
	}
	cout << "open " << path << " success! " << endl;

	// 获取流信息

	return 0;
}