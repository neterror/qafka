#include "producer.h"
#include <exception>
#include <QtCore>
#include <exception>
#include <qcoreapplication.h>
#include <stdexcept>
#include <string>
#include "kafka_client.h"
#include "qafka_events.h"
#include "config_options.h"


Producer::Producer(QObject *parent) : KafkaClient(parent) {
}

Producer::~Producer() {
    qDebug() << "producer is destroyed";
}

std::optional<QString> Producer::initialize() {
    auto config = getConfig("Producer");

    std::string error;
    if (config->set(kDeliveryReport, this, error) != RdKafka::Conf::CONF_OK) {
        return error.c_str();
    }

    mProducer.reset(RdKafka::Producer::create(config.get(), error));
    if (!mProducer) {
        return error.c_str();
    }
    return std::nullopt;
}


// delivery report callback
// strange method name - implements RdKafka::DeliveryReportCb
void Producer::dr_cb(RdKafka::Message &message) {
    if (message.err()) {
        auto errorStr = message.errstr();
        auto msg = QString("Message delivery failed: %1").arg(errorStr.c_str());
        emit error(msg);
    }
}


void Producer::work() {
    if (auto error = initialize(); error) {
        qWarning() << "producer initialization failed" << error;
        emit finished();
        return;
    }

    mRunning = true;
    while(mRunning) {
        mProducer->poll(10);
        QCoreApplication::processEvents( QEventLoop::AllEvents, 10);
    }

    mProducer->flush(1000);
    emit finished();
}


void Producer::send(const QString& topic, const QByteArray& key, const QByteArray& message)
{
    QCoreApplication::postEvent(this, new ProducerSendEvent(QafkaEvent::Send, topic, key, message));
}


void Producer::internalSend(const QString& topic, const QByteArray& key, const QByteArray& message)
{
    //PARTITION_UA -> unassigned partition
    //for messages that should be partitioned using the configured or default partitioner.
    auto result = mProducer->produce(
        topic.toStdString(),                // topic
        RdKafka::Topic::PARTITION_UA,       // partition number. -1 for default
        RdKafka::Producer::RK_MSG_COPY,     // msgflags
        const_cast<char *>(message.data()), // payload
        message.length(),                   // payload length
        key.data(),                         // void* key
        key.size(),
        0,                                  // timestamp. the default is for current time
        nullptr,                            // headers
        nullptr                             //msg_opaque
    );

    if (result != RdKafka::ERR_NO_ERROR) {
        auto errorText = RdKafka::err2str(result);
        auto message = QString("Failed to produce to topic %1: %2").arg(topic).arg(errorText.c_str());
        emit error(message);
    }
}


bool Producer::event(QEvent *event) {
    if (event->type() <= QEvent::User) {
        return QObject::event(event);
    }

    auto type = (QafkaEvent::Type)event->type();
    if (type == QafkaEvent::Send) {
        auto s = (ProducerSendEvent*)event;
        internalSend(s->topic, s->key, s->message);
    }
    return true;
}
