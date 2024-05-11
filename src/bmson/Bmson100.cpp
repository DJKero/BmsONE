#include "Bmson100.h"

namespace Bmson100
{
    namespace Bmson
    {
        const char* Version = "1.0.0";

        const char* Bmson::VersionKey = "version";
        const char* Bmson::InfoKey = "info";
        const char* Bmson::BarLinesKey = "lines";
        const char* Bmson::BpmEventsKey = "bpm_events";
        const char* Bmson::StopEventsKey = "stop_events";
        const char* Bmson::SoundChannelsKey = "sound_channels";
        const char* Bmson::BgaKey = "bga";

        const char* BmsonInfo::TitleKey = "title";
        const char* BmsonInfo::SubtitleKey = "subtitle";
        const char* BmsonInfo::ArtistKey = "artist";
        const char* BmsonInfo::SubartistsKey = "subartists";
        const char* BmsonInfo::GenreKey = "genre";
        const char* BmsonInfo::ModeHintKey = "mode_hint";
        const char* BmsonInfo::ChartNameKey = "chart_name";
        const char* BmsonInfo::LevelKey = "level";
        const char* BmsonInfo::InitBpmKey = "init_bpm";
        const char* BmsonInfo::JudgeRankKey = "judge_rank";
        const char* BmsonInfo::TotalKey = "total";
        const char* BmsonInfo::BackImageKey = "back_image";
        const char* BmsonInfo::EyecatchImageKey = "eyecatch_image";
        const char* BmsonInfo::TitleImageKey = "title_image";
        const char* BmsonInfo::BannerKey = "banner_image";
        const char* BmsonInfo::PreviewMusicKey = "preview_music";
        const char* BmsonInfo::ResolutionKey = "resolution";

        const char* BarLine::LocationKey = "y";

        const char* SoundChannel::NameKey = "name";
        const char* SoundChannel::NotesKey = "notes";

        const char* Note::LaneKey = "x";
        const char* Note::LocationKey = "y";
        const char* Note::LengthKey = "l";
        const char* Note::ContinueKey = "c";

        const char* BpmEvent::LocationKey = "y";
        const char* BpmEvent::BpmKey = "bpm";

        const char* StopEvent::LocationKey = "y";
        const char* StopEvent::DurationKey = "duration";

        const char* Bga::HeaderKey = "bga_header";
        const char* Bga::BgaEventsKey = "bga_events";
        const char* Bga::LayerEventsKey = "layer_events";
        const char* Bga::MissEventsKey = "poor_events";

        const char* BgaHeader::IdKey = "id";
        const char* BgaHeader::NameKey = "name";

        const char* BgaEvent::LocationKey = "y";
        const char* BgaEvent::IdKey = "id";
    } // Bmson
} // Bmson100
