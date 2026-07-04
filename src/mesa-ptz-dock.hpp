#pragma once
/*
 * Mesa PTZ Monitor — dock nativo para OBS Studio
 * - Mostra o estado da mesa (joystick, presets, zoom, conexoes)
 * - Gerencia o script Python (inicia junto com o OBS, encerra com
 *   seguranca ao fechar) e permite escolher as portas COM na interface
 */

#include <QWidget>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QProcess>

class QTimer;
class QNetworkReply;
class QLabel;
class QPushButton;

/* Area pintada com o estado da mesa */
class PainelMesa : public QWidget {
	Q_OBJECT
public:
	explicit PainelMesa(QWidget *parent = nullptr);
	void setEstado(const QJsonObject &estado, bool online);

protected:
	void paintEvent(QPaintEvent *event) override;

private:
	QJsonObject e;
	bool on = false;
};

/* Dock completo: cabecalho de controle + painel */
class MesaPtzDock : public QWidget {
	Q_OBJECT
public:
	explicit MesaPtzDock(QWidget *parent = nullptr);
	~MesaPtzDock() override;

private slots:
	void poll();
	void onReply(QNetworkReply *reply);
	void abrirConfig();
	void iniciarOuParar();
	void procTerminou(int codigo, QProcess::ExitStatus status);

private:
	bool online() const;
	bool procRodando() const;
	void carregarConfig();
	void salvarConfig();
	void iniciarScript();
	void pararScript(bool bloqueante);
	void atualizarCabecalho();

	/* configuracao persistida */
	QString cmdPython;
	QString scriptPath;
	QString portaArduino;
	QString portaCamera;
	QString baudCamera;
	bool autoStart = false;

	/* execucao */
	QNetworkAccessManager nam;
	QTimer *timer = nullptr;
	QJsonObject e;
	qint64 lastOkMs = 0;
	bool pending = false;
	QProcess *proc = nullptr;

	/* interface */
	PainelMesa *painel = nullptr;
	QLabel *lblStatus = nullptr;
	QPushButton *btnRun = nullptr;
};
