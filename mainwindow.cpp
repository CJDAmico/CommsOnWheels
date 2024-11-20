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
    // Window Icon (Default)
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
        clearRightPanel();
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
        }
    });

    // Import DBC
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

            if (newModel->importDBC(selectedFile)) { // Pass the selected DBC file path
                dbcModels.append(newModel);
                updateDbcTree();
            } else {
                QMessageBox::warning(this, "Import Error", "Failed to import DBC file.");
                delete newModel;
            }
        }
    });

    // Export DBC
    QAction *exportDBC = new QAction("Export DBC...", this);
    fileMenu->addAction(exportDBC);
    connect(exportDBC, &QAction::triggered, this, [this]() {
        // TODO: Implement DBC export functionality
    });

    // Save workspace as a JSON
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

    // Add expand all action to Edit menu
    QAction *expandAllAction = new QAction("Expand All", this);
    editMenu->addAction(expandAllAction);
    connect(expandAllAction, &QAction::triggered, this, [this]() {
        QTreeWidgetItemIterator it(dbcTree);
        while(*it) {
            QTreeWidgetItem *item = *it;
            if(item->childCount() > 0) {
                item->setExpanded(true);
            }
            it++;
        }
    });

    // Add collapse all action to Edit menu
    QAction *collapseAllAction = new QAction("Collapse All", this);
    editMenu->addAction(collapseAllAction);
    connect(collapseAllAction, &QAction::triggered, this, [this]() {
        QTreeWidgetItemIterator it(dbcTree);
        while(*it) {
            QTreeWidgetItem *item = *it;
            if(item->childCount() > 0) {
                item->setExpanded(false);
            }
            it++;
        }
    });

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
    viewMenu->addAction(hexadecimalMode);
    viewMenu->addAction(binaryMode);
    viewMenu->addAction(decimalMode);

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
    connect(searchBar, &QLineEdit::textChanged, this, &MainWindow::filterTreeItems);

    leftLayout->addWidget(searchBar);
    leftLayout->addWidget(dbcTree);
    leftPanel->setLayout(leftLayout);


    // ------------------- Center Layout -----------------------
    splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(leftPanel);
    splitter->addWidget(rightPanel);

    splitter->setStretchFactor(0, 1);
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

void MainWindow::updateDbcTree()
{
    dbcTree->populateTree(dbcModels);
}

void MainWindow::filterTreeItems(const QString &filterText) {
    QTreeWidgetItemIterator it(dbcTree);

    while (*it) {
        QTreeWidgetItem *item = *it;
        QString itemType = item->data(0, Qt::UserRole).toString();

        // Always show category bars
        if (itemType == "Category") {
            item->setHidden(false);
            item->setExpanded(true);
        } else {
            bool match = item->text(0).contains(filterText, Qt::CaseInsensitive);
            bool hasMatchingChild = false;

            // Check if any child matches the filter, including nested children
            for (int i = 0; i < item->childCount(); ++i) {
                QTreeWidgetItem *child = item->child(i);
                bool childMatch = child->text(0).contains(filterText, Qt::CaseInsensitive);
                bool hasMatchingGrandchild = false;

                // Check if any grandchild matches the filter
                for (int j = 0; j < child->childCount(); ++j) {
                    QTreeWidgetItem *grandchild = child->child(j);
                    if (grandchild->text(0).contains(filterText, Qt::CaseInsensitive)) {
                        grandchild->setHidden(false);
                        hasMatchingGrandchild = true;
                    } else {
                        grandchild->setHidden(true);
                    }
                }

                if (childMatch || hasMatchingGrandchild) {
                    child->setHidden(false);
                    child->setExpanded(hasMatchingGrandchild); // Expand if it has matching grandchildren
                    hasMatchingChild = true;
                } else {
                    child->setHidden(true);
                }
            }

            // If the item itself or any of its children matches, make it visible
            if (match || hasMatchingChild) {
                item->setHidden(false);
                item->setExpanded(hasMatchingChild || match); // Expand if it has matching children or if the item matches
            } else {
                item->setHidden(true);
            }
        }

        ++it;
    }
}



