
#include "Utils.h"

#include "Logger.h"
using namespace logger;

namespace utils {

    Utils::Utils(QObject *parent) :
        QObject(parent)
    {
        // TODO
    }

    Utils::~Utils()
    {
        // TODO
    }

    const QString & Utils::DEFAULT_DOWNLOAD_DIR()
    {
        static const QString Default_Download_Dir = "/Users/rrios/Downloads/";

        return (Default_Download_Dir);
    }

    const QString & Utils::FILE_BASENAME()
    {
        static const QString FILE_Basename = "download";

        return (FILE_Basename);
    }

    const QString & Utils::DOWNLOAD_DIR_NAME()
    {
        static const QString DOWNLOAD_DIR = QStandardPaths::displayName(QStandardPaths::DownloadLocation);

        return (DOWNLOAD_DIR);
    }

    // From https://stackoverflow.com/questions/2598117/zipping-a-folder-file-using-qt
    // bool Utils::archiveDir(const QString & zipFilePath,
    //                       const QDir & dirToArchive,
    //                       const QString & comment)
    // {
    //    if ( ! dirToArchive.exists() )
    //    {
    //        Logger::instance()->log(QtDebugMsg,
    //                                QString(tr("Utils::archive(const QString & filePath, const QDir & dir, const QString & comment) - ") +
    //                                        tr("dir.exists(%1)=FALSE")).arg(dirToArchive.absolutePath()));
    //        return (false);
    //    }

    //    // Archive file.
    //    QuaZip zip(zipFilePath);

    // #ifdef Q_OS_WIN32
    //    zip.setFileNameCodec("IBM866");
    // #endif

    //    // Open zip file.
    //    if ( ! zip.open(QuaZip::mdCreate) )
    //    {
    //        Logger::instance()->log(QtDebugMsg,
    //                                QString(tr("Utils::archive(const QString & filePath, const QDir & dir, const QString & comment) - ") +
    //                                        tr("testCreate(): zip.open(): %1")).arg(zip.getZipError()));

    //        return (false);
    //    }

    //    // Get a list of files and folders recursively.
    //    QStringList filesNDirsList;
    //    recursiveAddDir(dirToArchive, filesNDirsList);

    //    // Create an array of QFileInfo objects.
    //    QFileInfoList filesInfoList;
    //    foreach (QString fn, filesNDirsList) filesInfoList << QFileInfo(fn);

    //    // File to be inserted in an archive file.
    //    QFile inFile;

    //    // Zip file inside an archive file.
    //    QuaZipFile outZipFile(&zip);

    //    char c;

    //    foreach(QFileInfo fileInfo, filesInfoList) {

    //        if ( ! fileInfo.isFile() )
    //        {
    //            continue;
    //        }

    //        // If the file is in a subdirectory, then add the name of this subdirectory to the file names.
    //        // For example: fileInfo.filePath() = "D:\Work\Sources\SAGO\svn\sago\Release\tmp_DOCSWIN\Folder\123.opn",
    //        // then after removing part of the line fileNameWithSubFolders will be "Folder\123.opn", etc.
    //        QString fileNameWithRelativePath = fileInfo.filePath().remove(0, dirToArchive.absolutePath().length() + 1);

    //        inFile.setFileName(fileInfo.filePath());

    //        if ( ! inFile.open(QIODevice::ReadOnly) )
    //        {
    //            Logger::instance()->log(QtDebugMsg,
    //                                    QString(tr("Utils::archive(const QString & filePath, const QDir & dir, const QString & comment) - ") +
    //                                            tr("testCreate(): inFile.open(): %1")).arg(inFile.errorString().toLocal8Bit().constData()));

    //            return (false);
    //        }

    //        if ( ! outZipFile.open(QIODevice::WriteOnly, QuaZipNewInfo(fileNameWithRelativePath, fileInfo.filePath())) )
    //        {
    //            Logger::instance()->log(QtDebugMsg,
    //                                    QString(tr("Utils::archive(const QString & filePath, const QDir & dir, const QString & comment) - ") +
    //                                            tr("testCreate(): outFile.open(): %1")).arg(outZipFile.getZipError()));

    //            return (false);
    //        }

    // //                            while (inFile.getChar(&c) && outZipFile.putChar(c));

    //        while (inFile.getChar(&c))
    //        {
    //            outZipFile.putChar(c);
    //        }

