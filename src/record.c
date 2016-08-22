#include "record.h"

#include <portaudio.h>

#include "spectrogram.h"

struct SampleArray
{
	real* samples;
	size_t nSamples;
	SDL_mutex* mutex;
	_Atomic bool paused;
};

// This function's signature matches Pa_StreamCallback
int record_callback(float const* input, void* output, unsigned long nFrames,
                    PaStreamCallbackTimeInfo const* timeInfo,
                    PaStreamCallbackFlags flags,
                    struct SampleArray* const sa)
{
	(void) output;
	(void) timeInfo;
	(void) flags;

	if (sa->paused) return paContinue;
	
	SDL_LockMutex(sa->mutex);
	if (nFrames >= sa->nSamples)
	{
		for (size_t i = 0; i < sa->nSamples; ++i)
			sa->samples[i] = (real) input[i + nFrames - sa->nSamples];
	}
	else
	{
		memmove(sa->samples, sa->samples + nFrames,
		        sizeof(real) * (sa->nSamples - nFrames));
		for (size_t i = 0; i < nFrames; ++i)
		{
			sa->samples[i + sa->nSamples - nFrames] = (real) input[i];
		}
	}
	SDL_UnlockMutex(sa->mutex);


	return paContinue;
}

struct CalculationData
{
	struct SampleArray* sampleArray;
	struct DSTFT* dstft;
	struct Display* display;
};
int record_calculation_thread(struct CalculationData* const calculationData)
{
	struct Display* d = calculationData->display;
	struct SampleArray* sa = calculationData->sampleArray;
	struct DSTFT* dstft = calculationData->dstft;

	uint8_t* image = malloc(3 * d->width * d->height * sizeof(uint8_t));
	uint8_t const* dataIn[3];
	int linesizeIn[3];
	uint8_t* dataOut[3];
	int linesizeOut[3];
	{
		dataIn[0] = dataIn[1] = dataIn[2] = image;
		linesizeIn[0] = linesizeIn[1] = linesizeIn[2] = d->width * 3;
		size_t pitchUV = d->width / 2;
		linesizeOut[0] = d->width;
		linesizeOut[1] = pitchUV;
		linesizeOut[2] = pitchUV;
	}
	while (Display_pictQueue_write(d))
	{
		struct Picture* p = &d->pictQueue[d->pictQueueIW];
		dataOut[0] = p->planeY;
		dataOut[1] = p->planeU;
		dataOut[2] = p->planeV;

		// Populate image
		SDL_LockMutex(sa->mutex);
		spectrogram_populate(image, d->width, d->height,
		                     sa->samples, sa->nSamples, true, &d->colourGradient,
		                     dstft);
		SDL_UnlockMutex(sa->mutex);

		sws_scale(d->swsContext,
		          dataIn, linesizeIn, 0, d->height,
		          dataOut, linesizeOut);

		++d->pictQueueIW;
		if (d->pictQueueIW == DISPLAY_PICTQUEUE_SIZE_MAX)
			d->pictQueueIW = 0;
		SDL_LockMutex(d->pictQueueMutex);
		++d->pictQueueSize;
		SDL_UnlockMutex(d->pictQueueMutex);

	}

	free(image);
	return 0;
}
void record_exec(struct Display* const d, struct DSTFT* const dstft,
			size_t nSamples)
{
	(void) d;
	(void) dstft;

	fprintf(stdout, "Recording spectrogram\n");


	struct SampleArray sa;
	sa.nSamples = nSamples; // 2 secs
	sa.samples = calloc(sizeof(real), sa.nSamples);
	sa.mutex = SDL_CreateMutex();
	sa.paused = false;

	int paError = Pa_Initialize();
	if (paError != paNoError)
	{
		fprintf(stderr, "[PortAudio] %s\n", Pa_GetErrorText(paError));
		return;
	}

	PaStreamParameters params;
	PaStream* stream;
	params.device = Pa_GetDefaultInputDevice();
	if (params.device == paNoDevice)
	{
		fprintf(stderr, "No default input device found\n");
		goto complete;
	}
	params.channelCount = 1;
	params.sampleFormat = paFloat32;
	params.suggestedLatency = Pa_GetDeviceInfo(params.device)->defaultLowInputLatency;
	params.hostApiSpecificStreamInfo = NULL;
	paError = Pa_OpenStream(
	            &stream,
	            &params,
	            NULL,
	            48000, // Sample rate
	            paFramesPerBufferUnspecified, // Frame/Buffer
	            paClipOff,
	            (PaStreamCallback*) record_callback,
	            &sa); // Cannot be null
	if (paError != paNoError) goto complete;
	paError = Pa_StartStream(stream);
	if (paError != paNoError) goto complete;

	struct CalculationData calculationData;
	calculationData.display = d;
	calculationData.sampleArray = &sa;
	calculationData.dstft = dstft;
	SDL_CreateThread((SDL_ThreadFunction) record_calculation_thread,
	                 "calculation", &calculationData);

	
	schedule_refresh(d, 40);
	while ((paError = Pa_IsStreamActive(stream)) == 1)
	{
		SDL_Event event;
		if (SDL_PollEvent(&event) == 0)
			continue;

		switch (event.type)
		{
		case SDL_QUIT:
			d->quit = true;
			sa.paused = false;
			paError = Pa_CloseStream(stream);
			goto complete;
			break;
		case EVENT_REFRESH:
			refresh(event.user.data1);
			break;
		case SDL_KEYDOWN:
			switch(event.key.keysym.sym)
			{
			case SDLK_SPACE:
				sa.paused = !sa.paused;
				break;
			}
			break;
		default:
			break;
		}
	}

	if (paError < 0) goto complete;

complete:
	if (paError != paNoError)
	{
		fprintf(stderr, "[PortAudio] %s\n", Pa_GetErrorText(paError));
	}
	Pa_Terminate();
	SDL_DestroyMutex(sa.mutex);
	free(sa.samples);
}
