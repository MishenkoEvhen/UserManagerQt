#include "usermodel.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTextStream>
#include <algorithm>

UserModel::UserModel(QObject *parent) : QAbstractListModel(parent) {}

int UserModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_users.size();
}

QVariant UserModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_users.size()) return {};
    const User &u = m_users[index.row()];
    switch (role) {
    case NameRole:    return u.name;
    case AgeRole:     return u.age;
    case CreatedRole: return u.created.toString("dd.MM.yyyy HH:mm:ss");
    }
    return {};
}

QHash<int, QByteArray> UserModel::roleNames() const
{
    return {
        { NameRole,    "name" },
        { AgeRole,     "age" },
        { CreatedRole, "created" }
    };
}

// ---------- Проверка дубликатов (с исключением строки при редактировании) ----------
bool UserModel::hasUser(const QString &name, int exceptRow) const
{
    const QString trimmed = name.trimmed();
    for (int i = 0; i < m_users.size(); ++i) {
        if (i == exceptRow) continue;
        if (m_users[i].name.compare(trimmed, Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

// ---------- Добавление ----------
bool UserModel::addUser(const QString &name, int age)
{
    const QString trimmed = name.trimmed();

    if (trimmed.isEmpty()) {
        emit errorOccurred("Имя не может быть пустым!");
        return false;
    }
    if (hasUser(trimmed)) {
        emit errorOccurred(QString("Пользователь \"%1\" уже существует!").arg(trimmed));
        return false;
    }

    beginInsertRows(QModelIndex(), m_users.size(), m_users.size());
    m_users.append({ trimmed, age, QDateTime::currentDateTime() });
    endInsertRows();

    if (!m_autoSavePath.isEmpty()) autoSave();
    return true;
}

// ---------- Редактирование ----------
bool UserModel::updateUser(int row, const QString &name, int age)
{
    if (row < 0 || row >= m_users.size()) {
        emit errorOccurred("Некорректная строка для редактирования.");
        return false;
    }

    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        emit errorOccurred("Имя не может быть пустым!");
        return false;
    }
    if (hasUser(trimmed, row)) {
        emit errorOccurred(QString("Пользователь \"%1\" уже существует!").arg(trimmed));
        return false;
    }

    // Дата создания сохраняется — редактируем только имя и возраст
    m_users[row].name = trimmed;
    m_users[row].age  = age;

    const QModelIndex idx0 = index(row, 0);
    const QModelIndex idx1 = index(row, 1);
    emit dataChanged(idx0, idx1, { NameRole, AgeRole });

    if (!m_autoSavePath.isEmpty()) autoSave();
    return true;
}

// ---------- Удаление ----------
void UserModel::removeUser(int row)
{
    if (row < 0 || row >= m_users.size()) return;
    beginRemoveRows(QModelIndex(), row, row);
    m_users.remove(row);
    endRemoveRows();

    if (!m_autoSavePath.isEmpty()) autoSave();
}

// ---------- Сортировка ----------
void UserModel::sortBy(int field, bool ascending)
{
    if (m_users.isEmpty()) return;

    beginResetModel();

    std::stable_sort(m_users.begin(), m_users.end(),
                     [field, ascending](const User &a, const User &b) {
                         bool less = false;
                         switch (field) {
                         case SortByName: less = a.name.compare(b.name, Qt::CaseInsensitive) < 0; break;
                         case SortByAge:  less = a.age < b.age;                                   break;
                         case SortByDate: less = a.created < b.created;                           break;
                         }
                         return ascending ? less : !less;
                     });

    endResetModel();

    if (!m_autoSavePath.isEmpty()) autoSave();
}

// ---------- JSON: сохранение ----------
bool UserModel::saveToFile(const QString &path)
{
    QJsonArray arr;
    for (const User &u : m_users) {
        QJsonObject obj;
        obj["name"]    = u.name;
        obj["age"]     = u.age;
        obj["created"] = u.created.toString(Qt::ISODate);
        arr.append(obj);
    }

    QJsonObject root;
    root["users"] = arr;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit errorOccurred(QString("Не удалось открыть файл для записи: %1").arg(path));
        return false;
    }
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

// ---------- JSON: загрузка ----------
bool UserModel::loadFromFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit errorOccurred(QString("Не удалось открыть файл: %1").arg(path));
        return false;
    }

    const QByteArray data = f.readAll();
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        emit errorOccurred("Ошибка парсинга JSON: " + err.errorString());
        return false;
    }

    const QJsonArray arr = doc.object().value("users").toArray();

    beginResetModel();
    m_users.clear();
    for (const QJsonValue &v : arr) {
        const QJsonObject obj = v.toObject();
        User u;
        u.name    = obj.value("name").toString();
        u.age     = obj.value("age").toInt();
        u.created = QDateTime::fromString(obj.value("created").toString(), Qt::ISODate);
        if (!u.name.isEmpty())
            m_users.append(u);
    }
    endResetModel();
    return true;
}

