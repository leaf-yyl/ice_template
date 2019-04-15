




#ifndef HELPER_H
#define HELPER_H


#include <QTimer>
#include <QEvent>
#include <QObject>
#include <QThread>
#include <QCoreApplication>


#include <stdexcept>

#include <Ice/Ice.h>
#include "Printer.h"

using namespace std;

/* custom event used for event loop */
class CustomEvent : public QEvent
{
public:
    explicit CustomEvent(Type type) :QEvent(type) {}

    enum PMAlgoEventType {
        CustomEvent_RequireCallback = 0x00000001,
    };

    string m_params;
};

class ClientI : public Demo::ClientCallback
{
public:
    ClientI(){}

    /* Use event loop implement in QObject by Qt to post client requirements to user thread.
     * May be replaced by event loop in pure C++, handler in java and so on.
     */
    void setImplement(QObject *implement) {
        m_server_implement = implement;
    }

    void callback(string s, const ::Ice::Current&) override
    {
        /* we donot generate the client requirement here, but post it to main thread as this function is called in ice server threads.
         * When the interface is not thread safe or time-consuming, or has thread associated context like python interpreter, we must post
         * it to a constant thread managered by ourself to avoid running exceptions.
         * s : params used for servcie
         */
        CustomEvent *e = new CustomEvent(QEvent::Type(QEvent::User + CustomEvent::CustomEvent_RequireCallback));
        e->m_params    = s;
        QCoreApplication::postEvent(m_server_implement, e);
    }

private:
    QObject *m_server_implement;
};

class ServerRequirer : public QObject
{
    Q_OBJECT
public:
    explicit ServerRequirer(const Ice::CommunicatorHolder &ich, const Ice::ObjectAdapterPtr &adapter, const string& ident) {
        m_ic        = ich.communicator();
        m_adapter   = adapter;
        m_ident     = ident;
    }

public slots:
    void slot_requireServcie(const string& s) {
        if (nullptr == m_server_prx.get()) {
            /* connection is not established, try to establish connection first */
            try {
                auto base = m_ic->stringToProxy("Server:default -h localhost -p 10000");
                m_server_prx = Ice::checkedCast<Demo::ServerServicePrx>(base);
                if(nullptr != m_server_prx.get())
                {
                    /* set up eseential configurations on Ice connection to keep this connection alive */
                    m_server_prx->ice_getConnection()->setACM(Ice::nullopt, Ice::ACMClose::CloseOff, Ice::ACMHeartbeat::HeartbeatAlways);
                    m_server_prx->ice_getConnection()->setAdapter(m_adapter);
                }
            }
            catch(const exception& e)
            {
                cerr << e.what() << endl;
            }
        }

        if (nullptr != m_server_prx.get()) {
            try {
                /* require remote object call via object proxy */
                m_server_prx->requireService(m_ident, s);
            }
            catch(const exception& e)
            {
                /* connection lost, reset it and re-establish it on next call */
                cerr << e.what() << endl;
                m_server_prx.reset();
            }
        }
    }

private:
    string m_ident;
    Ice::CommunicatorPtr      m_ic;
    Ice::ObjectAdapterPtr     m_adapter;
    Demo::ServerServicePrxPtr m_server_prx;
};

class IceManager : public QThread
{
    Q_OBJECT
public:
    /* Craete ice global communicator and manager ice requirement.
     * Server requirements are posted in seperate thread as they may be time-consuming
     * Local object that implements callback is identified through bidirectional connection instead of
     * creating a new connection fron server to client, as client may be defensed behind firewall or local network.
     */
    IceManager() {

        /* register qt meta type */
        qRegisterMetaType<string>("string");

        /* set up global ice configurations, here we just set thread pool to 2 to avoid deadlock on ice callback,
         * other settings are configurable as the same.
         */
        Ice::PropertiesPtr props0 = Ice::createProperties();
        props0->setProperty("Ice.ThreadPool.Server.Size", "2");
        props0->setProperty("Ice.ThreadPool.Server.SizeMax", "2");
        props0->setProperty("Ice.ThreadPool.Client.Size", "2");
        props0->setProperty("Ice.ThreadPool.Client.SizeMax", "2");
        props0->setProperty("Ice.Trace.ThreadPool", "1");

        /* create global ice communicator */
        Ice::InitializationData id;
        id.properties = props0;
        m_ich = Ice::CommunicatorHolder(id);
    }

    bool start(QObject *implement) {

        try {
            /* create client object and add it to ice adapter to receive server callbacks.
             * The adapter is created without identification as we donot access it by endpoints.
             * Instead, we pass it through the connection established with server to build a bidirectional connection.
             */
            m_ident = "Client";
            shared_ptr<ClientI> servant = make_shared<ClientI>();
            servant->setImplement(implement);
            m_adapter = m_ich->createObjectAdapter("");
            m_adapter->add(servant, Ice::stringToIdentity(m_ident));
            m_adapter->activate();
        } catch (const exception &e) {
            cout << "Failed to create ice object, error-->%s" << e.what() << endl;
            return false;
        }

        /* create ice service requirer in seperate thread, as network request may be time-consuming */
        m_requirer = new ServerRequirer(m_ich, m_adapter, m_ident);
        m_requirer->moveToThread(this);
        connect(this, SIGNAL(signal_requireService(string)), m_requirer, SLOT(slot_requireServcie(string)));

        QThread::start();
        return true;
    }

public slots:
    void slot_requireService() {
        string s("Hello world!");
        emit signal_requireService(s);
    }

signals:
    void signal_requireService(string s);

private:
    Ice::CommunicatorHolder m_ich;
    Ice::ObjectAdapterPtr   m_adapter;

    string          m_ident;        /* local object identification */
    ServerRequirer *m_requirer;
};

class MainWorker : public QObject
{
public:
    MainWorker(QObject *parent = nullptr) : QObject(parent) {}

protected:

    /* Receive callbacks from server and deliver to associated callback function */
    void customEvent(QEvent *e) {
        CustomEvent *event = (CustomEvent *)e;

        int type = event->type() - QEvent::User;
        if (CustomEvent::CustomEvent_RequireCallback == type) {
            renderCallback(event->m_params);
        } else {
            cout << "Unrecognized event type-->" << type << "!" << endl;
        }
    }

private:
    /* a simple implement */
    void renderCallback(const string& s) {
        cout << s << endl;
    }
};

#endif // HELPER_H
