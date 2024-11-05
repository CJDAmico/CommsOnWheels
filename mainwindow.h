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
    void displayBitLayout(const Message &message);

    // Left and Right Tab Splitter
    QSplitter *splitter;

    // Right panel widgets
    QTabWidget *rightPanel;

    // Tabs
    QWidget *definitionTab;
    QWidget *transmittersTab;
    QWidget *receiversTab;
    QWidget *networkTab;
    QWidget *nodeTab;
    QWidget *signalTab;
    QWidget *layoutTab;

    // Widgets for Definition Tab (used for Message)
    QFormLayout *definitionFormLayout;
    QLineEdit *pgnLineEdit;
    QLineEdit *nameLineEdit;
    QLineEdit *descLineEdit;
    QSpinBox *prioritySpinBox;
    QSpinBox *lengthSpinBox;
    QCheckBox *extendedDataPageCheckBox;
    QCheckBox *dataPageCheckBox;
    QTableWidget *messageAttributesTable;
    QListWidget *signalsList;

    // Widgets for transmitters and receivers tabs
    QFormLayout *transmittersFormLayout;
    QFormLayout *receiversFormLayout;
    QTableWidget *transmittersTable;
    QTableWidget *receiversTable;

    // Widgets for layout tab
    QFormLayout *layoutFormLayout;
    QTableWidget *bitGrid;
    // 64 possible colors for signals in the layout tab, circular array accessed by using colors index
    int colorsIndex;
    const std::vector<QColor> signalColors = {
        QColor(255, 182, 193), QColor(255, 228, 225), QColor(255, 240, 245), QColor(255, 192, 203),
        QColor(240, 128, 128), QColor(255, 218, 185), QColor(255, 239, 213), QColor(255, 222, 173),
        QColor(245, 222, 179), QColor(255, 250, 205), QColor(250, 250, 210), QColor(255, 255, 224),
        QColor(240, 230, 140), QColor(238, 232, 170), QColor(230, 230, 250), QColor(216, 191, 216),
        QColor(221, 160, 221), QColor(238, 130, 238), QColor(218, 112, 214), QColor(199, 21, 133),
        QColor(255, 160, 122), QColor(255, 182, 193), QColor(255, 228, 225), QColor(255, 250, 240),
        QColor(240, 255, 240), QColor(245, 245, 220), QColor(255, 245, 238), QColor(255, 228, 181),
        QColor(255, 239, 213), QColor(245, 222, 179), QColor(240, 230, 140), QColor(238, 232, 170),
        QColor(250, 240, 230), QColor(255, 228, 225), QColor(255, 240, 245), QColor(255, 245, 238),
        QColor(255, 228, 181), QColor(255, 235, 205), QColor(255, 239, 213), QColor(245, 222, 179),
        QColor(255, 250, 240), QColor(240, 255, 240), QColor(240, 255, 255), QColor(240, 248, 255),
        QColor(240, 255, 240), QColor(245, 245, 245), QColor(255, 250, 250), QColor(255, 228, 225),
        QColor(255, 228, 196), QColor(255, 228, 181), QColor(250, 250, 210), QColor(250, 235, 215),
        QColor(255, 239, 213), QColor(245, 245, 220), QColor(255, 245, 238), QColor(255, 228, 181),
        QColor(255, 240, 245), QColor(255, 248, 220), QColor(253, 245, 230), QColor(250, 235, 215),
        QColor(245, 245, 245), QColor(255, 250, 250), QColor(255, 250, 205), QColor(245, 255, 250)
    };

    // Widgets for network Tab
    QFormLayout *networkFormLayout;
    QLineEdit *networkNameLineEdit;
    QLineEdit *baudRateLineEdit;
    QTableWidget *networkAttributesTable;

    // Widgets for node Tab
    QFormLayout* nodeFormLayout;
    QLineEdit* nodeNameLineEdit;
    QTableWidget *nodeAddressTable;
    QTableWidget *nodeAttributesTable;

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
    QTableWidget *signalAttributesTable;


};
#endif // MAINWINDOW_H
