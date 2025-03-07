
#include <string>

#include "Mp4Parse.h"

using namespace std;
static void splitpath(std::string &path, std::string &drv, std::string &dir, std::string &name, std::string &ext)
{
    uint64_t drv_pos  = path.find(':');
    uint64_t name_pos = path.rfind('/');
    if (string::npos == name_pos)
        name_pos = path.rfind('\\');
    uint64_t ext_pos = path.rfind('.');

    if (drv_pos != string::npos)
        drv = path.substr(0, ++drv_pos);
    else
        drv_pos = 0;

    if (name_pos != string::npos)
        dir = path.substr(drv_pos, ++name_pos - drv_pos);
    else
        name_pos = 0;

    if (ext_pos != string::npos)
        ext = path.substr(ext_pos, path.length() - ext_pos);
    else
        ext_pos = path.length();

    name = path.substr(name_pos, ext_pos - name_pos);
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("no input\n");
        return 0;
    }

    Mp4ParserHandle mp4Parser = createMp4Parser();

    int ret = 0;

    string mp4_file(argv[1]);

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
        return 0;
    }

    printf("%s\n", mp4Parser->getBasicInfoString().c_str());

    if (mp4Parser->getTracksInfo().empty())
    {
        printf("nothing found\n");
    }

    string fileName, ext, path, drive;

    splitpath(mp4_file, drive, path, fileName, ext);

    string outPath;
    outPath = drive + path;

    string outname;
    string outnameAudio;

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
        outname = outPath + fileName + ".h264";
    else if (mp4GetCodecType(codec) == MP4_CODEC_HEVC)
        outname = outPath + fileName + ".h265";
    else
        outname = outPath + fileName + ".stream";

    FILE *audioFp = nullptr;
    FILE *fp      = fopen(outname.c_str(), "wb+");
    if (!fp)
        return 0;
    if (audioTrackIdx >= 0)
    {
        codec = tracksInfo[audioTrackIdx]->mediaInfo->codecCode;
        if (MP4_CODEC_AAC == mp4GetCodecType(codec))
        {
            outnameAudio = outPath + fileName + ".aac";
        }
        else
        {
            outnameAudio = outPath + fileName + "_audio.stream";
        }
        audioFp = fopen(outnameAudio.c_str(), "wb+");
    }
    unsigned long long wrSize = 0;
    for (uint32_t sampleIdx = 0; sampleIdx < tracksInfo[videoTrackIdx]->mediaInfo->samplesInfo.size(); sampleIdx++)
    {
        Mp4VideoFrame frame;
        ret = mp4Parser->getVideoSample(videoTrackIdx, sampleIdx, frame);
        if (ret < 0)
            break;
        wrSize += fwrite(frame.sampleData.get(), 1, frame.dataSize, fp);
        fprintf(stderr, "\rWriting %lld...", wrSize);
    }

    if (audioTrackIdx >= 0 && audioFp)
    {
        for (uint32_t sampleIdx = 0; sampleIdx < tracksInfo[audioTrackIdx]->mediaInfo->samplesInfo.size(); sampleIdx++)
        {
            Mp4AudioFrame frame;
            ret = mp4Parser->getAudioSample(audioTrackIdx, sampleIdx, frame);
            if (ret < 0)
                break;
            wrSize += fwrite(frame.sampleData.get(), 1, frame.dataSize, fp);
            fwrite(frame.sampleData.get(), 1, frame.dataSize, audioFp);
        }
    }

    fclose(fp);
    if (audioFp)
        fclose(audioFp);

    fprintf(stderr, "\nFinish\n");

    printf("Press Enter to Exit\n");
    getchar();

    return 0;
}
