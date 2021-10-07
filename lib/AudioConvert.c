#include "AudioConvert.h"
#include "HermosauhuLib.h"
#include "AudioTools.h"

void Audio_ByteSwap(AudioSampleInfo* sampleInfo) {
	if (sampleInfo->bit == 16) {
		for (s32 i = 0; i < sampleInfo->samplesNum * sampleInfo->channelNum; i++) {
			Lib_ByteSwap(&sampleInfo->audio.data16[i], sampleInfo->bit / 8);
		}
		
		return;
	}
	if (sampleInfo->bit == 32) {
		for (s32 i = 0; i < sampleInfo->samplesNum * sampleInfo->channelNum; i++) {
			Lib_ByteSwap(&sampleInfo->audio.data32[i], sampleInfo->bit / 8);
		}
		
		return;
	}
}
u32 Audio_ConvertBigEndianFloat80(AiffInfo* aiffInfo) {
	f80 float80 = 0;
	u8* pointer = (u8*)&float80;
	
	for (s32 i = 0; i < 10; i++) {
		pointer[i] = (u8)aiffInfo->sampleRate[9 - i];
	}
	
	return (s32)float80;
}
u32 Audio_ByteSwap_FromHighLow(u16* h, u16* l) {
	Lib_ByteSwap(h, SWAP_U16);
	Lib_ByteSwap(l, SWAP_U16);
	
	return (*h << 16) | *l;
}
void Audio_ByteSwap_ToHighLow(u32* source, u16* h) {
	h[0] = (source[0] & 0xFFFF0000) >> 16;
	h[1] = source[0] & 0x0000FFFF;
	
	Lib_ByteSwap(h, SWAP_U16);
	Lib_ByteSwap(&h[1], SWAP_U16);
}

void Audio_Normalize(AudioSampleInfo* sampleInfo) {
	u32 sampleNum = sampleInfo->samplesNum;
	u32 channelNum = sampleInfo->channelNum;
	f32 max;
	f32 mult;
	
	printf_info("Normalizing.");
	
	if (sampleInfo->bit == 16) {
		max = 0;
		
		for (s32 i = 0; i < sampleNum * channelNum; i++) {
			if (sampleInfo->audio.data16[i] > max) {
				max = ABS(sampleInfo->audio.data16[i]);
			}
			
			if (max >= __INT16_MAX__) {
				return;
			}
		}
		
		mult = __INT16_MAX__ / max;
		
		for (s32 i = 0; i < sampleNum * channelNum; i++) {
			sampleInfo->audio.data16[i] *= mult;
		}
		
		return;
	}
	
	if (sampleInfo->bit == 32) {
		max = 0;
		
		for (s32 i = 0; i < sampleNum * channelNum; i++) {
			if (sampleInfo->audio.dataFloat[i] > max) {
				max = fabsf(sampleInfo->audio.dataFloat[i]);
			}
			
			if (max == 1.0f) {
				return;
			}
		}
		
		mult = 1.0f / max;
		
		for (s32 i = 0; i < sampleNum * channelNum; i++) {
			sampleInfo->audio.dataFloat[i] *= mult;
		}
		
		return;
	}
}
void Audio_ConvertToMono(AudioSampleInfo* sampleInfo) {
	if (sampleInfo->channelNum != 2)
		return;
	
	printf_info("Converting to mono.");
	
	if (sampleInfo->bit == 16) {
		for (s32 i = 0, j = 0; i < sampleInfo->samplesNum; i++, j += 2) {
			sampleInfo->audio.data16[i] = ((f32)sampleInfo->audio.data16[j] + (f32)sampleInfo->audio.data16[j + 1]) * 0.5f;
		}
	}
	
	if (sampleInfo->bit == 32) {
		for (s32 i = 0, j = 0; i < sampleInfo->samplesNum; i++, j += 2) {
			sampleInfo->audio.dataFloat[i] = (sampleInfo->audio.dataFloat[j] + sampleInfo->audio.dataFloat[j + 1]) * 0.5f;
		}
	}
	
	sampleInfo->size /= 2;
	sampleInfo->channelNum = 1;
}
void Audio_ConvertFrom24bit(AudioSampleInfo* sampleInfo, u32 bigEnd) {
	if (sampleInfo->bit != 24)
		return;
	
	printf_info("24-bit rate detected. Converting to 16-bit.");
	
	if (bigEnd) {
		bigEnd = 1;
	}
	for (s32 i = 0; i < sampleInfo->samplesNum * sampleInfo->channelNum; i++) {
		u16 samp = *(u16*)&sampleInfo->audio.data[3 * i + bigEnd];
		sampleInfo->audio.data16[i] = samp;
	}
	
	sampleInfo->bit = 16;
	sampleInfo->size *= (2.0 / 3.0);
	// sampleInfo->instrument.loop.start *= (2.0 / 3.0);
	// sampleInfo->instrument.loop.end *= (2.0 / 3.0);
}

