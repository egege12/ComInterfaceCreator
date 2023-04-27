#ifndef LISTMODEL_H
#define LISTMODEL_H

#include <QAbstractListModel>

class listmodel : public QAbstractListModel
{
    Q_OBJECT
    enum ListRoles{
        ListDataRole = Qt::UserRole+1,
        Selected,
        GenerationInfos,
        Clickable,
        ImpWidth
    };
public:
    explicit listmodel(QObject *parent = nullptr);
    QList<QString> list;
    // Header:

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    virtual QHash<int,QByteArray> roleNames() const override;

private:

signals:
    void listChanged();

public slots:
    void setList(QList<QString> list);
};

#endif // LISTMODEL_H
