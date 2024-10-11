#pragma once


struct QafkaEvent : public QEvent{
    enum Type{
        Send = QEvent::User + 1,
        Recv = QEvent::User + 2
    };
    QafkaEvent(Type type) : QEvent((QEvent::Type)type) {}
};

struct ProducerSendEvent : public QafkaEvent {
    ProducerSendEvent(Type type, QString t, QByteArray k, QByteArray m):
        QafkaEvent(type),
        topic{t},
        key{k},
        message{m}
    {
    }
    QString topic;
    QByteArray key;
    QByteArray message;
};

struct ConsmerEvent : public QafkaEvent {
    ConsmerEvent(Type type, QString t, QByteArray m):
        QafkaEvent(type),
        topic{t},
        message{m}
    {
    }
    QString topic;
    QByteArray message;
};
