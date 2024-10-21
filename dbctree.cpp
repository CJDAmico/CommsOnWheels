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
    clear();

    // Create unified top-level categories
    QTreeWidgetItem* busesItem = new QTreeWidgetItem(this, QStringList("Buses"));
    busesItem->setData(0, Qt::UserRole + 1, "Buses");

    QTreeWidgetItem* ecusItem = new QTreeWidgetItem(this, QStringList("ECUs"));
    ecusItem->setData(0, Qt::UserRole + 1, "ECUs");

    QTreeWidgetItem* messagesItem = new QTreeWidgetItem(this, QStringList("Messages"));
    messagesItem->setData(0, Qt::UserRole + 1, "Messages");

    // Iterate through all models and aggregate data
    for (const DbcDataModel* model : models) {
        QString modelName = model->fileName();

        // Populate Buses
        for (const Bus& bus : model->buses()) {
            // Check if bus already exists
            QTreeWidgetItem* existingBus = nullptr;
            for (int i = 0; i < busesItem->childCount(); ++i) {
                QTreeWidgetItem* child = busesItem->child(i);
                if (child->text(0) == bus.name) {
                    existingBus = child;
                    break;
                }
            }
            if (!existingBus) {
                existingBus = new QTreeWidgetItem(busesItem, QStringList(bus.name));
                existingBus->setData(0, Qt::UserRole, "Bus");
                existingBus->setData(0, Qt::UserRole + 1, QStringList() << modelName);
            }
            else {
                // Append model name if not already present
                QStringList modelsList = existingBus->data(0, Qt::UserRole + 1).toStringList();
                if (!modelsList.contains(modelName)) {
                    modelsList << modelName;
                    existingBus->setData(0, Qt::UserRole + 1, modelsList);
                }
            }
        }

        // Populate ECUs
        for (const ECU& ecu : model->ecus()) { // Assuming 'ECU' replaces 'Node'
            QTreeWidgetItem* existingECU = nullptr;
            for (int i = 0; i < ecusItem->childCount(); ++i) {
                QTreeWidgetItem* child = ecusItem->child(i);
                if (child->text(0) == ecu.name) {
                    existingECU = child;
                    break;
                }
            }
            if (!existingECU) {
                existingECU = new QTreeWidgetItem(ecusItem, QStringList(ecu.name));
                existingECU->setData(0, Qt::UserRole, "ECU");
                existingECU->setData(0, Qt::UserRole + 1, QStringList() << modelName);
            }
            else {
                QStringList modelsList = existingECU->data(0, Qt::UserRole + 1).toStringList();
                if (!modelsList.contains(modelName)) {
                    modelsList << modelName;
                    existingECU->setData(0, Qt::UserRole + 1, modelsList);
                }
            }


            // Add NodeBuses under ECU
            for (const NodeBus& nodeBus : ecu.buses) {
                QTreeWidgetItem* nodeBusItem = new QTreeWidgetItem(existingECU, QStringList(nodeBus.name));
                nodeBusItem->setData(0, Qt::UserRole, "NodeBus");
                nodeBusItem->setData(0, Qt::UserRole + 1, QStringList() << modelName);

                // Add Transmitted Messages
                QTreeWidgetItem* txMessagesItem = new QTreeWidgetItem(nodeBusItem, QStringList("Transmitted Messages"));
                txMessagesItem->setData(0, Qt::UserRole, "TxMessages");

                for (const TxRxMessage& txMsg : nodeBus.tx) {
                    QTreeWidgetItem* msgItem = new QTreeWidgetItem(txMessagesItem, QStringList(txMsg.name));
                    msgItem->setData(0, Qt::UserRole, "TxMessage");
                    msgItem->setData(0, Qt::UserRole + 1, QStringList() << modelName);
                }

                // Add Received Messages
                QTreeWidgetItem* rxMessagesItem = new QTreeWidgetItem(nodeBusItem, QStringList("Received Messages"));
                rxMessagesItem->setData(0, Qt::UserRole, "RxMessages");

                for (const TxRxMessage& rxMsg : nodeBus.rx) {
                    QTreeWidgetItem* msgItem = new QTreeWidgetItem(rxMessagesItem, QStringList(rxMsg.name));
                    msgItem->setData(0, Qt::UserRole, "RxMessage");
                    msgItem->setData(0, Qt::UserRole + 1, QStringList() << modelName);
                }
            }
        }

        // Populate Messages and Signals
        for (const Message& message : model->messages()) {
            QTreeWidgetItem* existingMessage = nullptr;
            for (int i = 0; i < messagesItem->childCount(); ++i) {
                QTreeWidgetItem* child = messagesItem->child(i);
                if (child->text(0) == message.name) {
                    existingMessage = child;
                    break;
                }
            }
            if (!existingMessage) {
                existingMessage = new QTreeWidgetItem(messagesItem, QStringList(message.name));
                existingMessage->setData(0, Qt::UserRole, "Message");
                existingMessage->setData(0, Qt::UserRole + 1, QStringList() << modelName);
            }
            else {
                QStringList modelsList = existingMessage->data(0, Qt::UserRole + 1).toStringList();
                if (!modelsList.contains(modelName)) {
                    modelsList << modelName;
                    existingMessage->setData(0, Qt::UserRole + 1, modelsList);
                }
            }

            // Add Signals under each message
            for (const Signal& signal : message.data) {
                QTreeWidgetItem* signalItem = new QTreeWidgetItem(existingMessage, QStringList(signal.name));
                signalItem->setData(0, Qt::UserRole, "Signal");
                signalItem->setData(0, Qt::UserRole + 1, QStringList() << modelName);
            }
        }
    }

    // After populating, sort children alphabetically
    QList<QTreeWidgetItem*> topLevelBuses = this->findItems("Buses", Qt::MatchExactly, 0);
    if (!topLevelBuses.isEmpty()) {
        QTreeWidgetItem* busesItem = topLevelBuses.first();
        busesItem->sortChildren(0, Qt::AscendingOrder);
    }

    QList<QTreeWidgetItem*> topLevelECUs = this->findItems("ECUs", Qt::MatchExactly, 0);
    if (!topLevelECUs.isEmpty()) {
        QTreeWidgetItem* ecusItem = topLevelECUs.first();
        ecusItem->sortChildren(0, Qt::AscendingOrder);
    }

    QList<QTreeWidgetItem*> topLevelMessages = this->findItems("Messages", Qt::MatchExactly, 0);
    if (!topLevelMessages.isEmpty()) {
        QTreeWidgetItem* messagesItem = topLevelMessages.first();
        messagesItem->sortChildren(0, Qt::AscendingOrder);
    }
}
