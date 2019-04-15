




#include "helper.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    /* local worker */
    MainWorker worker(&a);

    /* ice service manager */
    IceManager ice_manager;
    ice_manager.start(&worker);

    /* require serve service every 3 seconds */
    QTimer timer;
    QObject::connect(&timer, SIGNAL(timeout()), &ice_manager, SLOT(slot_requireService()));
    timer.start(3 * 1000);

    return a.exec();
}
