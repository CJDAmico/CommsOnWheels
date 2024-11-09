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
        qDeleteAll(dbcModels);
        dbcModels.clear();
        updateDbcTree();
    });

    QAction *openJson = new QAction("Open from JSON...", this);
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
                qDeleteAll(dbcModels);
                dbcModels.clear();
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
    // this->styleMode = new QAction("Dark Mode", this);
    viewMenu->addAction(hexadecimalMode);
    viewMenu->addAction(binaryMode);
    viewMenu->addAction(decimalMode);
    // viewMenu->addAction(styleMode);
    // connect(styleMode, &QAction::triggered, this, &MainWindow::toggleDarkMode);  // Connect the button to the toggleDarkMode function

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

// TODO: Figure out whether to use system default or custom dark mode
// void MainWindow::toggleDarkMode() {
//     static bool darkModeEnabled = false;


//     if (darkModeEnabled) {
//         // Revert to light mode
//         qApp->setPalette(defaultPalette);
//         this->setStyleSheet(defaultStyleSheet);
//         this->menuBar()->setStyleSheet("");

//         this->styleMode->setText("Dark Mode");
//         darkModeEnabled = false;
//     } else {
//         defaultPalette = QApplication::palette();
//         defaultStyleSheet = this->styleSheet();

//         // Apply dark mode
//         QPalette darkPalette;
//         // Window and background colors
//         darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
//         darkPalette.setColor(QPalette::WindowText, Qt::white);
//         darkPalette.setColor(QPalette::Base, QColor(42, 42, 42));
//         darkPalette.setColor(QPalette::AlternateBase, QColor(66, 66, 66));
//         darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
//         darkPalette.setColor(QPalette::ToolTipText, Qt::white);

//         // Text and buttons
//         darkPalette.setColor(QPalette::Text, Qt::white);
//         darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
//         darkPalette.setColor(QPalette::ButtonText, Qt::white);
//         darkPalette.setColor(QPalette::BrightText, Qt::red);
//         darkPalette.setColor(QPalette::PlaceholderText, Qt::white);

//         // Highlighted text and selections
//         darkPalette.setColor(QPalette::Highlight, Qt::white);
//         darkPalette.setColor(QPalette::HighlightedText, Qt::black);

//         // Set Menu Bar to dark style
//         this->menuBar()->setStyleSheet("QMenuBar { background-color: #353535; color: white; } "
//                                        "QMenuBar::item { background-color: #353535; color: white; } "
//                                        "QMenuBar::item:selected { background-color: #454545; } "
//                                        "QMenuBar::item:pressed { background-color: #252525; } "
//                                        "QMenu { background-color: #353535; color: white; } "
//                                        "QMenu::item:selected { background-color: #454545; }");

//         this->styleMode->setText("Light Mode");
//         qApp->setPalette(darkPalette);

//         QString newStyleSheet = "QTableView::item:selected { background-color: #55aaff; color: black; } "
//                                 "QListView::item:selected { background-color: #55aaff; color: black; } "
//                                 "QTreeView::item:selected { background-color: #55aaff; color: black; } "
//                                 "QTreeView { outline: 0; }"
//                                 "QListView { outline: 0; }"
//                                 "QTableView { outline: 0; }";

