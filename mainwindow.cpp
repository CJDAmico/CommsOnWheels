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
#include <QSettings>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow), dbcTree(new DbcTree()) {
    // Window Icon
    // TODO: Determine icon, set to a default one for now
    this->setWindowIcon(QIcon(":/icons/windowIcon.svg"));

    // Central Widget
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Set up Tree item selection
    connect(dbcTree, &QTreeWidget::itemClicked, this, &MainWindow::onTreeItemClicked);

    // Set up Right Panels
    setupRightPanel();

    // ------------------- Menu Bar -----------------------
    QMenuBar *menuBar = this->menuBar();

    // File Menu
    QMenu *fileMenu = menuBar->addMenu("File");

    // File Actions
    QAction *newJson = new QAction("New File", this);
    fileMenu->addAction(newJson);
    connect(newJson, &QAction::triggered, this, [this](){
        dbcModels.clear();
        updateDbcTree();
    });

    QAction *openJson = new QAction("Import JSON...", this);
    fileMenu->addAction(openJson);
    // Connect File Actions
    connect(openJson, &QAction::triggered, this, [this]() {
        QSettings settings("Oshkosh", "HeavyInsight");

        QString lastDir = settings.value("lastWorkingDir", QDir::currentPath()).toString();

        QString selectedFile = QFileDialog::getOpenFileName(
            nullptr,
            "Import JSON File",
            lastDir,
            "JSON Files (*.json)"
            );
        if (!selectedFile.isEmpty()) {
            // Store last directory used
            QFileInfo fileInfo(selectedFile);
            QString selectedDir = fileInfo.absolutePath();
            settings.setValue("lastWorkingDir", selectedDir);

            // Load the JSON file into the data model
            DbcDataModel* newModel = new DbcDataModel();
            newModel->setFileName(fileInfo.fileName());
            if (newModel->loadJson(selectedFile)) {
                dbcModels.append(newModel);
                updateDbcTree();
            } else {
                QMessageBox::warning(this, "Import Error", "Failed to import JSON file.");
                delete newModel;
            }

            // Inform the user the open was succesful
            QMessageBox::information(this, "Import Successful", "JSON File Imported:\n" + selectedFile);
        }
    });

    QAction *importDBC = new QAction("Import DBC...", this);
    fileMenu->addAction(importDBC);
    connect(importDBC, &QAction::triggered, this, [this]() {
        QSettings settings("Oshkosh", "HeavyInsight");

        QString lastDir = settings.value("lastWorkingDir", QDir::currentPath()).toString();

        QString selectedFile = QFileDialog::getOpenFileName(
            nullptr,
            "Import DBC File",
            lastDir,
            "DBC Files (*.dbc)"
            );
        if (!selectedFile.isEmpty()) {
            // Store last directory used
            QFileInfo fileInfo(selectedFile);
            QString selectedDir = fileInfo.absolutePath();
            settings.setValue("lastWorkingDir", selectedDir);

            // Import the DBC file into the data model
            DbcDataModel* newModel = new DbcDataModel();
            newModel->setFileName(fileInfo.fileName());

            // TODO: Implement importDBC() in DbcDataModel
            if (newModel->importDBC(selectedFile)) { // Pass the selected DBC file path
                dbcModels.append(newModel);
                updateDbcTree();

                // Inform the user the import was successful
                QMessageBox::information(this, "Import Successful", "DBC File Imported:\n" + selectedFile);
            } else {
                QMessageBox::warning(this, "Import Error", "Failed to import DBC file.");
                delete newModel;
            }
        }
    });

    QAction *save = new QAction("Save", this);
    QAction *saveAs = new QAction("Save As...", this);
    fileMenu->addAction(save);
    fileMenu->addAction(saveAs);

    connect(save, &QAction::triggered, this, [this]() {
        // Define the JSON directory path
        QString jsonDirPath = "./JSON";

        // Ensure the JSON directory exists
        QDir jsonDir;
        if (!jsonDir.exists(jsonDirPath)) {
            jsonDir.mkpath(jsonDirPath);
        }

        // Generate a unique default file name (e.g., based on current timestamp)
        QString defaultFileName = QString("DbcData_%1.json")
                                      .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));

        // Construct the full file path
        QString filePath = jsonDirPath + QDir::separator() + defaultFileName;

        // TODO: Convert the current list of DbcDataModels into a single JSON
        // QJsonDocument jsonDoc; // Placeholder for the JSON document

        // TODO: Save the JSON document to the specified file path
        // Example:
        // QFile file(filePath);
        // if (file.open(QIODevice::WriteOnly)) {
        //     file.write(jsonDoc.toJson());
        //     file.close();
        // }
    });

    connect(saveAs, &QAction::triggered, this, [this]() {
        // Open a QFileDialog to let the user select the save location and file name
        QString selectedFilePath = QFileDialog::getSaveFileName(
            this,
            tr("Save DBC Data As"),
            "./JSON/DbcData.json", // Default file name and directory
            tr("JSON Files (*.json);;All Files (*)")
            );

        // Check if the user selected a file
        if (!selectedFilePath.isEmpty()) {
            // TODO: Convert the current list of DbcDataModels into a single JSON
            // QJsonDocument jsonDoc; // Placeholder for the JSON document

            // TODO: Save the JSON document to the selected file path
            // Example:
            // QFile file(selectedFilePath);
            // if (file.open(QIODevice::WriteOnly)) {
            //     file.write(jsonDoc.toJson());
            //     file.close();
            // }
        }
    });


    QAction *exitAction = new QAction("Exit", this);
    connect(exitAction, &QAction::triggered, this, &MainWindow::close);
    fileMenu->addAction(exitAction);

    // Edit Menu
    QMenu *editMenu = menuBar->addMenu("Edit");

    // Edit Actions
    QAction *undoAction = new QAction("Undo", this);
    QAction *redoAction = new QAction("Redo", this);
    // TODO: Take snapshots of the data before and after any change so that the user can undo or redo
    editMenu->addAction(undoAction);
    editMenu->addAction(redoAction);

    // View Menu
    QMenu *viewMenu = menuBar->addMenu("View");

    // View Actions
    // TODO: Look into these modes
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
    // TODO: Create a documentation/guide
    QMenu *helpMenu = menuBar->addMenu("Help");
    QAction *guide = new QAction("Guide", this);
    helpMenu->addAction(guide);


    // ------------------- Left Panel -----------------------
    QWidget *leftPanel = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout;

    // Search Bar
    QLineEdit *searchBar = new QLineEdit;
    searchBar->setPlaceholderText("Search...");


    leftLayout->addWidget(searchBar);
    leftLayout->addWidget(dbcTree);
    leftPanel->setLayout(leftLayout);


    // ------------------- Center Layout -----------------------
    splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(leftPanel);
    splitter->addWidget(rightPanel);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    splitter->updateGeometry();

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

