
#include <inttypes.h>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>

#include "Mp4Parse.h"
using namespace std;
namespace fs = std::filesystem;

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("no input\n");
        return 0;
    }

    Mp4ParserHandle mp4Parser = createMp4Parser();

    int ret = 0;

    string mp4FilePath(argv[1]);

    ret = mp4Parser->parse(mp4FilePath);

    if (ret < 0 || !mp4Parser->isParseSuccess())
    {
        printf("parse %s fail: \n", mp4FilePath.c_str());
        string err_msg = mp4Parser->getErrorMessage();
        while (!err_msg.empty())
        {
            printf("%s\n", err_msg.c_str());
            err_msg = mp4Parser->getErrorMessage();
        }
        return 0;
    }

    printf("%s\n", mp4Parser->getBasicInfoString().c_str());

    if (mp4Parser->getTracksInfo().empty())
    {
        printf("nothing found\n");
    }

    fs::path filePath(mp4FilePath);

    string outPath      = filePath.parent_path().string() + "/";
    string fileBaseName = filePath.stem().string();
    string videoOutFilePath;
    string audioOutFilePath;

    int  videoTrackIdx = -1;
    int  audioTrackIdx = -1;
    auto tracksInfo    = mp4Parser->getTracksInfo();
    for (int i = 0; i < tracksInfo.size(); i++)
    {
        if (tracksInfo[i]->mediaInfo->mediaType == MP4_MEDIA_TYPE_VIDEO)
        {
            videoTrackIdx = i;
            break;
        }
        else if (tracksInfo[i]->mediaInfo->mediaType == MP4_MEDIA_TYPE_AUDIO)
        {
            audioTrackIdx = i;
        }
    }
    if (videoTrackIdx < 0)
        return -1;

    uint8_t codec = tracksInfo[videoTrackIdx]->mediaInfo->codecCode;
    if (mp4GetCodecType(codec) == MP4_CODEC_H264)
        videoOutFilePath = outPath + fileBaseName + ".h264";
    else if (mp4GetCodecType(codec) == MP4_CODEC_HEVC)
        videoOutFilePath = outPath + fileBaseName + ".h265";
    else
        videoOutFilePath = outPath + fileBaseName + ".stream";

    ofstream audioFile;
    ofstream videoFile;
    videoFile.open(videoOutFilePath, ios_base::out | ios_base::binary | ios_base::trunc);
    if (!videoFile.is_open())
        return 0;

    if (audioTrackIdx >= 0)
    {
        codec = tracksInfo[audioTrackIdx]->mediaInfo->codecCode;
        if (MP4_CODEC_AAC == mp4GetCodecType(codec))
        {
            audioOutFilePath = outPath + fileBaseName + ".aac";
        }
        else
        {
            audioOutFilePath = outPath + fileBaseName + "_audio.stream";
        }
        audioFile.open(audioOutFilePath, ios_base::out | ios_base::binary | ios_base::trunc);
    }
    uint64_t wrSize = 0;
    for (uint32_t sampleIdx = 0; sampleIdx < tracksInfo[videoTrackIdx]->mediaInfo->samplesInfo.size(); sampleIdx++)
    {
        Mp4VideoFrame frame;
        ret = mp4Parser->getVideoSample(videoTrackIdx, sampleIdx, frame);
        if (ret < 0)
            break;
        videoFile.write((char *)frame.sampleData.get(), frame.dataSize);
        if (videoFile.fail())
        {
            printf("Writing to file failed.");
            break;
        }
        wrSize += frame.dataSize;
        fprintf(stderr, "\rWriting %" PRIu64 "...", wrSize);
    }

    if (audioTrackIdx >= 0 && audioFile.is_open())
    {
        for (uint32_t sampleIdx = 0; sampleIdx < tracksInfo[audioTrackIdx]->mediaInfo->samplesInfo.size(); sampleIdx++)
        {
            Mp4AudioFrame frame;
            ret = mp4Parser->getAudioSample(audioTrackIdx, sampleIdx, frame);
            if (ret < 0)
                break;
            audioFile.write((char *)frame.sampleData.get(), frame.dataSize);
            if (audioFile.fail())
            {
                printf("Writing to file failed.");
                break;
            }
        }
    }

    videoFile.close();
    if (audioFile.is_open())
        audioFile.close();

    fprintf(stderr, "\nFinish\n");

    printf("Press Enter to Exit\n");
    getchar();

    return 0;
}
