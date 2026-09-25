#ifndef VIDEOMODELCONFIG_H
#define VIDEOMODELCONFIG_H

#include <QObject>
#include <QString>
#include <QMetaClassInfo>

#include <QSharedDataPointer>

class VideoModelConfigData;

class VideoModelConfig
{
        // Q_GADGET enables Q_PROPERTY and Q_CLASSINFO without inheriting QObject.
        // NOTE: Q_GADGETs cannot have signals or slots.
        Q_GADGET

        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")
        Q_CLASSINFO("Description", "Video Configuration Wrapper")

        // Properties allow access via Qt's Meta-System (e.g. QMetaProperty)
        Q_PROPERTY(QString suffix READ suffix WRITE setSuffix)
        Q_PROPERTY(int fps READ fps WRITE setFps)
        Q_PROPERTY(int crf READ crf WRITE setCrf)

        /**
         * @brief classVersion
         */
        static const quint16 classVersion = quint16(101);

    public:

        /**
         * @brief VideoModelConfig
         * @param parent
         */
        explicit VideoModelConfig();

        /**
         * @brief VideoModelConfig
         * @param suffix
         * @param fps
         * @param crf
         * @param parent
         */
        VideoModelConfig(const QString & suffix,
                    const int & fps,
                    const int & crf);

        /**
         * @brief ~VideoModelConfig
         */
        ~VideoModelConfig();

        /**
         * @brief VideoModelConfig
         * @param rhs
         */
        VideoModelConfig(const VideoModelConfig & rhs);

        /**
         * @brief operator =
         * @param right
         * @return
         */
        const VideoModelConfig &operator=(const VideoModelConfig & right);

        /**
         * @brief operator ==
         * @param right
         * @return
         */
        bool operator ==(const VideoModelConfig & right) const;

        /**
         * @brief operator !=
         * @param right
         * @return
         */
        bool operator !=(const VideoModelConfig & right) const;

        /**
         * @brief operator ==
         * @param ptr
         * @return
         */
        bool operator==(const VideoModelConfig *ptr) const;

        /**
         * @brief operator !=
         * @param ptr
         * @return
         */
        bool operator!=(const VideoModelConfig *ptr) const;

        /**
         * @brief suffix
         * @return
         */
        QString suffix() const;

        /**
         * @brief setSuffix
         * @param suffix
         */
        void setSuffix(const QString &suffix);

        /**
         * @brief fps
         * @return
         */
        qint32 fps() const;

        /**
         * @brief setFps
         * @param fps
         */
        void setFps(qint32 fps);

        /**
         * @brief crf
         * @return
         */
        qint32 crf() const;

        /**
         * @brief setCrf
         * @param crf
         */
        void setCrf(qint32 crf);

        /**
         * @brief isValid
         * @return
         */
        bool isValid() const;

    private:

        QSharedDataPointer<VideoModelConfigData> sharedDataP;

};

Q_DECLARE_METATYPE(VideoModelConfig)
Q_DECLARE_METATYPE(VideoModelConfig *)
//Q_DECLARE_OPAQUE_POINTER(VideoModelConfig *)

#endif // VIDEOMODELCONFIG_H
