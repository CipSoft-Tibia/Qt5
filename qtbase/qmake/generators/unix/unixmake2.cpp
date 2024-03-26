// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "unixmake.h"
#include "option.h"
#include "meta.h"
#include <qregularexpression.h>
#include <qbytearray.h>
#include <qfile.h>
#include <qdir.h>
#include <qdebug.h>
#include <qtversion.h>
#include <time.h>

#include <tuple>
#include <utility>

QT_BEGIN_NAMESPACE

void
UnixMakefileGenerator::writePrlFile(QTextStream &t)
{
    MakefileGenerator::writePrlFile(t);
    const ProString tmplt = project->first("TEMPLATE");
    if (tmplt != "lib" && tmplt != "aux")
        return;
    // libtool support
    if (project->isActiveConfig("create_libtool")) {
        writeLibtoolFile();
    }
    // pkg-config support
    if (project->isActiveConfig("create_pc"))
        writePkgConfigFile();
}

bool
UnixMakefileGenerator::writeMakefile(QTextStream &t)
{

    writeHeader(t);
    if (writeDummyMakefile(t))
        return true;

    if (project->first("TEMPLATE") == "app" ||
        project->first("TEMPLATE") == "lib" ||
        project->first("TEMPLATE") == "aux") {
        writeMakeParts(t);
        return MakefileGenerator::writeMakefile(t);
    } else if (project->first("TEMPLATE") == "subdirs") {
        MakefileGenerator::writeSubDirs(t);
        return true;
    }
    return false;
}

void
UnixMakefileGenerator::writeDefaultVariables(QTextStream &t)
{
    MakefileGenerator::writeDefaultVariables(t);
    t << "TAR           = " << var("QMAKE_TAR") << Qt::endl;
    t << "COMPRESS      = " << var("QMAKE_GZIP") << Qt::endl;

    if (project->isEmpty("QMAKE_DISTNAME")) {
        ProString distname = project->first("QMAKE_ORIG_TARGET");
        if (!project->isActiveConfig("no_dist_version"))
            distname += project->first("VERSION");
        project->values("QMAKE_DISTNAME") = distname;
    }
    t << "DISTNAME      = " << fileVar("QMAKE_DISTNAME") << Qt::endl;

    if (project->isEmpty("QMAKE_DISTDIR"))
        project->values("QMAKE_DISTDIR") = project->first("QMAKE_DISTNAME");
    t << "DISTDIR = " << escapeFilePath(fileFixify(
            (project->isEmpty("OBJECTS_DIR") ? ProString(".tmp/") : project->first("OBJECTS_DIR")) + project->first("QMAKE_DISTDIR"),
            FileFixifyFromOutdir | FileFixifyAbsolute)) << Qt::endl;
}

void
UnixMakefileGenerator::writeSubTargets(QTextStream &t, QList<MakefileGenerator::SubTarget*> targets, int flags)
{
    MakefileGenerator::writeSubTargets(t, targets, flags);

    t << "dist: distdir FORCE" << Qt::endl;
    t << "\t(cd `dirname $(DISTDIR)` && $(TAR) $(DISTNAME).tar $(DISTNAME) && $(COMPRESS) $(DISTNAME).tar)"
         " && $(MOVE) `dirname $(DISTDIR)`/$(DISTNAME).tar.gz . && $(DEL_FILE) -r $(DISTDIR)";
    t << Qt::endl << Qt::endl;

    t << "distdir:";
    for (int target = 0; target < targets.size(); ++target) {
        SubTarget *subtarget = targets.at(target);
        t << " " << subtarget->target << "-distdir";
    }
    t << " FORCE\n\t"
      << mkdir_p_asstring("$(DISTDIR)", false) << "\n\t"
      << "$(COPY_FILE) --parents " << fileVar("DISTFILES") << " $(DISTDIR)" << Option::dir_sep << Qt::endl << Qt::endl;

    const QString abs_source_path = project->first("QMAKE_ABSOLUTE_SOURCE_PATH").toQString();
    for (int target = 0; target < targets.size(); ++target) {
        SubTarget *subtarget = targets.at(target);
        QString in_directory = subtarget->in_directory;
        if (!in_directory.isEmpty() && !in_directory.endsWith(Option::dir_sep))
            in_directory += Option::dir_sep;
        QString out_directory = subtarget->out_directory;
        if (!out_directory.isEmpty() && !out_directory.endsWith(Option::dir_sep))
            out_directory += Option::dir_sep;
        if (!abs_source_path.isEmpty() && out_directory.startsWith(abs_source_path))
            out_directory = Option::output_dir + out_directory.mid(abs_source_path.size());

        QString dist_directory = out_directory;
        if (dist_directory.endsWith(Option::dir_sep))
            dist_directory.chop(Option::dir_sep.size());
        if (!dist_directory.startsWith(Option::dir_sep))
            dist_directory.prepend(Option::dir_sep);

        QString out_directory_cdin = out_directory.isEmpty() ? QString("\n\t")
                                                             : "\n\tcd " + escapeFilePath(out_directory) + " && ";
        QString makefilein = " -e -f " + escapeFilePath(subtarget->makefile)
                + " distdir DISTDIR=$(DISTDIR)" + escapeFilePath(dist_directory);

        QString out = subtarget->makefile;
        QString in = escapeFilePath(fileFixify(in_directory + subtarget->profile, FileFixifyAbsolute));
        if (out.startsWith(in_directory))
            out.remove(0, in_directory.size());

        t << subtarget->target << "-distdir: FORCE";
        writeSubTargetCall(t, in_directory, in, out_directory, escapeFilePath(out),
                           out_directory_cdin, makefilein);
        t << Qt::endl;
    }
}

static QString rfc1034Identifier(const QString &str)
{
    QString s = str;
    for (QChar &ch : s) {
        const char c = ch.toLatin1();

        const bool okChar = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z')
                || (c >= 'a' && c <= 'z') || c == '-' || c == '.';
        if (!okChar)
            ch = QChar::fromLatin1('-');
    }
    return s;
}

static QString escapeDir(const QString &dir)
{
    // When building on non-MSys MinGW, the path ends with a backslash, which
    // GNU make will interpret that as a line continuation. Doubling the backslash
    // avoids the problem, at the cost of the variable containing *both* backslashes.
    if (dir.endsWith('\\'))
        return dir + '\\';
    return dir;
}

