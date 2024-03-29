/* See LICENSE for license details.
 * Developed by Peter McMaster.
 * Audio tool used to capture, or playback raw audio files.
 */

#include <getopt.h>
#include <alsa/asoundlib.h>

#include "defaults.h"

/* Stores pcm information */
typedef struct {
	unsigned int samplerate;
	snd_pcm_uframes_t periodsize;
	int channels;
	snd_pcm_format_t bitdepth;
	snd_pcm_stream_t pcmdir;
	snd_pcm_t *pcmhandle;	
	snd_pcm_hw_params_t *hwparams;
	int dir;
	char *filename;
} SignalConfig;

/* Prints out alsa hw params. */
void alsa_info(SignalConfig *sc);
/* Populates the alsa hw params struct. */
int configure_alsa(SignalConfig *sc);
/* Fills a new signal_config struct with DEFINE values. */
SignalConfig *init_signal_config();
/* prints error_string and uses snd_strerror() to resolve err to a alsa error string. */
void print_error(char *error_string, int err);

/* Reads from the hw buffer and writes to stdout. */
int capture_audio(SignalConfig *sc, char *buf, int size);
/* Reads values from stdin and writes to hardware buffer. */
int playback_audio(SignalConfig *sc, char *buf, int size);

/* Sets values in signal config struct with value from argv. */
int parse_args(SignalConfig *sc, int argc, char *argv[]);
/* Converts the arg passed to a pcm bit format type. */
snd_pcm_format_t search_pcmformat_from_argbitformat(char *arg);

#ifdef DEBUG 
void 
alsa_info(SignalConfig *sc)
{
	snd_pcm_hw_params_get_rate(sc->hwparams,&(sc->samplerate),&(sc->dir));
	printf("Sample rate(bps): %d\n", sc->samplerate);

	snd_pcm_hw_params_get_periods(sc->hwparams,&(sc->samplerate), &(sc->dir));
	printf("Periods: %d\n", sc->samplerate);

	snd_pcm_hw_params_get_period_time(sc->hwparams,&(sc->samplerate),&(sc->dir));
	printf("Period time(microseconds): %d\n", sc->samplerate);

  	snd_pcm_hw_params_get_period_size(sc->hwparams,&(sc->periodsize), &(sc->dir));
  	printf("Period size(frames): %d\n", (int)sc->periodsize);
	
	snd_pcm_hw_params_get_buffer_size(sc->hwparams,(snd_pcm_uframes_t *)&(sc->samplerate));
	printf("Buffer Size(frames): %d\n", sc->samplerate);
}
#endif /* DEBUG */

