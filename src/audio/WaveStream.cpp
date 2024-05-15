#include "Wave.h"
#include "QOggVorbisAdapter.h"


WaveStreamSource::WaveStreamSource(const QString &srcPath, QObject *parent)
	: AudioStreamSource(parent)
	, file(srcPath)
	, din(&file)
{
}

WaveStreamSource::~WaveStreamSource()
{
}

int WaveStreamSource::Open()
{
	error = NoError;
	int safetyCounter = 0;
	if (!file.open(QFile::ReadOnly)){
		error = CannotOpenFile;
		return error;
	}
	din.setByteOrder(QDataStream::LittleEndian);
	if (file.read(4) != "RIFF"){
		error = NotWaveFile;
		return error;
	}
	din.skipRawData(4);
	if (file.read(4) != "WAVE"){
		error = NotWaveFile;
		return error;
	}

	quint16 formatTag;
	quint16 channelsCount;
	quint32 samplesPerSec;
	quint32 avgBytesPerSec;
	quint16 blockAlign;
	quint16 bitsPerSample;
	{	// read [fmt ] chunk
		if (file.atEnd()){
			error = FormatMissing;
			return error;
		}
		while (file.read(4) != "fmt "){
			if (++safetyCounter > 1000 || file.atEnd()){
				error = FormatMissing;
				return error;
			}
			quint32 ckSize = 0;
			din >> ckSize;
			din.skipRawData(ckSize);
		}
		quint32 ckSize = 0;
		din >> ckSize;
		if (ckSize < 16){
			error = FormatMissing;
			return error;
		}
		din >> formatTag >> channelsCount >> samplesPerSec >> avgBytesPerSec >> blockAlign >> bitsPerSample;
		din.skipRawData(ckSize - 16);
		if ((formatTag != 1 /*WAVE_FORMAT_PCM*/ && formatTag != 3 /*WAVE_FORMAT_IEEE_FLOAT*/) || channelsCount > 2){
			qDebug() << "formatTag: " << formatTag;
			qDebug() << "channels: " << channelsCount;
			error = UnsupportedFormat;
			return error;
		}

        if (channelsCount == 1)
            format.setChannelConfig(QAudioFormat::ChannelConfigMono);
        else if (channelsCount == 2)
            format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
        else
            format.setChannelConfig(QAudioFormat::ChannelConfigUnknown);
        format.setSampleRate(samplesPerSec);

        // format.setByteOrder(QAudioFormat::LittleEndian);
        // TODO?: Make sure byte endianness is handled properly.

		switch (bitsPerSample){
            case 8:
                // 8bit unsigned int (00 - 80 - FF)
                format.setSampleFormat(QAudioFormat::UInt8);
                if (blockAlign != channelsCount){
                    qDebug() << "8bit x " << channelsCount << "ch -> " << blockAlign;
                    error = UnsupportedFormat;
                    return error;
                }
                break;
            case 16:
                // 16bit signed int (8000 - 0000 - 7FFF )
                format.setSampleFormat(QAudioFormat::Int16);;
                if (blockAlign != 2*channelsCount){
                    qDebug() << "16bit x " << channelsCount << "ch -> " << blockAlign;
                    error = UnsupportedFormat;
                    return error;
                }
                break;
            case 24:
                // 24bit signed int (800000 - 000000 - 7FFFFF)
                format.setSampleFormat(QAudioFormat::NSampleFormats);
                if (blockAlign != 3*channelsCount){
                    qDebug() << "24bit x " << channelsCount << "ch -> " << blockAlign;
                    error = UnsupportedFormat;
                    return error;
                }
                break;
            case 32:
                // 32bit float
                format.setSampleFormat(QAudioFormat::Float);
                if (blockAlign != 4*channelsCount){
                    qDebug() << "32bit x " << channelsCount << "ch -> " << blockAlign;
                    error = UnsupportedFormat;
                    return error;
                }
                break;
            default:
                qDebug() << "bits: " << bitsPerSample;
                error = UnsupportedFormat;
                return error;
		}
	}
	{	// get ready to read [data] chunk
		if (file.atEnd()){
			error = DataMissing;
			return error;
		}
		while (file.read(4) != "data"){
			if (++safetyCounter > 1000 || file.atEnd()){
				error = DataMissing;
				return error;
			}
			quint32 ckSize = 0;
			din >> ckSize;
			din.skipRawData(ckSize);
		}

		quint32 ckSize = 0;
		din >> ckSize;
        if (blockAlign != 0){
            bytes = ckSize;
            frames = bytes / blockAlign;
        }
		if (frames == 0){
			error = DataMissing;
			return error;
		}
		if (bytes >= 0x40000000){ // >= 1G Bytes
			error = DataSizeOver;
			return error;
		}

		current = 0;
		dataOffset = file.pos();
	}
	return NoError;
}

quint64 WaveStreamSource::Read(char *buffer, quint64 bufferSize)
{
	int bpf = format.bytesPerFrame();
	int readSize = din.readRawData(buffer, std::min(bufferSize / bpf * bpf, (frames-current) * bpf));
	current += readSize / bpf;
	return readSize;
}

void WaveStreamSource::SeekRelative(qint64 relativeFrames)
{
	file.seek(file.pos() + relativeFrames * format.bytesPerFrame());
	current += relativeFrames;
}

void WaveStreamSource::SeekAbsolute(quint64 absoluteFrames)
{
	file.seek(dataOffset + absoluteFrames * format.bytesPerFrame());
	current = absoluteFrames;
}


OggStreamSource::OggStreamSource(const QString &srcPath, QObject *parent)
	: AudioStreamSource(parent)
	, srcPath(srcPath)
	, file(nullptr)
{
}


OggStreamSource::~OggStreamSource()
{
	if (file){
		ov_clear(file);
		delete file;
	}
}

