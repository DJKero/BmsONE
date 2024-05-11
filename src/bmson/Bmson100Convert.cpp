#include "Bmson100Convert.h"
#include "Bmson100.h"
#include "Bmson021.h"
#include "../util/ResolutionUtil.h"

const char* Bmson100::ConverterFrom021::OriginalModeHint = "beat-7k";
const int Bmson100::ConverterFrom021::OriginalResolution = 240;


QJsonObject Bmson100::ConverterFrom021::Convert(const QJsonObject &v021)
{
	QJsonObject v100 = v021;
    v100.remove(Bmson021::Bmson::Bmson::InfoKey);
    v100.remove(Bmson021::Bmson::Bmson::BarLinesKey);
    v100.remove(Bmson021::Bmson::Bmson::BpmNotesKey);
    v100.remove(Bmson021::Bmson::Bmson::StopNotesKey);
    v100.remove(Bmson021::Bmson::Bmson::SoundChannelsKey);
    v100.remove(Bmson021::Bmson::Bmson::BgaKey);
    v100[Bmson100::Bmson::Bmson::VersionKey] = Bmson100::Bmson::Version;
    v100[Bmson100::Bmson::Bmson::InfoKey] = ConvertBmsonInfo(v021[Bmson021::Bmson::Bmson::InfoKey].toObject());
	{
        QJsonArray barLines100, barLines021 = v021[Bmson021::Bmson::Bmson::BarLinesKey].toArray();
		for (auto bar021 : barLines021){
			barLines100.append(ConvertBarLine(bar021.toObject()));
		}
        v100[Bmson100::Bmson::Bmson::BarLinesKey] = barLines100;
	}
	{
        QJsonArray bpmEvents100, bpmEvents021 = v021[Bmson021::Bmson::Bmson::BpmNotesKey].toArray();
		for (auto bpm021 : bpmEvents021){
			bpmEvents100.append(ConvertBpmEvent(bpm021.toObject()));
		}
        v100[Bmson100::Bmson::Bmson::BpmEventsKey] = bpmEvents100;
	}
	{
        QJsonArray stopEvents100, stopEvents021 = v021[Bmson021::Bmson::Bmson::StopNotesKey].toArray();
		for (auto stop021 : stopEvents021){
			stopEvents100.append(ConvertStopEvent(stop021.toObject()));
		}
        v100[Bmson100::Bmson::Bmson::StopEventsKey] = stopEvents100;
	}
	{
        QJsonArray channels100, channels021 = v021[Bmson021::Bmson::Bmson::SoundChannelsKey].toArray();
		for (auto c021 : channels021){
			channels100.append(ConvertSoundChannel(c021.toObject()));
		}
        v100[Bmson100::Bmson::Bmson::SoundChannelsKey] = channels100;
	}
    v100[Bmson100::Bmson::Bmson::BgaKey] = ConvertBga(v021[Bmson021::Bmson::Bmson::BgaKey].toObject());
	return v100;
}

