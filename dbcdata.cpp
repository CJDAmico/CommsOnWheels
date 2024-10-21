#include "dbcdata.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonValue>
#include <QDebug>

DbcDataModel::DbcDataModel() {
    // Constructor
}

void DbcDataModel::setFileName(const QString& name)
{
    m_fileName = name;
}

QString DbcDataModel::fileName() const
{
    return m_fileName;
}

bool DbcDataModel::loadFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Couldn't open JSON file:" << filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc(QJsonDocument::fromJson(data, &parseError));
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON document in file:" << filePath << ":" << parseError.errorString();
        return false;
    }

    parseJson(doc.object());
    return true;
}

void DbcDataModel::parseJson(const QJsonObject& jsonObject) {
    // Clear existing data
    m_buses.clear();
    m_messages.clear();
    m_nodes.clear();

    // Parse buses
    QJsonArray busesArray = jsonObject.value("buses").toArray();
    for (const QJsonValue& value : busesArray) {
        QJsonObject busObject = value.toObject();
        Bus bus;
        bus.name = busObject.value("name").toString();
        bus.baud = busObject.value("baud").toString();
        m_buses.append(bus);
    }

    // Parse messages
    QJsonArray messagesArray = jsonObject.value("messages").toArray();
    for (const QJsonValue& value : messagesArray) {
        QJsonObject messageObject = value.toObject();
        Message message;

        message.pgn = messageObject.value("pgn").toVariant().toULongLong();
        QString messageName = messageObject.value("name").toString();
        if(messageName.endsWith(":")) {
            messageName.chop(1);
        }
        message.name = messageName;
        message.description = messageObject.value("description").toString();
        message.priority = messageObject.value("priority").toInt();
        message.length = messageObject.value("length").toInt();
        message.txPeriodicity = messageObject.value("tx_periodicity").toInt(0);
        message.txOnChange = messageObject.value("tx_onChange").toBool(false);

        // Parse signals (data)
        QJsonArray dataArray = messageObject.value("data").toArray();
        for (const QJsonValue& dataValue : dataArray) {
            QJsonObject signalObject = dataValue.toObject();
            Signal signal;
            signal.spn = signalObject.value("spn").toInt(0);
            signal.name = signalObject.value("name").toString();
            signal.description = signalObject.value("description").toString();
            signal.startBit = signalObject.value("start_bit").toInt();
            signal.bitLength = signalObject.value("bit_length").toInt();
            signal.isBigEndian = signalObject.value("is_bigEndian").toBool(false);
            signal.isTwosComplement = signalObject.value("is_twosComplement").toBool(false);
            signal.factor = signalObject.value("factor").toDouble(1.0);
            signal.offset = signalObject.value("offset").toDouble(0.0);
            signal.units = signalObject.value("units").toString();

            // Handle scaled_min, scaled_max, scaled_default
            signal.scaledMin = signalObject.value("scaled_min").toVariant();
            signal.scaledMax = signalObject.value("scaled_max").toVariant();
            signal.scaledDefault = signalObject.value("scaled_default").toVariant();

            // If scaled_default is null, default to scaled_min
            if (signal.scaledDefault.isNull()) {
                signal.scaledDefault = signal.scaledMin;
            }

            // Parse enumerations
            QJsonArray enumArray = signalObject.value("enumerations").toArray();
            for (const QJsonValue& enumValue : enumArray) {
                QJsonObject enumObject = enumValue.toObject();
                Enumeration enumeration;
                enumeration.name = enumObject.value("name").toString();
                enumeration.description = enumObject.value("description").toString();
                enumeration.value = enumObject.value("value").toInt();
                signal.enumerations.append(enumeration);
            }

            message.data.append(signal);
        }

        m_messages.append(message);
    }

    // Parse nodes
    QJsonArray nodesArray = jsonObject.value("nodes").toArray();
    for (const QJsonValue& value : nodesArray) {
        QJsonObject nodeObject = value.toObject();
        Node node;
        node.name = nodeObject.value("name").toString();

        // Parse node buses
        QJsonArray nodeBusesArray = nodeObject.value("buses").toArray();
        for (const QJsonValue& busValue : nodeBusesArray) {
            QJsonObject busObject = busValue.toObject();
            NodeBus nodeBus;
            nodeBus.name = busObject.value("name").toString();
            nodeBus.sourceAddress = busObject.value("source_address").toInt();

            // Parse tx messages
            QJsonArray txArray = busObject.value("tx").toArray();
            for (const QJsonValue& txValue : txArray) {
                QJsonObject txObject = txValue.toObject();
                TxRxMessage txMessage;
                QString txMessageName = txObject.value("name").toString();
                if(txMessageName.endsWith(":")) {
                    txMessageName.chop(1);
                }
                txMessage.name = txMessageName;
                nodeBus.tx.append(txMessage);
            }

            // Parse rx messages
            QJsonArray rxArray = busObject.value("rx").toArray();
            for (const QJsonValue& rxValue : rxArray) {
                QJsonObject rxObject = rxValue.toObject();
                TxRxMessage rxMessage;
                QString rxMessageName = rxObject.value("name").toString();
                if(rxMessageName.endsWith(":")) {
                    rxMessageName.chop(1);
                }
                rxMessage.name = rxMessageName;
                nodeBus.rx.append(rxMessage);
            }

            node.buses.append(nodeBus);
        }

        m_nodes.append(node);
    }
}

void DbcDataModel::merge(const DbcDataModel& other) {
    // Merge buses
    for (const Bus& bus : other.m_buses) {
        bool exists = std::any_of(m_buses.begin(), m_buses.end(), [&bus](const Bus& b) {
            return b.name == bus.name;
        });
        if (!exists) {
            m_buses.append(bus);
        }
    }

    // Merge messages
    for (const Message& message : other.m_messages) {
        bool exists = std::any_of(m_messages.begin(), m_messages.end(), [&message](const Message& m) {
            return m.name == message.name;
        });
        if (!exists) {
            m_messages.append(message);
        }
    }

    // Merge nodes
    for (const Node& node : other.m_nodes) {
        bool exists = std::any_of(m_nodes.begin(), m_nodes.end(), [&node](const Node& n) {
            return n.name == node.name;
        });
        if (!exists) {
            m_nodes.append(node);
        }
    }
}

QList<Bus> DbcDataModel::buses() const {
    return m_buses;
}

QList<Message> DbcDataModel::messages() const {
    return m_messages;
}

QList<Node> DbcDataModel::nodes() const {
    return m_nodes;
}
