#include <QCoreApplication>
#include "aschandler.h"
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    ASCHandler data(&a,"C:/Users/ege-t/Desktop/diagnose_dbc_v.1.9.dbc");

    return a.exec();
}
