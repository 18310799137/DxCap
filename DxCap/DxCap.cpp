// DxCap.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include <stdio.h>
#include "directx_cap.h"
#include <mmsystem.h>
#include <stdlib.h>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}


void CALLBACK write_frame_callback(UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);

directx_cap cap;
AVPacket avpkt;
int size = 0;
DWORD* buffer = NULL;
AVFrame *rgb_frame_ptr = NULL;
AVFrame *yuv_frame_ptr = NULL;
uint8_t * yuv_buff = NULL;
SwsContext * scxt = NULL;
AVCodecContext *c = NULL;
uint8_t * outbuf = NULL;
int outbuf_size = 0;
int u_size = 0;
FILE *f = NULL;

int frames = 0;


int main(int argc, char* argv[])
{
	if (!cap.init())
	{
		printf("��ʼ�� DirectXʧ��\n");
		return 1;
	}

	size = cap.get_disp_height()*cap.get_disp_width();
	buffer = new DWORD[size];

	av_register_all();
	avcodec_register_all();


	rgb_frame_ptr = new AVFrame[1];  //RGB֡����
	yuv_frame_ptr = new AVFrame[1];;  //YUV֡����
	yuv_frame_ptr->pts = 0;


	//����h264������
	AVCodec *h264_codec_ptr = avcodec_find_encoder(CODEC_ID_H264);
	if (!h264_codec_ptr)
	{
		printf("h264 codec not found\n");
		return 1;
	}

	c = avcodec_alloc_context3(h264_codec_ptr);
	c->bit_rate = 3000000;// put sample parameters 
	c->width = cap.get_disp_width();
	c->height = cap.get_disp_height();

	// frames per second 
	AVRational rate;
	rate.num = 1;
	rate.den = 25;
	c->time_base = rate;//(AVRational){1,25};
	c->gop_size = 10; // emit one intra frame every ten frames 
	c->max_b_frames = 1;
	c->thread_count = 1;
	c->pix_fmt = PIX_FMT_YUV420P;//PIX_FMT_RGB24;

	if (avcodec_open2(c, h264_codec_ptr, NULL) < 0)
	{
		printf("���ܴ򿪱����\n");
		return 1;
	}

	yuv_buff = new uint8_t[(size * 3) / 2]; // size for YUV 420 

	//ͼ�����
	outbuf_size = size*4;
	outbuf = new uint8_t[outbuf_size];
	char  filename[MAX_PATH];
	printf("������¼���ļ��ı���·����");
	scanf("%s",filename);
	f = fopen(filename, "wb");
	if (!f)
	{
		printf("could not open %s\n", filename);
		return 1;
	}

	//��ʼ��SwsContext
	scxt = sws_getContext(c->width, c->height, PIX_FMT_0RGB32, c->width, c->height, PIX_FMT_YUV420P, SWS_POINT, NULL, NULL, NULL);

	MMRESULT id = timeSetEvent(40, 1, &write_frame_callback, NULL, TIME_PERIODIC);

	printf("�����������¼��...");
	flushall();
	getchar();

	timeKillEvent(id);

	return 0;
}


void CALLBACK write_frame_callback(UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR)
{
	++frames;

	int index = (frames / 25) % 5;

	while (!cap.get_frame_to_buffer(size, buffer))
	{
		continue;
	}

	avpicture_fill((AVPicture*)rgb_frame_ptr, (uint8_t*)buffer, PIX_FMT_0RGB32, cap.get_disp_width(), cap.get_disp_height());

	//��YUV buffer ���YUV Frame
	avpicture_fill((AVPicture*)yuv_frame_ptr, (uint8_t*)yuv_buff, PIX_FMT_YUV420P, cap.get_disp_width(), cap.get_disp_height());

	//��RGBת��ΪYUV
	sws_scale(scxt, rgb_frame_ptr->data, rgb_frame_ptr->linesize, 0, c->height, yuv_frame_ptr->data, yuv_frame_ptr->linesize);

	int got_packet_ptr = 0;
	av_init_packet(&avpkt);
	avpkt.data = outbuf;
	avpkt.size = outbuf_size;
	u_size = avcodec_encode_video2(c, &avpkt, yuv_frame_ptr, &got_packet_ptr);
	yuv_frame_ptr->pts++;
	if (u_size == 0)
	{
		fwrite(avpkt.data, 1, avpkt.size, f);
	}

}