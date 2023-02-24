#include "listmodel.h"

listmodel::listmodel(QObject *parent)
    : QAbstractListModel(parent)
{
    list.append("Warnings");
}


int listmodel::rowCount(const QModelIndex &parent) const
{
    return list.size();
}

QVariant listmodel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case ListDataRole:
        return list.at(index.row());
     default:
        break;
    }
    return QVariant();
}

QHash<int, QByteArray> listmodel::roleNames() const
{
    QHash<int,QByteArray> roles;
    roles[ListDataRole] = "listdata";
    roles[Selected] = "selected";

    return roles;
}

void listmodel::setList(QList<QString> list)
{
    beginResetModel();
    this->list.clear();
    qInfo()<<"list appended";
    this->list.append(list);
    endResetModel();
    emit this->listChanged();
}
