
#include <QSharedData>

#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#include "inc/Defs.h"
#include "inc/LogUtils.h"

#include "inc/Message.h"

class MessageData : public QSharedData
{
    public:

        MessageData() = default;
        ~MessageData() = default;

        MessageData(const MessageData &other) :
            QSharedData(other),
            m_role(other.m_role),
            m_content(other.m_content),
            m_filePath(other.m_filePath),
            m_mediaBase64(other.m_mediaBase64),
            m_fileUri(other.m_fileUri),
            m_mimeType(other.m_mimeType)
        {
            // TODO
        }

        bool operator==(const MessageData &other) const
        {
            return (this->m_role == other.m_role &&
                    this->m_content == other.m_content &&
                    this->m_filePath == other.m_filePath &&
                    this->m_mediaBase64 == other.m_mediaBase64 &&
                    this->m_fileUri == other.m_fileUri &&
                    this->m_mimeType == other.m_mimeType);
        }

        Message::RoleType m_role;
        QString m_content;
        QString m_filePath;
        QString m_mediaBase64;
        QString m_fileUri;          // The file URI in model file API.
        QString m_mimeType;
};

Message::Message() :
    sharedDataP(new MessageData)
{
    this->sharedDataP->m_role = Message::RoleType::NONE;
    this->sharedDataP->m_content.clear();
    this->sharedDataP->m_filePath.clear();
    this->sharedDataP->m_mediaBase64.clear();
    this->sharedDataP->m_fileUri.clear();
    this->sharedDataP->m_mimeType.clear();
}

Message::Message(const Message::RoleType role,
                 const QString &content,
                 const QString &mediaBase64,
                 const QString &mimeType,
                 const QString &filePath,
                 const QString &fileUri) :
    sharedDataP(new MessageData)
{
    this->sharedDataP->m_role = role;
    this->sharedDataP->m_content = content;
    this->sharedDataP->m_filePath = filePath;
    this->sharedDataP->m_mediaBase64 = mediaBase64;
    this->sharedDataP->m_fileUri = fileUri;
    this->sharedDataP->m_mimeType = mimeType;
}

Message::~Message() = default;

Message::Message(const Message &other) :
    sharedDataP(other.sharedDataP)
{
    // TODO
}

Message &Message::operator=(const Message &rhs)
{
    if (*this != rhs)
    {
        this->sharedDataP.operator=(rhs.sharedDataP);
    }

    return *this;
}

bool Message::operator !=(const Message &rhs) const
{
    return !(this == &rhs);
}

bool Message::operator ==(const Message &rhs) const
{
    // return ( this->sharedDataP->m_role == rhs.sharedDataP->m_role &&
    //          this->sharedDataP->m_content == rhs.sharedDataP->m_content &&
    //          this->sharedDataP->m_filePath == rhs.sharedDataP->m_filePath &&
    //          this->sharedDataP->m_mediaBase64 == rhs.sharedDataP->m_mediaBase64 &&
    //          this->sharedDataP->m_fileUri == rhs.sharedDataP->m_fileUri &&
    //          this->sharedDataP->m_mimeType == rhs.sharedDataP->m_mimeType
    // );

    if (this->sharedDataP == rhs.sharedDataP) return true; // Same memory object

    return (*this->sharedDataP == *rhs.sharedDataP); // Deep value compare
}

bool Message::operator==(const Message *ptr) const
{
    return (this == ptr);
}

bool Message::operator!=(const Message *ptr) const
{
    return (this != ptr);
}

Message::RoleType Message::role() const
{
    return this->sharedDataP->m_role;
}

Message::RoleType Message::roleFromStr(const QString &role)
{
    if ( ! QString::compare(role, "assistant", Qt::CaseInsensitive) )
    {
        return Message::RoleType::Assistant;
    }

    if ( ! QString::compare(role, "system",Qt::CaseInsensitive) )
    {
        return Message::RoleType::Assistant;
    }

    if ( ! QString::compare(role, "tool",Qt::CaseInsensitive) )
    {
        return Message::RoleType::Assistant;
    }

    if ( ! QString::compare(role, "user",Qt::CaseInsensitive) )
    {
        return Message::RoleType::Assistant;
    }

    if ( ! QString::compare(role, "model",Qt::CaseInsensitive) )
    {
        return Message::RoleType::Assistant;
    }

    return Message::RoleType::NONE;
}

