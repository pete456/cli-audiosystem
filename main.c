#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <getopt.h>

#include <alsa/asoundlib.h>

#define ALSA_PCM_NEW_HW_PARAMS_API

#define DEFAULT_CHANNELS 2
#define DEFAULT_SAMPLE_RATE 48000
#define DEFAULT_BITDEPTH SND_PCM_FORMAT_S16_LE 
#define DEFAULT_PERIOD_SIZE 1024;

struct signal_config{
	unsigned int samplerate;
	snd_pcm_uframes_t periodsize;
	int channels;
	snd_pcm_format_t bitdepth;
	snd_pcm_stream_t pcmdir;
	snd_pcm_t *pcmhandle;	
	snd_pcm_hw_params_t *hwparams;
	int dir;
};

struct signal_config* init_signal_config()
{
	struct signal_config* sc = malloc(sizeof(struct signal_config));
	sc->samplerate=DEFAULT_SAMPLE_RATE;
	sc->periodsize=DEFAULT_PERIOD_SIZE;
	sc->channels=DEFAULT_CHANNELS;
	sc->bitdepth=DEFAULT_BITDEPTH;
	return sc;
}

int parse_args(struct signal_config* sc, int argc, char* argv[])
{
	struct option options[] = {
		{"p",0,0,0},
		{"c",0,0,0},
		{"r",1,0,'r'},
		{"b",1,0,'b'},
		{"l",1,0,'l'}
	};
	int optionindex=0;
	char c;
	short int dirset=0; //Will be set two one if 
	while((c = getopt_long(argc,argv,"pcr:b:l:",options,&optionindex)) != -1) {
		switch(c) {
			case 0:
				printf("Long option = %s\n",options[optionindex].name);
				break;
			case 'p':
				sc->pcmdir=SND_PCM_STREAM_PLAYBACK;
				printf("Playback\n");
				dirset++;
				break;
			case 'c':
				sc->pcmdir=SND_PCM_STREAM_CAPTURE;
				printf("Capture\n");
				dirset++;
				break;
			case 'r':
				sc->samplerate=atoi(optarg);
				break;
			case 'b':
				//sc->atoi(optarg);
				break;
			case 'l':
				sc->channels=atoi(optarg);
				break;
			default:
				exit(1);
		};
	}
	return 0;
}

int main(int argc, char* argv[])
{
	struct signal_config* sc = init_signal_config();
		
	int err;
	char *buf;
	
	parse_args(sc,argc,argv);

	if((err = snd_pcm_open(&(sc->pcmhandle), "default", sc->pcmdir, 0)) < 0) {
		fprintf(stderr,"Cannot open audio device %s\n",snd_strerror(err));
	} else {
		printf("pcm is open\n");
	}

	snd_pcm_hw_params_alloca(&(sc->hwparams));

	if((err = snd_pcm_hw_params_any(sc->pcmhandle,sc->hwparams)) < 0) {
		printf("Cannot set default hw params\n");
		return -1;
	}

	if((err = snd_pcm_hw_params_set_access(sc->pcmhandle,sc->hwparams,SND_PCM_ACCESS_RW_INTERLEAVED))) {
		printf("Cannot set access mode to interleaved\n");
	}

	snd_pcm_hw_params_set_format(sc->pcmhandle,sc->hwparams,sc->bitdepth);
	
	snd_pcm_hw_params_set_channels(sc->pcmhandle,sc->hwparams,sc->channels);
	
	snd_pcm_hw_params_set_rate_near(sc->pcmhandle, sc->hwparams, &(sc->samplerate), &(sc->dir));
	
	snd_pcm_hw_params_set_period_size_near(sc->pcmhandle, sc->hwparams, &(sc->periodsize), &(sc->dir));

	if((err = snd_pcm_hw_params(sc->pcmhandle,sc->hwparams)) < 0) {
		printf("Can not set hw params\n");
		return -1;
	}

	snd_pcm_hw_params_get_rate(sc->hwparams,&(sc->samplerate),&(sc->dir));
	printf("Rate = %d bps\n",sc->samplerate);

	snd_pcm_hw_params_get_periods(sc->hwparams,&(sc->samplerate), &(sc->dir));
	printf("Periods = %d\n",sc->samplerate);

	snd_pcm_hw_params_get_period_time(sc->hwparams,&(sc->samplerate),&(sc->dir));
	printf("Period time =  %d microseconds\n",sc->samplerate);

  	snd_pcm_hw_params_get_period_size(sc->hwparams,&(sc->periodsize), &(sc->dir));
  	printf("period size = %d frames\n", (int)sc->periodsize);
	
	int size = sc->periodsize * 4; /* 2 bytes per sample * 2 channels */
	buf = malloc(size);
	printf("Size of application buffer = %d\n",size);
	
	snd_pcm_hw_params_get_buffer_size(sc->hwparams,(snd_pcm_uframes_t *)&(sc->samplerate));
	printf("Buffer Size = %d frames\n",sc->samplerate);
	
	while(1) {
		err = read(0,buf,size);

		if(err == 0) {
			printf("End of file on input\n");
			break;
		} else if(err != size) {
			printf("Short read: read %d bytes\n",err);
		}
		err = snd_pcm_writei(sc->pcmhandle,buf,sc->periodsize);
	}
	
	if((err = snd_pcm_drain(sc->pcmhandle)) < 0) {
		fprintf(stderr,"Can't drain pcm handle, %s\n",snd_strerror(err));
		return -1;
	}
	snd_pcm_close(sc->pcmhandle);
	free(buf);
	free(sc);
	return 0;
}
