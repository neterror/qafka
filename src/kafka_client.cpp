#include "kafka_client.h"


std::unique_ptr<RdKafka::Conf> KafkaClient::getConfig(const QString& section) {
    QSettings settings;
    settings.beginGroup(section);
    std::string error;
    std::unique_ptr<RdKafka::Conf> config(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
    for(auto key: settings.allKeys()) {
        auto value = settings.value(key).toString();
        if (config->set(key.toStdString(), value.toStdString(), error) != RdKafka::Conf::CONF_OK) {
            qWarning() << "Failed to set config option" << key << error;
        } else {
            qDebug() <<  "Set config option" << key << "=" << value;
        }
    }
    settings.endGroup();
    return config;
}