QString Message::roleToStr(Message::RoleType roleType)
{
    QString role;

    switch (roleType)
    {
        case Message::RoleType::Assistant:
        {
            role = "assistant";

            break;
        }

        case Message::RoleType::System:
        {
            role = "system";

            break;
        }

        case Message::RoleType::Tool:
        {
            role = "tool";

            break;
        }

        case Message::RoleType::User:
        {
            role = "user";

            break;
        }

        case Message::RoleType::Model:
        {
            role = "model";

            break;
        }

        case Message::RoleType::NONE:
        {
            //
        }
    }

    return role;
}

void Message::setRole(const Message::RoleType newRole)
{
    if (newRole != this->sharedDataP->m_role)
        this->sharedDataP->m_role = newRole;
}

bool Message::isSystemRole() const
{
    return (this->sharedDataP->m_role == Message::RoleType::System);
}

QString Message::content() const
{
    return this->sharedDataP->m_content;
}

void Message::setContent(const QString & newContent)
{
    if (newContent != this->sharedDataP->m_content)
        this->sharedDataP->m_content = newContent;
}

QString Message::filePath() const
{
    return this->sharedDataP->m_filePath;
}

void Message::setFilePath(const QString & newFilePath)
{
    if (newFilePath != this->sharedDataP->m_filePath)
        this->sharedDataP->m_filePath = newFilePath;
}

QString Message::mediaBase64() const
{
    return this->sharedDataP->m_mediaBase64;
}

void Message::setMediaBase64(const QString &newMediaBase64)
{
    if (newMediaBase64 != this->sharedDataP->m_mediaBase64)
        this->sharedDataP->m_mediaBase64 = newMediaBase64;
}

const QString &Message::fileUri() const
{
    return this->sharedDataP->m_fileUri;
}

void Message::setFileUri(const QString &newFileUri)
{
    if (newFileUri == this->sharedDataP->m_fileUri) return;

    this->sharedDataP->m_fileUri = newFileUri;
}

QString Message::mimeType() const
{
    return this->sharedDataP->m_mimeType;
}

void Message::setMimeType(const QString & newMimeType)
{
    if (newMimeType != this->sharedDataP->m_mimeType)
        this->sharedDataP->m_mimeType = newMimeType;
}

bool Message::hasMedia() const
{
    return (!this->sharedDataP->m_mediaBase64.isEmpty() ||
            !this->sharedDataP->m_fileUri.isEmpty() ||
            !this->sharedDataP->m_filePath.isEmpty());
}

bool Message::isEmpty() const
{
    return (!this->content().isEmpty());
}

Message Message::fromLocalFile(const Message::RoleType role, const QString &text, const QString &filePath)
{
    Message msg(role, text);
    msg.setFilePath(filePath);

    QFile file(filePath);

    if (file.size() <= *Constants::Q_QGS_DEFAULT_FILE_SIZE)
    {
        if (file.open(QIODevice::ReadOnly))
        {
            msg.setMediaBase64(file.readAll().toBase64());
        }
    }

    QMimeDatabase db;
    msg.setMimeType(db.mimeTypeForFile(filePath).name());

    return msg;
}

