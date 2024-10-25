#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include "dbcdata.h"
#include "dbctree.h"
#include <QFormLayout>
#include <QSpinBox>
#include <QCheckBox>
#include <QListWidget>
#include <QSplitter>
#include <QLineEdit>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onTreeItemClicked(QTreeWidgetItem* item, int column);
    // Handle selection of node types to open tabs
    void handleMessageItem(QTreeWidgetItem* item, const QString& name, const QStringList& models);
    void handleSignalItem(QTreeWidgetItem* item, const QString& name, const QStringList& models);
    void handleNetworkItem(QTreeWidgetItem* item, const QString& name, const QStringList& models);
    void handleNodeItem(QTreeWidgetItem* item, const QString& name, const QStringList& models);

private:
    Ui::MainWindow *ui;
    QList<DbcDataModel*> dbcModels;
    DbcTree* dbcTree;

    void addAttributeRow(QTableWidget *table, const QStringList &rowData);
    // void toggleDarkMode();
    // QAction *styleMode;
    // QPalette defaultPalette;
    // QString defaultStyleSheet;

    void updateDbcTree();
    void setupRightPanel();
    void clearRightPanel();

    // Left and Right Tab Splitter
    QSplitter *splitter;

    // Right panel widgets
    QTabWidget *rightPanel;

    // Tabs
    QWidget *definitionTab;
    QWidget *networkTab;
    QWidget *nodeTab;
    QWidget *signalTab;

    // Widgets for Definition Tab (used for Message)
    QFormLayout *definitionFormLayout;
    QLineEdit *pgnLineEdit;
    QLineEdit *nameLineEdit;
    QLineEdit *descLineEdit;
    QSpinBox *prioritySpinBox;
    QSpinBox *lengthSpinBox;
    QCheckBox *extendedDataPageCheckBox;
    QCheckBox *dataPageCheckBox;
    QTableWidget *attributesTable;
    QListWidget *signalsList;

    // Widgets for network Tab
    QFormLayout *networkFormLayout;
    QLineEdit *networkNameLineEdit;
    QLineEdit *baudRateLineEdit;

    // Widgets for node Tab
    QFormLayout* nodeFormLayout;
    QLineEdit* nodeNameLineEdit;

    // Widgets for Signal Tab
    QFormLayout *signalFormLayout;
    QSpinBox *spnSpinBox;
    QLineEdit *signalNameLineEdit;
    QLineEdit *signalDescLineEdit;
    QSpinBox *startBitSpinBox;
    QSpinBox *bitLengthSpinBox;
    QCheckBox *isBigEndianCheckBox;
    QCheckBox *isTwosComplementCheckBox;
    QDoubleSpinBox *factorSpinBox;
    QDoubleSpinBox *offsetSpinBox;
    QLineEdit *unitsLineEdit;
    QTableWidget *enumerationsTable;


};
#endif // MAINWINDOW_H