void MainWindow::onTreeItemClicked(QTreeWidgetItem* item)
{
    if (!item) return;

    QString itemType = item->data(0, Qt::UserRole).toString();
    QString name = item->text(0);
    QString model = item->data(0, Qt::UserRole + 1).toString();

    // Clear existing tabs
    clearRightPanel();

    currentTreeItem = item;

    if (itemType == "Message" || itemType == "TxMessage" || itemType == "RxMessage") {
        handleMessageItem(item, name, model);
    }
    else if (itemType == "Signal") {
        handleSignalItem(item, name, model);
    }
    else if (itemType == "Network") {
        handleNetworkItem(item, name, model);
    }
    else if (itemType == "Node") {
        handleNodeItem(item, name, model);
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

void MainWindow::handleMessageItem(QTreeWidgetItem* item, const QString& name, const QString& modelName)
{
    DbcDataModel* model = nullptr;
    for (DbcDataModel* m : dbcModels) {
        if (m->fileName() == modelName) {
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

    // Assign the found message to the context-wide variable
    currentMessage = message;


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
    extendedDataPageCheckBox->setChecked(message->extendedDataPage);
    dataPageCheckBox->setChecked(message->dataPage);

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

    // Connect the signalsList item click event
    connect(signalsList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        QString signalName = item->text();

        // Locate the QTreeWidgetItem corresponding to the signal name
        if (currentTreeItem) {
            QTreeWidgetItem* signalsCategoryItem = nullptr;

            // Traverse through children of currentMessageItem to find the <Signals> node
            for (int i = 0; i < currentTreeItem->childCount(); ++i) {
                if (currentTreeItem->child(i)->text(0) == "<Signals>") {
                    signalsCategoryItem = currentTreeItem->child(i);
                    break;
                }
            }

            // Now find the signal item within the <Signals> category
            if (signalsCategoryItem) {
                for (int i = 0; i < signalsCategoryItem->childCount(); ++i) {
                    QTreeWidgetItem* signalItem = signalsCategoryItem->child(i);
                    if (signalItem->text(0) == signalName) {
                        dbcTree->setCurrentItem(signalItem);
                        onTreeItemClicked(signalItem);
                        break;
                    }
                }
            }
        }
    });

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

    // Populate the combo box with multiplexer values
    multiplexerComboBox->clear();
    multiplexerComboBox->addItem("No Multiplexer", -1);
    for (Signal& signal : message->messageSignals) {
        if(signal.isMultiplexer) {
            for(Enumeration& enumeration : signal.enumerations) {
                QString hexValue = QString("0x") + QString::number(enumeration.value, 16).toUpper();
                multiplexerComboBox->addItem(signal.name + ": " + hexValue + " (" + enumeration.name + ")", enumeration.value);
            }
        }
    }

    // Disconnect any existing connections to prevent multiple triggers
    disconnect(multiplexerComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
               this, nullptr);

    // Connect the combo box signal to update multiplexerValue
    connect(multiplexerComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                if (!currentMessage) return;

                int selectedMultiplexer = multiplexerComboBox->currentData().toInt();
                currentMessage->multiplexValue = selectedMultiplexer;
                displayBitLayout(*currentMessage, selectedMultiplexer); // Update the bit layout
            });

    displayBitLayout(*message, message->multiplexValue);

    // Connect pgnLineEdit to handle PGN changes
    connect(pgnLineEdit, &QLineEdit::textChanged, this, [this, item](const QString &text) {
        if (currentMessage) {
            bool ok;
            quint64 newPgn = text.toULongLong(&ok, 16);
            if (ok && newPgn != currentMessage->pgn) {
                // Update the PGN in the tree item and ensure all references are updated
                item->setData(0, Qt::UserRole + 2, QString::number(newPgn));

                // Iterate through all items and update PGN references where applicable
                QList<QTreeWidgetItem *> items = dbcTree->findItems(QString::number(currentMessage->pgn), Qt::MatchExactly | Qt::MatchRecursive);
                for (QTreeWidgetItem *currentItem : items) {
                    if (currentItem->data(0, Qt::UserRole + 2).toString() == QString::number(currentMessage->pgn)) {
                        currentItem->setData(0, Qt::UserRole + 2, QString::number(newPgn));
                    }
                }

                currentMessage->pgn = newPgn;
            }
        }
    });

    // Connect nameLineEdit to handle name changes
    connect(nameLineEdit, &QLineEdit::textChanged, this, [this, item](const QString &text) {
        if (currentMessage && !text.isEmpty() && text != currentMessage->name) {
            // Update the name in the tree item
            item->setText(0, text);

            // Update all tree items that reference the old name
            QList<QTreeWidgetItem *> items = dbcTree->findItems(currentMessage->name, Qt::MatchExactly | Qt::MatchRecursive);
            for (QTreeWidgetItem *currentItem : items) {
                if (currentItem->text(0) == currentMessage->name) {
                    currentItem->setText(0, text);
                }
            }
            currentMessage->name = text;
        }
    });

    // Connect descLineEdit to handle description changes
    connect(descLineEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (currentMessage) {
            currentMessage->description = text;
        }
    });

    // Connect prioritySpinBox to handle priority changes
    connect(prioritySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
        if (currentMessage) {
            currentMessage->priority = value;
        }
    });

    // Connect lengthSpinBox to handle length changes
    connect(lengthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
        if (currentMessage) {
            currentMessage->length = value;
        }
    });

    // Connect extendedDataPageCheckBox to handle extended data page changes
    connect(extendedDataPageCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        if (currentMessage) {
            currentMessage->extendedDataPage = checked;
        }
    });

    // Connect dataPageCheckBox to handle data page changes
    connect(dataPageCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        if (currentMessage) {
            currentMessage->dataPage = checked;
        }
    });

    // Show layout tab
    if (rightPanel->indexOf(layoutTab) == -1) {
        rightPanel->addTab(layoutTab, "Layout");
    }

    rightPanel->setCurrentWidget(definitionTab);
}


