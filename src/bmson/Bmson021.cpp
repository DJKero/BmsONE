#include "Bmson021.h"

namespace Bmson021
{
    namespace Bmson
    {
        const char* Bmson::InfoKey = "info";
        const char* Bmson::BpmNotesKey = "bpmNotes";
        const char* Bmson::StopNotesKey = "stopNotes";
        const char* Bmson::SoundChannelsKey = "soundChannel";
        const char* Bmson::BarLinesKey = "lines";
        const char* Bmson::BgaKey = "bga";

        const char* BmsonInfo::TitleKey = "title";
        const char* BmsonInfo::GenreKey = "genre";
        const char* BmsonInfo::ArtistKey = "artist";
        const char* BmsonInfo::JudgeRankKey = "judgeRank";
        const char* BmsonInfo::TotalKey = "total";
        const char* BmsonInfo::InitBpmKey = "initBPM";
        const char* BmsonInfo::LevelKey = "level";

        const char* BarLine::LocationKey = "y";
        const char* BarLine::KindKey = "k";

        const char* SoundChannel::NameKey = "name";
        const char* SoundChannel::NotesKey = "notes";

        const char* SoundNote::LaneKey = "x";
        const char* SoundNote::LocationKey = "y";
        const char* SoundNote::LengthKey = "l";
        const char* SoundNote::CutKey = "c";

        const char* EventNote::LocationKey = "y";
        const char* EventNote::ValueKey = "v";

        const char* Bga::DefinitionsKey = "bgaHeader";
        const char* Bga::BgaNotesKey = "bgaNotes";
        const char* Bga::LayerNotesKey = "layerNotes";
        const char* Bga::MissNotesKey = "poorNotes";

        const char* BgaDefinition::IdKey = "ID";
        const char* BgaDefinition::NameKey = "name";

        const char* BgaNote::IdKey = "ID";
        const char* BgaNote::LocationKey = "y";
    } // Bmson
} // Bmson100