void MainWindow::updateDbcTree()
{
    dbcTree->populateTree(dbcModels);
}

void MainWindow::onTreeItemClicked(QTreeWidgetItem* item, int column)
{
    if (!item) return;

    QString itemType = item->data(0, Qt::UserRole).toString();
    QString name = item->text(0);
    QStringList models = item->data(0, Qt::UserRole + 1).toStringList();

    // Clear existing tabs
    clearRightPanel();

    if (itemType == "Message" || itemType == "TxMessage" || itemType == "RxMessage") {
        handleMessageItem(item, name, models);
    }
    else if (itemType == "Signal") {
        handleSignalItem(item, name, models);
    }
    else if (itemType == "Bus") {
        handleBusItem(item, name, models);
    }
    else if (itemType == "ECU") {
        handleECUItem(item, name, models);
    }
    else if(itemType == "NodeBus") {
        // TODO: Show Bus info?
    }
    else {
        QMessageBox::information(this, "Info", "Please select a valid item.");
    }

    // Adjust splitter if needed
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    splitter->updateGeometry();

    int totalWidth = splitter->width();
    if (totalWidth > 0) {
        QList<int> sizes;
        sizes << totalWidth / 2 << totalWidth / 2;
        splitter->setSizes(sizes);
    }
}

void MainWindow::handleMessageItem(QTreeWidgetItem* item, const QString& name, const QStringList& models)
{
    if (models.isEmpty()) {
        QMessageBox::warning(this, "Error", "No associated models found for this message.");
        return;
    }

    QString modelFileName = models.first();
    DbcDataModel* model = nullptr;
    for (DbcDataModel* m : dbcModels) {
        if (m->fileName() == modelFileName) {
            model = m;
            break;
        }
    }

    if (!model) {
        QMessageBox::warning(this, "Error", "Model not found.");
        return;
    }

    QString itemType = item->data(0, Qt::UserRole).toString();
    const Message* message = nullptr;
    QString txRxType; // "Transmitted" or "Received"

    // Directly search for the message by the item's own name
    if (itemType == "Message" || itemType == "TxMessage" || itemType == "RxMessage") {
        if (itemType == "TxMessage" || itemType == "RxMessage") {
            txRxType = (itemType == "TxMessage") ? "Transmitted" : "Received";
        }

        for (const Message& msg : model->messages()) {
            if (msg.name.trimmed().compare(name.trimmed(), Qt::CaseInsensitive) == 0) {
                message = &msg;
                break;
            }
        }
    }

    if (!message) {
        QMessageBox::warning(this, "Error", "Message not found in the model.");
        return;
    }

    // Populate Definition tab
    pgnLineEdit->setText(QString::number(message->pgn));
    QString displayName = message->name;
    if (!txRxType.isEmpty()) {
        displayName += QString(" (%1)").arg(txRxType); // e.g., "Message1 (Transmitted)"
    }
    nameLineEdit->setText(displayName);
    descLineEdit->setText(message->description);
    prioritySpinBox->setValue(message->priority);
    lengthSpinBox->setValue(message->length);
    extendedDataPageCheckBox->setChecked(false);
    dataPageCheckBox->setChecked(false);

    // Update attributes table
    attributesTable->setRowCount(0);
    addAttributeRow(attributesTable, {"TxPeriodicity", "Integer", QString::number(message->txPeriodicity)});
    addAttributeRow(attributesTable, {"TxOnChange", "Bool", message->txOnChange ? "True" : "False"});

    // Update signals list
    signalsList->clear();
    for (const Signal& signal : message->data) {
        signalsList->addItem(signal.name);
    }

    // Show the Definition tab
    if (rightPanel->indexOf(definitionTab) == -1) {
        rightPanel->addTab(definitionTab, "Definition");
    }

    rightPanel->setCurrentWidget(definitionTab);
}

