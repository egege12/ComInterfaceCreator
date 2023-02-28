#ifndef TABLEMODEL_H
#define TABLEMODEL_H

#include <QObject>
#include <QAbstractTableModel>
#include <QDebug>

class tablemodel : public QAbstractTableModel
{
    Q_OBJECT
    enum TableRoles{
        TableDataRole = Qt::UserRole+1,
        HeadingRole,
        MessageID,
        MessageName,
        Selected,
        SortHeader,
        ActiveSortHeader
    };
    static unsigned int scolumnID;
    static unsigned int lastColumnID;
    static QVector<bool> columnNumber;
    static bool variableLessThan(const QList<QString>& a, const QList<QString>& b);
    static bool variableHigherThan(const QList<QString>& a, const QList<QString>& b);

public:

Q_PROPERTY(QList<QList<QString>> table READ getTable WRITE setTable NOTIFY tableChanged)


    QList<QList<QString>> table;

    explicit tablemodel(QAbstractTableModel *parent = nullptr);

    int rowCount(const QModelIndex & = QModelIndex()) const override;

    int columnCount(const QModelIndex & = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    QHash<int, QByteArray> roleNames() const override;

    QList<QList<QString>> getTable();

signals:
    void tableChanged();
public slots:
    void setTable(QList<QList<QString>> table);
    void updateTable(QList<QList<QString>> table);
    void sortColumn();
    void sortColumnPrivate();
};

#endif // TABLEMODEL_H
