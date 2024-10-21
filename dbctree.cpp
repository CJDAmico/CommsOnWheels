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


void DbcTree::populateTree(const QList<DbcDataModel*>& models)
{
    // Clear existing items
    clear();
    m_models = models;

    // Map to track nodes by name
    QMap<QString, QTreeWidgetItem*> nodeMap;

    // Map to collect all node buses by node name
    QMap<QString, QList<NodeBus>> nodeBusesMap;

    // For each model (DBC file), create a root node
    for (const DbcDataModel* model : models) {
        QString rootName = model->fileName(); // Use the DBC file name as the root name
        QTreeWidgetItem* rootItem = new QTreeWidgetItem(this);
        rootItem->setText(0, rootName);
        addTopLevelItem(rootItem);

        // Store the node type and model file name
        rootItem->setData(0, Qt::UserRole + 1, "Root");
        rootItem->setData(0, Qt::UserRole + 2, model->fileName());

        // Buses
        QTreeWidgetItem* busesItem = new QTreeWidgetItem(rootItem);
        busesItem->setText(0, "Buses");
        busesItem->setData(0, Qt::UserRole + 1, "Buses");

        // Collect and sort buses
        QList<Bus> buses = model->buses();
        std::sort(buses.begin(), buses.end(), [](const Bus& a, const Bus& b) {
            return a.name.toLower() < b.name.toLower();
        });

        for (const Bus& bus : buses) {
            QTreeWidgetItem* busItem = new QTreeWidgetItem(busesItem);
            busItem->setText(0, bus.name);
            busItem->setData(0, Qt::UserRole, bus.name); // Store bus name
            busItem->setData(0, Qt::UserRole + 1, "Bus");
            busItem->setData(0, Qt::UserRole + 2, model->fileName());
        }

        // Nodes
        QTreeWidgetItem* nodesItem = new QTreeWidgetItem(rootItem);
        nodesItem->setText(0, "Nodes");
        nodesItem->setData(0, Qt::UserRole + 1, "Nodes");

        // Collect and sort nodes
        QList<Node> nodes = model->nodes();
        std::sort(nodes.begin(), nodes.end(), [](const Node& a, const Node& b) {
            return a.name.toLower() < b.name.toLower();
        });

        for (const Node& node : nodes) {
            QString nodeName = node.name;

            // Ensure the node is in the nodeMap
            QTreeWidgetItem* nodeItem;
            if (nodeMap.contains(nodeName)) {
                nodeItem = nodeMap[nodeName];
                // Retrieve existing modelFileNames
                QStringList existingModels = nodeItem->data(0, Qt::UserRole + 2).toStringList();
                // Append the current model's file name if not already present
                if (!existingModels.contains(model->fileName())) {
                    existingModels.append(model->fileName());
                    nodeItem->setData(0, Qt::UserRole + 2, existingModels);
                }
            } else {
                nodeItem = new QTreeWidgetItem();
                nodeItem->setText(0, nodeName);
                nodeItem->setData(0, Qt::UserRole, nodeName);
                nodeItem->setData(0, Qt::UserRole + 1, "Node");
                nodeItem->setData(0, Qt::UserRole + 2, QStringList() << model->fileName()); // Initialize with current model
                nodeMap[nodeName] = nodeItem;
            }

            // Collect node buses
            QList<NodeBus>& nodeBusesList = nodeBusesMap[nodeName];
            nodeBusesList.append(node.buses);
        }

        // Add nodes and their sorted node buses to the tree
        for (const QString& nodeName : nodeMap.keys()) {
            QTreeWidgetItem* nodeItem = nodeMap[nodeName];
            nodesItem->addChild(nodeItem); // Add nodeItem under "Nodes"

            // Get the node buses and sort them
            QList<NodeBus> nodeBusesList = nodeBusesMap[nodeName];
            std::sort(nodeBusesList.begin(), nodeBusesList.end(), [](const NodeBus& a, const NodeBus& b) {
                return a.name.toLower() < b.name.toLower();
            });

            // Add sorted node buses under the node
            for (const NodeBus& nodeBus : nodeBusesList) {
                QTreeWidgetItem* nodeBusItem = new QTreeWidgetItem(nodeItem);
                nodeBusItem->setText(0, nodeBus.name);
                nodeBusItem->setData(0, Qt::UserRole, nodeBus.name);
                nodeBusItem->setData(0, Qt::UserRole + 1, "NodeBus");
                nodeBusItem->setData(0, Qt::UserRole + 2, model->fileName()); // Correctly associate model file name

                // Transmitted Messages
                QTreeWidgetItem* txMessagesItem = new QTreeWidgetItem(nodeBusItem);
                txMessagesItem->setText(0, "Transmitted Messages");
                txMessagesItem->setData(0, Qt::UserRole + 1, "TxMessages");

                // Sort transmitted messages
                QList<TxRxMessage> txMessages = nodeBus.tx;
                std::sort(txMessages.begin(), txMessages.end(), [](const TxRxMessage& a, const TxRxMessage& b) {
                    return a.name.toLower() < b.name.toLower();
                });

                for (const TxRxMessage& txMsg : txMessages) {
                    QTreeWidgetItem* msgItem = new QTreeWidgetItem(txMessagesItem);
                    msgItem->setText(0, txMsg.name);
                    msgItem->setData(0, Qt::UserRole, txMsg.name);
                    msgItem->setData(0, Qt::UserRole + 1, "TxMessage"); // Assign specific type
                    msgItem->setData(0, Qt::UserRole + 2, model->fileName());
                }

                // Received Messages
                QTreeWidgetItem* rxMessagesItem = new QTreeWidgetItem(nodeBusItem);
                rxMessagesItem->setText(0, "Received Messages");
                rxMessagesItem->setData(0, Qt::UserRole + 1, "RxMessages");

                // Sort received messages
                QList<TxRxMessage> rxMessages = nodeBus.rx;
                std::sort(rxMessages.begin(), rxMessages.end(), [](const TxRxMessage& a, const TxRxMessage& b) {
                    return a.name.toLower() < b.name.toLower();
                });

                for (const TxRxMessage& rxMsg : rxMessages) {
                    QTreeWidgetItem* msgItem = new QTreeWidgetItem(rxMessagesItem);
                    msgItem->setText(0, rxMsg.name);
                    msgItem->setData(0, Qt::UserRole, rxMsg.name);
                    msgItem->setData(0, Qt::UserRole + 1, "RxMessage"); // Assign specific type
                    msgItem->setData(0, Qt::UserRole + 2, model->fileName());
                }
            }
        }

        // Messages
        QTreeWidgetItem* messagesItem = new QTreeWidgetItem(rootItem);
        messagesItem->setText(0, "Messages");
        messagesItem->setData(0, Qt::UserRole + 1, "Messages");

        // Collect and sort messages
        QList<Message> messages = model->messages();
        std::sort(messages.begin(), messages.end(), [](const Message& a, const Message& b) {
            return a.name.toLower() < b.name.toLower();
        });

        for (const Message& message : messages) {
            QTreeWidgetItem* messageItem = new QTreeWidgetItem(messagesItem);
            messageItem->setText(0, message.name);
            messageItem->setData(0, Qt::UserRole, message.name); // Store message name
            messageItem->setData(0, Qt::UserRole + 1, "Message"); // Top-level message type
            messageItem->setData(0, Qt::UserRole + 2, model->fileName());

            // Collect and sort signals
            QList<Signal> messageSignals = message.data;
            std::sort(messageSignals.begin(), messageSignals.end(), [](const Signal& a, const Signal& b) {
                return a.name.toLower() < b.name.toLower();
            });

            for (const Signal& signal : messageSignals) {
                QTreeWidgetItem* signalItem = new QTreeWidgetItem(messageItem);
                signalItem->setText(0, signal.name);
                signalItem->setData(0, Qt::UserRole, signal.name); // Store signal name
                signalItem->setData(0, Qt::UserRole + 1, "Signal");
                signalItem->setData(0, Qt::UserRole + 2, model->fileName());
            }
        }
    }
}
