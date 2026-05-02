//	Altirra - Qt port
//	Debugger Watch pane implementation.
//
//	A two-column QTableWidget (Expression / Value) plus an Add button
//	that prompts for a new expression, and a Remove button that drops
//	the selected row. Each pause re-evaluates every expression via
//	IATDebugger::EvaluateThrow; failures show "?" in the value column.

#include "debuggerwatchpane.h"

#include <QDockWidget>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <cstdio>

#undef signals
#undef slots

#include <vd2/system/VDString.h>
#include <vd2/system/error.h>
#include <debugger.h>
#include <irqcontroller.h>
#include <simeventmanager.h>
#include <simulator.h>

namespace {

class ATWatchWidget : public QWidget, public IATSimulatorCallback {
public:
	ATWatchWidget(QWidget *parent, ATSimulator *sim)
		: QWidget(parent), mpSim(sim)
	{
		auto *layout = new QVBoxLayout(this);
		layout->setContentsMargins(2, 2, 2, 2);
		layout->setSpacing(2);

		mpTable = new QTableWidget(0, 2, this);
		mpTable->setHorizontalHeaderLabels({tr("Expression"), tr("Value")});
		mpTable->horizontalHeader()->setStretchLastSection(true);
		mpTable->verticalHeader()->setVisible(false);
		mpTable->setSelectionBehavior(QAbstractItemView::SelectRows);
		mpTable->setSelectionMode(QAbstractItemView::SingleSelection);
		mpTable->setEditTriggers(QAbstractItemView::DoubleClicked
		                       | QAbstractItemView::EditKeyPressed);
		mpTable->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
		layout->addWidget(mpTable, 1);

		auto *buttons = new QHBoxLayout;
		mpAdd = new QPushButton(tr("Add..."), this);
		mpRemove = new QPushButton(tr("Remove"), this);
		buttons->addWidget(mpAdd);
		buttons->addWidget(mpRemove);
		buttons->addStretch(1);
		layout->addLayout(buttons);

		connect(mpAdd, &QPushButton::clicked, this, [this]{
			bool ok = false;
			const QString text = QInputDialog::getText(this,
				tr("Add watch"), tr("Expression:"), QLineEdit::Normal, {}, &ok);
			if (!ok || text.isEmpty()) return;
			addRow(text);
			refresh();
		});
		connect(mpRemove, &QPushButton::clicked, this, [this]{
			const int row = mpTable->currentRow();
			if (row >= 0) mpTable->removeRow(row);
		});
		connect(mpTable, &QTableWidget::itemChanged, this, [this](QTableWidgetItem *item){
			if (!item || item->column() != 0) return;
			refresh();
		});

		mpSim->GetEventManager()->AddCallback(this);
		refresh();
	}

	~ATWatchWidget() override {
		if (mpSim)
			mpSim->GetEventManager()->RemoveCallback(this);
	}

	void OnSimulatorEvent(ATSimulatorEvent ev) override {
		switch (ev) {
			case kATSimEvent_CPUSingleStep:
			case kATSimEvent_CPUStackBreakpoint:
			case kATSimEvent_CPUPCBreakpoint:
			case kATSimEvent_ReadBreakpoint:
			case kATSimEvent_WriteBreakpoint:
			case kATSimEvent_AnonymousPause:
			case kATSimEvent_AnonymousInterrupt:
			case kATSimEvent_ColdReset:
			case kATSimEvent_WarmReset:
			case kATSimEvent_StateLoaded:
			case kATSimEvent_SimPause:
				refresh();
				break;
			default:
				break;
		}
	}

private:
	void addRow(const QString& expr) {
		const int row = mpTable->rowCount();
		mpTable->insertRow(row);
		auto *exprItem = new QTableWidgetItem(expr);
		auto *valItem = new QTableWidgetItem(QStringLiteral("?"));
		valItem->setFlags(valItem->flags() & ~Qt::ItemIsEditable);
		mpTable->setItem(row, 0, exprItem);
		mpTable->setItem(row, 1, valItem);
	}

	void refresh() {
		IATDebugger *dbg = ATGetDebugger();
		if (!dbg) return;

		const int rows = mpTable->rowCount();
		for (int r = 0; r < rows; ++r) {
			QTableWidgetItem *expr = mpTable->item(r, 0);
			QTableWidgetItem *val  = mpTable->item(r, 1);
			if (!expr || !val) continue;

			const QByteArray utf8 = expr->text().toUtf8();
			QString rendered = QStringLiteral("?");
			try {
				const sint32 v = dbg->EvaluateThrow(utf8.constData());
				char buf[64];
				std::snprintf(buf, sizeof buf, "%d ($%X)", (int)v, (unsigned)(uint32)v);
				rendered = QString::fromUtf8(buf);
			} catch (const MyError& e) {
				rendered = QStringLiteral("err: %1").arg(QString::fromUtf8(e.c_str()));
			} catch (...) {
				rendered = QStringLiteral("err");
			}
			val->setText(rendered);
		}
	}

	ATSimulator *mpSim = nullptr;
	QTableWidget *mpTable = nullptr;
	QPushButton *mpAdd = nullptr;
	QPushButton *mpRemove = nullptr;
};

QDockWidget *g_dock = nullptr;

}  // namespace

void ATShowDebuggerWatchPane(QMainWindow *parent, ATSimulator *sim) {
	if (!g_dock) {
		g_dock = new QDockWidget(QObject::tr("Watches"), parent);
		g_dock->setObjectName(QStringLiteral("DebuggerWatchDock"));
		g_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
		auto *w = new ATWatchWidget(g_dock, sim);
		g_dock->setWidget(w);
		parent->addDockWidget(Qt::RightDockWidgetArea, g_dock);
	}
	g_dock->show();
	g_dock->raise();
}
