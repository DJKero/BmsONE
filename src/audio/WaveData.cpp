#include "Wave.h"
#include "QOggVorbisAdapter.h"
#include <cstring>


WaveData::WaveData()
	: data(nullptr)
{
	err = NoError;
    fmt = UnsupportedFormat;
    format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    format.setSampleFormat(QAudioFormat::Int16);
    format.setSampleRate(44100);
	bytes = 0;
    frames = 0;
}

WaveData::WaveData(const QString &srcPath)
	: data(nullptr)
{
	err = NoError;
    fmt = UnsupportedFormat;
	QFileInfo fi(srcPath);
	QString ext = fi.suffix().toLower();
	if (ext == "wav"){
        LoadWav(srcPath);
	}else if (ext == "ogg"){
        LoadOgg(srcPath);
	}else{
		err = UnknownFileType;
		return;
	}
}

void WaveData::LoadWav(const QString &srcPath)
{
	int safetyCounter = 0;
	QFile file(srcPath);
	if (!file.open(QFile::ReadOnly)){
		err = CannotOpenFile;
		return;
	}
	QDataStream din(&file);
	din.setByteOrder(QDataStream::LittleEndian);
	if (file.read(4) != "RIFF"){
		err = NotWaveFile;
		return;
	}
	din.skipRawData(4);
	if (file.read(4) != "WAVE"){
		err = NotWaveFile;
		return;
	}
	quint16 formatTag;
	quint16 channelsCount;
	quint32 samplesPerSec;
	quint32 avgBytesPerSec;
	quint16 blockAlign;
	quint16 bitsPerSample;
	{	// read [fmt ] chunk
		if (file.atEnd()){
			err = FormatMissing;
			return;
		}
		while (file.read(4) != "fmt "){
			if (++safetyCounter > 1000 || file.atEnd()){
				err = FormatMissing;
				return;
			}
			quint32 ckSize = 0;
			din >> ckSize;
			din.skipRawData(ckSize);
		}
		quint32 ckSize = 0;
		din >> ckSize;
		if (ckSize < 16){
			err = FormatMissing;
			return;
		}
		din >> formatTag >> channelsCount >> samplesPerSec >> avgBytesPerSec >> blockAlign >> bitsPerSample;
		din.skipRawData(ckSize - 16);
		if (formatTag != 1 /*WAVE_FORMAT_PCM*/ || channelsCount > 2){
			qDebug() << "formatTag: " << formatTag;
			qDebug() << "channels: " << channelsCount;
			err = UnsupportedFormat;
			return;
        }

        if (channelsCount == 1)
            format.setChannelConfig(QAudioFormat::ChannelConfigMono);
        else if (channelsCount == 2)
            format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
        else
            format.setChannelConfig(QAudioFormat::ChannelConfigUnknown);
        format.setSampleRate(samplesPerSec);

		switch (bitsPerSample){
            case 8:
                // 8bit unsigned int (00 - 80 - FF)
                format.setSampleFormat(QAudioFormat::UInt8);
                if (blockAlign != channelsCount){
                    qDebug() << "8bit x " << channelsCount << "ch -> " << blockAlign;
                    err = UnsupportedFormat;
                    return;
                }
                break;
            case 16:
                // 16bit signed int (8000 - 0000 - 7FFF )
                format.setSampleFormat(QAudioFormat::Int16);;
                if (blockAlign != 2*channelsCount){
                    qDebug() << "16bit x " << channelsCount << "ch -> " << blockAlign;
                    err = UnsupportedFormat;
                    return;
                }
                break;
            case 24:
                // 24bit signed int (800000 - 000000 - 7FFFFF)
                format.setSampleFormat(QAudioFormat::NSampleFormats);
                if (blockAlign != 3*channelsCount){
                    qDebug() << "24bit x " << channelsCount << "ch -> " << blockAlign;
                    err = UnsupportedFormat;
                    return;
                }
                break;
            case 32:
                // 32bit float
                format.setSampleFormat(QAudioFormat::Float);
                if (blockAlign != 4*channelsCount){
                    qDebug() << "32bit x " << channelsCount << "ch -> " << blockAlign;
                    err = UnsupportedFormat;
                    return;
                }
                break;
            default:
                qDebug() << "bits: " << bitsPerSample;
                err = UnsupportedFormat;
                return;
		}
	}
	{	// read [data] chunk
		if (file.atEnd()){
			err = DataMissing;
			return;
		}
		while (file.read(4) != "data"){
			if (++safetyCounter > 1000 || file.atEnd()){
				err = DataMissing;
				return;
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
        else{
            err = Unknown;
            return;
        }
		if (frames == 0){
			err = DataMissing;
			return;
		}
		if (bytes >= 0x40000000){ // >= 1G Bytes
			err = DataSizeOver;
			return;
		}

        fmt = WAV;
		data = new quint8[bytes];
		char *d = (char*)data;
		int remainingSize = bytes;
		static const int unitSize = 0x00100000;
		while (remainingSize > unitSize){
			if (din.readRawData(d, unitSize) != unitSize){
				err = DataMissing;
				return;
			}
			remainingSize -= unitSize;
			d += unitSize;
		}
		if (din.readRawData(d, remainingSize) != remainingSize){
			err = DataMissing;
			return;
		}
	}
}

void WaveData::LoadOgg(const QString &srcPath)
{
	OggVorbis_File file;
	int e = QOggVorbisAdapter::Open(QDir::toNativeSeparators(srcPath), &file);
	if (e != 0){
		switch (e){
		case OV_EREAD:
			err = CannotOpenFile;
			break;
		case OV_ENOTVORBIS:
			err = NotVorbisFile;
			break;
		case OV_EVERSION:
			err = UnsupportedVersion;
			break;
		case OV_EBADHEADER:
			err = MalformedVorbis;
			break;
		case OV_EFAULT:
		default:
			err = VorbisUnknown;
		}
		return;
	}

    const vorbis_info *info = ov_info(&file, -1);
    if (info->channels == 1)
        format.setChannelConfig(QAudioFormat::ChannelConfigMono);
    else if (info->channels == 2)
        format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    format.setSampleFormat(QAudioFormat::Int16);
    format.setSampleRate(info->rate);

    // format.setByteOrder(QAudioFormat::LittleEndian);
    // TODO?: Make sure byte endianness is handled properly.

	// read data
	frames = ov_pcm_total(&file, -1);
    bytes = frames * 2 * info->channels;
	if (bytes > 0x40000000){
		err = DataSizeOver;
		ov_clear(&file);
		return;
	}
	static const int bufferSize = 4096;

    fmt = OGG;
	data = new char[bytes + bufferSize];
	char *d = (char*)data;
	int bitstream;
	int remainingSize = bytes;
	while (remainingSize > bufferSize){
		int sizeRead = ov_read(&file, d, bufferSize, 0, 2, 1, &bitstream);
		if (sizeRead < 0){
			switch (sizeRead){
			case OV_HOLE:
				qDebug() << "************************ OV_HOLE";
				continue;
			case OV_EBADLINK:
				qDebug() << "************************ OV_EBADLINK";
				continue;
			case OV_EINVAL:
				qDebug() << "************************ OV_EINVAL";
				continue;
			default:
				continue;
			}
		}
		if (sizeRead == 0){
			break;
		}
		d += sizeRead;
		remainingSize -= sizeRead;
	}
	if (remainingSize > 0){
		char temp[bufferSize];
		while (remainingSize > 0){
			int sizeRead = ov_read(&file, temp, bufferSize, 0, 2, 1, &bitstream);
			if (sizeRead < 0){
				switch (sizeRead){
				case OV_HOLE:
					qDebug() << "************************ OV_HOLE ******************************";
					continue;
				case OV_EBADLINK:
					qDebug() << "************************ OV_EBADLINK **************************";
					continue;
				case OV_EINVAL:
					qDebug() << "************************ OV_EINVAL ****************************";
					continue;
				default:
					continue;
				}
			}
			if (sizeRead == 0){
				break;
			}
			int size = std::min(remainingSize, sizeRead);
			std::memcpy(d, temp, size);
			d += size;
			remainingSize -= size;
		}
	}
	ov_clear(&file);
}

WaveData::~WaveData()
{
    if (data != nullptr){
        switch (fmt) {
            case WAV:
                delete[] static_cast<quint8*>(data);
                break;
            case OGG:
                delete[] static_cast<char*>(data);
                break;
        }
	}
}

void WaveData::Save([[maybe_unused]] const QString &dstPath)
{
	err = Unknown;
}



StandardWaveData::StandardWaveData()
	: data(nullptr)
{
	samplingRate = 44100;
	frames = 0;
}


StandardWaveData::StandardWaveData(WaveData *src)
    : data(nullptr)
{
    QAudioFormat fmt = src->GetFormat();
    samplingRate = fmt.sampleRate();
    frames = 0;

	const void *s = src->GetRawData();
	if (fmt.channelCount() <= 0 || fmt.channelCount() > 2){
		return;
	}

    frames = src->GetFrameCount();
    data = new SampleType[frames];

    switch (fmt.channelConfig()){
    case QAudioFormat::ChannelConfigMono:
        switch (fmt.bytesPerSample()){
        case 1:
            switch (fmt.sampleFormat()){
            case QAudioFormat::UInt8:
                for (int i=0; i<frames; i++){
                    quint8 v = ((const quint8*)s)[i];
                    data[i].setValue(QAudioFormat::FrontLeft, (v-128)*256);
                    data[i].setValue(QAudioFormat::FrontRight, (v-128)*256);
                }
                break;
            case QAudioFormat::Unknown:
                for (int i=0; i<frames; i++){
                    qint8 v = ((const qint8*)s)[i];
                    data[i].setValue(QAudioFormat::FrontLeft, v*256);
                    data[i].setValue(QAudioFormat::FrontRight, v*256);
                }
                break;
            default:
                break;
            }
            break;
        case 2:
            switch (fmt.sampleFormat()){
            case QAudioFormat::Unknown:
                for (int i=0; i<frames; i++){
                    quint16 v = ((const quint16*)s)[i];
                    data[i].setValue(QAudioFormat::FrontLeft, v-32768);
                    data[i].setValue(QAudioFormat::FrontRight, v-32768);
                }
                break;
            case QAudioFormat::Int16:
                for (int i=0; i<frames; i++){
                    qint16 v = ((const qint16*)s)[i];
                    data[i].setValue(QAudioFormat::FrontLeft, v);
                    data[i].setValue(QAudioFormat::FrontRight, v);
                }
                break;
            default:
                break;
            }
            break;
        case 3:
            switch (fmt.sampleFormat()){
            case QAudioFormat::NSampleFormats:
                for (int i=0; i<frames; i++){
                    qint32 vl = ((const quint8*)s)[i*3+1];
                    qint32 vh = ((const qint8*)s)[i*3+2];
                    data[i].setValue(QAudioFormat::FrontLeft, qint16(vl | (vh<<8)));
                    data[i].setValue(QAudioFormat::FrontRight, qint16(vl | (vh<<8)));
                }
                break;
            default:
                break;
            }
            break;
        case 4:
            switch (fmt.sampleFormat()){
            case QAudioFormat::Float:
                for (int i=0; i<frames; i++){
                    float v = ((const float*)s)[i];
                    data[i].setValue(QAudioFormat::FrontLeft, std::max<qint16>(-32768, std::min<qint16>(32767, qint16(v * 32768.0f))));
                    data[i].setValue(QAudioFormat::FrontRight, std::max<qint16>(-32768, std::min<qint16>(32767, qint16(v * 32768.0f))));
                }
                break;
            default:
                break;
            }
            break;
        }
        break;
    case QAudioFormat::ChannelConfigStereo:
        switch (fmt.bytesPerSample()){
        case 1:
            switch (fmt.sampleFormat()){
            case QAudioFormat::UInt8:
                for (int i=0; i<frames; i++){
                    quint8 l = ((const quint8*)s)[i*2];
                    quint8 r = ((const quint8*)s)[i*2+1];
                    data[i].setValue(QAudioFormat::FrontLeft, (l-128)*256);
                    data[i].setValue(QAudioFormat::FrontRight, (r-128)*256);
                }
                break;
            case QAudioFormat::Unknown:
                for (int i=0; i<frames; i++){
                    qint8 l = ((const qint8*)s)[i*2];
                    qint8 r = ((const qint8*)s)[i*2+1];
                    data[i].setValue(QAudioFormat::FrontLeft, l*256);
                    data[i].setValue(QAudioFormat::FrontRight, r*256);
                }
                break;
            default:
                break;
            }
            break;
        case 2:
            switch (fmt.sampleFormat()){
            case QAudioFormat::Unknown:
                for (int i=0; i<frames; i++){
                    quint16 l = ((const quint16*)s)[i*2];
                    quint16 r = ((const quint16*)s)[i*2+1];
                    data[i].setValue(QAudioFormat::FrontLeft, l-32768);
                    data[i].setValue(QAudioFormat::FrontRight, r-32768);
                }
                break;
            case QAudioFormat::Int16:
                for (int i=0; i<frames; i++){
                    qint16 l = ((const qint16*)s)[i*2];
                    qint16 r = ((const qint16*)s)[i*2+1];
                    data[i].setValue(QAudioFormat::FrontLeft, l);
                    data[i].setValue(QAudioFormat::FrontRight, r);
                }
                break;
            default:
                break;
            }
            break;
        case 3:
            switch (fmt.sampleFormat()){
            case QAudioFormat::NSampleFormats:
                for (int i=0; i<frames; i++){
                    qint32 ll = ((const quint8*)s)[i*6+1];
                    qint32 lh = ((const qint8*)s)[i*6+2];
                    qint32 rl = ((const quint8*)s)[i*6+4];
                    qint32 rh = ((const qint8*)s)[i*6+5];
                    data[i].setValue(QAudioFormat::FrontLeft, qint16(ll | (lh<<8)));
                    data[i].setValue(QAudioFormat::FrontRight, qint16(rl | (rh<<8)));
                }
                break;
            default:
                break;
            }
            break;
        case 4:
            switch (fmt.sampleFormat()){
            case QAudioFormat::Float:
                for (int i=0; i<frames; i++){
                    float l = ((const float*)s)[i*2];
                    float r = ((const float*)s)[i*2+1];
                    data[i].setValue(QAudioFormat::FrontLeft, std::max<qint16>(-32768, std::min<qint16>(32767, qint16(l * 32768.0f))));
                    data[i].setValue(QAudioFormat::FrontRight, std::max<qint16>(-32768, std::min<qint16>(32767, qint16(r * 32768.0f))));
                }
                break;
            default:
                break;
            }
            break;
        }
        break;
    default:
        break;
    }
}

StandardWaveData::~StandardWaveData()
{
	if (data){
        delete[] data;
	}
}


