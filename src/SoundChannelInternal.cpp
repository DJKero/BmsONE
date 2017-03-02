#include "Document.h"
#include "SoundChannel.h"
#include "SoundChannelInternal.h"
#include <cstdlib>
#include <cmath>


AudioStreamSource *SoundChannelUtil::OpenSourceFile(const QString &srcPath, QObject *parent)
{
	AudioStreamSource *wave = nullptr;
	QFileInfo file(srcPath);
	QString ext = file.suffix().toLower();
	if (ext == "wav"){
		if (file.exists()){
			wave = new WaveStreamSource(srcPath, parent);
		}else{
			QString srcPath2 = file.dir().absoluteFilePath(file.completeBaseName().append(".ogg"));
			QFileInfo file2(srcPath2);
			if (file2.exists()){
				file = file2;
				wave = new OggStreamSource(srcPath2, parent);
			}else{
				//
			}
		}
	}else if (ext == "ogg"){
		if (file.exists()){
			wave = new OggStreamSource(srcPath, parent);
		}else{
			QString srcPath2 = file.dir().absoluteFilePath(file.completeBaseName().append(".wav"));
			QFileInfo file2(srcPath2);
			if (file2.exists()){
				file = file2;
				wave = new WaveStreamSource(srcPath2, parent);
			}else{
				//
			}
		}
	}else{
		//
	}
	return wave;
}



QMap<int, int> SoundChannelUtil::MergeRegions(const QMultiMap<int, int> &regs)
{
	if (regs.empty()){
		return QMap<int, int>();
	}
	QMap<int, int> merged;
	QPair<int, int> r(regs.begin().key(), regs.begin().value());
	for (QMap<int, int>::const_iterator i=regs.begin()+1; i!=regs.end(); i++){
		if (i.key() <= r.second){
			r.second = std::max(r.second, i.value());
			continue;
		}
		merged.insert(r.first, r.second);
		r.first = i.key();
		r.second = i.value();
	}
	merged.insert(r.first, r.second);
	return merged;
}


// regions in regsOld must be disjoint & regions in regNew must be disjoint
void SoundChannelUtil::RegionsDiff(const QMap<int, int> &regsOld, const QMap<int, int> &regsNew, QMap<int, int> &regsAdded, QMap<int, int> &regsRemoved)
{
	regsAdded.clear();
	regsRemoved.clear();
	QMap<int, bool> marks;
	for (QMap<int, int>::const_iterator i=regsOld.begin(); i!=regsOld.end(); i++){
		marks.insert(i.key(), false);
		marks.insert(i.value(), true);
	}
	for (QMap<int, int>::const_iterator i=regsNew.begin(); i!=regsNew.end(); i++){
		marks.insert(i.key(), true);
		marks.insert(i.value(), false);
	}
	int c = 0;
	int from;
	for (QMap<int, bool>::const_iterator i=marks.begin(); i!=marks.end(); i++){
		switch (c){
		case 0:
			from = i.key();
			i.value() ? c++ : c--;
			break;
		case 1:
			// i.value() == false
			regsAdded.insert(from, i.key());
			c = 0;
			break;
		case -1:
			// i.value() == true
			regsRemoved.insert(from, i.key());
			c = 0;
			break;
		}
	}
}


