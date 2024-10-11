#pragma once
#include <QObject>
#include <QThread>
#include "kafka_client.h"
#include <librdkafka/rdkafkacpp.h>
#include <optional>


class Producer : public KafkaClient, public RdKafka::DeliveryReportCb {
    Q_OBJECT
    std::unique_ptr<RdKafka::Producer> mProducer;

    //delivery report callback
    void dr_cb(RdKafka::Message &message) override;

    bool mRunning;
    void internalSend(const QString& topic, const QByteArray& key, const QByteArray& message);
    std::optional<QString> initialize();

public:
    explicit Producer(QObject* parent = nullptr);
    ~Producer();
    static std::unique_ptr<Producer> create(QThread& thread);
    void stop() {mRunning = false;}
    bool event(QEvent *event) override;
    void send(const QString& topic, const QByteArray& key, const QByteArray& message);

    
public slots:
    void work() override;
};
