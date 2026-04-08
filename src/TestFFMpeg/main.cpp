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
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavutil/error.h"
}
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "swresample.lib")

#include <string>
#include <iostream>
#include <thread>
#include <windows.h>

using namespace std;

void XSleep(uint64_t ms) {
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

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
	for (unsigned int i = 0; i < ic->nb_streams; i++) {
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
	//////////////////////////////////////////////////////////
    ///视频解码器打开
    ///找到视频解码器
	AVCodec* vcodec = avcodec_find_decoder(ic->streams[videoStream]->codecpar->codec_id);
	if (!vcodec) {
		cout << "can't find the codec id " << ic->streams[videoStream]->codecpar->codec_id;
		return -1;
	}
	cout << "find the AVCodec " << ic->streams[videoStream]->codecpar->codec_id << endl;

	AVCodecContext* vc = avcodec_alloc_context3(vcodec);
	///配置解码器上下文参数
	avcodec_parameters_to_context(vc, ic->streams[videoStream]->codecpar);
	vc->thread_count = 8; // 多线程解码

	// 打开解码器
	ret = avcodec_open2(vc, 0, 0);
	if (ret != 0) {
		char buf[1024] = { 0 };
		av_strerror(ret, buf, sizeof(buf) - 1);
		cout << "avcodec_open2  failed! :" << buf << endl;
		return -1;
	}
	cout << "video avcodec_open2 success!" << endl;

	//////////////////////////////////////////////////////////
    ///音频解码器打开
	AVCodec* acodec = avcodec_find_decoder(ic->streams[audioStream]->codecpar->codec_id);
	if (!acodec)
	{
		cout << "can't find the codec id " << ic->streams[audioStream]->codecpar->codec_id;
		return -1;
	}
	cout << "find the AVCodec " << ic->streams[audioStream]->codecpar->codec_id << endl;
	///创建解码器上下文呢
	AVCodecContext* ac = avcodec_alloc_context3(acodec);

	///配置解码器上下文参数
	avcodec_parameters_to_context(ac, ic->streams[audioStream]->codecpar);
	//八线程解码
	ac->thread_count = 8;

	///打开解码器上下文
	ret = avcodec_open2(ac, 0, 0);
	if (ret != 0)
	{
		char buf[1024] = { 0 };
		av_strerror(ret, buf, sizeof(buf) - 1);
		cout << "avcodec_open2  failed! :" << buf << endl;
		return -1;
	}
	cout << "audio avcodec_open2 success!" << endl;

	//malloc AVPacket并初始化
	AVPacket* pkt = av_packet_alloc();
	AVFrame* frame = av_frame_alloc();

	//像素格式和尺寸转换上下文
	SwsContext* vctx = NULL;
	unsigned char* rgb = NULL;

	//音频重采样 上下文初始化
	SwrContext* actx = swr_alloc();
	actx = swr_alloc_set_opts(actx,
							  av_get_default_channel_layout(2),	//输出格式
							  AV_SAMPLE_FMT_S16,				//输出样本格式
							  ac->sample_rate,					//输出采样率
							  av_get_default_channel_layout(ac->channels),//输入格式
							  ac->sample_fmt,
							  ac->sample_rate,
							  0, 0
	);
	ret = swr_init(actx);
	if (ret != 0)
	{
		char buf[1024] = { 0 };
		av_strerror(ret, buf, sizeof(buf) - 1);
		cout << "swr_init  failed! :" << buf << endl;
		return -1;
	}
	unsigned char* pcm = NULL;

	for (;;)
	{
		int ret = av_read_frame(ic, pkt);
		if (ret != 0) {
			cout << "==============================end==============================" << endl;
			int ms = 3000; // 第三秒的位置
			int64_t pos = (double)ms / (double)1000 * r2d(ic->streams[pkt->stream_index]->time_base);
			av_seek_frame(ic, videoStream, pos, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);
			continue;
		}
		cout << "pkt->size = " << pkt->size << endl;
		//显示的时间
		cout << "pkt->pts = " << pkt->pts << endl;

		//转换为毫秒，方便做同步
		cout << "pkt->pts ms = " << pkt->pts * (r2d(ic->streams[pkt->stream_index]->time_base) * 1000) << endl;

		AVCodecContext* cc = 0;

		//解码时间
		cout << "pkt->dts = " << pkt->dts << endl;
		if (pkt->stream_index == videoStream)
		{
			cout << "图像" << endl;
			cc = vc;
		}
		if (pkt->stream_index == audioStream)
		{
			cout << "音频" << endl;
			cc = ac;
		}
		///解码视频
		//发送packet到解码线程  send传NULL后调用多次receive取出所有缓冲帧
		ret = avcodec_send_packet(cc, pkt);
		//释放，引用计数-1 为0释放空间
		av_packet_unref(pkt);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			cout << "avcodec_send_packet  failed! :" << buf << endl;
			continue;
		}

		for (;;)
		{
			//从线程中获取解码接口,一次send可能对应多次receive
			ret = avcodec_receive_frame(cc, frame);
			if (ret != 0) break;
			cout << "recv frame " << frame->format << " " << frame->linesize[0] << endl;

			//视频
			if (cc == vc)
			{
				vctx = sws_getCachedContext(
					vctx,	//传NULL会新创建
					frame->width, frame->height,	//输入的宽高
					(AVPixelFormat)frame->format,	//输入格式 YUV420p
					frame->width, frame->height,	//输出的宽高
					AV_PIX_FMT_RGBA,				//输入格式RGBA
					SWS_BILINEAR,					//尺寸变化的算法
					0, 0, 0);
				//if(vctx)
					//cout << "像素格式尺寸转换上下文创建或者获取成功！" << endl;
				//else
				//	cout << "像素格式尺寸转换上下文创建或者获取失败！" << endl;
				if (vctx)
				{
					if (!rgb) rgb = new unsigned char[frame->width * frame->height * 4];
					uint8_t* data[2] = { 0 };
					data[0] = rgb;
					int lines[2] = { 0 };
					lines[0] = frame->width * 4;
					ret = sws_scale(vctx,
								   frame->data,		//输入数据
								   frame->linesize,	//输入行大小
								   0,
								   frame->height,		//输入高度
								   data,				//输出数据和大小
								   lines
					);
					cout << "sws_scale = " << ret << endl;
				}

			}
			else //音频
			{
				uint8_t* data[2] = { 0 };
				if (!pcm) pcm = new uint8_t[frame->nb_samples * 2 * 2];
				data[0] = pcm;
				ret = swr_convert(actx,
								 data, frame->nb_samples,		//输出
								 (const uint8_t**)frame->data, frame->nb_samples	//输入
				);
				cout << "swr_convert = " << ret << endl;
			}
		}
		XSleep(1000);
	}
	av_frame_free(&frame);
	av_packet_free(&pkt);
	if (ic) {
		avformat_close_input(&ic);
	}
	
	return 0;
}