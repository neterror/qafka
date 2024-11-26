#pragma once
#include <QObject>
#include "kafka_client.h"
#include <librdkafka/rdkafkacpp.h>
#include <librdkafka/rdkafka.h>
#include "qafka_export.h"

class QAFKA_EXPORT Consumer : public KafkaClient,
                 public RdKafka::RebalanceCb,
                 public RdKafka::EventCb {
    Q_OBJECT
    void rebalance_cb(RdKafka::KafkaConsumer *consumer,
                      RdKafka::ErrorCode err,
                      std::vector<RdKafka::TopicPartition *> &partitions) override;

    void event_cb(RdKafka::Event &event) override;
    bool mRunning;
    RdKafka::KafkaConsumer* mConsumer;
    QStringList mTopics;
    QElapsedTimer mTimer;
    QMap<QString, int> mOffsets;
    QMap<QByteArray, int64_t> mLastMsgTime;
    QMap<QByteArray, int64_t> mDelay;
    

    //When replaying history records, emit the messages with the time delay between them as recorded in the timestamp of the event
    bool mReplayWithTimeDelays {false};

    void setTopicOffsets(const std::vector<RdKafka::TopicPartition *>& partitions);
    void processMessage(RdKafka::Message* message);
public:
    Consumer(QStringList topics, const QMap<QString, int>& offsets = {} , QObject* parent = nullptr);
    ~Consumer();

    void stop() { mRunning = false; }
    std::optional<QString> initialize();

    void replayWithTimeDelays(bool enable);
public slots:
    void work() override;

signals:
    void message(RdKafka::MessageTimestamp timestamp, quint64 offset, QByteArray topic, QByteArray key, QByteArray data);
    void initialized();
};
