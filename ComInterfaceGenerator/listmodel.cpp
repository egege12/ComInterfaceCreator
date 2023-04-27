#include "listmodel.h"

listmodel::listmodel(QObject *parent)
    : QAbstractListModel(parent)
{
    list.append("Henüz işlenmedi");
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
    case GenerationInfos:
        return list.at(index.row()).contains("oluşturuldu");
    case ImpWidth:
        return list.at(index.row()).length();
    case Clickable:
        return ((list.at(index.row()).contains("**")) || (list.at(index.row()).contains("mesajları")));
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
    roles[GenerationInfos] = "generationinfo";
    roles[Clickable] = "clickable";
    roles[ImpWidth]="impwidth";

    return roles;
}

void listmodel::setList(QList<QString> list)
{
    beginResetModel();
    this->list.clear();
    std::reverse(list.begin(),list.end());
    this->list.append(list);
    endResetModel();
    emit this->listChanged();
}
