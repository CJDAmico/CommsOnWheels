#ifndef DBCTREE_H
#define DBCTREE_H

#include <QTreeWidget>
#include <QSet>
#include "dbcdata.h"

class DbcTree : public QTreeWidget {
    Q_OBJECT

    public:
        // Constructor
        DbcTree(QWidget *parent = nullptr);

        // Destructor
        ~DbcTree();

        void populateTree(const QList<DbcDataModel*>& models);

    protected:
        // Event handler declarations
        void dragMoveEvent(QDragMoveEvent *event) override;
        void dropEvent(QDropEvent *event) override;

        // Used for alphabetical sorting
        void sortChildItems(QTreeWidgetItem* parentItem);
        QList<DbcDataModel*> m_models;

};

#endif // DBCTREE_H
