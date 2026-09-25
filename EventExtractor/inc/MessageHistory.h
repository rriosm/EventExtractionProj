#ifndef MESSAGEHISTORY_H
#define MESSAGEHISTORY_H

#include <QSharedDataPointer>
#include <QList>
#include <QJsonArray>

#include "inc/Message.h"
#include "inc/Defs.h"


class MessageHistoryData;

class MessageHistory
{
        Q_GADGET

        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")
        Q_CLASSINFO("Description", "")

    public:

        /**
         * @brief MessageHistory
         */
        MessageHistory();

        /**
         * @brief MessageHistory
         */
        MessageHistory(const MessageHistory &rhs);

        /**
         * @brief operator =
         * @return
         */
        MessageHistory &operator=(const MessageHistory &rhs);

        /**
         * @brief operator ==
         * @param right
         * @return
         */
        bool operator ==(const MessageHistory &right) const;

        /**
         * @brief operator !=
         * @param right
         * @return
         */
        bool operator !=(const MessageHistory &right) const;

        /**
         * @brief operator ==
         * @param ptr
         * @return
         */
        bool operator==(const MessageHistory *ptr) const;

        /**
         * @brief operator !=
         * @param ptr
         * @return
         */
        bool operator!=(const MessageHistory *ptr) const;

        /**
         * @brief ~MessageHistory
         */
        ~MessageHistory();

        // Manipulation

        /**
         * @brief append
         * @param newMessage
         */
        void append(const Message &newMessage);

        /**
         * @brief append
         * @param newRole
         * @param newText
         */
        void append(Message::RoleType newRole, const QString &newText, const QString &media = "");

        /**
         * @brief prepend
         * @param message
         */
        void prepend(const Message &message);

        /**
         * @brief clear
         */
        void clear();

        // Inspection

        /**
         * @brief count
         * @return
         */
        qint32 count() const;

        /**
         * @brief isEmpty
         * @return
         */
        bool isEmpty() const;

        /**
         * @brief at
         * @param index
         * @return
         */
        Message at(qint32 index) const;

        /**
         * @brief last
         * @return
         */
        Message last() const;

        /**
         * @brief removeLast: Remove the last N messages (useful for "Retry" logic)
         * @param n
         */
        void removeLast(qint32 n = 1);

        /**
         * @brief messages
         * @return
         */
        QList<Message> messages() const;

        /**
         * @brief size
         * @return
         */
        qint32 size() const;

        /**
         * @brief updateMessageFileUri
         * @param filename
         * @param fileUri
         * @return
         */
        bool updateMessageFileUri(const QString &filename, const QString &fileUri);

        // Serialization and Playload construction

        /**
         * @brief toPayload: Generates the full root payload for API requests.
         *                   Handles Gemini's 'system_instruction' vs OpenRouter's 'messages' array.
         * @param schema
         * @param model
         * @return
         */
        QJsonObject toPayload(Message::Schema schema, const QString &model) const;

        /**
         * @brief toJsonArray: Exports the messages to a simple JSON array.
         * @return
         */
        QJsonArray toJsonArray(Message::Schema schema = Message::Schema::OpenRouter) const;

        /**
         * @brief fromJsonArray: Static factory to reconstruct history from a JSON array.
         * @param array
         * @return
         */
        static MessageHistory fromJsonArray(const QJsonArray &array);

        /**
         * @brief toString
         * @return
         */
        QString toString(Message::Schema schema = Message::Schema::OpenRouter) const;

    private:

        QSharedDataPointer<MessageHistoryData> sharedDataP;
};

#endif // MESSAGEHISTORY_H