void MainWindow::handleSignalItem(QTreeWidgetItem* item, const QString& name, const QString& modelName)
{
    DbcDataModel* model = nullptr;
    for (DbcDataModel* m : dbcModels) {
        if (m->fileName() == modelName) {
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

    Message* message = nullptr;
    for (Message& msg : model->messages()) {
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
    Signal* signal = nullptr;
    for (Signal& sig : message->messageSignals) {
        if (sig.name.trimmed() == name.trimmed()) {
            signal = &sig;
            break;
        }
    }

    if (!signal) {
        QMessageBox::warning(this, "Error", "Signal not found in the message.");
        return;
    }

    currentSignal = signal;

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

    // Connect SPN SpinBox to handle SPN changes
    connect(spnSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
        if (currentSignal) {
            currentSignal->spn = value;
        }
    });

    // Connect signalNameLineEdit to handle name changes
    connect(signalNameLineEdit, &QLineEdit::textChanged, this, [this, item](const QString &text) {
        if (currentSignal) {
            // Update the name in the tree item
            item->setText(0, text);

            // Update all tree items that reference the old name
            QList<QTreeWidgetItem *> items = dbcTree->findItems(currentMessage->name, Qt::MatchExactly | Qt::MatchRecursive);
            for (QTreeWidgetItem *currentItem : items) {
                if (currentItem->text(0) == currentMessage->name) {
                    currentItem->setText(0, text);
                }
            }

            currentSignal->name = text;
        }
    });

    // Connect signalDescLineEdit to handle description changes
    connect(signalDescLineEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (currentSignal) {
            currentSignal->description = text;
        }
    });

    // Connect startBitSpinBox to handle start bit changes
    connect(startBitSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
        if (currentSignal) {
            currentSignal->startBit = value;
        }
    });

    // Connect bitLengthSpinBox to handle bit length changes
    connect(bitLengthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
        if (currentSignal) {
            currentSignal->bitLength = value;
        }
    });

    // Connect isBigEndianCheckBox to handle endianness changes
    connect(isBigEndianCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        if (currentSignal) {
            currentSignal->isBigEndian = checked;
        }
    });

    // Connect isTwosComplementCheckBox to handle two's complement changes
    connect(isTwosComplementCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        if (currentSignal) {
            currentSignal->isTwosComplement = checked;
        }
    });

    // Connect factorSpinBox to handle factor changes
    connect(factorSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        if (currentSignal) {
            currentSignal->factor = value;
        }
    });

    // Connect offsetSpinBox to handle offset changes
    connect(offsetSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        if (currentSignal) {
            currentSignal->offset = value;
        }
    });

    // Connect unitsLineEdit to handle unit changes
    connect(unitsLineEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (currentSignal) {
            currentSignal->units = text;
        }
    });

    // Show the Signal tab
    if (rightPanel->indexOf(signalTab) == -1) {
        rightPanel->addTab(signalTab, "Signal");
    }

    rightPanel->setCurrentWidget(signalTab);
}