QJsonObject Bmson100::ConverterFrom021::ConvertBmsonInfo(const QJsonObject &v021)
{
	QJsonObject v100 = v021;
    v100.remove(Bmson021::Bmson::BmsonInfo::TitleKey);
    v100.remove(Bmson021::Bmson::BmsonInfo::ArtistKey);
    v100.remove(Bmson021::Bmson::BmsonInfo::GenreKey);
    v100.remove(Bmson021::Bmson::BmsonInfo::LevelKey);
    v100.remove(Bmson021::Bmson::BmsonInfo::InitBpmKey);
    v100.remove(Bmson021::Bmson::BmsonInfo::JudgeRankKey);
    v100.remove(Bmson021::Bmson::BmsonInfo::TotalKey);
    v100[Bmson100::Bmson::BmsonInfo::TitleKey] = v021[Bmson021::Bmson::BmsonInfo::TitleKey];
    //v100[Bmson::BmsonInfo::SubtitleKey] = QString();
    v100[Bmson100::Bmson::BmsonInfo::ArtistKey] = v021[Bmson021::Bmson::BmsonInfo::ArtistKey];
    if (!v100.contains(Bmson100::Bmson::BmsonInfo::SubartistsKey) || !v100[Bmson100::Bmson::BmsonInfo::SubartistsKey].isArray()){
        v100[Bmson100::Bmson::BmsonInfo::SubartistsKey] = QJsonArray();
	}
    v100[Bmson100::Bmson::BmsonInfo::GenreKey] = v021[Bmson021::Bmson::BmsonInfo::GenreKey];
    v100[Bmson100::Bmson::BmsonInfo::ModeHintKey] = OriginalModeHint;
    //v100[Bmson100::Bmson::BmsonInfo::ChartNameKey] = QString();
    v100[Bmson100::Bmson::BmsonInfo::LevelKey] = v021[Bmson021::Bmson::BmsonInfo::LevelKey];
    v100[Bmson100::Bmson::BmsonInfo::InitBpmKey] = v021[Bmson021::Bmson::BmsonInfo::InitBpmKey];
    v100[Bmson100::Bmson::BmsonInfo::JudgeRankKey] = v021[Bmson021::Bmson::BmsonInfo::JudgeRankKey];
    v100[Bmson100::Bmson::BmsonInfo::TotalKey] = v021[Bmson021::Bmson::BmsonInfo::TotalKey];
    //v100[Bmson100::Bmson::BmsonInfo::BackImageKey] = QString();
    //v100[Bmson100::Bmson::BmsonInfo::EyecatchImageKey] = QString();
    //v100[Bmson100::Bmson::BmsonInfo::BannerKey] = QString();
    v100[Bmson100::Bmson::BmsonInfo::ResolutionKey] = OriginalResolution;
	return v100;
}

QJsonObject Bmson100::ConverterFrom021::ConvertBarLine(const QJsonObject &v021)
{
	QJsonObject v100 = v021;
    v100.remove(Bmson021::Bmson::BarLine::LocationKey);
    v100[Bmson100::Bmson::BarLine::LocationKey] = v021[Bmson021::Bmson::BarLine::LocationKey];
    // = v021[Bmson021::Bmson::BarLine::KindKey];
	return v100;
}

QJsonObject Bmson100::ConverterFrom021::ConvertSoundChannel(const QJsonObject &v021)
{
    QJsonObject v100 = v021;
    v100.remove(Bmson021::Bmson::SoundChannel::NameKey);
    v100.remove(Bmson021::Bmson::SoundChannel::NotesKey);
    v100[Bmson100::Bmson::SoundChannel::NameKey] = v021[Bmson021::Bmson::SoundChannel::NameKey];
    {
        QJsonArray notes100, notes021 = v021[Bmson021::Bmson::SoundChannel::NotesKey].toArray();
        for (auto n021 : notes021){
            notes100.append(ConvertNote(n021.toObject()));
        }
        v100[Bmson100::Bmson::SoundChannel::NotesKey] = notes100;
    }
    return v100;
}

QJsonObject Bmson100::ConverterFrom021::ConvertNote(const QJsonObject &v021)
{
    QJsonObject v100 = v021;
    v100.remove(Bmson021::Bmson::SoundNote::LaneKey);
    v100.remove(Bmson021::Bmson::SoundNote::LocationKey);
    v100.remove(Bmson021::Bmson::SoundNote::LengthKey);
    v100.remove(Bmson021::Bmson::SoundNote::CutKey);
    v100[Bmson100::Bmson::Note::LaneKey] = v021[Bmson021::Bmson::SoundNote::LaneKey];
    v100[Bmson100::Bmson::Note::LocationKey] = v021[Bmson021::Bmson::SoundNote::LocationKey];
    v100[Bmson100::Bmson::Note::LengthKey] = v021[Bmson021::Bmson::SoundNote::LengthKey];
    v100[Bmson100::Bmson::Note::ContinueKey] = v021[Bmson021::Bmson::SoundNote::CutKey];
    return v100;
}

