#-------------------------------------------------
#
# Project created by QtCreator 2015-08-14T09:15:37
#
# Project modified by DJKero 2024-04-29T22:30:00
#
#-------------------------------------------------

TARGET = BmsONE
CONFIG += c++17 warn_on
FORMS    +=
QT       += core concurrent gui multimedia
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
#QT += gui-private
TEMPLATE = app
INCLUDEPATH += $$PWD/ $$PWD/libvorbis
DEPENDPATH += $$PWD/
RESOURCES += bmsone.qrc
#DISTFILES += images/symbols/sound_channel_lane.png
TRANSLATIONS = i18n/ja.ts

linux-g++ {
    QMAKE_CXXFLAGS += -std=c++17
    QMAKE_CXXFLAGS_DEBUG += -ggdb -O0 # For use with GDB
}

macx_clang {
    QMAKE_CXXFLAGS += -std=c++17 -stdlib=libc++
    QMAKE_CXXFLAGS_DEBUG += -g3 -O0 # For use with other debuggers.
}

#
# TODO: UPDATE TO MSVC2017+
#
# Don't know how to update it to a newer version of MSVC or make it use MINGW.
win32_msvc2015 {
    QMAKE_LFLAGS_DEBUG += /NODEFAULTLIB:MSVCRT /NODEFAULTLIB:libcmt
    QMAKE_LFLAGS_RELEASE += /NODEFAULTLIB:libcmt
}

debug {
    CONFIG += sanitizer sanitize_address
}


# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += main.cpp\
                MainWindow.cpp \
    InfoView.cpp \
        ChannelInfoView.cpp \
    AudioPlayer.cpp \
    SequenceTools.cpp \
        BpmEditTool.cpp \
        StatusBar.cpp \
        PreviewConfig.cpp \
        Preferences.cpp \
        ViewMode.cpp \
        MasterView.cpp \
        EditConfig.cpp \
        PrefEdit.cpp \
        ChannelFindTools.cpp \
        PrefPreview.cpp \
        ExternalViewer.cpp \
        ExternalViewerTools.cpp \
        NoteEditTool.cpp \
    MasterOutDialog.cpp \
    PrefBms.cpp \
        sequence_view/SequenceView.cpp \
        sequence_view/SequenceViewInternal.cpp \
        sequence_view/SequenceViewContents.cpp \
        sequence_view/SequenceViewCursor.cpp \
        sequence_view/SequenceViewEditMode.cpp \
        sequence_view/SequenceViewWriteMode.cpp \
        sequence_view/SequenceViewPreview.cpp \
        sequence_view/SequenceViewChannelInternal.cpp \
        sequence_view/Skin.cpp \
        util/ScrollableForm.cpp \
        util/QuasiModalEdit.cpp \
        util/CollapseButton.cpp \
        util/JsonExtension.cpp \
        util/Util.cpp \
        util/SymbolIconManager.cpp \
        util/ScalarRegion.cpp \
        util/SignalFunction.cpp \
        audio/QOggVorbisAdapter.cpp \
        audio/WaveMix.cpp \
        audio/WaveData.cpp \
        audio/WaveStream.cpp \
        document/History.cpp \
        document/Document.cpp \
        document/SoundChannel.cpp \
        document/SoundChannelPreview.cpp \
        document/SoundChannelInternal.cpp \
        document/HistoryUtil.cpp \
        document/DocumentInfo.cpp \
    document/MasterCache.cpp \
    document/Bga.cpp \
        bmson/Bmson021.cpp \
        bmson/Bmson100.cpp \
        bmson/Bmson100Convert.cpp \
        bmson/BmsonConvertDef.cpp \
        bmson/Bmson.cpp \
        bms/Bms.cpp \
    bms/BmsUtil.cpp \
    bms/BmsImportDialog.cpp \
    libogg/bitwise.c \
    libogg/framing.c \
    libvorbis/analysis.c \
    libvorbis/bitrate.c \
    libvorbis/block.c \
    libvorbis/codebook.c \
    libvorbis/envelope.c \
    libvorbis/floor0.c \
    libvorbis/floor1.c \
    libvorbis/info.c \
    libvorbis/lookup.c \
    libvorbis/lpc.c \
    libvorbis/lsp.c \
    libvorbis/mapping0.c \
    libvorbis/mdct.c \
    libvorbis/psy.c \
    libvorbis/registry.c \
    libvorbis/res0.c \
    libvorbis/sharedbook.c \
    libvorbis/smallft.c \
    libvorbis/synthesis.c \
    libvorbis/vorbisenc.c \
    libvorbis/vorbisfile.c \
    libvorbis/window.c

