
#include "VideoModelConfig.h"

#include <QString>
#include <QSharedData>

class VideoModelConfigData : public QSharedData
{
    public:

        VideoModelConfigData() :
            QSharedData(),
            m_fps(0),
            m_crf(0)
        {
            this->m_suffix.clear();
        }

        VideoModelConfigData(const QString & suffix,
                        const int & fps,
                        const int & crf) :
            QSharedData(),
            m_suffix(suffix),
            m_fps(fps),
            m_crf(crf)
        {
            // TODO
        }

        virtual ~VideoModelConfigData()
        {
            // TODO
        }

        VideoModelConfigData(const VideoModelConfigData & modelConfigData) :
            m_suffix(modelConfigData.m_suffix),
            m_fps(modelConfigData.m_fps),
            m_crf(modelConfigData.m_crf)
        {
            // TODO
        }

        const VideoModelConfigData &operator=(const VideoModelConfigData &rhs)
        {
            if (this != &rhs)
            {
                this->m_suffix = rhs.m_suffix;
                this->m_fps = rhs.m_fps;
                this->m_crf = rhs.m_crf;
            }

            return *this;
        }

        bool operator !=(const VideoModelConfigData &rhs) const
        {
            return !(this == &rhs);
        }

        bool operator ==(const VideoModelConfigData &rhs) const
        {
            if ( (this->m_suffix != rhs.m_suffix) ||
                 (this->m_fps != rhs.m_fps) ||
                 (this->m_crf != rhs.m_crf) )
            {
                return false;
            }

            return true;
        }

        bool operator==(const VideoModelConfigData *ptr) const
        {
            return (this == ptr);
        }

        bool operator!=(const VideoModelConfigData *ptr) const
        {
            return (this != ptr);
        }

        QString suffix() const
        {
            return (this->m_suffix);
        }

        void setSuffix(const QString & newSuffix)
        {
            this->m_suffix = newSuffix;
        }

        int fps() const
        {
            return this->m_fps;
        }

        void setFps(qint32 newFps)
        {
            this->m_fps = newFps;
        }

        qint32 crf() const
        {
            return this->m_crf;
        }

        void setCrf(qint32 newCrf)
        {
            this->m_crf = newCrf;
        }

        bool isValid() const
        {
            return ( ! this->suffix().isEmpty() );
        }

    private:

        QString m_suffix;
        int m_fps;
        int m_crf;
};

VideoModelConfig::VideoModelConfig() :
    sharedDataP(new VideoModelConfigData)
{
    // TODO
}

VideoModelConfig::VideoModelConfig(const QString & suffix,
                         const int & fps,
                         const int & crf) :
    sharedDataP(new VideoModelConfigData(suffix, fps, crf))
{
    // TODO
}

VideoModelConfig::~VideoModelConfig()
{
    // TODO
}

VideoModelConfig::VideoModelConfig(const VideoModelConfig & rhs) :
    sharedDataP(rhs.sharedDataP)
{
    // TODO
}

const VideoModelConfig & VideoModelConfig::operator=(const VideoModelConfig & rhs)
{
    if (this != &rhs)
    {
        sharedDataP = rhs.sharedDataP;
    }

    return *this;
}

bool VideoModelConfig::operator ==(const VideoModelConfig & right) const
{
    if (this->sharedDataP != right.sharedDataP)
        return true;

    if (this->sharedDataP && right.sharedDataP)
    {
        return (*this->sharedDataP == *right.sharedDataP);
    }

    return false;
}

bool VideoModelConfig::operator !=(const VideoModelConfig & right) const
{
    return !(*this == right);
}

bool VideoModelConfig::operator==(const VideoModelConfig *ptr) const
{
    return (this == ptr);
}

bool VideoModelConfig::operator!=(const VideoModelConfig *ptr) const
{
    return (this != ptr);
}

QString VideoModelConfig::suffix() const
{
    return (this->sharedDataP->suffix());
}

void VideoModelConfig::setSuffix(const QString &newSuffix)
{
    if (this->sharedDataP->suffix() != newSuffix)
        this->sharedDataP->setSuffix(newSuffix);
}

qint32 VideoModelConfig::fps() const
{
    return (this->sharedDataP->fps());
}

void VideoModelConfig::setFps(qint32 newFps)
{
    if (newFps >= 0 && this->sharedDataP->fps() != newFps)
        this->sharedDataP->setFps(newFps);
}

qint32 VideoModelConfig::crf() const
{
    return (this->sharedDataP->crf());
}

void VideoModelConfig::setCrf(qint32 newCrf)
{
    if (newCrf >= 0 && this->sharedDataP->crf() != newCrf)
        this->sharedDataP->setCrf(newCrf);
}

bool VideoModelConfig::isValid() const
{
    return this->sharedDataP->isValid();
}
