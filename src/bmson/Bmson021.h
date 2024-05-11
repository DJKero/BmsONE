#ifndef BMSON_021_H
#define BMSON_021_H

namespace Bmson021
{
    namespace Bmson
    {
        struct Bmson
        {
            static const char* InfoKey;
            static const char* BarLinesKey;
            static const char* BpmNotesKey;
            static const char* StopNotesKey;
            static const char* SoundChannelsKey;
            static const char* BgaKey;
        };

        struct BmsonInfo
        {
            static const char* TitleKey;
            static const char* GenreKey;
            static const char* ArtistKey;
            static const char* JudgeRankKey;
            static const char* TotalKey;
            static const char* InitBpmKey;
            static const char* LevelKey;
        };

        struct BarLine
        {
            static const char* LocationKey;
            static const char* KindKey;
        };

        struct SoundChannel
        {
            static const char* NameKey;
            static const char* NotesKey;
        };

        struct SoundNote
        {
            static const char* LaneKey;
            static const char* LocationKey;
            static const char* LengthKey;
            static const char* CutKey;
        };

        struct EventNote
        {
            static const char* LocationKey;
            static const char* ValueKey;
        };

        struct Bga
        {
            static const char* DefinitionsKey;
            static const char* BgaNotesKey;
            static const char* LayerNotesKey;
            static const char* MissNotesKey;
        };

        struct BgaDefinition
        {
            static const char* IdKey;
            static const char* NameKey;
        };

        struct BgaNote
        {
            static const char* IdKey;
            static const char* LocationKey;
        };
    } // Bmson
} // Bmson021


#endif // BMSON_021_H

