#ifndef PIPELINEWORKER_H
#define PIPELINEWORKER_H

#include <QObject>

#include "inc/Defs.h"

class PipelineWorker : public QObject
{
        Q_OBJECT

        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")
        Q_CLASSINFO("Description", "Controller for managing asynchronous MP3 audio segmentation")

        static const quint16 classVersion = quint16(101);

    public:

        explicit PipelineWorker(QObject *parent = nullptr);

    public slots:

        virtual void handleInput(FileInfo info) = 0;

    signals:

        /**
         * @brief finished
         * @param info
         */
        void finished(FileInfo info);

        /**
         * @brief manifestReady
         * @param info
         */
        void manifestReady(FileInfo info);
};

#endif // PIPELINEWORKER_H
