#include "mesa-ptz-dock.hpp"

#include <QPainter>
#include <QTimer>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QUrl>

static const char *URL_ESTADO = "http://127.0.0.1:8089/estado";

MesaPtzDock::MesaPtzDock(QWidget *parent) : QWidget(parent)
{
	setMinimumSize(240, 470);
	connect(&nam, &QNetworkAccessManager::finished, this,
		&MesaPtzDock::onReply);
	timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &MesaPtzDock::poll);
	timer->start(100);
}

void MesaPtzDock::poll()
{
	if (pending)
		return;
	pending = true;
	QNetworkRequest req{QUrl(QString::fromLatin1(URL_ESTADO))};
	req.setTransferTimeout(500);
	nam.get(req);
}

void MesaPtzDock::onReply(QNetworkReply *reply)
{
	pending = false;
	if (reply->error() == QNetworkReply::NoError) {
		const QJsonDocument doc =
			QJsonDocument::fromJson(reply->readAll());
		if (doc.isObject()) {
			e = doc.object();
			lastOkMs = QDateTime::currentMSecsSinceEpoch();
		}
	}
	reply->deleteLater();
	update();
}

bool MesaPtzDock::online() const
{
	return lastOkMs > 0 &&
	       QDateTime::currentMSecsSinceEpoch() - lastOkMs < 3000;
}

