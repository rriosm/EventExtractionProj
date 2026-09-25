#ifndef UTILS_H
#define UTILS_H

#include "Utils_global.h"

QT_BEGIN_NAMESPACE
#include <QDir>
#include <QObject>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
QT_END_NAMESPACE

//#include <quazip.h>
//#include <quazipfile.h>

namespace utils {

    class UTILSSHARED_EXPORT Utils: public QObject
    {
        Q_OBJECT

        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")

        /**
         * @brief classVersion
         */
        static const quint16 classVersion = quint16(101);

        public:

            /**
             * @brief Utils
             * @param parent
             */
            Utils(QObject *parent = nullptr);

            /**
             * @brief ~Utils
             */
            virtual ~Utils();

            /**
             * @brief DEFAULT_DOWNLOAD_DIR
             * @return
             */
            static const QString & DEFAULT_DOWNLOAD_DIR();

            /**
             * @brief FILE_BASENAME
             * @return
             */
            static const QString & FILE_BASENAME();

            /**
             * @brief DOWNLOAD_DIR_NAME
             * @return
             */
            static const QString & DOWNLOAD_DIR_NAME();

            // Check https://cpp.hotexamples.com/examples/-/QuaZip/-/cpp-quazip-class-examples.html

        //     /**
        //      * @brief archive
        //      * @param zipFilePath
        //      * @param dirToArchive
        //      * @param comment
        //      * @return
        //      */
        //     static bool archiveDir(const QString & zipFilePath,
        //                            const QDir & dirToArchive,
        //                            const QString & comment = QString(""));

        //     /**
        //      * @brief extract
        //      * @param zipFilePath
        //      * @param extDirPath
        //      * @param singleFileName
        //      * @return
        //      */
        //     static bool extractDir(const QString & zipFilePath,
        //                            const QString & extDirPath,
        //                            const QString & singleFileName = QString(""));

        // private:

        //     /**
        //      * @brief recursiveAddDir Recursively searches for all files in the directory
        //      *                        and adds to the list.
        //      * @param dir
        //      * @param filesList
        //      */
        //     static void recursiveAddDir(QDir dir,
        //                                 QStringList & filesList);
    };

} // utils

#endif // UTILS_H
