/*
 * Copyright (C) Pedram Pourang (aka Tsu Jan) 2014-2019 <tsujan2000@gmail.com>
 *
 * FeatherPad is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FeatherPad is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @license GPL-3.0+ <https://spdx.org/licenses/GPL-3.0+.html>
 */

#include "highlighter.h"

namespace FeatherPad {

QStringList Highlighter::keywords (const QString &lang)
{
    QStringList keywordPatterns;
    if (lang == "c" || lang == "cpp")
    {
        keywordPatterns << "\\b(and|asm|auto)(?!(\\.|-|@|#|\\$))\\b"
                        << "\\b(const|case|catch|cdecl|continue)(?!(\\.|-|@|#|\\$))\\b"
                        << "\\b(break|default|do)(?!(\\.|-|@|#|\\$))\\b"
                        << "\\b(enum|explicit|else|extern)(?!(\\.|-|@|#|\\$))\\b"
                        << "\\b(for|goto|if|NULL|or|pasca|register|return)(?!(\\.|-|@|#|\\$))\\b"
                        << "\\b(signals|sizeof|static|struct|switch)(?!(\\.|-|@|#|\\$))\\b"
                        << "\\b(typedef|typename|union|volatile|while)(?!(\\.|-|@|#|\\$))\\b";

        if (lang == "c")
            keywordPatterns << "\\b(FALSE|TRUE)(?!(\\.|-|@|#|\\$))\\b";
        else
            keywordPatterns << "\\b(class|const_cast|delete|dynamic_cast)(?!(\\.|-|@|#|\\$))\\b"
                            << "\\b(false|foreach|friend|inline|namespace|new|operator)(?!(\\.|-|@|#|\\$))\\b"
                            << "\\b(nullptr|override|private|protected|public|qobject_cast|reinterpret_cast|slots|static_cast)(?!(\\.|-|@|#|\\$))\\b"
                            << "\\b(template|true|this|throw|try|typeid|using|virtual)(?!(\\.|-|@|#|\\$))\\b"
                            << "\\bthis(?=->)\\b"; // "this" can be followed by "->"
    }
    else if (lang == "sh" || lang == "makefile" || lang == "cmake") // the characters "(", ";" and "&" will be reformatted after this
    {
        keywordPatterns << "((^\\s*|[\\(\\);&`\\|{}]+\\s*)((if|then|elif|elseif|else|fi|while|do|done|esac)\\s+)*)(alias|bg|bind|break|builtin)(?!(\\.|-|@|#|\\$))\\b"
                        << "((^\\s*|[\\(\\);&`\\|{}]+\\s*)((if|then|elif|elseif|else|fi|while|do|done|esac)\\s+)*)(caller|case|command|compgen|complete|continue)(?!(\\.|-|@|#|\\$))\\b"
                        << "((^\\s*|[\\(\\);&`\\|{}]+\\s*)((if|then|elif|elseif|else|fi|while|do|done|esac)\\s+)*)(declare|default|dirs|disown|do|done)(?!(\\.|-|@|#|\\$))\\b"
                        << "((^\\s*|[\\(\\);&`\\|{}]+\\s*)((if|then|elif|elseif|else|fi|while|do|done|esac)\\s+)*)(echo|enable|esac|eval|exec|exit|export)(?!(\\.|-|@|#|\\$))\\b"
                        << "(^\\s*|[\\(\\);&`\\|{}]+\\s*)(elif|elseif|else)(?!(\\.|-|@|#|\\$))\\b"
                        << "\\bfalse(?!(\\.|-|@|#|\\$))\\b"
                        << "((^\\s*|[\\(\\);&`\\|{}]+\\s*)((if|then|elif|elseif|else|fi|while|do|done|esac)\\s+)*)(fg|fi|for|foreach|getopts|hash|help|history)(?!(\\.|-|@|#|\\$))\\b"
                        << "((^\\s*|[\\(\\);&`\\|{}]+\\s*)((then|else|done|esac)\\s+)*)if(?!(\\.|-|@|#|\\$))\\b" << "\\bin(?!(\\.|-|@|#|\\$))\\b"
                        << "((^\\s*|[\\(\\);&`\\|{}]+\\s*)((if|then|elif|elseif|else|fi|while|do|done|esac)\\s+)*)(jobs|let|local|logout|popd|printf|pushd|read|readonly|return)(?!(\\.|-|@|#|\\$))\\b"
                        << "((^\\s*|[\\(\\);&`\\|{}]+\\s*)((if|then|elif|elseif|else|fi|while|do|done|esac)\\s+)*)(select|seq|set|shift|shopt|source|suspend|test|times|trap|type|typeset)(?!(\\.|-|@|#|\\$))\\b"
                        << "(^\\s*|[\\(\\);&`\\|{}]+\\s*)then(?!(\\.|-|@|#|\\$))\\b" << "\\btrue(?!(\\.|-|@|#|\\$))\\b"
                        << "((^\\s*|[\\(\\);&`\\|{}]+\\s*)((if|then|elif|elseif|else|fi|while|do|done|esac)\\s+)*)(umask|unalias|unset|until|wait|while)(?!(\\.|-|@|#|\\$))\\b";
        if (lang == "cmake") // (?i) is supported by QRegularExpression for ignoring case-sensitivity
            keywordPatterns << "(?i)(^\\s*|[\\(\\);&`\\|{}]+\\s*)(endif|endmacro|endwhile|file|include|option|project|add_compile_options|add_custom_command|add_custom_target|add_definitions|add_dependencies|add_executable|add_library|add_subdirectory|add_feature_info|add_test|aux_source_directory|build_command|cmake_dependent_option|cmake_host_system_information|cmake_minimum_required|cmake_parse_arguments|cmake_policy|configure_file|configure_package_config_file|create_test_sourcelist|define_property|ecm_configure_package_config_file|ecm_setup_version|enable_language|enable_testing|endforeach|endfunction|exec_program|execute_process|feature_summary|find_file|find_library|find_package|find_package_handle_standard_args|find_path|find_program|fltk_wrap_ui|function|get_cmake_property|get_directory_property|get_filename_component|get_property|get_source_file_property|get_target_property|get_test_property|include_directories|include_external_msproject|include_regular_expressionlink_directories|list|load_cache|load_command|macro|mark_as_advanced|math|message|pkg_check_modules|pkg_search_module|print_enabled_features|print_disabled_features|qt_wrap_cpp|qt_wrap_ui|remove_definitions|separate_arguments|set_directory_properties|set_feature_info|set_package_properties|set_property|set_source_files_properties|set_target_properties|set_tests_properties|site_name|source_group|string|target_compile_definitions|target_compile_options|target_include_directories|target_link_libraries|try_compile|try_run|variable_watch|write_basic_package_version_file)\\s*(?=(\\(|$))";
    }
    else if (lang == "qmake")
    {
        keywordPatterns << "\\b(CONFIG|DEFINES|DEF_FILE|DEPENDPATH|DEPLOYMENT_PLUGIN|DESTDIR|DISTFILES|DLLDESTDIR|FORMS|GUID|HEADERS|ICON|IDLSOURCES|INCLUDEPATH|INSTALLS|LEXIMPLS|LEXOBJECTS|LEXSOURCES|LIBS|LITERAL_HASH|MAKEFILE|MAKEFILE_GENERATOR|MOC_DIR|OBJECTS|OBJECTS_DIR|POST_TARGETDEPS|PRE_TARGETDEPS|PRECOMPILED_HEADER|PWD|OTHER_FILES|OUT_PWD|QMAKE|QMAKESPEC|QMAKE_AR_CMD|QMAKE_BUNDLE_DATA|QMAKE_BUNDLE_EXTENSION|QMAKE_BUNDLE_EXTENSION|QMAKE_CFLAGS|QMAKE_CFLAGS|QMAKE_CFLAGS_RELEASE|QMAKE_CFLAGS_SHLIB|QMAKE_CFLAGS_THREAD|QMAKE_CFLAGS_WARN_OFF|QMAKE_CFLAGS_WARN_ON|QMAKE_CLEAN|QMAKE_CXX|QMAKE_CXXFLAGS|QMAKE_CXXFLAGS_DEBUG|QMAKE_CXXFLAGS_RELEASE|QMAKE_CXXFLAGS_SHLIB|QMAKE_CXXFLAGS_THREAD|QMAKE_CXXFLAGS_WARN_OFF|QMAKE_CXXFLAGS_WARN_ON|QMAKE_DISTCLEAN|QMAKE_EXTENSION_SHLIB|QMAKE_EXTENSION_STATICLIB|QMAKE_EXT_MOC|QMAKE_EXT_UI|QMAKE_EXT_PRL|QMAKE_EXT_LEX|QMAKE_EXT_YACC|QMAKE_EXT_OBJ|QMAKE_EXT_CPP|QMAKE_EXT_H|QMAKE_EXTRA_COMPILERS|QMAKE_EXTRA_TARGETS|QMAKE_FAILED_REQUIREMENTS|QMAKE_FRAMEWORK_BUNDLE_NAME|QMAKE_FRAMEWORK_VERSION|QMAKE_HOST|QMAKE_INCDIR|QMAKE_INCDIR_EGL|QMAKE_INCDIR_OPENGL|QMAKE_INCDIR_OPENGL_ES2|QMAKE_INCDIR_OPENVG|QMAKE_INCDIR_X11|QMAKE_INFO_PLIST|QMAKE_LFLAGS|QMAKE_LFLAGS_CONSOLE|QMAKE_LFLAGS_DEBUG|QMAKE_LFLAGS_PLUGIN|QMAKE_LFLAGS_RPATH|QMAKE_LFLAGS_REL_RPATH|QMAKE_REL_RPATH_BASE|QMAKE_LFLAGS_RPATHLINK|QMAKE_LFLAGS_RELEASE|QMAKE_LFLAGS_APP|QMAKE_LFLAGS_SHLIB|QMAKE_LFLAGS_SONAME|QMAKE_LFLAGS_THREAD|QMAKE_LFLAGS_WINDOWS|QMAKE_LIBDIR|QMAKE_LIBDIR_FLAGS|QMAKE_LIBDIR_EGL|QMAKE_LIBDIR_OPENGL|QMAKE_LIBDIR_OPENVG|QMAKE_LIBDIR_X11|QMAKE_LIBS|QMAKE_LIBS_EGL|QMAKE_LIBS_OPENGL|QMAKE_LIBS_OPENGL_ES1|QMAKE_LIBS_OPENGL_ES2|QMAKE_LIBS_OPENVG|QMAKE_LIBS_THREAD|QMAKE_LIBS_X11|QMAKE_LIB_FLAG|QMAKE_LINK_SHLIB_CMD|QMAKE_LN_SHLIB|QMAKE_OBJECTIVE_CFLAGS|QMAKE_POST_LINK|QMAKE_PRE_LINK|QMAKE_PROJECT_NAME|QMAKE_MAC_SDK|QMAKE_MACOSX_DEPLOYMENT_TARGET|QMAKE_MAKEFILE|QMAKE_QMAKE|QMAKE_RESOURCE_FLAGS|QMAKE_RPATHDIR|QMAKE_RPATHLINKDIR|QMAKE_RUN_CC|QMAKE_RUN_CC_IMP|QMAKE_RUN_CXX|QMAKE_RUN_CXX_IMP|QMAKE_SONAME_PREFIX|QMAKE_TARGET|QMAKE_TARGET_COMPANY|QMAKE_TARGET_DESCRIPTION|QMAKE_TARGET_COPYRIGHT|QMAKE_TARGET_PRODUCT|QT|QTPLUGIN|QT_VERSION|QT_MAJOR_VERSION|QT_MINOR_VERSION|QT_PATCH_VERSION|RC_CODEPAGE|RC_DEFINES|RCC_DIR|RC_FILE|RC_ICONS|RC_INCLUDEPATH|RC_LANG|REQUIRES|RES_FILE|RESOURCES|SIGNATURE_FILE|SOURCES|SUBDIRS|TARGET|TARGET_EXT|TARGET_x|TARGET_x.y.z|TEMPLATE|TRANSLATIONS|UI_DIR|VERSION|VERSION_PE_HEADER|VER_MAJ|VER_MIN|VER_PAT|VPATH|WINRT_MANIFEST|YACCSOURCES|_PRO_FILE_|_PRO_FILE_PWD_)(?!(@|#|\\$))\\b";
    }
    else if (lang == "troff")
    {
        keywordPatterns << "^\\.(AT|B|BI|BR|BX|CW|DT|EQ|EN|I|IB|IR|IP|LG|LP|NL|P|PE|PD|PP|PS|R|RI|RB|RS|RE|SH|SM|SB|SS|TH|TS|TE|HP|TP|UC|UL|ab|ad|af|am|as|bd|bp|br|brp|c2|cc|ce|cend|cf|ch|cs|cstart|cu|da|de|di|ds|dt|ec|el|em|end|eo|ev|ex|fc|fi|fl|fp|ft|hc|hw|hy|ie|if|ig|in|it|lc|lg|ll|ls|lt|mc|mk|na|ne|nf|nh|nm|nn|nr|ns|nx|os|pc|pi|pl|pm|pm|pn|po|ps|rd|rj|rm|rn|rr|rs|rt|so|sp|ss|sv|sy|ta|tc|ti|tl|tm|tr|uf|ul|vs|wh)(?!(\\.|-|@|#|\\$))\\b";
    }
    else if (lang == "perl")
    {
        keywordPatterns << "\\b(?<!(@|#|\\$))(q|qq|qr|qw|qx|m|s|y|tr)(?!\\s*\\=>)\\b" // special meanings (the => operator is excluded)
                        << "\\b(?<!(@|#|\\$))(abs|alarm|and|atan2|binmode|bless)(?!(@|#|\\$))\\b"
                        << "\\b(?<!(@|#|\\$))(caller|chdir|chmod|chown|chroot|chomp|chop|chr|close|closedir|cmp|continue|cos|crypt)(?!(@|#|\\$))\\b"
                        << "\\b(?<!(@|#|\\$))(dbmclose|dbmopen|define|delete|die|do|dump)(?!(@|#|\\$))\\b"
                        << "\\b(?<!(@|#|\\$))(each|else|elsif|eof|eq|eva|exec|exists|exit|exp)(?!(@|#|\\$))\\b"
                        << "\\b(?<!(@|#|\\$))(fcntl|fileno|flock|for|foreach|fork|format)(?!(@|#|\\$))\\b"
                        << "\\b(?<!(@|#|\\$))(g|getc|getpgrp|getppid|getpriority|glob|goto|grep|hex)(?!(@|#|\\$))\\b"
                        << "\\b(?<!(@|#|\\$))(i|if|import|index|int|ioctl|join|keys|kill)(?!(@|#|\\$))\\b"
                        << "\\b(?<!(@|#|\\$))(last|lc|lcfirst|length|link|local|log|lstat|map|mkdir|my|next)(?!(@|#|\\$))\\b"
                        << "\\b(?<!(@|#|\\$))(oct|open|opendir|or|ord|our|pack|package|pipe|pop|print|printf|push)(?!(@|#|\\$))\\b"
                        << "\\b(?<!(@|#|\\$))(rand|read|readdir|readlink|redo|ref|rename|require|return|reverse|rewinddir|rindex|rmdir)(?!(@|#|\\$))\\b"
                        << "\\b(?<!(@|#|\\$))(seek|seekdir|select|setpgrp|setpriority|shift|sin|sleep|sort|splice|split|sprintf|sqrt|srand|stat|sub|substr|symlink|syscall|sysread|sysseek|system|syswrite|switch)(?!(@|#|\\$))\\b"
                        << "\\b(?<!(@|#|\\$))(tell|telldir|tie|tied|times|truncate)(?!(@|#|\\$))\\b"
                        << "\\b(?<!(@|#|\\$))(uc|ucfirst|umask|undef|unless|unlink|unpack|unshift|untie|use|utime)(?!(@|#|\\$))\\b"
                        << "\\b(?<!(@|#|\\$))(values|vec|x)(?!(@|#|\\$))\\b"
                        << "\\b(?<!(@|#|\\$))(wait|waitpid|warn|wantarray|while|write)(?!(@|#|\\$))\\b";
    }
    else if (lang == "ruby")
    {
        keywordPatterns << "\\b(__FILE__|__LINE__)(?!(@|#|\\$))\\b"
                        << "\\b(alias|and|begin|BEGIN|break)(?!(@|#|\\$))\\b"
                        << "\\b(case|class|catch|defined|do|def)(?!(@|#|\\$))\\b"
                        << "\\b(else|elsif|end|END|ensure|for|false|fail)(?!(@|#|\\$))\\b"
                        << "\\b(if|in|include|loop|load|module)(?!(@|#|\\$))\\b"
                        << "\\b(next|nil|not|or)(?!(@|#|\\$))\\b"
                        << "\\b(raise|redo|rescue|retry|return)(?!(@|#|\\$))\\b"
                        << "\\b(super|self|then|true|throw)(?!(@|#|\\$))\\b"
                        << "\\b(undef|unless|until|when|while|yield)(?!(@|#|\\$))\\b";
    }
    else if (lang == "lua")
        keywordPatterns << "\\b(and|break|do)(?!(\\.|@|#|\\$))\\b"
                        << "\\b(else|elseif|end)(?!(\\.|@|#|\\$))\\b"
                        << "\\b(false|for|function)(?!(\\.|@|#|\\$))\\b"
                        << "\\b(if|in|local|nil|not|or|repeat|return)(?!(\\.|@|#|\\$))\\b"
                        << "\\b(then|true|until|while)(?!(\\.|@|#|\\$))\\b";
    else if (lang == "python")
        keywordPatterns << "\\b(and|as|assert|break|class|continue)(?!(@|\\$))\\b"
                        << "\\b(def|del|elif|else|except|False|finally|for|from|global)(?!(@|\\$))\\b"
                        << "\\b(if|is|import|in|lambda|None|not|or|raise|return|True|try|while|with|yield)(?!(@|\\$))\\b"
                        << "\\b(exec|print)(?!(@|\\$|\\s*\\())\\b";
    else if (lang == "javascript" || lang == "qml")
    {
        keywordPatterns << "\\b(abstract|break)(?!(@|#|\\$))\\b"
                        << "\\b(case|catch|class|const|continue)(?!(@|#|\\$))\\b"
                        << "\\b(debugger|default|delete|do)(?!(@|#|\\$))\\b"
                        << "\\b(else|enum|export|extends)(?!(@|#|\\$))\\b"
                        << "\\b(false|final|finally|for|function|goto)(?!(@|#|\\$))\\b"
                        << "\\b(if|implements|in|instanceof|interface|let)(?!(@|#|\\$))\\b"
                        << "\\b(native|new|null)(?!(@|#|\\$))\\b"
                        << "\\b(private|protected|prototype|public|return)(?!(@|#|\\$))\\b"
                        << "\\b(static|super|switch|synchronized)(?!(@|#|\\$))\\b"
                        << "\\b(throw|throws|this|transient|true|try|typeof)(?!(@|#|\\$))\\b"
                        << "\\b(volatile|while|with)(?!(@|#|\\$))\\b";
        if (lang == "javascript")
            keywordPatterns << "\\b(var)(?!(@|#|\\$))\\b";
        else if (lang == "qml")
            keywordPatterns << "\\b(alias|id|import|property|readonly|signal)(?!(@|#|\\$))\\b";
    }
    else if (lang == "php")
        keywordPatterns << "\\b(__FILE__|__LINE__|__FUNCTION__|__CLASS__|__METHOD__|__DIR__|__NAMESPACE__)(?!(#|\\$))\\b"
                        << "\\b(and|abstract|array|as|break)(?!(#|\\$))\\b"
                        << "\\b(case|catch|cfunction|class|clone|const|continue)(?!(#|\\$))\\b"
                        << "\\b(declare|default|die|do)(?!(#|\\$))\\b"
                        << "\\b(each|echo|else|elseif|empty|enddeclare|endfor|endforeach|endif|endswitch|endwhile|eval|exception|exit|extends)(?!(#|\\$))\\b"
                        << "\\b(false|final|for|foreach|function)(?!(#|\\$))\\b"
                        << "\\b(global|goto|if|implements|interface|instanceof|isset)(?!(#|\\$))\\b"
                        << "\\b(list|namespace|new|null|old_function|or)(?!(#|\\$))\\b"
                        << "\\b(php_user_filter|print|private|protected|public|return)(?!(#|\\$))\\b"
                        << "\\b(static|switch|throw|true|try)(?!(#|\\$))\\b"
                        << "\\b(unset|use|var|while|xor)(?!(#|\\$))\\b";
    else if (lang == "scss") // taken from http://sass-lang.com/documentation/Sass/Script/Functions.html
        keywordPatterns << "\\b(none|null)(?!(\\.|-|@|#|\\$))\\b"
                        << "\\b(abs|adjust-color|adjust-hue|alpha|append|blue|call|ceil|change-color|comparable|complement|content-exists|darken|desaturate|feature-exists|floor|function-exists|get-function|global-variable-exists|grayscale|green|hsl|hsla|hue|ie-hex-str|if|index|inspect|invert|is-bracketed|is-superselector|join|keywords|length|lighten|lightness|list-separator|map-get|map-has-key|map-keys|map-merge|map-remove|map-values|max|min|mixin-exists|mix|nth|opacify|percentage|quote|random|red|rgb|rgba|round|scale-color|saturate|saturation|selector-nest|selector-append|selector-extend|selector-parse|selector-replace|selector-unify|set-nth|simple-selectors|str-index|str-insert|str-length|str-slice|to-lower-case|to-upper-case|transparentize|type-of|unit|unitless|unquote|variable-exists|zip)(?=\\()"
                        << "\\bunique-id\\(\\s*\\)";

    return keywordPatterns;
}
/*************************/
QStringList Highlighter::types()
{
    QStringList typePatterns;
    if (progLan == "c" || progLan == "cpp")
    {
        typePatterns << "\\b(bool|char|double|float)(?!(\\.|-|@|#|\\$))\\b"
                     << "\\b(gchar|gint|guint|guint8|guint16|guint32|guint64|gboolean)(?!(\\.|-|@|#|\\$))\\b"
                     << "\\b(int|long|short)(?!(\\.|-|@|#|\\$))\\b"
                     << "\\b(unsigned|uint|uint8|uint16|uint32|uint64|uint8_t|uint16_t|uint32_t|uint64_t)(?!(\\.|-|@|#|\\$))\\b"
                     << "\\b(uid_t|gid_t|mode_t)(?!(\\.|-|@|#|\\$))\\b"
                     << "\\b(void|wchar_t)(?!(\\.|-|@|#|\\$))\\b";
        if (progLan == "cpp")
            typePatterns << "\\b(qreal|qint8|quint8|qint16|quint16|qint32|quint32|qint64|quint64|qlonglong|qulonglong|qptrdiff|quintptr)(?!(\\.|-|@|#|\\$))\\b"
                         << "\\b(uchar|ulong|ushort)(?!(\\.|-|@|#|\\$))\\b"
                         << "\\b(std::[a-z_]+)(?=\\s*\\S+)(?!(\\s*\\(|\\.|-|@|#|\\$))\\b";
    }
    else if (progLan == "qml")
    {
        typePatterns << "\\b(bool|double|enumeration|int|list|real|string|url|var)(?!(@|#|\\$))\\b"
                     << "\\b(color|date|font|matrix4x4|point|quaternion|rect|size|vector2d|vector3d|vector4d)(?!(@|#|\\$))\\b";
    }
    return typePatterns;
}

}
