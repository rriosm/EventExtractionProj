#ifndef AUDIOETIANDRE_H
#define AUDIOETIANDRE_H

#include <QObject>
#include <QExplicitlySharedDataPointer>

#include "Url.h"
using namespace br::pucrio::telemidia::nemo::utils;

class AudioData;

// Source: https://etiand.re/posts/2025/01/how-to-decode-audio-streams-in-c-cpp-using-libav/

class Audio : public QObject
{
        Q_OBJECT

    public:

        explicit Audio(Url mediaPath, QObject *parent = nullptr);

        void my_custom_audio_processing_function(int16_t *buf, int nsamples, int nchannels, int samplerate);

        void my_other_custom_audio_processing_function(int16_t *buf, int nchannels, int rate);

        bool cleanup();

    private:

        bool openAndDecodeContainerFormat(Url mediaPath);

        bool findAudioStream();

        bool discoverCodecForDecodingStream();

        bool getAudioStreamDetails();

        bool allocateOutputBuffer();

    signals:

    private:

        QExplicitlySharedDataPointer<AudioData> sharedDataP;

};

#endif // AUDIOETIANDRE_H

Q_DECLARE_METATYPE(Audio)
Q_DECLARE_METATYPE(Audio *)
Q_DECLARE_OPAQUE_POINTER(Audio *)
