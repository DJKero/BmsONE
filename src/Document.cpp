#include "Document.h"
#include <QFile>


const double BmsConsts::MaxBpm = 1.e+6;
const double BmsConsts::MinBpm = 1.e-6;

bool BmsConsts::IsBpmValid(double value)
{
	return value >= MinBpm && value <= MaxBpm;
}

double BmsConsts::ClampBpm(double value)
{
	return std::max(BmsConsts::MinBpm, std::min(BmsConsts::MaxBpm, value));
}



Document::Document(QObject *parent)
	: QObject(parent)
	, history(new EditHistory(this))
	, info(this)
{
	connect(&info, SIGNAL(InitBpmChanged(double)), this, SLOT(OnInitBpmChanged()));
}

Document::~Document()
{
}

void Document::Initialize()
{
	directory = QDir::root();

	timeBase = 240;
	totalLength = 0;
	info.Initialize();
	for (int i=0; i<8; i++){
		int t = i * timeBase * 4;
		barLines.insert(t, BarLine(t, 0));
	}
}

void Document::LoadFile(QString filePath)
	throw(Bmson::BmsonIoException)
{
	directory = QFileInfo(filePath).absoluteDir();

	Bmson::Bms bms;
	Bmson::BmsonIo::LoadFile(bms, filePath);
	timeBase = 240;
	totalLength = 0;
	info.LoadBmson(bms.info);
	barLines.insert(0, BarLine(0, 0)); // make sure BarLine at 0, even if bms.barLines is empty.
	for (Bmson::BarLine bar : bms.barLines){
		barLines.insert(bar.location, BarLine(bar.location, bar.kind));
	}
	for (Bmson::EventNote event : bms.bpmNotes){
		bpmEvents.insert(event.location, BpmEvent(event.location, event.value));
	}
	for (size_t i=0; i<bms.soundChannels.size(); i++){
		auto *channel = new SoundChannel(this);
		channel->LoadBmson(bms.soundChannels[i]);
		soundChannelLength.insert(channel, channel->GetLength());
		soundChannels.push_back(channel);
	}
	for (int length : soundChannelLength){
		if (length > totalLength)
			totalLength = length;
	}
	this->filePath = filePath;
	emit FilePathChanged();
}

QString Document::GetRelativePath(QString filePath)
{
	QFileInfo fi(filePath);
	if (directory.isRoot()){
		directory = fi.absoluteDir();
		return fi.fileName();
	}else{
		return directory.relativeFilePath(filePath);
	}
}

QString Document::GetAbsolutePath(QString fileName) const
{
	if (directory.isRoot()){
		return QString();
	}
	return directory.absoluteFilePath(fileName);
}


void Document::Save()
	throw(Bmson::BmsonIoException)
{
	//Bmson::BmsonIo::SaveFile(bms, fileName);
	history->MarkClean();
}

void Document::SaveAs(const QString &filePath)
{
	this->filePath = filePath;
	Save();
	emit FilePathChanged();
}

int Document::GetTotalLength() const
{
	return totalLength;
}

QList<QPair<int, int> > Document::FindConflictingNotes(SoundNote note) const
{
	QList<QPair<int, int>> noteRefs;
	for (int i=0; i<soundChannels.size(); i++){
		const QMap<int, SoundNote> &notes = soundChannels[i]->GetNotes();
		QMap<int, SoundNote>::const_iterator inote = notes.lowerBound(note.location);
		while (inote != notes.end() && inote->location <= note.location + note.length){
			if (inote->lane > 0 && inote->lane == note.lane){
				noteRefs.append(QPair<int,int>(i, inote->location));
			}
			inote++;
		}
	}
	return noteRefs;
}

void Document::ChannelLengthChanged(SoundChannel *channel, int length)
{
	soundChannelLength.insert(channel, length);
	int oldValue = totalLength;
	totalLength = 0;
	for (int length : soundChannelLength){
		if (length > totalLength)
			totalLength = length;
	}
	if (oldValue != totalLength){
		emit TotalLengthChanged(totalLength);
	}
}

void Document::OnInitBpmChanged()
{
	emit TimeMappingChanged();
}





DocumentInfo::DocumentInfo(Document *document)
	: QObject(document)
	, document(document)
{
}

DocumentInfo::~DocumentInfo()
{
}

void DocumentInfo::Initialize()
{
	title = QString();
	genre = QString();
	artist = QString();
	judgeRank = 3;
	total = 400.;
	initBpm = 120.;
	level = 1;
}

void DocumentInfo::LoadBmson(Bmson::BmsInfo &info)
{
	title = info.title;
	genre = info.genre;
	artist = info.artist;
	judgeRank = info.judgeRank;
	total = info.total;
	initBpm = info.initBpm;
	level = info.level;
}

void DocumentInfo::SetInitBpm(double value)
{
	// caller of this function should check value is valid.
	// default behavior is to retain old value if new value is invalid.

	if (BmsConsts::IsBpmValid(value)){

		// TODO: undo

		initBpm = value;
	}
	// initBpm = std::max(BmsConsts::MinBpm, std::min(BmsConsts::MaxBpm, value));
	emit InitBpmChanged(initBpm);
}











