#pragma once
#include <QObject>
#include "kafka_client.h"
#include <librdkafka/rdkafkacpp.h>
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
public:
    explicit Consumer(QStringList topics, QObject* parent = nullptr);
    ~Consumer();

    void stop() { mRunning = false; }
    std::optional<QString> initialize();    

public slots:
    void work() override;

signals:
    void message(QByteArray topic, QByteArray data);
};
