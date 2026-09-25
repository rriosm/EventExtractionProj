
#include <QSharedData>
#include <QJsonDocument>
#include <QJsonObject>

#include "inc/MessageHistory.h"
#include "inc/Message.h"

class MessageHistoryData : public QSharedData
{
    public:

        MessageHistoryData() = default;

        MessageHistoryData(const MessageHistoryData &other) :
            QSharedData(other),
            m_messages(other.m_messages)
        {
            // TODO
        }

        bool operator==(const MessageHistoryData &other) const
        {
            return (this->m_messages == other.m_messages);
        }

        QList<Message> m_messages;
};

MessageHistory::~MessageHistory() = default;

MessageHistory::MessageHistory() :
    sharedDataP(new MessageHistoryData)
{
    //
}

MessageHistory::MessageHistory(const MessageHistory &rhs) :
    sharedDataP(rhs.sharedDataP)
{
    //
}

MessageHistory &MessageHistory::operator=(const MessageHistory &rhs)
{
    if (this != &rhs)
    {
        this->sharedDataP.operator=(rhs.sharedDataP);
    }

    return *this;
}

bool MessageHistory::operator ==(const MessageHistory &right) const
{
    if (this->sharedDataP == right.sharedDataP) return true; // Same memory object

    return (*this->sharedDataP == *right.sharedDataP); // Deep value compare
}

bool MessageHistory::operator !=(const MessageHistory &right) const
{
    return !(*this == right);
}

bool MessageHistory::operator==(const MessageHistory *ptr) const
{
    return (this == ptr);
}

bool MessageHistory::operator!=(const MessageHistory *ptr) const
{
    return (this != ptr);
}

void MessageHistory::append(const Message &newMessage)
{
    this->sharedDataP->m_messages.append(newMessage);
}

void MessageHistory::append(Message::RoleType newRole, const QString &newText, const QString &media)
{
    Message msg(newRole, newText, media);
    this->sharedDataP->m_messages.append(msg);
}

void MessageHistory::prepend(const Message &message)
{
    this->sharedDataP->m_messages.prepend(message);
}

void MessageHistory::clear()
{
    this->sharedDataP->m_messages.clear();
}

qint32 MessageHistory::count() const
{
    return this->sharedDataP->m_messages.count();
}

bool MessageHistory::isEmpty() const
{
    return this->sharedDataP->m_messages.isEmpty();
}

Message MessageHistory::at(qint32 index) const
{
    return this->sharedDataP->m_messages.at(index);
}

Message MessageHistory::last() const
{
    return this->sharedDataP->m_messages.last();
}

void MessageHistory::removeLast(qint32 n)
{
    for (qint32 i = 0; i < n && !this->sharedDataP->m_messages.isEmpty(); ++i)
    {
        this->sharedDataP->m_messages.removeLast();
    }
}

QList<Message> MessageHistory::messages() const
{
    QList<Message> mList = this->sharedDataP->m_messages;

    return mList;
}

qint32 MessageHistory::size() const
{
    return this->sharedDataP->m_messages.size();
}

bool MessageHistory::updateMessageFileUri(const QString &filename, const QString &fileUri)
{
    if (filename.isEmpty() || fileUri.isEmpty()) return false;

    bool updated = false;

    for (qint32 idx = 0; idx < this->sharedDataP->m_messages.size(); ++idx)
    {
        QFileInfo fileInfo(this->sharedDataP->m_messages[idx].filePath());

        if (!fileInfo.exists()) continue;

        if (fileInfo.fileName() == filename)
        {
            this->sharedDataP->m_messages[idx].setFileUri(fileUri);

            updated = true;
        }
    }

    return updated;
}

// Serialization and Payload construction

QJsonObject MessageHistory::toPayload(Message::Schema schema, const QString & model) const
{
    QJsonObject root;
    root["model"] = model;

    QJsonArray contentsArr;
    QJsonObject systemInstruction;
    bool hasSystemInstruction = false;

    for (const Message &msg : this->sharedDataP->m_messages)
    {
        if ( schema == Message::Schema::Gemini && msg.isSystemRole())
        {
            QJsonArray parts;
            parts.append(QJsonObject{{"text", msg.content()}});
            systemInstruction["parts"] = parts;
            hasSystemInstruction = true;
        } else {
            contentsArr.append(msg.toJsonObject(schema));
        }
    }

    if (schema == Message::Schema::Gemini)
    {
        root["content"] = contentsArr;

        if (hasSystemInstruction)
        {
            root["system_instruction"] = systemInstruction;
        }
    } else {
        root["messages"] =  contentsArr;
    }

    return root;
}

QJsonArray MessageHistory::toJsonArray(Message::Schema schema) const
{
    QJsonArray msgArray;

    for (const auto &msg : this->sharedDataP->m_messages)
    {
        msgArray.append(msg.toJsonObject(schema));
    }

    return msgArray;
}

MessageHistory MessageHistory::fromJsonArray(const QJsonArray &array)
{
    MessageHistory history;

    for (const QJsonValue &value : array)
    {
        if (!value.isObject()) continue;
        QJsonObject obj = value.toObject();

        Message msg;
        msg.setRole(Message::roleFromStr(obj["role"].toString()));

        // Handle OpenRouter / OpenAI format (content can be string or array)
        if (obj.contains("content"))
        {
            if (obj["content"].isString()) {
                msg.setContent(obj["content"].toString());

            } else if (obj["content"].isArray()) {

                QJsonArray contentArray = obj["content"].toArray();

                for (const QJsonValue &partVal : qAsConst(contentArray))
                {
                    QJsonObject part = partVal.toObject();
                    QString type = part["type"].toString();

                    if (type == "text")
                    {
                        msg.setContent(part["text"].toString());

                    } else if (type == "image_url") {
                        QString url = part["image_url"].toObject()["url"].toString();
                        // Extract base64 if it's a data URI

                        if (url.startsWith("data:"))
                        {
                            msg.setMimeType(url.section(':', 1).section(';', 0));
                            msg.setMediaBase64(url.section(',', 1));
                        }
                    }
                }
            }
        }
        // Handle Gemini format (always uses 'parts' array)
        else if (obj.contains("parts") && obj["parts"].isArray())
        {
            QJsonArray partsArray = obj["parts"].toArray();

            for (const QJsonValue &partVal : qAsConst(partsArray))
            {
                QJsonObject part = partVal.toObject();

                if (part.contains("text")) {
                    msg.setContent(part["text"].toString());
                } else if (part.contains("inline_data")) {

                    QJsonObject data = part["inline_data"].toObject();
                    msg.setMimeType(data["mime_type"].toString());
                    msg.setMediaBase64(data["data"].toString());
                }
            }
        }

        history.append(msg);
    }

    return history;
}

QString MessageHistory::toString(Message::Schema schema) const
{
    return QJsonDocument(this->toJsonArray(schema)).toJson(QJsonDocument::Indented);
}
