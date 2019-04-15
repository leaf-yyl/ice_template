







#include <QEvent>
#include <QObject>
#include <QCoreApplication>


#include <stdexcept>

#include <Ice/Ice.h>
#include "Printer.h"

using namespace std;

class CustomEvent : public QEvent
{
public:
    explicit CustomEvent(Type type) :QEvent(type) {

    }

    enum PMAlgoEventType {
        CustomEvent_RequireService = 0x00000001,
    };

    string m_params;
    Demo::ClientCallbackPrxPtr m_callback;
};

class ServerI : public Demo::ServerService
{
public:
    ServerI(){}

    /* Use event loop implement in QObject by Qt to post client requirements to user thread.
     * May be replaced by event loop in pure C++, handler in java and so on.
     */
    void setImplement(QObject *implement) {
        m_implement = implement;
    }

    void requireService(string ident, string s, const ::Ice::Current& current) override
    {
        /* we donot generate the client requirement here, but post it to main thread as this function is called in ice server threads.
         * When the interface is not thread safe or time-consuming, or has thread associated context like python interpreter, we must post
         * it to a constant thread managered by ourself to avoid running exceptions.
         * ident : client object identification, used to build bidirectional connection to cross firewall and local network
         * s : params used for servcie
         */
        CustomEvent *e = new CustomEvent(QEvent::Type(QEvent::User + CustomEvent::CustomEvent_RequireService));
        e->m_params   = s;
        e->m_callback = Ice::uncheckedCast<Demo::ClientCallbackPrx>(current.con->createProxy(Ice::stringToIdentity(ident)));
        QCoreApplication::postEvent(m_implement, e);
    }

private:
    QObject *m_implement;
};

class IceManager
{
public:
    IceManager() {

        /* set up global ice configurations, here we just set thread pool to 2 to avoid deadlock on ice callback,
         * other settings are configurable as the same.
         */
        Ice::PropertiesPtr props0 = Ice::createProperties();
        props0->setProperty("Ice.ThreadPool.Server.Size", "2");
        props0->setProperty("Ice.ThreadPool.Server.SizeMax", "2");
        props0->setProperty("Ice.ThreadPool.Client.Size", "2");
        props0->setProperty("Ice.ThreadPool.Client.SizeMax", "2");
        props0->setProperty("Ice.Trace.ThreadPool", "1");

        Ice::InitializationData id;
        id.properties = props0;
        m_ich = Ice::CommunicatorHolder(id);
    }

    bool setImplement(QObject *implement) {
        try {
            /* create server object and add it to ice adapter to receive client requirements.
             * The adapter identification is used for ice pack service, we donot use it now.
             * The endpoints is the location url where client can access to require service.
             * The servant identification is used to identify servant as we can add multiple servants to one adapter.
             */
            shared_ptr<ServerI> servant = make_shared<ServerI>();
            servant->setImplement(implement);
            auto adapter = m_ich->createObjectAdapterWithEndpoints("ServerAdapter", "default -h localhost -p 10000");
            adapter->add(servant, Ice::stringToIdentity("Server"));
            adapter->activate();
        } catch (const exception &e)
        {
            cout << "Failed to create ice server object, error-->%s" << e.what() << endl;
            return false;
        }

        return true;
    }

private:
    Ice::CommunicatorHolder m_ich;
};

class MainWorker : public QObject
{
public:
    MainWorker(QObject *parent = nullptr) : QObject(parent) {}

protected:

    /* Receive client requirements and deliver to associated servcie function */
    void customEvent(QEvent *e) {
        CustomEvent *event = (CustomEvent *)e;

        int type = event->type() - QEvent::User;
        if (CustomEvent::CustomEvent_RequireService == type) {
            renderService(event->m_params, event->m_callback);
        } else {
            cout << "Unrecognized event type-->" << type << "!" << endl;
        }
    }

private:
    /* a simple implement */
    void renderService(const string& s, const Demo::ClientCallbackPrxPtr& callback) {
        cout << s << endl;
        callback->callback("Requirement done!");
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    MainWorker worker(&a);

    IceManager ice_manager;
    ice_manager.setImplement(&worker);

    return a.exec();
}