void Audio_TableDesign(AudioSampleInfo* sampleInfo) {
	char basename[256];
	char path[256];
	char buffer[256];
	char* tableDesignArgv[] = {
		String_Generate("tabledesign"),
		String_Generate("-i"),
		String_Generate("30"),
		sampleInfo->output,
		NULL
	};
	
	String_GetBasename(basename, sampleInfo->output);
	String_GetPath(path, sampleInfo->output);
	
	String_Copy(buffer, path);
	String_Combine(buffer, basename);
	String_Combine(buffer, ".table");
	
	FILE* table = fopen(buffer, "w");
	
	if (table == NULL) {
		printf_error("Audio_TableDesign: Could not create/open file [%s]", buffer);
	}
	
	printf_debug(
		"%s %s %s %s",
		tableDesignArgv[0],
		tableDesignArgv[1],
		tableDesignArgv[2],
		tableDesignArgv[3]
	);
	
	if (tabledesign(4, tableDesignArgv, table))
		printf_error("TableDesign has failed");
	
	fclose(table);
	free(tableDesignArgv[0]);
	free(tableDesignArgv[1]);
	free(tableDesignArgv[2]);
}
void Audio_VadpcmEnc(AudioSampleInfo* sampleInfo) {
	char basename[256];
	char path[256];
	char buffer[256];
	char* vadpcmArgv[] = {
		String_Generate("vadpcm_enc"),
		String_Generate("-c"),
		NULL,
		sampleInfo->output,
		NULL,
		NULL
	};
	
	String_GetBasename(basename, sampleInfo->output);
	String_GetPath(path, sampleInfo->output);
	String_Copy(buffer, path);
	String_Combine(buffer, basename);
	String_Combine(buffer, ".table");
	vadpcmArgv[2] = String_Generate(buffer);
	
	String_GetBasename(basename, sampleInfo->output);
	String_GetPath(path, sampleInfo->output);
	String_Copy(buffer, path);
	String_Combine(buffer, basename);
	String_Combine(buffer, ".aifc");
	vadpcmArgv[4] = String_Generate(buffer);
	
	FILE* table = fopen(buffer, "w");
	
	if (table == NULL) {
		printf_error("Audio_TableDesign: Could not create/open file [%s]", buffer);
	}
	
	printf_debug(
		"%s %s %s %s %s",
		vadpcmArgv[0],
		vadpcmArgv[1],
		vadpcmArgv[2],
		vadpcmArgv[3],
		vadpcmArgv[4]
	);
	
	if (vadpcm_enc(5, vadpcmArgv))
		printf_error("VadpcmEnc has failed");
	
	fclose(table);
	free(vadpcmArgv[0]);
	free(vadpcmArgv[1]);
	free(vadpcmArgv[2]);
	free(vadpcmArgv[4]);
}

void Audio_InitSampleInfo(AudioSampleInfo* sampleInfo, char* input, char* output) {
	memset(sampleInfo, 0, sizeof(*sampleInfo));
	sampleInfo->input = input;
	sampleInfo->output = output;
}
void Audio_FreeSample(AudioSampleInfo* sampleInfo) {
	MemFile_Free(&sampleInfo->memFile);
	
	*sampleInfo = (AudioSampleInfo) { 0 };
}