void
UnixMakefileGenerator::writeMakeParts(QTextStream &t)
{
    bool do_incremental = (project->isActiveConfig("incremental") &&
                           !project->values("QMAKE_INCREMENTAL").isEmpty() &&
                           (!project->values("QMAKE_APP_FLAG").isEmpty() ||
                            (!project->isActiveConfig("staticlib")))),
         src_incremental=false;

    ProStringList &bundledFiles = project->values("QMAKE_BUNDLED_FILES");

    writeExportedVariables(t);

    t << "####### Compiler, tools and options\n\n";
    t << "CC            = " << var("QMAKE_CC") << Qt::endl;
    t << "CXX           = " << var("QMAKE_CXX") << Qt::endl;
    t << "DEFINES       = "
      << varGlue("PRL_EXPORT_DEFINES","-D"," -D"," ")
      << varGlue("DEFINES","-D"," -D","") << Qt::endl;
    t << "CFLAGS        = " << var("QMAKE_CFLAGS") << " $(DEFINES)\n";
    t << "CXXFLAGS      = " << var("QMAKE_CXXFLAGS") << " $(DEFINES)\n";
    t << "INCPATH       =";
    {
        const ProStringList &incs = project->values("INCLUDEPATH");
        for(int i = 0; i < incs.size(); ++i) {
            const ProString &inc = incs.at(i);
            if (inc.isEmpty())
                continue;

            t << " -I" << escapeFilePath(inc);
        }
    }
    if(!project->isEmpty("QMAKE_FRAMEWORKPATH_FLAGS"))
       t << " " << var("QMAKE_FRAMEWORKPATH_FLAGS");
    t << Qt::endl;

    writeDefaultVariables(t);

    if(!project->isActiveConfig("staticlib")) {
        t << "LINK          = " << var("QMAKE_LINK") << Qt::endl;
        t << "LFLAGS        = " << var("QMAKE_LFLAGS") << Qt::endl;
        t << "LIBS          = $(SUBLIBS) " << fixLibFlags("LIBS").join(' ') << ' '
                                           << fixLibFlags("LIBS_PRIVATE").join(' ') << ' '
                                           << fixLibFlags("QMAKE_LIBS").join(' ') << ' '
                                           << fixLibFlags("QMAKE_LIBS_PRIVATE").join(' ') << Qt::endl;
    }

    t << "AR            = " << var("QMAKE_AR") << Qt::endl;
    t << "RANLIB        = " << var("QMAKE_RANLIB") << Qt::endl;
    t << "SED           = " << var("QMAKE_STREAM_EDITOR") << Qt::endl;
    t << "STRIP         = " << var("QMAKE_STRIP") << Qt::endl;

    t << Qt::endl;

    t << "####### Output directory\n\n";
    // This is used in commands by some .prf files.
    if (! project->values("OBJECTS_DIR").isEmpty())
        t << "OBJECTS_DIR   = " << escapeDir(fileVar("OBJECTS_DIR")) << Qt::endl;
    else
        t << "OBJECTS_DIR   = ./\n";
    t << Qt::endl;

    /* files */
    t << "####### Files\n\n";
    // This is used by the dist target.
    t << "SOURCES       = " << fileVarList("SOURCES") << ' ' << fileVarList("GENERATED_SOURCES") << Qt::endl;

    src_incremental = writeObjectsPart(t, do_incremental);
    if(do_incremental && !src_incremental)
        do_incremental = false;
    t << "DIST          = " << valList(fileFixify(project->values("DISTFILES").toQStringList())) << " "
                            << fileVarList("HEADERS") << ' ' << fileVarList("SOURCES") << Qt::endl;
    t << "QMAKE_TARGET  = " << fileVar("QMAKE_ORIG_TARGET") << Qt::endl;
    t << "DESTDIR       = " << escapeDir(fileVar("DESTDIR")) << Qt::endl;
    t << "TARGET        = " << fileVar("TARGET") << Qt::endl;
    if(project->isActiveConfig("plugin")) {
        t << "TARGETD       = " << fileVar("TARGET") << Qt::endl;
    } else if(!project->isActiveConfig("staticlib") && project->values("QMAKE_APP_FLAG").isEmpty()) {
        t << "TARGETA       = " << fileVar("TARGETA") << Qt::endl;
        if(!project->isEmpty("QMAKE_BUNDLE")) {
            t << "TARGETD       = " << fileVar("TARGET_x.y") << Qt::endl;
            t << "TARGET0       = " << fileVar("TARGET_") << Qt::endl;
        } else if (!project->isActiveConfig("unversioned_libname")) {
            t << "TARGET0       = " << fileVar("TARGET_") << Qt::endl;
            if (project->isEmpty("QMAKE_HPUX_SHLIB")) {
                t << "TARGETD       = " << fileVar("TARGET_x.y.z") << Qt::endl;
                t << "TARGET1       = " << fileVar("TARGET_x") << Qt::endl;
                t << "TARGET2       = " << fileVar("TARGET_x.y") << Qt::endl;
            } else {
                t << "TARGETD       = " << fileVar("TARGET_x") << Qt::endl;
            }
        }
    }
    writeExtraCompilerVariables(t);
    writeExtraVariables(t);
    t << Qt::endl;

    // blasted includes
    const ProStringList &qeui = project->values("QMAKE_EXTRA_INCLUDES");
    ProStringList::ConstIterator it;
    for(it = qeui.begin(); it != qeui.end(); ++it)
        t << "include " << escapeDependencyPath(*it) << Qt::endl;

    /* rules */
    t << "first:" << (!project->isActiveConfig("no_default_goal_deps") ? " all" : "") << "\n";

    if(include_deps) {
        if (project->isActiveConfig("gcc_MD_depends")) {
            ProStringList objects = project->values("OBJECTS");
            for (ProStringList::Iterator it = objects.begin(); it != objects.end(); ++it) {
                QString d_file = (*it).toQString().replace(QRegularExpression(Option::obj_ext + "$"), ".d");
                t << "-include " << escapeDependencyPath(d_file) << Qt::endl;
                project->values("QMAKE_DISTCLEAN") << d_file;
            }
        } else {
            QString cmd=var("QMAKE_CFLAGS_DEPS") + " ";
            cmd += varGlue("DEFINES","-D"," -D","") + varGlue("PRL_EXPORT_DEFINES"," -D"," -D","");
            if(!project->isEmpty("QMAKE_ABSOLUTE_SOURCE_PATH"))
                cmd += " -I" + fileVar("QMAKE_ABSOLUTE_SOURCE_PATH") + ' ';
            cmd += " $(INCPATH) " + fileVarGlue("DEPENDPATH", "-I", " -I", "");
            ProString odir;
            if(!project->values("OBJECTS_DIR").isEmpty())
                odir = project->first("OBJECTS_DIR");
            QString odird = escapeDependencyPath(odir.toQString());

            QString pwd = escapeFilePath(fileFixify(qmake_getpwd()));

            t << "###### Dependencies\n\n";
            t << odird << ".deps/%.d: " << pwd << "/%.cpp\n\t";
            if(project->isActiveConfig("echo_depend_creation"))
                t << "@echo Creating depend for $<\n\t";
            t << mkdir_p_asstring("$(@D)", false) << "\n\t"
              << "@$(CXX) " << cmd << " $< | sed \"s,^\\($(*F).o\\):," << odir << "\\1:,g\" >$@\n\n";

            t << odird << ".deps/%.d: " << pwd << "/%.c\n\t";
            if(project->isActiveConfig("echo_depend_creation"))
                t << "@echo Creating depend for $<\n\t";
            t << mkdir_p_asstring("$(@D)", false) << "\n\t"
              << "@$(CC) " << cmd << " $< | sed \"s,^\\($(*F).o\\):," << odir << "\\1:,g\" >$@\n\n";

            static const char * const src[] = { "SOURCES", "GENERATED_SOURCES", nullptr };
            for (int x = 0; src[x]; x++) {
                const ProStringList &l = project->values(src[x]);
                for (ProStringList::ConstIterator it = l.begin(); it != l.end(); ++it) {
                    if(!(*it).isEmpty()) {
                        QString d_file;
                        for(QStringList::Iterator cit = Option::c_ext.begin();
                            cit != Option::c_ext.end(); ++cit) {
                            if((*it).endsWith((*cit))) {
                                d_file = (*it).left((*it).length() - (*cit).size()).toQString();
                                break;
                            }
                        }
                        if(d_file.isEmpty()) {
                            for(QStringList::Iterator cppit = Option::cpp_ext.begin();
                                cppit != Option::cpp_ext.end(); ++cppit) {
                                if((*it).endsWith((*cppit))) {
                                    d_file = (*it).left((*it).length() - (*cppit).size()).toQString();
                                    break;
                                }
                            }
                        }

                        if(!d_file.isEmpty()) {
                            d_file = odir + ".deps/" + fileFixify(d_file, FileFixifyBackwards) + ".d";
                            QString d_file_d = escapeDependencyPath(d_file);
                            QStringList deps = findDependencies((*it).toQString()).filter(QRegularExpression(
                                        "((^|/)" + Option::h_moc_mod + "|" + Option::cpp_moc_ext + "$)"));
                            if(!deps.isEmpty())
                                t << d_file_d << ": " << finalizeDependencyPaths(deps).join(' ') << Qt::endl;
                            t << "-include " << d_file_d << Qt::endl;
                            project->values("QMAKE_DISTCLEAN") += d_file;
                        }
                    }
                }
            }
        }
    }

    t << "####### Build rules\n\n";
    if(!project->values("SUBLIBS").isEmpty()) {
        ProString libdir = "tmp/";
        if(!project->isEmpty("SUBLIBS_DIR"))
            libdir = project->first("SUBLIBS_DIR");
        t << "SUBLIBS       = ";
        const ProStringList &l = project->values("SUBLIBS");
        for (ProStringList::ConstIterator it = l.begin(); it != l.end(); ++it)
            t << escapeFilePath(libdir + project->first("QMAKE_PREFIX_STATICLIB") + (*it) + '.'
                                + project->first("QMAKE_EXTENSION_STATICLIB")) << ' ';
        t << Qt::endl << Qt::endl;
    }
    QString target_deps;
    if ((project->isActiveConfig("depend_prl") || project->isActiveConfig("fast_depend_prl"))
        && !project->isEmpty("QMAKE_PRL_INTERNAL_FILES")) {
        const ProStringList &l = project->values("QMAKE_PRL_INTERNAL_FILES");
        ProStringList::ConstIterator it;
        for(it = l.begin(); it != l.end(); ++it) {
            QMakeMetaInfo libinfo;
            if (libinfo.readLib((*it).toQString()) && !libinfo.isEmpty("QMAKE_PRL_BUILD_DIR")) {
                ProString dir;
                int slsh = (*it).lastIndexOf(Option::dir_sep);
                if(slsh != -1)
                    dir = (*it).left(slsh + 1);
                QString targ = dir + libinfo.first("QMAKE_PRL_TARGET");
                QString targ_d = escapeDependencyPath(targ);
                target_deps += ' ' + targ_d;
                t << targ_d;
                if (project->isActiveConfig("fast_depend_prl"))
                    t << ":\n\t@echo \"Creating '";
                else
                    t << ": FORCE\n\t@echo \"Creating/updating '";
                t << targ << "'\"\n\t"
                  << "(cd " << escapeFilePath(libinfo.first("QMAKE_PRL_BUILD_DIR")) << ';'
                  << "$(MAKE))\n";
            }
        }
    }
    LinkerResponseFileInfo linkerResponseFile = maybeCreateLinkerResponseFile();
    QString deps = escapeDependencyPath(fileFixify(Option::output.fileName()));
    QString allDeps;
    if (!project->values("QMAKE_APP_FLAG").isEmpty() || project->first("TEMPLATE") == "aux") {
        QString destdir = project->first("DESTDIR").toQString();
        if(!project->isEmpty("QMAKE_BUNDLE")) {
            QString bundle_loc = project->first("QMAKE_BUNDLE_LOCATION").toQString();
            if(!bundle_loc.isEmpty() && !bundle_loc.startsWith("/"))
                bundle_loc.prepend("/");
            if(!bundle_loc.endsWith("/"))
                bundle_loc += "/";
            destdir += project->first("QMAKE_BUNDLE") + bundle_loc;
        }
        if(do_incremental) {
            //incremental target
            QString incr_target = var("TARGET") + "_incremental";
            if(incr_target.indexOf(Option::dir_sep) != -1)
                incr_target = incr_target.right(incr_target.size() -
                                                (incr_target.lastIndexOf(Option::dir_sep) + 1));
            QString incr_deps, incr_objs;
            if(project->first("QMAKE_INCREMENTAL_STYLE") == "ld") {
                QString incr_target_dir = var("OBJECTS_DIR") + incr_target + Option::obj_ext;
                QString incr_target_dir_d = escapeDependencyPath(incr_target_dir);
                QString incr_target_dir_f = escapeFilePath(incr_target_dir);
                //actual target
                t << incr_target_dir_d << ": $(OBJECTS)\n\t"
                  << "ld -r  -o " << incr_target_dir_f << " $(OBJECTS)\n";
                //communicated below
                deps.prepend(incr_target_dir_d + ' ');
                incr_deps = "$(INCREMENTAL_OBJECTS)";
                if(!incr_objs.isEmpty())
                    incr_objs += " ";
                incr_objs += incr_target_dir_f;
            } else {
                //actual target
                QString incr_target_dir = var("DESTDIR") + "lib" + incr_target + "." +
                                          project->first("QMAKE_EXTENSION_SHLIB");
                QString incr_target_dir_d = escapeDependencyPath(incr_target_dir);
                QString incr_target_dir_f = escapeFilePath(incr_target_dir);
                QString incr_lflags = var("QMAKE_LFLAGS_SHLIB") + " ";
                if(project->isActiveConfig("debug"))
                    incr_lflags += var("QMAKE_LFLAGS_DEBUG");
                else if (project->isActiveConfig("debug_info"))
                    incr_lflags += var("QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO");
                else
                    incr_lflags += var("QMAKE_LFLAGS_RELEASE");
                t << incr_target_dir_d << ": $(INCREMENTAL_OBJECTS)\n\t";
                if(!destdir.isEmpty())
                    t << "\n\t" << mkdir_p_asstring(destdir) << "\n\t";
                t << "$(LINK) " << incr_lflags << " -o "<< incr_target_dir_f <<
                    " $(INCREMENTAL_OBJECTS)\n";
                //communicated below
                if(!destdir.isEmpty()) {
                    if(!incr_objs.isEmpty())
                        incr_objs += " ";
                    incr_objs += "-L" + escapeFilePath(destdir);
                } else {
                    if(!incr_objs.isEmpty())
                        incr_objs += " ";
                    incr_objs += "-L" + escapeFilePath(qmake_getpwd());
                }
                if(!incr_objs.isEmpty())
                    incr_objs += " ";
                incr_objs += " -l" + escapeFilePath(incr_target);
                deps.prepend(incr_target_dir_d + ' ');
                incr_deps = "$(OBJECTS)";
            }

            //real target
            t << var("TARGET") << ": " << depVar("PRE_TARGETDEPS") << ' ' << incr_deps << ' ' << target_deps
              << ' ' << depVar("POST_TARGETDEPS") << "\n\t";
            if(!destdir.isEmpty())
                t << "\n\t" << mkdir_p_asstring(destdir) << "\n\t";
            if(!project->isEmpty("QMAKE_PRE_LINK"))
                t << var("QMAKE_PRE_LINK") << "\n\t";
            t << "$(LINK) $(LFLAGS) " << var("QMAKE_LINK_O_FLAG") << "$(TARGET) " << incr_deps << " " << incr_objs << " $(OBJCOMP) $(LIBS)";
            if(!project->isEmpty("QMAKE_POST_LINK"))
                t << "\n\t" << var("QMAKE_POST_LINK");
            t << Qt::endl << Qt::endl;
        } else {
            t << depVar("TARGET") << ": " << depVar("PRE_TARGETDEPS") << " $(OBJECTS) "
              << target_deps << ' ' << depVar("POST_TARGETDEPS") << "\n\t";
            if (project->first("TEMPLATE") != "aux") {
                if (!destdir.isEmpty())
                    t << mkdir_p_asstring(destdir) << "\n\t";
                if (!project->isEmpty("QMAKE_PRE_LINK"))
                    t << var("QMAKE_PRE_LINK") << "\n\t";
                t << "$(LINK) $(LFLAGS) " << var("QMAKE_LINK_O_FLAG") << "$(TARGET) ";
                if (!linkerResponseFile.isValid())
                    t << " $(OBJECTS) $(OBJCOMP) $(LIBS)";
                else if (linkerResponseFile.onlyObjects)
                    t << " @" << linkerResponseFile.filePath << " $(OBJCOMP) $(LIBS)";
                else
                    t << " $(OBJCOMP) @" << linkerResponseFile.filePath;
                if (!project->isEmpty("QMAKE_POST_LINK"))
                    t << "\n\t" << var("QMAKE_POST_LINK");
            }
            t << Qt::endl << Qt::endl;
        }
        allDeps = ' ' + depVar("TARGET");
    } else if(!project->isActiveConfig("staticlib")) {
        QString destdir_r = project->first("DESTDIR").toQString(), incr_deps;
        if(!project->isEmpty("QMAKE_BUNDLE")) {
            QString bundle_loc = project->first("QMAKE_BUNDLE_LOCATION").toQString();
            if(!bundle_loc.isEmpty() && !bundle_loc.startsWith("/"))
                bundle_loc.prepend("/");
            if(!bundle_loc.endsWith("/"))
                bundle_loc += "/";
            destdir_r += project->first("QMAKE_BUNDLE") + bundle_loc;
        }
        QString destdir_d = escapeDependencyPath(destdir_r);
        QString destdir = escapeFilePath(destdir_r);

        if(do_incremental) {
            ProString s_ext = project->first("QMAKE_EXTENSION_SHLIB");
            QString incr_target = var("QMAKE_ORIG_TARGET").replace(
                QRegularExpression("\\." + s_ext), "").replace(QRegularExpression("^lib"), "") + "_incremental";
            if(incr_target.indexOf(Option::dir_sep) != -1)
                incr_target = incr_target.right(incr_target.size() -
                                                (incr_target.lastIndexOf(Option::dir_sep) + 1));

            if(project->first("QMAKE_INCREMENTAL_STYLE") == "ld") {
                QString incr_target_dir = var("OBJECTS_DIR") + incr_target + Option::obj_ext;
                QString incr_target_dir_d = escapeDependencyPath(incr_target_dir);
                QString incr_target_dir_f = escapeFilePath(incr_target_dir);
                //actual target
                const QString link_deps = "$(OBJECTS) ";
                t << incr_target_dir_d << ": " << link_deps << "\n\t"
                  << "ld -r  -o " << incr_target_dir_f << ' ' << link_deps << Qt::endl;
                //communicated below
                ProStringList &cmd = project->values("QMAKE_LINK_SHLIB_CMD");
                cmd[0] = cmd.at(0).toQString().replace(QLatin1String("$(OBJECTS) "), QLatin1String("$(INCREMENTAL_OBJECTS)")); //ick
                cmd.append(incr_target_dir_f);
                deps.prepend(incr_target_dir_d + ' ');
                incr_deps = "$(INCREMENTAL_OBJECTS)";
            } else {
                //actual target
                QString incr_target_dir = destdir_r + "lib" + incr_target + '.' + s_ext;
                QString incr_target_dir_d = escapeDependencyPath(incr_target_dir);
                QString incr_target_dir_f = escapeFilePath(incr_target_dir);
                QString incr_lflags = var("QMAKE_LFLAGS_SHLIB") + " ";
                if(!project->isEmpty("QMAKE_LFLAGS_INCREMENTAL"))
                    incr_lflags += var("QMAKE_LFLAGS_INCREMENTAL") + " ";
                if(project->isActiveConfig("debug"))
                    incr_lflags += var("QMAKE_LFLAGS_DEBUG");
                else if (project->isActiveConfig("debug_info"))
                    incr_lflags += var("QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO");
                else
                    incr_lflags += var("QMAKE_LFLAGS_RELEASE");
                t << incr_target_dir_d << ": $(INCREMENTAL_OBJECTS)\n\t";
                if(!destdir.isEmpty())
                    t << mkdir_p_asstring(destdir, false) << "\n\t";
                t << "$(LINK) " << incr_lflags << ' ' << var("QMAKE_LINK_O_FLAG") << incr_target_dir_f <<
                    " $(INCREMENTAL_OBJECTS)\n";
                //communicated below
                ProStringList &cmd = project->values("QMAKE_LINK_SHLIB_CMD");
                if(!destdir.isEmpty())
                    cmd.append(" -L" + destdir);
                cmd.append(" -l" + escapeFilePath(incr_target));
                deps.prepend(incr_target_dir_d + ' ');
                incr_deps = "$(OBJECTS)";
            }

            //real target
            t << destdir_d << depVar("TARGET") << ": " << depVar("PRE_TARGETDEPS") << ' '
              << incr_deps << " $(SUBLIBS) " << target_deps << ' ' << depVar("POST_TARGETDEPS");
        } else {
            ProStringList &cmd = project->values("QMAKE_LINK_SHLIB_CMD");
            if (linkerResponseFile.isValid()) {
                cmd[0] = cmd.at(0).toQString().replace(QLatin1String("$(OBJECTS)"),
                                                       "@" + linkerResponseFile.filePath);
            }
            t << destdir_d << depVar("TARGET") << ": " << depVar("PRE_TARGETDEPS")
              << " $(OBJECTS) $(SUBLIBS) $(OBJCOMP) " << target_deps
              << ' ' << depVar("POST_TARGETDEPS");
        }
        allDeps = ' ' + destdir_d + depVar("TARGET");
        if(!destdir.isEmpty())
            t << "\n\t" << mkdir_p_asstring(destdir, false);
        if(!project->isEmpty("QMAKE_PRE_LINK"))
            t << "\n\t" << var("QMAKE_PRE_LINK");

        if (project->isActiveConfig("plugin")) {
            t << "\n\t"
              << "-$(DEL_FILE) $(TARGET)\n\t"
              << var("QMAKE_LINK_SHLIB_CMD");
            if(!destdir.isEmpty())
                t << "\n\t"
                  << "-$(MOVE) $(TARGET) " << destdir << "$(TARGET)";
            if(!project->isEmpty("QMAKE_POST_LINK"))
                t << "\n\t" << var("QMAKE_POST_LINK");
            t << Qt::endl << Qt::endl;
        } else if(!project->isEmpty("QMAKE_BUNDLE")) {
            bundledFiles << destdir_r + var("TARGET");
            t << "\n\t"
              << "-$(DEL_FILE) $(TARGET) $(TARGET0) $(DESTDIR)$(TARGET0)\n\t"
              << var("QMAKE_LINK_SHLIB_CMD") << "\n\t"
              << mkdir_p_asstring("\"`dirname $(DESTDIR)$(TARGETD)`\"", false) << "\n\t"
              << "-$(MOVE) $(TARGET) $(DESTDIR)$(TARGETD)\n\t"
              << mkdir_p_asstring("\"`dirname $(DESTDIR)$(TARGET0)`\"", false) << "\n\t";
            if (!project->isActiveConfig("shallow_bundle"))
                t << varGlue("QMAKE_LN_SHLIB", "-", " ",
                             " Versions/Current/$(TARGET) $(DESTDIR)$(TARGET0)") << "\n\t";
            if(!project->isEmpty("QMAKE_POST_LINK"))
                t << "\n\t" << var("QMAKE_POST_LINK");
            t << Qt::endl << Qt::endl;
        } else if(project->isEmpty("QMAKE_HPUX_SHLIB")) {
            t << "\n\t";

            if (!project->isActiveConfig("unversioned_libname"))
                t << "-$(DEL_FILE) $(TARGET) $(TARGET0) $(TARGET1) $(TARGET2)";
            else
                t << "-$(DEL_FILE) $(TARGET)";

            t << "\n\t" << var("QMAKE_LINK_SHLIB_CMD");

            if (!project->isActiveConfig("unversioned_libname")) {
                t << "\n\t"
                  << varGlue("QMAKE_LN_SHLIB","-"," "," $(TARGET) $(TARGET0)") << "\n\t"
                  << varGlue("QMAKE_LN_SHLIB","-"," "," $(TARGET) $(TARGET1)") << "\n\t"
                  << varGlue("QMAKE_LN_SHLIB","-"," "," $(TARGET) $(TARGET2)");
            }
            if (!destdir.isEmpty()) {
                t << "\n\t"
                  << "-$(DEL_FILE) " << destdir << "$(TARGET)\n\t"
                  << "-$(MOVE) $(TARGET) " << destdir << "$(TARGET)";

                if (!project->isActiveConfig("unversioned_libname")) {
                    t << "\n\t"
                      << "-$(DEL_FILE) " << destdir << "$(TARGET0)\n\t"
                      << "-$(DEL_FILE) " << destdir << "$(TARGET1)\n\t"
                      << "-$(DEL_FILE) " << destdir << "$(TARGET2)\n\t"
                      << "-$(MOVE) $(TARGET0) " << destdir << "$(TARGET0)\n\t"
                      << "-$(MOVE) $(TARGET1) " << destdir << "$(TARGET1)\n\t"
                      << "-$(MOVE) $(TARGET2) " << destdir << "$(TARGET2)";
                }
            }
            if(!project->isEmpty("QMAKE_POST_LINK"))
                t << "\n\t" << var("QMAKE_POST_LINK");
            t << Qt::endl << Qt::endl;
        } else {
            t << "\n\t"
              << "-$(DEL_FILE) $(TARGET) $(TARGET0)\n\t"
              << var("QMAKE_LINK_SHLIB_CMD") << "\n\t";
            t << varGlue("QMAKE_LN_SHLIB",""," "," $(TARGET) $(TARGET0)");
            if(!destdir.isEmpty())
                t  << "\n\t"
                   << "-$(DEL_FILE) " << destdir << "$(TARGET)\n\t"
                   << "-$(DEL_FILE) " << destdir << "$(TARGET0)\n\t"
                   << "-$(MOVE) $(TARGET) " << destdir << "$(TARGET)\n\t"
                   << "-$(MOVE) $(TARGET0) " << destdir << "$(TARGET0)\n\t";
            if(!project->isEmpty("QMAKE_POST_LINK"))
                t << "\n\t" << var("QMAKE_POST_LINK");
            t << Qt::endl << Qt::endl;
        }
        t << Qt::endl << Qt::endl;

        if (! project->isActiveConfig("plugin")) {
            t << "staticlib: " << depVar("TARGETA") << "\n\n";
            t << depVar("TARGETA") << ": " << depVar("PRE_TARGETDEPS") << " $(OBJECTS) $(OBJCOMP)";
            if(do_incremental)
                t << " $(INCREMENTAL_OBJECTS)";
            t << ' ' << depVar("POST_TARGETDEPS") << "\n\t";
            if (!project->isEmpty("QMAKE_PRE_LINK"))
                t << var("QMAKE_PRE_LINK") << "\n\t";
            t << "-$(DEL_FILE) $(TARGETA) \n\t"
              << var("QMAKE_AR_CMD");
            if(do_incremental)
                t << " $(INCREMENTAL_OBJECTS)";
            if (!project->isEmpty("QMAKE_POST_LINK"))
                t << "\n\t" << var("QMAKE_POST_LINK");
            if(!project->isEmpty("QMAKE_RANLIB"))
                t << "\n\t$(RANLIB) $(TARGETA)";
            t << Qt::endl << Qt::endl;
        }
    } else {
        QString destdir_r = project->first("DESTDIR").toQString();
        QString destdir_d = escapeDependencyPath(destdir_r);
        QString destdir = escapeFilePath(destdir_r);
        allDeps = ' ' + destdir_d + depVar("TARGET");
        t << "staticlib: " << destdir_d << "$(TARGET)\n\n"
          << destdir_d << depVar("TARGET") << ": " << depVar("PRE_TARGETDEPS")
          << " $(OBJECTS) $(OBJCOMP) " << depVar("POST_TARGETDEPS") << "\n\t";
        if (!destdir.isEmpty())
            t << mkdir_p_asstring(destdir, false) << "\n\t";
        if (!project->isEmpty("QMAKE_PRE_LINK"))
            t << var("QMAKE_PRE_LINK") << "\n\t";
        t << "-$(DEL_FILE) " << destdir << "$(TARGET)\n\t"
          << var("QMAKE_AR_CMD") << "\n";
        if (!project->isEmpty("QMAKE_POST_LINK"))
            t << "\t" << var("QMAKE_POST_LINK") << "\n";
        if (!project->isEmpty("QMAKE_RANLIB"))
            t << "\t$(RANLIB) " << destdir << "$(TARGET)\n";
        t << Qt::endl << Qt::endl;
    }

    writeMakeQmake(t);
    if(project->isEmpty("QMAKE_FAILED_REQUIREMENTS") && !project->isActiveConfig("no_autoqmake")) {
        QStringList meta_files;
        if (project->isActiveConfig("create_libtool") && project->first("TEMPLATE") == "lib") { //libtool
            meta_files += libtoolFileName();
        }
        if(project->isActiveConfig("create_pc") && project->first("TEMPLATE") == "lib") { //pkg-config
            meta_files += pkgConfigFileName();
        }
        if(!meta_files.isEmpty())
            t << escapeDependencyPaths(meta_files).join(" ") << ": \n\t"
              << "@$(QMAKE) -prl " << escapeFilePath(project->projectFile()) << ' ' << buildArgs(true) << Qt::endl;
    }

    if (!project->isEmpty("QMAKE_BUNDLE")) {
        QHash<QString, QString> symlinks;
        ProStringList &alldeps = project->values("ALL_DEPS");
        QString bundle_dir = project->first("DESTDIR") + project->first("QMAKE_BUNDLE") + "/";
        if (!project->first("QMAKE_PKGINFO").isEmpty()) {
            ProString pkginfo = project->first("QMAKE_PKGINFO");
            ProString pkginfo_f = escapeFilePath(pkginfo);
            ProString pkginfo_d = escapeDependencyPath(pkginfo);
            bundledFiles << pkginfo;
            alldeps << pkginfo;
            QString destdir = bundle_dir + "Contents";
            t << pkginfo_d << ": \n\t";
            if (!destdir.isEmpty())
                t << mkdir_p_asstring(destdir) << "\n\t";
            t << "@$(DEL_FILE) " << pkginfo_f << "\n\t"
              << "@echo \"APPL"
              << (project->isEmpty("QMAKE_PKGINFO_TYPEINFO")
                  ? QString::fromLatin1("????") : project->first("QMAKE_PKGINFO_TYPEINFO").left(4))
              << "\" > " << pkginfo_f << Qt::endl;
        }
        if (!project->first("QMAKE_BUNDLE_RESOURCE_FILE").isEmpty()) {
            ProString resources = project->first("QMAKE_BUNDLE_RESOURCE_FILE");
            ProString resources_f = escapeFilePath(resources);
            ProString resources_d = escapeDependencyPath(resources);
            bundledFiles << resources;
            alldeps << resources;
            QString destdir = bundle_dir + "Contents/Resources";
            t << resources_d << ": \n\t";
            t << mkdir_p_asstring(destdir) << "\n\t";
            t << "@touch " << resources_f << "\n\t\n";
        }
        //copy the plist
        while (!project->isActiveConfig("no_plist")) {  // 'while' just to be able to 'break'
            QString info_plist = project->first("QMAKE_INFO_PLIST").toQString();
            if (info_plist.isEmpty()) {
                info_plist = escapeFilePath(specdir() + QDir::separator() + "Info.plist." + project->first("TEMPLATE"));
            } else if (!exists(Option::normalizePath(info_plist))) {
                warn_msg(WarnLogic, "Could not resolve Info.plist: '%s'. Check if QMAKE_INFO_PLIST points to a valid file.",
                         info_plist.toLatin1().constData());
                break;
            } else {
                info_plist = escapeFilePath(fileFixify(info_plist));
            }
            bool isFramework = project->first("TEMPLATE") == "lib"
                && !project->isActiveConfig("plugin")
                && project->isActiveConfig("lib_bundle");
            bool isShallowBundle = project->isActiveConfig("shallow_bundle");
            QString info_plist_out = bundle_dir +
                (!isShallowBundle
                    ? (isFramework
                        ? ("Versions/" + project->first("QMAKE_FRAMEWORK_VERSION") + "/Resources/")
                        : QString("Contents/"))
                    : QString())
                + "Info.plist";
            bundledFiles << info_plist_out;
            alldeps << info_plist_out;
            QString destdir = info_plist_out.section(Option::dir_sep, 0, -2);
            t << escapeDependencyPath(info_plist_out) << ": \n\t";
            info_plist_out = escapeFilePath(info_plist_out);
            if (!destdir.isEmpty())
                t << mkdir_p_asstring(destdir) << "\n\t";
            ProStringList commonSedArgs;
            if (!project->values("VERSION").isEmpty()) {
                const ProString shortVersion =
                    project->first("VER_MAJ") + "." +
                    project->first("VER_MIN");
                commonSedArgs << "-e \"s,@SHORT_VERSION@," << shortVersion << ",g\" ";
                commonSedArgs << "-e \"s,\\$${QMAKE_SHORT_VERSION}," << shortVersion << ",g\" ";
                const ProString fullVersion =
                    project->first("VER_MAJ") + "." +
                    project->first("VER_MIN") + "." +
                    project->first("VER_PAT");
                commonSedArgs << "-e \"s,@FULL_VERSION@," << fullVersion << ",g\" ";
                commonSedArgs << "-e \"s,\\$${QMAKE_FULL_VERSION}," << fullVersion << ",g\" ";
            }
            const ProString typeInfo = project->isEmpty("QMAKE_PKGINFO_TYPEINFO")
                ? QString::fromLatin1("????")
                : project->first("QMAKE_PKGINFO_TYPEINFO").left(4);
            commonSedArgs << "-e \"s,@TYPEINFO@," << typeInfo << ",g\" ";
            commonSedArgs << "-e \"s,\\$${QMAKE_PKGINFO_TYPEINFO}," << typeInfo << ",g\" ";

            QString bundlePrefix = project->first("QMAKE_TARGET_BUNDLE_PREFIX").toQString();
            if (bundlePrefix.isEmpty())
                bundlePrefix = "com.yourcompany";
            if (bundlePrefix.endsWith("."))
                bundlePrefix.chop(1);
            QString bundleIdentifier =  bundlePrefix + "." + var("QMAKE_BUNDLE");
            if (bundleIdentifier.endsWith(".app"))
                bundleIdentifier.chop(4);
            if (bundleIdentifier.endsWith(".framework"))
                bundleIdentifier.chop(10);
            // replace invalid bundle id characters
            bundleIdentifier = rfc1034Identifier(bundleIdentifier);
            commonSedArgs << "-e \"s,@BUNDLEIDENTIFIER@," << bundleIdentifier << ",g\" ";
            commonSedArgs << "-e \"s,\\$${PRODUCT_BUNDLE_IDENTIFIER}," << bundleIdentifier << ",g\" ";

            commonSedArgs << "-e \"s,\\$${MACOSX_DEPLOYMENT_TARGET},"
                          << project->first("QMAKE_MACOSX_DEPLOYMENT_TARGET").toQString() << ",g\" ";
            commonSedArgs << "-e \"s,\\$${IPHONEOS_DEPLOYMENT_TARGET},"
                          << project->first("QMAKE_IPHONEOS_DEPLOYMENT_TARGET").toQString() << ",g\" ";
            commonSedArgs << "-e \"s,\\$${TVOS_DEPLOYMENT_TARGET},"
                          << project->first("QMAKE_TVOS_DEPLOYMENT_TARGET").toQString() << ",g\" ";
            commonSedArgs << "-e \"s,\\$${WATCHOS_DEPLOYMENT_TARGET},"
                          << project->first("QMAKE_WATCHOS_DEPLOYMENT_TARGET").toQString() << ",g\" ";

            QString launchScreen = var("QMAKE_IOS_LAUNCH_SCREEN");
            if (launchScreen.isEmpty())
                launchScreen = QLatin1String("LaunchScreen");
            else
                launchScreen = QFileInfo(launchScreen).baseName();
            commonSedArgs << "-e \"s,\\$${IOS_LAUNCH_SCREEN}," << launchScreen << ",g\" ";

            if (!isFramework) {
                ProString app_bundle_name = var("QMAKE_APPLICATION_BUNDLE_NAME");
                if (app_bundle_name.isEmpty())
                    app_bundle_name = var("QMAKE_ORIG_TARGET");

                ProString plugin_bundle_name = var("QMAKE_PLUGIN_BUNDLE_NAME");
                if (plugin_bundle_name.isEmpty())
                    plugin_bundle_name = var("QMAKE_ORIG_TARGET");

                QString icon = fileFixify(var("ICON"));
                t << "@$(DEL_FILE) " << info_plist_out << "\n\t"
                  << "@plutil -convert xml1 -o - " << info_plist << " | "
                  << "sed ";
                for (const ProString &arg : std::as_const(commonSedArgs))
                    t << arg;
                const QString iconName = icon.section(Option::dir_sep, -1);
                t << "-e \"s,@ICON@," << iconName << ",g\" "
                  << "-e \"s,\\$${ASSETCATALOG_COMPILER_APPICON_NAME}," << iconName << ",g\" "
                  << "-e \"s,@EXECUTABLE@," << app_bundle_name << ",g\" "
                  << "-e \"s,@LIBRARY@," << plugin_bundle_name << ",g\" "
                  << "-e \"s,\\$${EXECUTABLE_NAME}," << (!app_bundle_name.isEmpty() ? app_bundle_name : plugin_bundle_name) << ",g\" "
                  << "-e \"s,@TYPEINFO@,"<< typeInfo << ",g\" "
                  << "-e \"s,\\$${QMAKE_PKGINFO_TYPEINFO},"<< typeInfo << ",g\" "
                  << ">" << info_plist_out << Qt::endl;
                //copy the icon
                if (!project->isEmpty("ICON")) {
                    QString dir = bundle_dir + "Contents/Resources/";
                    const QString icon_path = dir + icon.section(Option::dir_sep, -1);
                    QString icon_path_f = escapeFilePath(icon_path);
                    bundledFiles << icon_path;
                    alldeps << icon_path;
                    t << escapeDependencyPath(icon_path) << ": " << escapeDependencyPath(icon) << "\n\t"
                      << mkdir_p_asstring(dir) << "\n\t"
                      << "@$(DEL_FILE) " << icon_path_f << "\n\t"
                      << "@$(COPY_FILE) " << escapeFilePath(icon) << ' ' << icon_path_f << Qt::endl;
                }
            } else {
                ProString lib_bundle_name = var("QMAKE_FRAMEWORK_BUNDLE_NAME");
                if (lib_bundle_name.isEmpty())
                    lib_bundle_name = var("QMAKE_ORIG_TARGET");

                if (!isShallowBundle)
                    symlinks[bundle_dir + "Resources"] = "Versions/Current/Resources";
                t << "@$(DEL_FILE) " << info_plist_out << "\n\t"
                  << "@plutil -convert xml1 -o - " << info_plist << " | "
                  << "sed ";
                for (const ProString &arg : std::as_const(commonSedArgs))
                    t << arg;
                t << "-e \"s,@LIBRARY@," << lib_bundle_name << ",g\" "
                  << "-e \"s,\\$${EXECUTABLE_NAME}," << lib_bundle_name << ",g\" "
                  << "-e \"s,@TYPEINFO@," << typeInfo << ",g\" "
                  << "-e \"s,\\$${QMAKE_PKGINFO_TYPEINFO}," << typeInfo << ",g\" "
                  << ">" << info_plist_out << Qt::endl;
            }
            break;
        } // project->isActiveConfig("no_plist")
        //copy other data
        if(!project->isEmpty("QMAKE_BUNDLE_DATA")) {
            const ProStringList &bundle_data = project->values("QMAKE_BUNDLE_DATA");
            for(int i = 0; i < bundle_data.size(); i++) {
                const ProStringList &files = project->values(ProKey(bundle_data[i] + ".files"));
                QString path = bundle_dir;
                const ProKey pkey(bundle_data[i] + ".path");
                if (!project->isActiveConfig("shallow_bundle")) {
                    const ProKey vkey(bundle_data[i] + ".version");
                    if (!project->isEmpty(vkey)) {
                        QString version = project->first(vkey) + "/" +
                                          project->first("QMAKE_FRAMEWORK_VERSION") + "/";
                        ProString name = project->first(pkey);
                        int pos = name.indexOf('/');
                        if (pos > 0)
                            name = name.mid(0, pos);
                        symlinks[Option::fixPathToTargetOS(path + name)] =
                                project->first(vkey) + "/Current/" + name;
                        path += version;
                    }
                }
                path += project->first(pkey).toQString();
                path = Option::fixPathToTargetOS(path);
                for(int file = 0; file < files.size(); file++) {
                    QString fn = files.at(file).toQString();
                    QString src = fileFixify(fn, FileFixifyAbsolute);
                    if (!QFile::exists(src))
                        src = fileFixify(fn, FileFixifyFromOutdir);
                    else
                        src = fileFixify(fn);
                    QString dst = path + Option::dir_sep + fileInfo(fn).fileName();
                    bundledFiles << dst;
                    alldeps << dst;
                    t << escapeDependencyPath(dst) << ": " << escapeDependencyPath(src) << "\n\t"
                      << mkdir_p_asstring(path) << "\n\t";
                    src = escapeFilePath(src);
                    dst = escapeFilePath(dst);
                    QFileInfo fi(fileInfo(fn));
                    if(fi.isDir())
                        t << "@$(DEL_FILE) -r " << dst << "\n\t"
                          << "@$(COPY_DIR) " << src << " " << dst << Qt::endl;
                    else
                        t << "@$(DEL_FILE) " << dst << "\n\t"
                          << "@$(COPY_FILE) " << src << " " << dst << Qt::endl;
                }
            }
        }
        if (!symlinks.isEmpty()) {
            QString bundle_dir_f = escapeFilePath(bundle_dir);
            QHash<QString, QString>::ConstIterator symIt = symlinks.constBegin(),
                                                   symEnd = symlinks.constEnd();
            for (; symIt != symEnd; ++symIt) {
                bundledFiles << symIt.key();
                alldeps << symIt.key();
                t << escapeDependencyPath(symIt.key()) << ":\n\t"
                  << mkdir_p_asstring(bundle_dir) << "\n\t"
                  << "@$(SYMLINK) " << escapeFilePath(symIt.value()) << ' ' << bundle_dir_f << Qt::endl;
            }

            if (!project->isActiveConfig("shallow_bundle")) {
                QString currentLink = bundle_dir + "Versions/Current";
                QString currentLink_f = escapeDependencyPath(currentLink);
                bundledFiles << currentLink;
                alldeps << currentLink;
                t << currentLink_f << ": $(MAKEFILE)\n\t"
                  << mkdir_p_asstring(bundle_dir + "Versions") << "\n\t"
                  << "@-$(DEL_FILE) " << currentLink_f << "\n\t"
                  << "@$(SYMLINK) " << project->first("QMAKE_FRAMEWORK_VERSION")
                             << ' ' << currentLink_f << Qt::endl;
            }
        }
    }

    t << Qt::endl << "all: " << deps
      << valGlue(escapeDependencyPaths(project->values("ALL_DEPS")), " \\\n\t\t", " \\\n\t\t", "")
      << allDeps << Qt::endl << Qt::endl;

    t << "dist: distdir FORCE\n\t";
    t << "(cd `dirname $(DISTDIR)` && $(TAR) $(DISTNAME).tar $(DISTNAME) && $(COMPRESS) $(DISTNAME).tar)"
         " && $(MOVE) `dirname $(DISTDIR)`" << Option::dir_sep << "$(DISTNAME).tar.gz ."
         " && $(DEL_FILE) -r $(DISTDIR)";
    t << Qt::endl << Qt::endl;

    t << "distdir: FORCE\n\t"
      << mkdir_p_asstring("$(DISTDIR)", false) << "\n\t"
      << "$(COPY_FILE) --parents $(DIST) $(DISTDIR)" << Option::dir_sep << Qt::endl;
    if(!project->isEmpty("QMAKE_EXTRA_COMPILERS")) {
        const ProStringList &quc = project->values("QMAKE_EXTRA_COMPILERS");
        for (ProStringList::ConstIterator it = quc.begin(); it != quc.end(); ++it) {
            const ProStringList &var = project->values(ProKey(*it + ".input"));
            for (ProStringList::ConstIterator var_it = var.begin(); var_it != var.end(); ++var_it) {
                const ProStringList &val = project->values((*var_it).toKey());
                if(val.isEmpty())
                    continue;
                t << "\t$(COPY_FILE) --parents " << escapeFilePaths(val).join(' ')
                                                 << " $(DISTDIR)" << Option::dir_sep << Qt::endl;
            }
        }
    }
    if(!project->isEmpty("TRANSLATIONS"))
        t << "\t$(COPY_FILE) --parents " << fileVar("TRANSLATIONS") << " $(DISTDIR)" << Option::dir_sep << Qt::endl;
    t << Qt::endl << Qt::endl;

    QString clean_targets = " compiler_clean " + depVar("CLEAN_DEPS");
    if(do_incremental) {
        t << "incrclean:\n";
        if(src_incremental)
            t << "\t-$(DEL_FILE) $(INCREMENTAL_OBJECTS)\n";
        t << Qt::endl;
    }

    t << "clean:" << clean_targets << "\n\t";
    if(!project->isEmpty("OBJECTS")) {
        t << "-$(DEL_FILE) $(OBJECTS)\n\t";
    }
    if(doPrecompiledHeaders() && !project->isEmpty("PRECOMPILED_HEADER")) {
        ProStringList precomp_files;
        ProString precomph_out_dir;

        if(!project->isEmpty("PRECOMPILED_DIR"))
            precomph_out_dir = project->first("PRECOMPILED_DIR");
        precomph_out_dir += project->first("QMAKE_ORIG_TARGET");
        precomph_out_dir += project->first("QMAKE_PCH_OUTPUT_EXT");

        if (project->isActiveConfig("icc_pch_style")) {
            // icc style
            ProString pchBaseName = project->first("QMAKE_ORIG_TARGET");
            ProStringList pchArchs = project->values("QMAKE_PCH_ARCHS");
            if (pchArchs.isEmpty())
                pchArchs << ProString(); // normal single-arch PCH
            for (const ProString &arch : std::as_const(pchArchs)) {
                ProString pchOutput;
                if (!project->isEmpty("PRECOMPILED_DIR"))
                    pchOutput = project->first("PRECOMPILED_DIR");
                pchOutput += pchBaseName + project->first("QMAKE_PCH_OUTPUT_EXT");
                if (!arch.isEmpty())
                    pchOutput = ProString(pchOutput.toQString().replace(
                        QStringLiteral("${QMAKE_PCH_ARCH}"), arch.toQString()));

                ProString sourceFile = pchOutput + Option::cpp_ext.first();
                ProString objectFile = createObjectList(ProStringList(sourceFile)).first();
                precomp_files << precomph_out_dir << sourceFile << objectFile;
            }
        } else {
            // gcc style (including clang_pch_style)
            precomph_out_dir += Option::dir_sep;

            ProString header_prefix = project->first("QMAKE_PRECOMP_PREFIX");
            ProString header_suffix = project->isActiveConfig("clang_pch_style")
                               ? project->first("QMAKE_PCH_OUTPUT_EXT") : "";

            for (const ProString &compiler : project->values("QMAKE_BUILTIN_COMPILERS")) {
                if (project->isEmpty(ProKey("QMAKE_" + compiler + "FLAGS_PRECOMPILE")))
                    continue;
                ProString language = project->first(ProKey("QMAKE_LANGUAGE_" + compiler));
                if (language.isEmpty())
                    continue;

                ProStringList pchArchs = project->values("QMAKE_PCH_ARCHS");
                if (pchArchs.isEmpty())
                    pchArchs << ProString(); // normal single-arch PCH
                for (const ProString &arch : std::as_const(pchArchs)) {
                    QString file = precomph_out_dir + header_prefix + language + header_suffix;
                    if (!arch.isEmpty())
                        file.replace(QStringLiteral("${QMAKE_PCH_ARCH}"), arch.toQString());
                    precomp_files += file;
                }
            }
        }
        t << "-$(DEL_FILE) " << escapeFilePaths(precomp_files).join(' ') << "\n\t";
    }
    if(src_incremental)
        t << "-$(DEL_FILE) $(INCREMENTAL_OBJECTS)\n\t";
    t << fileVarGlue("QMAKE_CLEAN","-$(DEL_FILE) "," ","\n\t")
      << "-$(DEL_FILE) *~ core *.core\n"
      << fileVarGlue("CLEAN_FILES","\t-$(DEL_FILE) "," ","") << Qt::endl << Qt::endl;

    ProString destdir = project->first("DESTDIR");
    if (!destdir.isEmpty() && !destdir.endsWith(Option::dir_sep))
        destdir += Option::dir_sep;
    t << "distclean: clean " << depVar("DISTCLEAN_DEPS") << '\n';
    if(!project->isEmpty("QMAKE_BUNDLE")) {
        QString bundlePath = escapeFilePath(destdir + project->first("QMAKE_BUNDLE"));
        t << "\t-$(DEL_FILE) -r " << bundlePath << Qt::endl;
    } else if (project->isActiveConfig("staticlib") || project->isActiveConfig("plugin")) {
        t << "\t-$(DEL_FILE) " << escapeFilePath(destdir) << "$(TARGET) \n";
    } else if (project->values("QMAKE_APP_FLAG").isEmpty()) {
        destdir = escapeFilePath(destdir);
        t << "\t-$(DEL_FILE) " << destdir << "$(TARGET) \n";
        if (!project->isActiveConfig("unversioned_libname")) {
            t << "\t-$(DEL_FILE) " << destdir << "$(TARGET0) " << destdir << "$(TARGET1) "
              << destdir << "$(TARGET2) $(TARGETA)\n";
        } else {
            t << "\t-$(DEL_FILE) $(TARGETA)\n";
        }
    } else {
        t << "\t-$(DEL_FILE) $(TARGET) \n";
    }
    t << fileVarGlue("QMAKE_DISTCLEAN","\t-$(DEL_FILE) "," ","\n");
    {
        QString ofile = fileFixify(Option::output.fileName());
        if(!ofile.isEmpty())
            t << "\t-$(DEL_FILE) " << escapeFilePath(ofile) << Qt::endl;
    }
    t << Qt::endl << Qt::endl;

    t << "####### Sub-libraries\n\n";
    if (!project->values("SUBLIBS").isEmpty()) {
        ProString libdir = "tmp/";
        if (!project->isEmpty("SUBLIBS_DIR"))
            libdir = project->first("SUBLIBS_DIR");
        const ProStringList &l = project->values("SUBLIBS");
        for (it = l.begin(); it != l.end(); ++it)
            t << escapeDependencyPath(libdir + project->first("QMAKE_PREFIX_STATICLIB") + (*it) + '.'
                                      + project->first("QMAKE_EXTENSION_STATICLIB")) << ":\n\t"
              << var(ProKey("MAKELIB" + *it)) << Qt::endl << Qt::endl;
    }

    if(doPrecompiledHeaders() && !project->isEmpty("PRECOMPILED_HEADER")) {
        QString pchInput = project->first("PRECOMPILED_HEADER").toQString();
        t << "###### Precompiled headers\n";
        for (const ProString &compiler : project->values("QMAKE_BUILTIN_COMPILERS")) {
            QString pchOutputDir;
            QString pchFlags = var(ProKey("QMAKE_" + compiler + "FLAGS_PRECOMPILE"));
            if(pchFlags.isEmpty())
                continue;

            QString cflags;
            if (compiler == "C" || compiler == "OBJC")
                cflags += " $(CFLAGS)";
            else
                cflags += " $(CXXFLAGS)";

            ProStringList pchArchs = project->values("QMAKE_PCH_ARCHS");
            if (pchArchs.isEmpty())
                pchArchs << ProString(); // normal single-arch PCH
            ProString pchBaseName = project->first("QMAKE_ORIG_TARGET");
            ProString pchOutput;
            if(!project->isEmpty("PRECOMPILED_DIR"))
                pchOutput = project->first("PRECOMPILED_DIR");
            pchOutput += pchBaseName;
            pchOutput += project->first("QMAKE_PCH_OUTPUT_EXT");

            if (!project->isActiveConfig("icc_pch_style")) {
                // gcc style (including clang_pch_style)
                ProString header_prefix = project->first("QMAKE_PRECOMP_PREFIX");
                ProString header_suffix = project->isActiveConfig("clang_pch_style")
                                  ? project->first("QMAKE_PCH_OUTPUT_EXT") : "";
                pchOutput += Option::dir_sep;
                pchOutputDir = pchOutput.toQString();

                QString language = project->first(ProKey("QMAKE_LANGUAGE_" + compiler)).toQString();
                if (language.isEmpty())
                    continue;

                pchOutput += header_prefix + language + header_suffix;
            }
            pchFlags.replace(QLatin1String("${QMAKE_PCH_INPUT}"), escapeFilePath(pchInput))
                    .replace(QLatin1String("${QMAKE_PCH_OUTPUT_BASE}"), escapeFilePath(pchBaseName.toQString()));
            for (const ProString &arch : std::as_const(pchArchs)) {
                auto pchArchOutput = pchOutput.toQString();
                if (!arch.isEmpty())
                    pchArchOutput.replace(QStringLiteral("${QMAKE_PCH_ARCH}"), arch.toQString());

                const auto pchFilePath_d = escapeDependencyPath(pchArchOutput);
                if (!arch.isEmpty()) {
                    t << pchFilePath_d << ": " << "EXPORT_ARCH_ARGS = -arch " << arch << "\n\n";
                    t << pchFilePath_d << ": "
                      << "EXPORT_QMAKE_XARCH_CFLAGS = $(EXPORT_QMAKE_XARCH_CFLAGS_" << arch << ")" << "\n\n";
                    t << pchFilePath_d << ": "
                      << "EXPORT_QMAKE_XARCH_LFLAGS = $(EXPORT_QMAKE_XARCH_LFLAGS_" << arch << ")" << "\n\n";
                }
                t << pchFilePath_d << ": " << escapeDependencyPath(pchInput) << ' '
                  << finalizeDependencyPaths(findDependencies(pchInput)).join(" \\\n\t\t");
                if (project->isActiveConfig("icc_pch_style")) {
                    QString sourceFile = pchArchOutput + Option::cpp_ext.first();
                    QString sourceFile_f = escapeFilePath(sourceFile);
                    QString objectFile = createObjectList(ProStringList(sourceFile)).first().toQString();

                    pchFlags.replace(QLatin1String("${QMAKE_PCH_TEMP_SOURCE}"), sourceFile_f)
                            .replace(QLatin1String("${QMAKE_PCH_TEMP_OBJECT}"), escapeFilePath(objectFile));

                    t << "\n\techo \"// Automatically generated, do not modify\" > " << sourceFile_f
                      << "\n\trm -f " << escapeFilePath(pchArchOutput);
                } else {
                    QString outDir = pchOutputDir;
                    if (!arch.isEmpty())
                        outDir.replace(QStringLiteral("${QMAKE_PCH_ARCH}"), arch.toQString());
                    t << "\n\t" << mkdir_p_asstring(outDir);
                }

                auto pchArchFlags = pchFlags;
                pchArchFlags.replace(QLatin1String("${QMAKE_PCH_OUTPUT}"), escapeFilePath(pchArchOutput));

                QString compilerExecutable;
                if (compiler == "C" || compiler == "OBJC")
                    compilerExecutable = "$(CC)";
                else
                    compilerExecutable = "$(CXX)";

                // compile command
                t << "\n\t" << compilerExecutable << cflags << " $(INCPATH) " << pchArchFlags << Qt::endl << Qt::endl;
            }
        }
    }

    writeExtraTargets(t);
    writeExtraCompilerTargets(t);
}

