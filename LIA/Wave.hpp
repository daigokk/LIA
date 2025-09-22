#pragma once
#pragma comment(lib,"winmm.lib")

#include <cmath>
#include <windows.h>
#include <numbers> // For std::numbers::pi

class Wave
{
private:
	int bufCount;   //      �Đ�����o�C�g��
	int sz;                 //      �o�b�t�@��
	HWAVEOUT hWaveOut;
	WAVEFORMATEX wfe;
	WAVEHDR* whdr;
	short** lpWave; //      �o�b�t�@
public:
	bool reset;
	void init(const int rate, const int sec)
	{
		this->bufCount = 2;   //      �g�p����o�b�t�@��
		this->reset = true;
		this->wfe.wFormatTag = WAVE_FORMAT_PCM;
		this->wfe.nChannels = 1;      //      ���m����    
		this->wfe.wBitsPerSample = 16;        //      ����\16bit
		this->wfe.nBlockAlign = wfe.nChannels * wfe.wBitsPerSample / 8;
		this->wfe.nSamplesPerSec = rate;      //      �T���v�����[�g
		this->wfe.nAvgBytesPerSec = wfe.nSamplesPerSec * wfe.nBlockAlign;
		waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfe, 0, (DWORD_PTR)this, 0);
		this->sz = wfe.nAvgBytesPerSec * sec;
		this->whdr = new WAVEHDR[bufCount];
		this->lpWave = (short**)new void* [bufCount];
		for (int n = 0; n < bufCount; n++)
		{
			this->lpWave[n] = new short[this->wfe.nAvgBytesPerSec * sec];
			this->whdr[n].lpData = (LPSTR)this->lpWave[n];
		}
	}
	Wave(const int rate, const int sec)
	{
		this->init(rate, sec);
	}
	Wave(const int rate, const int sec, const int freq)
	{
		this->init(rate, sec);
		this->makeSin(freq);
	}
	void close()
	{
		this->stop();
		for (int n = 0; n < this->bufCount; n++) {
			waveOutUnprepareHeader(this->hWaveOut, &this->whdr[n], sizeof(WAVEHDR));
			delete[]this->lpWave[n];
		}
		delete[]this->whdr;
		delete[]this->lpWave;
	}
	~Wave()
	{
		close();
	}
	void makeSin(const double freq)
	{ //      1�����̐����g�f�[�^�쐬
		double qstep = 2 * std::numbers::pi * freq / this->wfe.nSamplesPerSec;
		double rq = 2;
		double q = 0;
		int n;
		for (n = 0; n < this->sz; n++) {
			int d;
			d = int(32767 * sin(q));
			for (int i = 0; i < this->bufCount; i++)
				this->lpWave[i][n] = d;
			q += qstep;
			if (rq < 0 && 0 <= q) {
				break;
			}
			rq = q;
		}
		for (int i = 0; i < this->bufCount; i++)
		{
			this->whdr[i].dwBufferLength = n * 2; //      �o�b�t�@�o�C�g��
			this->whdr[i].dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
			this->whdr[i].dwLoops = 20000 * freq / this->wfe.nSamplesPerSec;
			this->whdr[i].dwUser = 0;
		}
	}
	void play(void)
	{ 
		this->reset = false;
		for (int i = 0; i < this->bufCount; i++)
		{
			waveOutPrepareHeader(this->hWaveOut, &this->whdr[i], sizeof(WAVEHDR));
			waveOutWrite(this->hWaveOut, &this->whdr[i], sizeof(WAVEHDR));
		}
	}
	void stop(void)
	{
		this->reset = true;
		waveOutReset(this->hWaveOut);
	}
};
