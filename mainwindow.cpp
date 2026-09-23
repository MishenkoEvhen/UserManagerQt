#include "mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    setWindowTitle("Менеджер пользователей");
    resize(700, 500);
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi()
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    // --- Форма ввода ---
    QGroupBox *inputGroup = new QGroupBox("Добавить пользователя", central);
    QFormLayout *formLayout = new QFormLayout(inputGroup);

    nameEdit = new QLineEdit(inputGroup);
    nameEdit->setPlaceholderText("Введите имя...");

    ageSpin = new QSpinBox(inputGroup);
    ageSpin->setRange(1, 150);
    ageSpin->setValue(18);

    formLayout->addRow("Имя:", nameEdit);
    formLayout->addRow("Возраст:", ageSpin);

    // --- Кнопки ---
    addButton    = new QPushButton("Добавить", central);
    deleteButton = new QPushButton("Удалить выбранного", central);
    saveButton   = new QPushButton("Сохранить в файл", central);
    loadButton   = new QPushButton("Загрузить из файла", central);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(addButton);
    btnLayout->addWidget(deleteButton);
    btnLayout->addStretch();
    btnLayout->addWidget(saveButton);
    btnLayout->addWidget(loadButton);

    // --- Таблица ---
    table = new QTableWidget(0, 3, central);
    table->setHorizontalHeaderLabels({"Имя", "Возраст", "Дата и время записи"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // --- Статус ---
    statusLabel = new QLabel("Готово", central);

    // --- Компоновка ---
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->addWidget(inputGroup);
    mainLayout->addLayout(btnLayout);
    mainLayout->addWidget(table);
    mainLayout->addWidget(statusLabel);

    // --- Подключения ---
    connect(addButton,    &QPushButton::clicked, this, &MainWindow::addUser);
    connect(deleteButton, &QPushButton::clicked, this, &MainWindow::deleteUser);
    connect(saveButton,   &QPushButton::clicked, this, &MainWindow::saveToFile);
    connect(loadButton,   &QPushButton::clicked, this, &MainWindow::loadFromFile);
    connect(nameEdit,     &QLineEdit::returnPressed, this, &MainWindow::addUser);
}

void MainWindow::addUser()
{
    QString name = nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите имя пользователя!");
        return;
    }

    int age = ageSpin->value();
    QString dateTime = QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm:ss");

    int row = table->rowCount();
    table->insertRow(row);
    table->setItem(row, 0, new QTableWidgetItem(name));
    table->setItem(row, 1, new QTableWidgetItem(QString::number(age)));
    table->setItem(row, 2, new QTableWidgetItem(dateTime));

    nameEdit->clear();
    ageSpin->setValue(18);
    nameEdit->setFocus();

    statusLabel->setText(QString("Добавлен пользователь: %1").arg(name));
}

void MainWindow::deleteUser()
{
    int row = table->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "Инфо", "Выберите строку для удаления.");
        return;
    }
    table->removeRow(row);
    statusLabel->setText("Запись удалена");
}

void MainWindow::saveToFile()
{
    QString fileName = QFileDialog::getSaveFileName(
        this, "Сохранить файл", "users.txt", "Текстовые файлы (*.txt);;Все файлы (*)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось открыть файл для записи!");
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    for (int i = 0; i < table->rowCount(); ++i) {
        QString name = table->item(i, 0)->text();
        QString age  = table->item(i, 1)->text();
        QString dt   = table->item(i, 2)->text();
        // разделитель "|"
        out << name << "|" << age << "|" << dt << "\n";
    }

    file.close();
    statusLabel->setText("Сохранено в " + fileName);
    QMessageBox::information(this, "Успех", "Данные сохранены!");
}

void MainWindow::loadFromFile()
{
    QString fileName = QFileDialog::getOpenFileName(
        this, "Открыть файл", "", "Текстовые файлы (*.txt);;Все файлы (*)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось открыть файл!");
        return;
    }

    table->setRowCount(0);

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.isEmpty()) continue;

        QStringList parts = line.split('|');
        if (parts.size() < 3) continue;

        int row = table->rowCount();
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(parts[0]));
        table->setItem(row, 1, new QTableWidgetItem(parts[1]));
        table->setItem(row, 2, new QTableWidgetItem(parts[2]));
    }

    file.close();
    statusLabel->setText("Загружено из " + fileName);
    QMessageBox::information(this, "Успех", "Данные загружены!");
}