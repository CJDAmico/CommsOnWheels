#include "dbctree.h"
#include <QTreeWidgetItem>
#include <QDragMoveEvent>
#include <QDropEvent>

// Constructor implementation
DbcTree::DbcTree(QWidget *parent)
    : QTreeWidget(parent) {
    setUniformRowHeights(true);
    setDragDropMode(QAbstractItemView::InternalMove);  // Enable internal drag-and-drop reordering
    setStyleSheet(R"(
        QTreeWidget::item {
            padding: 10px; /* Add padding around the text */
        }
        QTreeWidget {
            font-size: 16px;  /* Increase font size */
        }
    )");
    expandAll();
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