void Audio_GetSampleInfo_Wav(WaveInstrumentInfo** waveInstInfo, WaveSampleInfo** waveSampleInfo, WaveHeader* header, u32 fileSize) {
	WaveInfo* wavInfo = Lib_MemMem(header, fileSize, "fmt ", 4);
	WaveDataInfo* wavData = Lib_MemMem(header, fileSize, "data", 4);
	u32 startOffset = sizeof(WaveHeader) + sizeof(WaveChunk) + wavInfo->chunk.size + sizeof(WaveChunk) + wavData->chunk.size;
	u32 searchLength = header->chunk.size - startOffset;
	u8* hayStart = ((u8*)header) + startOffset;
	
	printf_debug("Audio_GetSampleInfo_Wav: StartOffset  [0x%08X]", startOffset);
	printf_debug("Audio_GetSampleInfo_Wav: SearchLength [0x%08X]", searchLength);
	
	*waveInstInfo = Lib_MemMem(hayStart, searchLength, "inst", 4);
	*waveSampleInfo = Lib_MemMem(hayStart, searchLength, "smpl", 4);
	
	if (*waveInstInfo) {
		printf_debug("Audio_GetSampleInfo_Wav: Found WaveInstrumentInfo");
	}
	
	if (*waveSampleInfo) {
		printf_debug("Audio_GetSampleInfo_Wav: Found WaveSampleInfo");
	}
}
void Audio_GetSampleInfo_Aiff(AiffInstrumentInfo** aiffInstInfo, AiffMarkerInfo** aiffMarkerInfo, AiffHeader* header, u32 fileSize) {
	AiffInfo* aiffInfo = Lib_MemMem(header, fileSize, "COMM", 4);
	u32 startOffset = sizeof(AiffHeader) + aiffInfo->chunk.size;
	u32 searchLength = header->chunk.size - startOffset;
	u8* hayStart = ((u8*)header) + startOffset;
	
	printf_debug("Audio_GetSampleInfo_Aiff: StartOffset  [0x%08X]", startOffset);
	printf_debug("Audio_GetSampleInfo_Aiff: SearchLength [0x%08X]", searchLength);
	
	*aiffInstInfo = Lib_MemMem(hayStart, searchLength, "INST", 4);
	*aiffMarkerInfo = Lib_MemMem(hayStart, searchLength, "MARK", 4);
	
	if (*aiffInstInfo) {
		printf_debug("Audio_GetSampleInfo_Aiff: Found WaveInstrumentInfo");
	}
	
	if (*aiffMarkerInfo) {
		printf_debug("Audio_GetSampleInfo_Aiff: Found WaveSampleInfo");
	}
}

