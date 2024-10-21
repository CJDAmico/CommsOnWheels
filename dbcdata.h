#ifndef DBCDATA_H
#define DBCDATA_H

#include <QString>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>

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
        double factor;           // Optional, defaults to 1.0
        double offset;           // Optional, defaults to 0.0
        QString units;           // Optional, defaults to empty string
        QVariant scaledMin;      // Optional, defaults to null
        QVariant scaledMax;      // Optional, defaults to null
        QVariant scaledDefault;  // Optional, defaults to same as scaledMin
        QList<Enumeration> enumerations; // Optional, defaults to empty list
};

class Message {
    public:
        quint64 pgn;                 // Required
        QString name;            // Required
        QString description;     // Optional, defaults to empty string
        int priority;            // Required
        int length;              // Required
        int txPeriodicity;       // Optional, defaults to 0
        bool txOnChange;         // Optional, defaults to false
        QList<Signal> data;      // Optional, defaults to empty list
};

class Bus {
    public:
        QString name;            // Required
        QString baud;            // Required, one of ["250k", "500k", "1M"]
};

class TxRxMessage {
    public:
        QString name;            // Required, must match entry from top-level 'messages'
};

class NodeBus {
    public:
        QString name;                // Required, must match entry from top-level 'buses'
        int sourceAddress;           // Required, 0-255 inclusive
        QList<TxRxMessage> tx;       // Optional, defaults to empty list
        QList<TxRxMessage> rx;       // Optional, defaults to empty list
};

class Node {
    public:
        QString name;            // Required
        QList<NodeBus> buses;    // Optional, defaults to empty list
};

class DbcDataModel {
    public:
        DbcDataModel();

        void setFileName(const QString& name);
        bool loadFromFile(const QString& filePath);
        QString fileName() const;
        void merge(const DbcDataModel& other);

        QList<Bus> buses() const;
        QList<Message> messages() const;
        QList<Node> nodes() const;

    private:
        QString m_fileName;
        QList<Bus> m_buses;
        QList<Message> m_messages;
        QList<Node> m_nodes;

        void parseJson(const QJsonObject& jsonObject);
};

#endif // DBCDATA