//         this->setStyleSheet(this->styleSheet() + newStyleSheet);
//         darkModeEnabled = true;
//     }
// }

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
    else if (itemType == "Network") {
        handleNetworkItem(item, name, models);
    }
    else if (itemType == "Node") {
        handleNodeItem(item, name, models);
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

    // Retrieve the uniqueKey (which is the message's PGN)
    QString uniqueKey = item->data(0, Qt::UserRole + 2).toString();

    Message* message = nullptr;
    for (Message& msg : model->messages()) {
        if (QString::number(msg.pgn) == uniqueKey) {
            message = &msg;
            break;
        }
    }

    if (!message) {
        QMessageBox::warning(this, "Error", "Message not found in the model.");
        return;
    }

    QString txRxType;
    QString itemType = item->data(0, Qt::UserRole).toString();
    if (itemType == "TxMessage" || itemType == "RxMessage") {
        txRxType = (itemType == "TxMessage") ? "Transmitted" : "Received";
    }

    // Populate Definition tab
    pgnLineEdit->setText(QString("0x") + QString::number(message->pgn, 16).toUpper());
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
    messageAttributesTable->setRowCount(0);
    for(const Attribute& attribute : message->messageAttributes) {
        addAttributeRow(messageAttributesTable, { attribute.name, attribute.type, attribute.value });
    }

    // Update signals list
    signalsList->clear();
    for (const Signal& signal : message->messageSignals) {
        signalsList->addItem(signal.name);
    }

    // Show the Definition tab
    if (rightPanel->indexOf(definitionTab) == -1) {
        rightPanel->addTab(definitionTab, "Definition");
    }

    // Update transmitters table
    transmittersTable->setRowCount(0);
    for(const std::pair<QString, QString>& pair : message->messageTransmitters) {
        addAttributeRow(transmittersTable, {pair.first, pair.second});
    }

    // Show transmitters tab
    if (rightPanel->indexOf(transmittersTab) == -1) {
        rightPanel->addTab(transmittersTab, "Transmitters");
    }

    // Update receivers table
    receiversTable->setRowCount(0);
    for(const std::pair<QString, QString>& pair : message->messageReceivers) {
        addAttributeRow(receiversTable, {pair.first, pair.second});
    }

    // Show receivers tab
    if (rightPanel->indexOf(receiversTab) == -1) {
        rightPanel->addTab(receiversTab, "Receivers");
    }



    // Populate the combo box with multiplexor values
    multiplexorComboBox->clear();
    multiplexorComboBox->addItem("No Multiplexor", -1);
    for (Signal& signal : message->messageSignals) {
        if(signal.isMultiplexor) {
            qDebug() << "enumerations found";
            for(Enumeration& enumeration : signal.enumerations) {
                QString hexValue = QString("0x") + QString::number(enumeration.value, 16).toUpper();
                multiplexorComboBox->addItem(signal.name + ": " + hexValue + " (" + enumeration.name + ")", enumeration.value);
            }
        }
    }

    // Disconnect any existing connections to prevent multiple triggers
    disconnect(multiplexorComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
               this, nullptr);

    // Assign selected message to context wide variable
    currentMessage = message;

    // Connect the combo box signal to update multiplexorValue
    connect(multiplexorComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                if (!currentMessage) return;

                int selectedMultiplexor = multiplexorComboBox->currentData().toInt();
                currentMessage->multiplexValue = selectedMultiplexor;
                displayBitLayout(*currentMessage, selectedMultiplexor); // Update the bit layout
            });

    displayBitLayout(*message, message->multiplexValue);

    // Show layout tab
    if (rightPanel->indexOf(layoutTab) == -1) {
        rightPanel->addTab(layoutTab, "Layout");
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

    // Retrieve the uniqueKey of the parent message
    QTreeWidgetItem* signalsCategory = item->parent();
    if (!signalsCategory || signalsCategory->text(0) != "<Signals>") {
        QMessageBox::warning(this, "Error", "Parent <Signals> category not found.");
        return;
    }

    QTreeWidgetItem* parentItem = signalsCategory->parent();
    if (!parentItem) {
        QMessageBox::warning(this, "Error", "Parent message not found.");
        return;
    }

    QString messageUniqueKey = parentItem->data(0, Qt::UserRole + 2).toString();

    const Message* message = nullptr;
    for (const Message& msg : model->messages()) {
        if (QString::number(msg.pgn) == messageUniqueKey) {
            message = &msg;
            break;
        }
    }

    if (!message) {
        QMessageBox::warning(this, "Error", "Parent message not found in the model.");
        return;
    }

    // Find the signal by name
    const Signal* signal = nullptr;
    for (const Signal& sig : message->messageSignals) {
        if (sig.name.trimmed() == name.trimmed()) {
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

    // Update attributes table
    signalAttributesTable->setRowCount(0);
    for(const Attribute& attribute : signal->signalAttributes) {
        addAttributeRow(signalAttributesTable, { attribute.name, attribute.type, attribute.value });
    }

    // Update enumerations table
    enumerationsTable->setRowCount(0);
    for (const Enumeration& enumVal : signal->enumerations) {
        int row = enumerationsTable->rowCount();
        enumerationsTable->insertRow(row);
        enumerationsTable->setItem(row, 0, new QTableWidgetItem(enumVal.name));
        enumerationsTable->setItem(row, 1, new QTableWidgetItem(QString::number(enumVal.value)));
        enumerationsTable->setItem(row, 2, new QTableWidgetItem(enumVal.description));
    }

    // Show the Signal tab
    if (rightPanel->indexOf(signalTab) == -1) {
        rightPanel->addTab(signalTab, "Signal");
    }

    rightPanel->setCurrentWidget(signalTab);
}


void MainWindow::handleNetworkItem(QTreeWidgetItem* item, const QString& name, const QStringList& models)
{
    if (models.isEmpty()) {
        QMessageBox::warning(this, "Error", "No associated models found for this network.");
        return;
    }

    QString modelFileName = models.first();
    DbcDataModel* model = nullptr;

    // Find the corresponding model
    for (DbcDataModel* m : dbcModels) {
        if (m->fileName().trimmed() == modelFileName.trimmed()) {
            model = m;
            break;
        }
    }

    if (!model) {
        QMessageBox::warning(this, "Error", "Model not found.");
        return;
    }

    const QList<Network>& networks = model->networks();
    if (networks.isEmpty()) {
        qCritical() << "No networks found in the model!";
        return;
    }

    // Find the network by name
    const Network* network = nullptr;
    for (const Network& net : networks) {
        if (net.name.trimmed() == name.trimmed()) {
            network = &net;
            break;
        }
    }

    if (!network) {
        QMessageBox::warning(this, "Error", "Network not found in the model.");
        return;
    }

    // Check if network name is valid before accessing
    if (network->name.isEmpty()) {
        qCritical() << "Network name is empty!";
        return;
    }

    // Populate the network tab fields
    networkNameLineEdit->setText(network->name);
    baudRateLineEdit->setText(network->baud);

    // Update attributes table
    networkAttributesTable->setRowCount(0);
    for(const Attribute& attribute : network->networkAttributes) {
        addAttributeRow(networkAttributesTable, { attribute.name, attribute.type, attribute.value });
    }

    // Show the Network tab
    if (rightPanel->indexOf(networkTab) == -1) {
        rightPanel->addTab(networkTab, "Network");
    }
    rightPanel->setCurrentWidget(networkTab);
}



void MainWindow::handleNodeItem(QTreeWidgetItem* item, const QString& name, const QStringList& models)
{
    if (models.isEmpty()) {
        QMessageBox::warning(this, "Error", "No associated models found for this node.");
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

    const Node* node = nullptr;
    for (const Node& n : model->nodes()) {
        if (n.name.trimmed() == name) {
            node = &n;
            break;
        }
    }

    if (!node) {
        QMessageBox::warning(this, "Error", "Node not found in the model.");
        return;
    }

    nodeNameLineEdit->setText(node->name);
    nodeAddressTable->setRowCount(0);
    for (const NodeNetworkAssociation& association : node->networks) {
        int row = nodeAddressTable->rowCount();
        nodeAddressTable->insertRow(row);

        // Network Name
        QTableWidgetItem* networkNameItem = new QTableWidgetItem(association.networkName);
        networkNameItem->setFlags(networkNameItem->flags() & ~Qt::ItemIsEditable); // Make it read-only
        nodeAddressTable->setItem(row, 0, networkNameItem);

        // Source Address
        QTableWidgetItem* sourceAddressItem = new QTableWidgetItem(QString::number(association.sourceAddress));
        sourceAddressItem->setFlags(sourceAddressItem->flags() & ~Qt::ItemIsEditable); // Make it read-only
        nodeAddressTable->setItem(row, 1, sourceAddressItem);
    }

    // Update attributes table
    nodeAttributesTable->setRowCount(0);
    for(const Attribute& attribute : node->nodeAttributes) {
        addAttributeRow(nodeAttributesTable, { attribute.name, attribute.type, attribute.value });
    }

    // Show the node tab
    if (rightPanel->indexOf(nodeTab) == -1) {
        rightPanel->addTab(nodeTab, "Node");
    }

    rightPanel->setCurrentWidget(nodeTab);
}



void MainWindow::setupRightPanel()
{
    // Initialize widgets
    rightPanel = new QTabWidget;

    //--------Messages--------------------
    // Definition
    definitionTab = new QWidget;
    definitionFormLayout = new QFormLayout;
    pgnLineEdit = new QLineEdit;
    nameLineEdit = new QLineEdit;
    descLineEdit = new QLineEdit;
    prioritySpinBox = new QSpinBox;
    lengthSpinBox = new QSpinBox;
    extendedDataPageCheckBox = new QCheckBox;
    dataPageCheckBox = new QCheckBox;
    messageAttributesTable = new QTableWidget;
    signalsList = new QListWidget;

    // Transmitters
    transmittersTab = new QWidget;
    transmittersTable = new QTableWidget;
    transmittersFormLayout = new QFormLayout;

    // Receivers
    receiversTab = new QWidget;
    receiversTable = new QTableWidget;
    receiversFormLayout = new QFormLayout;

    // Layout
    layoutTab = new QWidget;
    bitGrid = new QTableWidget;
    layoutFormLayout = new QFormLayout;
    // Multiplexor selection dropdown
    multiplexorComboBox = new QComboBox;

    //--------Network--------------------
        networkTab = new QWidget;
    networkFormLayout = new QFormLayout;
    networkNameLineEdit = new QLineEdit;
    baudRateLineEdit = new QLineEdit;
    networkAttributesTable = new QTableWidget;

    //--------Node--------------------
    nodeTab = new QWidget;
    nodeFormLayout = new QFormLayout;
    nodeNameLineEdit = new QLineEdit;
    nodeAddressTable = new QTableWidget;
    nodeAttributesTable = new QTableWidget;


    //--------Signal--------------------
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
    signalAttributesTable = new QTableWidget;


    // Set up the Definition Form Layout
    definitionFormLayout->addRow("PGN:", pgnLineEdit);
    definitionFormLayout->addRow("Name:", nameLineEdit);
    definitionFormLayout->addRow("Description:", descLineEdit);
    definitionFormLayout->addRow("Priority:", prioritySpinBox);
    definitionFormLayout->addRow("Length:", lengthSpinBox);
    definitionFormLayout->addRow("Extended Data Page:", extendedDataPageCheckBox);
    definitionFormLayout->addRow("Data Page:", dataPageCheckBox);
    definitionFormLayout->addRow(new QLabel("Message Attributes:"));
    messageAttributesTable->setColumnCount(3);
    messageAttributesTable->setHorizontalHeaderLabels({"Name", "Type", "Value"});
    messageAttributesTable->horizontalHeader()->setStretchLastSection(true);
    definitionFormLayout->addRow(messageAttributesTable);
    definitionFormLayout->addRow(new QLabel("Signals:"));
    definitionFormLayout->addRow(signalsList);
    definitionTab->setLayout(definitionFormLayout);

    // Set up Transmitters form
    transmittersTable->setColumnCount(2);
    transmittersTable->setHorizontalHeaderLabels({"Name", "Address"});
    transmittersTable->horizontalHeader()->setStretchLastSection(true);
    transmittersFormLayout->addRow(transmittersTable);
    transmittersTab->setLayout(transmittersFormLayout);

    // Set up Receivers form
    receiversTable->setColumnCount(2);
    receiversTable->setHorizontalHeaderLabels({"Name", "Address"});
    receiversTable->horizontalHeader()->setStretchLastSection(true);
    receiversFormLayout->addRow(receiversTable);
    receiversTab->setLayout(receiversFormLayout);

    // Set up Layout form
    layoutFormLayout->addRow("Multiplexor:", multiplexorComboBox);
    layoutFormLayout->addRow(bitGrid);
    layoutTab->setLayout(layoutFormLayout);

    // Network Tab
    networkFormLayout->addRow("Network Name:", networkNameLineEdit);
    networkFormLayout->addRow("Baud Rate:", baudRateLineEdit);
    networkFormLayout->addRow(new QLabel("Network Attributes:"));
    networkAttributesTable->setColumnCount(3);
    networkAttributesTable->setHorizontalHeaderLabels({"Name", "Type", "Value"});
    networkAttributesTable->horizontalHeader()->setStretchLastSection(true);
    networkFormLayout->addRow(networkAttributesTable);
    networkTab->setLayout(networkFormLayout);

    // Node Tab
    nodeAddressTable->setColumnCount(2);
    nodeAddressTable->setHorizontalHeaderLabels({"Network Name", "Source Address"});
    nodeFormLayout->addRow("Node Name:", nodeNameLineEdit);
    nodeFormLayout->addRow(new QLabel("Node Address:"));
    nodeFormLayout->addRow(nodeAddressTable);
    nodeFormLayout->addRow(new QLabel("Node Attributes:"));
    nodeAttributesTable->setColumnCount(3);
    nodeAttributesTable->setHorizontalHeaderLabels({"Name", "Type", "Value"});
    nodeAttributesTable->horizontalHeader()->setStretchLastSection(true);
    nodeFormLayout->addRow(nodeAttributesTable);
    nodeTab->setLayout(nodeFormLayout);

    // Signal Tab
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
    signalFormLayout->addRow(new QLabel("Signal Attributes:"));
    signalAttributesTable->setColumnCount(3);
    signalAttributesTable->setHorizontalHeaderLabels({"Name", "Type", "Value"});
    signalAttributesTable->horizontalHeader()->setStretchLastSection(true);
    signalFormLayout->addRow(signalAttributesTable);
    signalFormLayout->addRow(new QLabel("Enumerations:"));
    enumerationsTable->setColumnCount(3);
    enumerationsTable->setHorizontalHeaderLabels({"Name", "Value", "Description"});
    enumerationsTable->horizontalHeader()->setStretchLastSection(true);
    signalFormLayout->addRow(enumerationsTable);

    signalTab->setLayout(signalFormLayout);

    // Initialize all tabs
    rightPanel->addTab(definitionTab, "Definition");
    rightPanel->addTab(transmittersTab, "Transmitters");
    rightPanel->addTab(receiversTab, "Receivers");
    rightPanel->addTab(layoutTab, "Layout");
    rightPanel->addTab(networkTab, "Network");
    rightPanel->addTab(signalTab, "Signal");
    rightPanel->addTab(nodeTab, "Node");
    // Hide all tabs initially
    rightPanel->removeTab(rightPanel->indexOf(definitionTab));
    rightPanel->removeTab(rightPanel->indexOf(networkTab));
    rightPanel->removeTab(rightPanel->indexOf(signalTab));
    rightPanel->removeTab(rightPanel->indexOf(nodeTab));
    rightPanel->removeTab(rightPanel->indexOf(transmittersTab));
    rightPanel->removeTab(rightPanel->indexOf(receiversTab));
    rightPanel->removeTab(rightPanel->indexOf(layoutTab));
}


void MainWindow::clearRightPanel()
{
    // Remove all tabs
    while (rightPanel->count() > 0) {
        rightPanel->removeTab(0);
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
    messageAttributesTable->setRowCount(0);
    networkAttributesTable->setRowCount(0);
    nodeAttributesTable->setRowCount(0);
    signalAttributesTable->setRowCount(0);
    transmittersTable->setRowCount(0);
    receiversTable->setRowCount(0);
    bitGrid->setRowCount(0);
    bitGrid->setColumnCount(0);
    currentMessage = nullptr;
    disconnect(multiplexorComboBox, nullptr, this, nullptr);
    signalsList->clear();

    // Node Tab
    nodeNameLineEdit->clear();
    nodeAddressTable->setRowCount(0);

    // Network Tab
    networkNameLineEdit->clear();
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


void MainWindow::displayBitLayout(Message& message , int selectedMultiplexor = -1) {
    int messageLength = message.length * 8;  // Convert message length from bytes to bits

    // Clear existing grid content
    bitGrid->clear();
    bitGrid->setRowCount((messageLength + 7) / 8); // Each row represents a byte (8 bits)
    bitGrid->setColumnCount(8); // Fixed 8 columns to represent 8 bits per byte

    // Set up column headers to show bit positions (7 to 0)
    for (int i = 0; i < 8; ++i) {
        bitGrid->setHorizontalHeaderItem(i, new QTableWidgetItem(QString::number(7 - i)));
    }

    // Set up row headers to indicate byte index
    for (int row = 0; row < bitGrid->rowCount(); ++row) {
        bitGrid->setVerticalHeaderItem(row, new QTableWidgetItem(QString::number(row)));
    }

    // Reset color index for this message:
    colorsIndex = 0;
    // Populate the grid based on each signal's bit allocation
    for (const Signal& signal : message.messageSignals) {
        // Skip signals if they're multiplexed and not matching the selected multiplexor value
        if (message.multiplexValue != -1 && signal.multiplexValue != message.multiplexValue) {
            continue;
        }

        int startBit = signal.startBit;
        int currentRow = startBit / 8;
        int currentCol = startBit % 8;
        int bitsRemaining = signal.bitLength;
        int spanLength;

        while (bitsRemaining > 0) {
            // Determine the span for this part of the signal within the current row
            spanLength = std::min(bitsRemaining, 8 - currentCol);

            // Create a QTableWidgetItem and set properties
            QTableWidgetItem* item = new QTableWidgetItem(signal.name);
            item->setToolTip(signal.description);
            item->setBackground(signalColors[colorsIndex]);
            item->setTextAlignment(Qt::AlignCenter);

            // Set the item and apply the span if the signal extends across multiple columns
            bitGrid->setItem(currentRow, currentCol, item);
            if (spanLength > 1) {
                bitGrid->setSpan(currentRow, currentCol, 1, spanLength);
            }

            // Move to the next row if more bits remain for this signal
            bitsRemaining -= spanLength;
            currentRow++;
            currentCol = 0; // Reset column to the start of the next row
        }

        // Increment color index for the next signal
        colorsIndex++;
    }
}





MainWindow::~MainWindow()
{
    delete ui;
    // Delete all models
    qDeleteAll(dbcModels);
    dbcModels.clear();
}

