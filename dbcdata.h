#ifndef DBCDATA_H
#define DBCDATA_H

#include <QString>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>

class Attribute {
    public:
        QString name;
        QString type;
        QString value;
};

class TxRxMessage {
    public:
        QString name;            // Required, must match entry from top-level 'messages'
};

class Network {
    public:
        QString name;            // Required
        QString baud;            // Required, one of ["250k", "500k", "1M"]
        QList<Attribute> networkAttributes;  // Optional, defaults to empty list
};

struct NodeNetworkAssociation {
    QString networkName;             // Reference to a Network's name
    int sourceAddress = 0;           // Required, 0-255 inclusive
    QList<TxRxMessage> tx;           // Optional, defaults to empty list
    QList<TxRxMessage> rx;           // Optional, defaults to empty list
};

// Node Class
class Node {
    public:
        QString name;                            // Required, must match entry from top-level 'Networks'
        QList<NodeNetworkAssociation> networks;  // Associations with Networks
        QList<Attribute> nodeAttributes;  // Optional, defaults to empty list
};

class Enumeration {
    public:
        QString name;            // Required
        QString description;     // Optional, defaults to empty string
        int value;               // Required
};

class Signal {
    public:
        int spn;                 // Optional, defaults to 0
        QString name;            // Required
        QString description;     // Optional, defaults to empty string
        int startBit;            // Required
        int bitLength;           // Required
        bool isBigEndian;        // Optional, defaults to false
        bool isTwosComplement;   // Optional, defaults to false
        bool isMultiplexor;      // Optional, defaults to false
        double factor;           // Optional, defaults to 1.0
        double offset;           // Optional, defaults to 0.0
        int multiplexValue;      // Optional, defaults to -1
        QString units;           // Optional, defaults to empty string
        QVariant scaledMin;      // Optional, defaults to null
        QVariant scaledMax;      // Optional, defaults to null
        QVariant scaledDefault;  // Optional, defaults to same as scaledMin
        QList<Enumeration> enumerations; // Optional, defaults to empty list
        QList<Attribute> signalAttributes;  // Optional, defaults to empty list
};

class Message {
    public:
        quint64 pgn;             // Required
        QString name;            // Required
        QString description;     // Optional, defaults to empty string
        int priority;            // Required
        int length;              // Required
        int txPeriodicity;       // Optional, defaults to 0
        int multiplexValue;      // Optional, default to -1
        bool txOnChange;         // Optional, defaults to false
        QList<Signal> messageSignals;      // Optional, defaults to empty list
        QList<Network> messageNetworks;      // Optional, defaults to empty list
        QList<Attribute> messageAttributes;  // Optional, defaults to empty list
        QList<std::pair<QString, QString>> messageTransmitters; // Pair of transmitting node name and source address
        QList<std::pair<QString, QString>> messageReceivers;    // Pair of receiving node name and source address
};

class DbcDataModel {
    public:
        DbcDataModel();

        void setFileName(const QString& name);
        bool loadJson(const QString& filePath);
        bool importDBC(const QString& filePath);
        QString fileName() const;

        QList<Network>& networks();
        QList<Node>& nodes();
        QList<Message>& messages();

    private:
        QString m_fileName;
        QList<Network> m_networks;
        QList<Node> m_nodes;
        QList<Message> m_messages;

        void parseJson(const QJsonObject& jsonObject);
};

#endif // DBCDATA