// all entries must be valid
RmsCachePacket::RmsCachePacket(const QList<RmsCacheEntry> &entries, int count)
	: blockCount(count)
{
	//qDebug() << "RmsCachePacket create" << count;
	// algorithm: silence compression & 8bit->4bit(when all samples in 4bit)
	qint8 peak = 0;
	QMap<int, int> silent;
	int silentCur = -1;
	for (int i=0; i<count; i++){
		if (entries[i].L > peak)
			peak = entries[i].R;
		if (entries[i].R > peak)
			peak = entries[i].R;
		if (entries[i].L == 0 && entries[i].R == 0){
			if (silentCur < 0){
				silentCur = i;
			}
		}else if (silentCur >= 0){
			silent.insert(silentCur, i - silentCur);
			silentCur = -1;
		}
	}
	if (silentCur >= 0){
		silent.insert(silentCur, count - silentCur);
	}
	if (peak == 0){
		// perfect silence!
	}else if (peak < 0x08){
		compressed.append(0x01);
		QMap<int, int>::const_iterator isilent = silent.begin();
		for (int i=0; i<count; i++){
			if (isilent != silent.end() && isilent.key() == i){
				if (isilent.value() >= 128){
					compressed.append(char(uchar(0x80)));
					compressed.append(char(uchar(isilent.value() & 0xff)));
					compressed.append(char(uchar((isilent.value() & 0xff00) >> 8)));
				}else{
					compressed.append(char(uchar(0x80 + isilent.value())));
				}
				i += isilent.value() - 1;
				isilent++;
			}else{
				compressed.append(uchar(entries[i].L & 0x07) | uchar((entries[i].R & 0x07) << 4));
			}
		}
	}else{
		compressed.append(0x02);
		QMap<int, int>::const_iterator isilent = silent.begin();
		for (int i=0; i<count; i++){
			if (isilent != silent.end() && isilent.key() == i){
				if (isilent.value() >= 128){
					compressed.append(char(uchar(0x80)));
					compressed.append(char(uchar(isilent.value() & 0xff)));
					compressed.append(char(uchar((isilent.value() & 0xff00) >> 8)));
				}else{
					compressed.append(char(uchar(0x80 + isilent.value())));
				}
				i += isilent.value() - 1;
				isilent++;
			}else{
				compressed.append(char(uchar(entries[i].L & 0x7f)));
				compressed.append(char(uchar(entries[i].R & 0x7f)));
			}
		}
	}
	//qDebug() << QString("compressed. %1 -> %2 (%3%)").arg(count*2).arg(compressed.size()).arg(50*compressed.size()/count);
}

QList<RmsCacheEntry> RmsCachePacket::Uncompress() const
{
	QList<RmsCacheEntry> entries;
	entries.reserve(blockCount);
	if (compressed.size() == 0){
		for (int i=0; i<blockCount; i++){
			entries.append(RmsCacheEntry(0, 0));
		}
		return entries;
	}
	int c=0;
	switch (compressed[c++]){
	case 1:
		while (c < compressed.size()){
			int v = (uchar)compressed[c++];
			if (v & 0x80){
				int size = v - 0x80;
				if (size > 0){
					for (int i=0; i<size; i++){
						entries.append(RmsCacheEntry(0, 0));
					}
				}else{
					if (c > compressed.size() - 2){
						qDebug() << "malformed1";
						break;
					}
					int sl = (uchar)compressed[c++];
					int sh = (uchar)compressed[c++];
					int sz = sl | (sh << 8);
					for (int i=0; i<sz; i++){
						entries.append(RmsCacheEntry(0, 0));
					}
				}
			}else{
				entries.append(RmsCacheEntry(v & 0x07, (v & 0x70) >> 4));
			}
		}
		break;
	case 2:
		while (c < compressed.size()){
			int v = (uchar)compressed[c++];
			if (v & 0x80){
				int size = v - 0x80;
				if (size > 0){
					for (int i=0; i<size; i++){
						entries.append(RmsCacheEntry(0, 0));
					}
				}else{
					if (c > compressed.size() - 2){
						qDebug() << "malformed 2-a";
						break;
					}
					int sl = (uchar)compressed[c++];
					int sh = (uchar)compressed[c++];
					int sz = sl | (sh << 8);
					for (int i=0; i<sz; i++){
						entries.append(RmsCacheEntry(0, 0));
					}
				}
			}else{
				if (c >= compressed.size()){
					qDebug() << "malformed 2-b";
					break;
				}
				int w = (uchar)compressed[c++];
				entries.append(RmsCacheEntry(v, w));
			}
		}
		break;
	default:
		break;
	}
	return entries;
}







SoundChannelResourceManager::SoundChannelResourceManager(QObject *parent)
	: QObject(parent)
	, wave(nullptr)
	, overallWaveform(320, 160, QImage::Format_ARGB32_Premultiplied)
{
	auxBuffer = new char[auxBufferSize];
}

SoundChannelResourceManager::~SoundChannelResourceManager()
{
	delete[] auxBuffer;
}