QJsonObject Bmson100::ConverterFrom021::ConvertBpmEvent(const QJsonObject &v021)
{
	QJsonObject v100 = v021;
    v100.remove(Bmson021::Bmson::EventNote::LocationKey);
    v100.remove(Bmson021::Bmson::EventNote::ValueKey);
    v100[Bmson100::Bmson::BpmEvent::LocationKey] = v021[Bmson021::Bmson::EventNote::LocationKey];
    v100[Bmson100::Bmson::BpmEvent::BpmKey] = v021[Bmson021::Bmson::EventNote::ValueKey];
	return v100;
}

QJsonObject Bmson100::ConverterFrom021::ConvertStopEvent(const QJsonObject &v021)
{
	QJsonObject v100 = v021;
    v100.remove(Bmson021::Bmson::EventNote::LocationKey);
    v100.remove(Bmson021::Bmson::EventNote::ValueKey);
    v100[Bmson100::Bmson::StopEvent::LocationKey] = v021[Bmson021::Bmson::EventNote::LocationKey];
    v100[Bmson100::Bmson::StopEvent::DurationKey] = v021[Bmson021::Bmson::EventNote::ValueKey];
	return v100;
}

QJsonObject Bmson100::ConverterFrom021::ConvertBga(const QJsonObject &v021)
{
	QJsonObject v100 = v021;
    v100.remove(Bmson021::Bmson::Bga::DefinitionsKey);
    v100.remove(Bmson021::Bmson::Bga::BgaNotesKey);
    v100.remove(Bmson021::Bmson::Bga::LayerNotesKey);
    v100.remove(Bmson021::Bmson::Bga::MissNotesKey);
	{
        QJsonArray headers100, headers021 = v021[Bmson021::Bmson::Bga::DefinitionsKey].toArray();
		for (auto h021 : headers021){
			headers100.append(ConvertBgaHeader(h021.toObject()));
		}
        v100[Bmson100::Bmson::Bga::HeaderKey] = headers100;
	}
	{
        QJsonArray events100, events021 = v021[Bmson021::Bmson::Bga::BgaNotesKey].toArray();
		for (auto e021 : events021){
			events100.append(ConvertBgaEvent(e021.toObject()));
		}
        v100[Bmson100::Bmson::Bga::BgaEventsKey] = events100;
	}
	{
        QJsonArray events100, events021 = v021[Bmson021::Bmson::Bga::LayerNotesKey].toArray();
		for (auto e021 : events021){
			events100.append(ConvertBgaEvent(e021.toObject()));
		}
        v100[Bmson100::Bmson::Bga::LayerEventsKey] = events100;
	}
	{
        QJsonArray events100, events021 = v021[Bmson021::Bmson::Bga::MissNotesKey].toArray();
		for (auto e021 : events021){
			events100.append(ConvertBgaEvent(e021.toObject()));
		}
        v100[Bmson100::Bmson::Bga::MissEventsKey] = events100;
	}
	return v100;
}

QJsonObject Bmson100::ConverterFrom021::ConvertBgaHeader(const QJsonObject &v021)
{
	QJsonObject v100 = v021;
    v100.remove(Bmson021::Bmson::BgaDefinition::IdKey);
    v100.remove(Bmson021::Bmson::BgaDefinition::NameKey);
    v100[Bmson100::Bmson::BgaHeader::IdKey] = v021[Bmson021::Bmson::BgaDefinition::IdKey];
    v100[Bmson100::Bmson::BgaHeader::NameKey] = v021[Bmson021::Bmson::BgaDefinition::NameKey];
	return v100;
}

QJsonObject Bmson100::ConverterFrom021::ConvertBgaEvent(const QJsonObject &v021)
{
	QJsonObject v100 = v021;
    v100.remove(Bmson021::Bmson::BgaNote::LocationKey);
    v100.remove(Bmson021::Bmson::BgaNote::IdKey);
    v100[Bmson100::Bmson::BgaEvent::LocationKey] = v021[Bmson021::Bmson::BgaNote::LocationKey];
    v100[Bmson100::Bmson::BgaEvent::IdKey] = v021[Bmson021::Bmson::BgaNote::IdKey];
	return v100;
}




const int Bmson100::ConverterTo021::TargetResolution = 240;
const int Bmson100::ConverterTo021::DefaultOriginalResolution = 240;
const int Bmson100::ConverterTo021::OriginalBarLineKind = 0;

