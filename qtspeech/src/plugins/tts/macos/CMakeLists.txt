# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_plugin(QTextToSpeechMacOSPlugin
    OUTPUT_NAME qtexttospeech_speech_macos
    PLUGIN_TYPE texttospeech
    SOURCES
        qtexttospeech_macos.h qtexttospeech_macos.mm
        qtexttospeech_macos_plugin.cpp qtexttospeech_macos_plugin.h
    LIBRARIES
        ${FWCocoa}
        Qt::Core
        Qt::Gui
        Qt::TextToSpeech
        Qt::Multimedia
)