void UnixMakefileGenerator::init2()
{
    if(project->isEmpty("QMAKE_FRAMEWORK_VERSION"))
        project->values("QMAKE_FRAMEWORK_VERSION").append(project->first("VER_MAJ"));

    if (project->first("TEMPLATE") == "aux") {
        project->values("PRL_TARGET") = {
            project->first("QMAKE_PREFIX_STATICLIB") +
            project->first("TARGET")
        };
    } else if (!project->values("QMAKE_APP_FLAG").isEmpty()) {
        if(!project->isEmpty("QMAKE_BUNDLE")) {
            ProString bundle_loc = project->first("QMAKE_BUNDLE_LOCATION");
            if(!bundle_loc.isEmpty() && !bundle_loc.startsWith("/"))
                bundle_loc.prepend("/");
            if(!bundle_loc.endsWith("/"))
                bundle_loc += "/";
            project->values("TARGET").first().prepend(project->first("QMAKE_BUNDLE") + bundle_loc);
        }
        if(!project->isEmpty("TARGET"))
            project->values("TARGET").first().prepend(project->first("DESTDIR"));
    } else if (project->isActiveConfig("staticlib")) {
        project->values("PRL_TARGET") =
            project->values("TARGET").first().prepend(project->first("QMAKE_PREFIX_STATICLIB"));
        project->values("TARGET").first() += "." + project->first("QMAKE_EXTENSION_STATICLIB");
        if(project->values("QMAKE_AR_CMD").isEmpty())
            project->values("QMAKE_AR_CMD").append("$(AR) $(DESTDIR)$(TARGET) $(OBJECTS)");
    } else {
        project->values("TARGETA").append(project->first("DESTDIR") + project->first("QMAKE_PREFIX_STATICLIB")
                + project->first("TARGET") + "." + project->first("QMAKE_EXTENSION_STATICLIB"));

        ProStringList &ar_cmd = project->values("QMAKE_AR_CMD");
        if (!ar_cmd.isEmpty())
            ar_cmd[0] = ar_cmd.at(0).toQString().replace(QLatin1String("(TARGET)"), QLatin1String("(TARGETA)"));
        else
            ar_cmd.append("$(AR) $(TARGETA) $(OBJECTS)");
        if (!project->isEmpty("QMAKE_BUNDLE")) {
            project->values("PRL_TARGET").prepend(project->first("QMAKE_BUNDLE") +
                "/Versions/" + project->first("QMAKE_FRAMEWORK_VERSION") +
                "/Resources/" + project->first("TARGET"));
            ProString bundle_loc = project->first("QMAKE_BUNDLE_LOCATION");
            if(!bundle_loc.isEmpty() && !bundle_loc.startsWith("/"))
                bundle_loc.prepend("/");
            if(!bundle_loc.endsWith("/"))
                bundle_loc += "/";
            const QString target = project->first("QMAKE_BUNDLE") +
                                        bundle_loc + project->first("TARGET");
            project->values("TARGET_").append(target);
            if (!project->isActiveConfig("shallow_bundle")) {
                project->values("TARGET_x.y").append(project->first("QMAKE_BUNDLE") +
                                                          "/Versions/" +
                                                          project->first("QMAKE_FRAMEWORK_VERSION") +
                                                          bundle_loc + project->first("TARGET"));
            } else {
                project->values("TARGET_x.y").append(target);
            }
        } else if(project->isActiveConfig("plugin")) {
            QString prefix;
            if(!project->isActiveConfig("no_plugin_name_prefix"))
                prefix = "lib";
            project->values("PRL_TARGET").prepend(prefix + project->first("TARGET"));
            project->values("TARGET_x.y.z").append(prefix +
                                                        project->first("TARGET") + "." +
                                                        project->first("QMAKE_EXTENSION_PLUGIN"));
            if(project->isActiveConfig("lib_version_first"))
                project->values("TARGET_x").append(prefix + project->first("TARGET") + "." +
                                                        project->first("VER_MAJ") + "." +
                                                        project->first("QMAKE_EXTENSION_PLUGIN"));
            else
                project->values("TARGET_x").append(prefix + project->first("TARGET") + "." +
                                                        project->first("QMAKE_EXTENSION_PLUGIN") +
                                                        "." + project->first("VER_MAJ"));
            project->values("TARGET") = project->values("TARGET_x.y.z");
        } else if (!project->isEmpty("QMAKE_HPUX_SHLIB")) {
            project->values("PRL_TARGET").prepend("lib" + project->first("TARGET"));
            project->values("TARGET_").append("lib" + project->first("TARGET") + ".sl");
            if(project->isActiveConfig("lib_version_first"))
                project->values("TARGET_x").append("lib" + project->first("VER_MAJ") + "." +
                                                        project->first("TARGET"));
            else
                project->values("TARGET_x").append("lib" + project->first("TARGET") + "." +
                                                        project->first("VER_MAJ"));
            project->values("TARGET") = project->values("TARGET_x");
        } else if (!project->isEmpty("QMAKE_AIX_SHLIB")) {
            project->values("PRL_TARGET").prepend("lib" + project->first("TARGET"));
            project->values("TARGET_").append(project->first("QMAKE_PREFIX_STATICLIB") + project->first("TARGET")
                    + "." + project->first("QMAKE_EXTENSION_STATICLIB"));
            if(project->isActiveConfig("lib_version_first")) {
                project->values("TARGET_x").append("lib" + project->first("TARGET") + "." +
                                                        project->first("VER_MAJ") + "." +
                                                        project->first("QMAKE_EXTENSION_SHLIB"));
                project->values("TARGET_x.y").append("lib" + project->first("TARGET") + "." +
                                                          project->first("VER_MAJ") +
                                                          "." + project->first("VER_MIN") + "." +
                                                          project->first("QMAKE_EXTENSION_SHLIB"));
                project->values("TARGET_x.y.z").append("lib" + project->first("TARGET") + "." +
                                                            project->first("VER_MAJ") + "." +
                                                            project->first("VER_MIN") + "." +
                                                            project->first("VER_PAT") + "." +
                                                            project->first("QMAKE_EXTENSION_SHLIB"));
            } else {
                project->values("TARGET_x").append("lib" + project->first("TARGET") + "." +
                                                        project->first("QMAKE_EXTENSION_SHLIB") +
                                                        "." + project->first("VER_MAJ"));
                project->values("TARGET_x.y").append("lib" + project->first("TARGET") + "." +
                                                          project->first("QMAKE_EXTENSION_SHLIB") +
                                                          "." + project->first("VER_MAJ") +
                                                          "." + project->first("VER_MIN"));
                project->values("TARGET_x.y.z").append("lib" + project->first("TARGET") + "." +
                                                            project->first("QMAKE_EXTENSION_SHLIB") + "." +
                                                            project->first("VER_MAJ") + "." +
                                                            project->first("VER_MIN") + "." +
                                                            project->first("VER_PAT"));
            }
            project->values("TARGET") = project->values("TARGET_x.y.z");
        } else {
            project->values("PRL_TARGET").prepend("lib" + project->first("TARGET"));
            project->values("TARGET_").append("lib" + project->first("TARGET") + "." +
                                                   project->first("QMAKE_EXTENSION_SHLIB"));
            if(project->isActiveConfig("lib_version_first")) {
                project->values("TARGET_x").append("lib" + project->first("TARGET") + "." +
                                                        project->first("VER_MAJ") + "." +
                                                        project->first("QMAKE_EXTENSION_SHLIB"));
                project->values("TARGET_x.y").append("lib" + project->first("TARGET") + "." +
                                                          project->first("VER_MAJ") +
                                                          "." + project->first("VER_MIN") + "." +
                                                          project->first("QMAKE_EXTENSION_SHLIB"));
                project->values("TARGET_x.y.z").append("lib" + project->first("TARGET") + "." +
                                                            project->first("VER_MAJ") + "." +
                                                            project->first("VER_MIN") +  "." +
                                                            project->first("VER_PAT") + "." +
                                                            project->first("QMAKE_EXTENSION_SHLIB"));
            } else {
                project->values("TARGET_x").append("lib" + project->first("TARGET") + "." +
                                                        project->first("QMAKE_EXTENSION_SHLIB") +
                                                        "." + project->first("VER_MAJ"));
                project->values("TARGET_x.y").append("lib" + project->first("TARGET") + "." +
                                                      project->first("QMAKE_EXTENSION_SHLIB")
                                                      + "." + project->first("VER_MAJ") +
                                                      "." + project->first("VER_MIN"));
                project->values("TARGET_x.y.z").append("lib" + project->first("TARGET") +
                                                            "." +
                                                            project->first(
                                                                "QMAKE_EXTENSION_SHLIB") + "." +
                                                            project->first("VER_MAJ") + "." +
                                                            project->first("VER_MIN") +  "." +
                                                            project->first("VER_PAT"));
            }
            if (project->isActiveConfig("unversioned_libname"))
                project->values("TARGET") = project->values("TARGET_");
            else
                project->values("TARGET") = project->values("TARGET_x.y.z");
        }
        if (!project->values("QMAKE_LFLAGS_SONAME").isEmpty()) {
            ProString soname;
            if(project->isActiveConfig("plugin")) {
                if(!project->values("TARGET").isEmpty())
                    soname += project->first("TARGET");
            } else if(!project->isEmpty("QMAKE_BUNDLE")) {
                soname += project->first("TARGET_x.y");
            } else if(project->isActiveConfig("unversioned_soname")) {
                soname = "lib" + project->first("QMAKE_ORIG_TARGET")
                    + "." + project->first("QMAKE_EXTENSION_SHLIB");
            } else if(!project->values("TARGET_x").isEmpty()) {
                soname += project->first("TARGET_x");
            }
            if(!soname.isEmpty()) {
                if(project->isActiveConfig("absolute_library_soname") &&
                   project->values("INSTALLS").indexOf("target") != -1 &&
                   !project->isEmpty("target.path")) {
                    QString instpath = Option::fixPathToTargetOS(project->first("target.path").toQString());
                    if(!instpath.endsWith(Option::dir_sep))
                        instpath += Option::dir_sep;
                    soname.prepend(instpath);
                } else if (!project->isEmpty("QMAKE_SONAME_PREFIX")) {
                    QString sonameprefix = project->first("QMAKE_SONAME_PREFIX").toQString();
                    if (!sonameprefix.startsWith('@'))
                        sonameprefix = Option::fixPathToTargetOS(sonameprefix, false);
                    if (!sonameprefix.endsWith(Option::dir_sep))
                        sonameprefix += Option::dir_sep;
                    soname.prepend(sonameprefix);
                }
                project->values("QMAKE_LFLAGS_SONAME").first() += escapeFilePath(soname);
            }
        }
        if (project->values("QMAKE_LINK_SHLIB_CMD").isEmpty())
            project->values("QMAKE_LINK_SHLIB_CMD").append(
                "$(LINK) $(LFLAGS) " + project->first("QMAKE_LINK_O_FLAG") + "$(TARGET) $(OBJECTS) $(LIBS) $(OBJCOMP)");
    }
    if (!project->values("QMAKE_APP_FLAG").isEmpty() || project->first("TEMPLATE") == "aux") {
        project->values("QMAKE_CFLAGS") += project->values("QMAKE_CFLAGS_APP");
        project->values("QMAKE_CXXFLAGS") += project->values("QMAKE_CXXFLAGS_APP");
        project->values("QMAKE_LFLAGS") += project->values("QMAKE_LFLAGS_APP");
    } else if (project->isActiveConfig("dll")) {
        if(!project->isActiveConfig("plugin") || !project->isActiveConfig("plugin_no_share_shlib_cflags")) {
            project->values("QMAKE_CFLAGS") += project->values("QMAKE_CFLAGS_SHLIB");
            project->values("QMAKE_CXXFLAGS") += project->values("QMAKE_CXXFLAGS_SHLIB");
        }
        if (project->isActiveConfig("plugin")) {
            project->values("QMAKE_CFLAGS") += project->values("QMAKE_CFLAGS_PLUGIN");
            project->values("QMAKE_CXXFLAGS") += project->values("QMAKE_CXXFLAGS_PLUGIN");
            project->values("QMAKE_LFLAGS") += project->values("QMAKE_LFLAGS_PLUGIN");
            if (project->isActiveConfig("plugin_with_soname"))
                project->values("QMAKE_LFLAGS") += project->values("QMAKE_LFLAGS_SONAME");
        } else {
            project->values("QMAKE_LFLAGS") += project->values("QMAKE_LFLAGS_SHLIB");
            if(!project->isEmpty("QMAKE_LFLAGS_COMPAT_VERSION")) {
                if(project->isEmpty("COMPAT_VERSION"))
                    project->values("QMAKE_LFLAGS") += QString(project->first("QMAKE_LFLAGS_COMPAT_VERSION") +
                                                                    project->first("VER_MAJ") + "." +
                                                                    project->first("VER_MIN"));
                else
                    project->values("QMAKE_LFLAGS") += QString(project->first("QMAKE_LFLAGS_COMPAT_VERSION") +
                                                                    project->first("COMPATIBILITY_VERSION"));
            }
            if(!project->isEmpty("QMAKE_LFLAGS_VERSION")) {
                project->values("QMAKE_LFLAGS") += QString(project->first("QMAKE_LFLAGS_VERSION") +
                                                                project->first("VER_MAJ") + "." +
                                                                project->first("VER_MIN") + "." +
                                                                project->first("VER_PAT"));
            }
            project->values("QMAKE_LFLAGS") += project->values("QMAKE_LFLAGS_SONAME");
        }
    }

    if (include_deps && project->isActiveConfig("gcc_MD_depends")) {
        ProString MD_flag("-MD");
        project->values("QMAKE_CFLAGS") += MD_flag;
        project->values("QMAKE_CXXFLAGS") += MD_flag;
    }
}