QJsonObject Bmson100::ConverterTo021::Convert(const QJsonObject &v100)
{
	QJsonObject v021 = v100;
    v021.remove(Bmson100::Bmson::Bmson::VersionKey);
    v021.remove(Bmson100::Bmson::Bmson::InfoKey);
    v021.remove(Bmson100::Bmson::Bmson::BarLinesKey);
    v021.remove(Bmson100::Bmson::Bmson::BpmEventsKey);
    v021.remove(Bmson100::Bmson::Bmson::StopEventsKey);
    v021.remove(Bmson100::Bmson::Bmson::SoundChannelsKey);
    v021.remove(Bmson100::Bmson::Bmson::BgaKey);
    QJsonObject info100 = v100[Bmson100::Bmson::Bmson::InfoKey].toObject();
    v021[Bmson021::Bmson::Bmson::InfoKey] = ConvertBmsonInfo(info100);
    int resolution = info100[Bmson100::Bmson::BmsonInfo::ResolutionKey].toInt(DefaultOriginalResolution);
	{
        QJsonArray bars021, bars100 = v100[Bmson100::Bmson::Bmson::BarLinesKey].toArray();
		for (auto b100 : bars100){
			bars021.append(ConvertBarLine(b100.toObject(), resolution));
		}
        v021[Bmson021::Bmson::Bmson::BarLinesKey] = bars021;
	}
	{
        QJsonArray bpms021, bpms100 = v100[Bmson100::Bmson::Bmson::BpmEventsKey].toArray();
		for (auto b100 : bpms100){
			bpms021.append(ConvertBpmEvent(b100.toObject(), resolution));
		}
        v021[Bmson021::Bmson::Bmson::BpmNotesKey] = bpms021;
	}
	{
        QJsonArray stops021, stops100 = v100[Bmson100::Bmson::Bmson::StopEventsKey].toArray();
		for (auto s100 : stops100){
			stops021.append(ConvertStopEvent(s100.toObject(), resolution));
		}
        v021[Bmson021::Bmson::Bmson::StopNotesKey] = stops021;
	}
	{
        QJsonArray channels021, channels100 = v100[Bmson100::Bmson::Bmson::SoundChannelsKey].toArray();
		for (auto c100 : channels100){
			channels021.append(ConvertSoundChannel(c100.toObject(), resolution));
		}
        v021[Bmson021::Bmson::Bmson::SoundChannelsKey] = channels021;
	}
    v021[Bmson021::Bmson::Bmson::BgaKey] = ConvertBga(v100[Bmson100::Bmson::Bmson::BgaKey].toObject(), resolution);
	return v021;
}

int Bmson100::ConverterTo021::ScaleTicks(int t100, int resolution)
{
	return ResolutionUtil::ConvertTicks(t100, TargetResolution, resolution);
}

