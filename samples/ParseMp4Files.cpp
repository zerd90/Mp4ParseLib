#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>

#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>

#include "Mp4Parse.h"

using std::string;
namespace fs = std::filesystem;

int createDir(const char *path)
{
    printf("create dir %s\n", path);
    fs::directory_entry dir(path);

    std::error_code errCode;
    if (!dir.exists(errCode))
        return fs::create_directories(dir, errCode);

    return 0;
}

void generateResultTable(Mp4ParserHandle mp4_info, string dirPath)
{
    if (createDir(dirPath.c_str()) < 0)
    {
        printf("dir %s create fail\n", dirPath.c_str());
        return;
    }
    auto     tracksInfo = mp4_info->getTracksInfo();
    fs::path mp4FilePath(mp4_info->getFilePath());
    string   prefix = mp4FilePath.stem().string();
    for (unsigned int i = 0; i < tracksInfo.size(); ++i)
    {
        auto          curTrackMedia = tracksInfo[i]->mediaInfo;
        std::ofstream trackInfoFile;
        string        trackInfoFilePath = dirPath + "/" + prefix + "_track_" + std::to_string(i + 1) + ".csv";
        trackInfoFile.open(trackInfoFilePath, std::ios_base::out | std::ios_base::trunc);
        if (!trackInfoFile.is_open())
        {
            printf("can't open %s: %s\n", trackInfoFilePath.c_str(), strerror(errno));
            break;
        }

        trackInfoFile
            << "sampleIdx, offset, size, pts(ms), dts(ms), dts_delta(ms), frame_type, nalu_type, isKeyFrame,"
               ",chunk_idx, offset, size, sample_start, sampleCount, start_pts(ms), delta(ms), avg_bitrate(Kbps)"
            << std::endl;
        for (unsigned int j = 0; j < curTrackMedia->samplesInfo.size(); j++)
        {
            trackInfoFile << curTrackMedia->samplesInfo[j].sampleIdx << ","
                          << "0x" << std::hex << curTrackMedia->samplesInfo[j].sampleOffset << ","
                          << "0x" << curTrackMedia->samplesInfo[j].sampleSize << std::dec << ","
                          << curTrackMedia->samplesInfo[j].ptsMs << "," << curTrackMedia->samplesInfo[j].dtsMs << ","
                          << curTrackMedia->samplesInfo[j].dtsDeltaMs << ", ";
            auto codecType = mp4GetCodecType(curTrackMedia->codecCode);
            trackInfoFile << mp4GetFrameTypeStr(curTrackMedia->samplesInfo[j].frameType) << ", ";
            for (int naluIdx = 0; naluIdx < curTrackMedia->samplesInfo[j].naluTypes.size(); naluIdx++)
            {
                trackInfoFile << mp4GetNaluTypeStr(codecType, curTrackMedia->samplesInfo[j].naluTypes[naluIdx]);
                if (naluIdx < curTrackMedia->samplesInfo[j].naluTypes.size() - 1)
                {
                    trackInfoFile << "|";
                }
            }
            trackInfoFile << ", " << curTrackMedia->samplesInfo[j].isKeyFrame;
            if (j < curTrackMedia->chunksInfo.size())
            {
                trackInfoFile << ",," << curTrackMedia->chunksInfo[j].chunkIdx << ","
                              << (unsigned long long)curTrackMedia->chunksInfo[j].chunkOffset << ","
                              << (unsigned int)curTrackMedia->chunksInfo[j].chunkSize << ","
                              << curTrackMedia->chunksInfo[j].sampleStartIdx << ","
                              << curTrackMedia->chunksInfo[j].sampleCount << ","
                              << curTrackMedia->chunksInfo[j].startPtsMs << ","
                              << curTrackMedia->chunksInfo[j].durationMs << ","
                              << curTrackMedia->chunksInfo[j].avgBitrateBps / 1024.;
            }

            trackInfoFile << std::endl;
        }
        trackInfoFile.close();
        printf("%s\n", trackInfoFilePath.c_str());
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("no input\n");
        return 0;
    }

    int fileIdx;

    Mp4ParserHandle mp4Parser = createMp4Parser();

    for (fileIdx = 1; fileIdx < argc; ++fileIdx)
    {
        int ret = 0;

        string mp4FilePath(argv[fileIdx]);
        if (!fs::exists(mp4FilePath))
        {
            printf("file %s not exist\n", mp4FilePath.c_str());
            continue;
        }

        printf("parse %s\n", mp4FilePath.c_str());

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
            continue;
        }

        string basicInfo = mp4Parser->getBasicInfoString();
        printf("%s\n", basicInfo.c_str());

        if (mp4Parser->getTracksInfo().empty())
        {
            printf("nothing found\n");
            continue;
        }

        fs::path filePath(mp4FilePath);
        string   dirPath    = filePath.parent_path().string();
        string   baseName   = filePath.stem().string();
        string   outDirPath = dirPath + "/" + baseName;

        auto tracksInfo = mp4Parser->getTracksInfo();
        for (int i = 0; i < tracksInfo.size(); i++)
        {
            if (TRACK_TYPE_VIDEO != tracksInfo[i]->trackType)
                continue;
            auto codecType = mp4GetCodecType(tracksInfo[i]->mediaInfo->codecCode);
            if (MP4_CODEC_H264 != codecType && MP4_CODEC_HEVC != codecType)
                continue;
            for (uint32_t sampleIdx = 0; sampleIdx < tracksInfo[i]->mediaInfo->samplesInfo.size(); sampleIdx++)
            {
                int frameType = mp4Parser->parseVideoNaluType(i, sampleIdx);
            }
        }

        generateResultTable(mp4Parser, outDirPath);
    }

    printf("Press Enter to Exit\n");
    getchar();

    return 0;
}
