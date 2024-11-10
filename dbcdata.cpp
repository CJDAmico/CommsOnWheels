#include "dbcdata.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonValue>
#include <QDebug>
#include <qfileinfo.h>
#include <QTextStream>
#include <QRegularExpression>

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


bool DbcDataModel::importDBC(const QString& filePath) {
    // TODO: Implement parsing a DBC file

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Couldn't open DBC file:" << filePath;
        return false;
    }

    QTextStream in(&file);
    QString line;
    QRegularExpression reNode("^BU_:\\s*(.*)");
    QRegularExpression reMessage("^BO_\\s+(\\d+)\\s+(\\w+):\\s+(\\d+)\\s+(\\w+)");
    QRegularExpression reSignal("^\\s+SG_\\s+(\\w+)\\s*:\\s*(\\d+)\\|(\\d+)@(\\d+)([+-])\\s+\\(([^,]+),([^\\)]+)\\)\\s+\\[([^\\|]+)\\|([^\\]]+)\\]\\s+\"([^\"]*)\"\\s+(\\w+)");

    // Set  network name to file name
    Network network;
    network.name = QFileInfo(filePath).fileName();
    m_networks.append(network);

    while (!in.atEnd()) {
        line = in.readLine();

        // Parse nodes
        QRegularExpressionMatch match = reNode.match(line);
        if (match.hasMatch()) {
            QStringList nodes = match.captured(1).split(' ', Qt::SkipEmptyParts);
            for (const QString& nodeName : nodes) {
                Node node;
                node.name = nodeName.trimmed();
                m_nodes.append(node);
            }
            continue;
        }

        // Parse messages
        match = reMessage.match(line);
        if (match.hasMatch()) {
            Message message;
            message.pgn = match.captured(1).toULongLong();
            message.name = match.captured(2).trimmed();
            message.length = match.captured(3).toInt();
            QString transmitter = match.captured(4).trimmed();
            message.messageTransmitters.append({transmitter, QString::number(message.pgn)});
            m_messages.append(message);
            continue;
        }

        // Parse signals
        match = reSignal.match(line);
        if (match.hasMatch()) {
            Signal signal;
            signal.name = match.captured(1).trimmed();
            signal.startBit = match.captured(2).toInt();
            signal.bitLength = match.captured(3).toInt();
            signal.isBigEndian = (match.captured(4).toInt() == 1);
            signal.isTwosComplement = (match.captured(5) == "-");
            signal.factor = match.captured(6).toDouble();
            signal.offset = match.captured(7).toDouble();
            signal.scaledMin = match.captured(8).toDouble();
            signal.scaledMax = match.captured(9).toDouble();
            signal.units = match.captured(10).trimmed();
            QString receiver = match.captured(11).trimmed();
            m_messages.last().messageSignals.append(signal);
            m_nodes.append({receiver, {}, {}});
            continue;
        }
    }

    file.close();
    return true;
}

bool DbcDataModel::loadJson(const QString& filePath) {
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
    m_networks.clear();
    m_messages.clear();
    m_nodes.clear();

    // Parse Networks ("buses" on JSON)
    QJsonArray busesArray = jsonObject.value("buses").toArray();
    for (const QJsonValue& value : busesArray) {
        QJsonObject busObject = value.toObject();
        Network network;
        network.name = busObject.value("name").toString();
        network.baud = busObject.value("baud").toString();
        m_networks.append(network);
    }

    // Sort m_networks alphabetically by name
    std::sort(m_networks.begin(), m_networks.end(), [](const Network& a, const Network& b) {
        return a.name.toLower() < b.name.toLower();
    });

    // Parse Messages
    QJsonArray messagesArray = jsonObject.value("messages").toArray();
    for (const QJsonValue& value : messagesArray) {
        QJsonObject messageObject = value.toObject();
        Message message;
        message.pgn = messageObject.value("pgn").toVariant().toULongLong();
        message.name = messageObject.value("name").toString();
        message.description = messageObject.value("description").toString();
        message.priority = messageObject.value("priority").toInt();
        message.length = messageObject.value("length").toInt();
        message.txPeriodicity = messageObject.value("tx_periodicity").toInt(0);
        message.txOnChange = messageObject.value("tx_onChange").toBool(false);
        message.multiplexValue = -1; // Always defaults to -1

        // Parse signals
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
            signal.multiplexValue = signalObject.value("multiplexValue").toInt(-1);
            signal.isMultiplexor = signalObject.value("is_multiplexor").toBool(false);

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

            message.messageSignals.append(signal);
        }

        // Sort message.messageSignals alphabetically by name
        std::sort(message.messageSignals.begin(), message.messageSignals.end(), [](Signal& a, Signal& b) {
            return a.name.toLower() < b.name.toLower();
        });

        m_messages.append(message);
    }

    // Sort m_messages alphabetically by name
    std::sort(m_messages.begin(), m_messages.end(), [](Message& a, Message& b) {
        return a.name.toLower() < b.name.toLower();
    });

    // Parse Nodes
    QJsonArray nodesArray = jsonObject.value("nodes").toArray();
    for (const QJsonValue& value : nodesArray) {
        QJsonObject nodeObject = value.toObject();
        Node node;
        node.name = nodeObject.value("name").toString();

        // Associate Nodes with their Networks (buses)
        QJsonArray nodeNetworksArray = nodeObject.value("buses").toArray();
        for (const QJsonValue& busValue : nodeNetworksArray) {
            QJsonObject busObject = busValue.toObject();
            NodeNetworkAssociation nodeNetwork;
            nodeNetwork.networkName = busObject.value("name").toString();
            nodeNetwork.sourceAddress = busObject.value("source_address").toInt();

            // Parse tx messages
            QJsonArray txArray = busObject.value("tx").toArray();
            for (const QJsonValue& txValue : txArray) {
                QJsonObject txObject = txValue.toObject();
                TxRxMessage txMessage;
                txMessage.name = txObject.value("name").toString();
                nodeNetwork.tx.append(txMessage);
            }

            // Sort nodeNetwork.tx alphabetically by name
            std::sort(nodeNetwork.tx.begin(), nodeNetwork.tx.end(), [](TxRxMessage& a, TxRxMessage& b) {
                return a.name.toLower() < b.name.toLower();
            });

            // Parse rx messages
            QJsonArray rxArray = busObject.value("rx").toArray();
            for (const QJsonValue& rxValue : rxArray) {
                QJsonObject rxObject = rxValue.toObject();
                TxRxMessage rxMessage;
                rxMessage.name = rxObject.value("name").toString();
                nodeNetwork.rx.append(rxMessage);
            }

            // Sort nodeNetwork.rx alphabetically by name
            std::sort(nodeNetwork.rx.begin(), nodeNetwork.rx.end(), [](TxRxMessage& a, TxRxMessage& b) {
                return a.name.toLower() < b.name.toLower();
            });

            node.networks.append(nodeNetwork);
        }

        // Sort node.networks alphabetically by networkName
        std::sort(node.networks.begin(), node.networks.end(), [](NodeNetworkAssociation& a, NodeNetworkAssociation& b) {
            return a.networkName.toLower() < b.networkName.toLower();
        });

        m_nodes.append(node);
    }

    // Sort m_nodes alphabetically by name
    std::sort(m_nodes.begin(), m_nodes.end(), [](Node& a, Node& b) {
        return a.name.toLower() < b.name.toLower();
    });
}



QList<Network>& DbcDataModel::networks() {
    return m_networks;
}

QList<Message>& DbcDataModel::messages() {
    return m_messages;
}

QList<Node>& DbcDataModel::nodes()  {
    return m_nodes;
}