QString
UnixMakefileGenerator::libtoolFileName(bool fixify)
{
    QString ret = var("TARGET");
    int slsh = ret.lastIndexOf(Option::dir_sep);
    if(slsh != -1)
        ret = ret.right(ret.size() - slsh - 1);
    int dot = ret.indexOf('.');
    if(dot != -1)
        ret = ret.left(dot);
    ret += Option::libtool_ext;
    if(!project->isEmpty("QMAKE_LIBTOOL_DESTDIR"))
        ret.prepend(project->first("QMAKE_LIBTOOL_DESTDIR") + Option::dir_sep);
    if(fixify) {
        if(QDir::isRelativePath(ret) && !project->isEmpty("DESTDIR"))
            ret.prepend(project->first("DESTDIR").toQString());
        ret = fileFixify(ret, FileFixifyBackwards);
    }
    return ret;
}

static std::pair<ProStringList, ProStringList>
splitFrameworksAndLibs(const ProStringList &linkArgs)
{
    std::pair<ProStringList, ProStringList> result;
    bool frameworkArg = false;
    for (auto arg : linkArgs) {
        if (frameworkArg) {
            frameworkArg = false;
            result.second += arg;
        } else if (arg == "-framework") {
            frameworkArg = true;
            result.second += arg;
        } else {
            result.first += arg;
        }
    }
    return result;
}

