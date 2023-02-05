#include <QCoreApplication>
#include "DBChandler.h"
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    DBCHandler data(&a,"C:/Users/egementurker/Desktop/PR1003_DBC_ECU_Interface_v33.dbc");
    data.printMessages();

    return a.exec();
}
