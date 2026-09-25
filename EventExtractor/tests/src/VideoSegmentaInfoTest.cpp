#include "tests/inc/VideoSegmentaInfoTest.h"

VideoSegmentaInfoTest::VideoSegmentaInfoTest()
{
    //
}

//#include <QCoreApplication>
//#include <QDebug>
//#include <algorithm>

//int main(int argc, char *argv[]) {
//    QCoreApplication a(argc, argv);
//    QString testFile = "segments_test.json";

//    // --- Scenario 1: Initial Save ---
//    VideoSegmentInfo s1; s1.segment_id = 1; s1.start_time = 10.5; s1.state = "Active";
//    VideoSegmentInfo s2; s2.segment_id = 2; s2.start_time = 0.0;  s2.state = "Idle";

//    QList<VideoSegmentInfo> initialList = {s1, s2};
//    qDebug() << "Saving initial list (IDs 1 and 2)...";
//    saveSegmentsListJsonAdvanced(initialList, testFile);

//    // --- Scenario 2: Merge and Update ---
//    // We create a new version of ID 1 (Update) and a brand new ID 3 (Append)
//    VideoSegmentInfo s1_updated;
//    s1_updated.segment_id = 1;
//    s1_updated.start_time = 10.5;
//    s1_updated.state = "Updated State";

//    VideoSegmentInfo s3;
//    s3.segment_id = 3;
//    s3.start_time = 5.0;
//    s3.state = "New Segment";

//    QList<VideoSegmentInfo> updates = {s1_updated, s3};
//    qDebug() << "Merging updates (Updating ID 1, Adding ID 3)...";
//    saveSegmentsListJsonAdvanced(updates, testFile);

//    // --- Scenario 3: Verify Results ---
//    QList<VideoSegmentInfo> result = loadSegmentsListJson(testFile);

//    qDebug() << "\nFinal Sorted Results:";
//    for (const auto &seg : result) {
//        qDebug() << "ID:" << seg.segment_id
//                 << "| Start:" << seg.start_time
//                 << "| State:" << seg.state;
//    }

//    return 0;
//}
