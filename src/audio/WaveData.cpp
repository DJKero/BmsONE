#include "Wave.h"
#include "QOggVorbisAdapter.h"
#include <cstring>


WaveData::WaveData()
	: data(nullptr)
{
    error = NoError;
    extension = UnsupportedFormat;
    format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    format.setSampleFormat(QAudioFormat::Int16);
    format.setSampleRate(44100);
	bytes = 0;
    frames = 0;
}

WaveData::WaveData(const QString &srcPath)
	: data(nullptr)
{
    error = NoError;
    extension = UnsupportedFormat;
	QFileInfo fi(srcPath);
	QString ext = fi.suffix().toLower();
	if (ext == "wav"){
        LoadWav(srcPath);
	}else if (ext == "ogg"){
        LoadOgg(srcPath);
	}else{
        error = UnknownFileType;
		return;
	}
}

void WaveData::LoadWav(const QString &srcPath)
{
	int safetyCounter = 0;
	QFile file(srcPath);
	if (!file.open(QFile::ReadOnly)){
        error = CannotOpenFile;
		return;
	}
	QDataStream din(&file);
	din.setByteOrder(QDataStream::LittleEndian);
	if (file.read(4) != "RIFF"){
        error = NotWaveFile;
		return;
	}
	din.skipRawData(4);
	if (file.read(4) != "WAVE"){
        error = NotWaveFile;
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
            error = FormatMissing;
			return;
		}
		while (file.read(4) != "fmt "){
			if (++safetyCounter > 1000 || file.atEnd()){
                error = FormatMissing;
				return;
			}
			quint32 ckSize = 0;
			din >> ckSize;
			din.skipRawData(ckSize);
		}
		quint32 ckSize = 0;
		din >> ckSize;
		if (ckSize < 16){
            error = FormatMissing;
			return;
		}
		din >> formatTag >> channelsCount >> samplesPerSec >> avgBytesPerSec >> blockAlign >> bitsPerSample;
		din.skipRawData(ckSize - 16);
		if (formatTag != 1 /*WAVE_FORMAT_PCM*/ || channelsCount > 2){
			qDebug() << "formatTag: " << formatTag;
			qDebug() << "channels: " << channelsCount;
            error = UnsupportedFormat;
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
                    error = UnsupportedFormat;
                    return;
                }
                break;
            case 16:
                // 16bit signed int (8000 - 0000 - 7FFF )
                format.setSampleFormat(QAudioFormat::Int16);;
                if (blockAlign != 2*channelsCount){
                    qDebug() << "16bit x " << channelsCount << "ch -> " << blockAlign;
                    error = UnsupportedFormat;
                    return;
                }
                break;
            case 24:
                // 24bit signed int (800000 - 000000 - 7FFFFF)
                format.setSampleFormat(QAudioFormat::NSampleFormats);
                if (blockAlign != 3*channelsCount){
                    qDebug() << "24bit x " << channelsCount << "ch -> " << blockAlign;
                    error = UnsupportedFormat;
                    return;
                }
                break;
            case 32:
                // 32bit float
                format.setSampleFormat(QAudioFormat::Float);
                if (blockAlign != 4*channelsCount){
                    qDebug() << "32bit x " << channelsCount << "ch -> " << blockAlign;
                    error = UnsupportedFormat;
                    return;
                }
                break;
            default:
                qDebug() << "bits: " << bitsPerSample;
                error = UnsupportedFormat;
                return;
		}
	}
	{	// read [data] chunk
		if (file.atEnd()){
            error = DataMissing;
			return;
		}
		while (file.read(4) != "data"){
			if (++safetyCounter > 1000 || file.atEnd()){
                error = DataMissing;
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
            error = Unknown;
            return;
        }
		if (frames == 0){
            error = DataMissing;
			return;
		}
		if (bytes >= 0x40000000){ // >= 1G Bytes
            error = DataSizeOver;
			return;
		}

        extension = WAV;
		data = new quint8[bytes];
		char *d = (char*)data;
		int remainingSize = bytes;
		static const int unitSize = 0x00100000;
		while (remainingSize > unitSize){
			if (din.readRawData(d, unitSize) != unitSize){
                error = DataMissing;
				return;
			}
			remainingSize -= unitSize;
			d += unitSize;
		}
		if (din.readRawData(d, remainingSize) != remainingSize){
            error = DataMissing;
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
        error = DataSizeOver;
		ov_clear(&file);
		return;
	}
	static const int bufferSize = 4096;

    extension = OGG;
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
        switch (extension) {
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
    error = Unknown;
}
