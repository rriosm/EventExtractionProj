
#include <QDir>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QProcess>

#include "inc/VideoSegmentationWorkerFactory.h"
#include "inc/ffmpeg_gen.h"
#include "inc/VideoSegmentationWorker.h"
#include "inc/VideoSegmentationWorkerCLI.h"


IVideoSegmentationWorker* VideoSegmentationWorkerFactory::createWorker(SegmentationMode_t mode, QObject *parent)
{
    if (mode == SegmentationMode_t::UseLibrary)
    {
        return new VideoSegmentationWorker(parent);
    } else {
        return new VideoSegmentationWorkerCLI(parent);
    }
}

bool VideoSegmentationWorkerFactory::verifyEnvironment(SegmentationMode_t mode,
                                                       const QString &outDir,
                                                       const QString &inputVideo,
                                                       QString &errorMessage)
{
    // 1. Check Directory and Write Permissions first (Common to both modes)
    if (outDir.isEmpty())
    {
        errorMessage = "Output directory path is empty.";
        return false;
    }

    QDir dir(outDir);
    // Try to create the directory if it doesn't exist
    if ( ! dir.exists() )
    {
        if ( ! dir.mkpath(".") )
        {
            errorMessage = QString("Failed to create output directory: %1").arg(outDir);
            return false;
        }
    }

    // Verify Write Permissions
    // Check if we can create and remove a temporary dummy file
    QString testFileName = outDir + "/.permission_test";
    QFile testFile(testFileName);

    if ( testFile.open(QIODevice::WriteOnly) )
    {
        testFile.close();
        testFile.remove();

    } else {
        errorMessage = QString("No write permissions in directory: %1").arg(outDir);
        return false;
    }

    // 3. Available Disk Space Check
    QStorageInfo storage(outDir);

    if (storage.isValid() && storage.isReady())
    {
        qint64 inputSize = QFileInfo(inputVideo).size();
        qint64 requiredSpace = static_cast<qint64>(inputSize * 2); // Safety margin: 2x input size
        qint64 availableSpace = storage.bytesAvailable();

        if (availableSpace < requiredSpace)
        {
            errorMessage = QString("Insufficient disk space on %1. Required: %2 MB, Available: %3 MB")
                               .arg(storage.rootPath())
                               .arg(requiredSpace / 1024 / 1024)
                               .arg(availableSpace / 1024 / 1024);
            return false;
        }
    }

    // 4. Mode-Specific FFmpeg Checks
    switch (mode)
    {
        case SegmentationMode_t::UseCLI:
        {
            // 1. Check if 'ffmpeg' executable is in the system PATH
            QString ffmpegPath = QStandardPaths::findExecutable("ffmpeg");

            if (ffmpegPath.isEmpty())
            {
                errorMessage = "FFmpeg executable not found in system PATH. Please install FFmpeg.";
                return false;
            }

            // 2. Check if 'ffprobe' is available (needed for metadata/gaps)
            if (QStandardPaths::findExecutable("ffprobe").isEmpty())
            {
                errorMessage = "FFprobe executable not found. Metadata collection will fail.";
                return false;
            }

            // 3. Optional: Quick version check to ensure it's runnable
            QProcess check;
            check.start(ffmpegPath, {"-version"});

            if ( ! check.waitForFinished(2000) )
            {
                errorMessage = "FFmpeg was found but failed to execute (Permissions or Architecture mismatch).";
                return false;
            }

            break;
        }

        case SegmentationMode_t::UseLibrary:
        {
            // 1. Verify that the linked headers match the loaded shared libraries
            // This prevents crashes caused by 'DLL Hell' or mismatched versions
            if (avformat_version() < 0)
            {
                errorMessage = "FFmpeg libraries failed to initialize or are not loaded.";
                return false;
            }

            // 2. Check for minimum required features (e.g., H.264 support)
            if ( ! avcodec_find_encoder(AV_CODEC_ID_H264) )
            {
                errorMessage = "FFmpeg libraries loaded, but H.264 encoder is missing (Licensing or Build issue).";
                return false;
            }

            // 2. Hardware Acceleration Check
            bool hasHW = false;
#ifdef Q_OS_MAC
            if (avcodec_find_encoder_by_name("h264_videotoolbox")) {
                hasHW = true;
            } else {
                errorMessage = "Hardware acceleration (VideoToolbox) not found in shared libraries.";
            }
#elif defined(Q_OS_WIN) || defined(Q_OS_LINUX)
            if (avcodec_find_encoder_by_name("h264_nvenc")) {
                hasHW = true;
            } else {
                errorMessage = "Hardware acceleration (NVENC) not found in shared libraries.";
            }
#endif

            if ( ! hasHW )
            {
                if (avcodec_find_encoder_by_name("libx264")) {
                    hasHW = true;
                } else {
                    errorMessage = "Neither Hardware (VideoToolbox/NVENC) nor Software (libx264) encoders were found.";
                }
            }

            // If hardware is missing, we return false to trigger a fallback
            if ( ! hasHW ) return false;

            break;
        }

        case SegmentationMode_t::None:
        default:
        {
            // TODO
        }
    }

    return true;
}