    //        if (outZipFile.getZipError() != UNZ_OK)
    //        {
    //            Logger::instance()->log(QtDebugMsg,
    //                                    QString(tr("Utils::archive(const QString & filePath, const QDir & dir, const QString & comment) - ") +
    //                                            tr("testCreate(): outFile.putChar(): %1")).arg(outZipFile.getZipError()));

    //            return (false);
    //        }

    //        // Close zip file.
    //        outZipFile.close();

    //        if (outZipFile.getZipError() != UNZ_OK)
    //        {
    //            Logger::instance()->log(QtDebugMsg,
    //                                    QString(tr("Utils::archive(const QString & filePath, const QDir & dir, const QString & comment) - ") +
    //                                            tr("testCreate(): outFile.close(): %1")).arg(outZipFile.getZipError()));

    //            return (false);
    //        }

    //        // Close inseted file.
    //        inFile.close();
    //    }

    //    // Add comment if zip file has none.
    //    if ( ! comment.isEmpty() )
    //    {
    //        zip.setComment(comment);
    //    }

    //    // Close zip file.
    //    zip.close();

    //    if (zip.getZipError() != UNZ_OK)
    //    {
    //        Logger::instance()->log(QtDebugMsg,
    //                                QString(tr("Utils::archive(const QString & filePath, const QDir & dir, const QString & comment) - ") +
    //                                        tr("testCreate(): zip.close(): %1")).arg(zip.getZipError()));

    //        return (false);
    //    }

    //    return true;
    // }

    // // From https://stackoverflow.com/questions/2598117/zipping-a-folder-file-using-qt
    // bool Utils::extractDir(const QString & zipFilePath,
    //                       const QString & extDirPath,
    //                       const QString & singleFileName)
    // {
    //    // Archive file to be extracted.
    //    QuaZip zip(zipFilePath);

    //    if ( ! zip.open(QuaZip::mdUnzip) )
    //    {
    //        Logger::instance()->log(QtWarningMsg,
    //                                QString(tr("Utils::archive(const QString & filePath, const QString & extDirPath, const QString & "
    //                                           "singleFileName) - ") +
    //                                        tr("testRead(): zip.open(): %1")).arg(zip.getZipError()));

    //        return (false);
    //    }

    // #ifdef Q_OS_WIN32
    //    zip.setFileNameCodec("IBM866");
    // #endif

    //    Logger::instance()->log(QtWarningMsg,
    //                            QString(tr("Utils::archive(const QString & filePath, const QString & extDirPath, const QString & "
    //                                       "singleFileName) - ") +
    //                                    tr("%1 entries\n")).arg(zip.getEntriesCount()));

    //    Logger::instance()->log(QtWarningMsg,
    //                            QString(tr("Utils::archive(const QString & filePath, const QString & extDirPath, const QString & "
    //                                       "singleFileName) - ") +
    //                                    tr("Global comment: %1\n")).arg(zip.getComment().toLocal8Bit().constData()));

    //    // Zip file inside an archive file.
    //    QuaZipFile zipFileInsideZip(&zip);

    //    // Info of a zip file inside an archive file.
    //    QuaZipFileInfo zipFileInfo;

    //    // File extracted from an archive file.
    //    QFile extractedFile;

    //    // Extracted filename.
    //    QString extractedFilename;

    //    char c;

    //    for (bool more = zip.goToFirstFile(); more; more = zip.goToNextFile())
    //    {
    //        if ( ! zip.getCurrentFileInfo(&zipFileInfo) )
    //        {
    //            Logger::instance()->log(QtWarningMsg,
    //                                    QString(tr("Utils::archive(const QString & filePath, const QString & extDirPath, const QString & "
    //                                               "singleFileName) - ") +
    //                                            tr("testRead(): getCurrentFileInfo(): %1\n")).arg(zip.getZipError()));
    //            return (false);
    //        }

    //       // if ( ! singleFileName.isEmpty() )
    //       // {
    //       //     if ( ! zipFileInfo.name.contains(singleFileName) )
    //       //     {
    //       //         continue;
    //       //     }
    //       // }

    //        // If singleFileName is not empty, then only one file must be extracted.
    //        if ( ( ! singleFileName.isEmpty() ) &&
    //             ( ! zipFileInfo.name.contains(singleFileName) ) )
    //        {
    //            continue;
    //        }

