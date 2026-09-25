#ifndef VIDEOSEGMENTATIONWORKERFACTORY_H
#define VIDEOSEGMENTATIONWORKERFACTORY_H

#include "inc/IVideoSegmentationWorker.h"

#include "Defs.h"

class VideoSegmentationWorkerFactory
{
        /**
         * @brief classVersion
         */
        static const quint16 classVersion = quint16(101);

    public:

        /**
         * @brief createWorker
         * @param mode
         * @param parent
         * @return
         */
        static IVideoSegmentationWorker* createWorker(SegmentationMode_t mode, QObject *parent = nullptr);

        /**
         * @brief verifyEnvironment
         * @param mode
         * @param outDir
         * @param inputVideo
         * @param errorMessage
         * @return
         */
        static bool verifyEnvironment(SegmentationMode_t mode,
                                      const QString &outDir,
                                      const QString &inputVideo,
                                      QString &errorMessage);
};

#endif // VIDEOSEGMENTATIONWORKERFACTORY_H