void SoundChannelResourceManager::UpdateWaveData(const QString &srcPath)
{
	if (currentTask.isRunning()){
		// cannot start task before current task finishes
		currentTask.waitForFinished();
	}
	if (wave){
		delete wave;
		wave = nullptr;
	}
	overallWaveform.fill(0x00000000);
	rmsCachePackets.clear();
	file = QFileInfo(srcPath);
	wave = SoundChannelUtil::OpenSourceFile(srcPath, this);
	currentTask = QtConcurrent::run([this](){
		RunTaskWaveData();
	});
}

void SoundChannelResourceManager::RequireRmsCachePacket(int position)
{
	// uncompression may be done before whole compression completes.
	QtConcurrent::run([=](){
		RunTaskRmsCachePacket(position);
	});
//	currentTask = QtConcurrent::run([this](QFuture<void> currentTask, int position){
//		if (currentTask.isRunning())
//			currentTask.waitForFinished();
//		RunTaskRmsCachePacket(position);
//	}, currentTask, position);
}

void SoundChannelResourceManager::RunTaskWaveData()
{
	if (!TaskLoadWaveSummary()){
		emit WaveSummaryReady(&summary);
		return;
	}
	emit WaveSummaryReady(&summary);

	TaskDrawOverallWaveformAndRmsCache();
	emit OverallWaveformReady();
	emit RmsCacheUpdated();
}

void SoundChannelResourceManager::RunTaskRmsCachePacket(int position)
{
	QMap<quint64, RmsCachePacket>::const_iterator i = rmsCachePackets.find(position);
	if (i == rmsCachePackets.end()){
		// packet not found
		emit RmsCachePacketReady(position, QList<RmsCacheEntry>());
		return;
	}
	emit RmsCachePacketReady(position, i->Uncompress());
}

bool SoundChannelResourceManager::TaskLoadWaveSummary()
{
	if (!wave){
		qDebug() << "No Such Audio File (or Unknown File Type): " << file.path();
		summary = WaveSummary();
		return false;
	}
	int error = wave->Open();
	if (error != 0){
		qDebug() << "Audio File Error: " << error;
		summary = WaveSummary();
		return false;
	}
	summary.Format = wave->GetFormat();
	summary.FrameCount = wave->GetFrameCount();
	return true;
}

