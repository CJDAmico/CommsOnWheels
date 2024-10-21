#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "./dbctree.h"
#include <QMainWindow>
#include <QTreeWidget>
#include <QFormLayout>
#include <QLineEdit>
#include <QTabWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QMessageBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QSplitter>
#include <QStyleFactory>
#include <QProcess>
#include <QFileDialog>
#include <QDir>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    // Window Icon
    // TODO: Determine icon, set to a default one for now
    this->setWindowIcon(QIcon(":/icons/windowIcon.svg"));

    // Central Widget
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // ------------------- Menu Bar -----------------------
    QMenuBar *menuBar = this->menuBar();

    // File Menu
    QMenu *fileMenu = menuBar->addMenu("File");

    // File Actions
    QAction *openAction = new QAction("Open", this);
    QAction *exitAction = new QAction("Exit", this);
    fileMenu->addAction(openAction);
    fileMenu->addAction(exitAction);
    // Connect File Actions
    connect(openAction, &QAction::triggered, this, []() {
        QString selectedFile = QFileDialog::getOpenFileName(
            nullptr,
            "Open DBC File",
            QDir::currentPath(),
            "DBC Files (*.dbc)"
            );
        if (!selectedFile.isEmpty()) {
            // Run Python Script to Convert DBC to JSON
            QProcess process;
            QStringList scriptParameters;
            scriptParameters << QDir::currentPath() + "/convert.py";
            scriptParameters << QDir::currentPath();
            scriptParameters << selectedFile;
            process.start("python3", scriptParameters);
            process.waitForFinished();

            // Inform the user the import was succesful
            QMessageBox::information(nullptr, "Import Successful", "DBC File Imported:\n" + selectedFile);
        }
    });
    connect(exitAction, &QAction::triggered, this, &MainWindow::close);

    // Edit Menu
    QMenu *editMenu = menuBar->addMenu("Edit");

    // Edit Actions
    QAction *undoAction = new QAction("Undo", this);
    QAction *redoAction = new QAction("Redo", this);
    editMenu->addAction(undoAction);
    editMenu->addAction(redoAction);

    // View Menu
    QMenu *viewMenu = menuBar->addMenu("View");

    // View Actions
    QAction *hexadecimalMode = new QAction("Hexadecimal", this);
    QAction *binaryMode = new QAction("Binary", this);
    QAction *decimalMode = new QAction("Decimal", this);
    this->styleMode = new QAction("Dark Mode", this);
    viewMenu->addAction(hexadecimalMode);
    viewMenu->addAction(binaryMode);
    viewMenu->addAction(decimalMode);
    viewMenu->addAction(styleMode);
    connect(styleMode, &QAction::triggered, this, &MainWindow::toggleDarkMode);  // Connect the button to the toggleDarkMode function

    // Help Menu
    QMenu *helpMenu = menuBar->addMenu("Help");
    QAction *guide = new QAction("Guide", this);
    helpMenu->addAction(guide);


    // ------------------- Left Panel -----------------------
    QWidget *leftPanel = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout;

    QTreeWidget *tree = new DbcTree(this);
    tree->setHeaderHidden(true);  // Hide headers if not needed
    // Enable drag and drop
    tree->setDragDropMode(QAbstractItemView::InternalMove);
    tree->setSelectionMode(QAbstractItemView::SingleSelection);

    // Initialize Tree Items
    // TODO: Dynamically create the nodes based on the imported DBC JSON.
    // TODO: Look into warnings about memory leak risk for TreeWidgetItems
    QTreeWidgetItem *buses = new QTreeWidgetItem(tree, QStringList("Buses"));
    QTreeWidgetItem *chassisBus = new QTreeWidgetItem(buses, QStringList("Chassis"));
    QTreeWidgetItem *chassisMessages = new QTreeWidgetItem(chassisBus, QStringList("Messages"));
    QTreeWidgetItem *chassisNodes = new QTreeWidgetItem(chassisBus, QStringList("Nodes"));
    QTreeWidgetItem *chassisSend = new QTreeWidgetItem(chassisBus, QStringList("Send Messages"));
    QTreeWidgetItem *chassisReceive = new QTreeWidgetItem(chassisBus, QStringList("Receive Messages"));
    QTreeWidgetItem *chassisSignals = new QTreeWidgetItem(chassisBus, QStringList("Signals"));

    QTreeWidgetItem *engineBus = new QTreeWidgetItem(buses, QStringList("Engine"));
    QTreeWidgetItem *engineMessages = new QTreeWidgetItem(engineBus, QStringList("Messages"));
    QTreeWidgetItem *engineNodes = new QTreeWidgetItem(engineBus, QStringList("Nodes"));
    QTreeWidgetItem *engineSend = new QTreeWidgetItem(engineBus, QStringList("Send Messages"));
    QTreeWidgetItem *engineReceive = new QTreeWidgetItem(engineBus, QStringList("Receive Messages"));
    QTreeWidgetItem *engineSignals = new QTreeWidgetItem(engineBus, QStringList("Signals"));

    // Search Bar
    QLineEdit *searchBar = new QLineEdit;
    searchBar->setPlaceholderText("Search...");


    leftLayout->addWidget(searchBar);
    leftLayout->addWidget(tree);
    leftPanel->setLayout(leftLayout);


    // ------------------- Right Panel -----------------------
    QTabWidget *rightPanel = new QTabWidget;

    // Definition Tab
    QWidget *definitionTab = new QWidget;
    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow("PGN:", new QSpinBox);
    formLayout->addRow("Name:", new QLineEdit);
    formLayout->addRow("Desc:", new QLineEdit);
    formLayout->addRow("Priority:", new QSpinBox);
    formLayout->addRow("Length:", new QSpinBox);
    formLayout->addRow("Extended Data Page:", new QCheckBox);
    formLayout->addRow("Data Page:", new QCheckBox);

    // Attributes Table
    formLayout->addRow(new QLabel("Additional Attributes:"));
    QTableWidget *attributesTable = new QTableWidget;
    attributesTable->setColumnCount(3);  // 3 columns: Name, Type, Value
    attributesTable->setHorizontalHeaderLabels({"Name", "Type", "Value"});
    attributesTable->horizontalHeader()->setStretchLastSection(true);

    // TODO: Replace dynamically with message attributes instead of placeholders
    addAttributeRow(attributesTable, {"TxPeriodicity", "Integer", "1000"});
    addAttributeRow(attributesTable, {"TxOnChange", "Bool", "True"});
    formLayout->addRow(attributesTable);

    QHBoxLayout *attributeButtons = new QHBoxLayout;
    QPushButton *atttributeDeleteButton = new QPushButton("Delete");
    QPushButton *attributeAddButton = new QPushButton("Add");
    attributeButtons->addWidget(atttributeDeleteButton);
    attributeButtons->addWidget(attributeAddButton);
    formLayout->addRow(attributeButtons);

    // Create the Signals list
    formLayout->addRow(new QLabel("Signals:"));
    QListWidget *signalsList = new QListWidget;
    // TODO: Populate based on message signals
    signalsList->addItem("EngineOverrideCtrlMode");
    formLayout->addRow(signalsList);

    // Add signal buttons
    QHBoxLayout *signalButtons = new QHBoxLayout;
    QPushButton *signalEditButton = new QPushButton("Edit");
    QPushButton *signalDeleteButton = new QPushButton("Delete");
    QPushButton *signalAddButton = new QPushButton("Add");
    signalButtons->addWidget(signalEditButton);
    signalButtons->addWidget(signalDeleteButton);
    signalButtons->addWidget(signalAddButton);

    // Add the buttons layout below the Signals list
    formLayout->addRow(signalButtons);

    // Add tabs panel widget
    rightPanel->addTab(definitionTab, "Definition");

    // Add inputs form to right tab
    definitionTab->setLayout(formLayout);

    // ------------------- Center Layout -----------------------
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(leftPanel);
    splitter->addWidget(rightPanel);

    QVBoxLayout *mainLayout = new QVBoxLayout;

    mainLayout->addWidget(splitter);
    centralWidget->setLayout(mainLayout);
}


