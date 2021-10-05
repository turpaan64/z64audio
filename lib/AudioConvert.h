typedef signed char s8;
typedef unsigned char u8;
typedef signed short int s16;
typedef unsigned short int u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long long int s64;
typedef unsigned long long int u64;
typedef float f32;
typedef double f64;
typedef long double f80;

#ifndef __Z64AUDIO_HEADER__
#define __Z64AUDIO_HEADER__

typedef union {
	u8*  data;
	s16* data16;
	s32* data32;
	f32* dataFloat;
} Sample;

typedef struct {
	u32 start;
	u32 end;
	u32 count;
} SampleLoop;

typedef struct {
	s8 note;
	s8 fineTune;
	u8 highNote;
	u8 lowNote;
	SampleLoop loop;
} SampleInstrument;

typedef struct {
	u8     channelNum;
	u8     bit;
	u32    sampleRate;
	u32    samplesNum;
	u32    size;
	Sample audio;
	SampleInstrument instrument;
} AudioSampleInfo;

#endif

#ifndef __WAVE_HEADER__
#define __WAVE_HEADER__

typedef struct {
	char name[4];
	u32  size;
} WaveChunk;

typedef struct {
	WaveChunk chunk;
	u16 format; // PCM = 1 (uncompressed)
	u16 channelNum; // Mono = 1, Stereo = 2, etc.
	u32 sampleRate;
	u32 byteRate; // == sampleRate * channelNum * bit
	u16 blockAlign;
	u16 bit;
} WaveInfo;

typedef struct {
	WaveChunk chunk;
	u8 data[];
} WaveDataInfo;

typedef struct {
	WaveChunk chunk;
	char format[4];
} WaveHeader;

typedef struct {
	u32 cuePointID;
	u32 type;
	u32 start;
	u32 end;
	u32 fraction;
	u32 count;
} WaveSampleLoop;

typedef struct {
	WaveChunk chunk; // Chunk Data Size == 36 + (Num Sample Loops * 24) + Sampler Data
	u32 manufacturer;
	u32 product;
	u32 samplePeriod;
	u32 unityNote; // 0-127
	u32 pitchFraction;
	u32 format;
	u32 offset;
	u32 numSampleLoops;
	u32 samplerData;
	WaveSampleLoop loopData[];
} WaveSampleInfo;

typedef struct {
	WaveChunk chunk;
	s8 note;
	s8 fineTune;
	s8 gain;
	s8 lowNote;
	s8 hiNote;
	s8 lowVel;
	s8 hiVel;
} WaveInstrumentInfo;

#endif

#ifndef __AIFF_HEADER__
#define __AIFF_HEADER__

typedef struct {
	char name[4];
	u32  size;
} AiffChunk;

typedef struct {
	AiffChunk chunk; // FORM
	char formType[4]; // AIFF
} AiffHeader;

typedef struct {
	AiffChunk chunk; // COMM
	u16 channelNum;
	u16 sampleNumH;
	u16 sampleNumL;
	u16 bit;
	u8  sampleRate[10]; // 80-bit float
} AiffInfo;

typedef struct {
	s16 index;
	u32 position;
	u16 string;
} AiffMarker;

typedef struct {
	AiffChunk  chunk; // MARK
	u16 markerNum;
	AiffMarker marker[];
} AiffMarkerInfo;

typedef struct {
	s16 playMode;
	s16 start;
	s16 end;
} AiffLoop;

typedef struct {
	AiffChunk chunk; // INST
	s8  baseNote;
	s8  detune;
	s8  lowNote;
	s8  highNote;
	s8  lowVelocity;
	s8  highVelocity;
	s16 gain;
	AiffLoop sustainLoop;
	AiffLoop releaseLoop;
} AiffInstrumentInfo;

typedef struct {
	AiffChunk chunk; // SSND
	u32 offset;
	u32 blockSize;
	u8  data[];
} AiffDataInfo;

#endif

void Audio_ByteSwapFloat80(f80* float80);
void Audio_ByteSwapData(AudioSampleInfo* audioInfo);
void Audio_LoadWav(void** dst, char* file, AudioSampleInfo* sampleInfo);
void Audio_LoadAiff(void** dst, char* file, AudioSampleInfo* sampleInfo);