void MainWindow::handleSignalItem(QTreeWidgetItem* item, const QString& name, const QStringList& models)
{
    if (models.isEmpty()) {
        QMessageBox::warning(this, "Error", "No associated models found for this signal.");
        return;
    }

    QString modelFileName = models.first();
    DbcDataModel* model = nullptr;
    for (DbcDataModel* m : dbcModels) {
        if (m->fileName() == modelFileName) {
            model = m;
            break;
        }
    }

    if (!model) {
        QMessageBox::warning(this, "Error", "Model not found.");
        return;
    }

    // Find parent message
    QTreeWidgetItem* parentItem = item->parent();
    if (!parentItem) {
        QMessageBox::warning(this, "Error", "Parent message not found.");
        return;
    }
    QString messageName = parentItem->text(0);

    const Message* message = nullptr;
    for (const Message& msg : model->messages()) {
        if (msg.name == messageName) {
            message = &msg;
            break;
        }
    }

    if (!message) {
        QMessageBox::warning(this, "Error", "Parent message not found in the model.");
        return;
    }

    const Signal* signal = nullptr;
    for (const Signal& sig : message->data) {
        if (sig.name == name) {
            signal = &sig;
            break;
        }
    }

    if (!signal) {
        QMessageBox::warning(this, "Error", "Signal not found in the message.");
        return;
    }

    // Populate Signal tab
    spnSpinBox->setValue(signal->spn);
    signalNameLineEdit->setText(signal->name);
    signalDescLineEdit->setText(signal->description);
    startBitSpinBox->setValue(signal->startBit);
    bitLengthSpinBox->setValue(signal->bitLength);
    isBigEndianCheckBox->setChecked(signal->isBigEndian);
    isTwosComplementCheckBox->setChecked(signal->isTwosComplement);
    factorSpinBox->setValue(signal->factor);
    offsetSpinBox->setValue(signal->offset);
    unitsLineEdit->setText(signal->units);

    // Update enumerations table
    enumerationsTable->setRowCount(0);
    for (const Enumeration& enumVal : signal->enumerations) {
        int row = enumerationsTable->rowCount();
        enumerationsTable->insertRow(row);
        enumerationsTable->setItem(row, 0, new QTableWidgetItem(enumVal.name));
        enumerationsTable->setItem(row, 1, new QTableWidgetItem(enumVal.description));
        enumerationsTable->setItem(row, 2, new QTableWidgetItem(QString::number(enumVal.value)));
    }

    // Show the Signal tab
    if (rightPanel->indexOf(signalTab) == -1) {
        rightPanel->addTab(signalTab, "Signal");
    }

    rightPanel->setCurrentWidget(signalTab);
}

