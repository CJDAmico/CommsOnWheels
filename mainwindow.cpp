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
    QAction *importAction = new QAction("Import DBC", this);
    QAction *exitAction = new QAction("Exit", this);
    fileMenu->addAction(importAction);
    fileMenu->addAction(exitAction);
    // Connect File Actions
    connect(importAction, &QAction::triggered, this, [this]() {
        QSettings settings("Oshkosh", "HeavyInsight");

        QString lastDir = settings.value("lastWorkingDir", QDir::currentPath()).toString();

        QString selectedFile = QFileDialog::getOpenFileName(
            nullptr,
            "Open DBC File",
            lastDir,
            "DBC Files (*.dbc)"
            );
        if (!selectedFile.isEmpty()) {
            // Store last directory used
            QFileInfo fileInfo(selectedFile);
            QString selectedDir = fileInfo.absolutePath();
            settings.setValue("lastWorkingDir", selectedDir);

            // Convert DBC to JSON
            // TODO: Add error handling
            QProcess process;
            QStringList scriptParameters;
            scriptParameters << QDir::currentPath() + "/convert.py";
            scriptParameters << QDir::currentPath();
            scriptParameters << selectedFile;
            process.start("python", scriptParameters);
            process.waitForFinished();

            QString jsonFileName = fileInfo.completeBaseName() + ".json";
            QString jsonFilePath = QDir(QDir::currentPath() + "/JSON/").filePath(jsonFileName);

            // Load the JSON file into the data model
            DbcDataModel* newModel = new DbcDataModel();
            newModel->setFileName(fileInfo.fileName());
            if (newModel->loadFromFile(jsonFilePath)) {
                dbcModels.append(newModel);
                updateDbcTree();
            } else {
                QMessageBox::warning(this, "Import Error", "Failed to import JSON file.");
                delete newModel;
            }

            // Inform the user the import was succesful
            QMessageBox::information(this, "Import Successful", "DBC File Imported:\n" + selectedFile);
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

    QString itemType = item->data(0, Qt::UserRole + 1).toString();

    // Clear existing tabs
    clearRightPanel();

    if (itemType == "Message") {
        handleMessageItem(item);
    }
    else if (itemType == "TxMessage") {
        handleTxMessageItem(item);
    }
    else if (itemType == "RxMessage") {
        handleRxMessageItem(item);
    }
    else if (itemType == "Signal") {
        handleSignalItem(item);
    }
    else if (itemType == "Bus") {
        handleBusItem(item);
    }
    else if (itemType == "Node") {
        handleNodeItem(item);
    }
    else {
        // Optionally handle other types or do nothing
        QMessageBox::information(this, "Info", "Please select a valid item.");
    }

    // When the right panel opens, equalize its width with the left panel
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

void MainWindow::handleMessageItem(QTreeWidgetItem* item)
{
    QString messageName = item->data(0, Qt::UserRole).toString();
    QString modelFileName = item->data(0, Qt::UserRole + 2).toString();

    // Find the model by file name
    DbcDataModel* model = nullptr;
    for (DbcDataModel* m : dbcModels) { // Assuming dbcModels is a member variable holding loaded models
        if (m->fileName() == modelFileName) {
            model = m;
            break;
        }
    }

    if (!model) {
        QMessageBox::warning(this, "Error", "Model not found.");
        return;
    }

    // Find the message in the model
    Message* message = nullptr;
    for (Message& msg : model->messages()) {
        if (msg.name == messageName) {
            message = &msg;
            break;
        }
    }

    if (!message) {
        QMessageBox::warning(this, "Error", "Message not found in the model.");
        return;
    }

    // Show the Definition tab for Messages
    if (rightPanel->indexOf(definitionTab) == -1) {
        rightPanel->addTab(definitionTab, "Definition");
    }

    // Populate Definition tab with message data
    pgnLineEdit->setText(QString::number(message->pgn));
    nameLineEdit->setText(message->name);
    descLineEdit->setText(message->description);
    prioritySpinBox->setValue(message->priority);
    lengthSpinBox->setValue(message->length);
    // TODO: Initial data page?
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

    // Set the active tab
    rightPanel->setCurrentWidget(definitionTab);
}

// Handler: Signal Items
void MainWindow::handleSignalItem(QTreeWidgetItem* item)
{
    QString signalName = item->data(0, Qt::UserRole).toString();
    QString modelFileName = item->data(0, Qt::UserRole + 2).toString();
    QString messageName;

    // Retrieve the parent message name
    QTreeWidgetItem* parentItem = item->parent();
    if (parentItem) {
        messageName = parentItem->data(0, Qt::UserRole).toString();
    } else {
        QMessageBox::warning(this, "Error", "Parent message not found.");
        return;
    }

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

    // Find the message in the model
    Message* message = nullptr;
    for (Message& msg : model->messages()) {
        if (msg.name == messageName) {
            message = &msg;
            break;
        }
    }

    if (!message) {
        QMessageBox::warning(this, "Error", "Message not found in the model.");
        return;
    }

    // Find the signal in the message
    Signal* signal = nullptr;
    for (Signal& sig : message->data) {
        if (sig.name == signalName) {
            signal = &sig;
            break;
        }
    }

    if (!signal) {
        QMessageBox::warning(this, "Error", "Signal not found in the message.");
        return;
    }

    // Show the Signal tab
    if (rightPanel->indexOf(signalTab) == -1) {
        rightPanel->addTab(signalTab, "Signal");
    }

    // Populate Signal tab with signal data
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
    for (const Enumeration& enumValue : signal->enumerations) {
        int row = enumerationsTable->rowCount();
        enumerationsTable->insertRow(row);
        enumerationsTable->setItem(row, 0, new QTableWidgetItem(enumValue.name));
        enumerationsTable->setItem(row, 1, new QTableWidgetItem(enumValue.description));
        enumerationsTable->setItem(row, 2, new QTableWidgetItem(QString::number(enumValue.value)));
    }

    // Set the active tab
    rightPanel->setCurrentWidget(signalTab);
}

// Handler: Bus Items
void MainWindow::handleBusItem(QTreeWidgetItem* item)
{
    QString busName = item->data(0, Qt::UserRole).toString();
    QString modelFileName = item->data(0, Qt::UserRole + 2).toString();

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
    Bus* bus = nullptr;
    for (Bus& b : model->buses()) {
        if (b.name == busName) {
            bus = &b;
            break;
        }
    }

    if (!bus) {
        QMessageBox::warning(this, "Error", "Bus not found in the model.");
        return;
    }

    // Show the Bus tab
    if (rightPanel->indexOf(busTab) == -1) {
        rightPanel->addTab(busTab, "Bus");
    }

    // Populate Bus tab with bus data
    busNameLineEdit->setText(bus->name);
    baudRateLineEdit->setText(bus->baud);

    // Set the active tab
    rightPanel->setCurrentWidget(busTab);
}

// Handler: Node Items
void MainWindow::handleNodeItem(QTreeWidgetItem* item)
{
    QString nodeName = item->data(0, Qt::UserRole).toString();
    QVariant variant = item->data(0, Qt::UserRole + 2);
    QStringList modelFileNames;

    if (variant.canConvert<QStringList>()) {
        modelFileNames = variant.toStringList();
    }
    else if (variant.canConvert<QString>()) {
        modelFileNames << variant.toString();
    }

    if (modelFileNames.isEmpty()) {
        QMessageBox::warning(this, "Error", "No associated models found for this node.");
        return;
    }

    // For simplicity, we'll display data from the first associated model
    // You can modify this to handle multiple models as needed

    DbcDataModel* model = nullptr;
    for (const QString& modelFileName : modelFileNames) {
        for (DbcDataModel* m : dbcModels) {
            if (m->fileName() == modelFileName) {
                model = m;
                break;
            }
        }
        if (model) break;
    }

    if (!model) {
        QMessageBox::warning(this, "Error", "Model not found.");
        return;
    }

    // Find the node in the model
    Node* node = nullptr;
    for (Node& n : model->nodes()) {
        if (n.name == nodeName) {
            node = &n;
            break;
        }
    }

    if (!node) {
        QMessageBox::warning(this, "Error", "Node not found in the model.");
        return;
    }

    // Show the Node tab
    if (rightPanel->indexOf(nodeTab) == -1) {
        rightPanel->addTab(nodeTab, "Node");
    }

    // Populate Node tab with node data
    nodeNameLineEdit->setText(node->name);
    // Populate other node-specific widgets as needed

    // Set the active tab
    rightPanel->setCurrentWidget(nodeTab);
}

// Handler: Transmitted Messages under NodeBus
void MainWindow::handleTxMessageItem(QTreeWidgetItem* item)
{
    QString txMessageName = item->data(0, Qt::UserRole).toString();
    QString modelFileName = item->data(0, Qt::UserRole + 2).toString();

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

    // Find the message in the model
    Message* message = nullptr;
    for (Message& msg : model->messages()) {
        if (msg.name == txMessageName) {
            message = &msg;
            break;
        }
    }

    if (!message) {
        QMessageBox::warning(this, "Error", "Transmitted Message not found in the model.");
        return;
    }

    // Show the Definition tab for Messages
    if (rightPanel->indexOf(definitionTab) == -1) {
        rightPanel->addTab(definitionTab, "Definition");
    }

    // Populate Definition tab with message data
    pgnLineEdit->setText(QString::number(message->pgn));
    nameLineEdit->setText(message->name);
    descLineEdit->setText(message->description);
    prioritySpinBox->setValue(message->priority);
    lengthSpinBox->setValue(message->length);
    // TODO: Initial data page?
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

    // Set the active tab
    rightPanel->setCurrentWidget(definitionTab);
}

void MainWindow::handleRxMessageItem(QTreeWidgetItem* item)
{
    QString rxMessageName = item->data(0, Qt::UserRole).toString();
    QString modelFileName = item->data(0, Qt::UserRole + 2).toString();

    qDebug() << "Clicked Received Message:" << rxMessageName << "Model File:" << modelFileName;

    // Find the model by file name
    DbcDataModel* model = nullptr;
    for (DbcDataModel* m : dbcModels) {
        if (m->fileName() == modelFileName) {
            model = m;
            break;
        }
    }

    if (!model) {
        QMessageBox::warning(this, "Error", QString("Model '%1' not found.").arg(modelFileName));
        return;
    }

    // Find the message in the model
    Message* message = nullptr;
    for (Message& msg : model->messages()) {
        if (msg.name == rxMessageName) {
            message = &msg;
            break;
        }
    }

    if (!message) {
        QMessageBox::warning(this, "Error", QString("Received Message '%1' not found in the model '%2'.")
                                 .arg(rxMessageName, modelFileName));
        return;
    }

    // Show the Definition tab for Messages
    if (rightPanel->indexOf(definitionTab) == -1) {
        rightPanel->addTab(definitionTab, "Definition");
    }

    // Populate Definition tab with message data
    pgnLineEdit->setText(QString::number(message->pgn));
    nameLineEdit->setText(message->name);
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

    // Set the active tab
    rightPanel->setCurrentWidget(definitionTab);
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

    // Node Tab
    nodeTab = new QWidget;
    nodeFormLayout = new QFormLayout;
    nodeNameLineEdit = new QLineEdit;
    nodeFormLayout->addRow("Node Name:", nodeNameLineEdit);
    nodeTab->setLayout(nodeFormLayout);

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

    // Hide all tabs initially
    rightPanel->removeTab(rightPanel->indexOf(definitionTab));
    rightPanel->removeTab(rightPanel->indexOf(busTab));
    rightPanel->removeTab(rightPanel->indexOf(nodeTab));
    rightPanel->removeTab(rightPanel->indexOf(signalTab));
}

void MainWindow::clearRightPanel()
{
    // Remove all tabs
    while (rightPanel->count() > 0) {
        QWidget* widget = rightPanel->widget(0);
        rightPanel->removeTab(0);
    }

    // Clear data from widgets
    // Definition Tab
    pgnLineEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("\\d+"), this));
    nameLineEdit->clear();
    descLineEdit->clear();
    prioritySpinBox->setValue(0);
    lengthSpinBox->setValue(0);
    extendedDataPageCheckBox->setChecked(false);
    dataPageCheckBox->setChecked(false);
    attributesTable->setRowCount(0);
    signalsList->clear();

    // Bus Tab
    busNameLineEdit->clear();
    baudRateLineEdit->clear();

    // Node Tab
    nodeNameLineEdit->clear();

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

