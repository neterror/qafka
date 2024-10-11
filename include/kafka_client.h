#pragma once

#include <QtCore>
#include <librdkafka/rdkafkacpp.h>

class KafkaClient : public QObject {
    Q_OBJECT
public:
    KafkaClient(QObject* parent = nullptr) : QObject(parent) {}
protected:
    std::unique_ptr<RdKafka::Conf> getConfig(const QString& section);
signals:
    void finished();
    void error(QString message);

public slots:
    virtual void work() = 0;
    
};
