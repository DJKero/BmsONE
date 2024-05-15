#ifndef SOUNDCHANNELINTERNAL
#define SOUNDCHANNELINTERNAL

#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QFuture>
#include "DocumentDef.h"
#include "SoundChannelDef.h"
#include "../audio/Wave.h"


class SoundChannelResourceManager : public QObject
{
	Q_OBJECT

private:
	static const quint64 BufferFrames = 1024;

public:
	static const int RmsCacheBlockSize = 64;
	static const int RmsCachePacketSize = 256;
	static const int RmsCachePacketSampleCount = RmsCacheBlockSize * RmsCachePacketSize;

private:
	QFuture<void> currentTask;
	QFileInfo file;
	AudioStreamSource *wave;

	WaveSummary summary;
	QImage overallWaveform;

	mutable QMutex rmsCacheMutex;
	QMap<quint64, RmsCachePacket> rmsCachePackets;

	static const quint64 auxBufferSize = 4096;
	char *auxBuffer;

private:
	// tasks
	void RunTaskWaveData();
	void RunTaskRmsCachePacket(int position);

	//bool TaskLoadWaveSummary();
	void TaskDrawOverallWaveformAndRmsCache();

	quint64 ReadAsS16S(QAudioBuffer::S16S *buffer, quint64 frames);
	void ConvertAuxBufferToS16S(QAudioBuffer::S16S *buffer, quint64 frames);

public:
	SoundChannelResourceManager(QObject *parent=nullptr);
	virtual ~SoundChannelResourceManager();

	// task request
	void UpdateWaveData(const QString &srcPath);
	void RequireRmsCachePacket(int position);

	// get data
	WaveSummary GetWaveSummary() const{ return summary; }
	const QImage &GetOverallWaveform() const{ return overallWaveform; }

signals:
	//void WaveSummaryReady(const WaveSummary *summary);
	void OverallWaveformReady();
	void RmsCacheUpdated();
	void RmsCachePacketReady(int position, QList<RmsCacheEntry> packet);

};


class SoundChannelUtil
{
public:
	static AudioStreamSource *OpenSourceFile(const QString &srcPath, QObject *parent=nullptr);
	static QMap<int, int> MergeRegions(const QMultiMap<int, int> &regs);
	static void RegionsDiff(const QMap<int, int> &regsOld, const QMap<int, int> &regsNew, QMap<int, int> &regsAdded, QMap<int, int> &regsRemoved);
};


#endif // SOUNDCHANNELINTERNAL
