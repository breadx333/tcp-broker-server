#ifndef SERVERWINDOW_H
#define SERVERWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>

class ServerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ServerWindow(QWidget *parent = nullptr);
    ~ServerWindow();

private slots:
    void startServer();
    void onNewConnection();
    void onClientDisconnected();
    void onReadyRead();

private:
    void broadcastClientCount();
    void forwardMessageToOthers(QTcpSocket* senderSocket, const QByteArray &data);

private:
    QTcpServer* m_tcpServer;
    QList<QTcpSocket*> m_clients;

    // GUI elements
    QLineEdit* m_portEdit;
    QPushButton* m_startButton;
    QLabel* m_statusLabel;
    QTextEdit* m_log;
};

#endif // SERVERWINDOW_H
