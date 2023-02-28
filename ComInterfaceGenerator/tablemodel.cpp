#include "tablemodel.h"
#include <QtAlgorithms>
unsigned int tablemodel::scolumnID = 1;
unsigned int tablemodel::lastColumnID = 0;
QVector<bool> tablemodel::columnNumber ={false,false,false,false,false,false,false,false};
bool tablemodel::variableLessThan(const QList<QString>& a, const QList<QString>& b)
{
    bool isNumber1,isNumber2;
    a.at(tablemodel::scolumnID).toDouble(&isNumber1);
    b.at(tablemodel::scolumnID).toDouble(&isNumber2);
    if(isNumber1 && isNumber2)
        return a.at(tablemodel::scolumnID).toDouble()< b.at(tablemodel::scolumnID).toDouble();
    else
        return a.at(tablemodel::scolumnID)< b.at(tablemodel::scolumnID);
}

bool tablemodel::variableHigherThan(const QList<QString> &a, const QList<QString> &b)
{
    bool isNumber1,isNumber2;
    a.at(tablemodel::scolumnID).toDouble(&isNumber1);
    b.at(tablemodel::scolumnID).toDouble(&isNumber2);
    if(isNumber1 && isNumber2)
        return a.at(tablemodel::scolumnID).toDouble()> b.at(tablemodel::scolumnID).toDouble();
    else
        return a.at(tablemodel::scolumnID)> b.at(tablemodel::scolumnID);
}

tablemodel::tablemodel(QAbstractTableModel *parent)
    : QAbstractTableModel{parent}
{
    table.append({" "," "," "," "});
}

int tablemodel::rowCount(const QModelIndex &) const
{
    return table.size();
}

int tablemodel::columnCount(const QModelIndex &) const
{
    return table.at(0).size();
}

QVariant tablemodel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case TableDataRole:
        return table.at(index.row()).at(index.column());
    case MessageID:
        return table.at(index.row()).at(2);
    case MessageName:
    {
        if(table.at(0).at(0) =="Name")
            return table.at(index.row()).at(0);
        else
            return table.at(index.row()).at(1);
    }
    case SelectionColumn:
        return (index.column()==0);
    case Selected:
        return (table.at(index.row()).at(0)== "X");
    case ActiveSortHeader:
        return index.column() ==tablemodel::lastColumnID;
    case SortHeader:
        return (index.row() ==0 ) && tablemodel::columnNumber.at(tablemodel::lastColumnID) && index.column() ==tablemodel::lastColumnID;
    case HeadingRole:
        if (index.row() ==0 ){
                tablemodel::scolumnID=index.column();
                return true;
        }else{
                return false;
        }
        break;

     default:
        break;
    }

    return QVariant();
}

QHash<int, QByteArray> tablemodel::roleNames() const
{
    QHash<int,QByteArray> roles;
    roles[TableDataRole] = "tabledata";
    roles[HeadingRole] = "heading";
    roles[MessageID] = "messageid";
    roles[MessageName] = "messagename";
    roles[Selected] = "selected";
    roles[SortHeader]="sortheader";
    roles[ActiveSortHeader]="activesortheader";
    roles[SelectionColumn]="selectioncolumn";
    return roles;
}

QList<QList<QString> > tablemodel::getTable()
{
    return table;
}

void tablemodel::setTable(QList<QList<QString> > table)
{
    beginResetModel();
    this->table.clear();
    qInfo()<<"table appended";
    this->table.append(table);
    endResetModel();
    emit this->tableChanged();
    this->tablebackup = this->table;
}

void tablemodel::updateTable(QList<QList<QString> > table) /*CID#0007*/
{
  beginResetModel();
  QList<QList<QString>>::iterator thisRow;
  QList<QList<QString>>::iterator inRow;
   /*apply changes to table without sorting*/
  for(inRow=table.begin();inRow!=table.end();inRow++){
    for(thisRow=this->table.begin();thisRow!=this->table.end();thisRow++){
        if(inRow->at(1)==thisRow->at(1)){
            *thisRow=*inRow;
        }
    }
  }
  /*apply changes to backup table for search ability*/
  for(inRow=table.begin();inRow!=table.end();inRow++){
    for(thisRow=this->tablebackup.begin();thisRow!=this->tablebackup.end();thisRow++){
        if(inRow->at(1)==thisRow->at(1)){
            *thisRow=*inRow;
        }
    }
  }
  endResetModel();
}

void tablemodel::sortColumn()
{
    beginResetModel();
    if(tablemodel::columnNumber.at(tablemodel::scolumnID)){
        std::sort(table.begin()+1,table.end(),variableLessThan);
    }else{
        std::sort(table.begin()+1,table.end(),variableHigherThan);
    }
    tablemodel::columnNumber[tablemodel::scolumnID]= !tablemodel::columnNumber[tablemodel::scolumnID];
    tablemodel::lastColumnID = tablemodel::scolumnID;
    endResetModel();
}
void tablemodel::sortColumnPrivate()
{
    beginResetModel();
    if(tablemodel::columnNumber.at(tablemodel::scolumnID)){
        std::sort(table.begin()+1,table.end(),variableLessThan);
    }else{
        std::sort(table.begin()+1,table.end(),variableHigherThan);
    }
    tablemodel::scolumnID = tablemodel::lastColumnID;
    endResetModel();
}

void tablemodel::search(QString text)
{
    QList<QList<QString>>::Iterator rowItr;
    QList<QString>::Iterator columnItr;
    QList<QList<QString>> searchTable;
    searchTable.append(tablebackup.at(0));
    if(text.isEmpty()){
        beginResetModel();
        table=tablebackup;
        endResetModel();
    }else{
        for(rowItr=tablebackup.begin();rowItr!=tablebackup.end();rowItr++){
            for(columnItr=rowItr->begin();columnItr!=rowItr->end();columnItr++){
                if(columnItr->contains(text,Qt::CaseInsensitive)){
                    searchTable.append(*rowItr);
                    break;
                }
            }
        }
        beginResetModel();
        table=searchTable;
        endResetModel();
    }


}

