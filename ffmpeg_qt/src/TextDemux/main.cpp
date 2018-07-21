#include <iostream>
#include <thread>
extern "C" {
#include "libavformat/avformat.h"
}

using namespace std;

#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"avcodec.lib")
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
	const char *path = "v1080.mp4";
	//��ʼ����װ��
	av_register_all();

	//��ʼ�������(���Դ�rtsp-��������ͷ rtmp-����ֱ�� http-������Ƶ Э�����ý����Ƶ)
	avformat_network_init();

	//��������
	AVDictionary *opts = NULL;

	//����rtsp��tcpЭ���
	av_dict_set(&opts, "rtsp_transport", "tcp", 0);

	//������ʱʱ��
	av_dict_set(&opts, "max_delay", "500", 0);

	//���װ������
	AVFormatContext *ic = NULL;
	int re = avformat_open_input(
		&ic,
		path,
		0,		//0��ʾ�Զ�ѡ����װ��
		&opts	//��������,����rtsp����ʱʱ��
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
	
	//��ȡ����Ϣ
	re = avformat_find_stream_info(ic, 0);
	
	//��ʱ��-����
	int totalMs = ic->duration / (AV_TIME_BASE / 1000);
	cout << "totalMs = " << totalMs << endl;

	//��ӡ��Ƶ������ϸ��Ϣ
	av_dump_format(ic,0,path,0);

	//����Ƶ����,��ȡʱ��������Ƶ

	int videoStream = 0;
	int audioStream = 1;

	for (int i = 0; i < ic->nb_streams; i++) {
		AVStream *as = ic->streams[i];
		cout << "format = " << as->codecpar->format << endl;
		cout << "codec_id = " << as->codecpar->codec_id << endl;
		//��Ƶ AVMEDIA_TYPE_AUDIO
		if (as->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audioStream = i;
			cout << i << " = ��Ƶ��Ϣ" << endl;
			cout << "sample_rate = " << as->codecpar->sample_rate << endl;
			//AVSampleFormat
			cout << "channels = " << as->codecpar->channels << endl;
			//һ֡����--��ͨ����������
			cout << "frame_size = " << as->codecpar->frame_size << endl;
		}
		//��Ƶ AVMEDIA_TYPE_VIDEO
		else if (as->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoStream = i;
			cout << i << " = ��Ƶ��Ϣ" << endl;
			cout << "width = " << as->codecpar->width << endl;
			cout << "height = " << as->codecpar->height << endl;
			cout << "fps = " << r2d(as->avg_frame_rate) << endl;
		}
	}

	//��ȡ��Ƶ����Ϣ
	videoStream = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	///ic->stream[videoStream]

	//malloc AVPacket ����ʼ��
	AVPacket *pkt = av_packet_alloc();
	for (;;)
	{
		int re = av_read_frame(ic, pkt);
		if (re != 0) {
			cout << "========================================end========================================";
			int ms = 3000; //����λ��
			long long pos = (double)ms/ (double)1000 *r2d(ic->streams[pkt->stream_index]->time_base);
			av_seek_frame(ic, videoStream, pos, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);
			continue;
		}
		cout << "pkt->size = "<< pkt->size << endl;
		//��ʾ��ʱ��
		cout << "pkt->pts = " << pkt->pts << endl;
		//�����ʱ��
		cout << "pkt->dts = " << pkt->dts << endl;
		//ת��Ϊ����,������ͬ��
		cout << "pkt->pts ms = " << pkt->pts * (r2d(ic->streams[pkt->stream_index]->time_base) * 1000) << endl;


		if (pkt->stream_index== videoStream)
		{
			cout << "ͼ��" << endl;
		}

		if (pkt->stream_index == audioStream)
		{
			cout << "��Ƶ" << endl;
		}

		//�ͷ�,���ü���-1 Ϊ0�ͷſռ�
		av_packet_unref(pkt);

		//XSleep(500);
	}

	av_packet_free(&pkt);



	if (ic) {
		//�ͷŷ�װ������,����ic����
		avformat_close_input(&ic);
	}

	getchar();
	return 0;
}