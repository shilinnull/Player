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
#include "libavcodec/avcodec.h"
#include "libavutil/error.h"
}
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib, "avcodec.lib")

#include <string>
#include <iostream>
#include <windows.h>

using namespace std;

static double r2d(AVRational r) {
	return r.num == 0 || r.den == 0 ? 0 : (double)r.num / (double)r.den;
}

int main() {
	const string path = "111.mp4";
	// 初始化封装库
	//av_register_all();
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
	ret = avformat_find_stream_info(ic, 0);
	
	// 总时长
	int64_t totalMs = ic->duration / (AV_TIME_BASE / 1000);
	cout << "总时长 = " << totalMs << " ms" << endl;

	// 打印视频流详细信息
	av_dump_format(ic, 0, path.c_str(), 0);

	// 音视频索引，读取时区分音视频
	int videoStream = 0;
	int audioStream = 1;

	// 获取音视频流信息
	for (int i = 0; i < ic->nb_streams; i++) {
		AVStream* as = ic->streams[i];
		cout << "codec_id = " << as->codecpar->codec_id << endl;
		cout << "format = " << as->codecpar->format << endl;

		// 视频流
		if (as->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			cout << i << "视频信息" << endl;
			cout << "width=" << as->codecpar->width << endl;
			cout << "height=" << as->codecpar->height << endl;
			cout << "video fps = " << r2d(as->avg_frame_rate) << endl;
		}
		else if (as->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) // 音频流
		{
			audioStream = i;
			cout << i << "音频信息" << endl;
			cout << "sample_rate=" << as->codecpar->sample_rate << endl;
			cout << "channels=" << as->codecpar->channels << endl;
			cout << "frame_size = " << as->codecpar->frame_size << endl;
		}
	}
	cout << "===============================================================" << endl;
	// 获取视频流
	videoStream = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	AVPacket* pkt = av_packet_alloc();
	for (;;)
	{
		int ret = av_read_frame(ic, pkt);
		if (ret != 0) {
			cout << "==============================end==============================" << endl;
			int ms = 3000; // 第三秒的位置
			long long pos = (double)ms / (double)1000 * r2d(ic->streams[pkt->stream_index]->time_base);
			av_seek_frame(ic, videoStream, pos, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);
			continue;
		}
		cout << "pkt->size = " << pkt->size << endl;
		//显示的时间
		cout << "pkt->pts = " << pkt->pts << endl;

		//转换为毫秒，方便做同步
		cout << "pkt->pts ms = " << pkt->pts * (r2d(ic->streams[pkt->stream_index]->time_base) * 1000) << endl;

		//解码时间
		cout << "pkt->dts = " << pkt->dts << endl;
		if (pkt->stream_index == videoStream)
		{
			cout << "图像" << endl;
		}
		if (pkt->stream_index == audioStream)
		{
			cout << "音频" << endl;
		}
		//释放，引用计数-1 为0释放空间
		av_packet_unref(pkt);
		//Sleep(1000);
	}
	av_packet_free(&pkt);
	if (ic) {
		avformat_close_input(&ic);
	}
	
	return 0;
}