// Helper function to add a row to the attributes table
void MainWindow::addAttributeRow(QTableWidget *table, const QStringList &rowData) {
    int row = table->rowCount();
    table->insertRow(row);  // Insert a new row

    for (int i = 0; i < rowData.size(); ++i) {
        table->setItem(row, i, new QTableWidgetItem(rowData[i]));  // Add data to the row
    }
}

void MainWindow::toggleDarkMode() {
    static bool darkModeEnabled = false;


    if (darkModeEnabled) {
        // Revert to light mode
        qApp->setPalette(defaultPalette);
        this->setStyleSheet(defaultStyleSheet);
        this->menuBar()->setStyleSheet("");

        this->styleMode->setText("Dark Mode");
        darkModeEnabled = false;
    } else {
        defaultPalette = QApplication::palette();
        defaultStyleSheet = this->styleSheet();

        // Apply dark mode
        QPalette darkPalette;
        // Window and background colors
        darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        darkPalette.setColor(QPalette::Base, QColor(42, 42, 42));
        darkPalette.setColor(QPalette::AlternateBase, QColor(66, 66, 66));
        darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
        darkPalette.setColor(QPalette::ToolTipText, Qt::white);

        // Text and buttons
        darkPalette.setColor(QPalette::Text, Qt::white);
        darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        darkPalette.setColor(QPalette::BrightText, Qt::red);
        darkPalette.setColor(QPalette::PlaceholderText, Qt::white);

        // Highlighted text and selections
        darkPalette.setColor(QPalette::Highlight, Qt::white);
        darkPalette.setColor(QPalette::HighlightedText, Qt::black);

        // Set Menu Bar to dark style
        this->menuBar()->setStyleSheet("QMenuBar { background-color: #353535; color: white; } "
                                       "QMenuBar::item { background-color: #353535; color: white; } "
                                       "QMenuBar::item:selected { background-color: #454545; } "
                                       "QMenuBar::item:pressed { background-color: #252525; } "
                                       "QMenu { background-color: #353535; color: white; } "
                                       "QMenu::item:selected { background-color: #454545; }");

        this->styleMode->setText("Light Mode");
        qApp->setPalette(darkPalette);

        QString newStyleSheet = "QTableView::item:selected { background-color: #55aaff; color: black; } "
                                "QListView::item:selected { background-color: #55aaff; color: black; } "
                                "QTreeView::item:selected { background-color: #55aaff; color: black; } "
                                "QTreeView { outline: 0; }"
                                "QListView { outline: 0; }"
                                "QTableView { outline: 0; }";

        this->setStyleSheet(this->styleSheet() + newStyleSheet);
        darkModeEnabled = true;
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