void SoundChannelResourceManager::TaskDrawOverallWaveformAndRmsCache()
{
	static const quint64 BufferSize = 4096;
	wave->SeekAbsolute(0);
	const int width = overallWaveform.width();
	const int height = overallWaveform.height();
	quint32 *temp = new quint32[width*height];
	std::memset(temp, 0, sizeof(quint32)*width*height);
	char *buffer = new char[BufferSize];
	quint64 ismp = 0;
	float dx = float(width) / summary.FrameCount;
	const int ditherRes = 32;
	float dxd = dx*ditherRes;
	QList<RmsCacheEntry> rmsBuf;
	QPair<float, float> rmsCur(0, 0);
	int rmsCurSize = 0;
	int rmsCurPos = 0;
	wave->EnumerateAllAsFloat([&](float v){
		// when Monoral
		float sqv = v*v;
		rmsCur.first += sqv;
		rmsCur.second += sqv;
		if (++rmsCurSize >= RmsCacheBlockSize){
			RmsCacheEntry entry(std::sqrtf(rmsCur.first / RmsCacheBlockSize) * 127.0f, std::sqrtf(rmsCur.second / RmsCacheBlockSize) * 127.0f);
			rmsBuf.append(entry);
			rmsCur.first = 0.0f;
			rmsCur.second = 0.0f;
			rmsCurSize = 0;
			if (rmsBuf.size() >= RmsCachePacketSize){
				QMutexLocker lockerRms(&rmsCacheMutex);
				rmsCachePackets.insert(rmsCurPos, RmsCachePacket(rmsBuf, RmsCachePacketSize));
				rmsBuf.erase(rmsBuf.begin(), rmsBuf.begin()+RmsCachePacketSize);
				rmsCurPos += RmsCachePacketSampleCount;
			}
		}
		float x = (float)ismp * dxd;
		float y = (0.5f - 0.48*v) * height;
		int ix = (int(x) + (std::rand() - RAND_MAX/2)/(RAND_MAX/2/ditherRes) ) / ditherRes;
		if (ix < width && ix >= 0){
			temp[ix * height + int(y)]++;
		}
		ismp++;
	}, [&](float l, float r){
		// when Stereo
		rmsCur.first += l*l;
		rmsCur.second += r*r;
		if (++rmsCurSize >= RmsCacheBlockSize){
			RmsCacheEntry entry(std::sqrtf(rmsCur.first / RmsCacheBlockSize) * 127.0f, std::sqrtf(rmsCur.second / RmsCacheBlockSize) * 127.0f);
			rmsBuf.append(entry);
			rmsCur.first = 0.0f;
			rmsCur.second = 0.0f;
			rmsCurSize = 0;
			if (rmsBuf.size() >= RmsCachePacketSize){
				QMutexLocker lockerRms(&rmsCacheMutex);
				rmsCachePackets.insert(rmsCurPos, RmsCachePacket(rmsBuf, RmsCachePacketSize));
				rmsBuf.erase(rmsBuf.begin(), rmsBuf.begin()+RmsCachePacketSize);
				rmsCurPos += RmsCachePacketSampleCount;
			}
		}
		float x = (float)ismp * dxd;
		int ix = (int(x) + (std::rand() - RAND_MAX/2)/(RAND_MAX/2/ditherRes) ) / ditherRes;
		if (ix < width && ix >= 0){
			temp[ix * height + int((0.25f - 0.24*l) * height)]++;
			temp[ix * height + int((0.75f - 0.24*r) * height)]++;
		}
		ismp++;
	});
	if (rmsBuf.size() > 0){
		QMutexLocker lockerRms(&rmsCacheMutex);
		rmsCachePackets.insert(rmsCurPos, RmsCachePacket(rmsBuf, rmsBuf.size()));
	}
	delete[] buffer;
	const int density = wave->GetFrameCount() / width;
	const int hd = wave->GetFormat().channelCount() == 1 ? height/2 : height/4;
	const int shd = hd*hd;
	for (int y=0; y<height; y++){
		quint32 *sl = (quint32 *)overallWaveform.scanLine(y);
		int amp = wave->GetFormat().channelCount() == 1
				? (y-height/2)*(y-height/2)
				: (y < height/2 ? (y-height/4)*(y-height/4) : (y-height*3/4)*(y-height*3/4) );
		int u = 32 * amp / shd;
		for (int x=0; x<width; x++){
			int v = temp[x*height + y] * 256 / density;
			int r = std::min(255, v * std::max(0, 48 - u));
			int g = std::min(255, v * (32 + u + u*u/4));
			int b = std::min(255, v * (16 + u/4 + 4*u*u));
			quint32 u = (0xFF << 24) | (r << 16) | (g << 8) | b;
			*sl++ = u;
		}
	}
	delete[] temp;
}

quint64 SoundChannelResourceManager::ReadAsS16S(QAudioBuffer::S16S *buffer, quint64 frames)
{
	const int frameSize = wave->GetFormat().bytesPerFrame();
	quint64 requiredSize = frameSize * frames;
	quint64 framesRead = 0;
	while (requiredSize > auxBufferSize){
		quint64 sizeRead = wave->Read(auxBuffer, auxBufferSize);
		if (sizeRead == 0){
			return framesRead;
		}
		ConvertAuxBufferToS16S(buffer + framesRead, sizeRead / frameSize);
		framesRead += sizeRead / frameSize;
		requiredSize -= auxBufferSize;
	}
	{
		quint64 sizeRead = wave->Read(auxBuffer, requiredSize);
		if (sizeRead == 0){
			return framesRead;
		}
		ConvertAuxBufferToS16S(buffer + framesRead, sizeRead / frameSize);
		framesRead += sizeRead / frameSize;
		return framesRead;
	}
}

