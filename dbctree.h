#ifndef DBCTREE_H
#define DBCTREE_H

#include <QTreeWidget>
#include <QSet>

class DbcTree : public QTreeWidget {
    Q_OBJECT

public:
    // Constructor
    DbcTree(QWidget *parent = nullptr);

    // Destructor
    ~DbcTree();

protected:
    // Event handler declarations
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
};

#endif // DBCTREE_H