    //        if ( ! zipFileInsideZip.open(QIODevice::ReadOnly) )
    //        {
    //            Logger::instance()->log(QtWarningMsg,
    //                                    QString(tr("Utils::archive(const QString & filePath, const QString & extDirPath, const QString & "
    //                                               "singleFileName) - ") +
    //                                            tr("testRead(): file.open(): %1")).arg(zip.getZipError()));

    //            return (false);
    //        }

    //        extractedFilename = QString("%1/%2").arg(extDirPath, zipFileInsideZip.getActualFileName());

    //        if (zipFileInsideZip.getZipError() != UNZ_OK)
    //        {
    //            Logger::instance()->log(QtWarningMsg,
    //                                    QString(tr("Utils::archive(const QString & filePath, const QString & extDirPath, const QString & "
    //                                               "singleFileName) - ") +
    //                                            tr("testRead(): file.fileName(): %1")).arg(zip.getZipError()));

    //            return (false);
    //        }

    // //       extractedFile.setFileName("extractedFile/" + extractedFilename);
    //        extractedFile.setFileName(extractedFilename);

    //        // This will fail if "extractedFilename" contains subdirectories, but we don't mind that.
    //        extractedFile.open(QIODevice::WriteOnly);

    //        // Slow like hell (on GNU/Linux at least), but it is not my fault.
    //        // Not ZIP/UNZIP package's fault either.
    //        // The slowest thing here is extractedFile.putChar(c).
    //        while (zipFileInsideZip.getChar(&c))
    //        {
    //            extractedFile.putChar(c);
    //        }

    //        extractedFile.close();

    //        if (zipFileInsideZip.getZipError() != UNZ_OK)
    //        {
    //            Logger::instance()->log(QtWarningMsg,
    //                                    QString(tr("Utils::archive(const QString & filePath, const QString & extDirPath, const QString & "
    //                                               "singleFileName) - ") +
    //                                            tr("testRead(): file.fileName(): %1")).arg(zip.getZipError()));

    //            return (false);
    //        }

    //        // If a zip file was not opened properly, or an archive was not opened...
    //        if ( ! zipFileInsideZip.atEnd() )
    //        {
    //            Logger::instance()->log(QtWarningMsg,
    //                                    QString(tr("Utils::archive(const QString & filePath, const QString & extDirPath, const QString & "
    //                                               "singleFileName) - ") +
    //                                            tr("testRead(): read all but not EOF")));

    //            return (false);
    //        }

    //        zipFileInsideZip.close();

    //        if (zipFileInsideZip.getZipError() != UNZ_OK)
    //        {
    //            Logger::instance()->log(QtWarningMsg,
    //                                    QString(tr("Utils::archive(const QString & filePath, const QString & extDirPath, const QString & "
    //                                               "singleFileName) - ") +
    //                                            tr("testRead(): file.close(): %1")).arg(zip.getZipError()));

    //            return (false);
    //        }
    //    }

    //    // Close an archive file.
    //    zip.close();

    //    if (zip.getZipError() != UNZ_OK)
    //    {
    //        Logger::instance()->log(QtWarningMsg,
    //                                QString(tr("Utils::archive(const QString & filePath, const QString & extDirPath, const QString & "
    //                                           "singleFileName) - ") +
    //                                        tr("testRead(): zip.close(): %1")).arg(zip.getZipError()));

    //        return (false);
    //    }

    //    return (true);
    // }

    // // From https://stackoverflow.com/questions/2598117/zipping-a-folder-file-using-qt
    // void Utils::recursiveAddDir(QDir dir, QStringList & filesList)
    // {
    //    QStringList dirEntryList = dir.entryList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);

    //    foreach (QString file, dirEntryList)
    //    {
    //        QFileInfo finfo(QString("%1/%2").arg(dir.path(), file));

    //        if (finfo.isSymLink())
    //        {
    //            Logger::instance()->log(QtDebugMsg,
    //                                    QString(tr("recursiveAddDir(QDir dir, QStringList & list) - ") +
    //                                            tr("File/Dir is symboic link!")));
    //            return;
    //        }

    //        if (finfo.isDir())
    //        {
    // //                                QString dirname = finfo.fileName();
    //            QDir subDir(finfo.filePath());

    //            Logger::instance()->log(QtDebugMsg,
    //                                    QString(tr("recursiveAddDir(QDir dir, QStringList & list) - ") +
    //                                            tr("Recursive add!")));

    //            recursiveAddDir(subDir, filesList);

    //        } else
    //            filesList << QDir::toNativeSeparators(finfo.filePath());
    //    }
    // }

} // utils