HEADERS  += MainWindow.h \
        ChannelInfoView.h \
    AudioPlayer.h \
        InfoView.h \
        SequenceTools.h \
        Preferences.h \
        ViewMode.h \
        MasterView.h \
        EditConfig.h \
        PrefEdit.h \
        AppInfo.h \
        ChannelFindTools.h \
        PreviewConfig.h \
        PrefPreview.h \
        ExternalViewer.h \
        ExternalViewerTools.h \
        NoteEditTool.h \
        MasterOutDialog.h \
    BpmEditTool.h \
    PrefBms.h \
        sequence_view/SequenceView.h \
        sequence_view/SequenceDef.h \
        sequence_view/SequenceViewChannelInternal.h \
        sequence_view/Skin.h \
        sequence_view/SequenceViewInternal.h \
        sequence_view/SequenceViewDef.h \
        sequence_view/SequenceViewContexts.h \
        util/QuasiModalEdit.h \
        util/ScrollableForm.h \
        util/CollapseButton.h \
        util/SignalFunction.h \
        util/JsonExtension.h \
        util/UIDef.h \
        util/ScalarRegion.h \
        util/ResolutionUtil.h \
        util/SymbolIconManager.h \
        util/Counter.h \
        audio/Wave.h \
        audio/QOggVorbisAdapter.h \
        document/History.h \
        document/Document.h \
        document/SoundChannel.h \
        document/SoundChannelInternal.h \
        document/DocumentDef.h \
        document/SoundChannelDef.h \
        document/MasterCache.h \
        document/HistoryUtil.h \
        document/DocumentAux.h \
        bmson/Bmson100.h \
        bmson/Bmson021.h \
        bmson/Bmson100Convert.h \
        bmson/BmsonConvertDef.h \
        bmson/Bmson.h \
    bms/Bms.h \
    bms/BmsImportDialog.h \
    libvorbis/books/coupled/res_books_51.h \
    libvorbis/books/coupled/res_books_stereo.h \
    libvorbis/books/floor/floor_books.h \
    libvorbis/books/uncoupled/res_books_uncoupled.h \
    libvorbis/modes/floor_all.h \
    libvorbis/modes/psych_8.h \
    libvorbis/modes/psych_11.h \
    libvorbis/modes/psych_16.h \
    libvorbis/modes/psych_44.h \
    libvorbis/modes/residue_8.h \
    libvorbis/modes/residue_16.h \
    libvorbis/modes/residue_44.h \
    libvorbis/modes/residue_44p51.h \
    libvorbis/modes/residue_44u.h \
    libvorbis/modes/setup_8.h \
    libvorbis/modes/setup_11.h \
    libvorbis/modes/setup_16.h \
    libvorbis/modes/setup_22.h \
    libvorbis/modes/setup_32.h \
    libvorbis/modes/setup_44.h \
    libvorbis/modes/setup_44p51.h \
    libvorbis/modes/setup_44u.h \
    libvorbis/modes/setup_X.h


macx: ICON = bmsone.icns
win32: RC_ICONS = bmsone.ico




# Default rules for deployment.
#qnx: target.path = /tmp/$${TARGET}/bin
#else: unix:!android: target.path = /opt/$${TARGET}/bin
#!isEmpty(target.path): INSTALLS += target
