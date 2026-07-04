#pragma once
/*
 * Mesa PTZ Monitor — dock nativo para OBS Studio
 * Le o estado da mesa em http://127.0.0.1:8089/estado
 * (servidor embutido no script controladora_ptz_obs.py)
 */

#include <QWidget>
#include <QNetworkAccessManager>
#include <QJsonObject>

class QTimer;
class QNetworkReply;

class MesaPtzDock : public QWidget {
	Q_OBJECT
public:
	explicit MesaPtzDock(QWidget *parent = nullptr);

protected:
	void paintEvent(QPaintEvent *event) override;

private slots:
	void poll();
	void onReply(QNetworkReply *reply);

private:
	bool online() const;

	QNetworkAccessManager nam;
	QTimer *timer = nullptr;
	QJsonObject e;        // ultimo estado JSON recebido
	qint64 lastOkMs = 0;  // quando recebemos dados validos
	bool pending = false; // evita empilhar requisicoes
};