void SoundChannelResourceManager::ConvertAuxBufferToS16S(QAudioBuffer::S16S *buffer, quint64 frames)
{
	const qint8 *abEnd = (const qint8*)auxBuffer + frames * wave->GetFormat().bytesPerFrame();
	switch (wave->GetFormat().sampleSize()){
	case 8:
		switch (wave->GetFormat().channelCount()){
		case 1:
			switch (wave->GetFormat().sampleType()){
			case QAudioFormat::UnSignedInt:
				for (int i=0; i<frames; i++){
					quint8 c = ((quint8*)auxBuffer)[i];
					buffer[i] = QAudioBuffer::StereoFrame<qint16>(((int)c - 128)*256, ((int)c - 128)*256);
				}
				break;
			case QAudioFormat::SignedInt:
				for (int i=0; i<frames; i++){
					qint8 c = ((qint8*)auxBuffer)[i];
					buffer[i] = QAudioBuffer::StereoFrame<qint16>((int)c*256, (int)c*256);
				}
				break;
			}
			break;
		case 2:
			switch (wave->GetFormat().sampleType()){
			case QAudioFormat::UnSignedInt:
				for (int i=0; i<frames; i++){
					QAudioBuffer::S8U s = ((QAudioBuffer::S8U*)auxBuffer)[i];
					buffer[i] = QAudioBuffer::StereoFrame<qint16>(((int)s.left - 128)*256, ((int)s.right - 128)*256);
				}
				break;
			case QAudioFormat::SignedInt:
				for (int i=0; i<frames; i++){
					QAudioBuffer::S8S s = ((QAudioBuffer::S8S*)auxBuffer)[i];
					buffer[i] = QAudioBuffer::StereoFrame<qint16>((int)s.left*256, (int)s.right*256);
				}
				break;
			}
			break;
		}
		break;
	case 16:
		switch (wave->GetFormat().channelCount()){
		case 1:
			switch (wave->GetFormat().sampleType()){
			case QAudioFormat::UnSignedInt:
				for (int i=0; i<frames; i++){
					quint16 c = ((quint16*)auxBuffer)[i];
					buffer[i] = QAudioBuffer::StereoFrame<qint16>((int)c - 32768, (int)c - 32768);
				}
				break;
			case QAudioFormat::SignedInt:
				for (int i=0; i<frames; i++){
					qint16 c = ((qint16*)auxBuffer)[i];
					buffer[i] = QAudioBuffer::StereoFrame<qint16>(c, c);
				}
				break;
			}
			break;
		case 2:
			switch (wave->GetFormat().sampleType()){
			case QAudioFormat::UnSignedInt:
				for (int i=0; i<frames; i++){
					QAudioBuffer::S16U s = ((QAudioBuffer::S16U*)auxBuffer)[i];
					buffer[i] = QAudioBuffer::StereoFrame<qint16>((int)s.left - 32768, (int)s.right - 32768);
				}
				break;
			case QAudioFormat::SignedInt:
				for (int i=0; i<frames; i++){
					buffer[i] = ((QAudioBuffer::S16S*)auxBuffer)[i];
				}
				break;
			}
			break;
		}
		break;
	case 24:
		switch (wave->GetFormat().channelCount()){
		case 1:
			switch (wave->GetFormat().sampleType()){
			case QAudioFormat::UnSignedInt:
				for (const qint8* b=(const qint8*)auxBuffer; b<abEnd; ){
					b++;
					qint32 vl = *b++;
					qint32 vh = *(const quint8*)b++;
					qint16 s = qint16((vl | (vh<<8)) - 32768);
					*buffer++ = QAudioBuffer::StereoFrame<qint16>(s, s);
				}
				break;
			case QAudioFormat::SignedInt:
				for (const qint8* b=(const qint8*)auxBuffer; b<abEnd; ){
					b++;
					qint32 vl = *b++;
					qint32 vh = *b++;
					qint16 s = qint16(vl | (vh<<8));
					*buffer++ = QAudioBuffer::StereoFrame<qint16>(s, s);
				}
				break;
			}
			break;
		case 2:
			switch (wave->GetFormat().sampleType()){
			case QAudioFormat::UnSignedInt:
				for (const qint8* b=(const qint8*)auxBuffer; b<abEnd; ){
					b++;
					qint32 ll = *b++;
					qint32 lh = *(const quint8*)b++;
					b++;
					qint32 rl = *b++;
					qint32 rh = *(const quint8*)b++;
					*buffer++ = QAudioBuffer::StereoFrame<qint16>(qint16((ll | (lh<<8)) - 32768), qint16((rl | (rh<<8)) - 32768));
				}
				break;
			case QAudioFormat::SignedInt:
				for (const qint8* b=(const qint8*)auxBuffer; b<abEnd; ){
					b++;
					qint32 ll = *b++;
					qint32 lh = *b++;
					b++;
					qint32 rl = *b++;
					qint32 rh = *b++;
					*buffer++ = QAudioBuffer::StereoFrame<qint16>(qint16(ll | (lh<<8)), qint16(rl | (rh<<8)));
				}
				break;
			}
			break;
		}
		break;
	case 32:
		switch (wave->GetFormat().channelCount()){
		case 1:
			switch (wave->GetFormat().sampleType()){
			case QAudioFormat::Float:
				for (int i=0; i<frames; i++){
					auto s = ((float*)auxBuffer)[i];
					qint16 c = std::max<qint16>(-32768, std::min<qint16>(32767, s));
					buffer[i] = QAudioBuffer::StereoFrame<qint16>(c, c);
				}
				break;
			case QAudioFormat::UnSignedInt:
				for (int i=0; i<frames; i++){
					auto s = ((quint32*)auxBuffer)[i];
					buffer[i] = QAudioBuffer::StereoFrame<qint16>(int(s - 2147483648)/65536, int(s - 2147483648)/65536);
				}
				break;
			case QAudioFormat::SignedInt:
				for (int i=0; i<frames; i++){
					auto s = ((qint32*)auxBuffer)[i];
					buffer[i] = QAudioBuffer::StereoFrame<qint16>(s/65536, s/65536);
				}
				break;
			}
			break;
		case 2:
			switch (wave->GetFormat().sampleType()){
			case QAudioFormat::Float:
				for (int i=0; i<frames; i++){
					auto s = ((QAudioBuffer::S32F*)auxBuffer)[i];
					buffer[i] = QAudioBuffer::StereoFrame<qint16>(
								std::max<qint16>(-32768, std::min<qint16>(32767, s.left)),
								std::max<qint16>(-32768, std::min<qint16>(32767, s.right)));
				}
				break;
			case QAudioFormat::UnSignedInt:
				for (int i=0; i<frames; i++){
					auto s = ((QAudioBuffer::StereoFrame<quint32>*)auxBuffer)[i];
					buffer[i] = QAudioBuffer::StereoFrame<qint16>(int(s.left - 2147483648)/65536, int(s.right - 2147483648)/65536);
				}
				break;
			case QAudioFormat::SignedInt:
				for (int i=0; i<frames; i++){
					auto s = ((QAudioBuffer::StereoFrame<qint32>*)auxBuffer)[i];
					buffer[i] = QAudioBuffer::StereoFrame<qint16>(s.left/65536, s.right/65536);
				}
				break;
			}
			break;
		}
		break;
	}
}