// ---------- CSV: экспорт ----------
bool UserModel::exportToCsv(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit errorOccurred(QString("Не удалось открыть файл для записи: %1").arg(path));
        return false;
    }

    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out << "\xEF\xBB\xBF"; // BOM для Excel
    out << "Имя;Возраст;Дата и время\n";

    auto escape = [](QString s) {
        s.replace('"', "\"\"");
        if (s.contains(';') || s.contains('"') || s.contains('\n'))
            s = "\"" + s + "\"";
        return s;
    };

    for (const User &u : m_users) {
        out << escape(u.name) << ";"
            << u.age << ";"
            << u.created.toString("dd.MM.yyyy HH:mm:ss") << "\n";
    }
    return true;
}

// ---------- CSV: импорт ----------
bool UserModel::importFromCsv(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit errorOccurred(QString("Не удалось открыть файл: %1").arg(path));
        return false;
    }

    QTextStream in(&f);
    in.setEncoding(QStringConverter::Utf8);

    int imported = 0;
    int skipped  = 0;
    bool firstLine = true;

    beginResetModel();
    // Режим добавления — не очищаем существующие, но при желании можно
    // m_users.clear(); раскомментировать, чтобы полностью заменять.

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.isEmpty()) continue;

        // Снимаем BOM у первой строки
        if (firstLine) {
            if (line.startsWith(QChar(0xFEFF))) line.remove(0, 1);
            firstLine = false;
            // Пропускаем заголовок
            if (line.startsWith("Имя") || line.startsWith("name", Qt::CaseInsensitive))
                continue;
        }

        // Разбор CSV-строки с поддержкой кавычек
        QStringList fields;
        QString cur;
        bool inQuotes = false;
        for (int i = 0; i < line.size(); ++i) {
            QChar c = line[i];
            if (inQuotes) {
                if (c == '"') {
                    if (i + 1 < line.size() && line[i + 1] == '"') { cur += '"'; ++i; }
                    else inQuotes = false;
                } else cur += c;
            } else {
                if (c == '"') inQuotes = true;
                else if (c == ';' || c == ',') { fields << cur; cur.clear(); }
                else cur += c;
            }
        }
        fields << cur;

        if (fields.size() < 2) { ++skipped; continue; }

        const QString name = fields[0].trimmed();
        const int age = fields[1].toInt();

        if (name.isEmpty() || age <= 0 || hasUser(name)) { ++skipped; continue; }

        User u;
        u.name = name;
        u.age  = age;
        if (fields.size() >= 3) {
            u.created = QDateTime::fromString(fields[2].trimmed(), "dd.MM.yyyy HH:mm:ss");
        }
        if (!u.created.isValid())
            u.created = QDateTime::currentDateTime();

        m_users.append(u);
        ++imported;
    }

    endResetModel();

    if (!m_autoSavePath.isEmpty()) autoSave();

    emit infoOccurred(QString("Импортировано: %1, пропущено: %2").arg(imported).arg(skipped));
    return imported > 0;
}

// ---------- Автосохранение ----------
QString UserModel::autoSavePath() const { return m_autoSavePath; }

void UserModel::setAutoSavePath(const QString &path)
{
    if (m_autoSavePath == path) return;
    m_autoSavePath = path;
    emit autoSavePathChanged();
}

bool UserModel::autoSave()
{
    if (m_autoSavePath.isEmpty()) return false;
    return saveToFile(m_autoSavePath);
}

// ============================================================
//                        UserFilterModel
// ============================================================

UserFilterModel::UserFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setDynamicSortFilter(true);
}

QString UserFilterModel::filterText() const { return m_filterText; }

void UserFilterModel::setFilterText(const QString &text)
{
    if (m_filterText == text) return;
    m_filterText = text;
    // Фильтр по колонке имени (0)
    setFilterFixedString(text);
    setFilterKeyColumn(0);
    emit filterTextChanged();
    invalidateFilter();
}

bool UserFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (m_filterText.isEmpty()) return true;

    const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
    if (!idx.isValid()) return false;

    const QString name = idx.data(Qt::DisplayRole).toString();
    return name.contains(m_filterText, Qt::CaseInsensitive);
}