int 
configure_alsa(SignalConfig *sc)
{
	int err;
	if((err = snd_pcm_open(&(sc->pcmhandle), "default", sc->pcmdir, 0)) < 0) {
		print_error("Error opening audio device",err);
		return -1;
	}

	snd_pcm_hw_params_alloca(&(sc->hwparams));

	if((err = snd_pcm_hw_params_any(sc->pcmhandle,sc->hwparams)) < 0) {
		print_error("Error setting default hw params",err);
		return -1;
	}

	if((err = snd_pcm_hw_params_set_access(sc->pcmhandle,sc->hwparams,SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		print_error("Error setting access mode to interleaved",err);
		return -1;
	}

	if((err = snd_pcm_hw_params_set_format(sc->pcmhandle,sc->hwparams,sc->bitdepth)) < 0) {
		print_error("Error setting bit format",err);
		return -1;
	}
	
	if((err = snd_pcm_hw_params_set_channels(sc->pcmhandle,sc->hwparams,sc->channels)) < 0) {
		print_error("Error setting channels",err);
		return -1;
	}

	if((err = snd_pcm_hw_params_set_rate_near(sc->pcmhandle, sc->hwparams, &(sc->samplerate), &(sc->dir))) < 0) {
		print_error("Error setting rate",err);
		return -1;
	}
	
	if((err = snd_pcm_hw_params_set_period_size_near(sc->pcmhandle, sc->hwparams, &(sc->periodsize), &(sc->dir))) < 0) {
		print_error("Error setting period size",err);
		return -1;
	}

	if((err = snd_pcm_hw_params(sc->pcmhandle,sc->hwparams)) < 0) {
		print_error("Error applying pcmhandle to hw params",err);
		return -1;
	}
	return 0;
}

SignalConfig *
init_signal_config()
{
	SignalConfig *sc = malloc(sizeof(SignalConfig));
	sc->samplerate=DEFAULT_SAMPLE_RATE;
	sc->periodsize=DEFAULT_PERIOD_SIZE;
	sc->channels=DEFAULT_CHANNELS;
	sc->bitdepth=DEFAULT_BITDEPTH;
	return sc;
}

void 
print_error(char *error_string, int err)
{
	fprintf(stderr,"%s\n%s\n",error_string,snd_strerror(err));
}

int 
capture_audio(SignalConfig *sc, char *buf, int size)
{
	int file = 1;
	int err;
	if(sc->filename != NULL) {
		file = open(sc->filename,O_RDWR);
	}
	while(1) {
		err = snd_pcm_readi(sc->pcmhandle,buf,sc->periodsize);
		if(err == -EPIPE) {
			fprintf(stderr,"Overrun occurred\n");
			snd_pcm_prepare(sc->pcmhandle);
		} else if(err < 0) {
			fprintf(stderr,"Error from read: %s\n", snd_strerror(err));
		} else if(err != (int)sc->periodsize) {
			fprintf(stderr,"Short read: read %d frames\n",err);
		}
		err = write(file,buf,size);
		if(err != size) {
			fprintf(stderr,"Short write: wrote %d bytes\n", err);
		}
	}
	return 0;
}

int 
playback_audio(SignalConfig *sc, char *buf, int size)
{
	int file = 0;
	int err;
	if(sc->filename != NULL) {
		file = open(sc->filename,O_RDWR);
	}
	while((err = read(file,buf,size)) != 0) {
		if(err == 0) {
			fprintf(stderr, "end of file on input\n");
			break;
			printf("what the fuck\n");
		} else if(err != size) {
			fprintf(stderr,"short read: read %d bytes\n",err);
		}

		err = snd_pcm_writei(sc->pcmhandle,buf,sc->periodsize);
		if(err == -EPIPE) {
			fprintf(stderr,"underrun occurred\n");
			snd_pcm_prepare(sc->pcmhandle);
			return -1;
		} else if(err < 0) {
			fprintf(stderr,"error from writei: %s\n",snd_strerror(err));
		} else if(err != (int)sc->periodsize) {
			fprintf(stderr,"Short write: written %d frames\n",err);
		}
	}
	return 0;
}

int parse_args(SignalConfig *sc, int argc, char *argv[])
{
	struct option options[] = {
		{"capture",no_argument,0,'c'},
		{"playback",no_argument,0,'p'},
		{"rate",optional_argument,0,'r'},
		{"bit",optional_argument,0,'b'},
		{"channels",optional_argument,0,'l'},
		{"file",required_argument,0,'f'},
		{0,0,0,0}
	};

	int optionindex = 0;
	char c;
	int bitdepth;
	short int dirset = 0; //Each time -c or -p is parsed dirset will increase by 1 
	while((c = getopt_long(argc,argv,"cpr:b:l:f:",options,&optionindex)) != -1) {
		switch(c) {
			case 'p':
				sc->pcmdir=SND_PCM_STREAM_PLAYBACK;
				dirset++;
				break;
			case 'c':
				sc->pcmdir=SND_PCM_STREAM_CAPTURE;
				dirset++;
				break;
			case 'r':
				sc->samplerate=atoi(optarg);
				break;
			case 'b':
				if((bitdepth = search_pcmformat_from_argbitformat(optarg)) != -1) {
					sc->bitdepth=bitdepth;
				}
				break;
			case 'l':
				sc->channels=atoi(optarg);
				break;
			case 'f':
				sc->filename=optarg;
				break;
			default:
				exit(1);
		};
	}
	
	if(dirset == 0) {
		printf("Must pass -c or -p.\n");
		exit(1);
	} else if (dirset > 1) {
		printf("Pass only -c or -p.\n");
		exit(1);
	}
	
	return 0;
}

snd_pcm_format_t 
search_pcmformat_from_argbitformat(char *arg)
{
	struct bitformat{
		snd_pcm_format_t sndvalue;
		char* inputvalue;
	};

	struct bitformat bflist[] = {
		{ .sndvalue=SND_PCM_FORMAT_S16_LE, .inputvalue="s16le" },
		{ .sndvalue=SND_PCM_FORMAT_S16_BE, .inputvalue="s16be" },
		{ .sndvalue=SND_PCM_FORMAT_U16_LE, .inputvalue="u16be" },
		{ .sndvalue=SND_PCM_FORMAT_U16_BE, .inputvalue="u16be" }
	};

	for(int i=0; i<sizeof(bflist)/sizeof(struct bitformat); ++i) {
		if(strcmp(bflist[i].inputvalue,arg)) {
			return bflist[i].sndvalue;
		}
	}
	return -1;
}

int 
main(int argc, char* argv[])
{
	SignalConfig *sc;		
	int err;
	char *buf;

	sc = init_signal_config();
	parse_args(sc,argc,argv);
	if(configure_alsa(sc) < 0) {
		exit(1);
	}

#ifdef DEBUG
	alsa_info(sc);
#endif	
	int size = sc->periodsize * sc->channels * snd_pcm_samples_to_bytes(sc->pcmhandle,1); /* 2 bytes per sample */
	buf = malloc(size);
		
	if(sc->pcmdir == SND_PCM_STREAM_CAPTURE) {
		if(capture_audio(sc,buf,size) < 0) {
			exit(1);
		}
	} else if(sc->pcmdir == SND_PCM_STREAM_PLAYBACK) {
		if(playback_audio(sc,buf,size) < 0) {
			exit(1);
		}	
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

