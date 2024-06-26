#include "BpmEditTool.h"
#include "MainWindow.h"
#include "util/SymbolIconManager.h"
#include "util/JsonExtension.h"

namespace BpmEditToolsSettings{
const char *SettingsGroup = "BpmEditView";
const char *SettingsShowExtraFields = "ShowExtraFields";
}
using namespace BpmEditToolsSettings;


BpmEditView::BpmEditView(MainWindow *mainWindow)
	: ScrollableForm(mainWindow)
	, mainWindow(mainWindow)
	, automated(false)
{
	auto *layout = new QFormLayout();
	formLayout = layout;
	auto *icon = new QLabel();
	icon->setPixmap(SymbolIconManager::GetIcon(SymbolIconManager::Icon::Event).pixmap(UIUtil::ToolBarIconSize, QIcon::Normal));
	{
		auto *captionLayout = new QHBoxLayout();
		captionLayout->setMargin(0);
		captionLayout->addWidget(icon);
		captionLayout->addWidget(message = new QLabel(), 1);
		layout->addRow(captionLayout);
	}
	layout->addRow(tr("BPM:"), edit = new QuasiModalEdit());
	layout->addRow(tr("Extra fields:"), buttonShowExtraFields = new CollapseButton(editExtraFields = new QuasiModalMultiLineEdit(), this));
	layout->addRow(editExtraFields);
	layout->addRow(dummy = new QWidget());
	dummy->setFixedSize(1, 1);

	layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
	layout->setSizeConstraint(QLayout::SetNoConstraint);
	editExtraFields->setAcceptRichText(false);
	editExtraFields->setAcceptDrops(false);
    editExtraFields->setTabStopDistance(24);
	editExtraFields->setLineWrapMode(QTextEdit::WidgetWidth);
	//editExtraFields->setMinimumHeight(24);
	//editExtraFields->setMaximumHeight(999999); // (チート)
	//editExtraFields->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	editExtraFields->SetSizeHint(QSize(200, 180)); // (普通)
	buttonShowExtraFields->setAutoRaise(true);
	buttonShowExtraFields->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	Initialize(layout);

	connect(buttonShowExtraFields, SIGNAL(Changed()), this, SLOT(UpdateFormGeom()));

	connect(edit, SIGNAL(editingFinished()), this, SLOT(Edited()));
	connect(edit, SIGNAL(EscPressed()), this, SLOT(EscPressed()));

	connect(editExtraFields, SIGNAL(EditingFinished()), this, SLOT(ExtraFieldsEdited()));
	connect(editExtraFields, SIGNAL(EscPressed()), this, SLOT(ExtraFieldsEscPressed()));

	auto *settings = mainWindow->GetSettings();
	settings->beginGroup(SettingsGroup);
	{
		bool showExtraFields = settings->value(SettingsShowExtraFields, false).toBool();
		if (showExtraFields){
			buttonShowExtraFields->Expand();
		}else{
			buttonShowExtraFields->Collapse();
		}
	}
	settings->endGroup();

	Update();
}

BpmEditView::~BpmEditView()
{
	auto *settings = mainWindow->GetSettings();
	settings->beginGroup(SettingsGroup);
	{
		settings->setValue(SettingsShowExtraFields, editExtraFields->isVisibleTo(this));
	}
	settings->endGroup();
}

void BpmEditView::UnsetBpmEvents()
{
	bpmEvents.clear();
	Update();
}

void BpmEditView::SetBpmEvent(BpmEvent event)
{
	bpmEvents.clear();
	bpmEvents.append(event);
	Update();
}

void BpmEditView::SetBpmEvents(QList<BpmEvent> events)
{
	bpmEvents = events;
	Update();
}

