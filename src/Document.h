#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QtCore>
#include <QtConcurrent>
#include <QtMultimedia>
#include <string>
#include <functional>
#include "Bmson.h"
#include "DocumentDef.h"
#include "History.h"

class Document;
class DocumentInfo;
class SoundChannel;
struct SoundNote;
class SoundLoader;





class DocumentInfo : public QObject, public BmsonObject
{
	Q_OBJECT

private:
	Document *document;

	QString title;
	QString genre;
	QString artist;
	int judgeRank;
	double total;
	double initBpm;
	int level;

	static QSet<QString> SupportedKeys;

public:
	DocumentInfo(Document *document);
	~DocumentInfo();
	void Initialize(); // for initialization
	void LoadBmson(QJsonValue json); // for initialization

	QJsonValue SaveBmson();

	QString GetTitle() const{ return title; }
	QString GetGenre() const{ return genre; }
	QString GetArtist() const{ return artist; }
	int GetJudgeRank() const{ return judgeRank; }
	double GetTotal() const{ return total; }
	double GetInitBpm() const{ return initBpm; }
	int GetLevel() const{ return level; }

	void SetTitle(QString value){ title = value; emit TitleChanged(title); }
	void SetGenre(QString value){ genre = value; emit GenreChanged(genre); }
	void SetArtist(QString value){ artist = value; emit ArtistChanged(artist); }
	void SetJudgeRank(int value){ judgeRank = value; emit JudgeRankChanged(judgeRank); }
	void SetTotal(double value){ total = value; emit TotalChanged(total); }
	void SetInitBpm(double value);
	void SetLevel(int value){ level = value; emit LevelChanged(level); }

	QMap<QString, QJsonValue> GetExtraFields() const;
	void SetExtraFields(const QMap<QString, QJsonValue> &fields);

signals:
	void TitleChanged(QString value);
	void GenreChanged(QString value);
	void ArtistChanged(QString value);
	void JudgeRankChanged(int value);
	void TotalChanged(double value);
	void InitBpmChanged(double value);
	void LevelChanged(double value);
	void ExtraFieldsChanged();
};




class Document : public QObject, public BmsonObject
{
	Q_OBJECT
	friend class SoundLoader;

private:
	QDir directory;
	QString filePath;
	EditHistory *history;

	// data
	DocumentInfo info;
	int timeBase;
	QMap<int, BarLine> barLines;
	QMap<int, BpmEvent> bpmEvents;
	QList<SoundChannel*> soundChannels;
	QMap<SoundChannel*, int> soundChannelLength;

	// utility
	int actualLength;
	int totalLength;

private slots:
	void OnInitBpmChanged();

public:
	Document(QObject *parent=nullptr);
	~Document();
	void Initialize(); // for initialization
	void LoadFile(QString filePath); // for initialization

	EditHistory *GetHistory(){ return history; }

	QDir GetProjectDirectory(const QDir &def=QDir::root()) const { return directory.isRoot() ? def : directory; }
	QString GetFilePath() const { return filePath; }
	QString GetRelativePath(QString filePath);
	QString GetAbsolutePath(QString fileName) const;
	void Save();
	void SaveAs(const QString &filePath);

	int GetTimeBase() const{ return timeBase; }
	DocumentInfo *GetInfo(){ return &info; }
	const QMap<int, BarLine> &GetBarLines() const{ return barLines; }
	const QMap<int, BpmEvent> &GetBpmEvents() const{ return bpmEvents; }
	const QList<SoundChannel*> &GetSoundChannels() const{ return soundChannels; }
	int GetTotalLength() const;
	int GetTotalVisibleLength() const;
	QList<QPair<int, int>> FindConflictingNotes(SoundNote note) const; // returns [Channel,Location]
	void InsertNewSoundChannels(const QList<QString> &soundFilePaths, int index=-1);
	void DestroySoundChannel(int index);
	void MoveSoundChannel(int indexBefore, int indexAfter);

	//void GetBpmEventsInRange(int startTick, int span, double &initBpm, QVector<BpmEvent> &bpmEvents) const; // span=0 = untill end

	void ChannelLengthChanged(SoundChannel *channel, int length);
	void UpdateTotalLength();

	bool InsertBarLine(BarLine bar);
	bool RemoveBarLine(int location);

	bool InsertBpmEvent(BpmEvent event);
	bool RemoveBpmEvent(int location);

signals:
	void FilePathChanged();
	void SoundChannelInserted(int index, SoundChannel *channel);
	void SoundChannelRemoved(int index, SoundChannel *channel);
	void SoundChannelMoved(int indexBefore, int indexAfter);
	void AfterSoundChannelsChange();

	// emitted when BPM(initialBPM/BPM Notes location) or timeBase changed.
	void TimeMappingChanged();

	// emitted when length of song (in ticks) changed.
	void TotalLengthChanged(int length);

	void BarLinesChanged();
};


#endif // DOCUMENT_H