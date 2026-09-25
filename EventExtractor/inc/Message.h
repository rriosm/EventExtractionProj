#ifndef MESSAGE_H
#define MESSAGE_H

#include <QObject>
#include <QPair>
#include <QVector>
#include <QSharedDataPointer>

//#include "Defs.h"

class MessageData;

class Message
{
        Q_GADGET

        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")
        Q_CLASSINFO("Description", "")

        /**
         * @brief classVersion
         */
        static const quint16 classVersion = quint16(101);

    public:

        /**
         * @brief The RoleType enum
         */
        enum class RoleType {
            NONE = 0X400D000,
            Assistant,
            System,
            Tool,
            User,
            Model
        };

        /**
         * @brief The Schema enum
         */
        enum class Schema {
            NONE = 0X400E000,
            Gemini,
            Google_HuggingFace,
            GPT_4o_Mini,
            OpenAI,
            OpenAI_Google,
            OpenRouter
        };

        /**
         * @brief ResponseMessage
         *          First = role
         *          Second = content
         */
        typedef QPair<Message::RoleType, QString> ResponseMessage;

        /**
         * @brief ResponseMessagesVec
         */
        typedef QVector<ResponseMessage> ResponseMessagesVec;

        /**
         * @brief Message
         */
        Message();

        /**
         * @brief Message
         * @param role
         * @param content
         * @param mediaBase64
         * @param mimeType
         */
        Message(const RoleType role,
                const QString &content,
                const QString &mediaBase64 = "",
                const QString &mimeType = "",
                const QString &filePath = "",
                const QString &fileUri = "");

        /**
         * @brief Message
         * @param other
         */
        Message(const Message &other);

        /**
         * @brief operator =
         * @param rhs
         * @return
         */
        Message &operator=(const Message &rhs);

        /**
         * @brief operator !=
         * @param rhs
         * @return
         */
        bool operator !=(const Message &rhs) const;

        /**
         * @brief operator ==
         * @param rhs
         * @return
         */
        bool operator ==(const Message &rhs) const;

        /**
         * @brief operator ==
         * @param ptr
         * @return
         */
        bool operator==(const Message *ptr) const;

        /**
         * @brief operator !=
         * @param ptr
         * @return
         */
        bool operator!=(const Message *ptr) const;

        /**
         * @brief ~Message
         */
        ~Message();

        /**
         * @brief role
         * @return
         */
        RoleType role() const;

        /**
         * @brief roleFromStr
         * @param role
         * @return
         */
        static RoleType roleFromStr(const QString &role);

        /**
         * @brief roleToStr
         * @param roleType
         * @return
         */

        static QString roleToStr(RoleType roleType);

        /**
         * @brief setRole
         * @param newRole
         */
        void setRole(const RoleType newRole);

        /**
         * @brief isSystemRole
         * @return
         */
        bool isSystemRole() const;

        /**
         * @brief content
         * @return
         */
        QString content() const;

        /**
         * @brief setContent
         * @param newContent
         */
        void setContent(const QString & newContent);

        /**
         * @brief filePath
         * @return
         */
        QString filePath() const;

        /**
         * @brief setFilePath
         * @param newFilePath
         */
        void setFilePath(const QString & newFilePath);

        /**
         * @brief mediaBase64
         * @return
         */
        QString mediaBase64() const;

        /**
         * @brief setMediaBase64
         * @param newMediaBase64
         */
        void setMediaBase64(const QString & newMediaBase64);

        /**
         * @brief fileUri
         * @return
         */
        const QString &fileUri() const;

        /**
         * @brief setFileUri
         * @param newFileUri
         */
        void setFileUri(const QString &newFileUri);

        /**
         * @brief mimeType
         * @return
         */
        QString mimeType() const;

        /**
         * @brief setMimeType
         * @param newMimeType
         */
        void setMimeType(const QString & newMimeType);

        /**
         * @brief hasMedia
         * @return
         */
        bool hasMedia() const;

        /**
         * @brief isEmpty
         * @return
         */
        bool isEmpty() const;

        /**
         * @brief fromLocalFile
         * @param role
         * @param text
         * @param filePath
         * @return
         */
        static Message fromLocalFile(const RoleType role, const QString &text, const QString &filePath);

        // Schema-aware serialization

        /**
         * @brief toJsonObject
         * @param schema
         * @return
         */
        QJsonObject toJsonObject(Schema schema = Schema::OpenRouter) const;

        /**
         * @brief toString
         * @param schema
         * @return
         */
        QString toString(Schema schema = Schema::OpenRouter) const;

    private:

        QSharedDataPointer<MessageData> sharedDataP;
};

Q_ENUMS(Message::RoleType);

inline bool operator==(const Message::RoleType type1, const Message::RoleType type2)
{
    return (static_cast<quint64> (type1) == static_cast<quint64> (type2));
}

inline uint qHash(const Message::RoleType type, uint seed = 0)
{
    return (::qHash(static_cast<quint64> (type), seed));
}

Q_DECLARE_METATYPE(Message)
Q_DECLARE_METATYPE(Message *)

#endif // MESSAGE_H
