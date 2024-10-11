#include <QtCore>
#include <memory>
#include <qcoreapplication.h>
#include <stdexcept>
#include "kafka_client.h"
#include "producer.h"
#include "consumer.h"
#include "thread.h"
#include <signal.h>
#include <iostream>

static void finishedThread(QString name) {
    qDebug() << "Kafka thread" << name << "finished. Terminating the application";
    QCoreApplication::quit();
}

static Consumer* consumer = nullptr;

static void sigterm(int sig) {
    std::cout << "signal caught: " << sig << std::endl;
    if (consumer) {
        consumer->stop();
    }
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName("abrites");
    QCoreApplication::setApplicationName("qafka");

    auto producer = Thread::spawn<Producer>(); //spawns a separate thread
    consumer = Thread::spawn<Consumer>(QStringList{"salute"});

    QObject::connect(producer, &KafkaClient::finished, [] {finishedThread("producer");});
    QObject::connect(consumer, &KafkaClient::finished, [] {finishedThread("receiver");});
    QObject::connect(consumer, &Consumer::message, [](QByteArray topic, QByteArray payload) {
        qDebug() << "incoming message from topic" << topic << payload;
    });


    QTimer timer;
    timer.setSingleShot(false);
    timer.setInterval(1000);

    QObject::connect(&timer, &QTimer::timeout, [producer, &timer, counter=0]() mutable {
        producer->send("salute", "key1", "hello");
        if (counter++ > 5) {
            //            producer->stop();
            timer.stop();
        }
    });

    signal(SIGINT, sigterm);
    signal(SIGTERM, sigterm);

    timer.start();
    qDebug() << "main working in thread " << QThread::currentThreadId();
    return app.exec();
}