void
UnixMakefileGenerator::writeLibtoolFile()
{
    auto fixDependencyLibs
            = [this](const ProStringList &libs)
              {
                  ProStringList result;
                  for (auto lib : libs) {
                      auto fi = fileInfo(lib.toQString());
                      if (fi.isAbsolute()) {
                          const QString libDirArg = "-L" + fi.path();
                          if (!result.contains(libDirArg))
                              result += libDirArg;
                          QString namespec = fi.fileName();
                          int dotPos = namespec.lastIndexOf('.');
                          if (dotPos != -1 && namespec.startsWith("lib")) {
                              namespec.truncate(dotPos);
                              namespec.remove(0, 3);
                          } else {
                              debug_msg(1, "Ignoring dependency library %s",
                                        lib.toLatin1().constData());
                              continue;
                          }
                          result += "-l" + namespec;
                      } else {
                          result += lib;
                      }
                  }
                  return result;
              };

    QString fname = libtoolFileName(), lname = fname;
    debug_msg(1, "Writing libtool file %s", fname.toLatin1().constData());
    mkdir(fileInfo(fname).path());
    int slsh = lname.lastIndexOf(Option::dir_sep);
    if(slsh != -1)
        lname = lname.right(lname.size() - slsh - 1);
    QFile ft(fname);
    if(!ft.open(QIODevice::WriteOnly))
        return;
    QString ffname(fileFixify(fname));
    project->values("ALL_DEPS").append(ffname);
    project->values("QMAKE_DISTCLEAN").append(ffname);

    QTextStream t(&ft);
    t << "# " << lname << " - a libtool library file\n";
    t << "# Generated by qmake/libtool (" QMAKE_VERSION_STR ") (Qt "
      << QT_VERSION_STR << ")";
    t << "\n";

    t << "# The name that we can dlopen(3).\n"
      << "dlname='" << fileVar(project->isActiveConfig("plugin") ? "TARGET" : "TARGET_x")
      << "'\n\n";

    t << "# Names of this library.\n";
    t << "library_names='";
    if(project->isActiveConfig("plugin")) {
        t << fileVar("TARGET");
    } else {
        if (project->isEmpty("QMAKE_HPUX_SHLIB"))
            t << fileVar("TARGET_x.y.z") << ' ';
        t << fileVar("TARGET_x") << ' ' << fileVar("TARGET_");
    }
    t << "'\n\n";

    t << "# The name of the static archive.\n"
      << "old_library='" << escapeFilePath(lname.left(lname.size()-Option::libtool_ext.size()))
                         << ".a'\n\n";

    t << "# Libraries that this one depends upon.\n";
    static const ProKey libVars[] = { "LIBS", "QMAKE_LIBS" };
    ProStringList libs;
    for (auto var : libVars)
        libs += fixLibFlags(var);
    ProStringList frameworks;
    std::tie(libs, frameworks) = splitFrameworksAndLibs(libs);
    t << "dependency_libs='" << fixDependencyLibs(libs).join(' ') << "'\n\n";
    if (!frameworks.isEmpty()) {
        t << "# Frameworks that this library depends upon.\n";
        t << "inherited_linker_flags='" << frameworks.join(' ') << "'\n\n";
    }

    t << "# Version information for " << lname << "\n";
    int maj = project->first("VER_MAJ").toInt();
    int min = project->first("VER_MIN").toInt();
    int pat = project->first("VER_PAT").toInt();
    t << "current=" << (10*maj + min) << "\n" // best I can think of
      << "age=0\n"
      << "revision=" << pat << "\n\n";

    t << "# Is this an already installed library.\n"
        "installed=yes\n\n"; // ###

    t << "# Files to dlopen/dlpreopen.\n"
        "dlopen=''\n"
        "dlpreopen=''\n\n";

    ProString install_dir = project->first("QMAKE_LIBTOOL_LIBDIR");
    if(install_dir.isEmpty())
        install_dir = project->first("target.path");
    if(install_dir.isEmpty())
        install_dir = project->first("DESTDIR");
    t << "# Directory that this library needs to be installed in:\n"
        "libdir='" << Option::fixPathToTargetOS(install_dir.toQString(), false) << "'\n";
}

