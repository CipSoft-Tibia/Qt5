HEADERS += $$PWD/assembler/*.h
SOURCES += $$PWD/assembler/ARMv7Assembler.cpp
SOURCES += $$PWD/assembler/LinkBuffer.cpp

HEADERS += $$PWD/wtf/*.h
SOURCES += $$PWD/wtf/PrintStream.cpp
HEADERS += $$PWD/wtf/PrintStream.h

SOURCES += $$PWD/wtf/FilePrintStream.cpp
HEADERS += $$PWD/wtf/FilePrintStream.h

HEADERS += $$PWD/wtf/RawPointer.h

winrt: SOURCES += $$PWD/wtf/OSAllocatorWinRT.cpp
else:win32: SOURCES += $$PWD/wtf/OSAllocatorWin.cpp
else:integrity: SOURCES += $$PWD/wtf/OSAllocatorIntegrity.cpp
else: SOURCES += $$PWD/wtf/OSAllocatorPosix.cpp
HEADERS += $$PWD/wtf/OSAllocator.h

SOURCES += $$PWD/wtf/PageAllocationAligned.cpp
HEADERS += $$PWD/wtf/PageAllocationAligned.h
HEADERS += $$PWD/wtf/PageAllocation.h

SOURCES += $$PWD/wtf/PageBlock.cpp
HEADERS += $$PWD/wtf/PageBlock.h

HEADERS += $$PWD/wtf/PageReservation.h

SOURCES += $$PWD/stubs/WTFStubs.cpp
HEADERS += $$PWD/stubs/WTFStubs.h

SOURCES += $$PWD/stubs/Options.cpp

HEADERS += $$PWD/stubs/wtf/FastAllocBase.h \
           $$PWD/stubs/wtf/FastMalloc.h \
           $$PWD/stubs/wtf/Noncopyable.h \
           $$PWD/stubs/wtf/OwnPtr.h \
           $$PWD/stubs/wtf/PassOwnPtr.h \
           $$PWD/stubs/wtf/PassRefPtr.h \
           $$PWD/stubs/wtf/RefCounted.h \
           $$PWD/stubs/wtf/RefPtr.h \
           $$PWD/stubs/wtf/TypeTraits.h \
           $$PWD/stubs/wtf/UnusedParam.h \
           $$PWD/stubs/wtf/Vector.h

SOURCES += $$PWD/disassembler/Disassembler.cpp
SOURCES += $$PWD/disassembler/UDis86Disassembler.cpp
contains(DEFINES, WTF_USE_UDIS86=1) {
    SOURCES += $$PWD/disassembler/udis86/udis86.c
    SOURCES += $$PWD/disassembler/udis86/udis86_decode.c
    SOURCES += $$PWD/disassembler/udis86/udis86_input.c
    SOURCES += $$PWD/disassembler/udis86/udis86_itab_holder.c
    SOURCES += $$PWD/disassembler/udis86/udis86_syn-att.c
    SOURCES += $$PWD/disassembler/udis86/udis86_syn.c
    SOURCES += $$PWD/disassembler/udis86/udis86_syn-intel.c

    ITAB = $$PWD/disassembler/udis86/optable.xml
    udis86.output = udis86_itab.h
    udis86.input = ITAB
    udis86.CONFIG += no_link
    udis86.commands = $$QMAKE_PYTHON $$PWD/disassembler/udis86/itab.py ${QMAKE_FILE_IN}
    QMAKE_EXTRA_COMPILERS += udis86

    udis86_tab_cfile.target = $$OUT_PWD/udis86_itab.c
    udis86_tab_cfile.depends = udis86_itab.h
    QMAKE_EXTRA_TARGETS += udis86_tab_cfile
}

# We can always compile these, they have ifdef guards inside
SOURCES += $$PWD/disassembler/ARMv7Disassembler.cpp
SOURCES += $$PWD/disassembler/ARMv7/ARMv7DOpcode.cpp
HEADERS += $$PWD/disassembler/ARMv7/ARMv7DOpcode.h
SOURCES += $$PWD/disassembler/Mips32Disassembler.cpp
SOURCES += $$PWD/disassembler/mips32/Mips32Opcode.cpp
HEADERS += $$PWD/disassembler/mips32/Mips32Opcode.h
SOURCES += $$PWD/disassembler/ARM64Disassembler.cpp
SOURCES += $$PWD/disassembler/ARM64/A64DOpcode.cpp
HEADERS += $$PWD/disassembler/ARM64/A64DOpcode.h

SOURCES += $$PWD/yarr/YarrCanonicalizeUCS2.cpp \
           $$PWD/yarr/YarrCanonicalizeUnicode.cpp \
           $$PWD/yarr/YarrInterpreter.cpp \
           $$PWD/yarr/YarrJIT.cpp \
           $$PWD/yarr/YarrPattern.cpp \
           $$PWD/yarr/YarrSyntaxChecker.cpp \
           $$PWD/stubs/yarr/YarrUnicodeProperties.cpp

HEADERS += $$PWD/yarr/Yarr.h \
           $$PWD/yarr/YarrCanonicalizeUCS2.h \
           $$PWD/yarr/YarrInterpreter.h \
           $$PWD/yarr/YarrJIT.h \
           $$PWD/yarr/YarrParser.h \
           $$PWD/yarr/YarrPattern.h \
           $$PWD/yarr/YarrSyntaxChecker.h \
           $$PWD/yarr/YarrUnicodeProperties.h

#
# Generate RegExpJitTables.h
#
GENERATEDDIR = .generated
debug_and_release {
    CONFIG(debug, debug|release) {
        GENERATEDDIR = $$GENERATEDDIR/debug
    } else {
        GENERATEDDIR = $$GENERATEDDIR/release
    }
}
INCLUDEPATH += $$GENERATEDDIR

retgen.output = $$GENERATEDDIR/RegExpJitTables.h
retgen.script = $$PWD/yarr/create_regex_tables
retgen.input = retgen.script
retgen.CONFIG += no_link
retgen.commands = $$QMAKE_PYTHON $$retgen.script > ${QMAKE_FILE_OUT}
QMAKE_EXTRA_COMPILERS += retgen

# Taken from WebKit/Tools/qmake/mkspecs/features/unix/default_post.prf
!c++11:!intel_icc {
    # Don't warn about OVERRIDE and FINAL, since they are feature-checked anyways
    clang {
        QMAKE_CXXFLAGS += -Wno-c++11-extensions -Wno-c++0x-extensions
    } else: gcc {
        greaterThan(QT_GCC_MAJOR_VERSION, 4)|greaterThan(QT_GCC_MINOR_VERSION, 5) {
            # We need to deactivate those warnings because some names conflicts with upcoming c++0x types (e.g.nullptr).
            QMAKE_CXXFLAGS += -Wno-c++0x-compat
        }
    }
}
