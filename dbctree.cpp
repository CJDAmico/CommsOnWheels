#include "dbctree.h"
#include <QTreeWidgetItem>
#include <QDragMoveEvent>
#include <QDropEvent>
#include "dbcdata.h"

// Constructor implementation
DbcTree::DbcTree(QWidget *parent)
    : QTreeWidget(parent) {
    setUniformRowHeights(true);
    setHeaderHidden(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setDragDropMode(QAbstractItemView::InternalMove);  // Enable internal drag-and-drop reordering
    setStyleSheet(R"(
        QTreeWidget::item {
            padding: 10px; /* Add padding around the text */
        }
        QTreeWidget {
            font-size: 16px;  /* Increase font size */
        }
    )");
}

// Destructor implementation
DbcTree::~DbcTree() {
    // Destructor body can be empty, unless you need custom cleanup
}

void DbcTree::dragMoveEvent(QDragMoveEvent *event) {
    QTreeWidgetItem *sourceItem = currentItem();
    QTreeWidgetItem *targetItem = itemAt(event->position().toPoint());

    // If no valid source or target, ignore the event
    if (!sourceItem || !targetItem) {
        event->ignore();
        return;
    }

    QTreeWidget::dragMoveEvent(event);  // Call base class implementation
}

void DbcTree::dropEvent(QDropEvent *event) {
    QTreeWidgetItem *sourceItem = currentItem();
    QTreeWidgetItem *targetItem = itemAt(event->position().toPoint());

    // If no valid source or target, ignore the event
    if (!sourceItem || !targetItem) {
        event->ignore();
        return;
    }

    QTreeWidget::dropEvent(event);  // Call base class implementation to reorder items
}


// Helper function to find or create a child item
QTreeWidgetItem* findOrCreateItem(QTreeWidgetItem* parent, const QString& name, const QString& type,
                                  const QStringList& modelNames, const QString& uniqueKey = "",
                                  const QString& iconPath = "")
{
    for (int i = 0; i < parent->childCount(); ++i) {
        QTreeWidgetItem* child = parent->child(i);
        bool keysMatch = uniqueKey.isEmpty() || (child->data(0, Qt::UserRole + 2).toString() == uniqueKey);
        if (child->text(0) == name &&
            child->data(0, Qt::UserRole).toString() == type &&
            keysMatch) {
            // Update model names if necessary
            QStringList existingModels = child->data(0, Qt::UserRole + 1).toStringList();
            QStringList newModels = modelNames;
            newModels.removeDuplicates();
            QStringList updatedModels = existingModels + newModels;
            updatedModels.removeDuplicates();
            child->setData(0, Qt::UserRole + 1, updatedModels);
            return child;
        }
    }

    // Create new child item
    QTreeWidgetItem* newItem = new QTreeWidgetItem(parent, QStringList(name));
    newItem->setData(0, Qt::UserRole, type);                   // Set the type
    newItem->setData(0, Qt::UserRole + 1, modelNames);         // Set associated models
    if (!uniqueKey.isEmpty()) {
        newItem->setData(0, Qt::UserRole + 2, uniqueKey);      // Set the unique key if provided
    }

    // Set icon if provided
    if (!iconPath.isEmpty()) {
        newItem->setIcon(0, QIcon(iconPath));
    }

    return newItem;
}



// Function to find a message by name
Message* findMessage(QList<Message>& messages, const QString& name)
{
    for (Message& msg : messages) {
        if (msg.name.trimmed().compare(name.trimmed(), Qt::CaseInsensitive) == 0) {
            return &msg;
        }
    }
    return nullptr;
}

void DbcTree::populateTree(const QList<DbcDataModel*>& models)
{
    clear();

    // Create top-level categories
    QTreeWidgetItem* networksCategory = new QTreeWidgetItem(this, QStringList("<Networks>"));
    networksCategory->setData(0, Qt::UserRole, "Category");
    networksCategory->setExpanded(true);

    QTreeWidgetItem* nodesCategory = new QTreeWidgetItem(this, QStringList("<Nodes>"));
    nodesCategory->setData(0, Qt::UserRole, "Category");
    nodesCategory->setExpanded(true);

    QTreeWidgetItem* messagesCategory = new QTreeWidgetItem(this, QStringList("<Messages>"));
    messagesCategory->setData(0, Qt::UserRole, "Category");
    messagesCategory->setExpanded(true);

    // Iterate through all models
    for (DbcDataModel* model : models) {
        if (!model) {
            qWarning() << "populateTree: Encountered a null DbcDataModel pointer.";
            continue;
        }

        QString modelName = model->fileName();

        // -------------------------
        // Populate <Networks> Section
        // -------------------------
        for (const Network& network : model->networks()) {
            // No uniqueKey for networks
            QTreeWidgetItem* networkItem = findOrCreateItem(networksCategory, network.name, "Network",
                                                            QStringList() << modelName, /*uniqueKey=*/"",
                                                            ":/icons/network.svg");

            // Iterate through Nodes to find those associated with this network
            for (Node& node : model->nodes()) {
                for (NodeNetworkAssociation& nodeNetwork : node.networks) {
                    if (nodeNetwork.networkName == network.name) {
                        QString uniqueNodeKey = node.name + "::" + nodeNetwork.networkName;
                        // Create or find the node under this network
                        QTreeWidgetItem* nodeItem = findOrCreateItem(networkItem, node.name, "Node",
                                                                     QStringList() << modelName, uniqueNodeKey,
                                                                     ":/icons/node.svg");

                        // -------------------------
                        // Add <Transmitted Messages>
                        // -------------------------
                        QTreeWidgetItem* txMessagesCategory = new QTreeWidgetItem(nodeItem, QStringList("<Transmitted Messages>"));
                        txMessagesCategory->setData(0, Qt::UserRole, "Collapsible");
                        txMessagesCategory->setExpanded(true);

                        for (TxRxMessage& txMsg : nodeNetwork.tx) {
                            // Find the message in the model to get its PGN
                            Message* message = findMessage(model->messages(), txMsg.name);
                            QString uniqueMessageKey = message ? QString::number(message->pgn) : txMsg.name;

                            QTreeWidgetItem* txMsgItem = new QTreeWidgetItem(txMessagesCategory, QStringList(txMsg.name));
                            txMsgItem->setData(0, Qt::UserRole, "TxMessage");
                            txMsgItem->setData(0, Qt::UserRole + 1, QStringList() << modelName);
                            txMsgItem->setData(0, Qt::UserRole + 2, uniqueMessageKey);
                            txMsgItem->setIcon(0, QIcon(":/icons/message.svg"));

                            // Add <Signals> under TxMessage
                            QTreeWidgetItem* signalsCategory = new QTreeWidgetItem(txMsgItem, QStringList("<Signals>"));
                            signalsCategory->setData(0, Qt::UserRole, "Collapsible");
                            signalsCategory->setExpanded(true);

                            if (message) {
                                for (const Signal& signal : message->messageSignals) {
                                    QTreeWidgetItem* signalItem = new QTreeWidgetItem(signalsCategory, QStringList(signal.name));
                                    signalItem->setData(0, Qt::UserRole, "Signal");
                                    signalItem->setData(0, Qt::UserRole + 1, QStringList() << modelName);
                                    // No uniqueKey for signals in this context
                                    signalItem->setIcon(0, QIcon(":/icons/signal.svg"));
                                }
                            }
                        }

                        // -------------------------
                        // Add <Received Messages>
                        // -------------------------
                        QTreeWidgetItem* rxMessagesCategory = new QTreeWidgetItem(nodeItem, QStringList("<Received Messages>"));
                        rxMessagesCategory->setData(0, Qt::UserRole, "Collapsible");
                        rxMessagesCategory->setExpanded(true);

                        for (const TxRxMessage& rxMsg : nodeNetwork.rx) {
                            // Find the message in the model to get its PGN
                            Message* message = findMessage(model->messages(), rxMsg.name);
                            QString uniqueMessageKey = message ? QString::number(message->pgn) : rxMsg.name;



                            QTreeWidgetItem* rxMsgItem = new QTreeWidgetItem(rxMessagesCategory, QStringList(rxMsg.name));
                            rxMsgItem->setData(0, Qt::UserRole, "RxMessage");
                            rxMsgItem->setData(0, Qt::UserRole + 1, QStringList() << modelName);
                            rxMsgItem->setData(0, Qt::UserRole + 2, uniqueMessageKey);
                            rxMsgItem->setIcon(0, QIcon(":/icons/message.svg"));

                            // Add <Signals> under RxMessage
                            QTreeWidgetItem* signalsCategory = new QTreeWidgetItem(rxMsgItem, QStringList("<Signals>"));
                            signalsCategory->setData(0, Qt::UserRole, "Collapsible");
                            signalsCategory->setExpanded(true);

                            if (message) {
                                for (const Signal& signal : message->messageSignals) {
                                    QTreeWidgetItem* signalItem = new QTreeWidgetItem(signalsCategory, QStringList(signal.name));
                                    signalItem->setData(0, Qt::UserRole, "Signal");
                                    signalItem->setData(0, Qt::UserRole + 1, QStringList() << modelName);
                                    // No uniqueKey for signals in this context
                                    signalItem->setIcon(0, QIcon(":/icons/signal.svg"));
                                }
                            }
                        }
                    }
                }
            }
        }

        // -------------------------
        // Populate <Nodes> Section
        // -------------------------
        for (const Node& node : model->nodes()) {
            QString uniqueNodeKey = node.name + "::" + modelName;
            QTreeWidgetItem* nodeItem = findOrCreateItem(nodesCategory, node.name, "Node",
                                                         QStringList() << modelName, uniqueNodeKey,
                                                         ":/icons/node.svg");

            for (const NodeNetworkAssociation& nodeNetwork : node.networks) {
                QTreeWidgetItem* networkUnderNode = findOrCreateItem(nodeItem, nodeNetwork.networkName, "Network",
                                                                     QStringList() << modelName, /*uniqueKey=*/"",
                                                                     ":/icons/network.svg");


                // -------------------------
                // Add <Transmitted Messages>
                // -------------------------
                QTreeWidgetItem* txMessagesCategory = new QTreeWidgetItem(networkUnderNode, QStringList("<Transmitted Messages>"));
                txMessagesCategory->setData(0, Qt::UserRole, "Collapsible");
                txMessagesCategory->setExpanded(true);

                for (const TxRxMessage& txMsg : nodeNetwork.tx) {
                    // Find the message in the model to get its PGN
                    Message* message = findMessage(model->messages(), txMsg.name);
                    QString uniqueMessageKey = message ? QString::number(message->pgn) : txMsg.name;

                    if (message) {
                        message->messageTransmitters.append(std::make_pair(node.name, QString::number(nodeNetwork.sourceAddress)));
                    } else {
                        qWarning() << "Message not found for Tx:" << txMsg.name;
                    }

                    QTreeWidgetItem* txMsgItem = new QTreeWidgetItem(txMessagesCategory, QStringList(txMsg.name));
                    txMsgItem->setData(0, Qt::UserRole, "TxMessage");
                    txMsgItem->setData(0, Qt::UserRole + 1, QStringList() << modelName);
                    txMsgItem->setData(0, Qt::UserRole + 2, uniqueMessageKey);
                    txMsgItem->setIcon(0, QIcon(":/icons/message.svg"));

                    // Add <Signals> under TxMessage
                    QTreeWidgetItem* signalsCategory = new QTreeWidgetItem(txMsgItem, QStringList("<Signals>"));
                    signalsCategory->setData(0, Qt::UserRole, "Collapsible");
                    signalsCategory->setExpanded(true);

                    if (message) {
                        for (const Signal& signal : message->messageSignals) {
                            QTreeWidgetItem* signalItem = new QTreeWidgetItem(signalsCategory, QStringList(signal.name));
                            signalItem->setData(0, Qt::UserRole, "Signal");
                            signalItem->setData(0, Qt::UserRole + 1, QStringList() << modelName);
                            // No uniqueKey for signals in this context
                            signalItem->setIcon(0, QIcon(":/icons/signal.svg"));
                        }
                    }
                }

                // -------------------------
                // Add <Received Messages>
                // -------------------------
                QTreeWidgetItem* rxMessagesCategory = new QTreeWidgetItem(networkUnderNode, QStringList("<Received Messages>"));
                rxMessagesCategory->setData(0, Qt::UserRole, "Collapsible");
                rxMessagesCategory->setExpanded(true);

                for (const TxRxMessage& rxMsg : nodeNetwork.rx) {
                    // Find the message in the model to get its PGN
                    Message* message = findMessage(model->messages(), rxMsg.name);
                    QString uniqueMessageKey = message ? QString::number(message->pgn) : rxMsg.name;


                    if (message) {
                        message->messageReceivers.append(std::make_pair(node.name, QString::number(nodeNetwork.sourceAddress)));
                    } else {
                        qWarning() << "Message not found for Tx:" << rxMsg.name;
                    }

                    QTreeWidgetItem* rxMsgItem = new QTreeWidgetItem(rxMessagesCategory, QStringList(rxMsg.name));
                    rxMsgItem->setData(0, Qt::UserRole, "RxMessage");
                    rxMsgItem->setData(0, Qt::UserRole + 1, QStringList() << modelName);
                    rxMsgItem->setData(0, Qt::UserRole + 2, uniqueMessageKey);
                    rxMsgItem->setIcon(0, QIcon(":/icons/message.svg"));

                    // Add <Signals> under RxMessage
                    QTreeWidgetItem* signalsCategory = new QTreeWidgetItem(rxMsgItem, QStringList("<Signals>"));
                    signalsCategory->setData(0, Qt::UserRole, "Collapsible");
                    signalsCategory->setExpanded(true);

                    if (message) {
                        for (const Signal& signal : message->messageSignals) {
                            QTreeWidgetItem* signalItem = new QTreeWidgetItem(signalsCategory, QStringList(signal.name));
                            signalItem->setData(0, Qt::UserRole, "Signal");
                            signalItem->setData(0, Qt::UserRole + 1, QStringList() << modelName);
                            // No uniqueKey for signals in this context
                            signalItem->setIcon(0, QIcon(":/icons/signal.svg"));
                        }
                    }
                }
            }
        }

        // -------------------------
        // Populate <Messages> Section
        // -------------------------
        for (const Message& message : model->messages()) {
            QString uniqueMessageKey = QString::number(message.pgn);
            QTreeWidgetItem* messageItem = findOrCreateItem(messagesCategory, message.name, "Message",
                                                            QStringList() << modelName, uniqueMessageKey,
                                                            ":/icons/message.svg");

            // -------------------------
            // Add <Signals> under Message
            // -------------------------
            QTreeWidgetItem* signalsCategory = new QTreeWidgetItem(messageItem, QStringList("<Signals>"));
            signalsCategory->setData(0, Qt::UserRole, "Collapsible");
            signalsCategory->setExpanded(true);

            for (const Signal& signal : message.messageSignals) {
                QTreeWidgetItem* signalItem = new QTreeWidgetItem(signalsCategory, QStringList(signal.name));
                signalItem->setData(0, Qt::UserRole, "Signal");
                signalItem->setData(0, Qt::UserRole + 1, QStringList() << modelName);
                // No uniqueKey for signals in this context
                signalItem->setIcon(0, QIcon(":/icons/signal.svg"));
            }

            // -------------------------
            // Add <Networks> under Message
            // -------------------------
            QTreeWidgetItem* networksUnderMessage = new QTreeWidgetItem(messageItem, QStringList("<Networks>"));
            networksUnderMessage->setData(0, Qt::UserRole, "Collapsible");
            networksUnderMessage->setExpanded(true);

            QMap<QString, QSet<QString>> networkTransmitters;
            QMap<QString, QSet<QString>> networkReceivers;

            // Find all transmitters and receivers for this message
            for (DbcDataModel* modelInner : models) {
                for (const Node& node : modelInner->nodes()) {
                    for (const NodeNetworkAssociation& nodeNetwork : node.networks) {
                        if (std::any_of(nodeNetwork.tx.begin(), nodeNetwork.tx.end(), [&](const TxRxMessage& tx) { return tx.name == message.name; })) {
                            networkTransmitters[nodeNetwork.networkName].insert(node.name);
                        }
                        if (std::any_of(nodeNetwork.rx.begin(), nodeNetwork.rx.end(), [&](const TxRxMessage& rx) { return rx.name == message.name; })) {
                            networkReceivers[nodeNetwork.networkName].insert(node.name);
                        }
                    }
                }
            }

            // Iterate through all networks that have transmitters or receivers
            QList<QString> networkTransmittersList = networkTransmitters.keys();
            QList<QString> networkReceiversList = networkReceivers.keys();

            QSet<QString> allNetworks(networkTransmittersList.begin(), networkTransmittersList.end());
            allNetworks = allNetworks.unite(QSet<QString>(networkReceiversList.begin(), networkReceiversList.end()));
            QList<QString> networks = allNetworks.values();
            for (const QString& networkName : networks) {
                // No uniqueKey for networks under messages
                QTreeWidgetItem* networkItem = findOrCreateItem(networksUnderMessage, networkName, "Network",
                                                                QStringList() << modelName, /*uniqueKey=*/"",
                                                                ":/icons/network.svg");

                // -------------------------
                // Add <Transmitters>
                // -------------------------
                QTreeWidgetItem* transmittersCategory = new QTreeWidgetItem(networkItem, QStringList("<Transmitters>"));
                transmittersCategory->setData(0, Qt::UserRole, "Collapsible");
                transmittersCategory->setExpanded(true);

                QList<QString> transmitters = networkTransmitters.value(networkName).values();
                for (const QString& tx : transmitters) {
                    QString uniqueNodeKey = tx + "::" + networkName;
                    findOrCreateItem(transmittersCategory, tx, "Node", QStringList() << modelName, uniqueNodeKey, ":/icons/node.svg");
                }

                // -------------------------
                // Add <Receivers>
                // -------------------------
                QTreeWidgetItem* receiversCategory = new QTreeWidgetItem(networkItem, QStringList("<Receivers>"));
                receiversCategory->setData(0, Qt::UserRole, "Collapsible");
                receiversCategory->setExpanded(true);

                QList<QString> receivers = networkReceivers.value(networkName).values();
                for (const QString& rx : receivers) {
                    QString uniqueNodeKey = rx + "::" + networkName;
                    findOrCreateItem(receiversCategory, rx, "Node", QStringList() << modelName, uniqueNodeKey, ":/icons/node.svg");
                }
            }
        }
    }
}