QJsonObject Bmson100::ConverterTo021::ConvertBmsonInfo(const QJsonObject &v100)
{
	QJsonObject v021 = v100;

	// remove only redefinied keys
    v021.remove(Bmson100::Bmson::BmsonInfo::TitleKey);
    v021.remove(Bmson100::Bmson::BmsonInfo::ArtistKey);
    v021.remove(Bmson100::Bmson::BmsonInfo::GenreKey);
    v021.remove(Bmson100::Bmson::BmsonInfo::LevelKey);
    v021.remove(Bmson100::Bmson::BmsonInfo::InitBpmKey);
    v021.remove(Bmson100::Bmson::BmsonInfo::JudgeRankKey);
    v021.remove(Bmson100::Bmson::BmsonInfo::TotalKey);

    v021[Bmson021::Bmson::BmsonInfo::TitleKey] = v100[Bmson100::Bmson::BmsonInfo::TitleKey];
    // = v100[Bmson100::Bmson::BmsonInfo::SubtitleKey];
    v021[Bmson021::Bmson::BmsonInfo::ArtistKey] = v100[Bmson100::Bmson::BmsonInfo::ArtistKey];
    // = v100[Bmson100::Bmson::BmsonInfo::SubartistsKey];
    v021[Bmson021::Bmson::BmsonInfo::GenreKey] = v100[Bmson100::Bmson::BmsonInfo::GenreKey];
    // = v100[Bmson100::Bmson::BmsonInfo::ModeHintKey];
    // = v100[Bmson100::Bmson::BmsonInfo::ChartNameKey];
    v021[Bmson021::Bmson::BmsonInfo::LevelKey] = v100[Bmson100::Bmson::BmsonInfo::LevelKey];
    v021[Bmson021::Bmson::BmsonInfo::InitBpmKey] = v100[Bmson100::Bmson::BmsonInfo::InitBpmKey];
    v021[Bmson021::Bmson::BmsonInfo::JudgeRankKey] = v100[Bmson100::Bmson::BmsonInfo::JudgeRankKey];
    v021[Bmson021::Bmson::BmsonInfo::TotalKey] = v100[Bmson100::Bmson::BmsonInfo::TotalKey];
    // = v100[Bmson100::Bmson::BmsonInfo::BackImageKey];
    // = v100[Bmson100::Bmson::BmsonInfo::EyecatchImageKey];
    // = v100[Bmson100::Bmson::BmsonInfo::BannerKey];
    // = v100[Bmson100::Bmson::BmsonInfo::ResolutionKey];
	return v021;
}

QJsonObject Bmson100::ConverterTo021::ConvertBarLine(const QJsonObject &v100, int resolution)
{
	QJsonObject v021 = v100;
    v021.remove(Bmson100::Bmson::BarLine::LocationKey);
    v021[Bmson021::Bmson::BarLine::LocationKey] = ScaleTicks(v100[Bmson100::Bmson::BarLine::LocationKey].toInt(), resolution);
    v021[Bmson021::Bmson::BarLine::KindKey] = OriginalBarLineKind;
	return v021;
}

QJsonObject Bmson100::ConverterTo021::ConvertBpmEvent(const QJsonObject &v100, int resolution)
{
	QJsonObject v021 = v100;
    v021.remove(Bmson100::Bmson::BpmEvent::LocationKey);
    v021.remove(Bmson100::Bmson::BpmEvent::BpmKey);
    v021[Bmson021::Bmson::EventNote::LocationKey] = ScaleTicks(v100[Bmson100::Bmson::BpmEvent::LocationKey].toInt(), resolution);
    v021[Bmson021::Bmson::EventNote::ValueKey] = v100[Bmson100::Bmson::BpmEvent::BpmKey];
	return v021;
}

QJsonObject Bmson100::ConverterTo021::ConvertStopEvent(const QJsonObject &v100, int resolution)
{
	QJsonObject v021 = v100;
    v021.remove(Bmson100::Bmson::StopEvent::LocationKey);
    v021.remove(Bmson100::Bmson::StopEvent::DurationKey);
    v021[Bmson021::Bmson::EventNote::LocationKey] = ScaleTicks(v100[Bmson100::Bmson::StopEvent::LocationKey].toInt(), resolution);
    v021[Bmson021::Bmson::EventNote::ValueKey] = ScaleTicks(v100[Bmson100::Bmson::StopEvent::DurationKey].toInt(), resolution); // duration is pulse-based
	return v021;
}

QJsonObject Bmson100::ConverterTo021::ConvertSoundChannel(const QJsonObject &v100, int resolution)
{
	QJsonObject v021 = v100;
    v021.remove(Bmson100::Bmson::SoundChannel::NameKey);
    v021.remove(Bmson100::Bmson::SoundChannel::NotesKey);
    v021[Bmson021::Bmson::SoundChannel::NameKey] = v100[Bmson100::Bmson::SoundChannel::NameKey];
	{
        QJsonArray notes021, notes100 = v100[Bmson100::Bmson::SoundChannel::NotesKey].toArray();
		for (auto n100 : notes100){
			notes021.append(ConvertNote(n100.toObject(), resolution));
		}
        v021[Bmson021::Bmson::SoundChannel::NotesKey] = notes021;
	}
	return v021;
}