// Handler: Bus Items
void MainWindow::handleBusItem(QTreeWidgetItem* item, const QString& name, const QStringList& models)
{
    // Assuming you have only one model for simplicity
    // If multiple models, decide how to display or aggregate data

    if (models.isEmpty()) {
        QMessageBox::warning(this, "Error", "No associated models found for this bus.");
        return;
    }

    // For demonstration, use the first model
    QString modelFileName = models.first();

    // Find the model by file name
    DbcDataModel* model = nullptr;
    for (DbcDataModel* m : dbcModels) {
        if (m->fileName() == modelFileName) {
            model = m;
            break;
        }
    }

    if (!model) {
        QMessageBox::warning(this, "Error", "Model not found.");
        return;
    }

    // Find the bus in the model
    const Bus* bus = nullptr;
    for (const Bus& b : model->buses()) {
        if (b.name == name) {
            bus = &b;
            break;
        }
    }

    if (!bus) {
        QMessageBox::warning(this, "Error", "Bus not found in the model.");
        return;
    }

    // Populate Bus tab with bus data
    busNameLineEdit->setText(bus->name);
    baudRateLineEdit->setText(bus->baud);

    // Show the Bus tab
    if (rightPanel->indexOf(busTab) == -1) {
        rightPanel->addTab(busTab, "Bus");
    }

    rightPanel->setCurrentWidget(busTab);
}


// Handler: ECU Items
void MainWindow::handleECUItem(QTreeWidgetItem* item, const QString& name, const QStringList& models)
{
    if (models.isEmpty()) {
        QMessageBox::warning(this, "Error", "No associated models found for this ECU.");
        return;
    }

    QString modelFileName = models.first();

    // Find the model by file name
    DbcDataModel* model = nullptr;
    for (DbcDataModel* m : dbcModels) {
        if (m->fileName() == modelFileName) {
            model = m;
            break;
        }
    }

    if (!model) {
        QMessageBox::warning(this, "Error", "Model not found.");
        return;
    }

    // Find the ECU in the model
    const ECU* ecu = nullptr;
    for (const ECU& n : model->ecus()) {
        if (n.name == name) {
            ecu = &n;
            break;
        }
    }

    if (!ecu) {
        QMessageBox::warning(this, "Error", "ECU not found in the model.");
        return;
    }

    // Populate ECU tab
    ecuNameLineEdit->setText(ecu->name);

    // Show the ECU tab
    if (rightPanel->indexOf(ecuTab) == -1) {
        rightPanel->addTab(ecuTab, "ECU");
    }

    rightPanel->setCurrentWidget(ecuTab);
}



