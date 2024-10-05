#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>

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

private:
    Ui::MainWindow *ui;

    void addAttributeRow(QTableWidget *table, const QStringList &rowData);
    void toggleDarkMode();
    QAction *styleMode;
    QPalette defaultPalette;
    QString defaultStyleSheet;
};
#endif // MAINWINDOW_H