int OggStreamSource::Open()
{
	error = NoError;
	file = new OggVorbis_File;
	int e = QOggVorbisAdapter::Open(QDir::toNativeSeparators(srcPath), file);
	if (e != 0){
		switch (e){
            case OV_EREAD:
                error = CannotOpenFile;
                break;
            case OV_ENOTVORBIS:
                error = NotVorbisFile;
                break;
            case OV_EVERSION:
                error = UnsupportedVersion;
                break;
            case OV_EBADHEADER:
                error = MalformedVorbis;
                break;
            case OV_EFAULT:
            default:
                error = VorbisUnknown;
		}
		delete file;
		file = nullptr;
		return error;
	}

    const vorbis_info *info = ov_info(file, -1);
    if (info->channels == 1)
        format.setChannelConfig(QAudioFormat::ChannelConfigMono);
    else if (info->channels == 2)
        format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    else
        format.setChannelConfig(QAudioFormat::ChannelConfigUnknown);
    format.setSampleFormat(QAudioFormat::Int16);
    format.setSampleRate(info->rate);


	// read data
    frames = ov_pcm_total(file, -1);
    bytes = frames * 2 * info->channels;
	if (bytes > 0x40000000){
		error = DataSizeOver;
		return error;
	}

	current = 0;
	return NoError;
}

quint64 OggStreamSource::Read(char *buffer, quint64 bufferSize)
{
	int bpf = format.bytesPerFrame();
	int bs;
	int readSize = ov_read(file, buffer, std::min(bufferSize / bpf * bpf, (frames-current) * bpf), 0, 2, 1, &bs);
	if (readSize < 0){
            switch (readSize){
            case OV_HOLE:
                qDebug() << "OV_HOLE";
                break;
            case OV_EBADLINK:
                qDebug() << "OV_EBADLINK";
                break;
            case OV_EINVAL:
                qDebug() << "OV_EINVAL";
                break;
            default:
                break;
		}
		return 0;
	}
	current += readSize / bpf;
	return readSize;
}

void OggStreamSource::SeekRelative(qint64 relativeFrames)
{
	ov_pcm_seek(file, current + relativeFrames);
	current += relativeFrames;
}

void OggStreamSource::SeekAbsolute(quint64 absoluteFrames)
{
	ov_pcm_seek(file, absoluteFrames);
	current = absoluteFrames;
}




S16S44100StreamTransformer::S16S44100StreamTransformer(AudioStreamSource *src, QObject *parent)
	: AudioStreamSource(parent)
	, src(src)
	, inputBuffer(new char[InputBufferSize])
{
	src->setParent(this);

    format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    format.setSampleFormat(QAudioFormat::Int16);
	format.setSampleRate(44100);

    // format.setByteOrder(QAudioFormat::LittleEndian);
    // TODO?: Make sure byte endianness is handled properly.

	bytes = src->GetTotalBytes() * src->GetFormat().bytesForFrames(src->GetFormat().sampleRate()) / format.bytesForFrames(format.sampleRate());
	frames = src->GetFrameCount() * src->GetFormat().sampleRate() / format.sampleRate();
	current = 0;
}

S16S44100StreamTransformer::~S16S44100StreamTransformer()
{
	delete[] inputBuffer;
}

bool S16S44100StreamTransformer::IsSourceS16S44100() const
{
	//return src->GetFormat() == format;
	QAudioFormat fmt = src->GetFormat();
    return fmt.channelConfig() == format.channelConfig() &&
           fmt.sampleFormat() == format.sampleFormat() &&
           fmt.sampleRate() == format.sampleRate();
}

int S16S44100StreamTransformer::Open()
{
	return src->Open();
}

quint64 S16S44100StreamTransformer::Read(char *buffer, quint64 bufferSize)
{
	quint64 framesRead = Read(reinterpret_cast<QAudioBuffer::S16S*>(buffer), bufferSize/sizeof(QAudioBuffer::S16S));
	return framesRead * sizeof(QAudioBuffer::S16S);
}

void S16S44100StreamTransformer::SeekRelative(qint64 relativeFrames)
{
	current += relativeFrames;
	src->SeekRelative(relativeFrames * src->GetFormat().sampleRate() / format.sampleRate());
	auxBuffer.clear();
}

void S16S44100StreamTransformer::SeekAbsolute(quint64 absoluteFrames)
{
	current = absoluteFrames;
	src->SeekAbsolute(absoluteFrames * src->GetFormat().sampleRate() / format.sampleRate());
	auxBuffer.clear();
}

static S16S44100StreamTransformer::SampleType Interpolate(S16S44100StreamTransformer::SampleType a, S16S44100StreamTransformer::SampleType b, qreal t)
{
    QAudioBuffer::S16S buf = QAudioBuffer::S16S();
    buf.setValue(QAudioFormat::FrontLeft, a.value(QAudioFormat::FrontLeft)*(1.0-t)+b.value(QAudioFormat::FrontLeft)*t);
    buf.setValue(QAudioFormat::FrontRight, a.value(QAudioFormat::FrontRight)*(1.0-t)+b.value(QAudioFormat::FrontRight)*t);
    return buf;
}

quint64 S16S44100StreamTransformer::Read(QAudioBuffer::S16S *buffer, quint64 frames)
{
	//QTime t0 = QTime::currentTime();
	if (IsSourceS16S44100()){
		quint64 framesRead = src->Read(reinterpret_cast<char*>(buffer), frames * sizeof(QAudioBuffer::S16S)) / sizeof(QAudioBuffer::S16S);
		current += framesRead;
		return framesRead;
	}
	const qreal samplingRatio = qreal(src->GetFormat().sampleRate()) / format.sampleRate();
	const qreal playHeadBegin = qreal(GetCurrentFrame()) * samplingRatio;
	if (playHeadBegin > qreal(src->GetFrameCount())){
		return 0;
	}
	const qreal playHeadEnd = qreal(GetCurrentFrame() + frames) * samplingRatio;
	Provide(playHeadEnd);
	quint64 sizeOutput = 0;
	for (quint64 i=0; i<frames; i++){
		// ketaochi shisou
		qreal playOffset = qreal(qint64(GetCurrentFrame() + i)) * samplingRatio - qreal(src->GetCurrentFrame() - auxBuffer.size());
		int p = playOffset;
		if (p >= 0 && p+1 < auxBuffer.size()){
			qreal t = playOffset - p;
			buffer[i] = Interpolate(auxBuffer[p], auxBuffer[p+1], t);
		}else{
			buffer[i].clear();
		}
		sizeOutput++;
	}
	Forget(playHeadEnd);
	current += sizeOutput;
	//qDebug() << QString("S16S44100StreamTransformer: output %1 samples in %2 ms.").arg(sizeOutput).arg(t0.msecsTo(QTime::currentTime()));
	return sizeOutput;
}