SoundChannelSourceFilePreviewer::SoundChannelSourceFilePreviewer(SoundChannel *channel, QObject *parent)
	: AudioPlaySource(parent)
	, wave(nullptr)
{
	QString srcPath = channel->document->GetAbsolutePath(channel->fileName);
	AudioStreamSource *native = SoundChannelUtil::OpenSourceFile(srcPath, this);
	if (native){
		wave = new S16S44100StreamTransformer(native);
		wave->Open();
	}
}

SoundChannelSourceFilePreviewer::~SoundChannelSourceFilePreviewer()
{
	//qDebug() << "~SoundChannelSourceFilePreviewer";
}

void SoundChannelSourceFilePreviewer::AudioPlayRelease()
{
	//qDebug() << "AudioPlayRelease";
	if (!wave){
		return;
	}
	delete wave;
	wave = nullptr;
	emit Stopped();
}

int SoundChannelSourceFilePreviewer::AudioPlayRead(AudioPlaySource::SampleType *buffer, int bufferSampleCount)
{
	if (!wave){
		return 0;
	}
	return wave->Read(buffer, bufferSampleCount);
}






SoundChannelNotePreviewer::SoundChannelNotePreviewer(SoundChannel *channel, int location, QObject *parent)
	: AudioPlaySource(parent)
	, SamplesPerSec(44100.0)
	, SamplesPerSecOrg(channel->waveSummary->Format.sampleRate())
	, TicksPerBeat(channel->document->GetTimeBase())
	, wave(nullptr)
	, cache(channel->cache)
{
	// scale any sample positions
	const double ratio = SamplesPerSec / SamplesPerSecOrg;
	for (SoundChannel::CacheEntry &c : cache){
		if (c.prevSamplePosition >= 0){
			c.prevSamplePosition *= ratio;
		}
		if (c.currentSamplePosition >= 0){
			c.currentSamplePosition *= ratio;
		}
	}

	auto inote = channel->notes.find(location);
	if (inote == channel->notes.end())
		return;
	note = inote.value();
	if (++inote == channel->notes.end()){
		nextNoteLocation = INT_MAX;
	}else{
		nextNoteLocation = inote.key();
	}
	icache = cache.lowerBound(location);
	if (icache.key() == location){
		// exactly hit on an Event!
		if (icache->currentSamplePosition >= 0){
			currentBpm = icache->currentTempo;
			currentSamplePos = icache->currentSamplePosition;
			icache++;
		}else{
			// no play
			return;
		}
	}else{
		if (icache == cache.begin() || icache->prevSamplePosition<0){
			// only silent slicing notes can come here
			return;
		}
		currentBpm = icache->prevTempo;
		currentSamplePos = icache->prevSamplePosition - (icache.key() - location)*(60.0 * SamplesPerSec / (icache->prevTempo * TicksPerBeat));
	}
	currentTicks = location;
	QString srcPath = channel->document->GetAbsolutePath(channel->fileName);
	AudioStreamSource *native = SoundChannelUtil::OpenSourceFile(srcPath, this);
	if (!native)
		return;
	wave = new S16S44100StreamTransformer(native);
	wave->Open();
	wave->SeekAbsolute(currentSamplePos);
}

