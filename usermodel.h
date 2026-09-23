#ifndef USERMODEL_H
#define USERMODEL_H

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QDateTime>

struct User {
    QString name;
    int age;
    QDateTime created;
};

class UserModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles { NameRole = Qt::UserRole + 1, AgeRole, CreatedRole };
    Q_ENUM(Roles)

    explicit UserModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // --- CRUD ---
    Q_INVOKABLE bool addUser(const QString &name, int age);
    Q_INVOKABLE bool updateUser(int row, const QString &name, int age);
    Q_INVOKABLE void removeUser(int row);
    Q_INVOKABLE bool hasUser(const QString &name, int exceptRow = -1) const;

    // --- JSON ---
    Q_INVOKABLE bool saveToFile(const QString &path);
    Q_INVOKABLE bool loadFromFile(const QString &path);

    // --- CSV ---
    Q_INVOKABLE bool exportToCsv(const QString &path);
    Q_INVOKABLE bool importFromCsv(const QString &path);

    // --- Сортировка ---
    enum SortField { SortByName, SortByAge, SortByDate };
    Q_ENUM(SortField)
    Q_INVOKABLE void sortBy(int field, bool ascending = true);

    // --- Автосохранение ---
    Q_PROPERTY(QString autoSavePath READ autoSavePath WRITE setAutoSavePath NOTIFY autoSavePathChanged)
    QString autoSavePath() const;
    void setAutoSavePath(const QString &path);
    Q_INVOKABLE bool autoSave();

signals:
    void autoSavePathChanged();
    void errorOccurred(const QString &message);
    void infoOccurred(const QString &message);

private:
    QVector<User> m_users;
    QString m_autoSavePath;
};

// ------------------ Прокси для фильтрации по имени ------------------
class UserFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)

public:
    explicit UserFilterModel(QObject *parent = nullptr);

    QString filterText() const;
    void setFilterText(const QString &text);

signals:
    void filterTextChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QString m_filterText;
};

#endif // USERMODEL_H