void Audio_LoadSample_Wav(AudioSampleInfo* sampleInfo) {
	MemFile_LoadToMemFile_ReqExt(&sampleInfo->memFile, sampleInfo->input, ".wav");
	WaveHeader* waveHeader = sampleInfo->memFile.data;
	WaveDataInfo* waveData;
	WaveInfo* waveInfo;
	
	printf_debug("Audio_LoadSample_Wav: File [%s] loaded to memory", sampleInfo->input);
	
	if (sampleInfo->memFile.dataSize == 0)
		printf_error("Audio_LoadSample_Wav: Something has gone wrong loading file [%s]", sampleInfo->input);
	waveInfo = Lib_MemMem(waveHeader, sampleInfo->memFile.dataSize, "fmt ", 4);
	waveData = Lib_MemMem(waveHeader, sampleInfo->memFile.dataSize, "data", 4);
	if (!Lib_MemMem(waveHeader->chunk.name, 4, "RIFF", 4))
		printf_error("Audio_LoadSample_Wav: Chunk header [%c%c%c%c] instead of [RIFF]",waveHeader->chunk.name[0],waveHeader->chunk.name[1],waveHeader->chunk.name[2],waveHeader->chunk.name[3]);
	if (!waveInfo || !waveData) {
		if (!waveData) {
			printf_error(
				"Audio_LoadSample_Wav: could not locate [data] from [%s]",
				sampleInfo->input
			);
		}
		if (!waveInfo) {
			printf_error(
				"Audio_LoadSample_Wav: could not locate [fmt] from [%s]",
				sampleInfo->input
			);
		}
	}
	
	sampleInfo->channelNum = waveInfo->channelNum;
	sampleInfo->bit = waveInfo->bit;
	sampleInfo->sampleRate = waveInfo->sampleRate;
	sampleInfo->samplesNum = waveData->chunk.size / (waveInfo->bit / 8) / waveInfo->channelNum;
	sampleInfo->size = waveData->chunk.size;
	sampleInfo->audio.data = waveData->data;
	sampleInfo->instrument.loop.end = sampleInfo->samplesNum;
	
	WaveInstrumentInfo* waveInstInfo;
	WaveSampleInfo* waveSampleInfo;
	
	Audio_GetSampleInfo_Wav(&waveInstInfo, &waveSampleInfo, waveHeader, sampleInfo->memFile.dataSize);
	
	if (waveInstInfo) {
		sampleInfo->instrument.fineTune = waveInstInfo->fineTune;
		sampleInfo->instrument.note = waveInstInfo->note;
		sampleInfo->instrument.highNote = waveInstInfo->hiNote;
		sampleInfo->instrument.lowNote = waveInstInfo->lowNote;
		
		if (waveSampleInfo && waveSampleInfo->numSampleLoops) {
			sampleInfo->instrument.loop.start = waveSampleInfo->loopData[0].start;
			sampleInfo->instrument.loop.end = waveSampleInfo->loopData[0].end;
			sampleInfo->instrument.loop.count = waveSampleInfo->loopData[0].count;
		}
	}
	
	Audio_ConvertFrom24bit(sampleInfo, true);
}
void Audio_LoadSample_Aiff(AudioSampleInfo* sampleInfo) {
	MemFile_LoadToMemFile_ReqExt(&sampleInfo->memFile, sampleInfo->input, ".aiff");
	AiffHeader* aiffHeader = sampleInfo->memFile.data;
	AiffDataInfo* aiffData;
	AiffInfo* aiffInfo;
	
	printf_debug("Audio_LoadSample_Aiff: File [%s] loaded to memory", sampleInfo->input);
	
	if (sampleInfo->memFile.dataSize == 0)
		printf_error("Audio_LoadSample_Aiff: Something has gone wrong loading file [%s]", sampleInfo->input);
	aiffInfo = Lib_MemMem(aiffHeader, sampleInfo->memFile.dataSize, "COMM", 4);
	aiffData = Lib_MemMem(aiffHeader, sampleInfo->memFile.dataSize, "SSND", 4);
	if (!Lib_MemMem(aiffHeader->formType, 4, "AIFF", 4)) {
		printf_error(
			"Audio_LoadSample_Aiff: Chunk header [%c%c%c%c] instead of [AIFF]",
			aiffHeader->formType[0],
			aiffHeader->formType[1],
			aiffHeader->formType[2],
			aiffHeader->formType[3]
		);
	}
	if (!aiffInfo || !aiffData) {
		if (!aiffData) {
			printf_error(
				"Audio_LoadSample_Aiff: could not locate [SSND] from [%s]",
				sampleInfo->input
			);
		}
		if (!aiffInfo) {
			printf_error(
				"Audio_LoadSample_Aiff: could not locate [COMM] from [%s]",
				sampleInfo->input
			);
		}
	}
	
	// Swap Chunk Sizes
	Lib_ByteSwap(&aiffHeader->chunk.size, SWAP_U32);
	Lib_ByteSwap(&aiffInfo->chunk.size, SWAP_U32);
	Lib_ByteSwap(&aiffData->chunk.size, SWAP_U32);
	
	// Swap INFO
	Lib_ByteSwap(&aiffInfo->channelNum, SWAP_U16);
	Lib_ByteSwap(&aiffInfo->sampleNumH, SWAP_U16);
	Lib_ByteSwap(&aiffInfo->sampleNumL, SWAP_U16);
	Lib_ByteSwap(&aiffInfo->bit, SWAP_U16);
	
	// Swap DATA
	Lib_ByteSwap(&aiffData->offset, SWAP_U32);
	Lib_ByteSwap(&aiffData->blockSize, SWAP_U32);
	
	u32 sampleNum = aiffInfo->sampleNumL + (aiffInfo->sampleNumH << 8);
	
	sampleInfo->channelNum = aiffInfo->channelNum;
	sampleInfo->bit = aiffInfo->bit;
	sampleInfo->sampleRate = Audio_ConvertBigEndianFloat80(aiffInfo);
	sampleInfo->samplesNum = sampleNum;
	sampleInfo->size = sampleNum * sampleInfo->channelNum * (sampleInfo->bit / 8);
	sampleInfo->audio.data = aiffData->data;
	
	sampleInfo->instrument.loop.end = sampleInfo->samplesNum;
	
	AiffInstrumentInfo* aiffInstInfo;
	AiffMarkerInfo* aiffMarkerInfo;
	
	Audio_GetSampleInfo_Aiff(&aiffInstInfo, &aiffMarkerInfo, aiffHeader, sampleInfo->memFile.dataSize);
	
	if (aiffInstInfo) {
		sampleInfo->instrument.note = aiffInstInfo->baseNote;
		sampleInfo->instrument.fineTune = aiffInstInfo->detune;
		sampleInfo->instrument.highNote = aiffInstInfo->highNote;
		sampleInfo->instrument.lowNote = aiffInstInfo->lowNote;
	}
	
	if (aiffMarkerInfo && aiffInstInfo && aiffInstInfo->sustainLoop.playMode >= 1) {
		s32 startIndex = aiffInstInfo->sustainLoop.start - 1;
		s32 endIndex = aiffInstInfo->sustainLoop.end - 1;
		u16 loopStartH = aiffMarkerInfo->marker[startIndex].positionH;
		u16 loopStartL = aiffMarkerInfo->marker[startIndex].positionL;
		u16 loopEndH = aiffMarkerInfo->marker[endIndex].positionH;
		u16 loopEndL = aiffMarkerInfo->marker[endIndex].positionL;
		
		sampleInfo->instrument.loop.start = Audio_ByteSwap_FromHighLow(&loopStartH, &loopStartL);
		sampleInfo->instrument.loop.end = Audio_ByteSwap_FromHighLow(&loopEndH, &loopEndL);
		sampleInfo->instrument.loop.count = 0xFFFFFFFF;
	}
	
	Audio_ConvertFrom24bit(sampleInfo, false);
	Audio_ByteSwap(sampleInfo);
}
void Audio_LoadSample(AudioSampleInfo* sampleInfo) {
	char* keyword[] = {
		".wav",
		".aiff",
		".ogg",
	};
	AudioFunc loadSample[] = {
		Audio_LoadSample_Wav,
		Audio_LoadSample_Aiff,
		NULL
	};
	
	if (!sampleInfo->input)
		printf_error("Audio_LoadSample: No input file set");
	if (!sampleInfo->output)
		printf_error("Audio_LoadSample: No output file set");
	
	for (s32 i = 0; i < ARRAY_COUNT(loadSample); i++) {
		if (Lib_MemMem(sampleInfo->input, strlen(sampleInfo->input), keyword[i], strlen(keyword[i]) - 1)) {
			char basename[128];
			
			String_GetBasename(basename, sampleInfo->input);
			printf_info("Loading [%s%s]", basename, keyword[i]);
			
			loadSample[i](sampleInfo);
			break;
		}
	}
}