void BpmEditView::Update()
{
	automated = true;
	if (bpmEvents.isEmpty()){
		message->setText(QString());
		edit->SetTextAutomated(QString());
		edit->setEnabled(false);
		edit->setPlaceholderText(QString());
		mainWindow->UnsetSelectedObjectsView(this);
	}else{
		if (bpmEvents.count() > 1){
			message->setText(tr("%1 selected BPM events").arg(bpmEvents.count()));
		}else{
			message->setText(tr("1 selected BPM event"));
		}
		edit->setEnabled(true);
		double bpm = bpmEvents.first().value;
		bool bpmUniform = true;
		QMap<QString, QJsonValue> extraFields = bpmEvents.first().GetExtraFields();
		bool extraFieldsUniform = true;
        for (const auto &event : std::as_const(bpmEvents)) {
            if (event.value != bpm){
				bpmUniform = false;
				break;
			}
        }
        for (const auto &event : std::as_const(bpmEvents)){
			if (event.GetExtraFields() != extraFields){
				extraFieldsUniform = false;
				break;
			}
		}
		SetBpm(bpm, bpmUniform);
		SetExtraFields(extraFields, extraFieldsUniform);
		mainWindow->SetSelectedObjectsView(this);
	}
	automated = false;
}

void BpmEditView::SetBpm(float bpm, bool uniform)
{
	if (uniform){
		edit->SetTextAutomated(QString::number(bpm));
		edit->setPlaceholderText(QString());
	}else{
		edit->SetTextAutomated(QString());
		edit->setPlaceholderText(tr("multiple values"));
	}
}

void BpmEditView::SetExtraFields(const QMap<QString, QJsonValue> &fields, bool uniform)
{
	QString s;
	if (uniform){
		for (QMap<QString, QJsonValue>::const_iterator i=fields.begin(); i!=fields.end(); ){
			s += "\"" + i.key() + "\": " + JsonExtension::RenderJsonValue(i.value(), QJsonDocument::Indented);
			i++;
			if (i==fields.end())
				break;
			s += ",\n";
		}
	}else{
		// フォーカスしただけでうっかり削除してしまわないように Placeholder を使わない（Textを設定しておけばエラーになってくれる）
		s = tr("(multiple values)");
	}
	editExtraFields->SetTextAutomated(s);
	buttonShowExtraFields->SetText(s);
}

void BpmEditView::Edited()
{
	if (automated || bpmEvents.empty()){
		return;
	}
	bool isOk;
	double bpm = edit->text().toDouble(&isOk);
	if (!isOk || !BmsConsts::IsBpmValid(bpm)){
		qApp->beep();
		Update();
		return;
	}
	for (auto i=bpmEvents.begin(); i!=bpmEvents.end(); i++){
		i->value = bpm;
	}
	emit Updated();
}

void BpmEditView::EscPressed()
{
	// revert
	Update();
}

void BpmEditView::ExtraFieldsEdited()
{
	if (automated || bpmEvents.empty()){
		return;
	}
	QString text = editExtraFields->toPlainText().trimmed();
	if (text.endsWith(',')){
		text.chop(1);
	}
	text.prepend('{').append('}');
	QJsonParseError err;
	QJsonObject json = QJsonDocument::fromJson(text.toUtf8(), &err).object();
	if (err.error != QJsonParseError::NoError){
		qApp->beep();
		ExtraFieldsEscPressed();
		return;
	}
	QMap<QString, QJsonValue> fields;
	for (QJsonObject::iterator i=json.begin(); i!=json.end(); i++){
		fields.insert(i.key(), i.value());
	}
	for (auto i=bpmEvents.begin(); i!=bpmEvents.end(); i++){
		i->SetExtraFields(fields);
	}
	emit Updated();
}

void BpmEditView::ExtraFieldsEscPressed()
{
	// revert
	Update();
}

void BpmEditView::UpdateFormGeom()
{
	Form()->setGeometry(0, 0, Form()->width(), 33333);
	Form()->setGeometry(0, 0, Form()->width(), dummy->y()+formLayout->spacing()+formLayout->margin());
}

