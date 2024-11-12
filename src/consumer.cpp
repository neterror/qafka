#include "consumer.h"
#include <QtCore>
#include <librdkafka/rdkafkacpp.h>
#include <iostream>
#include <qcoreapplication.h>
#include <string>
#include "config_options.h"

Consumer::Consumer(QStringList topics, const QMap<QString, int>& offsets, QObject* parent) :
    KafkaClient(parent), mTopics{topics}, mOffsets{offsets} {
    mTimer.start();
}


Consumer::~Consumer() {
    mConsumer->close();
    delete mConsumer;

    /*
     * Wait for RdKafka to decommission.
     * This is not strictly needed (with check outq_len() above), but
     * allows RdKafka to clean up all its resources before the application
     * exits so that memory profilers such as valgrind wont complain about
     * memory leaks.
     */
    RdKafka::wait_destroyed(500);
    qDebug() << "Kafka consumer destroyed";
}


std::optional<QString> Consumer::initialize() {
    std::string error;
    auto config = getConfig("Consumer");
    //    if (config->set("event_cb",(RdKafka::EventCb*)this, error) != RdKafka::Conf::CONF_OK) {
    //        return error.c_str();
    //    }

    if (config->set("rebalance_cb",(RdKafka::RebalanceCb*)this, error) != RdKafka::Conf::CONF_OK) {
        return error.c_str();
    }

    mConsumer = RdKafka::KafkaConsumer::create(config.get(), error);;
    if (!mConsumer) {
        return error.c_str();
    }
    qDebug() << "created consumer" << mConsumer->name().c_str();

    std::vector<std::string> topics;
    for(auto t: mTopics) {
        topics.push_back(t.toStdString());
        qDebug().noquote() << "subscribe to " << t;
    }
    
    RdKafka::ErrorCode err = mConsumer->subscribe(topics);
    if (err) {
        QString msg = QString("Failed to subscribe: %1").arg(RdKafka::err2str(err).c_str());
        return msg;
    }

    return std::nullopt;
}

void Consumer::setTopicOffsets(const std::vector<RdKafka::TopicPartition *>& partitions) {
    for (auto& partition : partitions) {
        auto topic = QString(partition->topic().c_str());
        if (mOffsets.contains(topic)) {
            qDebug() << "offset to topic" << topic << "is " << mOffsets.value(topic);
            partition->set_offset(mOffsets.value(topic));  // Latest messages
        } else {
            qDebug() << "offset to topic" << topic << "is RD_KAFKA_OFFSET_END";
            partition->set_offset(RD_KAFKA_OFFSET_END);  // Latest messages
        }
    }
    mOffsets.clear();
}


void Consumer::work() {
    if (auto error = initialize(); error) {
        qWarning() << "consumer initialization failed" << error;
        emit finished();
        return;
    }

    emit initialized();
    mRunning = true;
    qDebug() << "receiver started work at " << mTimer.elapsed();
    while (mRunning) {
        //todo. check what is RdKafka::ERR__PARTITION_EOF and how to deal with it
        auto data = mConsumer->consume(1000);

        if (data->err() == RdKafka::ERR_NO_ERROR) {
            QByteArray payload((char*)data->payload(), data->len());
            QByteArray topic = data->topic_name().c_str();
            QByteArray key;
            if (data->key()) {
                key = data->key()->c_str();
            }
            emit message(data->timestamp(), data->offset(), topic, key, payload);
        } else {
            if (data->err() != RdKafka::ERR__TIMED_OUT) {
                qWarning() << "incoming data error: " << RdKafka::err2str(data->err()).c_str();
            }
        }
        delete data;
        QCoreApplication::processEvents( QEventLoop::AllEvents, 10);
    }

    emit finished();
}


static void part_list_print(
                              const std::vector<RdKafka::TopicPartition *> &partitions) {
      for (unsigned int i = 0; i < partitions.size(); i++)
          std::cerr << partitions[i]->topic() << "[" << partitions[i]->partition()
                    << "], ";
      std::cerr << std::endl;
}
  
void Consumer::rebalance_cb(RdKafka::KafkaConsumer *consumer, RdKafka::ErrorCode err,std::vector<RdKafka::TopicPartition *> &partitions) {
    qDebug() << "rebalance event at " << mTimer.elapsed()
             << ",protocol: " << consumer->rebalance_protocol();
    
    part_list_print(partitions);

    RdKafka::Error *error      = NULL;
    RdKafka::ErrorCode ret_err = RdKafka::ERR_NO_ERROR;

    if (!mOffsets.isEmpty()) {
        setTopicOffsets(partitions);
    }
    
    if (err == RdKafka::ERR__ASSIGN_PARTITIONS) {
        if (consumer->rebalance_protocol() == "COOPERATIVE")
            error = consumer->incremental_assign(partitions);
        else 
            ret_err = consumer->assign(partitions);

    } else if (err == RdKafka::ERR__REVOKE_PARTITIONS) {
        if (consumer->rebalance_protocol() == "COOPERATIVE") {
            error = consumer->incremental_unassign(partitions);
        } else {
            ret_err       = consumer->unassign();
        }
    }

    if (error) {
        std::cerr << "incremental assign failed: " << error->str() << "\n";
        delete error;
    } else if (ret_err)
        std::cerr << "assign failed: " << RdKafka::err2str(ret_err) << "\n";
}



void Consumer::event_cb(RdKafka::Event &event) {
    switch (event.type()) {
    case RdKafka::Event::EVENT_ERROR:
        if (event.fatal()) {
            std::cerr << "FATAL ";
        }
        std::cerr << "ERROR (" << RdKafka::err2str(event.err())
                  << "): " << event.str() << std::endl;
        break;

    case RdKafka::Event::EVENT_STATS:
        std::cerr<< "\"STATS\": " << event.str() << std::endl;
        break;

    case RdKafka::Event::EVENT_LOG:
        fprintf(stderr, "LOG-%i-%s: %s\n", event.severity(), event.fac().c_str(),
                event.str().c_str());
        break;

    case RdKafka::Event::EVENT_THROTTLE:
        std::cerr << "THROTTLED: " << event.throttle_time() << "ms by "
                  << event.broker_name() << " id " << (int)event.broker_id()
                  << std::endl;
        break;

    default:
        std::cerr << "EVENT " << event.type() << " ("
                  << RdKafka::err2str(event.err()) << "): " << event.str()
                  << std::endl;
        break;
    }
}