void MainWindow::setupRightPanel()
{
    rightPanel = new QTabWidget;

    // Definition Tab (for Messages)
    definitionTab = new QWidget;
    definitionFormLayout = new QFormLayout;

    // Initialize widgets
    pgnLineEdit = new QLineEdit;
    nameLineEdit = new QLineEdit;
    descLineEdit = new QLineEdit;
    prioritySpinBox = new QSpinBox;
    lengthSpinBox = new QSpinBox;
    extendedDataPageCheckBox = new QCheckBox;
    dataPageCheckBox = new QCheckBox;
    attributesTable = new QTableWidget;
    signalsList = new QListWidget;

    // Set up the Definition Form Layout
    definitionFormLayout->addRow("PGN:", pgnLineEdit);
    definitionFormLayout->addRow("Name:", nameLineEdit);
    definitionFormLayout->addRow("Description:", descLineEdit);
    definitionFormLayout->addRow("Priority:", prioritySpinBox);
    definitionFormLayout->addRow("Length:", lengthSpinBox);
    definitionFormLayout->addRow("Extended Data Page:", extendedDataPageCheckBox);
    definitionFormLayout->addRow("Data Page:", dataPageCheckBox);
    definitionFormLayout->addRow(new QLabel("Additional Attributes:"));
    attributesTable->setColumnCount(3);
    attributesTable->setHorizontalHeaderLabels({"Name", "Type", "Value"});
    attributesTable->horizontalHeader()->setStretchLastSection(true);
    definitionFormLayout->addRow(attributesTable);
    definitionFormLayout->addRow(new QLabel("Signals:"));
    definitionFormLayout->addRow(signalsList);

    definitionTab->setLayout(definitionFormLayout);

    // Bus Tab
    busTab = new QWidget;
    busFormLayout = new QFormLayout;
    busNameLineEdit = new QLineEdit;
    baudRateLineEdit = new QLineEdit;
    busFormLayout->addRow("Bus Name:", busNameLineEdit);
    busFormLayout->addRow("Baud Rate:", baudRateLineEdit);
    busTab->setLayout(busFormLayout);

    // ECU Tab
    ecuTab = new QWidget;
    ecuFormLayout = new QFormLayout;
    ecuNameLineEdit = new QLineEdit;
    ecuFormLayout->addRow("ECU Name:", ecuNameLineEdit);
    ecuTab->setLayout(ecuFormLayout);

    // Signal Tab
    signalTab = new QWidget;
    signalFormLayout = new QFormLayout;
    spnSpinBox = new QSpinBox;
    signalNameLineEdit = new QLineEdit;
    signalDescLineEdit = new QLineEdit;
    startBitSpinBox = new QSpinBox;
    bitLengthSpinBox = new QSpinBox;
    isBigEndianCheckBox = new QCheckBox;
    isTwosComplementCheckBox = new QCheckBox;
    factorSpinBox = new QDoubleSpinBox;
    offsetSpinBox = new QDoubleSpinBox;
    unitsLineEdit = new QLineEdit;
    enumerationsTable = new QTableWidget;

    signalFormLayout->addRow("SPN:", spnSpinBox);
    signalFormLayout->addRow("Name:", signalNameLineEdit);
    signalFormLayout->addRow("Description:", signalDescLineEdit);
    signalFormLayout->addRow("Start Bit:", startBitSpinBox);
    signalFormLayout->addRow("Bit Length:", bitLengthSpinBox);
    signalFormLayout->addRow("Is Big Endian:", isBigEndianCheckBox);
    signalFormLayout->addRow("Is Two's Complement:", isTwosComplementCheckBox);
    signalFormLayout->addRow("Factor:", factorSpinBox);
    signalFormLayout->addRow("Offset:", offsetSpinBox);
    signalFormLayout->addRow("Units:", unitsLineEdit);
    signalFormLayout->addRow(new QLabel("Enumerations:"));
    enumerationsTable->setColumnCount(3);
    enumerationsTable->setHorizontalHeaderLabels({"Name", "Description", "Value"});
    enumerationsTable->horizontalHeader()->setStretchLastSection(true);
    signalFormLayout->addRow(enumerationsTable);

    signalTab->setLayout(signalFormLayout);

    // Initialize all tabs
    rightPanel->addTab(definitionTab, "Definition");
    rightPanel->addTab(busTab, "Bus");
    rightPanel->addTab(nodeTab, "Node");
    rightPanel->addTab(signalTab, "Signal");
    rightPanel->addTab(ecuTab, "ECU"); // Add ECU Tab

    // Hide all tabs initially
    rightPanel->removeTab(rightPanel->indexOf(definitionTab));
    rightPanel->removeTab(rightPanel->indexOf(busTab));
    rightPanel->removeTab(rightPanel->indexOf(nodeTab));
    rightPanel->removeTab(rightPanel->indexOf(signalTab));
    rightPanel->removeTab(rightPanel->indexOf(ecuTab)); // Hide ECU Tab initially
}

void MainWindow::clearRightPanel()
{
    // Remove all tabs
    while (rightPanel->count() > 0) {
        QWidget* widget = rightPanel->widget(0);
        rightPanel->removeTab(0);
        if (widget) {
            delete widget;
        }
    }

    // Definition Tab
    pgnLineEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("\\d+"), this));
    pgnLineEdit->clear();
    nameLineEdit->clear();
    descLineEdit->clear();
    prioritySpinBox->setValue(0);
    lengthSpinBox->setValue(0);
    extendedDataPageCheckBox->setChecked(false);
    dataPageCheckBox->setChecked(false);
    attributesTable->setRowCount(0);
    signalsList->clear();

    // ECU Tab
    ecuNameLineEdit->clear();

    // Bus Tab
    busNameLineEdit->clear();
    baudRateLineEdit->clear();

    // Signal Tab
    spnSpinBox->setValue(0);
    signalNameLineEdit->clear();
    signalDescLineEdit->clear();
    startBitSpinBox->setValue(0);
    bitLengthSpinBox->setValue(0);
    isBigEndianCheckBox->setChecked(false);
    isTwosComplementCheckBox->setChecked(false);
    factorSpinBox->setValue(0.0);
    offsetSpinBox->setValue(0.0);
    unitsLineEdit->clear();
    enumerationsTable->setRowCount(0);


}




MainWindow::~MainWindow()
{
    delete ui;
    // Delete all models
    qDeleteAll(dbcModels);
    dbcModels.clear();
}