bool UnixMakefileGenerator::writeObjectsPart(QTextStream &t, bool do_incremental)
{
    bool src_incremental = false;
    QString objectsLinkLine;
    const ProStringList &objs = project->values("OBJECTS");
    if (do_incremental) {
        const ProStringList &incrs = project->values("QMAKE_INCREMENTAL");
        ProStringList incrs_out;
        t << "OBJECTS       = ";
        for (ProStringList::ConstIterator objit = objs.begin(); objit != objs.end(); ++objit) {
            bool increment = false;
            for (ProStringList::ConstIterator incrit = incrs.begin(); incrit != incrs.end(); ++incrit) {
                auto regexp = QRegularExpression::fromWildcard((*incrit).toQString(), Qt::CaseSensitive,
                                                               QRegularExpression::UnanchoredWildcardConversion);
                if ((*objit).toQString().contains(regexp)) {
                    increment = true;
                    incrs_out.append((*objit));
                    break;
                }
            }
            if (!increment)
                t << "\\\n\t\t" << (*objit);
        }
        if (incrs_out.size() == objs.size()) { //we just switched places, no real incrementals to be done!
            t << escapeFilePaths(incrs_out).join(QString(" \\\n\t\t")) << Qt::endl;
        } else if (!incrs_out.size()) {
            t << Qt::endl;
        } else {
            src_incremental = true;
            t << Qt::endl;
            t << "INCREMENTAL_OBJECTS = "
              << escapeFilePaths(incrs_out).join(QString(" \\\n\t\t")) << Qt::endl;
        }
    } else {
        t << "OBJECTS       = " << valList(escapeDependencyPaths(objs)) << Qt::endl;
    }
    return src_incremental;
}

QT_END_NAMESPACE
