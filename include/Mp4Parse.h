#ifndef MP4_PARSE_H
#define MP4_PARSE_H

#include <functional>
#include "Mp4Defs.h"
#include "Mp4Types.h"

class Mp4Parser
{
public:
    virtual ~Mp4Parser() {}

public:
    virtual int  parse(std::string filePath) = 0;
    virtual void clear()                     = 0;

    virtual bool        isParseSuccess() const = 0;
    virtual std::string getErrorMessage()      = 0;

    virtual std::string getFileName() const      = 0;
    virtual float       getParseProgress() const = 0;
    virtual MP4_TYPE_E  getMp4Type() const       = 0;

    virtual BoxPtr                      asBox() const              = 0; // for more convenient box recursion
    virtual std::string                 getBasicInfoString() const = 0;
    virtual std::shared_ptr<Mp4BoxData> getBasicData() const       = 0;

    virtual const std::vector<TrackInfoPtr> &getTracksInfo() const = 0;
    virtual const std::vector<BoxPtr>        getBoxes() const      = 0;

    virtual bool isTrackHasProperty(unsigned int trackIdx, MP4_TRACK_PROPERTY_E prop) const = 0;

    virtual H26X_FRAME_TYPE_E parseVideoNaluType(uint32_t trackIdx, uint64_t sampleIdx) = 0;

    virtual int getAudioSample(uint32_t trackIdx, uint32_t sampleIdx, Mp4AudioFrame &frm) = 0;
    virtual int getVideoSample(uint32_t trackIdx, uint32_t sampleIdx, Mp4VideoFrame &frm) = 0;
    virtual int getSample(uint32_t trackIdx, uint32_t sampleIdx, Mp4RawSample &outFrame)  = 0;
};
typedef std::shared_ptr<Mp4Parser> Mp4ParserHandle;
Mp4ParserHandle                    createMp4Parser();

// data is after uuid(for uuid boxes) or box type(for other boxes);
// you can store the data in *pData in any form you prefer;
// return 0 if success, otherwise return a negative value;
using BoxParseFunc = std::function<int(uint8_t *data, uint64_t dataSize, void **pData)>;
//pData is the same as the one in BoxParseFunc;
using BoxDataFunc  = std::function<std::shared_ptr<Mp4BoxData>(void *pData)>;
void registerUdtaCallback(uint8_t uuid[MP4_UUID_LEN], BoxParseFunc parseDataCallback, BoxDataFunc getDataCallback);
void registerBoxCallback(Mp4BoxType boxType, BoxParseFunc parseDataCallback, BoxDataFunc getDataCallback);

void defaultLogCallback(MP4_LOG_LEVEL_E logLevel, const char *logBuffer);

#endif