QJsonObject Message::toJsonObject(Schema schema) const
{
    QJsonObject mObj;

#pragma clang diagnostic ignored "-Wswitch"
    switch (schema)
    {
        // 1. Logic for Gemini
        case Schema::Gemini:
        {
            if ( this->sharedDataP->m_role == Message::RoleType::System)
            {
                mObj["text"] = this->sharedDataP->m_content;

            } else {

                mObj["role"] = (this->sharedDataP->m_role == Message::RoleType::Assistant) ? this->roleToStr(Message::RoleType::Model) : this->roleToStr(Message::RoleType::User);

                // In Gemini, system messages are handled at the root payload,
                // but individual messages use the 'parts' structure.
                QJsonArray partsArr;

                if (!this->sharedDataP->m_content.isEmpty())
                {
                    QJsonObject textPart;
                    textPart["text"] = this->sharedDataP->m_content;
                    partsArr.append(textPart);
                }

                QJsonObject mediaPart;

                if (this->hasMedia())
                {
                    if (! this->sharedDataP->m_fileUri.isEmpty())
                    {
                        QJsonObject fileData;
                        fileData["mime_type"] = this->sharedDataP->m_mimeType;
                        fileData["file_uri"] = this->sharedDataP->m_fileUri;

                        mediaPart["file_data"] = fileData;
                    }
                    else if (!this->sharedDataP->m_mediaBase64.isEmpty())
                    {
                        QJsonObject inlineData;

                        inlineData["mime_type"] = this->sharedDataP->m_mimeType;
                        inlineData["data"] = this->sharedDataP->m_mediaBase64;

                        mediaPart["inline_data"] = inlineData;

                    }

                    if (!mediaPart.isEmpty())
                    {
                        partsArr.append(mediaPart);
                    }
                }

                if (!partsArr.isEmpty())
                {
                    mObj["parts"] = partsArr;
                }
            }

            break;
        }
        // 2. Logic for OpenRouter (OpenAI-compatible)
        case Schema::OpenRouter:
        {
            mObj["role"] = this->roleToStr(this->sharedDataP->m_role);

            if (!this->hasMedia())
            {
                mObj["content"] = this->sharedDataP->m_content;

            } else {

                QJsonArray contentArray;

                if (!this->sharedDataP->m_content.isEmpty())
                {
                    QJsonObject textObj;
                    textObj["type"] = "text";
                    textObj["text"] = this->sharedDataP->m_content;
                    contentArray.append(textObj);
                }

                QJsonObject mediaObj;

                if (this->sharedDataP->m_mimeType.startsWith("audio/"))
                {
                    // OpenRouter/OpenAI specific audio handling
                    mediaObj["type"] = "input_audio";

                    QJsonObject inputAudio;
                    inputAudio["data"] = this->sharedDataP->m_mediaBase64; // Raw Base64 (No Data URI prefix)

                    QString format = this->sharedDataP->m_mimeType.split("/").last();
                    inputAudio["format"] = format;

                    mediaObj["input_audio"] = inputAudio;
                } else {

                    QString dataUri;

                    if (!this->sharedDataP->m_mediaBase64.isEmpty())
                    {
                        dataUri = QString("data:%1;base64,%2").arg(this->sharedDataP->m_mimeType,
                                                                   this->sharedDataP->m_mediaBase64);

                    } else if (this->sharedDataP->m_filePath.isEmpty())
                    {
                        // TODO
                    }

                    if (!dataUri.isEmpty())
                    {
                        QJsonObject urlObj;
                        urlObj["url"] = dataUri;

                        if (this->sharedDataP->m_mimeType.startsWith("image/"))
                        {
                            mediaObj["type"] = "image_url";
                            mediaObj["image_url"] = dataUri;

                        } /*else {
                            mediaObj["type"] = "video_url";
                            mediaObj["video_url"] = dataUri;
                        }*/
                    }
                }

                if (!mediaObj.isEmpty())
                {
                    contentArray.append(mediaObj);
                }

                if (!contentArray.isEmpty())
                {
                    mObj["content"] = contentArray;
                }
            }

            break;
        }

        case Schema::Google_HuggingFace:
        {
            mObj["role"] = this->roleToStr(this->sharedDataP->m_role);

            // HF Vision Models (Idefics, Llava, etc.) support standard 'image_url'
            // Audio/Video is NOT standard in HF Chat API yet.
            if (!this->hasMedia())
            {
                // Standard Text Message
                mObj["content"] = this->sharedDataP->m_content;

            } else {

                QJsonArray contentArr;

                // 1. Text Part
                if (!this->sharedDataP->m_content.isEmpty())
                {
                    QJsonObject textObj;
                    textObj["type"] = "text";
                    textObj["text"] = this->sharedDataP->m_content;
                    contentArr.append(textObj);
                }

                QJsonObject mediaObj;

                // 2. Image Part Only
                if (this->sharedDataP->m_mimeType.startsWith("image/"))
                {
                    mediaObj["type"] = "image_url";

                    QString dataUri;

                    if (!this->sharedDataP->m_mediaBase64.isEmpty())
                    {
                        // HF expects Data URI: "data:image/jpeg;base64,..."
                        dataUri = QString("data:%1;base64,%2").arg(this->sharedDataP->m_mimeType,
                                                                   this->sharedDataP->m_mediaBase64);

                    } else if (this->sharedDataP->m_filePath.isEmpty())
                    {
                        // TODO
                    }

                    if (!dataUri.isEmpty())
                    {
                        QJsonObject urlObj;
                        urlObj["url"] = dataUri;

                        mediaObj["image_url"] = urlObj;
                    }
                }
                // Warn or skip Audio/Video for HF to prevent API rejection
                else if (this->sharedDataP->m_mimeType.startsWith("audio/") ||
                         this->sharedDataP->m_mimeType.startsWith("video/"))
                {
                    qDebug() << "Warning: Hugging Face Chat API does not natively support Audio/Video inputs. Skipping media.";

#ifdef QT_DEBUG
                    LogUtils::safeLog(QString("QJsonObject Message::toJsonObject(...) - Warning: Hugging Face Chat API does not natively support Audio/Video inputs. Skipping media."),
                                      __FILE__,
                                      __LINE__,
                                      Q_FUNC_INFO);
#endif
                }

                if (!mediaObj.isEmpty())
                {
                    contentArr.append(mediaObj);
                }

                if (!contentArr.isEmpty())
                {
                    mObj["content"] = contentArr;
                }
            }

            break;
        }

        case Schema::GPT_4o_Mini:
        case Schema::OpenAI:
        case Schema::OpenAI_Google:
        {
            QJsonObject mObj;
            mObj["role"] = this->roleToStr(this->sharedDataP->m_role);

            if (!this->hasMedia())
            {
                mObj["content"] = this->sharedDataP->m_content;

            } else {

                QJsonArray contentArr;

                // 1. Text Part
                if (!this->sharedDataP->m_content.isEmpty())
                {
                    QJsonObject textObj;
                    textObj["type"] = "text";
                    textObj["text"] = this->sharedDataP->m_content;
                    contentArr.append(textObj);
                }

                QJsonObject mediaObj;

                // 2. Audio Part (e.g., gpt-4o-audio-preview)
                if (this->sharedDataP->m_mimeType.startsWith("audio/"))
                {
                    mediaObj["type"] = "input_audio";

                    QJsonObject audioData;
                    audioData["data"] = this->sharedDataP->m_mediaBase64; // RAW Base64 (No prefix)

                    // Extract format: "audio/wav" -> "wav"
                    QString format = this->sharedDataP->m_mimeType.split("/").last();
                    audioData["format"] = format;

                    mediaObj["input_audio"] = audioData;
                }
                // 3. Image Part
                else if (this->sharedDataP->m_mimeType.startsWith("image/"))
                {
                    mediaObj["type"] = "image_url";

                    // OpenAI expects Data URI
                    QString dataUri;

                    if (!this->sharedDataP->m_mediaBase64.isEmpty())
                    {
                        // HF expects Data URI: "data:image/jpeg;base64,..."
                        dataUri = QString("data:%1;base64,%2").arg(this->sharedDataP->m_mimeType,
                                                                   this->sharedDataP->m_mediaBase64);

                    } else if (this->sharedDataP->m_filePath.isEmpty())
                    {
                        // TODO
                    }

                    if (!dataUri.isEmpty())
                    {
                        QJsonObject urlObj;
                        urlObj["url"] = dataUri;

                        mediaObj["image_url"] = urlObj;
                    }
                }
                // 4. Video Handling (Frame-based Workaround)
                else if (this->sharedDataP->m_mimeType.startsWith("video/"))
                {
                    // STRICT OPENAI RULE:
                    // OpenAI Chat API does NOT support 'video_url' or 'input_video'.
                    // You must send video as a sequence of images (frames).
                    // If you are just sending a file, this will fail.

                    qDebug() << "Warning: OpenAI Chat API requires video to be sent as image frames, not as a video file.";

#ifdef QT_DEBUG
                    LogUtils::safeLog(QString("QJsonObject Message::toJsonObject(...) - Warning: OpenAI Chat API requires video to be sent as image frames, not as a video file."),
                                      __FILE__,
                                      __LINE__,
                                      Q_FUNC_INFO);
#endif

                    // OPTIONAL: If using a wrapper that supports it (like OpenRouter), you can try 'video_url':
                    /*
                    QJsonObject vidObj;
                    vidObj["type"] = "video_url"; // Non-Standard!
                    QJsonObject urlObj;
                    urlObj["url"] = QString("data:%1;base64,%2").arg(msg.mimeType, msg.mediaBase64);
                    vidObj["video_url"] = urlObj;
                    contentArr.append(vidObj);
                    */
                }

                if (!mediaObj.isEmpty())
                {
                    contentArr.append(mediaObj);
                }

                if (!contentArr.isEmpty())
                {
                    mObj["content"] = contentArr;
                }
            }

            break;
        }
    }

    return mObj;
}

QString Message::toString(Schema schema) const
{
    return QJsonDocument(this->toJsonObject(schema)).toJson(QJsonDocument::Indented);
}