void S16S44100StreamTransformer::Provide(qreal playHeadEnd)
{
    QAudioFormat fmt = src->GetFormat();
    int srcSampleRate = fmt.sampleRate();
    frames = 0;

    const qreal samplingRatio = qreal(srcSampleRate) / format.sampleRate();
	while (playHeadEnd + 1.0*samplingRatio + 1.0 >= qreal(src->GetCurrentFrame())
		   && src->GetCurrentFrame() < src->GetFrameCount())
	{
		quint64 sizeRead = src->Read(inputBuffer, InputBufferSize);
        if (sizeRead == 0) break;
        switch (fmt.channelConfig()){
        case QAudioFormat::ChannelConfigMono:
            switch (fmt.bytesPerFrame()){
            // 8bit mono
            case 1:
                switch (fmt.sampleFormat()){
                // 8bit unsigned int mono
                case QAudioFormat::UInt8:{
                    const quint8 *be = (const quint8*)(inputBuffer + sizeRead);
                    for (const quint8 *b = (const quint8*)inputBuffer; b<be; ){
                        quint8 v = *b++;
                        short s = (short(v) - 128) * 256;
                        QAudioBuffer::S16S buf = QAudioBuffer::S16S();
                        buf.setValue(QAudioFormat::FrontLeft, s);
                        buf.setValue(QAudioFormat::FrontRight, s);
                        auxBuffer.append(buf);
                    }
                } break;
                // 8bit signed int mono
                case QAudioFormat::Unknown:{
                    const qint8 *be = (const qint8*)(inputBuffer + sizeRead);
                    for (const qint8 *b = (const qint8*)inputBuffer; b<be; ){
                        qint8 v = *b++;
                        short s = short(v) * 256;
                        QAudioBuffer::S16S buf = QAudioBuffer::S16S();
                        buf.setValue(QAudioFormat::FrontLeft, s);
                        buf.setValue(QAudioFormat::FrontRight, s);
                        auxBuffer.append(buf);
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
                    const quint16 *be = (const quint16*)(inputBuffer + sizeRead);
                    for (const quint16 *b = (const quint16*)inputBuffer; b<be; ){
                        quint16 v = *b++;
                        int s = int(v) - 32768;
                        QAudioBuffer::S16S buf = QAudioBuffer::S16S();
                        buf.setValue(QAudioFormat::FrontLeft, s);
                        buf.setValue(QAudioFormat::FrontRight, s);
                        auxBuffer.append(buf);
                    }
                } break;
                // 16bit signed int mono
                case QAudioFormat::Int16:{
                    const qint16 *be = (const qint16*)(inputBuffer + sizeRead);
                    for (const qint16 *b = (const qint16*)inputBuffer; b<be; ){
                        qint16 s = *b++;
                        QAudioBuffer::S16S buf = QAudioBuffer::S16S();
                        buf.setValue(QAudioFormat::FrontLeft, s);
                        buf.setValue(QAudioFormat::FrontRight, s);
                        auxBuffer.append(buf);
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
                    const quint8 *b = (const quint8*)inputBuffer;
                    int samples = sizeRead / 3;
                    for (int i=0; i<samples; i++){
                        qint16 s = (b[i*3+1] | (b[i*3+2] << 8)) - 32768;
                        QAudioBuffer::S16S buf = QAudioBuffer::S16S();
                        buf.setValue(QAudioFormat::FrontLeft, s);
                        buf.setValue(QAudioFormat::FrontRight, s);
                        auxBuffer.append(buf);
                    }
                    break;
                } break;
                // 24bit signed int mono
                case QAudioFormat::NSampleFormats:{
                    const quint8 *b = (const quint8*)inputBuffer;
                    int samples = sizeRead / 3;
                    for (int i=0; i<samples; i++){
                        qint16 s = (qint16)(b[i*3+1] | (b[i*3+2] << 8));
                        QAudioBuffer::S16S buf = QAudioBuffer::S16S();
                        buf.setValue(QAudioFormat::FrontLeft, s);
                        buf.setValue(QAudioFormat::FrontRight, s);
                        auxBuffer.append(buf);
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
                    const float *b = (const float*)inputBuffer;
                    int samples = sizeRead / 4;
                    for (int i=0; i<samples; i++){
                        qint16 s = b[i] * 32767.f;
                        QAudioBuffer::S16S buf = QAudioBuffer::S16S();
                        buf.setValue(QAudioFormat::FrontLeft, s);
                        buf.setValue(QAudioFormat::FrontRight, s);
                        auxBuffer.append(buf);
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
                    const quint8 *be = (const quint8*)(inputBuffer + sizeRead);
                    for (const quint8 *b = (const quint8*)inputBuffer; b<be; ){
                        quint8 l = *b++;
                        quint8 r = *b++;
                        short sl = (short(l) - 128) * 256;
                        short sr = (short(r) - 128) * 256;
                        QAudioBuffer::S16S buf = QAudioBuffer::S16S();
                        buf.setValue(QAudioFormat::FrontLeft, sl);
                        buf.setValue(QAudioFormat::FrontRight, sr);
                        auxBuffer.append(buf);
                    }
                } break;
                // 8bit signed int stereo
                case QAudioFormat::Unknown:{
                    const qint8 *be = (const qint8*)(inputBuffer + sizeRead);
                    for (const qint8 *b = (const qint8*)inputBuffer; b<be; ){
                        qint8 l = *b++;
                        qint8 r = *b++;
                        short sl = short(l) * 256;
                        short sr = short(r) * 256;
                        QAudioBuffer::S16S buf = QAudioBuffer::S16S();
                        buf.setValue(QAudioFormat::FrontLeft, sl);
                        buf.setValue(QAudioFormat::FrontRight, sr);
                        auxBuffer.append(buf);
                    }
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
                    const quint16 *be = (const quint16*)(inputBuffer + sizeRead);
                    for (const quint16 *b = (const quint16*)inputBuffer; b<be; ){
                        quint16 l = *b++;
                        quint16 r = *b++;
                        int sl = int(l) - 32768;
                        int sr = int(r) - 32768;
                        QAudioBuffer::S16S buf = QAudioBuffer::S16S();
                        buf.setValue(QAudioFormat::FrontLeft, sl);
                        buf.setValue(QAudioFormat::FrontRight, sr);
                        auxBuffer.append(buf);
                    }
                } break;
                // 16bit signed int stereo
                case QAudioFormat::Int16:{
                    const qint16 *be = (const qint16*)(inputBuffer + sizeRead);
                    for (const qint16 *b = (const qint16*)inputBuffer; b<be; ){
                        qint16 sl = *b++;
                        qint16 sr = *b++;
                        QAudioBuffer::S16S buf = QAudioBuffer::S16S();
                        buf.setValue(QAudioFormat::FrontLeft, sl);
                        buf.setValue(QAudioFormat::FrontRight, sr);
                        auxBuffer.append(buf);
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
                    const quint8 *b = (const quint8*)inputBuffer;
                    int samples = sizeRead / 6;
                    for (int i=0; i<samples; i++){
                        qint16 sl = (b[i*6+1] | (b[i*6+2] << 8)) - 32768;
                        qint16 sr = (b[i*6+4] | (b[i*6+5] << 8)) - 32768;
                        QAudioBuffer::S16S buf = QAudioBuffer::S16S();
                        buf.setValue(QAudioFormat::FrontLeft, sl);
                        buf.setValue(QAudioFormat::FrontRight, sr);
                        auxBuffer.append(buf);
                    }
                } break;
                // 24bit signed int stereo
                case QAudioFormat::NSampleFormats:{
                    const quint8 *b = (const quint8*)inputBuffer;
                    int samples = sizeRead / 6;
                    for (int i=0; i<samples; i++){
                        qint16 sl = (qint16)(b[i*6+1] | (b[i*6+2] << 8));
                        qint16 sr = (qint16)(b[i*6+4] | (b[i*6+5] << 8));
                        QAudioBuffer::S16S buf = QAudioBuffer::S16S();
                        buf.setValue(QAudioFormat::FrontLeft, sl);
                        buf.setValue(QAudioFormat::FrontRight, sr);
                        auxBuffer.append(buf);
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
                    const float *b = (const float*)inputBuffer;
                    int samples = sizeRead / 8;
                    for (int i=0; i<samples; i++){
                        qint16 sl = b[i*2] * 32767.f;
                        qint16 sr = b[i*2+1] * 32767.f;
                        QAudioBuffer::S16S buf = QAudioBuffer::S16S();
                        buf.setValue(QAudioFormat::FrontLeft, sl);
                        buf.setValue(QAudioFormat::FrontRight, sr);
                        auxBuffer.append(buf);
                    }
                } break;
                // 32bit unsigned int stereo
                // 32bit signed int stereo
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
}

void S16S44100StreamTransformer::Forget(qreal playHeadEnd)
{
	const qreal samplingRatio = qreal(src->GetFormat().sampleRate()) / format.sampleRate();
	int playHeadOffset = int(qint64(playHeadEnd - 1.0*samplingRatio - 1.0) - qint64(src->GetCurrentFrame() - auxBuffer.size()));
    int forgetOffset = std::max(0, std::min( (int) auxBuffer.size(), playHeadOffset));
	for (int i=0; i<forgetOffset; i++){
		auxBuffer.removeFirst();
	}
}




F32S44100StreamTransformer::F32S44100StreamTransformer(AudioStreamSource *src, QObject *parent)
	: AudioStreamSource(parent)
	, src(src)
	, inputBuffer(new char[InputBufferSize])
{
	src->setParent(this);
    format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    format.setSampleFormat(QAudioFormat::Float);
    format.setSampleRate(44100);

    // format.setByteOrder(QAudioFormat::LittleEndian);
    // TODO?: Make sure byte endianness is handled properly.

	bytes = src->GetTotalBytes() * src->GetFormat().bytesForFrames(src->GetFormat().sampleRate()) / format.bytesForFrames(format.sampleRate());
	frames = src->GetFrameCount() * src->GetFormat().sampleRate() / format.sampleRate();
	current = 0;
}

F32S44100StreamTransformer::~F32S44100StreamTransformer()
{
	delete[] inputBuffer;
}

bool F32S44100StreamTransformer::IsSourceF32S44100() const
{
	//return src->GetFormat() == format;
	QAudioFormat fmt = src->GetFormat();
    return fmt.channelConfig() == format.channelConfig() &&
           fmt.sampleFormat() == format.sampleFormat() &&
           fmt.sampleRate() == format.sampleRate();
}

int F32S44100StreamTransformer::Open()
{
	return src->Open();
}

quint64 F32S44100StreamTransformer::Read(char *buffer, quint64 bufferSize)
{
    quint64 framesRead = Read(reinterpret_cast<QAudioBuffer::F32S*>(buffer), bufferSize/sizeof(QAudioBuffer::F32S));
    return framesRead * sizeof(QAudioBuffer::F32S);
}

void F32S44100StreamTransformer::SeekRelative(qint64 relativeFrames)
{
	current += relativeFrames;
	src->SeekRelative(relativeFrames * src->GetFormat().sampleRate() / format.sampleRate());
	auxBuffer.clear();
}

void F32S44100StreamTransformer::SeekAbsolute(quint64 absoluteFrames)
{
	current = absoluteFrames;
	src->SeekAbsolute(absoluteFrames * src->GetFormat().sampleRate() / format.sampleRate());
	auxBuffer.clear();
}

static F32S44100StreamTransformer::SampleType Interpolate(F32S44100StreamTransformer::SampleType a, F32S44100StreamTransformer::SampleType b, qreal t)
{
    QAudioBuffer::F32S buf = QAudioBuffer::F32S();
    buf.setValue(QAudioFormat::FrontLeft, a.value(QAudioFormat::FrontLeft)*(1.0-t)+b.value(QAudioFormat::FrontLeft)*t);
    buf.setValue(QAudioFormat::FrontRight, a.value(QAudioFormat::FrontRight)*(1.0-t)+b.value(QAudioFormat::FrontRight)*t);
    return buf;
}

quint64 F32S44100StreamTransformer::Read(QAudioBuffer::F32S *buffer, quint64 frames)
{
	//QTime t0 = QTime::currentTime();
    if (IsSourceF32S44100()){
        quint64 framesRead = src->Read(reinterpret_cast<char*>(buffer), frames * sizeof(QAudioBuffer::F32S)) / sizeof(QAudioBuffer::F32S);
		current += framesRead;
		return framesRead;
	}
	const qreal samplingRatio = qreal(src->GetFormat().sampleRate()) / format.sampleRate();
	const qreal playHeadBegin = qreal(GetCurrentFrame()) * samplingRatio;
	if (playHeadBegin > qreal(src->GetFrameCount())){
		return 0;
	}
	const qreal playHeadEnd = qreal(GetCurrentFrame() + frames) * samplingRatio;
	Provide(playHeadEnd);
	quint64 sizeOutput = 0;
	for (quint64 i=0; i<frames; i++){
		// ketaochi shisou
		qreal playOffset = qreal(qint64(GetCurrentFrame() + i)) * samplingRatio - qreal(src->GetCurrentFrame() - auxBuffer.size());
		int p = playOffset;
		if (p >= 0 && p+1 < auxBuffer.size()){
			qreal t = playOffset - p;
			buffer[i] = Interpolate(auxBuffer[p], auxBuffer[p+1], t);
		}else{
			buffer[i].clear();
		}
		sizeOutput++;
	}
	Forget(playHeadEnd);
	current += sizeOutput;
	//qDebug() << QString("S16S44100StreamTransformer: output %1 samples in %2 ms.").arg(sizeOutput).arg(t0.msecsTo(QTime::currentTime()));
	return sizeOutput;
}

void F32S44100StreamTransformer::Provide(qreal playHeadEnd)
{
    QAudioFormat fmt = src->GetFormat();
    int srcSampleRate = fmt.sampleRate();
    frames = 0;

    const qreal samplingRatio = qreal(srcSampleRate) / format.sampleRate();
	while (playHeadEnd + 1.0*samplingRatio + 1.0 >= qreal(src->GetCurrentFrame())
		   && src->GetCurrentFrame() < src->GetFrameCount())
	{
		quint64 sizeRead = src->Read(inputBuffer, InputBufferSize);
        if (sizeRead == 0) break;
        switch (fmt.channelConfig()){
        case QAudioFormat::ChannelConfigMono:
            switch (fmt.bytesPerFrame()){
            // 8bit mono
            case 1:
                switch (fmt.sampleFormat()){
                // 8bit unsigned int mono
                case QAudioFormat::UInt8:{
                    const quint8 *be = (const quint8*)(inputBuffer + sizeRead);
                    for (const quint8 *b = (const quint8*)inputBuffer; b<be; ){
                        quint8 v = *b++;
                        float s = (short(v) - 128) / 128.f;
                        QAudioBuffer::F32S buf = QAudioBuffer::F32S();
                        buf.setValue(QAudioFormat::FrontLeft, s);
                        buf.setValue(QAudioFormat::FrontRight, s);
                        auxBuffer.append(buf);
                    }
                } break;
                // 8bit signed int mono
                case QAudioFormat::Unknown:{
                    const qint8 *be = (const qint8*)(inputBuffer + sizeRead);
                    for (const qint8 *b = (const qint8*)inputBuffer; b<be; ){
                        qint8 v = *b++;
                        float s = float(v) / 128.f;
                        QAudioBuffer::F32S buf = QAudioBuffer::F32S();
                        buf.setValue(QAudioFormat::FrontLeft, s);
                        buf.setValue(QAudioFormat::FrontRight, s);
                        auxBuffer.append(buf);
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
                    const quint16 *be = (const quint16*)(inputBuffer + sizeRead);
                    for (const quint16 *b = (const quint16*)inputBuffer; b<be; ){
                        quint16 v = *b++;
                        float s = (int(v) - 32768) / 32768.f;
                        QAudioBuffer::F32S buf = QAudioBuffer::F32S();
                        buf.setValue(QAudioFormat::FrontLeft, s);
                        buf.setValue(QAudioFormat::FrontRight, s);
                        auxBuffer.append(buf);
                    }
                } break;
                // 16bit signed int mono
                case QAudioFormat::Int16:{
                    const qint16 *be = (const qint16*)(inputBuffer + sizeRead);
                    for (const qint16 *b = (const qint16*)inputBuffer; b<be; ){
                        float s = (*b++) / 32768.f;
                        QAudioBuffer::F32S buf = QAudioBuffer::F32S();
                        buf.setValue(QAudioFormat::FrontLeft, s);
                        buf.setValue(QAudioFormat::FrontRight, s);
                        auxBuffer.append(buf);
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
                    const quint8 *b = (const quint8*)inputBuffer;
                    int samples = sizeRead / 3;
                    for (int i=0; i<samples; i++){
                        float s = ((b[i*3+1] | (b[i*3+2] << 8 )) - 32768) / 32768.f;
                        QAudioBuffer::F32S buf = QAudioBuffer::F32S();
                        buf.setValue(QAudioFormat::FrontLeft, s);
                        buf.setValue(QAudioFormat::FrontRight, s);
                        auxBuffer.append(buf);
                    }
                } break;
                // 24bit signed int mono
                case QAudioFormat::NSampleFormats:{
                    const quint8 *b = (const quint8*)inputBuffer;
                    int samples = sizeRead / 3;
                    for (int i=0; i<samples; i++){
                        float s = (qint16)(b[i*3+1] | (b[i*3+2] << 8 )) / 32768.f;
                        QAudioBuffer::F32S buf = QAudioBuffer::F32S();
                        buf.setValue(QAudioFormat::FrontLeft, s);
                        buf.setValue(QAudioFormat::FrontRight, s);
                        auxBuffer.append(buf);
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
                    const float *b = (const float*)inputBuffer;
                    int samples = sizeRead / 4;
                    for (int i=0; i<samples; i++){
                        float s = b[i];
                        QAudioBuffer::F32S buf = QAudioBuffer::F32S();
                        buf.setValue(QAudioFormat::FrontLeft, s);
                        buf.setValue(QAudioFormat::FrontRight, s);
                        auxBuffer.append(buf);
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
                    const quint8 *be = (const quint8*)(inputBuffer + sizeRead);
                    for (const quint8 *b = (const quint8*)inputBuffer; b<be; ){
                        quint8 l = *b++;
                        quint8 r = *b++;
                        float sl = (short(l) - 128) / 128.f;
                        float sr = (short(r) - 128) / 128.f;
                        QAudioBuffer::F32S buf = QAudioBuffer::F32S();
                        buf.setValue(QAudioFormat::FrontLeft, sl);
                        buf.setValue(QAudioFormat::FrontRight, sr);
                        auxBuffer.append(buf);
                    }
                } break;
                // 8bit signed int stereo
                case QAudioFormat::Unknown:{
                    const qint8 *be = (const qint8*)(inputBuffer + sizeRead);
                    for (const qint8 *b = (const qint8*)inputBuffer; b<be; ){
                        qint8 l = *b++;
                        qint8 r = *b++;
                        float sl = float(l) * 128.f;
                        float sr = float(r) * 128.f;
                        QAudioBuffer::F32S buf = QAudioBuffer::F32S();
                        buf.setValue(QAudioFormat::FrontLeft, sl);
                        buf.setValue(QAudioFormat::FrontRight, sr);
                        auxBuffer.append(buf);
                    }
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
                    const quint16 *be = (const quint16*)(inputBuffer + sizeRead);
                    for (const quint16 *b = (const quint16*)inputBuffer; b<be; ){
                        quint16 l = *b++;
                        quint16 r = *b++;
                        float sl = (int(l) - 32768) / 32768.f;
                        float sr = (int(r) - 32768) / 32768.f;
                        QAudioBuffer::F32S buf = QAudioBuffer::F32S();
                        buf.setValue(QAudioFormat::FrontLeft, sl);
                        buf.setValue(QAudioFormat::FrontRight, sr);
                        auxBuffer.append(buf);
                    }
                } break;
                // 16bit signed int stereo
                case QAudioFormat::Int16:{
                    const qint16 *be = (const qint16*)(inputBuffer + sizeRead);
                    for (const qint16 *b = (const qint16*)inputBuffer; b<be; ){
                        float sl = (*b++) / 32768.f;
                        float sr = (*b++) / 32768.f;
                        QAudioBuffer::F32S buf = QAudioBuffer::F32S();
                        buf.setValue(QAudioFormat::FrontLeft, sl);
                        buf.setValue(QAudioFormat::FrontRight, sr);
                        auxBuffer.append(buf);
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
                    const quint8 *b = (const quint8*)inputBuffer;
                    int samples = sizeRead / 6;
                    for (int i=0; i<samples; i++){
                        float sl = ((b[i*6+1] | (b[i*6+2] << 8 )) - 32768) / 32768.f;
                        float sr = ((b[i*6+4] | (b[i*6+5] << 8 )) - 32768) / 32768.f;
                        QAudioBuffer::F32S buf = QAudioBuffer::F32S();
                        buf.setValue(QAudioFormat::FrontLeft, sl);
                        buf.setValue(QAudioFormat::FrontRight, sr);
                        auxBuffer.append(buf);
                    }
                } break;
                // 24bit signed int stereo
                case QAudioFormat::NSampleFormats:{
                    const quint8 *b = (const quint8*)inputBuffer;
                    int samples = sizeRead / 6;
                    for (int i=0; i<samples; i++){
                        float sl = (qint16)(b[i*6+1] | (b[i*6+2] << 8 )) / 32768.f;
                        float sr = (qint16)(b[i*6+4] | (b[i*6+5] << 8 )) / 32768.f;
                        QAudioBuffer::F32S buf = QAudioBuffer::F32S();
                        buf.setValue(QAudioFormat::FrontLeft, sl);
                        buf.setValue(QAudioFormat::FrontRight, sr);
                        auxBuffer.append(buf);
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
                    const float *b = (const float*)inputBuffer;
                    int samples = sizeRead / 8;
                    for (int i=0; i<samples; i++){
                        float sl = b[i*2];
                        float sr = b[i*2+1];
                        QAudioBuffer::F32S buf = QAudioBuffer::F32S();
                        buf.setValue(QAudioFormat::FrontLeft, sl);
                        buf.setValue(QAudioFormat::FrontRight, sr);
                        auxBuffer.append(buf);
                    }
                } break;
                // 32bit unsigned int stereo
                // 32bit signed int stereo
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
}

void F32S44100StreamTransformer::Forget(qreal playHeadEnd)
{
	const qreal samplingRatio = qreal(src->GetFormat().sampleRate()) / format.sampleRate();
	int playHeadOffset = int(qint64(playHeadEnd - 1.0*samplingRatio - 1.0) - qint64(src->GetCurrentFrame() - auxBuffer.size()));
    int forgetOffset = std::max(0, std::min( (int) auxBuffer.size(), playHeadOffset));
	for (int i=0; i<forgetOffset; i++){
		auxBuffer.removeFirst();
	}
}




void AudioStreamSource::EnumerateAllAsFloat(std::function<void(float)> whenMonoral, std::function<void(float, float)> whenStereo)
{
	static const quint64 BufferSize = 4096;
	char *buffer = new char[BufferSize];

    switch (format.channelConfig()){
    case QAudioFormat::ChannelConfigMono:
        switch (format.bytesPerFrame()){
        // 8bit mono
        case 1:
            switch (format.sampleFormat()){
            // 8bit unsigned int mono
            case QAudioFormat::UInt8:
                while (GetRemainingFrameCount() > 0){
                    const quint64 sizeRead = Read(buffer, BufferSize);
                    if (sizeRead == 0)
                        break;
                    auto be = (const quint8 *)(buffer + sizeRead);
                    for (auto b = (const quint8 *)buffer; b<be; ){
                        quint8 v = *b++;
                        float vv = (float)((int)v - 128) / 128.0f;
                        whenMonoral(vv);
                    }
                }
                break;
            // 8bit signed int mono
            case QAudioFormat::Unknown:
                while (GetRemainingFrameCount() > 0){
                    const quint64 sizeRead = Read(buffer, BufferSize);
                    if (sizeRead == 0)
                        break;
                    auto be = (const qint8 *)(buffer + sizeRead);
                    for (auto b = (const qint8 *)buffer; b<be; ){
                        qint8 v = *b++;
                        float vv = (float)v / 128.0f;
                        whenMonoral(vv);
                    }
                }
                break;
            default:
                break;
            }
            break;
        // 16bit mono
        case 2:
            switch (format.sampleFormat()){
            // 16bit unsigned int mono
            case QAudioFormat::Unknown:
                while (GetRemainingFrameCount() > 0){
                    const quint64 sizeRead = Read(buffer, BufferSize);
                    if (sizeRead == 0)
                        break;
                    auto be = (const quint16*)(buffer + sizeRead);
                    for (auto b = (const quint16 *)buffer; b<be; ){
                        quint16 v = *b++;
                        whenMonoral(float((int)v-32768) / 32768.0f);
                    }
                }
                break;
            // 16bit signed int mono
            case QAudioFormat::Int16:
                while (GetRemainingFrameCount() > 0){
                    const quint64 sizeRead = Read(buffer, BufferSize);
                    if (sizeRead == 0)
                        break;
                    auto be = (const qint16*)(buffer + sizeRead);
                    for (auto b = (const qint16*)buffer; b<be; ){
                        qint16 v = *b++;
                        whenMonoral((float)v / 32768.0f);
                    }
                }
                break;
            default:
                break;
            }
            break;
        // 24bit mono
        case 3:
            switch (format.sampleFormat()){
            // 24bit unsigned int mono
            case QAudioFormat::Unknown:
                while (GetRemainingFrameCount() > 0){
                    const quint64 sizeRead = Read(buffer, BufferSize);
                    if (sizeRead == 0)
                        break;
                    auto be = (const quint8*)(buffer + sizeRead);
                    for (auto b = (const quint8 *)buffer; b<be; ){
                        b++;
                        int v = *((const quint16* &)b)++;
                        whenMonoral(float(v-32768) / 32768.0f);
                    }
                }
                break;
            // 24bit signed int mono
            case QAudioFormat::NSampleFormats:
                while (GetRemainingFrameCount() > 0){
                    const quint64 sizeRead = Read(buffer, BufferSize);
                    if (sizeRead == 0)
                        break;
                    auto be = (const qint8*)(buffer + sizeRead);
                    for (auto b = (const qint8*)buffer; b<be; ){
                        b++;
                        qint16 v = *((const qint16* &)b)++;
                        whenMonoral((float)v / 32768.0f);
                    }
                }
                break;
            default:
                break;
            }
            break;
        // 32bit mono
        case 4:
            switch (format.sampleFormat()){
            // 32bit float mono
            case QAudioFormat::Float:
                while (GetRemainingFrameCount() > 0){
                    const quint64 sizeRead = Read(buffer, BufferSize);
                    if (sizeRead == 0)
                        break;
                    auto be = (const float*)(buffer + sizeRead);
                    for (auto b = (const float *)buffer; b<be; ){
                        float v = *b++;
                        whenMonoral(v);
                    }
                }
                break;
            // 32bit unsigned int
            /*case QAudioFormat::UnSignedInt:
                            while (GetRemainingFrameCount() > 0){
                                const quint64 sizeRead = Read(buffer, BufferSize);
                                if (sizeRead == 0)
                                    break;
                                auto be = (const quint32*)(buffer + sizeRead);
                                for (auto b = (const quint32 *)buffer; b<be; ){
                                    quint32 v = *b++;
                                    whenMonoral(float(qint32(v-0x80000000)) / 0x7FFFFFFF);
                                }
                            }
                            break;*/
            // 32 bit signed int
            /*case QAudioFormat::SignedInt:
                            while (GetRemainingFrameCount() > 0){
                                const quint64 sizeRead = Read(buffer, BufferSize);
                                if (sizeRead == 0)
                                    break;
                                auto be = (const qint32*)(buffer + sizeRead);
                                for (auto b = (const qint32*)buffer; b<be; ){
                                    qint32 v = *b++;
                                    whenMonoral((float)v / 0x7FFFFFFF);
                                }
                            }
                            break;*/
            default:
                break;
            }
            break;
        }
        break;
    case QAudioFormat::ChannelConfigStereo:
        switch (format.bytesPerFrame()){
        // 8bit stereo
        case 2:
            switch (format.sampleFormat()){
            // 8bit unsigned int stereo
            case QAudioFormat::UInt8:
                while (GetRemainingFrameCount() > 0){
                    const quint64 sizeRead = Read(buffer, BufferSize);
                    if (sizeRead == 0)
                        break;
                    auto be = (const quint8 *)(buffer + sizeRead);
                    for (auto b = (const quint8 *)buffer; b<be; ){
                        quint8 l = *b++;
                        quint8 r = *b++;
                        whenStereo(float((int)l - 128) / 128.0f, float((int)r-128) / 128.0f);
                    }
                }
                break;
            // 8bit signed int stereo
            case QAudioFormat::Unknown:
                while (GetRemainingFrameCount() > 0){
                    const quint64 sizeRead = Read(buffer, BufferSize);
                    if (sizeRead == 0)
                        break;
                    auto be = (const qint8 *)(buffer + sizeRead);
                    for (auto b = (const qint8 *)buffer; b<be; ){
                        qint8 l = *b++;
                        qint8 r = *b++;
                        whenStereo((float)l / 128.0f, (float)r / 128.0f);
                    }
                }
                break;
            default:
                break;
            }
            break;
        // 16bit stereo
        case 4:
            switch (format.sampleFormat()){
            // 16bit unsigned int stereo
            case QAudioFormat::Unknown:
                while (GetRemainingFrameCount() > 0){
                    const quint64 sizeRead = Read(buffer, BufferSize);
                    if (sizeRead == 0)
                        break;
                    auto be = (const quint16*)(buffer + sizeRead);
                    for (auto b = (const quint16 *)buffer; b<be; ){
                        quint16 l = *b++;
                        quint16 r = *b++;
                        whenStereo(float((int)l-32768) / 32768.0f, float((int)r-32768) / 32768.0f);
                    }
                }
                break;
            // 16bit signed int stereo
            case QAudioFormat::Int16:
                while (GetRemainingFrameCount() > 0){
                    const quint64 sizeRead = Read(buffer, BufferSize);
                    if (sizeRead == 0)
                        break;
                    auto be = (const qint16*)(buffer + sizeRead);
                    for (auto b = (const qint16*)buffer; b<be; ){
                        qint16 l = *b++;
                        qint16 r = *b++;
                        whenStereo((float)l / 32768.0f, (float)r / 32768.0f);
                    }
                }
                break;
            default:
                break;
            }
            break;
        // 24bit stereo
        case 6:
            switch (format.sampleFormat()){
            // 24bit unsigned int stereo
            /*case QAudioFormat::NSampleFormats:
                            while (GetRemainingFrameCount() > 0){
                                const quint64 sizeRead = Read(buffer, BufferSize);
                                if (sizeRead == 0)
                                    break;
                                auto be = (const quint8*)(buffer + sizeRead);
                                for (auto b = (const quint8 *)buffer; b<be; ){
                                    b++;
                                    int ltm1 = *((const quint16* &)b)++;
                                    b++;
                                    int rtm1 = *((const quint16* &)b)++;
                                    whenStereo(float(ltm1-32768) / 32768.0f, float(rtm1-32768) / 32768.0f);
                                }
                            }
                            break;*/
            // 24bit signed int stereo
            case QAudioFormat::NSampleFormats:
                while (GetRemainingFrameCount() > 0){
                    const quint64 sizeRead = Read(buffer, BufferSize);
                    if (sizeRead == 0)
                        break;
                    auto be = (const qint8*)(buffer + sizeRead);
                    for (auto b = (const qint8*)buffer; b<be; ){
                        b++;
                        qint16 ltm1 = *((const qint16* &)b)++;
                        b++;
                        qint16 rtm1 = *((const qint16* &)b)++;
                        //b++;
                        //qint16 ltm2 = *((const qint16* &)b)++;
                        //b++;
                        //qint16 rtm2 = *((const qint16* &)b)++;
                        whenStereo((float)ltm1 / 32768.0f, (float)rtm1 / 32768.0f);
                        //whenStereo((float)ltm2 / 32768.0f, (float)rtm2 / 32768.0f);
                        //lout.write(QString("%1\n").arg(ltm1).toLocal8Bit());
                    }
                }
                break;
            default:
                break;
            }
            break;
        // 32bit stereo
        case 8:
            switch (format.sampleFormat()){
            // 32bit float stereo
            case QAudioFormat::Float:
                while (GetRemainingFrameCount() > 0){
                    const quint64 sizeRead = Read(buffer, BufferSize);
                    if (sizeRead == 0)
                        break;
                    auto be = (const float*)(buffer + sizeRead);
                    for (auto b = (const float *)buffer; b<be; ){
                        float l = *b++;
                        float r = *b++;
                        whenStereo(l, r);
                    }
                }
                break;
            // 32bit unsigned int stereo
            /*case QAudioFormat::UnSignedInt:
                            while (GetRemainingFrameCount() > 0){
                                const quint64 sizeRead = Read(buffer, BufferSize);
                                if (sizeRead == 0)
                                    break;
                                auto be = (const quint32*)(buffer + sizeRead);
                                for (auto b = (const quint32 *)buffer; b<be; ){
                                    quint32 l = *b++;
                                    quint32 r = *b++;
                                    whenStereo(float(qint32(l-0x80000000)) / 0x7FFFFFFF, float(qint32(r-0x80000000)) / 0x7FFFFFFF);
                                }
                            }
                            break;*/
            // 32bit signed int stereo
            /*case QAudioFormat::SignedInt:
                            while (GetRemainingFrameCount() > 0){
                                const quint64 sizeRead = Read(buffer, BufferSize);
                                if (sizeRead == 0)
                                    break;
                                auto be = (const qint32*)(buffer + sizeRead);
                                for (auto b = (const qint32*)buffer; b<be; ){
                                    qint32 l = *b++;
                                    qint32 r = *b++;
                                    whenStereo((float)l / 0x7FFFFFFF, (float)r / 0x7FFFFFFF);
                                }
                            }
                            break;*/
            default:
                break;
            }
        }
        break;
    default:
        break;
	}
	delete[] buffer;
}