SoundChannelNotePreviewer::~SoundChannelNotePreviewer()
{
}


void SoundChannelNotePreviewer::AudioPlayRelease()
{
	if (!wave)
		return;
	delete wave;
	wave = nullptr;
	emit Stopped();
}

int SoundChannelNotePreviewer::AudioPlayRead(AudioPlaySource::SampleType *buffer, int bufferSampleCount)
{
	if (!wave)
		return 0;
	const int samplesRead = wave->Read(buffer, bufferSampleCount);
	if (samplesRead == 0){
		return 0;
	}
	double samples = samplesRead;
	while (true){
		double nextExpectedTicks = currentTicks + (samples/SamplesPerSec)*currentBpm*TicksPerBeat/60.0;
		if (nextExpectedTicks <= icache.key()){
			// no BPM events
			if (nextExpectedTicks >= nextNoteLocation){
				// sound is cut
				int endSamples = (nextNoteLocation-currentTicks) * (60.0 * SamplesPerSec / (currentBpm * TicksPerBeat));
				//qDebug() << "Sound is cut by NEXT CUTTING NOTE" << currentTicks << currentSamplePos;
				currentTicks = nextNoteLocation;
				currentSamplePos += endSamples;
				return std::min(samplesRead, int((samplesRead - samples) + endSamples)); // use only samples until end
			}else{
				// continue
				currentTicks = nextExpectedTicks;
				currentSamplePos += samplesRead;
				return samplesRead;
			}
		}else if (icache.key() < nextNoteLocation){
			// occurs some event(s) (*icache)
			double samplesAtEvent = (icache.key() - currentTicks) * (60.0 * SamplesPerSec / (currentBpm * TicksPerBeat));
			if (icache->prevSamplePosition != icache->currentSamplePosition){
				// cut (restart)
				// (currently, this cannot happen??)
				currentTicks = icache.key();
				//qDebug() << "Sound is cut by NEXT RESTART NOTE" << currentTicks << currentSamplePos;
				currentSamplePos += samplesAtEvent;
				icache++;
				return std::min(samplesRead, int((samplesRead - samples) + samplesAtEvent));
			}else{
				// BPM event
				//qDebug() << "BPM changes to " << icache->currentTempo << "        " << currentTicks;
				currentTicks = icache.key();
				currentSamplePos += icache->currentSamplePosition;
				currentBpm = icache->currentTempo;
				samples -= samplesAtEvent;
				icache++;
			}
		}else{
			int endSamples = (nextNoteLocation-currentTicks) * (60.0 * SamplesPerSec / (currentBpm * TicksPerBeat));
			currentTicks = nextNoteLocation;
			currentSamplePos += endSamples;
			return std::min(samplesRead, int((samplesRead - samples) + endSamples)); // use only samples until end
		}
	}
}



