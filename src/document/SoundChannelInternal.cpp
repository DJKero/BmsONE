#include "SoundChannel.h"
#include "SoundChannelInternal.h"
#include "qtconcurrentrun.h"
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
    for (auto i=std::next(regs.begin()); i!=regs.end(); i++){
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
	quint8 peak = 0;
	QMap<int, int> silent;
	int silentCur = -1;
	for (int i=0; i<count; i++){
		if (entries[i].L > peak)
			peak = entries[i].L;
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
	}else if (peak <= 0x0E){
		compressed.append(0x01);
		QMap<int, int>::const_iterator isilent = silent.begin();
		for (int i=0; i<count; i++){
			if (isilent != silent.end() && isilent.key() == i){
				if (isilent.value() >= 0x100){
					compressed.append(char(uchar(0xFE)));
					compressed.append(char(uchar(isilent.value() & 0xFF)));
					compressed.append(char(uchar((isilent.value() & 0xFF00) >> 8)));
				}else{
					compressed.append(char(uchar(0xFF)));
					compressed.append(char(uchar(isilent.value())));
				}
				i += isilent.value() - 1;
				isilent++;
			}else{
				compressed.append(entries[i].L | (entries[i].R << 4));
			}
		}
	}else if (peak <= 0xEF){
		compressed.append(0x02);
		QMap<int, int>::const_iterator isilent = silent.begin();
		for (int i=0; i<count; i++){
			if (isilent != silent.end() && isilent.key() == i){
				if (isilent.value() >= 0x100){
					compressed.append(char(uchar(0xFE)));
					compressed.append(char(uchar(isilent.value() & 0xFF)));
					compressed.append(char(uchar((isilent.value() & 0xFF00) >> 8)));
				}else{
					compressed.append(char(uchar(0xFF)));
					compressed.append(char(uchar(isilent.value())));
				}
				i += isilent.value() - 1;
				isilent++;
			}else{
				compressed.append(char(uchar(entries[i].L)));
				compressed.append(char(uchar(entries[i].R)));
			}
		}
	}else{
		qDebug() << "malformed source: peak >= F0";
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
	int c=1;
	switch (compressed[0]){
	case 1:
		while (c < compressed.size()){
			uchar v = (uchar)compressed[c++];
			if (v >= 0xF0){
				if (v == 0xFF){
#ifdef _DEBUG
					if (c > compressed.size() - 1){
						qDebug() << "malformed 1-FF";
						break;
					}
#endif
					int sz = (uchar)compressed[c++];
					for (int i=0; i<sz; i++){
						entries.append(RmsCacheEntry(0, 0));
					}
				}else if (v == 0xFE){
#ifdef _DEBUG
					if (c > compressed.size() - 2){
						qDebug() << "malformed 1-FE";
						break;
					}
#endif
					int sl = (uchar)compressed[c++];
					int sh = (uchar)compressed[c++];
					int sz = sl | (sh << 8);
					for (int i=0; i<sz; i++){
						entries.append(RmsCacheEntry(0, 0));
					}
				}else{
					qDebug() << "malformed 1-Fx" << v;
					break;
				}
			}else{
				entries.append(RmsCacheEntry(v & 0x0F, (v & 0xF0) >> 4));
			}
		}
		break;
	case 2:
		while (c < compressed.size()){
			uchar v = (uchar)compressed[c++];
			if (v >= 0xF0){
				if (v == 0xFF){
#ifdef _DEBUG
					if (c > compressed.size() - 1){
						qDebug() << "malformed 2-FF";
						break;
					}
#endif
					int sz = (uchar)compressed[c++];
					for (int i=0; i<sz; i++){
						entries.append(RmsCacheEntry(0, 0));
					}
				}else if (v == 0xFE){
#ifdef _DEBUG
					if (c > compressed.size() - 2){
						qDebug() << "malformed 2-FE";
						break;
					}
#endif
					int sl = (uchar)compressed[c++];
					int sh = (uchar)compressed[c++];
					int sz = sl | (sh << 8);
					for (int i=0; i<sz; i++){
						entries.append(RmsCacheEntry(0, 0));
					}
				}else{
					qDebug() << "malformed 2-Fx";
					break;
				}
			}else{
#ifdef _DEBUG
				if (c >= compressed.size()){
					qDebug() << "malformed 2-b";
					break;
				}
#endif
				uchar w = (uchar)compressed[c++];
				entries.append(RmsCacheEntry(v, w));
			}
		}
		break;
	default: // includes silence
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

	// Load Wave Summary
	if (!wave){
		qDebug() << "No Such Audio File (or Unknown File Type): " << file.path();
		summary = WaveSummary();
		return;
	}
	int error = wave->Open();
	if (error != 0){
		qDebug() << "Audio File Error: " << error;
		summary = WaveSummary();
		return;
	}
	summary.Format = wave->GetFormat();
	summary.FrameCount = wave->GetFrameCount();

    currentTask = QtConcurrent::run([this](){
		RunTaskWaveData();
	});
}

void SoundChannelResourceManager::RequireRmsCachePacket(int position)
{
	// uncompression may be done before whole compression completes.
    auto future = QtConcurrent::run([=](){
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

void SoundChannelResourceManager::TaskDrawOverallWaveformAndRmsCache()
{
    [[maybe_unused]] static const quint64 BufferSize = 4096;
	wave->SeekAbsolute(0);
	const int width = overallWaveform.width();
	const int height = overallWaveform.height();
	int samplesPerPixel = summary.FrameCount / width;
	float dx = float(width) / summary.FrameCount;
	Rms wfPeak, wfRms;
	QList<RmsCacheEntry> waveformPeak, waveformRms;
	int ismpPx = 0;
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
            RmsCacheEntry entry(std::sqrt(rmsCur.first / RmsCacheBlockSize) * 127.0f, std::sqrt(rmsCur.second / RmsCacheBlockSize) * 127.0f);
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
		wfRms.L += sqv;
        if (std::fabs(v) > wfPeak.L) wfPeak.L = std::fabs(v);
		if (++ismpPx >= samplesPerPixel){
			waveformPeak.append(RmsCacheEntry(wfPeak.L * 255, wfPeak.L * 255));
            waveformRms.append(RmsCacheEntry(std::sqrt(wfRms.L*dx) * 255, std::sqrt(wfRms.L*dx) * 255));
			wfPeak = Rms();
			wfRms = Rms();
			ismpPx = 0;
		}
	}, [&](float l, float r){
		// when Stereo
		rmsCur.first += l*l;
		rmsCur.second += r*r;
		if (++rmsCurSize >= RmsCacheBlockSize){
            RmsCacheEntry entry(std::sqrt(rmsCur.first / RmsCacheBlockSize) * 127.0f, std::sqrt(rmsCur.second / RmsCacheBlockSize) * 127.0f);
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
		wfRms.L += l*l;
		wfRms.R += r*r;
        if (std::fabs(l) > wfPeak.L) wfPeak.L = std::fabs(l);
        if (std::fabs(r) > wfPeak.R) wfPeak.R = std::fabs(r);
		if (++ismpPx >= samplesPerPixel){
			waveformPeak.append(RmsCacheEntry(wfPeak.L * 255, wfPeak.R * 255));
            waveformRms.append(RmsCacheEntry(std::sqrt(wfRms.L*dx) * 255, std::sqrt(wfRms.R*dx) * 255));
			wfPeak = Rms();
			wfRms = Rms();
			ismpPx = 0;
		}
	});
	if (rmsBuf.size() > 0){
		QMutexLocker lockerRms(&rmsCacheMutex);
		rmsCachePackets.insert(rmsCurPos, RmsCachePacket(rmsBuf, rmsBuf.size()));
	}
	overallWaveform.fill(QColor(0, 0, 0));
	if (summary.Format.channelCount() == 1){
		QPainter painter(&overallWaveform);
        int w = std::min(width, (int) waveformPeak.size());
		painter.setPen(QColor(0, 0x99, 0));
		for (int i=0; i<w; i++){
			painter.drawLine(i, height/2 - waveformPeak[i].L*height/2/255, i, height/2 + waveformPeak[i].L*height/2/255);
		}
		painter.setPen(QColor(0x66, 0xCC, 0x66));
		for (int i=0; i<w; i++){
			painter.drawLine(i, height/2 - waveformRms[i].L*height/2/255, i, height/2 + waveformRms[i].L*height/2/255);
		}
	}else{
		QPainter painter(&overallWaveform);
        int w = std::min(width, (int) waveformPeak.size());
		painter.setPen(QColor(0, 0x99, 0));
		for (int i=0; i<w; i++){
			painter.drawLine(i, height/4 - waveformPeak[i].L*height/4/255, i, height/4 + waveformPeak[i].L*height/4/255);
			painter.drawLine(i, height*3/4 - waveformPeak[i].R*height/4/255, i, height*3/4 + waveformPeak[i].R*height/4/255);
		}
		painter.setPen(QColor(0x66, 0xCC, 0x66));
		for (int i=0; i<w; i++){
			painter.drawLine(i, height/4 - waveformRms[i].L*height/4/255, i, height/4 + waveformRms[i].L*height/4/255);
			painter.drawLine(i, height*3/4 - waveformRms[i].R*height/4/255, i, height*3/4 + waveformRms[i].R*height/4/255);
		}
	}
//	int totalSize = 0;
//	for (auto p : rmsCachePackets){
//		totalSize += p.GetSize();
//	}
//	qDebug() << this->file.baseName() << "\t" << 2*wave->GetFrameCount()/RmsCacheBlockSize << " -> " << totalSize
//			 << " (" << totalSize*100/(2*wave->GetFrameCount()/RmsCacheBlockSize) << "%)";
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
    QAudioFormat fmt = wave->GetFormat();
	const qint8 *abEnd = (const qint8*)auxBuffer + frames * wave->GetFormat().bytesPerFrame();

    switch (fmt.channelConfig()){
    case QAudioFormat::ChannelConfigMono:
        switch (fmt.bytesPerFrame()){
        // 8bit mono
        case 1:
            switch (fmt.sampleFormat()){
            // 8bit unsigned int mono
            case QAudioFormat::UInt8:{
                for (quint64 i=0; i<frames; i++){
                    quint8 c = ((quint8*)auxBuffer)[i];
                    buffer[i].setValue(QAudioFormat::FrontLeft, ((int)c - 128)*256);
                    buffer[i].setValue(QAudioFormat::FrontRight, ((int)c - 128)*256);
                }
            } break;
            // 8bit signed int mono
            case QAudioFormat::Unknown:{
                for (quint64 i=0; i<frames; i++){
                    qint8 c = ((qint8*)auxBuffer)[i];
                    buffer[i].setValue(QAudioFormat::FrontLeft, (int)c*256);
                    buffer[i].setValue(QAudioFormat::FrontRight, (int)c*256);
                }
            } break;
            default:
                break;
            }
            break;
        // 16bit mono
        case 2:
            switch (fmt.sampleFormat()){
            // 16bit unsigned int mono
            case QAudioFormat::Unknown: {
                for (quint64 i=0; i<frames; i++){
                    quint16 c = ((quint16*)auxBuffer)[i];
                    buffer[i].setValue(QAudioFormat::FrontLeft, (int)c - 32768);
                    buffer[i].setValue(QAudioFormat::FrontRight, (int)c - 32768);
                }
            } break;
            // 16bit signed int mono
            case QAudioFormat::Int16:{
                for (quint64 i=0; i<frames; i++){
                    qint16 c = ((qint16*)auxBuffer)[i];
                    buffer[i].setValue(QAudioFormat::FrontLeft, c);
                    buffer[i].setValue(QAudioFormat::FrontRight, c);
                }
            } break;
            default:
                break;
            }
            break;
        // 24bit mono
        case 3:
            switch (fmt.sampleFormat()){
            // 24bit unsigned int mono
            case QAudioFormat::Unknown:{
                /*for (const qint8* b=(const qint8*)auxBuffer; b<abEnd; ){
                    b++;
                    qint32 vl = *b++;
                    qint32 vh = *(const quint8*)b++;
                    qint16 s = qint16((vl | (vh<<8)) - 32768);
                    *buffer++ = QAudioBuffer::StereoFrame<qint16>(s, s);
                }*/
                break;
            } break;
            // 24bit signed int mono
            case QAudioFormat::NSampleFormats:{
                for (const qint8* b=(const qint8*)auxBuffer; b<abEnd; ){
                    b++;
                    qint32 vl = *b++;
                    qint32 vh = *b++;
                    qint16 s = qint16(vl | (vh<<8));
                    // Maybe introduced a bug here?
                    QAudioBuffer::S16S buf = QAudioBuffer::S16S();
                    buf.setValue(QAudioFormat::FrontLeft, s);
                    buf.setValue(QAudioFormat::FrontRight, s);
                    *buffer++ = buf;
                }
            } break;
            default:
                break;
            }
            break;
        // 32bit mono
        case 4:
            switch (fmt.sampleFormat()){
            // 32bit float mono
            case QAudioFormat::Float:{
                for (quint64 i=0; i<frames; i++){
                    auto s = ((float*)auxBuffer)[i];
                    qint16 c = std::max<qint16>(-32768, std::min<qint16>(32767, s));
                    QAudioBuffer::S16S buf = QAudioBuffer::S16S();
                    buf.setValue(QAudioFormat::FrontLeft, c);
                    buf.setValue(QAudioFormat::FrontRight, c);
                    buffer[i] = buf;
                }
            } break;
            // 32bit unsigned int mono
            // 32bit signed int mono
            default:
                break;
            }
            break;
        }
        break; // end mono
    case QAudioFormat::ChannelConfigStereo:
        switch (fmt.bytesPerFrame()){
        // 8bit stereo
        case 1:
            switch (fmt.sampleFormat()){
            // 8bit unsigned int stereo
            case QAudioFormat::UInt8:{
                for (quint64 i=0; i<frames; i++){
                    QAudioBuffer::U8S s = ((QAudioBuffer::U8S*)auxBuffer)[i];
                    buffer[i].setValue(QAudioFormat::FrontLeft, ((int)s.value(QAudioFormat::FrontLeft) - 128)*256);
                    buffer[i].setValue(QAudioFormat::FrontRight, ((int)s.value(QAudioFormat::FrontRight) - 128)*256);
                }
            } break;
            // 8bit signed int stereo
            case QAudioFormat::Unknown:{
                /*for (quint64 i=0; i<frames; i++){
                    QAudioBuffer::S8S s = ((QAudioBuffer::S8S*)auxBuffer)[i];
                    buffer[i] = QAudioBuffer::StereoFrame<qint16>((int)s.left*256, (int)s.right*256);
                }*/
            } break;
            default:
                break;
            }
            break;
        // 16bit stereo
        case 2:
            switch (fmt.sampleFormat()){
            // 16bit unsigned int stereo
            case QAudioFormat::Unknown: {
                /*for (quint64 i=0; i<frames; i++){
                    QAudioBuffer::S16U s = ((QAudioBuffer::S16U*)auxBuffer)[i];
                    buffer[i] = QAudioBuffer::StereoFrame<qint16>((int)s.left - 32768, (int)s.right - 32768);
                }*/
            } break;
            // 16bit signed int stereo
            case QAudioFormat::Int16:{
                for (quint64 i=0; i<frames; i++){
                    buffer[i] = ((QAudioBuffer::S16S*)auxBuffer)[i];
                }
            } break;
            default:
                break;
            }
            break;
        // 24bit stereo
        case 3:
            switch (fmt.sampleFormat()){
            // 24bit unsigned int stereo
            case QAudioFormat::Unknown:{
                /*for (const qint8* b=(const qint8*)auxBuffer; b<abEnd; ){
                    b++;
                    qint32 ll = *b++;
                    qint32 lh = *(const quint8*)b++;
                    b++;
                    qint32 rl = *b++;
                    qint32 rh = *(const quint8*)b++;
                    *buffer++ = QAudioBuffer::StereoFrame<qint16>(qint16((ll | (lh<<8)) - 32768), qint16((rl | (rh<<8)) - 32768));
                }*/
            } break;
            // 24bit signed int stereo
            case QAudioFormat::NSampleFormats:{
                for (const qint8* b=(const qint8*)auxBuffer; b<abEnd; ){
                    b++;
                    qint32 ll = *b++;
                    qint32 lh = *b++;
                    b++;
                    qint32 rl = *b++;
                    qint32 rh = *b++;
                    QAudioBuffer::S16S buf = QAudioBuffer::S16S();
                    buf.setValue(QAudioFormat::FrontLeft, qint16(ll | (lh<<8)));
                    buf.setValue(QAudioFormat::FrontRight, qint16(rl | (rh<<8)));
                    *buffer++ = buf;
                }
            } break;
            default:
                break;
            }
            break;
        // 32bit stereo
        case 4:
            switch (fmt.sampleFormat()){
            // 32bit float stereo
            case QAudioFormat::Float:{
                for (quint64 i=0; i<frames; i++){
                    auto s = ((QAudioBuffer::F32S*)auxBuffer)[i];
                    QAudioBuffer::S16S buf = QAudioBuffer::S16S();
                    buf.setValue(QAudioFormat::FrontLeft, std::max<qint16>(-32768, std::min<qint16>(32767, s.value(QAudioFormat::FrontLeft))));
                    buf.setValue(QAudioFormat::FrontRight, std::max<qint16>(-32768, std::min<qint16>(32767, s.value(QAudioFormat::FrontRight))));
                    buffer[i] = buf;
                }
            } break;
            // 32bit unsigned int stereo
            case QAudioFormat::Unknown:{
                /*for (quint64 i=0; i<frames; i++){
                    auto s = ((QAudioBuffer::StereoFrame<quint32>*)auxBuffer)[i];
                    buffer[i] = QAudioBuffer::StereoFrame<qint16>(int(s.left - 2147483648)/65536, int(s.right - 2147483648)/65536);
                }*/
            } break;
            // 32bit signed int stereo
            case QAudioFormat::NSampleFormats:{
                /*for (quint64 i=0; i<frames; i++){
                    auto s = ((QAudioBuffer::StereoFrame<qint32>*)auxBuffer)[i];
                    buffer[i] = QAudioBuffer::StereoFrame<qint16>(s.left/65536, s.right/65536);
                }*/
            } break;
            default:
                break;
            }
            break;
        }
        break; // end stereo
    default:
        break;
    }
}