void Audio_SaveSample_Aiff(AudioSampleInfo* sampleInfo) {
	AiffHeader header = { 0 };
	AiffInfo info = { 0 };
	AiffDataInfo dataInfo = { 0 };
	AiffMarker marker[2] = { 0 };
	AiffMarkerInfo markerInfo = { 0 };
	AiffInstrumentInfo instrument = { 0 };
	
	char basename[128];
	
	String_GetBasename(basename, sampleInfo->output);
	printf_info("Saving [%s.aiff]", basename);
	
	Audio_ByteSwap(sampleInfo);
	
	/* Write chunk headers */ {
		info.chunk.size = sizeof(AiffInfo) - sizeof(AiffChunk) - 2;
		dataInfo.chunk.size = sampleInfo->size + 0x8;
		markerInfo.chunk.size = sizeof(u16) + sizeof(AiffMarker) * 2;
		instrument.chunk.size = sizeof(AiffInstrumentInfo) - sizeof(AiffChunk);
		header.chunk.size = info.chunk.size +
		    dataInfo.chunk.size +
		    markerInfo.chunk.size +
		    instrument.chunk.size +
		    sizeof(AiffChunk) +
		    sizeof(AiffChunk) +
		    sizeof(AiffChunk) +
		    sizeof(AiffChunk) + 4;
		
		Lib_ByteSwap(&info.chunk.size, SWAP_U32);
		Lib_ByteSwap(&dataInfo.chunk.size, SWAP_U32);
		Lib_ByteSwap(&markerInfo.chunk.size, SWAP_U32);
		Lib_ByteSwap(&instrument.chunk.size, SWAP_U32);
		Lib_ByteSwap(&header.chunk.size, SWAP_U32);
		
		f80 sampleRate = sampleInfo->sampleRate;
		u8* p = (u8*)&sampleRate;
		
		for (s32 i = 0; i < 10; i++) {
			info.sampleRate[i] = p[9 - i];
		}
		
		info.channelNum = sampleInfo->channelNum;
		Audio_ByteSwap_ToHighLow(&sampleInfo->samplesNum, &info.sampleNumH);
		info.bit = sampleInfo->bit;
		Lib_ByteSwap(&info.channelNum, SWAP_U16);
		Lib_ByteSwap(&info.bit, SWAP_U16);
		
		markerInfo.markerNum = 2;
		marker[0].index = 1;
		Audio_ByteSwap_ToHighLow(&sampleInfo->instrument.loop.start, &marker[0].positionH);
		marker[1].index = 2;
		Audio_ByteSwap_ToHighLow(&sampleInfo->instrument.loop.end, &marker[1].positionH);
		
		Lib_ByteSwap(&markerInfo.markerNum, SWAP_U16);
		Lib_ByteSwap(&marker[0].index, SWAP_U16);
		Lib_ByteSwap(&marker[1].index, SWAP_U16);
		
		instrument.baseNote = sampleInfo->instrument.note;
		instrument.detune = sampleInfo->instrument.fineTune;
		instrument.lowNote = sampleInfo->instrument.lowNote;
		instrument.highNote = sampleInfo->instrument.highNote;
		instrument.lowVelocity = 0;
		instrument.highNote = 127;
		instrument.gain = __INT16_MAX__;
		Lib_ByteSwap(&instrument.gain, SWAP_U16);
		
		instrument.sustainLoop.start = 1;
		instrument.sustainLoop.end = 2;
		instrument.sustainLoop.playMode = 1;
		Lib_ByteSwap(&instrument.sustainLoop.start, SWAP_U16);
		Lib_ByteSwap(&instrument.sustainLoop.end, SWAP_U16);
		Lib_ByteSwap(&instrument.sustainLoop.playMode, SWAP_U16);
	}
	memcpy(header.chunk.name, "FORM", 4);
	memcpy(header.formType, "AIFF", 4);
	memcpy(info.chunk.name, "COMM", 4);
	memcpy(info.compressionType, "NONE", 4);
	memcpy(dataInfo.chunk.name, "SSND", 4);
	memcpy(markerInfo.chunk.name, "MARK", 4);
	memcpy(instrument.chunk.name, "INST", 4);
	
	FILE* output = fopen(sampleInfo->output, "w");
	
	if (output == NULL)
		printf_error("Audio_SaveSample_Aiff: Could not open outputfile [%s]", sampleInfo->output);
	
	fwrite(&header, 1, sizeof(header), output);
	fwrite(&info, 1, 0x16 + sizeof(AiffChunk), output);
	fwrite(&markerInfo, 1, 0xA, output);
	fwrite(&marker[0], 1, sizeof(AiffMarker), output);
	fwrite(&marker[1], 1, sizeof(AiffMarker), output);
	fwrite(&instrument, 1, sizeof(instrument), output);
	fwrite(&dataInfo, 1, 16, output);
	fwrite(sampleInfo->audio.data, 1, sampleInfo->size, output);
	fclose(output);
}
void Audio_SaveSample_Wav(AudioSampleInfo* sampleInfo) {
}