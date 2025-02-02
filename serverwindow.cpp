#include "ServerWindow.h"
#include <QDataStream>
#include <QFileDialog>
#include <QMessageBox>

ServerWindow::ServerWindow(QWidget *parent)
    : QMainWindow(parent),
    m_tcpServer(nullptr)
{
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    QHBoxLayout* topLayout = new QHBoxLayout();
    m_portEdit = new QLineEdit("12345");
    m_startButton = new QPushButton("Start Server");
    m_statusLabel = new QLabel("Server not running");
    topLayout->addWidget(new QLabel("Port:"));
    topLayout->addWidget(m_portEdit);
    topLayout->addWidget(m_startButton);
    topLayout->addWidget(m_statusLabel);

    m_log = new QTextEdit();
    m_log->setReadOnly(true);

    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(m_log);

    setCentralWidget(centralWidget);

    connect(m_startButton, &QPushButton::clicked, this, &ServerWindow::startServer);
}

ServerWindow::~ServerWindow()
{
    if (m_tcpServer) {
        m_tcpServer->close();
    }

    for (QTcpSocket* client : m_clients) {
        client->disconnectFromHost();
        client->close();
    }
}

void ServerWindow::startServer()
{
    if (m_tcpServer) {
        QMessageBox::information(this, "Info", "Server is already running.");
        return;
    }

    bool ok;
    quint16 port = m_portEdit->text().toUShort(&ok);
    if (!ok) {
        QMessageBox::warning(this, "Error", "Invalid port number.");
        return;
    }

    m_tcpServer = new QTcpServer(this);
    if (!m_tcpServer->listen(QHostAddress::Any, port)) {
        QMessageBox::critical(this, "Error", "Unable to start server: " + m_tcpServer->errorString());
        m_tcpServer->deleteLater();
        m_tcpServer = nullptr;
        return;
    }

    connect(m_tcpServer, &QTcpServer::newConnection, this, &ServerWindow::onNewConnection);

    m_statusLabel->setText(QString("Listening on port %1").arg(port));
    m_log->append("Server started...");
    m_startButton->setEnabled(false);
}

void ServerWindow::onNewConnection()
{
    while (m_tcpServer->hasPendingConnections()) {
        QTcpSocket* clientSocket = m_tcpServer->nextPendingConnection();
        m_clients.append(clientSocket);

        connect(clientSocket, &QTcpSocket::disconnected, this, &ServerWindow::onClientDisconnected);
        connect(clientSocket, &QTcpSocket::readyRead, this, &ServerWindow::onReadyRead);

        m_log->append("New client connected from " + clientSocket->peerAddress().toString()
                      + ":" + QString::number(clientSocket->peerPort()));

        broadcastClientCount();
    }
}

void ServerWindow::onClientDisconnected()
{
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (clientSocket) {
        m_log->append("Client disconnected: " + clientSocket->peerAddress().toString());
        m_clients.removeAll(clientSocket);
        clientSocket->deleteLater();
    }
    broadcastClientCount();
}

void ServerWindow::onReadyRead()
{
    QTcpSocket* senderSocket = qobject_cast<QTcpSocket*>(sender());
    if (!senderSocket) return;

    QByteArray data = senderSocket->readAll();

    forwardMessageToOthers(senderSocket, data);

    m_log->append("Relayed data from client " + senderSocket->peerAddress().toString());
}

/*!
 * \brief ServerWindow::broadcastClientCount
 * Broadcasts the current number of connected clients to everyone.
 * We define a simple "message type" integer at the start of the packet:
 * 0 = text
 * 1 = file
 * 2 = clientCount
 */
void ServerWindow::broadcastClientCount()
{
    int count = m_clients.size();

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (qint32)2 << (qint32)count; // type=2, then the count

    for (QTcpSocket* client : m_clients) {
        client->write(block);
    }

    m_log->append(QString("Broadcasting client count: %1").arg(count));
}

void ServerWindow::forwardMessageToOthers(QTcpSocket* senderSocket, const QByteArray &data)
{
    for (QTcpSocket* client : m_clients) {
        if (client != senderSocket) {
            client->write(data);
        }
    }
}