QJsonObject Bmson100::ConverterTo021::ConvertNote(const QJsonObject &v100, int resolution)
{
	QJsonObject v021 = v100;
    v021.remove(Bmson100::Bmson::Note::LaneKey);
    v021.remove(Bmson100::Bmson::Note::LocationKey);
    v021.remove(Bmson100::Bmson::Note::LengthKey);
    v021.remove(Bmson100::Bmson::Note::ContinueKey);
    v021[Bmson021::Bmson::SoundNote::LaneKey] = v100[Bmson100::Bmson::Note::LaneKey];
    v021[Bmson021::Bmson::SoundNote::LocationKey] = ScaleTicks(v100[Bmson100::Bmson::Note::LocationKey].toInt(), resolution);
    v021[Bmson021::Bmson::SoundNote::LengthKey] = ScaleTicks(v100[Bmson100::Bmson::Note::LengthKey].toInt(), resolution);
    v021[Bmson021::Bmson::SoundNote::CutKey] = v100[Bmson100::Bmson::Note::ContinueKey];
	return v021;
}

QJsonObject Bmson100::ConverterTo021::ConvertBga(const QJsonObject &v100, int resolution)
{
	QJsonObject v021 = v100;
    v021.remove(Bmson100::Bmson::Bga::HeaderKey);
    v021.remove(Bmson100::Bmson::Bga::BgaEventsKey);
    v021.remove(Bmson100::Bmson::Bga::LayerEventsKey);
    v021.remove(Bmson100::Bmson::Bga::MissEventsKey);
	{
        QJsonArray headers021, headers100 = v100[Bmson100::Bmson::Bga::HeaderKey].toArray();
		for (auto h100 : headers100){
			headers021.append(ConvertBgaHeader(h100.toObject()));
		}
        v021[Bmson021::Bmson::Bga::DefinitionsKey] = headers021;
	}
	{
        QJsonArray events021, events100 = v100[Bmson100::Bmson::Bga::BgaEventsKey].toArray();
		for (auto e100 : events100){
			events100.append(ConvertBgaEvent(e100.toObject(), resolution));
		}
        v021[Bmson021::Bmson::Bga::BgaNotesKey] = events021;
	}
	{
        QJsonArray events021, events100 = v100[Bmson100::Bmson::Bga::LayerEventsKey].toArray();
		for (auto e100 : events100){
			events100.append(ConvertBgaEvent(e100.toObject(), resolution));
		}
        v021[Bmson021::Bmson::Bga::LayerNotesKey] = events021;
	}
	{
        QJsonArray events021, events100 = v100[Bmson100::Bmson::Bga::MissEventsKey].toArray();
		for (auto e100 : events100){
			events100.append(ConvertBgaEvent(e100.toObject(), resolution));
		}
        v021[Bmson021::Bmson::Bga::MissNotesKey] = events021;
	}
	return v021;
}

QJsonObject Bmson100::ConverterTo021::ConvertBgaHeader(const QJsonObject &v100)
{
	QJsonObject v021 = v100;
    v021.remove(Bmson100::Bmson::BgaHeader::IdKey);
    v021.remove(Bmson100::Bmson::BgaHeader::NameKey);
    v021[Bmson021::Bmson::BgaDefinition::IdKey] = v100[Bmson100::Bmson::BgaHeader::IdKey];
    v021[Bmson021::Bmson::BgaDefinition::NameKey] = v100[Bmson100::Bmson::BgaHeader::NameKey];
	return v021;
}

QJsonObject Bmson100::ConverterTo021::ConvertBgaEvent(const QJsonObject &v100, int resolution)
{
	QJsonObject v021 = v100;
    v021.remove(Bmson100::Bmson::BgaEvent::IdKey);
    v021.remove(Bmson100::Bmson::BgaEvent::LocationKey);
    v021[Bmson021::Bmson::BgaNote::IdKey] = v100[Bmson100::Bmson::BgaEvent::IdKey];
    v021[Bmson021::Bmson::BgaNote::LocationKey] = ScaleTicks(v100[Bmson100::Bmson::BgaEvent::LocationKey].toInt(), resolution);
	return v021;
}