void MesaPtzDock::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);

	const QColor bg(20, 22, 26), panel(27, 30, 36), line(42, 46, 55),
		dark(16, 18, 22), txt(232, 234, 237), dim(139, 145, 157),
		ok(61, 220, 132), err(255, 82, 82), warn(255, 176, 32),
		zoomC(77, 208, 225), dz(58, 63, 75);

	p.fillRect(rect(), bg);

	const int m = 10;
	const int w = width();
	int y = m;

	const bool on = online();
	const bool ard = on && e.value("arduino").toBool();
	const bool cam = on && e.value("camera").toBool();
	const bool sim = on && e.value("simulacao").toBool();
	const bool joyOk = e.value("joystick_ok").toBool(true);
	const bool gravacao = e.value("gravacao").toBool();
	const bool movendo = e.value("movendo").toBool();
	const QString zoom = e.value("zoom").toString();

	QFont fChip = font();
	fChip.setPixelSize(11);
	fChip.setBold(true);
	QFont fMono(QStringLiteral("Consolas"));
	fMono.setPixelSize(13);
	QFont fLabel = font();
	fLabel.setPixelSize(11);

	/* ---------- chips de conexao ---------- */
	p.setFont(fChip);
	int cx = m;
	struct Chip {
		const char *t;
		bool lit;
		QColor cor;
		bool mostrar;
	};
	const Chip chips[3] = {{"ARDUINO", ard, ok, true},
			       {"CAMERA", cam || sim, ok, true},
			       {"SIM", sim, warn, sim}};
	for (const Chip &c : chips) {
		if (!c.mostrar)
			continue;
		const int cw =
			p.fontMetrics().horizontalAdvance(
				QString::fromLatin1(c.t)) + 32;
		const QRectF r(cx, y, cw, 22);
		p.setPen(QPen(c.lit ? c.cor : line, 1));
		p.setBrush(panel);
		p.drawRoundedRect(r, 11, 11);
		p.setPen(Qt::NoPen);
		p.setBrush(c.lit ? c.cor : err);
		p.drawEllipse(QPointF(cx + 12, y + 11), 3.5, 3.5);
		p.setPen(txt);
		p.drawText(r.adjusted(20, 0, -4, 0), Qt::AlignVCenter,
			   QString::fromLatin1(c.t));
		cx += cw + 6;
	}
	y += 32;

	/* ---------- scope do joystick ---------- */
	const int side = qMin(w - 2 * m, 170);
	const QRectF sc(m, y, side, side);
	p.setPen(QPen(line, 1));
	p.setBrush(dark);
	p.drawRoundedRect(sc, 8, 8);
	p.drawLine(QPointF(sc.center().x(), sc.top() + 2),
		   QPointF(sc.center().x(), sc.bottom() - 2));
	p.drawLine(QPointF(sc.left() + 2, sc.center().y()),
		   QPointF(sc.right() - 2, sc.center().y()));

	const QJsonArray zona = e.value("zona").toArray();
	if (zona.size() == 4) {
		const double zx1 = zona[0].toDouble() / 1023.0;
		const double zx2 = zona[1].toDouble() / 1023.0;
		const double zy1 = zona[2].toDouble() / 1023.0;
		const double zy2 = zona[3].toDouble() / 1023.0;
		p.setPen(QPen(dz, 1, Qt::DashLine));
		p.setBrush(Qt::NoBrush);
		p.drawRoundedRect(QRectF(sc.left() + zx1 * side,
					 sc.top() + zy1 * side,
					 (zx2 - zx1) * side,
					 (zy2 - zy1) * side),
				  4, 4);
	}

	const double px = e.value("x").toDouble(512) / 1023.0;
	const double py = e.value("y").toDouble(512) / 1023.0;
	QColor dotc = ok;
	if (!joyOk)
		dotc = err;
	else if (e.value("sw").toInt() == 1)
		dotc = zoomC;
	p.setPen(Qt::NoPen);
	p.setBrush(dotc);
	p.drawEllipse(QPointF(sc.left() + px * side, sc.top() + py * side),
		      6, 6);
	y += side + 8;

	/* ---------- leituras ---------- */
	p.setFont(fMono);
	p.setPen(txt);
	const QString l1 = QStringLiteral("X %1   Y %2")
				   .arg(e.value("x").toInt(512), 4)
				   .arg(e.value("y").toInt(512), 4);
	const QString l2 = QStringLiteral("PAN %1   TILT %2")
				   .arg(e.value("pan_vel").toInt(), 2)
				   .arg(e.value("tilt_vel").toInt(), 2);
	p.drawText(QRectF(m, y, w - 2 * m, 16), Qt::AlignVCenter, l1);
	y += 17;
	p.drawText(QRectF(m, y, w - 2 * m, 16), Qt::AlignVCenter, l2);
	y += 24;

	/* ---------- badge de status ---------- */
	QString badge = QStringLiteral("PARADO");
	QColor bcor = dim;
	if (!joyOk) {
		badge = QStringLiteral("JOYSTICK BLOQUEADO");
		bcor = err;
	} else if (gravacao) {
		badge = QStringLiteral("MODO GRAVACAO: 1-9");
		bcor = warn;
	} else if (!zoom.isEmpty()) {
		badge = QStringLiteral("ZOOM %1 v%2")
				.arg(zoom == QStringLiteral("in")
					     ? QStringLiteral("IN")
					     : QStringLiteral("OUT"))
				.arg(e.value("zoom_vel").toInt());
		bcor = zoomC;
	} else if (movendo) {
		badge = QStringLiteral("MOVENDO");
		bcor = ok;
	}
	p.setFont(fChip);
	const int bw = p.fontMetrics().horizontalAdvance(badge) + 20;
	const QRectF br(m, y, bw, 22);
	p.setPen(QPen(bcor, 1));
	p.setBrush(panel);
	p.drawRoundedRect(br, 5, 5);
	p.setPen(bcor);
	p.drawText(br, Qt::AlignCenter, badge);
	y += 32;

	/* ---------- presets 1-9 ---------- */
	const int ativo = e.value("preset_ativo").toInt();
	QList<int> gravados;
	for (const auto v : e.value("presets_gravados").toArray())
		gravados.append(v.toInt());

	const int gap = 6;
	const int cw3 = (w - 2 * m - 2 * gap) / 3;
	const int ch = 34;
	p.setFont(fMono);
	for (int n = 1; n <= 9; n++) {
		const int col = (n - 1) % 3;
		const int row = (n - 1) / 3;
		const QRectF cel(m + col * (cw3 + gap), y + row * (ch + gap),
				 cw3, ch);
		const bool isAtivo = (ativo == n);
		p.setPen(QPen(isAtivo ? ok : line, 1));
		p.setBrush(dark);
		p.drawRoundedRect(cel, 6, 6);
		p.setPen(isAtivo ? ok
				 : (gravados.contains(n) ? txt : dim));
		p.drawText(cel, Qt::AlignCenter, QString::number(n));
		if (gravados.contains(n)) {
			p.setPen(Qt::NoPen);
			p.setBrush(warn);
			p.drawEllipse(
				QPointF(cel.right() - 8, cel.top() + 8),
				2.5, 2.5);
		}
	}
	y += 3 * ch + 2 * gap + 12;

	/* ---------- ultimo evento ---------- */
	p.setFont(fLabel);
	p.setPen(dim);
	p.drawText(QRectF(m, y, w - 2 * m, 14), Qt::AlignVCenter,
		   QStringLiteral("ULTIMO EVENTO"));
	y += 16;
	p.setFont(fMono);
	p.setPen(txt);
	const QString ev = p.fontMetrics().elidedText(
		e.value("evento").toString(QStringLiteral("...")),
		Qt::ElideRight, w - 2 * m);
	p.drawText(QRectF(m, y, w - 2 * m, 18), Qt::AlignVCenter, ev);

	/* ---------- overlay: script offline ---------- */
	if (!on) {
		p.setPen(Qt::NoPen);
		p.setBrush(QColor(10, 11, 14, 225));
		p.drawRect(rect());
		QFont fBig = font();
		fBig.setPixelSize(14);
		fBig.setBold(true);
		p.setFont(fBig);
		p.setPen(err);
		p.drawText(rect().adjusted(0, -14, 0, -14),
			   Qt::AlignCenter,
			   QStringLiteral("SCRIPT PYTHON OFFLINE"));
		p.setFont(fLabel);
		p.setPen(dim);
		p.drawText(rect().adjusted(0, 14, 0, 14), Qt::AlignCenter,
			   QStringLiteral("Inicie controladora_ptz_obs.py"));
	}
}