void MainWindow::handleNetworkItem(QTreeWidgetItem* item, const QString& name, const QString& modelName)
{
    DbcDataModel* model = nullptr;
    // Find the corresponding model
    for (DbcDataModel* m : dbcModels) {
        if (m->fileName().trimmed() == modelName) {
            model = m;
            break;
        }
    }

    if (!model) {
        QMessageBox::warning(this, "Error", "Model not found.");
        return;
    }

    QList<Network>& networks = model->networks();
    if (networks.isEmpty()) {
        qCritical() << "No networks found in the model!";
        return;
    }

    // Find the network by name
    Network* network = nullptr;
    for (Network& net : networks) {
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

    currentNetwork = network;

    // Populate the network tab fields
    networkNameLineEdit->setText(network->name);
    baudRateLineEdit->setText(network->baud);

    // Update attributes table
    networkAttributesTable->setRowCount(0);
    for(const Attribute& attribute : network->networkAttributes) {
        addAttributeRow(networkAttributesTable, { attribute.name, attribute.type, attribute.value });
    }

    // Connect Network Name to handle name changes
    connect(networkNameLineEdit, &QLineEdit::textChanged, this, [this, item](const QString &text) {
        if (currentNetwork && !text.isEmpty() && text != currentNetwork->name) {
            // Check if the new name is unique in the network list
            bool unique = true;
            for (DbcDataModel* model : dbcModels) {
                for (Network& net : model->networks()) {
                    if (net.name == text) {
                        unique = false;
                        break;
                    }
                }
                if(!unique) {
                    break;
                }
            }
            if (!unique) {
                QMessageBox::warning(this, "Name Conflict", "The network name must be unique. Please choose a different name.");
                networkNameLineEdit->setText(currentNetwork->name); // Revert to the original name
                return;
            }

            // Update the name in the data model
            QString oldName = currentNetwork->name;
            currentNetwork->name = text;

            // Update the tree item for the current network node
            item->setText(0, text);

            // Update all references to the old network name in the tree widget
            QList<QTreeWidgetItem *> items = dbcTree->findItems(oldName, Qt::MatchExactly | Qt::MatchRecursive);
            for (QTreeWidgetItem *currentItem : items) {
                if (currentItem->text(0) == oldName) {
                    currentItem->setText(0, text);
                }
            }

            // Update all nodes that reference this network
            for (DbcDataModel* model : dbcModels) {
                for (Node& node : model->nodes()) {
                    for (NodeNetworkAssociation& association : node.networks) {
                        if (association.networkName == oldName) {
                            association.networkName = text;
                        }
                    }
                }
            }
        }
    });

    // Connect Baud Rate to handle rate changes
    connect(baudRateLineEdit, &QLineEdit::textChanged, this, [this, item](const QString &text) {
        if (currentNetwork) {
            currentNetwork->baud = text;
        }
    });

    // Show the Network tab
    if (rightPanel->indexOf(networkTab) == -1) {
        rightPanel->addTab(networkTab, "Network");
    }
    rightPanel->setCurrentWidget(networkTab);
}


void MainWindow::handleNodeItem(QTreeWidgetItem* item, const QString& name, const QString& modelName)
{
    DbcDataModel* model = nullptr;
    for (DbcDataModel* m : dbcModels) {
        if (m->fileName() == modelName) {
            model = m;
            break;
        }
    }

    if (!model) {
        QMessageBox::warning(this, "Error", "Model not found.");
        return;
    }

    Node* node = nullptr;
    for (Node& n : model->nodes()) {
        if (n.name.trimmed() == name) {
            node = &n;
            break;
        }
    }

    if (!node) {
        QMessageBox::warning(this, "Error", "Node not found in the model.");
        return;
    }

    currentNode = node;

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

    // Connect Node Name to handle name changes
    connect(nodeNameLineEdit, &QLineEdit::textChanged, this, [this, item](const QString &text) {
        if (currentNode) {
            // Update the name in the data model
            QString oldName = currentNode->name;
            currentNode->name = text;

            // Update the tree item for the current node
            item->setText(0, text);

            // Update all references to the old node name in the tree widget
            QList<QTreeWidgetItem *> items = dbcTree->findItems(oldName, Qt::MatchExactly | Qt::MatchRecursive);
            for (QTreeWidgetItem *currentItem : items) {
                if (currentItem->text(0) == oldName) {
                    currentItem->setText(0, text);
                }
            }

            // Update the transmitters and receivers tables in messages
            for (DbcDataModel* model : dbcModels) {
                for (Message& message : model->messages()) {
                    for (auto& transmitter : message.messageTransmitters) {
                        if (transmitter.first == oldName) {
                            transmitter.first = text;
                        }
                    }

                    for (auto& receiver : message.messageReceivers) {
                        if (receiver.first == oldName) {
                            receiver.first = text;
                        }
                    }
                }
            }
        }
    });

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

    // Initialize selections
    currentNetwork = nullptr;
    currentNode = nullptr;
    currentMessage = nullptr;
    currentSignal = nullptr;
    currentTreeItem = nullptr;

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
    // Multiplexer selection dropdown
    multiplexerComboBox = new QComboBox;

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
    definitionFormLayout->addRow("Message Name:", nameLineEdit);
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
    layoutFormLayout->addRow("Multiplexer:", multiplexerComboBox);
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
    signalFormLayout->addRow("Signal Name:", signalNameLineEdit);
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
    // Disconnect Network UI fields
    disconnect(networkNameLineEdit, &QLineEdit::textChanged, nullptr, nullptr);
    disconnect(baudRateLineEdit, &QLineEdit::textChanged, nullptr, nullptr);

    // Disconnect Node UI fields
    disconnect(nodeNameLineEdit, &QLineEdit::textChanged, nullptr, nullptr);

    // Disconnect Message UI fields
    disconnect(pgnLineEdit, &QLineEdit::textChanged, this, nullptr);
    disconnect(nameLineEdit, &QLineEdit::textChanged, this, nullptr);
    disconnect(descLineEdit, &QLineEdit::textChanged, this, nullptr);
    disconnect(prioritySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, nullptr);
    disconnect(lengthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, nullptr);
    disconnect(extendedDataPageCheckBox, &QCheckBox::toggled, this, nullptr);
    disconnect(dataPageCheckBox, &QCheckBox::toggled, this, nullptr);
    disconnect(signalsList, &QListWidget::itemDoubleClicked, this, nullptr);

    // Disconnect Signal UI fields
    disconnect(spnSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), nullptr, nullptr);
    disconnect(signalNameLineEdit, &QLineEdit::textChanged, nullptr, nullptr);
    disconnect(signalDescLineEdit, &QLineEdit::textChanged, nullptr, nullptr);
    disconnect(startBitSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), nullptr, nullptr);
    disconnect(bitLengthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), nullptr, nullptr);
    disconnect(isBigEndianCheckBox, &QCheckBox::toggled, nullptr, nullptr);
    disconnect(isTwosComplementCheckBox, &QCheckBox::toggled, nullptr, nullptr);
    disconnect(factorSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), nullptr, nullptr);
    disconnect(offsetSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), nullptr, nullptr);
    disconnect(unitsLineEdit, &QLineEdit::textChanged, nullptr, nullptr);


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
    disconnect(multiplexerComboBox, nullptr, this, nullptr);
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


void MainWindow::displayBitLayout(Message& message , int selectedMultiplexer = -1) {
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
        // Skip signals if they're multiplexed and not matching the selected multiplexer value
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

