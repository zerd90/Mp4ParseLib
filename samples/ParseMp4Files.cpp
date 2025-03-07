#include <stdio.h>
#include <stdlib.h>
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

void generateResultTable(Mp4ParserHandle mp4_info, string path)
{
    int    ret = 0;
    char   fn[200];
    string real_fn;

    if (createDir(path.c_str()) < 0)
    {
        printf("dir %s create fail\n", path.c_str());
        return;
    }
    auto tracksInfo = mp4_info->getTracksInfo();

    string prefix;
    string fileName = mp4_info->getFileName();
    size_t slash    = fileName.rfind('\\');
    if (slash == string::npos)
    {
        slash = fileName.rfind('/');
        if (slash == string::npos)
        {
            slash = 0;
        }
        else
        {
            slash++;
        }
    }
    else
    {
        slash++;
    }
    prefix = fileName.substr(slash, fileName.rfind('.') - slash);
    for (unsigned int i = 0; i < tracksInfo.size(); ++i)
    {
        auto          curTrackMedia = tracksInfo[i]->mediaInfo;
        std::ofstream res_fp;

        snprintf(fn, 200, "%s/%s_track_%d.csv", path.c_str(), prefix.c_str(), i + 1);
        real_fn = fn;
        res_fp.open(real_fn);
        if (!res_fp)
        {
            printf("can't open %s: %s\n", real_fn.c_str(), strerror(errno));
            break;
        }

        res_fp << "sampleIdx, offset, size, pts(ms), dts(ms), dts_delta(ms), frame_type, nalu_type, isKeyFrame,"
                  ",chunk_idx, offset, size, sample_start, sampleCount, start_pts(ms), delta(ms), avg_bitrate(Kbps)"
               << std::endl;
        for (unsigned int j = 0; j < curTrackMedia->samplesInfo.size(); j++)
        {
            res_fp << curTrackMedia->samplesInfo[j].sampleIdx << ","
                   << "0x" << std::hex << curTrackMedia->samplesInfo[j].sampleOffset << ","
                   << "0x" << curTrackMedia->samplesInfo[j].sampleSize << std::dec << ","
                   << curTrackMedia->samplesInfo[j].ptsMs << "," << curTrackMedia->samplesInfo[j].dtsMs << ","
                   << curTrackMedia->samplesInfo[j].dtsDeltaMs << ", ";
            auto codecType = mp4GetCodecType(curTrackMedia->codecCode);
            res_fp << mp4GetFrameTypeStr(curTrackMedia->samplesInfo[j].frameType) << ", ";
            for (int i = 0; i < curTrackMedia->samplesInfo[j].naluTypes.size(); i++)
            {
                res_fp << mp4GetNaluTypeStr(codecType, curTrackMedia->samplesInfo[j].naluTypes[i]);
                if (i < curTrackMedia->samplesInfo[j].naluTypes.size() - 1)
                {
                    res_fp << "|";
                }
            }
            res_fp << ", " << curTrackMedia->samplesInfo[j].isKeyFrame;
            if (j < curTrackMedia->chunksInfo.size())
            {
                res_fp << ",," << curTrackMedia->chunksInfo[j].chunkIdx << ","
                       << (unsigned long long)curTrackMedia->chunksInfo[j].chunkOffset << ","
                       << (unsigned int)curTrackMedia->chunksInfo[j].chunkSize << ","
                       << curTrackMedia->chunksInfo[j].sampleStartIdx << "," << curTrackMedia->chunksInfo[j].sampleCount
                       << "," << curTrackMedia->chunksInfo[j].startPtsMs << ","
                       << curTrackMedia->chunksInfo[j].durationMs << ","
                       << curTrackMedia->chunksInfo[j].avgBitrateBps / 1024.;
            }

            res_fp << std::endl;
        }
        res_fp.close();
        printf("%s\n", real_fn.c_str());
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("no input\n");
        return 0;
    }

    int file_idx;

    Mp4ParserHandle mp4Parser = createMp4Parser();

    for (file_idx = 1; file_idx < argc; ++file_idx)
    {
        int ret = 0;

        string mp4_file(argv[file_idx]);

        ret = mp4Parser->parse(mp4_file);

        if (ret < 0 || !mp4Parser->isParseSuccess())
        {
            printf("parse %s fail: \n", mp4_file.c_str());
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

        string   baseName, dirPath;
        fs::path filePath(mp4_file);
        dirPath  = filePath.parent_path().string();
        baseName = filePath.stem().string();
        string outPath;
        outPath = dirPath + "/" + baseName;

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

        generateResultTable(mp4Parser, outPath);
    }

    printf("Press Enter to Exit\n");
    getchar();

    return 0;
}
