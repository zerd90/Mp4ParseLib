#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>

#include <memory>
#include <vector>
#include <string>

#include "Mp4Parse.h"

using std::string;

#define output_tab(fp, n)            \
    for (int _i = 0; _i < (n); ++_i) \
    {                                \
        fprintf(fp, "\t");           \
    }

void printBoxData(std::shared_ptr<Mp4BoxData> data, int layer, FILE *fp)
{
    for (unsigned int i = 0; i < data->size(); i++)
    {
        output_tab(fp, layer + 1);
        std::string key;
        key = data->kvGetKey(i);
        if (key == "Entries")
            fprintf(fp, "%s: ...\n", key.c_str());
        else
        {
            auto value = data->kvGetValue(key);
            if (MP4_BOX_DATA_TYPE_KEY_VALUE_PAIRS == value->getDataType())
            {
                fprintf(fp, "%s:\n", key.c_str());
                for (int keyIdx = 0; keyIdx < value->size(); keyIdx++)
                {
                    auto subKey  = value->kvGetKey(keyIdx);
                    auto subData = value->kvGetValue(subKey);
                    output_tab(fp, layer + 2);
                    fprintf(fp, "%s: %s\n", subKey.c_str(), subData->toString().c_str());
                }
            }
            else
                fprintf(fp, "%s: %s\n", key.c_str(), value->toString().c_str());
        }
    }
}
int printBoxProperty(const Mp4Box *box, int layer, FILE *fp)
{
    output_tab(fp, layer);
    fprintf(fp, "[%s]\n", box->getBoxTypeStr().c_str());

    std::shared_ptr<Mp4BoxData> data = box->getData();

    printBoxData(data, layer, fp);

    auto containBoxes = box->getSubBoxes();

    for (unsigned int i = 0; i < containBoxes.size(); i++)
    {
        printBoxProperty(containBoxes[i].get(), layer + 1, fp);
    }

    return 0;
}

void display(Mp4ParserHandle parser, FILE *outStream)
{
    auto file = parser->asBox();
    printBoxProperty(file.get(), 0, outStream);
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("no input\n");
        return 0;
    }

    Mp4ParserHandle mp4Parser = createMp4Parser();
    int             ret       = 0;
    string          mp4File(argv[1]);

    FILE *outStream = stderr;

    if (argc >= 3)
    {
        outStream = fopen(argv[2], "w+");
        if (!outStream)
        {
            printf("Open %s fail\n", argv[2]);
            return 0;
        }
    }

    ret = mp4Parser->parse(mp4File);

    if (ret < 0 || !mp4Parser->isParseSuccess())
    {
        return 0;
    }

    printf("%s\n", mp4Parser->getBasicInfoString().c_str());

    if (mp4Parser->getTracksInfo().empty())
    {
        printf("nothing found\n");
        return 0;
    }
    display(mp4Parser, outStream);
    if (outStream != stderr)
    {
        fclose(outStream);
    }
    return 0;
}
