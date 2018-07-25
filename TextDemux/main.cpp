#include <iostream>
#include <thread>
extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}

using namespace std;

#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"swscale.lib")
#pragma comment(lib,"swresample.lib")

static double r2d(AVRational r) {
	return r.den == 0 ? 0 : (double)r.num / (double)r.den;
}

void XSleep(int ms) {
	//C++11
	chrono::milliseconds du(ms);
	this_thread::sleep_for(du);
}

int main() {
	cout << "Text Demux" << endl;
	const char *path = "test.flv";
	//初始化封装库
	av_register_all();

	//初始化网络库(可以打开rtsp-网络摄像头 rtmp-网络直播 http-网络视频 协议的流媒体视频)
	avformat_network_init();

	//注册解码器
	avcodec_register_all();


	//参数设置
	AVDictionary *opts = NULL;

	//设置rtsp流tcp协议打开
	av_dict_set(&opts, "rtsp_transport", "tcp", 0);

	//网络延时时间
	av_dict_set(&opts, "max_delay", "500", 0);

	//解封装上下文
	AVFormatContext *ic = NULL;
	int re = avformat_open_input(
		&ic,
		path,
		0,		//0表示自动选择解封装器
		&opts	//参数设置,比如rtsp的延时时间
		);

	if (re != 0)
	{
		char buf[1024] = { 0 };
		av_strerror(re, buf, sizeof(buf) - 1);
		cout << "open " << path << "failed! :" << buf << endl;
		getchar();
		return -1;
	}
	cout << "open " << path << "Succeed!" << endl;
	
	//获取流信息
	re = avformat_find_stream_info(ic, 0);
	
	//总时长-毫秒
	int totalMs = ic->duration / (AV_TIME_BASE / 1000);
	cout << "totalMs = " << totalMs << endl;

	//打印视频流的详细信息
	av_dump_format(ic,0,path,0);

	//音视频索引,读取时区分音视频

	int videoStream = 0;
	int audioStream = 1;

	//获取音视频流信息 (遍历,函数获取)
	for (int i = 0; i < ic->nb_streams; i++) {
		AVStream *as = ic->streams[i];
		cout << "format = " << as->codecpar->format << endl;
		cout << "codec_id = " << as->codecpar->codec_id << endl;
		//音频 AVMEDIA_TYPE_AUDIO
		if (as->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audioStream = i;
			cout << i << " = 音频信息" << endl;
			cout << "sample_rate = " << as->codecpar->sample_rate << endl;
			//AVSampleFormat
			cout << "channels = " << as->codecpar->channels << endl;
			//一帧数据--单通道的样本数
			cout << "frame_size = " << as->codecpar->frame_size << endl;
		}
		//视频 AVMEDIA_TYPE_VIDEO
		else if (as->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoStream = i;
			cout << i << " = 视频信息" << endl;
			cout << "width = " << as->codecpar->width << endl;
			cout << "height = " << as->codecpar->height << endl;
			cout << "fps = " << r2d(as->avg_frame_rate) << endl;
		}
	}

	//获取视频流信息
	videoStream = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	///ic->stream[videoStream]


	/////////////////////////////////////////////////////////////////////
	//视频解码器打开
	//找到解码器
	AVCodec *vcodec = avcodec_find_decoder(ic->streams[videoStream]->codecpar->codec_id);
	if (!vcodec)
	{
		cout << "can't find the codec id" << ic->streams[videoStream]->codecpar->codec_id<<endl;
		getchar();
		return -1;
	}
	cout << "find the AVCodec " << ic->streams[videoStream]->codecpar->codec_id << endl;
	

	///创建解码器上下文
	AVCodecContext *vc = avcodec_alloc_context3(vcodec);
	///配置解码器上下文参数
	avcodec_parameters_to_context(vc, ic->streams[videoStream]->codecpar);
	
	vc->thread_count = 8;
	///打开解码器上下文
	re = avcodec_open2(vc, 0, 0);
	if (re != 0)
	{
		char buf[1024] = { 0 };
		av_strerror(re, buf, sizeof(buf) - 1);
		cout << "avcodec_open2 " << path << "failed! :" << buf << endl;
		getchar();
		return -1;
	}
	cout << "video avcodec_open2 success" << endl;

	/////////////////////////////////////////////////////////////////////
	//音频解码器打开
	//找到解码器
	AVCodec *acodec = avcodec_find_decoder(ic->streams[audioStream]->codecpar->codec_id);
	if (!acodec)
	{
		cout << "can't find the codec id" << ic->streams[audioStream]->codecpar->codec_id << endl;
		getchar();
		return -1;
	}
	cout << "find the AVCodec " << ic->streams[audioStream]->codecpar->codec_id << endl;
	///创建解码器上下文
	AVCodecContext *ac = avcodec_alloc_context3(acodec);
	///配置解码器上下文参数
	avcodec_parameters_to_context(ac, ic->streams[audioStream]->codecpar);

	//八线程解码
	ac->thread_count = 8;
	///打开解码器上下文
	re = avcodec_open2(ac, 0, 0);
	if (re != 0)
	{
		char buf[1024] = { 0 };
		av_strerror(re, buf, sizeof(buf) - 1);
		cout << "avcodec_open2 " << path << "failed! :" << buf << endl;
		getchar();
		return -1;
	}
	cout << "audio avcodec_open2 success" << endl;

	//malloc AVPacket 并初始化
	AVFrame *frame = av_frame_alloc();
	AVPacket *pkt = av_packet_alloc();

	//像素格式和尺寸转换上下文
	SwsContext *vctx = NULL;
	unsigned char *rgb = NULL;

	//音频重采样,上下文初始化
	SwrContext *actx = swr_alloc();
	actx = swr_alloc_set_opts(actx,
		av_get_default_channel_layout(2),	//输出格式
		AV_SAMPLE_FMT_S16,					//输出样本格式
		ac->sample_rate,					//输出采样率
		av_get_default_channel_layout(ac->channels),//输入格式
		ac->sample_fmt,
		ac->sample_rate,
		0, 0
		);
	re = swr_init(actx);
	if (re != 0)
	{
		char buf[1024] = { 0 };
		av_strerror(re, buf, sizeof(buf) - 1);
		cout << "swr_init failed! :" << buf << endl;
		getchar();
		return -1;
	}
	unsigned char *pcm = NULL;

	for (;;)
	{
		int re = av_read_frame(ic, pkt);
		if (re != 0) {
			cout << "========================================end========================================";
			int ms = 3000; //三秒位置
			long long pos = (double)ms/ (double)1000 *r2d(ic->streams[pkt->stream_index]->time_base);
			av_seek_frame(ic, videoStream, pos, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);
			continue;
		}
		cout << "pkt->size = "<< pkt->size << endl;
		//显示的时间
		cout << "pkt->pts = " << pkt->pts << endl;
		//解码的时间
		cout << "pkt->dts = " << pkt->dts << endl;
		//转换为毫秒,方便做同步
		cout << "pkt->pts ms = " << pkt->pts * (r2d(ic->streams[pkt->stream_index]->time_base) * 1000) << endl;

		//解码时间
		cout << "pkt->dts=" << pkt->dts << endl;
		AVCodecContext *cc = 0;
		if (pkt->stream_index== videoStream)
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
		//发送packet到解码线程,不占用cpu,send传NULL后调用receive取出所有缓冲帧
		re = avcodec_send_packet(cc, pkt);
		//释放,引用计数-1 为0释放空间
		av_packet_unref(pkt);
		if (re !=0) {
			char buf[1024] = { 0 };
			av_strerror(re, buf, sizeof(buf) - 1);
			cout << "avcodec_open2 " << path << "failed! :" << buf << endl;
			continue;
		}

		for (;;)
		{
			//不占用cpu,从线程中获取解码接口,一次send可能对应多次receive
			re = avcodec_receive_frame(cc, frame);
			if (re != 0)break;
			cout << "recev_frame" << frame->format << " " << frame->linesize[0] << endl;

			if (cc == vc)
			{
				vctx = sws_getCachedContext(
					vctx,							//传NULL会新创建
					frame->width, frame->height,		//输入的宽高
					(AVPixelFormat)frame->format,	//输入格式YUV420p
					frame->width, frame->height,		//输入的宽高
					AV_PIX_FMT_RGBA,				//输入格式RGBA
					SWS_BILINEAR,					//尺寸变化的算法
					0, 0, 0);
				//if (vctx)
					//cout << "像素格式转换上下文创建或者获取成功!" << endl;
				//else
					//cout << "像素格式转换上下文创建或者获取失败!" << endl;
				if (vctx)
				{
					if (!rgb)rgb = new unsigned char[frame->width*frame->height * 4];
					
						uint8_t *data[2] = { 0 };
						data[0] = rgb;
						int lines[2] = { 0 };
						lines[0] = frame->width * 4;
						re = sws_scale(vctx,
							frame->data,		//输入数据
							frame->linesize,	//输入行大小	
							0,
							frame->height,		//输入高度
							data,				//输出数据和大小
							lines
						);
						cout << "sws_scale = " << re << endl;
				}
				
			}
			else //音频
			{
				uint8_t *data[2] = { 0 };
				if (!pcm) pcm = new uint8_t[frame->nb_samples * 4];
				data[0] = pcm;
				re = swr_convert(actx,
					data,
					frame->nb_samples,//输出
					(const uint8_t**)frame->data,
					frame->nb_samples//输入
				);
				cout << "swr_convert = " << re << endl;
			}
		}

		//XSleep(500);
	}

	av_frame_free(&frame);
	av_packet_free(&pkt);



	if (ic) {
		//释放封装上下文,并把ic置零
		avformat_close_input(&ic);
	}

	getchar();
	return 0;
}