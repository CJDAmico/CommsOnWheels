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

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Couldn't open DBC file:" << filePath;
        return false;
    }

    QTextStream in(&file);
    QString line;

    QMap<QString, Attribute> defaultMessageAttributes; // To store default attributes for messages
    QMap<QString, Attribute> defaultSignalAttributes;  // To store default attributes for signals
    QMap<QString, Attribute> defaultNodeAttributes;    // To store default attributes for nodes

    QRegularExpression reNode("^BU_:\\s*(.*)");
    QRegularExpression reMessage("^BO_\\s+(\\d+)\\s+(\\w+):\\s+(\\d+)\\s+(\\w+)");
    QRegularExpression reSignal(
        "^\\s*SG_\\s+(\\w+)\\s*(?:([mM]\\d*[mM]?)\\s*)?:\\s*" // Signal name and optional multiplexer indicator
        "(\\d+)\\|(\\d+)@(\\d+)([+-])\\s+"                    // Start bit, length, byte order, sign
        "\\(([^,]+),([^\\)]+)\\)\\s+"                         // Factor and offset
        "\\[([^\\|]+)\\|([^\\]]+)\\]\\s+"                     // Min and max
        "\"([^\"]*)\"\\s*"                                    // Unit
        "(.*)"                                                // Receivers
        );
    QRegularExpression reAttributeDef("^BA_DEF_\\s+(\\w+)\\s+\"([^\"]+)\"\\s+(\\w+)");
    QRegularExpression reAttributeDefDef("^BA_DEF_DEF_\\s+\"([^\"]+)\"\\s+\"?([^\"]+)\"?");
    QRegularExpression reAttributeAssignment("^BA_\\s+\"([^\"]+)\"\\s+(\\w+)\\s+(\\w+)\\s+\"?([^\"]+)\"?");

    // Set network name to file name
    Network network;
    network.name = QFileInfo(filePath).fileName();
    network.baud = ""; // Set baud rate
    m_networks.append(network);

    // Map to keep track of nodes indices by name
    QMap<QString, int> nodeMap;

    while (!in.atEnd()) {
        line = in.readLine();

        // Parse nodes
        QRegularExpressionMatch match = reNode.match(line);
        if (match.hasMatch()) {
            QStringList nodes = match.captured(1).split(' ', Qt::SkipEmptyParts);
            for (const QString& nodeName : nodes) {
                Node node;
                node.name = nodeName.trimmed();

                // Update Node-Network Association
                NodeNetworkAssociation nodeNetworkAssociation;
                nodeNetworkAssociation.networkName = network.name;
                nodeNetworkAssociation.sourceAddress = 0; // TODO: Source address parsing
                node.networks.append(nodeNetworkAssociation);

                m_nodes.append(node);
                nodeMap.insert(node.name, m_nodes.size() - 1);
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
            message.priority = 0; // Initialize priority

            // Add to Node-Network Association
            if (!nodeMap.contains(transmitter)) {
                qWarning() << "Transmitter node not found: " << transmitter << ". Creating new node.";
                Node newNode;
                newNode.name = transmitter;

                // Update Node-Network Association
                NodeNetworkAssociation nodeNetworkAssociation;
                nodeNetworkAssociation.networkName = network.name;
                nodeNetworkAssociation.sourceAddress = 0; // TODO: Source address parsing
                newNode.networks.append(nodeNetworkAssociation);

                m_nodes.append(newNode);
                nodeMap.insert(transmitter, m_nodes.size() - 1);
            }

            Node& transmitterNode = m_nodes[nodeMap[transmitter]];
            for (NodeNetworkAssociation& networkAssociation : transmitterNode.networks) {
                if (networkAssociation.networkName == network.name) {
                    // Check if the message already exists in the tx list
                    bool messageExistsInTx = false;
                    for (const TxRxMessage& txMsg : networkAssociation.tx) {
                        if (txMsg.name == message.name) {
                            messageExistsInTx = true;
                            break;
                        }
                    }
                    if (!messageExistsInTx) {
                        TxRxMessage txRxMessage;
                        txRxMessage.name = message.name;
                        networkAssociation.tx.append(txRxMessage);
                    }
                }
            }

            m_messages.append(message);
            continue;
        }

        // Parse signals
        match = reSignal.match(line);
        if (match.hasMatch()) {
            Signal signal;
            signal.name = match.captured(1).trimmed();
            QString multiplexerIndicator = match.captured(2).trimmed(); // Optional multiplexer indicator
            signal.isMultiplexer = false; // Default value
            signal.multiplexValue = -1; // Default value
            signal.startBit = match.captured(3).toInt();
            signal.bitLength = match.captured(4).toInt();
            signal.isBigEndian = (match.captured(5).toInt() == 1);
            signal.isTwosComplement = (match.captured(6) == "-");
            signal.factor = match.captured(7).toDouble();
            signal.offset = match.captured(8).toDouble();
            signal.scaledMin = match.captured(9).toDouble();
            signal.scaledMax = match.captured(10).toDouble();
            signal.units = match.captured(11).trimmed();
            QString receivers = match.captured(12).trimmed();
            QStringList receiverNames = receivers.split(',', Qt::SkipEmptyParts);

            // Determine if the signal is a multiplexer or multiplexed signal
            if (!multiplexerIndicator.isEmpty()) {
                if (multiplexerIndicator == "M") {
                    // This is a multiplexer signal
                    signal.isMultiplexer = true;
                    signal.multiplexValue = -1;
                } else if (multiplexerIndicator.startsWith('m')) {
                    // This is a multiplexed signal, possibly also a multiplexer
                    bool isMultiplexer = false;
                    QString valuePart = multiplexerIndicator.mid(1);
                    if (valuePart.endsWith('M')) {
                        isMultiplexer = true;
                        valuePart.chop(1); // Remove the 'M' at the end
                    }
                    bool ok;
                    int value = valuePart.toInt(&ok);
                    if (ok) {
                        signal.multiplexValue = value;
                        signal.isMultiplexer = isMultiplexer;
                    } else {
                        qWarning() << "Invalid multiplex value for signal" << signal.name << "with indicator" << multiplexerIndicator;
                        signal.multiplexValue = -1;
                        signal.isMultiplexer = isMultiplexer; // Set isMultiplexer based on presence of 'M' (Multiplexer)
                    }
                } else {
                    qWarning() << "Unknown multiplexer indicator:" << multiplexerIndicator << "for signal" << signal.name;
                }
            }

            // Parse all receiver names
            for (const QString& receiverName : receiverNames) {
                QString trimmedReceiverName = receiverName.trimmed();
                if (trimmedReceiverName.isEmpty()) {
                    qWarning() << "Receiver Names Empty";
                    continue;
                }
                // Add to Node-Network Association
                if (!nodeMap.contains(trimmedReceiverName)) {
                    qWarning() << "Receiver node not found: " << trimmedReceiverName;
                    // Create a new node and add it to m_nodes and nodeMap
                    Node node;
                    node.name = trimmedReceiverName;

                    NodeNetworkAssociation nodeNetworkAssociation;
                    nodeNetworkAssociation.networkName = network.name;
                    nodeNetworkAssociation.sourceAddress = 0;
                    node.networks.append(nodeNetworkAssociation);

                    m_nodes.append(node);
                    nodeMap.insert(node.name, m_nodes.size() - 1);
                }
                Node& receiverNode = m_nodes[nodeMap[trimmedReceiverName]];
                for (NodeNetworkAssociation& networkAssociation : receiverNode.networks) {
                    if (networkAssociation.networkName == network.name) {
                        // Check if the message already exists in the rx list
                        bool messageExistsInRx = false;
                        for (const TxRxMessage& rxMsg : networkAssociation.rx) {
                            if (rxMsg.name == m_messages.last().name) {
                                messageExistsInRx = true;
                                break;
                            }
                        }
                        if (!messageExistsInRx) {
                            TxRxMessage txRxMessage;
                            txRxMessage.name = m_messages.last().name;
                            networkAssociation.rx.append(txRxMessage);
                        }
                    }
                }
            }

            m_messages.last().messageSignals.append(signal);
            continue;
        }

        // Parse attribute definitions (BA_DEF_)
        match = reAttributeDef.match(line);
        if (match.hasMatch()) {
            QString attributeType = match.captured(1).trimmed(); // Object type (BO_, SG_, etc.)
            QString attributeName = match.captured(2).trimmed();
            QString attributeValueType = match.captured(3).trimmed(); // Value type (STRING, INT, etc.)

            // Handle attribute definition based on its type (BO_ for messages, SG_ for signals)
            if (attributeType == "BO_") {
                // Define attribute for all messages
                defaultMessageAttributes.insert(attributeName, {attributeName, ""});
            } else if (attributeType == "SG_") {
                // Define attribute for all signals
                defaultSignalAttributes.insert(attributeName, {attributeName, ""});
            } else if (attributeType == "BU_") {
                // Define attribute for all nodes
                defaultNodeAttributes.insert(attributeName, {attributeName, ""});
            }
            continue;
        }

        // Parse default attribute values (BA_DEF_DEF_)
        match = reAttributeDefDef.match(line);
        if (match.hasMatch()) {
            QString attributeName = match.captured(1).trimmed();
            QString defaultValue = match.captured(2).trimmed();

            // Assign default value to the appropriate type
            if (defaultMessageAttributes.contains(attributeName)) {
                defaultMessageAttributes[attributeName].value = defaultValue;

                // Add default attribute to message attributes
                for (auto& message : m_messages) {
                    message.messageAttributes.append(defaultMessageAttributes[attributeName]);
                }
            }
            if (defaultSignalAttributes.contains(attributeName)) {
                defaultSignalAttributes[attributeName].value = defaultValue;
                // Add default attribute to signal attributes
                for (auto& message : m_messages) {
                    for (auto& signal : message.messageSignals) {
                        signal.signalAttributes.append(defaultSignalAttributes[attributeName]);
                    }
                }
            }
            if (defaultNodeAttributes.contains(attributeName)) {
                defaultNodeAttributes[attributeName].value = defaultValue;
                // Add default attribute to node attributes
                for (auto& node : m_nodes) {
                    node.nodeAttributes.append(defaultNodeAttributes[attributeName]);
                }
            }
            continue;
        }

        // Parse assigned attributes (BA_)
        match = reAttributeAssignment.match(line);
        if (match.hasMatch()) {
            QString attributeName = match.captured(1).trimmed();
            QString targetType = match.captured(2).trimmed();
            QString targetName = match.captured(3).trimmed();
            QString value = match.captured(4).trimmed();

            // Create a new Attribute object
            Attribute attribute;
            attribute.name = attributeName;
            attribute.value = value;

            // Replace existing attribute value if already present
            if (targetType == "BO_") {
                // Replace attribute in Message
                for (auto& message : m_messages) {
                    QString pgnString = QString::number(message.pgn);
                    if (pgnString == targetName) {
                        // Check if the attribute already exists and replace
                        bool found = false;
                        for (auto& msgAttr : message.messageAttributes) {
                            if (msgAttr.name == attributeName) {
                                msgAttr.value = value; // Replace the value
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            message.messageAttributes.append(attribute); // Append if not found
                        }
                        break;
                    }
                }
            } else if (targetType == "SG_") {
                // Replace attribute in Signal
                for (auto& message : m_messages) {
                    QString pgnString = QString::number(message.pgn);
                    if (pgnString == targetName) {
                        // For each signal in that message
                        for (auto& signal : message.messageSignals) {
                            bool found = false;
                            // For each attribute in the signal
                            for (auto& sigAttr : signal.signalAttributes) {
                                if (sigAttr.name == attributeName) {
                                    sigAttr.value = value; // Replace the value
                                    found = true;
                                    break;
                                }
                            }
                            if (!found) {
                                signal.signalAttributes.append(attribute); // Append if not found
                            }
                            break;
                        }
                    }
                }
            } else if (targetType == "BU_") {
                // Replace attribute in Node
                for (auto& node : m_nodes) {
                    if (node.name == targetName) {
                        bool found = false;
                        for (auto& nodeAttr : node.nodeAttributes) {
                            if (nodeAttr.name == attributeName) {
                                nodeAttr.value = value; // Replace the value
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            node.nodeAttributes.append(attribute); // Append if not found
                        }
                        break;
                    }
                }
            }
            continue;
        }
    }

    // Close the file
    file.close();

    // Add sorting code here

    // Sort m_networks alphabetically by name
    std::sort(m_networks.begin(), m_networks.end(), [](const Network& a, const Network& b) {
        return a.name.toLower() < b.name.toLower();
    });

    // Sort m_nodes alphabetically by name
    std::sort(m_nodes.begin(), m_nodes.end(), [](Node& a, Node& b) {
        return a.name.toLower() < b.name.toLower();
    });

    // For each node, sort node.networks alphabetically by networkName
    for (Node& node : m_nodes) {
        std::sort(node.networks.begin(), node.networks.end(), [](NodeNetworkAssociation& a, NodeNetworkAssociation& b) {
            return a.networkName.toLower() < b.networkName.toLower();
        });

        // Sort tx and rx messages
        for (NodeNetworkAssociation& nodeNetwork : node.networks) {
            std::sort(nodeNetwork.tx.begin(), nodeNetwork.tx.end(), [](const TxRxMessage& a, const TxRxMessage& b) {
                return a.name.toLower() < b.name.toLower();
            });

            std::sort(nodeNetwork.rx.begin(), nodeNetwork.rx.end(), [](const TxRxMessage& a, const TxRxMessage& b) {
                return a.name.toLower() < b.name.toLower();
            });
        }
    }

    // Sort m_messages alphabetically by name
    std::sort(m_messages.begin(), m_messages.end(), [](Message& a, Message& b) {
        return a.name.toLower() < b.name.toLower();
    });

    // For each message, sort messageSignals alphabetically by name
    for (Message& message : m_messages) {
        std::sort(message.messageSignals.begin(), message.messageSignals.end(), [](Signal& a, Signal& b) {
            return a.name.toLower() < b.name.toLower();
        });
    }

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

        // Parse Network Attributes
        QJsonArray attributesArray = busObject.value("networkAttributes").toArray();
        for (const QJsonValue& attrValue : attributesArray) {
            QJsonObject attrObject = attrValue.toObject();
            Attribute attribute;
            attribute.name = attrObject.value("name").toString();
            attribute.type = attrObject.value("type").toString();
            attribute.value = attrObject.value("value").toString();
            network.networkAttributes.append(attribute);
        }

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

        // Parse Message Attributes
        QJsonArray messageAttributesArray = messageObject.value("messageAttributes").toArray();
        for (const QJsonValue& attrValue : messageAttributesArray) {
            QJsonObject attrObject = attrValue.toObject();
            Attribute attribute;
            attribute.name = attrObject.value("name").toString();
            attribute.type = attrObject.value("type").toString();
            attribute.value = attrObject.value("value").toString();
            message.messageAttributes.append(attribute);
        }

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
            signal.isMultiplexer = signalObject.value("is_multiplexer").toBool(false);

            // Handle scaled_min, scaled_max, scaled_default
            signal.scaledMin = signalObject.value("scaled_min").toVariant();
            signal.scaledMax = signalObject.value("scaled_max").toVariant();
            signal.scaledDefault = signalObject.value("scaled_default").toVariant();

            // If scaled_default is null, default to scaled_min
            if (signal.scaledDefault.isNull()) {
                signal.scaledDefault = signal.scaledMin;
            }

            // Parse Signal Attributes
            QJsonArray signalAttributesArray = signalObject.value("signalAttributes").toArray();
            for (const QJsonValue& attrValue : signalAttributesArray) {
                QJsonObject attrObject = attrValue.toObject();
                Attribute attribute;
                attribute.name = attrObject.value("name").toString();
                attribute.type = attrObject.value("type").toString();
                attribute.value = attrObject.value("value").toString();
                signal.signalAttributes.append(attribute);
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

        // Parse Node Attributes
        QJsonArray nodeAttributesArray = nodeObject.value("nodeAttributes").toArray();
        for (const QJsonValue& attrValue : nodeAttributesArray) {
            QJsonObject attrObject = attrValue.toObject();
            Attribute attribute;
            attribute.name = attrObject.value("name").toString();
            attribute.type = attrObject.value("type").toString();
            attribute.value = attrObject.value("value").toString();
            node.nodeAttributes.append(attribute);
        }

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
