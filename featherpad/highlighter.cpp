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
#include <QTextDocument>

Q_DECLARE_METATYPE(QTextBlock)

namespace FeatherPad {

/* NOTE: It is supposed that a URL does not end with punctuation marks, parentheses or brackets. */
static const QRegularExpression urlPattern ("[A-Za-z0-9_]+://((?!&quot;|&gt;|&lt;)[A-Za-z0-9_.+/\\?\\=~&%#\\-:\\(\\)\\[\\]])+(?<!\\.|\\?|:|\\(|\\)|\\[|\\])|[A-Za-z0-9_.\\-]+@[A-Za-z0-9_\\-]+\\.[A-Za-z0-9.]+(?<!\\.)");
static const QRegularExpression notePattern ("\\b(NOTE|TODO|FIXME|WARNING)\\b");

TextBlockData::~TextBlockData()
{
    while (!allParentheses.isEmpty())
    {
        ParenthesisInfo *info = allParentheses.at (0);
        allParentheses.remove (0);
        delete info;
    }
    while (!allBraces.isEmpty())
    {
        BraceInfo *info = allBraces.at (0);
        allBraces.remove(0);
        delete info;
    }
    while (!allBrackets.isEmpty())
    {
        BracketInfo *info = allBrackets.at (0);
        allBrackets.remove(0);
        delete info;
    }
}
/*************************/
QVector<ParenthesisInfo *> TextBlockData::parentheses() const
{
    return allParentheses;
}
/*************************/
QVector<BraceInfo *> TextBlockData::braces() const
{
    return allBraces;
}
/*************************/
QVector<BracketInfo *> TextBlockData::brackets() const
{
    return allBrackets;
}
/*************************/
QString TextBlockData::labelInfo() const
{
    return label;
}
/*************************/
bool TextBlockData::isHighlighted() const
{
    return Highlighted;
}
/*************************/
bool TextBlockData::getProperty() const
{
    return Property;
}
/*************************/
int TextBlockData::lastState() const
{
    return LastState;
}
/*************************/
int TextBlockData::openNests() const
{
    return OpenNests;
}
/*************************/
int TextBlockData::lastFormattedQuote() const
{
    return LastFormattedQuote;
}
/*************************/
int TextBlockData::lastFormattedRegex() const
{
    return LastFormattedRegex;
}
/*************************/
QSet<int> TextBlockData::openQuotes() const
{
    return OpenQuotes;
}
/*************************/
void TextBlockData::insertInfo (ParenthesisInfo *info)
{
    int i = 0;
    while (i < allParentheses.size()
           && info->position > allParentheses.at (i)->position)
    {
        ++i;
    }

    allParentheses.insert (i, info);
}
/*************************/
void TextBlockData::insertInfo (BraceInfo *info)
{
    int i = 0;
    while (i < allBraces.size()
           && info->position > allBraces.at (i)->position)
    {
        ++i;
    }

    allBraces.insert (i, info);
}
/*************************/
void TextBlockData::insertInfo (BracketInfo *info)
{
    int i = 0;
    while (i < allBrackets.size()
           && info->position > allBrackets.at (i)->position)
    {
        ++i;
    }

    allBrackets.insert (i, info);
}
/*************************/
void TextBlockData::insertInfo (const QString &str)
{
    label = str;
}
/*************************/
void TextBlockData::setHighlighted()
{
    Highlighted = true;
}
/*************************/
void TextBlockData::setProperty (bool p)
{
    Property = p;
}
/*************************/
void TextBlockData::setLastState (int state)
{
    LastState = state;
}
/*************************/
void TextBlockData::insertNestInfo (int nests)
{
    OpenNests = nests;
}
/*************************/
void TextBlockData::insertLastFormattedQuote (int last)
{
    LastFormattedQuote = last;
}
/*************************/
void TextBlockData::insertLastFormattedRegex (int last)
{
    LastFormattedRegex = last;
}
/*************************/
void TextBlockData::insertOpenQuotes (const QSet<int> &openQuotes)
{
    OpenQuotes.unite (openQuotes);
}
/*************************/
// Here, the order of formatting is important because of overrides.
Highlighter::Highlighter (QTextDocument *parent, const QString& lang,
                          const QTextCursor &start, const QTextCursor &end,
                          bool darkColorScheme,
                          bool showWhiteSpace, bool showEndings) : QSyntaxHighlighter (parent)
{
    if (lang.isEmpty()) return;

    if (showWhiteSpace || showEndings)
    {
        QTextOption opt =  document()->defaultTextOption();
        QTextOption::Flags flags = opt.flags();
        if (showWhiteSpace)
            flags |= QTextOption::ShowTabsAndSpaces;
        if (showEndings)
            flags |= QTextOption::ShowLineAndParagraphSeparators
                     | QTextOption::AddSpaceForLineAndParagraphSeparators // never show the horizontal scrollbar on wrapping
                     | QTextOption::ShowDocumentTerminator;
        opt.setFlags (flags);
        document()->setDefaultTextOption (opt);
    }

    /* for highlighting next block inside highlightBlock() when needed */
    qRegisterMetaType<QTextBlock>();

    startCursor = start;
    endCursor = end;
    progLan = lang;

    quoteMark.setPattern ("\""); // the standard quote mark (always a single character)

    HighlightingRule rule;
    QColor Faded, translucent;
    if (!darkColorScheme)
    {
        mainFormat.setForeground (Qt::black);
        neutralFormat.setForeground (QColor (1, 1, 1));
        Blue = Qt::blue;
        DarkBlue = Qt::darkBlue;
        Red = Qt::red;
        DarkRed = QColor (150, 0, 0);
        Verda = QColor (0, 110, 110);
        DarkGreen = Qt::darkGreen;
        DarkMagenta = Qt::darkMagenta;
        Violet = QColor (126, 0, 230); //d556e6
        Brown = QColor (160, 80, 0);
        DarkYellow = Qt::darkYellow;
        Faded = QColor (165, 165, 165);
        translucent = QColor (0, 0, 0, 190);
    }
    else
    {
        mainFormat.setForeground (Qt::white);
        neutralFormat.setForeground (QColor (254, 254, 254));
        Blue = QColor (85, 227, 255);
        DarkBlue = QColor (65, 154, 255);
        Red = QColor (255, 120, 120);
        DarkRed = QColor (255, 160, 0);
        Verda = QColor (150, 255, 0);
        DarkGreen = Qt::green;
        DarkMagenta = QColor (255, 153, 255);
        Violet = QColor (255, 255, 0);
        Brown = QColor (255, 200, 0);
        DarkYellow = Qt::yellow;
        Faded = QColor (105, 105, 105);
        translucent = QColor (255, 255, 255, 190);
    }
    DarkGreenAlt = DarkGreen.lighter (101); // almost identical

    whiteSpaceFormat.setForeground (Faded);
    translucentFormat.setForeground (translucent);
    translucentFormat.setFontItalic (true);

    quoteFormat.setForeground (DarkGreen);
    altQuoteFormat.setForeground (DarkGreen);
    urlInsideQuoteFormat.setForeground (DarkGreen);
    altQuoteFormat.setFontItalic (true);
    urlInsideQuoteFormat.setFontItalic (true);
    urlInsideQuoteFormat.setFontUnderline (true);
    /*quoteStartExpression.setPattern ("\"([^\"'])");
    quoteEndExpression.setPattern ("([^\"'])\"");*/
    regexFormat.setForeground (DarkRed);

    /*************
     * Functions *
     *************/

    /* there may be javascript inside html */
    QString Lang = progLan == "html" ? "javascript" : progLan;

    /* may be overridden by the keywords format */
    if (progLan == "c" || progLan == "cpp"
        || Lang == "javascript" || progLan == "qml"
        || progLan == "lua" || progLan == "python" || progLan == "php")
    {
        QTextCharFormat functionFormat;
        functionFormat.setFontItalic (true);
        functionFormat.setForeground (Blue);
        /* before parentheses... */
        rule.pattern.setPattern ("\\b[A-Za-z0-9_]+(?=\\s*\\()");
        rule.format = functionFormat;
        highlightingRules.append (rule);
        /* ... but make exception for what comes after "#define" */
        if (progLan == "c" || progLan == "cpp")
        {
            rule.pattern.setPattern ("^\\s*#\\s*define\\s+[^\"\']" // may contain slash but no quote
                                    "+(?=\\s*\\()");
            rule.format = neutralFormat;
            highlightingRules.append (rule);
        }
        else if (progLan == "python")
        { // built-in functions
            functionFormat.setFontWeight (QFont::Bold);
            functionFormat.setForeground (Qt::magenta);
            rule.pattern.setPattern ("\\b(abs|add|all|append|any|as_integer_ratio|ascii|basestring|bin|bit_length|bool|bytearray|bytes|callable|c\\.conjugate|capitalize|center|chr|classmethod|clear|cmp|compile|complex|count|critical|debug|decode|delattr|dict|difference_update|dir|discard|divmod|encode|endswith|enumerate|error|eval|expandtabs|exception|exec|execfile|extend|file|filter|find|float|format|fromhex|fromkeys|frozenset|get|getattr|globals|hasattr|hash|has_key|help|hex|id|index|info|input|insert|int|intersection_update|isalnum|isalpha|isdecimal|isdigit|isinstance|islower|isnumeric|isspace|issubclass|istitle|items|iter|iteritems|iterkeys|itervalues|isupper|is_integer|join|keys|len|list|ljust|locals|log|long|lower|lstrip|map|max|memoryview|min|next|object|oct|open|ord|partition|pop|popitem|pow|print|property|range|raw_input|read|reduce|reload|remove|replace|repr|reverse|reversed|rfind|rindex|rjust|rpartition|round|rsplit|rstrip|run|seek|set|setattr|slice|sort|sorted|split|splitlines|staticmethod|startswith|str|strip|sum|super|symmetric_difference_update|swapcase|title|translate|tuple|type|unichr|unicode|update|upper|values|vars|viewitems|viewkeys|viewvalues|warning|write|xrange|zip|zfill|(__(abs|add|and|cmp|coerce|complex|contains|delattr|delete|delitem|delslice|div|divmod|enter|eq|exit|float|floordiv|ge|get|getattr|getattribute|getitem|getslice|gt|hex|iadd|iand|idiv|ifloordiv|ilshift|invert|imod|import|imul|init|instancecheck|index|int|ior|ipow|irshift|isub|iter|itruediv|ixor|le|len|long|lshift|lt|missing|mod|mul|neg|nonzero|oct|or|pos|pow|radd|rand|rdiv|rdivmod|reversed|rfloordiv|rlshift|rmod|rmul|ror|rpow|rshift|rsub|rrshift|rtruediv|rxor|set|setattr|setitem|setslice|sub|subclasses|subclasscheck|truediv|unicode|xor)__))(?=\\s*\\()");
            rule.format = functionFormat;
            highlightingRules.append (rule);
        }
    }

    /**********************
     * Keywords and Types *
     **********************/

    /* keywords */
    QTextCharFormat keywordFormat;
    /* bash extra keywords */
    if (progLan == "sh" || progLan == "makefile" || progLan == "cmake")
    {
        if (progLan == "cmake")
        {
            keywordFormat.setForeground (Brown);
            rule.pattern.setPattern ("\\$\\{\\s*[A-Za-z0-9_.+/\\?#\\-:]*\\s*\\}");
            rule.format = keywordFormat;
            highlightingRules.append (rule);

            /*
               previously, the first six patterns were started with:
               ((^\\s*|[\\(\\);&`\\|{}]+\\s*)((if|then|elif|elseif|else|fi|while|do|done|esac)\\s+)*)
               instead of: (?<=^|\\(|\\s)
            */
            keywordFormat.setForeground (DarkBlue);
            rule.pattern.setPattern ("(?<=^|\\(|\\s)(CMAKE_ARGC|CMAKE_ARGV0|CMAKE_ARGS|CMAKE_AR|CMAKE_BINARY_DIR|CMAKE_BUILD_TOOL|CMAKE_CACHE_ARGS|CMAKE_CACHE_DEFAULT_ARGS|CMAKE_CACHEFILE_DIR|CMAKE_CACHE_MAJOR_VERSION|CMAKE_CACHE_MINOR_VERSION|CMAKE_CACHE_PATCH_VERSION|CMAKE_CFG_INTDIR|CMAKE_COMMAND|CMAKE_CROSSCOMPILING|CMAKE_CTEST_COMMAND|CMAKE_CURRENT_BINARY_DIR|CMAKE_CURRENT_LIST_DIR|CMAKE_CURRENT_LIST_FILE|CMAKE_CURRENT_LIST_LINE|CMAKE_CURRENT_SOURCE_DIR|CMAKE_DL_LIBS|CMAKE_EDIT_COMMAND|CMAKE_EXECUTABLE_SUFFIX|CMAKE_EXTRA_GENERATOR|CMAKE_EXTRA_SHARED_LIBRARY_SUFFIXES|CMAKE_GENERATOR|CMAKE_GENERATOR_INSTANCE|CMAKE_GENERATOR_PLATFORM|CMAKE_GENERATOR_TOOLSET|CMAKE_HOME_DIRECTORY|CMAKE_IMPORT_LIBRARY_PREFIX|CMAKE_IMPORT_LIBRARY_SUFFIX|CMAKE_JOB_POOL_COMPILE|CMAKE_JOB_POOL_LINK|CMAKE_LINK_LIBRARY_SUFFIX|CMAKE_MAJOR_VERSION|CMAKE_MAKE_PROGRAM|CMAKE_MINIMUM_REQUIRED_VERSION|CMAKE_MINOR_VERSION|CMAKE_PARENT_LIST_FILE|CMAKE_PATCH_VERSION|CMAKE_PROJECT_NAME|CMAKE_RANLIB|CMAKE_ROOT|CMAKE_SCRIPT_MODE_FILE|CMAKE_SHARED_LIBRARY_PREFIX|CMAKE_SHARED_LIBRARY_SUFFIX|CMAKE_SHARED_MODULE_PREFIX|CMAKE_SHARED_MODULE_SUFFIX|CMAKE_SIZEOF_VOID_P|CMAKE_SKIP_INSTALL_RULES|CMAKE_SKIP_RPATH|CMAKE_SOURCE_DIR|CMAKE_STANDARD_LIBRARIES|CMAKE_STATIC_LIBRARY_PREFIX|CMAKE_STATIC_LIBRARY_SUFFIX|CMAKE_TOOLCHAIN_FILE|CMAKE_TWEAK_VERSION|CMAKE_VERBOSE_MAKEFILE|CMAKE_VERSION|CMAKE_VS_DEVENV_COMMAND|CMAKE_VS_INTEL_Fortran_PROJECT_VERSION|CMAKE_VS_MSBUILD_COMMAND|CMAKE_VS_MSDEV_COMMAND|CMAKE_VS_PLATFORM_TOOLSETCMAKE_XCODE_PLATFORM_TOOLSET|PROJECT_BINARY_DIR|PROJECT_NAME|PROJECT_SOURCE_DIR|PROJECT_VERSION|PROJECT_VERSION_MAJOR|PROJECT_VERSION_MINOR|PROJECT_VERSION_PATCH|PROJECT_VERSION_TWEAK|BUILD_SHARED_LIBS|CMAKE_ABSOLUTE_DESTINATION_FILES|CMAKE_APPBUNDLE_PATH|CMAKE_AUTOMOC_RELAXED_MODE|CMAKE_BACKWARDS_COMPATIBILITY|CMAKE_BUILD_TYPE|CMAKE_COLOR_MAKEFILE|CMAKE_CONFIGURATION_TYPES|CMAKE_DEBUG_TARGET_PROPERTIES|CMAKE_ERROR_DEPRECATED|CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION|CMAKE_SYSROOT|CMAKE_FIND_LIBRARY_PREFIXES|CMAKE_FIND_LIBRARY_SUFFIXES|CMAKE_FIND_NO_INSTALL_PREFIX|CMAKE_FIND_PACKAGE_WARN_NO_MODULE|CMAKE_FIND_ROOT_PATH|CMAKE_FIND_ROOT_PATH_MODE_INCLUDE|CMAKE_FIND_ROOT_PATH_MODE_LIBRARY|CMAKE_FIND_ROOT_PATH_MODE_PACKAGE|CMAKE_FIND_ROOT_PATH_MODE_PROGRAM|CMAKE_FRAMEWORK_PATH|CMAKE_IGNORE_PATH|CMAKE_INCLUDE_PATH|CMAKE_INCLUDE_DIRECTORIES_BEFORE|CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE|CMAKE_INSTALL_DEFAULT_COMPONENT_NAME|CMAKE_INSTALL_PREFIX|CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT|CMAKE_LIBRARY_PATH|CMAKE_MFC_FLAG|CMAKE_MODULE_PATH|CMAKE_NOT_USING_CONFIG_FLAGS|CMAKE_PREFIX_PATH|CMAKE_PROGRAM_PATH|CMAKE_SKIP_INSTALL_ALL_DEPENDENCY|CMAKE_STAGING_PREFIX|CMAKE_SYSTEM_IGNORE_PATH|CMAKE_SYSTEM_INCLUDE_PATH|CMAKE_SYSTEM_LIBRARY_PATH|CMAKE_SYSTEM_PREFIX_PATH|CMAKE_SYSTEM_PROGRAM_PATH|CMAKE_USER_MAKE_RULES_OVERRIDE|CMAKE_WARN_DEPRECATED|CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION|BORLAND|CMAKE_CL_64|CMAKE_COMPILER_2005|CMAKE_HOST_APPLE|CMAKE_HOST_SYSTEM_NAME|CMAKE_HOST_SYSTEM_PROCESSOR|CMAKE_HOST_SYSTEM|CMAKE_HOST_SYSTEM_VERSION|CMAKE_HOST_UNIX|CMAKE_HOST_WIN32|CMAKE_LIBRARY_ARCHITECTURE_REGEX|CMAKE_LIBRARY_ARCHITECTURE|CMAKE_OBJECT_PATH_MAX|CMAKE_SYSTEM_NAME|CMAKE_SYSTEM_PROCESSOR|CMAKE_SYSTEM|CMAKE_SYSTEM_VERSION|ENV|MSVC10|MSVC11|MSVC12|MSVC60|MSVC70|MSVC71|MSVC80|MSVC90|MSVC_IDE|MSVC|MSVC_VERSION|XCODE_VERSION|CMAKE_ARCHIVE_OUTPUT_DIRECTORY|CMAKE_AUTOMOC_MOC_OPTIONS|CMAKE_AUTOMOC|CMAKE_AUTORCC|CMAKE_AUTORCC_OPTIONS|CMAKE_AUTOUIC|CMAKE_AUTOUIC_OPTIONS|CMAKE_BUILD_WITH_INSTALL_RPATH|CMAKE_DEBUG_POSTFIX|CMAKE_EXE_LINKER_FLAGS|CMAKE_Fortran_FORMAT|CMAKE_Fortran_MODULE_DIRECTORY|CMAKE_GNUtoMS|CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE|CMAKE_INCLUDE_CURRENT_DIR|CMAKE_INSTALL_NAME_DIR|CMAKE_INSTALL_RPATH|CMAKE_INSTALL_RPATH_USE_LINK_PATH|CMAKE_LIBRARY_OUTPUT_DIRECTORY|CMAKE_LIBRARY_PATH_FLAG|CMAKE_LINK_DEF_FILE_FLAG|CMAKE_LINK_DEPENDS_NO_SHARED|CMAKE_LINK_INTERFACE_LIBRARIES|CMAKE_LINK_LIBRARY_FILE_FLAG|CMAKE_LINK_LIBRARY_FLAG|CMAKE_MACOSX_BUNDLE|CMAKE_MACOSX_RPATH|CMAKE_MODULE_LINKER_FLAGS|CMAKE_NO_BUILTIN_CHRPATH|CMAKE_NO_SYSTEM_FROM_IMPORTED|CMAKE_OSX_ARCHITECTURES|CMAKE_OSX_DEPLOYMENT_TARGET|CMAKE_OSX_SYSROOT|CMAKE_PDB_OUTPUT_DIRECTORY|CMAKE_POSITION_INDEPENDENT_CODE|CMAKE_RUNTIME_OUTPUT_DIRECTORY|CMAKE_SHARED_LINKER_FLAGS|CMAKE_SKIP_BUILD_RPATH|CMAKE_SKIP_INSTALL_RPATH|CMAKE_STATIC_LINKER_FLAGS|CMAKE_TRY_COMPILE_CONFIGURATION|CMAKE_USE_RELATIVE_PATHS|CMAKE_VISIBILITY_INLINES_HIDDEN|CMAKE_WIN32_EXECUTABLE|EXECUTABLE_OUTPUT_PATH|LIBRARY_OUTPUT_PATH|CMAKE_Fortran_MODDIR_DEFAULT|CMAKE_Fortran_MODDIR_FLAG|CMAKE_Fortran_MODOUT_FLAG|CMAKE_INTERNAL_PLATFORM_ABI)(?!(\\.|-|@|#|\\$))\\b");
            rule.format = keywordFormat;
            highlightingRules.append (rule);

            rule.pattern.setPattern ("(?<=^|\\(|\\s)(CMAKE_DISABLE_FIND_PACKAGE_|CMAKE_EXE_LINKER_FLAGS_|CMAKE_MAP_IMPORTED_CONFIG_|CMAKE_MODULE_LINKER_FLAGS_|CMAKE_PDB_OUTPUT_DIRECTORY_|CMAKE_SHARED_LINKER_FLAGS_|CMAKE_STATIC_LINKER_FLAGS_|CMAKE_COMPILER_IS_GNU)[A-Za-z0-9_]+(?!(\\.|-|@|#|\\$))\\b");
            highlightingRules.append (rule);

            rule.pattern.setPattern ("(?<=^|\\(|\\s)(CMAKE_)[A-Za-z0-9_]+(_POSTFIX|_VISIBILITY_PRESET|_ARCHIVE_APPEND|_ARCHIVE_CREATE|_ARCHIVE_FINISH|_COMPILE_OBJECT|_COMPILER_ABI|_COMPILER_ID|_COMPILER_LOADED|_COMPILER|_COMPILER_EXTERNAL_TOOLCHAIN|_COMPILER_TARGET|_COMPILER_VERSION|_CREATE_SHARED_LIBRARY|_CREATE_SHARED_MODULE|_CREATE_STATIC_LIBRARY|_FLAGS_DEBUG|_FLAGS_MINSIZEREL|_FLAGS_RELEASE|_FLAGS_RELWITHDEBINFO|_FLAGS|_IGNORE_EXTENSIONS|_IMPLICIT_INCLUDE_DIRECTORIES|_IMPLICIT_LINK_DIRECTORIES|_IMPLICIT_LINK_FRAMEWOR)(?!(\\.|-|@|#|\\$))\\b");
            highlightingRules.append (rule);

            rule.pattern.setPattern ("(?<=^|\\(|\\s)(CMAKE_PROJECT_)[A-Za-z0-9_]+(_INCLUDE)(?!(\\.|-|@|#|\\$))\\b");
            highlightingRules.append (rule);

            /* platforms */
            keywordFormat.setFontItalic (true);
            rule.pattern.setPattern ("(?<=^|\\(|\\s)(APPLE|CRLF|CYGWIN|DOS|HAIKU|LF|MINGW|MSYS|UNIX|WIN32)(?=$|\\)|\\s)");
            rule.format = keywordFormat;
            highlightingRules.append (rule);
            keywordFormat.setFontItalic (false);

            /* projects */
            keywordFormat.setForeground (Blue);
            rule.pattern.setPattern ("(?<=^|\\(|\\s)[A-Za-z0-9_]+(_BINARY_DIR|_SOURCE_DIR|_VERSION|_VERSION_MAJOR|_VERSION_MINOR|_VERSION_PATCH|_VERSION_TWEAK|_SOVERSION)(?!(\\.|-|@|#|\\$))\\b");
            rule.format = keywordFormat;
            highlightingRules.append (rule);

            keywordFormat.setForeground (DarkMagenta);
            rule.pattern.setPattern ("(?<=^|\\(|\\s)(AND|OR|NOT)(?=$|\\s)");
            rule.format = keywordFormat;
            highlightingRules.append (rule);

            rule.pattern.setPattern ("(?<=^|\\(|\\s)(ABSOLUTE|AFTER|ALIAS|ALIASED_TARGET|ALL|ALWAYS|APPEND|ARG_VAR|ARGS|ARCHIVE|ATTACHED_FILES|ATTACHED_FILES_ON_FAIL|AUTHOR_WARNING|BEFORE|BINARY_DIR|BOOL|BRIEF_DOCS|BUNDLE|BYPRODUCTS|CACHE|CACHED_VARIABLE|CLEAR|CMAKE_FIND_ROOT_PATH_BOTH|COMP|COMMAND|COMMENT|COMPONENT|COMPONENTS|CONFIG|CONFIG_MODE|CONFIGS|CONFIGURE_COMMAND|CONFIGURATIONS|COPY|COPYONLY|COMPATIBILITY|COST|CREATE_LINK|DEFAULT_MSG|DEPENDS|DEPENDEES|DEPENDERS|DEPRECATION|DESCRIPTION|DESTINATION|DIRECTORY|DIRECTORY_PERMISSIONS|DISABLED|DISABLED_FEATURES|DOC|DOWNLOAD|DOWNLOAD_COMMAND|DOWNLOAD_DIR|DOWNLOAD_NAME|DOWNLOAD_NO_EXTRACT|DOWNLOAD_NO_PROGRESS|ENABLED_FEATURES|ENCODING|ERROR_VARIABLE|ERROR_FILE|ERROR_QUIET|ERROR_STRIP_TRAILING_WHITESPACE|ESCAPE_QUOTES|EXACT|EXCLUDE|EXCLUDE_FROM_ALL|EXCLUDE_FROM_MAIN|EXPORT|EXPORT_LINK_INTERFACE_LIBRARIES|EXPORT_NAME|EXPORT_PROPERTIES|EXT|FAIL_MESSAGE|FALSE|FATAL_ERROR|FATAL_ON_MISSING_REQUIRED_PACKAGES|FILE_PERMISSIONS|FILENAME|FILEPATH|FILES|FILES_MATCHING|FIND|FILTER|FOLDER|FRAMEWORK|FORCE|FOUND_VAR|FULL_DOCS|GENERATE|GET|GLOB|GLOB_RECURSE|GLOBAL|GROUP_EXECUTE|GROUP_READ|GUARD|HANDLE_COMPONENTS|HINTS|IMMEDIATE|IMPLICIT_DEPENDS|IMPORTED|IN|INDEPENDENT_STEP_TARGETS|INCLUDE_QUIET_PACKAGES|INCLUDES|INPUT_FILE|INSERT|INSTALL|INSTALL_COMMAND|INSTALL_DIR|INSTALL_DESTINATION|INTERNAL|INTERFACE|JOIN|LABELS|LAST_EXT|LENGTH|LIBRARY|LIMIT|LINK_PRIVATE|LINK_PUBLIC|LOCK|LOG|LOG_DIR|MAIN_DEPENDENCY|MAKE_DIRECTORY|MEASUREMENT|MODULE|MODIFIED|NAMELINK_ONLY|NAMELINK_SKIP|NAMESPACE|NAME|NAME_WE|NAME_WLE|NAMES|NEWLINE_STYLE|NO_CHECK_REQUIRED_COMPONENTS_MACRO|NO_CMAKE_BUILDS_PATH|NO_CMAKE_ENVIRONMENT_PATH|NO_CMAKE_FIND_ROOT_PATH|NO_CMAKE_PACKAGE_REGISTRY|NO_CMAKE_PATH|NO_CMAKE_SYSTEM_PACKAGE_REGISTRY|NO_CMAKE_SYSTEM_PATH|NO_DEPENDS|NO_DEFAULT_PATH|NO_MODULE|NO_POLICY_SCOPE|NO_SET_AND_CHECK_MACRO|NO_SOURCE_PERMISSIONS|NO_SYSTEM_ENVIRONMENT_PATH|OBJECT|OFF|OFFSET|ON|ONLY_CMAKE_FIND_ROOT_PATH|OPTIONAL|OPTIONAL_PACKAGES_FOUND|OPTIONAL_PACKAGES_NOT_FOUND|OUTPUT_FILE|OUTPUT_QUIET|OUTPUT_STRIP_TRAILING_WHITESPACE|OUTPUT_VARIABLE|OWNER_EXECUTE|OWNER_READ|OWNER_WRITE|PATH|PACKAGES_FOUND|PACKAGES_NOT_FOUND|PACKAGE_VERSION_FILE|PARENT_SCOPE|PATHS|PATH_SUFFIXES|PATCH_COMMAND|PATH_VARS|PATTERN|PERMISSIONS|PKG_CONFIG_FOUND|PKG_CONFIG_EXECUTABLE|PKG_CONFIG_VERSION_STRING|PRE_BUILD|PRE_LINK|PREFIX|POST_BUILD|PRIVATE|PRIVATE_HEADER|PROGRAM|PROGRAM_ARGS|PROGRAMS|PROPERTY|PROPERTIES|PUBLIC|PUBLIC_HEADER|PURPOSE|QUIET|RANGE|READ|READ_SYMLINK|REALPATH|RECOMMENDED|RECOMMENDED_PACKAGES_FOUND|RECOMMENDED_PACKAGES_NOT_FOUND|REGEX|RELATIVE_PATH|RENAME|REMOVE|REMOVE_AT|REMOVE_DUPLICATES|REMOVE_RECURSE|REMOVE_ITEM|REQUIRED|REQUIRED_PACKAGES_FOUND|REQUIRED_PACKAGES_NOT_FOUND|REQUIRED_VARS|RESOURCE|REVERSE|RESULT_VARIABLE|RESULTS_VARIABLE|RUNTIME|RUNTIME_PACKAGES_FOUND|RUNTIME_PACKAGES_NOT_FOUND|RETURN_VALUE|SEND_ERROR|SHARED|SIZE|SORT|SOURCE|SOURCE_DIR|SOURCE_SUBDIR|SOURCES|SOVERSION|STAMP_DIR|STATIC|STATUS|STEP_TARGETS|STRING|STRINGS|SUBLIST|SUFFIX|SYSTEM|TARGETS|TEST|TIMEOUT|TIMESTAMP|TMP_DIR|TO_CMAKE_PATH|TO_NATIVE_PATH|TOUCH|TOUCH_NOCREATE|TRANSFORM|TRUE|TYPE|UNKNOWN|UPDATE_COMMAND|UPDATE_DISCONNECTED|UPLOAD|URL|URL_HASH|USES_TERMINAL|VAR|VARIABLE|VARIABLE_PREFIX|VERBATIM|VERSION|VERSION_HEADER|VERSION_VAR|WARNING|WHAT|WILL_FAIL|WORKING_DIRECTORY|WRITE)(?=$|\\)|\\s)");
            highlightingRules.append (rule);

            keywordFormat.setFontWeight (QFont::Bold);
            keywordFormat.setFontItalic (true);
            rule.pattern.setPattern ("(?<=^|\\(|\\s)(@ONLY|DEFINED|EQUAL|EXISTS|GREATER|IS_ABSOLUTE|IS_NEWER_THAN|IS_SYMLINK|LESS|MATCHES|STREQUAL|STRGREATER|STRLESS|VERSION_GREATER|VERSION_EQUAL|VERSION_LESS|AnyNewerVersion|ExactVersion|SameMajorVersion)(?=$|\\)|\\s)");
            rule.format = keywordFormat;
            highlightingRules.append (rule);
            keywordFormat.setFontItalic (false);
        }
        keywordFormat.setFontWeight (QFont::Bold);
        keywordFormat.setForeground (Qt::magenta);
        rule.pattern.setPattern ("((^\\s*|[\\(\\);&`\\|{}]+\\s*)((if|then|elif|elseif|else|fi|while|do|done|esac)\\s+)*(torify\\s+)?)(adduser|addgroup|apropos|apt-get|aspell|awk|basename|bash|bc|bunzip2|bzip2|cal|cat|cd|cfdisk|chgrp|chmod|chown|chroot|chkconfig|cksum|clear|cmake|cmp|comm|cp|cron|crontab|csplit|curl|cut|cvs|date|dc|dd|ddrescue|df|diff|diff3|dig|dir|dircolors|dirname|dirs|dmesg|dpkg|du|egrep|eject|emacs|env|ethtool|expect|expand|expr|fdformat|fdisk|featherpad|fgrep|file|find|finger|fmt|fold|format|fpad|free|fsck|ftp|function|fuser|gawk|gcc|git|grep|groups|gunzip|gzip|head|hostname|id|ifconfig|ifdown|ifup|import|install|java|javac|jobs|join|kdialog|kill|killall|less|links|ln|locate|logname|look|lpc|lpr|lprint|lprintd|lprintq|lprm|ls|lsof|lynx|mail|make|man|mkdir|mkfifo|mkisofs|mknod|mktemp|more|mount|mtools|mv|mmv|nano|netstat|nice|nl|nohup|nslookup|open|op|passwd|paste|pathchk|perl|pico|pine|ping|pkill|popd|pr|printcap|printenv|ps|pwd|python|qarma|qmake(-qt[3-9])*|quota|quotacheck|quotactl|ram|rcp|readarray|reboot|rename|renice|remsync|rev|rm|rmdir|rsync|ruby|screen|scp|sdiff|sed|sftp|shutdown|sleep|slocate|sort|split|ssh|strace|su|sudo|sum|svn|symlink|sync|tail|tar|tee|time|touch|top|traceroute|tr|tsort|tty|type|ulimit|umask|umount|uname|unexpand|uniq|units|unshar|unzip|useradd|usermod|users|usleep|uuencode|uudecode|vdir|vi|vim|vmstat|wall|watch|wc|whereis|which|who|whoami|wget|write|xargs|xeyes|yad|yes|zenity|zip)(?!\\.)(?!-)(?!(\\.|-|@|#|\\$))\\b");
        rule.format = keywordFormat;
        highlightingRules.append (rule);
    }
    else
        keywordFormat.setFontWeight (QFont::Bold);
    keywordFormat.setForeground (DarkBlue);

    /* types */
    QTextCharFormat typeFormat;
    typeFormat.setForeground (DarkMagenta); //(QColor (102, 0, 0));

    const QStringList keywordPatterns = keywords (Lang);
    for (const QString &pattern : keywordPatterns)
    {
        rule.pattern.setPattern (pattern);
        rule.format = keywordFormat;
        highlightingRules.append (rule);
    }

    if (progLan == "qmake")
    {
        QTextCharFormat qmakeFormat;
        /* qmake test functions */
        qmakeFormat.setForeground (DarkMagenta);
        rule.pattern.setPattern ("\\b(cache|CONFIG|contains|count|debug|defined|equals|error|eval|exists|export|files|for|greaterThan|if|include|infile|isActiveConfig|isEmpty|isEqual|lessThan|load|log|message|mkpath|packagesExist|prepareRecursiveTarget|qtCompileTest|qtHaveModule|requires|system|touch|unset|warning|write_file)(?=\\s*\\()");
        rule.format = qmakeFormat;
        highlightingRules.append (rule);
        /* qmake paths */
        qmakeFormat.setForeground (Blue);
        rule.pattern.setPattern ("\\${1,2}([A-Za-z0-9_]+|\\[[A-Za-z0-9_]+\\]|\\([A-Za-z0-9_]+\\))");
        rule.format = qmakeFormat;
        highlightingRules.append (rule);
    }

    const QStringList typePatterns = types();
    for (const QString &pattern : typePatterns)
    {
        rule.pattern.setPattern (pattern);
        rule.format = typeFormat;
        highlightingRules.append (rule);
    }

    /***********
     * Details *
     ***********/

    /* these are used for all comments */
    commentFormat.setForeground (Red);
    commentFormat.setFontItalic (true);

    /* these can also be used inside multiline comments */
    urlFormat.setFontUnderline (true);
    urlFormat.setForeground (Blue);
    urlFormat.setFontItalic (true);

    if (progLan == "c" || progLan == "cpp")
    {
        QTextCharFormat cFormat;

        /* Qt and Gtk+ specific classes */
        cFormat.setFontWeight (QFont::Bold);
        cFormat.setForeground (DarkMagenta);
        if (progLan == "cpp")
            rule.pattern.setPattern ("\\bQ[A-Z][A-Za-z0-9]+(?!(\\.|-|@|#|\\$))\\b");
        else
            rule.pattern.setPattern ("\\bG[A-Za-z]+(?!(\\.|-|@|#|\\$))\\b");
        rule.format = cFormat;
        highlightingRules.append (rule);

        /* QtGlobal functions and enum Qt::GlobalColor */
        if (progLan == "cpp")
        {
            cFormat.setFontItalic (true);
            rule.pattern.setPattern ("\\bq(App)(?!(\\@|#|\\$))\\b|\\bq(Abs|Bound|Critical|Debug|Fatal|FuzzyCompare|InstallMsgHandler|MacVersion|Max|Min|Round64|Round|Version|Warning|getenv|putenv|rand|srand|tTrId|unsetenv|_check_ptr|t_set_sequence_auto_mnemonic|t_symbian_exception2Error|t_symbian_exception2LeaveL|t_symbian_throwIfError)(?!(\\.|-|@|#|\\$))\\b");
            rule.format = cFormat;
            highlightingRules.append (rule);
            cFormat.setFontItalic (false);

            cFormat.setForeground (Qt::magenta);
            rule.pattern.setPattern ("\\bQt\\s*::\\s*(white|black|red|darkRed|green|darkGreen|blue|darkBlue|cyan|darkCyan|magenta|darkMagenta|yellow|darkYellow|gray|darkGray|lightGray|transparent|color0|color1)(?!(\\.|-|@|#|\\$))\\b");
            rule.format = cFormat;
            highlightingRules.append (rule);
        }

        /* preprocess */
        cFormat.setForeground (Blue);
        rule.pattern.setPattern ("^\\s*#\\s*include\\s|^\\s*#\\s*ifdef\\s|^\\s*#\\s*elif\\s|^\\s*#\\s*ifndef\\s|^\\s*#\\s*endif\\b|^\\s*#\\s*define\\s|^\\s*#\\s*undef\\s|^\\s*#\\s*error\\s|^\\s*#\\s*if\\s|^\\s*#\\s*else(?!(\\.|-|@|#|\\$))\\b");
        rule.format = cFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "python")
    {
        QTextCharFormat pFormat;
        pFormat.setFontWeight (QFont::Bold);
        pFormat.setForeground (DarkMagenta);
        rule.pattern.setPattern ("\\bself(?!(@|\\$))\\b");
        rule.format = pFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "qml")
    {
        QTextCharFormat qmlFormat;
        qmlFormat.setFontWeight (QFont::Bold);
        qmlFormat.setForeground (DarkMagenta);
        rule.pattern.setPattern ("\\b(Qt[A-Za-z]+|Accessible|AnchorAnimation|AnchorChanges|AnimatedImage|AnimatedSprite|Animation|AnimationController|Animator|Behavior|BorderImage|Canvas|CanvasGradient|CanvasImageData|CanvasPixelArray|ColorAnimation|Column|Context2D|DoubleValidator|Drag|DragEvent|DropArea|EnterKey|Flickable|Flipable|Flow|FocusScope|FontLoader|FontMetrics|Gradient|GradientStop|Grid|GridMesh|GridView|Image|IntValidator|Item|ItemGrabResult|KeyEvent|KeyNavigation|Keys|LayoutMirroring|ListView|Loader|Matrix4x4|MouseArea|MouseEvent|MultiPointTouchArea|NumberAnimation|OpacityAnimator|OpenGLInfo|ParallelAnimation|ParentAnimation|ParentChange|Path|PathAnimation|PathArc|PathAttribute|PathCubic|PathCurve|PathElement|PathInterpolator|PathLine|PathPercent|PathQuad|PathSvg|PathView|PauseAnimation|PinchArea|PinchEvent|Positioner|PropertyAction|PropertyAnimation|PropertyChanges|Rectangle|RegExpValidator|Repeater|Rotation|RotationAnimation|RotationAnimator|Row|Scale|ScaleAnimator|ScriptAction|SequentialAnimation|ShaderEffect|ShaderEffectSource|Shortcut|SmoothedAnimation|SpringAnimation|Sprite|SpriteSequence|State|StateChangeScript|StateGroup|SystemPalette|Text|TextEdit|TextInput|TextMetrics|TouchPoint|Transform|Transition|Translate|UniformAnimator|Vector3dAnimation|ViewTransition|WheelEvent|XAnimator|YAnimator|CloseEvent|ColorDialog|ColumnLayout|Dialog|FileDialog|FontDialog|GridLayout|Layout|MessageDialog|RowLayout|StackLayout|LocalStorage|Screen|SignalSpy|TestCase|Window|XmlListModel|XmlRole|Action|ApplicationWindow|BusyIndicator|Button|Calendar|CheckBox|ComboBox|ExclusiveGroup|GroupBox|Label|Menu|MenuBar|MenuItem|MenuSeparator|ProgressBar|RadioButton|ScrollView|Slider|SpinBox|SplitView|Stack|StackView|StackViewDelegate|StatusBar|Switch|Tab|TabView|TableView|TableViewColumn|TextArea|TextField|ToolBar|ToolButton|TreeView|Affector|Age|AngleDirection|Attractor|CumulativeDirection|CustomParticle|Direction|EllipseShape|Emitter|Friction|Gravity|GroupGoal|ImageParticle|ItemParticle|LineShape|MaskShape|Particle|ParticleGroup|ParticlePainter|ParticleSystem|PointDirection|RectangleShape|Shape|SpriteGoal|TargetDirection|TrailEmitter|Turbulence|Wander|Timer)(?!(\\-|@|#|\\$))\\b");
        rule.format = qmlFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "xml")
    {
        QTextCharFormat xmlElementFormat;
        xmlElementFormat.setFontWeight (QFont::Bold);
        xmlElementFormat.setForeground (Violet);
        /* after </ or before /> */
        rule.pattern.setPattern ("\\s*(<|&lt;)/?[A-Za-z0-9_\\-:]+|\\s*(<|&lt;)!(DOCTYPE|ENTITY|ELEMENT|ATTLIST|NOTATION)\\s|\\s*/?(>|&gt;)");
        rule.format = xmlElementFormat;
        highlightingRules.append (rule);

        QTextCharFormat xmlAttributeFormat;
        xmlAttributeFormat.setFontItalic (true);
        xmlAttributeFormat.setForeground (Blue);
        /* before = */
        rule.pattern.setPattern ("\\s+[A-Za-z0-9_\\-:]+(?=\\s*\\=\\s*(\"|&quot;))");
        rule.format = xmlAttributeFormat;
        highlightingRules.append (rule);

        /* <?xml ... ?> */
        rule.pattern.setPattern ("^\\s*<\\?xml\\s+(?=.*\\?>)|\\s*\\?>");
        rule.format = keywordFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "changelog")
    {
        /* before colon */
        rule.pattern.setPattern ("^\\s+\\*\\s+[^:]+:");
        rule.format = keywordFormat;
        highlightingRules.append (rule);

        QTextCharFormat asteriskFormat;
        asteriskFormat.setForeground (DarkMagenta);
        /* the first asterisk */
        rule.pattern.setPattern ("^\\s+\\*\\s+");
        rule.format = asteriskFormat;
        highlightingRules.append (rule);

        rule.pattern.setPattern (urlPattern.pattern());
        rule.format = urlFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "sh" || progLan == "makefile" || progLan == "cmake"
             || progLan == "perl" || progLan == "ruby")
    {
        /* # is the sh comment sign when it doesn't follow a character */
        if (progLan == "sh" || progLan == "makefile" || progLan == "cmake")
            rule.pattern.setPattern ("(?<=^|\\s|;)#.*");
        else if (progLan == "perl") // $# isn't a comment
            rule.pattern.setPattern ("(?<!\\$)#.*");
        else
            rule.pattern.setPattern ("#.*");
        rule.format = commentFormat;
        highlightingRules.append (rule);

        QTextCharFormat shFormat;

        if (progLan == "sh" || progLan == "makefile" || progLan == "cmake")
        {
            /* make parentheses, braces and ; neutral as they were in keyword patterns */
            rule.pattern.setPattern ("[\\(\\){};]");
            rule.format = neutralFormat;
            highlightingRules.append (rule);

            shFormat.setForeground (Blue);
            /* words before = */
             if (progLan == "sh")
                 rule.pattern.setPattern ("\\b[A-Za-z0-9_]+(?=\\=)");
             else
                 rule.pattern.setPattern ("\\b[A-Za-z0-9_]+\\s*(?=\\+{0,1}\\=)");
            rule.format = shFormat;
            highlightingRules.append (rule);

            /* but don't format a word before =
               if it follows a dash */
            rule.pattern.setPattern ("-+[^\\s\\\"\\\']+(?=\\=)");
            rule.format = neutralFormat;
            highlightingRules.append (rule);
        }

        if (progLan == "makefile" || progLan == "cmake")
        {
            shFormat.setForeground (DarkYellow);
            /* automake/autoconf variables */
            rule.pattern.setPattern ("@[A-Za-z0-9_-]+@|^[a-zA-Z0-9_-]+\\s*(?=:)");
            rule.format = shFormat;
            highlightingRules.append (rule);
        }

        shFormat.setForeground (DarkMagenta);
        /* operators */
        rule.pattern.setPattern ("[=\\+\\-*/%<>&`\\|~\\^\\!,]|\\s+-eq\\s+|\\s+-ne\\s+|\\s+-gt\\s+|\\s+-ge\\s+|\\s+-lt\\s+|\\s+-le\\s+|\\s+-z\\s+");
        rule.format = shFormat;
        highlightingRules.append (rule);

        if (progLan == "sh" || progLan == "makefile" || progLan == "cmake")
        {
            shFormat.setFontWeight (QFont::Bold);
            /* brackets */
            rule.pattern.setPattern ("\\s+\\[{1,2}\\s+|^\\[{1,2}\\s+|\\s+\\]{1,2}\\s+|\\s+\\]{1,2}$|\\s+\\]{1,2}\\s*(?=;)");
            rule.format = shFormat;
            highlightingRules.append (rule);
        }
    }
    else if (progLan == "diff")
    {
        QTextCharFormat diffMinusFormat;
        diffMinusFormat.setForeground (Red);
        rule.pattern.setPattern ("^\\-\\s*.*");
        rule.format = diffMinusFormat;
        highlightingRules.append (rule);

        QTextCharFormat diffPlusFormat;
        diffPlusFormat.setForeground (Blue);
        rule.pattern.setPattern ("^\\+\\s*.*");
        rule.format = diffPlusFormat;
        highlightingRules.append (rule);

        QTextCharFormat diffLinesFormat;
        diffLinesFormat.setFontWeight (QFont::Bold);
        diffLinesFormat.setForeground (DarkGreenAlt);
        rule.pattern.setPattern ("^@{2}[\\d,\\-\\+\\s]+@{2}");
        rule.format = diffLinesFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "log")
    {
        /* example:
         * May 19 02:01:44 debian sudo:
         *   blue  green  magenta bold */
        QTextCharFormat logFormat = neutralFormat;
        logFormat.setFontWeight (QFont::Bold);
        rule.pattern.setPattern ("^[A-Za-z]{3}\\s+\\d{1,2}\\s{1}\\d{2}:\\d{2}:\\d{2}\\s+[A-Za-z0-9_\\[\\]\\s]+(?=\\s*:)");
        rule.format = logFormat;
        highlightingRules.append (rule);

        QTextCharFormat logFormat1;
        logFormat1.setForeground (Qt::magenta);
        rule.pattern.setPattern ("^[A-Za-z]{3}\\s+\\d{1,2}\\s{1}\\d{2}:\\d{2}:\\d{2}\\s+[A-Za-z]+");
        rule.format = logFormat1;
        highlightingRules.append (rule);

        QTextCharFormat logDateFormat;
        logDateFormat.setFontWeight (QFont::Bold);
        logDateFormat.setForeground (Blue);
        rule.pattern.setPattern ("^[A-Za-z]{3}\\s+\\d{1,2}(?=\\s{1}\\d{2}:\\d{2}:\\d{2})");
        rule.format = logDateFormat;
        highlightingRules.append (rule);

        QTextCharFormat logTimeFormat;
        logTimeFormat.setFontWeight (QFont::Bold);
        logTimeFormat.setForeground (DarkGreenAlt);
        rule.pattern.setPattern ("\\s{1}\\d{2}:\\d{2}:\\d{2}\\b");
        rule.format = logTimeFormat;
        highlightingRules.append (rule);

        QTextCharFormat logInOutFormat;
        logInOutFormat.setFontWeight (QFont::Bold);
        logInOutFormat.setForeground (Brown);
        rule.pattern.setPattern ("\\s+IN(?=\\s*\\=)|\\s+OUT(?=\\s*\\=)");
        rule.format = logInOutFormat;
        highlightingRules.append (rule);

        QTextCharFormat logRootFormat;
        logRootFormat.setFontWeight (QFont::Bold);
        logRootFormat.setForeground (Red);
        rule.pattern.setPattern ("\\broot\\b");
        rule.format = logRootFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "srt")
    {
        QTextCharFormat srtFormat;
        srtFormat.setFontWeight (QFont::Bold);

        /* <...> */
        srtFormat.setForeground (Violet);
        rule.pattern.setPattern ("</?[A-Za-z0-9_#\\s\"\\=]+>");
        rule.format = srtFormat;
        highlightingRules.append (rule);

        /* hh:mm:ss,ttt */
        srtFormat = neutralFormat;
        srtFormat.setFontItalic (true);
        rule.pattern.setPattern ("^\\d+$|^\\d{2}:\\d{2}:\\d{2},\\d{3}\\s-->\\s\\d{2}:\\d{2}:\\d{2},\\d{3}$");
        rule.format = srtFormat;
        highlightingRules.append (rule);

        /* subtitle line */
        srtFormat.setForeground (Red);
        rule.pattern.setPattern ("^\\d+$");
        rule.format = srtFormat;
        highlightingRules.append (rule);

        /* mm */
        srtFormat.setForeground (DarkGreenAlt);
        rule.pattern.setPattern ("\\d{2}(?=:\\d{2},\\d{3}\\s-->\\s\\d{2}:\\d{2}:\\d{2},\\d{3}$)|\\d{2}(?=:\\d{2},\\d{3}$)");
        rule.format = srtFormat;
        highlightingRules.append (rule);

        /* hh */
        srtFormat.setForeground (Blue);
        rule.pattern.setPattern ("^\\d{2}(?=:\\d{2}:\\d{2},\\d{3}\\s-->\\s\\d{2}:\\d{2}:\\d{2},\\d{3}$)|\\s\\d{2}(?=:\\d{2}:\\d{2},\\d{3}$)");
        rule.format = srtFormat;
        highlightingRules.append (rule);

        /* ss */
        srtFormat.setForeground (Brown);
        rule.pattern.setPattern ("\\d{2}(?=,\\d{3}\\s-->\\s\\d{2}:\\d{2}:\\d{2},\\d{3}$)|\\d{2}(?=,\\d{3}$)");
        rule.format = srtFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "desktop" || progLan == "config" || progLan == "theme")
    {
        QTextCharFormat desktopFormat = neutralFormat;
        if (progLan == "config")
        {
            desktopFormat.setFontWeight (QFont::Bold);
            desktopFormat.setFontItalic (true);
            /* color values */
            rule.pattern.setPattern ("#([A-Fa-f0-9]{3}){0,2}(?![A-Za-z0-9_]+)|#([A-Fa-f0-9]{3}){2}[A-Fa-f0-9]{2}(?![A-Za-z0-9_]+)");
            rule.format = desktopFormat;
            highlightingRules.append (rule);
            desktopFormat.setFontItalic (false);
            desktopFormat.setFontWeight (QFont::Normal);

            /* URLs */
            rule.pattern = urlPattern;
            rule.format = urlFormat;
            highlightingRules.append (rule);
        }

        desktopFormat.setForeground (DarkMagenta);
        rule.pattern.setPattern ("^[^\\=]+=|^[^\\=]+\\[.*\\]=|;|/|%|\\+|-");
        rule.format = desktopFormat;
        highlightingRules.append (rule);

        desktopFormat = neutralFormat;
        desktopFormat.setFontWeight (QFont::Bold);
        /* [...] */
        rule.pattern.setPattern ("^\\[.*\\]$");
        rule.format = desktopFormat;
        highlightingRules.append (rule);

        desktopFormat.setForeground (Blue);
        /* [...] and before = (like ...[en]=)*/
        rule.pattern.setPattern ("^[^\\=]+\\[.*\\](?=\\s*\\=)");
        rule.format = desktopFormat;
        highlightingRules.append (rule);

        desktopFormat.setForeground (DarkGreenAlt);
        /* before = and [] */
        rule.pattern.setPattern ("^[^\\=\\[]+(?=(\\[.*\\])*\\s*\\=)");
        rule.format = desktopFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "yaml")
    {
        rule.pattern.setPattern ("#.*");
        rule.format = commentFormat;
        highlightingRules.append (rule);

        QTextCharFormat yamlFormat;

        /* keys */
        // NOTE: This is the first time I use \K with Qt and it seems to work well.
        yamlFormat.setForeground (Blue);
        rule.pattern.setPattern ("[^:,#]*:(\\s+|$)");
        rule.format = yamlFormat;
        highlightingRules.append (rule);

        /* values */
        yamlFormat.setForeground (Violet);
        rule.pattern.setPattern ("[^:#]*:\\s+\\K[^#]+");
        rule.format = yamlFormat;
        highlightingRules.append (rule);

        /* non-value numbers (including the scientific notation) */
        yamlFormat.setForeground (Brown);
        rule.pattern.setPattern ("^((\\s*-\\s)+)?\\s*\\K[-+]?\\d*\\.?\\d+(e(\\+|-)\\d+)?\\s*(?=(#|$))");
        rule.format = yamlFormat;
        highlightingRules.append (rule);

        /* lists */
        yamlFormat.setForeground (DarkBlue);
        yamlFormat.setFontWeight (QFont::Bold);
        rule.pattern.setPattern ("^(\\s*-(\\s|$))+");
        rule.format = yamlFormat;
        highlightingRules.append (rule);

        /* booleans */
        yamlFormat.setForeground (DarkBlue);
        yamlFormat.setFontWeight (QFont::Bold);
        rule.pattern.setPattern ("^((\\s*-\\s)+)?\\s*\\K(true|false|yes|no|TRUE|FALSE|YES|NO|True|False|Yes|No)\\s*(?=(#|$))");
        rule.format = yamlFormat;
        highlightingRules.append (rule);

        /* | and > */
        codeBlockFormat.setForeground (DarkMagenta);
        codeBlockFormat.setFontWeight (QFont::Bold);
        rule.pattern.setPattern (".*\\s+\\K(\\||>)\\s*$");
        rule.format = codeBlockFormat;
        highlightingRules.append (rule);

        yamlFormat.setForeground (Verda);
        rule.pattern.setPattern ("^---.*");
        rule.format = yamlFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "fountain")
    {
        QTextCharFormat fFormat;

        /* sections */
        fFormat.setForeground (DarkRed);
        fFormat.setFontWeight (QFont::Bold);
        rule.pattern.setPattern ("^\\s*#.*");
        rule.format = fFormat;
        highlightingRules.append (rule);

        /* centered */
        rule.pattern.setPattern (">|<");
        rule.format = fFormat;
        highlightingRules.append (rule);

        /* synopses */
        fFormat.setFontItalic (true);
        rule.pattern.setPattern ("^\\s*=.*");
        rule.format = fFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "url")
    {
        rule.pattern.setPattern (urlPattern.pattern());
        rule.format = urlFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "gtkrc")
    {
        QTextCharFormat gtkrcFormat;
        gtkrcFormat.setFontWeight (QFont::Bold);
        /* color value format (#xyz) */
        /*gtkrcFormat.setForeground (DarkGreenAlt);
        rule.pattern.setPattern ("#([A-Fa-f0-9]{3}){0,2}(?![A-Za-z0-9_]+)|#([A-Fa-f0-9]{3}){2}[A-Fa-f0-9]{2}(?![A-Za-z0-9_]+)");
        rule.format = gtkrcFormat;
        highlightingRules.append (rule);*/

        gtkrcFormat.setForeground (Blue);
        rule.pattern.setPattern ("(fg|bg|base|text)(\\[NORMAL\\]|\\[PRELIGHT\\]|\\[ACTIVE\\]|\\[SELECTED\\]|\\[INSENSITIVE\\])");
        rule.format = gtkrcFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "markdown")
    {
        /* Inline code is almost like a single-line quote.
           "`" will be distinguished from "```" at multiLineQuote(). */
        quoteMark.setPattern ("`");

        blockQuoteFormat.setForeground (DarkGreen);
        codeBlockFormat.setForeground (DarkRed);
        QTextCharFormat markdownFormat;

        /* lists */
        markdownFormat.setFontWeight (QFont::Bold);
        markdownFormat.setForeground (DarkBlue);
        rule.pattern.setPattern ("^ {0,3}((\\*\\s+){1,}|(\\+\\s+){1,}|(\\-\\s+){1,}|\\d+\\.\\s+|\\d+\\)\\s+)");
        rule.format = markdownFormat;
        highlightingRules.append (rule);

        /* footnotes */
        markdownFormat.setFontItalic (true);
        rule.pattern.setPattern ("\\[\\^[^\\]]+\\]");
        rule.format = markdownFormat;
        highlightingRules.append (rule);
        markdownFormat.setFontItalic (false);

        /* horizontal rules */
        markdownFormat.setForeground (DarkMagenta);
        rule.pattern.setPattern ("^ {0,3}(\\* {0,2}){3,}\\s*$"
                                 "|"
                                 "^ {0,3}(- {0,2}){3,}\\s*$"
                                 "|"
                                 "^ {0,3}(\\= {0,2}){3,}\\s*$");
        rule.format = markdownFormat;
        highlightingRules.append (rule);

        /*
           links:
           [link text] [link]
           [link text] (http://example.com "Title")
           [link text]: http://example.com
           <http://example.com>
        */
        rule.pattern.setPattern ("\\[[^\\]\\^]*\\]\\s*\\[[^\\]\\s]*\\]"
                                 "|"
                                 "\\[[^\\]\\^]*\\]\\s*\\(\\s*[^\\)\\(\\s]+(\\s+\\\".*\\\")*\\s*\\)"
                                 "|"
                                 "\\[[^\\]\\^]*\\]:\\s+\\s*[^\\)\\(\\s]+(\\s+\\\".*\\\")*"
                                 "|"
                                 "<([A-Za-z0-9_]+://[A-Za-z0-9_.+/\\?\\=~&%#\\-:\\(\\)\\[\\]]+|[A-Za-z0-9_.\\-]+@[A-Za-z0-9_\\-]+\\.[A-Za-z0-9.]+)>");
        rule.format = urlFormat;
        highlightingRules.append (rule);

        /*
           images:
           ![image](image.jpg "An image")
           ![Image][1]
           [1]: /path/to/image "alt text"
        */
        markdownFormat.setFontWeight (QFont::Normal);
        markdownFormat.setForeground (Violet);
        markdownFormat.setFontUnderline (true);
        rule.pattern.setPattern ("\\!\\[[^\\]\\^]*\\]\\s*"
                                 "(\\(\\s*[^\\)\\(\\s]+(\\s+\\\".*\\\")*\\s*\\)|\\s*\\[[^\\]]*\\])");
        rule.format = markdownFormat;
        highlightingRules.append (rule);
        markdownFormat.setFontUnderline (false);

        /* code blocks */
        rule.pattern.setPattern ("^( {4,}|\\s*\\t+\\s*).*");
        rule.format = codeBlockFormat;
        highlightingRules.append (rule);

        /* headings */
        markdownFormat.setFontWeight (QFont::Bold);
        markdownFormat.setForeground (Blue);
        rule.pattern.setPattern ("^#+\\s+.*");
        rule.format = markdownFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "reST")
    {
        /* For bold, italic, verbatim and link

           possible characters before the start:  ([{<:'"/
           possible characters after the end:     )]}>.,;:-'"/\
        */

        codeBlockFormat.setForeground (DarkRed);
        QTextCharFormat reSTFormat;

        /* headings */
        reSTFormat.setFontWeight (QFont::Bold);
        reSTFormat.setForeground (Blue);
        rule.pattern.setPattern ("^([^\\s\\w:])\\1{2,}$");
        rule.format = reSTFormat;
        highlightingRules.append (rule);

        /* lists */
        reSTFormat.setFontWeight (QFont::Bold);
        reSTFormat.setForeground (DarkMagenta);
        rule.pattern.setPattern ("^\\s*(\\*|-|\\+|#\\.|[0-9]+\\.|[0-9]+\\)|\\([0-9]+\\)|[a-zA-Z]\\.|[a-zA-Z]\\)|\\([a-zA-Z]\\))\\s+");
        rule.format = reSTFormat;
        highlightingRules.append (rule);


        /* verbatim */
        quoteFormat.setForeground (Blue);
        rule.pattern.setPattern ("(?<=[\\s\\(\\[{<:'\\\"/]|^)``"
                                 "([^`\\s]((?!``).)*[^`\\s]"
                                 "|"
                                 "[^`\\s])"
                                 "``(?=[\\s\\)\\]}>.,;:\\-'\\\"/\\\\]|$)");
        rule.format = quoteFormat;
        highlightingRules.append (rule);

        /* links */
        reSTFormat = urlFormat;
        rule.pattern.setPattern ("(?<=[\\s\\(\\[{<:'\\\"/]|^)"
                                 "(`(\\S+|\\S((?!`_ ).)*\\S)`|\\w+|\\[\\w+\\])"
                                 "_(?=[\\s\\)\\]}>.,;:\\-'\\\"/\\\\]|$)");
        rule.format = reSTFormat;
        highlightingRules.append (rule);

        reSTFormat = neutralFormat;

        /* bold */
        reSTFormat.setFontWeight (QFont::Bold);
        rule.pattern.setPattern ("(?<=[\\s\\(\\[{<:'\\\"/]|^)\\*\\*"
                                 "([^*\\s]|[^*\\s][^*]*[^*\\s])"
                                 "\\*\\*(?=[\\s\\)\\]}>.,;:\\-'\\\"/\\\\]|$)");
        rule.format = reSTFormat;
        highlightingRules.append (rule);
        reSTFormat.setFontWeight (QFont::Normal);

        /* italic */
        reSTFormat.setFontItalic (true);
        rule.pattern.setPattern ("(?<=[\\s\\(\\[{<:'\\\"/]|^)\\*"
                                 "([^*\\s]|[^*\\s]+[^*]*[^*\\s])"
                                 "\\*(?=[\\s\\)\\]}>.,;:\\-'\\\"/\\\\]|$)");
        rule.format = reSTFormat;
        highlightingRules.append (rule);
        reSTFormat.setFontItalic (false);

        /* labels and substitutions (".. _X:" and ".. |X| Y::") */
        reSTFormat.setForeground (Violet);
        rule.pattern.setPattern ("^\\s*\\.{2} _[\\w\\s\\-+]*:(?!\\S)");
        rule.format = reSTFormat;
        highlightingRules.append (rule);
        rule.pattern.setPattern ("^\\s*\\.{2} \\|[\\w\\s]+\\|\\s+\\w+::(?!\\S)");
        rule.format = reSTFormat;
        highlightingRules.append (rule);

        /* aliases ("|X|") */
        rule.pattern.setPattern ("(?<=[\\s\\(\\[{<:'\\\"/]|^)"
                                 "\\|(?!\\s)[^|]+\\|"
                                 "(?=[\\s\\)\\]}>.,;:\\-'\\\"/\\\\]|$)");
        rule.format = reSTFormat;
        highlightingRules.append (rule);

        /* footnotes (".. [#X]") */
        rule.pattern.setPattern ("^\\s*\\.{2} (\\[(\\w|\\s|-|\\+|\\*)+\\]|\\[#(\\w|\\s|-|\\+)*\\])\\s+");
        rule.format = reSTFormat;
        highlightingRules.append (rule);

        /* references */
        reSTFormat.setForeground (Brown);
        rule.pattern.setPattern (":[\\w\\-+]+:`[^`]*`");
        rule.format = reSTFormat;
        highlightingRules.append (rule);

        /* ".. X::" (like literalinclude) */
        reSTFormat.setFontWeight (QFont::Bold);
        reSTFormat.setForeground (DarkGreen);
        //rule.pattern.setPattern ("^\\s*\\.{2} literalinclude::(?!\\S)");
        rule.pattern.setPattern ("^\\s*\\.{2} ((\\w|-)+::(?!\\S)|code-block::)");
        rule.format = reSTFormat;
        highlightingRules.append (rule);

        /* ":X:" */
        reSTFormat.setFontWeight (QFont::Normal);
        reSTFormat.setForeground (DarkMagenta);
        rule.pattern.setPattern ("^\\s*:[^:]+:(?!\\S)");
        rule.format = reSTFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "lua")
    {
        QTextCharFormat luaFormat;
        luaFormat.setFontWeight (QFont::Bold);
        luaFormat.setFontItalic (true);
        luaFormat.setForeground (DarkMagenta);
        rule.pattern.setPattern ("\\bos(?=\\.)");
        rule.format = luaFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "m3u")
    {
        QTextCharFormat plFormat = neutralFormat;
        plFormat.setFontWeight (QFont::Bold);
        rule.pattern.setPattern ("^#EXTM3U\\b");
        rule.format = plFormat;
        highlightingRules.append (rule);

        /* after "," */
        plFormat.setFontWeight (QFont::Normal);
        plFormat.setForeground (DarkRed);
        rule.pattern.setPattern ("^#EXTINF\\s*:\\s*-*\\d+\\s*,.*|^#EXTINF\\s*:\\s*,.*");
        rule.format = plFormat;
        highlightingRules.append (rule);

        /* before "," and after "EXTINF:" */
        plFormat.setForeground (DarkYellow);
        rule.pattern.setPattern ("^#EXTINF\\s*:\\s*-*\\d+\\b");
        rule.format = plFormat;
        highlightingRules.append (rule);

        plFormat = neutralFormat;
        rule.pattern.setPattern ("^#EXTINF\\s*:");
        rule.format = plFormat;
        highlightingRules.append (rule);

        plFormat.setForeground (DarkGreen);
        rule.pattern.setPattern ("^#EXTINF\\b");
        rule.format = plFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "scss")
    {
        /* scss supports nested css blocks but, instead of making its highlighting complex,
           we format it without considering that and so, without syntax error, but with keywords() */

        QTextCharFormat scssFormat;

        /* definitions (starting with @) */
        scssFormat.setForeground (Brown);
        rule.pattern.setPattern ("\\s*@[A-Za-z-]+\\s+|;\\s*@[A-Za-z-]+\\s+");
        rule.format = scssFormat;
        highlightingRules.append (rule);

        /* numbers */
        scssFormat.setFontItalic (true);
        rule.pattern.setPattern ("(-|\\+){0,1}\\b\\d*\\.{0,1}\\d+%*");
        rule.format = scssFormat;
        highlightingRules.append (rule);

        /* colors */
        scssFormat.setForeground (Verda);
        scssFormat.setFontWeight (QFont::Bold);
        rule.pattern.setPattern ("#([A-Fa-f0-9]{3}){0,2}(?![A-Za-z0-9_]+)|#([A-Fa-f0-9]{3}){2}[A-Fa-f0-9]{2}(?![A-Za-z0-9_]+)");
        rule.format = scssFormat;
        highlightingRules.append (rule);


        /* before :...; */
        scssFormat.setForeground (Blue);
        scssFormat.setFontItalic (false);
        /* exclude note patterns artificially */
        rule.pattern.setPattern ("[A-Za-z0-9_\\-]+(?<!\\bNOTE|\\bTODO|\\bFIXME|\\bWARNING)(?=\\s*:.*;*)");
        rule.format = scssFormat;
        highlightingRules.append (rule);
        scssFormat.setFontWeight (QFont::Normal);

        /* variables ($...) */
        scssFormat.setFontItalic (true);
        rule.pattern.setPattern ("\\$[A-Za-z0-9_\\-]+");
        rule.format = scssFormat;
        highlightingRules.append (rule);
    }

    if (showWhiteSpace)
    {
        rule.pattern.setPattern ("\\s+");
        rule.format = whiteSpaceFormat;
        highlightingRules.append (rule);
    }

    /************
     * Comments *
     ************/

    /* single line comments */
    rule.pattern.setPattern (QString());
    if (progLan == "c" || progLan == "cpp"
        || Lang == "javascript" || progLan == "qml"
        || progLan == "php" || progLan == "scss")
    {
        rule.pattern.setPattern ("//.*"); // why had I set it to ("//(?!\\*).*")?
    }
    else if (progLan == "python"
             || progLan == "qmake"
             || progLan == "gtkrc")
    {
        rule.pattern.setPattern ("#.*"); // or "#[^\n]*"
    }
    else if (progLan == "desktop" || progLan == "config" || progLan == "theme")
    {
        rule.pattern.setPattern ("^\\s*#.*"); // only at start
    }
    /*else if (progLan == "deb")
    {
        rule.pattern.setPattern ("^#[^\\s:]+:(?=\\s*)");
    }*/
    else if (progLan == "m3u")
    {
        rule.pattern.setPattern ("^\\s+#|^#(?!(EXTM3U|EXTINF))");
    }
    else if (progLan == "lua")
        rule.pattern.setPattern ("--(?!\\[).*");
    else if (progLan == "troff")
        rule.pattern.setPattern ("\\\\\"|\\.\\s*\\\\\"");
    if (!rule.pattern.pattern().isEmpty())
    {
        rule.format = commentFormat;
        highlightingRules.append (rule);
    }

    /* multiline comments */
    if (progLan == "c" || progLan == "cpp"
        || progLan == "javascript" || progLan == "qml"
        || progLan == "php" || progLan == "css" || progLan == "scss"
        || progLan == "fountain")
    {
        commentStartExpression.setPattern ("/\\*");
        commentEndExpression.setPattern ("\\*/");
    }
    else if (progLan == "lua")
    {
        commentStartExpression.setPattern ("\\[\\[|--\\[\\[");
        commentEndExpression.setPattern ("\\]\\]");
    }
    else if (progLan == "python")
    {
        commentStartExpression.setPattern ("\"\"\"|\'\'\'");
        commentEndExpression = commentStartExpression;
    }
    else if (progLan == "xml" || progLan == "html")
    {
        commentStartExpression.setPattern ("<!--");
        commentEndExpression.setPattern ("-->");
    }
    else if (progLan == "perl")
    {
        commentStartExpression.setPattern ("^=[A-Za-z0-9_]+($|\\s+)");
        commentEndExpression.setPattern ("^=cut.*");
    }
    else if (progLan == "markdown")
    {
        quoteFormat.setForeground (DarkRed); // not a quote but a code block
        urlInsideQuoteFormat.setForeground (DarkRed);
        commentStartExpression.setPattern ("<!--");
        commentEndExpression.setPattern ("-->");
    }
}
/*************************/
Highlighter::~Highlighter()
{
    if (QTextDocument *doc = document())
    {
        QTextOption opt =  doc->defaultTextOption();
        opt.setFlags (opt.flags() & ~QTextOption::ShowTabsAndSpaces
                                  & ~QTextOption::ShowLineAndParagraphSeparators
                                  & ~QTextOption::AddSpaceForLineAndParagraphSeparators
                                  & ~QTextOption::ShowDocumentTerminator);
        doc->setDefaultTextOption (opt);
    }
}
/*************************/
// Should be used only with characters that can be escaped in a language.
bool Highlighter::isEscapedChar (const QString &text, const int pos) const
{
    if (pos < 1) return false;
    int i = 0;
    while (pos - i - 1 >= 0 && text.at (pos - i - 1) == '\\')
        ++i;
    if (i % 2 != 0)
        return true;
    return false;
}
/*************************/
// Check if a start or end quotation mark (positioned at "pos") is escaped.
// If "skipCommandSign" is true (only for SH), start double quotes are escaped before "$(".
bool Highlighter::isEscapedQuote (const QString &text, const int pos, bool isStartQuote,
                                  bool skipCommandSign)
{
    if (pos < 0) return false;

    if (progLan == "html" || progLan == "xml")
        return false;

    if (progLan == "yaml")
    {
        if (isStartQuote)
        {
            if (format (pos) == codeBlockFormat) // inside a block
                return true;
            /* skip the start quote if it's inside a key or value */
            QRegularExpressionMatch match;
            if (format (pos) == neutralFormat)
            { // inside preformatted braces, when multiLineQuote() is called (not needed; repeated below)
                int index = text.lastIndexOf (QRegularExpression ("(^|{|,|\\[)\\s*\\K[^:,#]*:\\s+"), pos, &match);
                if (index > -1 && index < pos && index + match.capturedLength() > pos)
                    return true;
                index = text.lastIndexOf (QRegularExpression ("(^|{|,|\\[)[^:#]*:\\s+\\K[^{\\[,#\\s][^,#]*"), pos, &match);
                if (index > -1 && index < pos && index + match.capturedLength() > pos)
                    return true;
            }
            else
            {
                /* inside braces before preformatting (indirectly used by yamlOpenBraces()) */
                int index = text.lastIndexOf (QRegularExpression ("(^|{|,|\\[)\\s*\\K[^:,#]*:\\s+"), pos, &match);
                if (index > -1 && index < pos && index + match.capturedLength() > pos)
                    return true;
                index = text.lastIndexOf (QRegularExpression ("(^|{|,|\\[)[^:#]*:\\s+\\K[^{\\[,#\\s][^,#]*"), pos, &match);
                if (index > -1 && index < pos && index + match.capturedLength() > pos)
                    return true;
                /* outside braces */
                index = text.lastIndexOf (QRegularExpression ("^[^:#]*:\\s+"), pos, &match);
                if (index > -1 && index < pos && index + match.capturedLength() > pos)
                    return true;
                index = text.lastIndexOf (QRegularExpression ("^[^:#]*:\\s+\\K[^\\[\\s#].*"), pos, &match);
                if (index > -1 && index < pos && index + match.capturedLength() > pos)
                    return true;
            }
        }
        else if (text.length() > pos && text.at (pos) == '\'')
        { // a pair of single quotes means escaping them
            QRegularExpressionMatch match;
            if (text.lastIndexOf (QRegularExpression ("('')+"), pos) == pos)
                return true;
            int index = text.lastIndexOf (QRegularExpression ("('')+"), pos, &match);
            if (index > -1 && index < pos && index + match.capturedLength() >= pos)
                return true;
        }
    }

    /* there's no need to check for quote marks because this function is used only with them */
    /*if (progLan == "perl"
        && pos != text.indexOf (quoteMark, pos)
        && pos != text.indexOf ("\'", pos)
        && pos != text.indexOf ("`", pos))
    {
        return false;
    }
    if (pos != text.indexOf (quoteMark, pos)
        && (progLan == "markdown" || pos != text.indexOf (QRegularExpression ("\'"), pos)))
    {
        return false;
    }*/

    /* check if the quote surrounds a here-doc delimiter */
    if ((currentBlockState() >= endState || currentBlockState() < -1)
        && currentBlockState() % 2 == 0)
    {
        QRegularExpressionMatch match;
        QRegularExpression delimPart ("<<\\s*");
        QRegularExpressionMatch match1;
        QRegularExpression delimPart1;
        if (progLan == "perl") // space is allowed
            delimPart1.setPattern ("<<(?:\\s*)(\'[A-Za-z0-9_\\s]+)|<<(?:\\s*)(\"[A-Za-z0-9_\\s]+)|<<(?:\\s*)(`[A-Za-z0-9_\\s]+)");
        else
            delimPart1.setPattern ("<<(?:\\s*)(\'[A-Za-z0-9_]+)|<<(?:\\s*)(\"[A-Za-z0-9_]+)");
        if (text.lastIndexOf (delimPart, pos, &match) == pos - match.capturedLength()
            || text.lastIndexOf (delimPart1, pos, &match1) == pos - match1.capturedLength())
        {
            return true;
        }
    }

    /* escaped start quotes are just for Bash, Perl, markdown and yaml */
    if (isStartQuote)
    {
        if (progLan == "perl")
        {
            if (pos >= 1)
            {
                if (text.at (pos - 1) == '$') // in Perl, $' has a (deprecated?) meaning
                    return true;
                if (text.at (pos) == '\'')
                {
                    if (text.at (pos - 1) == '&')
                        return true;
                    int index = pos - 1;
                    while (index >= 0 && (text.at (index).isLetterOrNumber() || text.at (index) == '_'))
                        -- index;
                    if (index >= 0 && (text.at (index) == '$' || text.at (index) == '@'
                                       || text.at (index) == '%' || text.at (index) == '*'
                                       /*|| text.at (index) == '!'*/))
                    {
                        return true;
                    }
                }
            }
            return false; // no other case of escaping at the start
        }
        else if (progLan != "sh" && progLan != "makefile" && progLan != "cmake"
                 && progLan != "markdown" && progLan != "yaml")
        {
            return false;
        }
    }

    if (isStartQuote && skipCommandSign && text.at (pos) == quoteMark.pattern().at (0)
        && text.indexOf (QRegularExpression ("[^\"]*\\$\\("), pos) == pos + 1)
    {
        return true;
    }

    int i = 0;
    while (pos - i > 0 && text.at (pos - i - 1) == '\\')
        ++i;
    /* only an odd number of backslashes means that the quote is escaped */
    if (
        i % 2 != 0
        && ((progLan == "yaml"
             && text.at (pos) == quoteMark.pattern().at (0))
            /* for these languages, both single and double quotes can be escaped (also for perl?) */
            || progLan == "c" || progLan == "cpp"
            || progLan == "javascript" || progLan == "qml"
            || progLan == "python"
            || progLan == "perl"
            /* markdown is an exception */
            || progLan == "markdown"
            /* however, in Bash, single quote can be escaped only at start */
            || ((progLan == "sh" || progLan == "makefile" || progLan == "cmake")
                && (isStartQuote || text.at (pos) == quoteMark.pattern().at (0))))
       )
    {
        return true;
    }

    return false;
}
/*************************/
// Checks if a character is inside quotation marks, considering the language
// (should be used with care because it gives correct results only in special places).
// If "skipCommandSign" is true (only for SH), start double quotes are escaped before "$(".
bool Highlighter::isQuoted (const QString &text, const int index,
                            bool skipCommandSign)
{
    if (progLan == "perl") return isPerlQuoted (text, index);

    if (index < 0) return false;

    int pos = -1;

    /* with regex, the text will be formatted below to know whether
       the regex start sign is quoted (-> isEscapedRegex) */
    bool hasRegex (progLan == "javascript" || progLan == "qml");
    if (hasRegex)
    {
        if (format (index) == quoteFormat || format (index) == altQuoteFormat)
            return true;
        if (TextBlockData *data = static_cast<TextBlockData *>(currentBlock().userData()))
        {
            pos = data->lastFormattedQuote() - 1;
            if (index <= pos) return false;
        }
    }

    bool res = false;
    int N;
    bool mixedQuotes = false;
    if (hasRegex
        || progLan == "c" || progLan == "cpp"
        || progLan == "python" || progLan == "sh"
        || progLan == "makefile" || progLan == "cmake"
        || progLan == "lua"
        || progLan == "xml" // never used with xml; otherwise, we should consider "&quot;"
        || progLan == "ruby" || progLan == "html" || progLan == "scss"
        || progLan == "yaml")
    {
        mixedQuotes = true;
    }
    QRegularExpression quoteExpression;
    if (mixedQuotes)
        quoteExpression.setPattern ("\"|\'");
    else
        quoteExpression = quoteMark;
    if (pos == -1)
    {
        int prevState = previousBlockState();
        if ((prevState < doubleQuoteState
             || prevState > SH_MixedSingleQuoteState)
                && prevState != htmlStyleSingleQuoteState
                && prevState != htmlStyleDoubleQuoteState)
        {
            N = 0;
        }
        else
        {
            N = 1;
            res = true;
            if (mixedQuotes)
            {
                if (prevState == doubleQuoteState
                    || prevState == SH_DoubleQuoteState
                    || prevState == SH_MixedDoubleQuoteState
                    || prevState == htmlStyleDoubleQuoteState)
                {
                    quoteExpression = quoteMark;
                    if (skipCommandSign)
                    {
                        if (text.indexOf (QRegularExpression ("[^\"]*\\$\\("), 0) == 0)
                        {
                            N = 0;
                            res = false;
                        }
                        else
                        {
                            QTextBlock prevBlock = currentBlock().previous();
                            if (prevBlock.isValid())
                            {
                                if (TextBlockData *prevData = static_cast<TextBlockData *>(prevBlock.userData()))
                                {
                                    int N = prevData->openNests();
                                    if (N > 0 && (prevState == doubleQuoteState || !prevData->openQuotes().contains (N)))
                                    {
                                        N = 0;
                                        res = false;
                                    }
                                }
                            }
                        }
                    }
                }
                else
                    quoteExpression.setPattern ("\'");
            }
        }
    }
    else N = 0; // a new search from the last position

    int nxtPos;
    while ((nxtPos = text.indexOf (quoteExpression, pos + 1)) >= 0)
    {
        /* skip formatted comments */
        if (format (nxtPos) == commentFormat
            || (N % 2 == 0 && hasRegex
                && (isMLCommented (text, nxtPos, commentState)
                    || isMLCommented (text, nxtPos, htmlJavaCommentState))))
        {
            pos = nxtPos;
            continue;
        }

        ++N;
        if ((N % 2 == 0 // an escaped end quote...
             && isEscapedQuote (text, nxtPos, false))
            || (N % 2 != 0 // ... or an escaped start quote Bash
                && (isEscapedQuote (text, nxtPos, true, skipCommandSign) || isInsideRegex (text, nxtPos))))
        {
            if (res && hasRegex)
            { // -> isEscapedRegex()
                pos = qMax (pos, 0);
                if (text.at (nxtPos) == quoteMark.pattern().at (0))
                    setFormat (pos, nxtPos - pos + 1, quoteFormat);
                else
                    setFormat (pos, nxtPos - pos + 1, altQuoteFormat);
            }
            --N;
            pos = nxtPos;
            continue;
        }

        if (N % 2 == 0 && hasRegex)
        { // -> isEscapedRegex()
            if (TextBlockData *data = static_cast<TextBlockData *>(currentBlock().userData()))
                data->insertLastFormattedQuote (nxtPos + 1);
            pos = qMax (pos, 0);
            if (text.at (nxtPos) == quoteMark.pattern().at (0))
                setFormat (pos, nxtPos - pos + 1, quoteFormat);
            else
                setFormat (pos, nxtPos - pos + 1, altQuoteFormat);
        }

        if (index < nxtPos)
        {
            if (N % 2 == 0) res = true;
            else res = false;
            break;
        }

        /* "pos" might be negative next time */
        if (N % 2 == 0) res = false;
        else res = true;

        if (mixedQuotes)
        {
            if (N % 2 != 0)
            { // each quote neutralizes the other until it's closed
                if (text.at (nxtPos) == quoteMark.pattern().at (0))
                    quoteExpression = quoteMark;
                else
                    quoteExpression.setPattern ("\'");
            }
            else
                quoteExpression.setPattern ("\"|\'");
        }
        pos = nxtPos;
    }

    return res;
}
/*************************/
// Perl has a separate method to support backquotes.
// Also see multiLinePerlQuote().
bool Highlighter::isPerlQuoted (const QString &text, const int index)
{
    if (index < 0) return false;

    int pos = -1;

    if (format (index) == quoteFormat || format (index) == altQuoteFormat)
        return true;
    if (TextBlockData *data = static_cast<TextBlockData *>(currentBlock().userData()))
    {
        pos = data->lastFormattedQuote() - 1;
        if (index <= pos) return false;
    }

    bool res = false;
    int N;
    QRegularExpression quoteExpression ("\"|\'|`");
    if (pos == -1)
    {
        int prevState = previousBlockState();
        if (prevState != doubleQuoteState && prevState != singleQuoteState)
            N = 0;
        else
        {
            N = 1;
            res = true;
            if (prevState == doubleQuoteState)
                quoteExpression = quoteMark;
            else
            {
                bool backquoted (false);
                QTextBlock prevBlock = currentBlock().previous();
                if (prevBlock.isValid())
                {
                    TextBlockData *prevData = static_cast<TextBlockData *>(prevBlock.userData());
                    if (prevData && prevData->getProperty())
                        backquoted = true;
                }
                if (backquoted)
                    quoteExpression.setPattern ("`");
                else
                    quoteExpression.setPattern ("\'");
            }
        }
    }
    else N = 0; // a new search from the last position

    int nxtPos;
    while ((nxtPos = text.indexOf (quoteExpression, pos + 1)) >= 0)
    {
        /* skip formatted comments */
        if (format (nxtPos) == commentFormat
            || (N % 2 == 0 && isMLCommented (text, nxtPos, commentState)))
        {
            pos = nxtPos;
            continue;
        }

        ++N;
        if ((N % 2 == 0 // an escaped end quote...
             && isEscapedQuote (text, nxtPos, false))
            || (N % 2 != 0 // ... or an escaped start quote
                && (isEscapedQuote (text, nxtPos, true, false) || isInsideRegex (text, nxtPos))))
        {
            if (res)
            { // -> isEscapedRegex()
                pos = qMax (pos, 0);
                if (text.at (nxtPos) == quoteMark.pattern().at (0))
                    setFormat (pos, nxtPos - pos + 1, quoteFormat);
                else
                    setFormat (pos, nxtPos - pos + 1, altQuoteFormat);
            }
            --N;
            pos = nxtPos;
            continue;
        }

        if (N % 2 == 0)
        { // -> isEscapedRegex()
            if (TextBlockData *data = static_cast<TextBlockData *>(currentBlock().userData()))
                data->insertLastFormattedQuote (nxtPos + 1);
            pos = qMax (pos, 0);
            if (text.at (nxtPos) == quoteMark.pattern().at (0))
                setFormat (pos, nxtPos - pos + 1, quoteFormat);
            else
                setFormat (pos, nxtPos - pos + 1, altQuoteFormat);
        }

        if (index < nxtPos)
        {
            if (N % 2 == 0) res = true;
            else res = false;
            break;
        }

        /* "pos" might be negative next time */
        if (N % 2 == 0) res = false;
        else res = true;

        if (N % 2 != 0)
        { // each quote neutralizes the other until it's closed
            if (text.at (nxtPos) == quoteMark.pattern().at (0))
                quoteExpression = quoteMark;
            else if (text.at (nxtPos) == '\'')
                quoteExpression.setPattern ("\'");
            else
                quoteExpression.setPattern ("`");
        }
        else
            quoteExpression.setPattern ("\"|\'");
        pos = nxtPos;
    }

    return res;
}
/*************************/
// Checks if a start quote or a start single-line comment sign is inside a multiline comment.
// If "start" > 0, it will be supposed that "start" is not inside a previous comment.
// (May give an incorrect result with other characters and works only with real comments
// whose state is "comState".)
bool Highlighter::isMLCommented (const QString &text, const int index, int comState,
                                 const int start)
{
    if (index < 0 || start < 0 || index < start
        // commentEndExpression is always set if commentStartExpression is
        || commentStartExpression.pattern().isEmpty())
    {
        return false;
    }

    /* not for Python */
    if (progLan == "python") return false;

    int prevState = previousBlockState();
    if (prevState == nextLineCommentState)
        return true; // see singleLineComment()

    bool res = false;
    int pos = start - 1;
    int N;
    QRegularExpressionMatch commentMatch;
    QRegularExpression commentExpression;
    if (pos >= 0 || prevState != comState)
    {
        N = 0;
        commentExpression = commentStartExpression;
    }
    else
    {
        N = 1;
        res = true;
        commentExpression = commentEndExpression;
    }

    while ((pos = text.indexOf (commentExpression, pos + 1, &commentMatch)) >= 0)
    {
        /* skip formatted quotations */
        QTextCharFormat fi = format (pos);
        if (fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat)
            continue;

        ++N;

        /* All (or most) multiline comments have more than one character
           and this trick is needed for knowing if a double slash follows
           an asterisk without using "lookbehind", for example. */
        if (index < pos + (N % 2 == 0 ? commentMatch.capturedLength() : 0))
        {
            if (N % 2 == 0) res = true;
            else res = false;
            break;
        }

        if (N % 2 != 0)
        {
            commentExpression = commentEndExpression;
            res = true;
        }
        else
        {
            commentExpression = commentStartExpression;
            res = false;
        }
    }

    return res;
}
/*************************/
// This handles multiline python comments separately because they aren't normal.
// It comes after singleLineComment() and before multiLineQuote().
void Highlighter::pythonMLComment (const QString &text, const int indx)
{
    if (progLan != "python") return;

    QTextCharFormat noteFormat;
    noteFormat.setFontWeight (QFont::Bold);
    noteFormat.setFontItalic (true);
    noteFormat.setForeground (DarkRed);

    /* we reset the block state because this method is also called
       during the multiline quotation formatting after clearing formats */
    setCurrentBlockState (-1);

    int index = indx;
    int quote;

    /* find the start quote */
    int prevState = previousBlockState();
    if (prevState != pyDoubleQuoteState
        && prevState != pySingleQuoteState)
    {
        index = text.indexOf (commentStartExpression, indx);

        QTextCharFormat fi = format (index);
        while ((index > 0 && isQuoted (text, index - 1)) // because two quotes may follow an end quote
               || fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat) // not needed
        {
            index = text.indexOf (commentStartExpression, index + 3);
            fi = format (index);
        }
        if (format (index) == commentFormat)
            return;

        /* if the comment start is found... */
        if (index >= indx)
        {
            /* ... distinguish between double and single quotes */
            if (index == text.indexOf (QRegularExpression ("\"\"\""), index))
            {
                commentStartExpression.setPattern ("\"\"\"");
                quote = pyDoubleQuoteState;
            }
            else
            {
                commentStartExpression.setPattern ("\'\'\'");
                quote = pySingleQuoteState;
            }
        }
    }
    else // but if we're inside a triple quotation...
    {
        /* ... distinguish between the two quote kinds
           by checking the previous line */
        quote = prevState;
        if (quote == pyDoubleQuoteState)
            commentStartExpression.setPattern ("\"\"\"");
        else
            commentStartExpression.setPattern ("\'\'\'");
    }

    while (index >= indx)
    {
        /* if the search is continued... */
        if (commentStartExpression.pattern() == "\"\"\"|\'\'\'")
        {
            /* ... distinguish between double and single quotes
               again because the quote mark may have changed... */
            if (text.at (index) == quoteMark.pattern().at (0))
            {
                commentStartExpression.setPattern ("\"\"\"");
                quote = pyDoubleQuoteState;
            }
            else
            {
                commentStartExpression.setPattern ("\'\'\'");
                quote = pySingleQuoteState;
            }
        }

        /* search for the end quote from the start quote */
        QRegularExpressionMatch startMatch;
        int endIndex = text.indexOf (commentStartExpression, index + 3, &startMatch);

        /* but if there's no start quote ... */
        if (index == indx
            && (prevState == pyDoubleQuoteState || prevState == pySingleQuoteState))
        {
            /* ... search for the end quote from the line start */
            endIndex = text.indexOf (commentStartExpression, indx, &startMatch);
        }

        /* check if the quote is escaped */
        while ((endIndex > 0 && text.at (endIndex - 1) == '\\'
                /* backslash shouldn't be escaped itself */
                && (endIndex < 2 || text.at (endIndex - 2) != '\\'))
                   /* also consider ^' and ^" */
                   || ((endIndex > 0 && text.at (endIndex - 1) == '^')
                       && (endIndex < 2 || text.at (endIndex - 2) != '\\')))
        {
            endIndex = text.indexOf (commentStartExpression, endIndex + 3, &startMatch);
        }

        int quoteLength;
        if (endIndex == -1)
        {
            setCurrentBlockState (quote);
            quoteLength = text.length() - index;
        }
        else
            quoteLength = endIndex - index
                          + startMatch.capturedLength(); // 3
        setFormat (index, quoteLength, commentFormat);

        /* format urls and email addresses inside the comment */
        QString str = text.mid (index, quoteLength);
        int pIndex = 0;
        QRegularExpressionMatch urlMatch;
        while ((pIndex = str.indexOf (urlPattern, pIndex, &urlMatch)) > -1)
        {
            setFormat (pIndex + index, urlMatch.capturedLength(), urlFormat);
            pIndex += urlMatch.capturedLength();
        }
        /* format note patterns too */
        pIndex = 0;
        while ((pIndex = str.indexOf (notePattern, pIndex, &urlMatch)) > -1)
        {
            if (format (pIndex + index) != urlFormat)
                setFormat (pIndex + index, urlMatch.capturedLength(), noteFormat);
            pIndex += urlMatch.capturedLength();
        }

        /* the next quote may be different */
        commentStartExpression.setPattern ("\"\"\"|\'\'\'");
        index = text.indexOf (commentStartExpression, index + quoteLength);
        QTextCharFormat fi = format (index);
        while ((index > 0 && isQuoted (text, index - 1))
               || fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat)
        {
            index = text.indexOf (commentStartExpression, index + 3);
            fi = format (index);
        }
        if (format (index) == commentFormat)
            return;
    }
}
/*************************/
// This should come before multiline comments highlighting.
int Highlighter::cssHighlighter (const QString &text, bool mainFormatting, const int start)
{
    if (progLan != "css") return -1;

    int cssIndx = -1;
    /* CSS can have huge lines, which will take
       a lot of CPU time if they're formatted completely. */
    /*bool hugeText (text.length() > 50000);
    if (hugeText)
        mainFormatting = false;*/

    /**************************
     * (Multiline) CSS Blocks *
     **************************/

    QRegularExpressionMatch cssStartMatch;
    QRegularExpression cssStartExpression ("\\{");
    QRegularExpressionMatch cssEndtMatch;
    QRegularExpression cssEndExpression ("\\}");
    QRegularExpressionMatch numMatch;
    QRegularExpression numExpression ("(-|\\+){0,1}\\b\\d*\\.{0,1}\\d+");
    int index = start;

    QTextCharFormat cssValueFormat;
    cssValueFormat.setFontItalic (true);
    cssValueFormat.setForeground (Verda);

    QTextCharFormat numFormat;
    numFormat.setFontItalic (true);
    numFormat.setForeground (Brown);

    /*QTextCharFormat cssErrorFormat;
    cssErrorFormat.setFontUnderline (true);
    cssErrorFormat.setForeground (Red);*/

    int prevState = previousBlockState();
    if (index > 0
        || (prevState != cssBlockState
            && prevState != commentInCssState
            && prevState != cssValueState))
    {
        index = text.indexOf (cssStartExpression, index, &cssStartMatch);
        if (index >= 0) cssIndx = index;
    }

    while (index >= 0)
    {
        /*if (hugeText)
            setFormat (index, text.length() - index, translucentFormat);*/
        int endIndex;
        /* when the css block starts in the previous line
           and the search for its end has just begun... */
        if ((prevState == cssBlockState
             || prevState == commentInCssState
             || prevState == cssValueState) // subset of cssBlockState
            && index == 0)
            /* ... search for its end from the line start */
            endIndex = text.indexOf (cssEndExpression, 0, &cssEndtMatch);
        else
            endIndex = text.indexOf (cssEndExpression,
                                     index + cssStartMatch.capturedLength(),
                                     &cssEndtMatch);

        int cssLength;
        if (endIndex == -1)
        {
            endIndex = text.length() - 1;
            setCurrentBlockState (cssBlockState);
            cssLength = text.length() - index;
        }
        else
            cssLength = endIndex - index
                        + cssEndtMatch.capturedLength();

        if (mainFormatting)
        {
            /* at first, we suppose all syntax is wrong */
            QRegularExpressionMatch match;
            QRegularExpression expression ("[^\\{\\}\\s]+");
            int indxTmp = text.indexOf (expression, index, &match);
            while (isQuoted (text, indxTmp))
                indxTmp = text.indexOf (expression, indxTmp + 1, &match);
            while (indxTmp >= 0 && indxTmp < endIndex)
            {
                setFormat (indxTmp, match.capturedLength(), neutralFormat);
                indxTmp = text.indexOf (expression, indxTmp + match.capturedLength(), &match);
            }

            /* css attribute format (before :...;) */
            QTextCharFormat cssAttFormat;
            cssAttFormat.setFontItalic (true);
            cssAttFormat.setForeground (Blue);
            expression.setPattern ("[A-Za-z0-9_\\-]+(?=\\s*(?<!:):(?!:))");
            indxTmp = text.indexOf (expression, index, &match);
            while (isQuoted (text, indxTmp))
                indxTmp = text.indexOf (expression, indxTmp + 1, &match);
            while (indxTmp >= 0 && indxTmp < endIndex)
            {
                setFormat (indxTmp, match.capturedLength(), cssAttFormat);
                indxTmp = text.indexOf (expression, indxTmp + match.capturedLength(), &match);
            }
        }

        index = text.indexOf (cssStartExpression, index + cssLength, &cssStartMatch);
    }

    /**************************
     * (Multiline) CSS Values *
     **************************/

    cssStartExpression.setPattern ("(?<!:):(?!:)");
    cssEndExpression.setPattern (";|\\}");
    index = 0;
    if (prevState != cssValueState || start > 0)
    {
        index = text.indexOf (cssStartExpression, start, &cssStartMatch);
        if (index > -1)
        {
            while (format (index) != neutralFormat)
            {
                index = text.indexOf (cssStartExpression, index + 1, &cssStartMatch);
                if (index == -1) break;
            }
        }
    }

    while (index >= 0)
    {
        int endIndex;
        int startMatch = 0;
        if (prevState == cssValueState
            && index == 0)
            endIndex = text.indexOf (cssEndExpression, 0, &cssEndtMatch);
        else
        {
            startMatch = cssStartMatch.capturedLength();
            endIndex = text.indexOf (cssEndExpression,
                                     index + startMatch,
                                     &cssEndtMatch);
        }

        int cssLength;
        if (endIndex == -1)
        {
            setCurrentBlockState (cssValueState);
            cssLength = text.length() - index;
        }
        else
            cssLength = endIndex - index
                        + cssEndtMatch.capturedLength();
        if (mainFormatting)
        {
            /* css value format */
            setFormat (index, cssLength, cssValueFormat);

            /* numbers in css values */
            int nIndex = text.indexOf (numExpression, index + startMatch, &numMatch);
            while (nIndex > -1
                   && nIndex + numMatch.capturedLength() <= index + cssLength)
            {
                setFormat (nIndex, numMatch.capturedLength(), numFormat);
                nIndex = text.indexOf (numExpression, nIndex + numMatch.capturedLength(), &numMatch);
            }

            setFormat (index, startMatch, mainFormat);
            if (endIndex > -1)
                setFormat (endIndex, 1, mainFormat);
        }

        index = text.indexOf (cssStartExpression, index + cssLength, &cssStartMatch);
        if (index > -1)
        {
            if (!mainFormatting) break; // there's no neutralFormat
            while (format (index) != neutralFormat)
            {
                index = text.indexOf (cssStartExpression, index + 1, &cssStartMatch);
                if (index == -1) break;
            }
        }
    }

    if (mainFormatting)
    {
        /* color value format (#xyz, #abcdef, #abcdefxy) */
        QTextCharFormat cssColorFormat;

        cssColorFormat.setForeground (Verda);
        cssColorFormat.setFontWeight (QFont::Bold);
        cssColorFormat.setFontItalic (true);
        QRegularExpressionMatch match;
        // previously: "#\\b([A-Za-z0-9]{3}){0,4}(?![A-Za-z0-9_]+)"
        QRegularExpression expression ("#([A-Fa-f0-9]{3}){0,2}(?![A-Za-z0-9_]+)|#([A-Fa-f0-9]{3}){2}[A-Fa-f0-9]{2}(?![A-Za-z0-9_]+)");
        int indxTmp = text.indexOf (expression, start, &match);
        while (isQuoted (text, indxTmp))
            indxTmp = text.indexOf (expression, indxTmp + 1, &match);
        while (indxTmp >= 0)
        {
            if (/*format (indxTmp) == cssValueFormat // should be a value
                    &&*/ format (indxTmp) != neutralFormat) // not an error
            {
                setFormat (indxTmp, match.capturedLength(), cssColorFormat);
            }
            indxTmp = text.indexOf (expression, indxTmp + match.capturedLength(), &match);
        }

        /* definitions (starting with @) */
        QTextCharFormat cssDefinitionFormat;
        cssDefinitionFormat.setForeground (Brown);
        expression.setPattern ("^\\s*@[A-Za-z-]+\\s+|;\\s*@[A-Za-z-]+\\s+");
        indxTmp = text.indexOf (expression, start, &match);
        while (isQuoted (text, indxTmp))
            indxTmp = text.indexOf (expression, indxTmp + 1, &match);
        while (indxTmp >= 0)
        {
            int length = match.capturedLength();
            if (format (indxTmp) != cssValueFormat
                    && format (indxTmp) != neutralFormat)
            {
                if (text.at (indxTmp) == ';')
                {
                    ++indxTmp;
                    --length;
                }
                setFormat (indxTmp, length, cssDefinitionFormat);
            }
            indxTmp = text.indexOf (expression, indxTmp + length, &match);
        }
    }

    return cssIndx;
}
/*************************/
void Highlighter::singleLineComment (const QString &text, const int start)
{
    for (const HighlightingRule &rule : qAsConst (highlightingRules))
    {
        if (rule.format == commentFormat)
        {
            int startIndex = qMax (start, 0);
            if (previousBlockState() == nextLineCommentState)
                startIndex = 0;
            else
            {
                startIndex = text.indexOf (rule.pattern, startIndex);
                /* skip quoted comments (and, automatically, those inside multiline python comments) */
                while (startIndex > -1
                       && (isQuoted (text, startIndex) || isInsideRegex (text, startIndex)))
                {
                    startIndex = text.indexOf (rule.pattern, startIndex + 1);
                }
            }
            if (startIndex > -1)
            {
                int l = text.length();
                setFormat (startIndex, l - startIndex, commentFormat);

                /* also format urls and email addresses inside the comment */
                QString str = text.mid (startIndex, l - startIndex);
                QTextCharFormat noteFormat;
                noteFormat.setFontWeight (QFont::Bold);
                noteFormat.setFontItalic (true);
                noteFormat.setForeground (DarkRed);
                int pIndex = 0;
                QRegularExpressionMatch urlMatch;
                while ((pIndex = str.indexOf (urlPattern, pIndex, &urlMatch)) > -1)
                {
                    setFormat (pIndex + startIndex, urlMatch.capturedLength(), urlFormat);
                    pIndex += urlMatch.capturedLength();
                }
                /* format note patterns too */
                pIndex = 0;
                while ((pIndex = str.indexOf (notePattern, pIndex, &urlMatch)) > -1)
                {
                    if (format (pIndex + startIndex) != urlFormat)
                        setFormat (pIndex + startIndex, urlMatch.capturedLength(), noteFormat);
                    pIndex += urlMatch.capturedLength();
                }
                /* take care of next-line comments with languages, for which
                   no highlighting function is called after singleLineComment()
                   and before the main formatting in highlightBlock()
                   (only c and c++ for now) */
                if ((progLan == "c" || progLan == "cpp")
                    && text.endsWith (QLatin1Char('\\')))
                {
                    setCurrentBlockState (nextLineCommentState);
                }
            }
            break;
        }
    }
}
/*************************/
void Highlighter::multiLineComment (const QString &text,
                                    const int index, const int cssIndx,
                                    const QRegularExpression &commentStartExp, const QRegularExpression &commentEndExp,
                                    const int commState,
                                    const QTextCharFormat &comFormat)
{
    if (index < 0) return;
    int prevState = previousBlockState();
    if (prevState == nextLineCommentState)
        return;  // was processed by singleLineComment()

    /* CSS can have huge lines, which will take
       a lot of CPU time if they're formatted completely. */
    //bool hugeText = ((progLan == "css" || progLan == "scss" ) && text.length() > 50000);

    bool commentBeforeBrace = false; // in css, not as: "{...
    int startIndex = index;
    QTextCharFormat noteFormat;
    noteFormat.setFontWeight (QFont::Bold);
    noteFormat.setFontItalic (true);
    noteFormat.setForeground (DarkRed);

    QRegularExpressionMatch startMatch;
    QRegularExpressionMatch endMatch;

    if ((prevState != commState
         && prevState != commentInCssState)
        || startIndex > 0)
    {
        startIndex = text.indexOf (commentStartExp, startIndex, &startMatch);
        /* skip single-line comments */
        if (format (startIndex) == commentFormat || format (startIndex) == urlFormat)
            startIndex = -1;
        /* skip quotations (usually all formatted to this point) */
        QTextCharFormat fi = format (startIndex);
        while (fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat)
        {
            startIndex = text.indexOf (commentStartExp, startIndex + 1, &startMatch);
            fi = format (startIndex);
        }
        if (startIndex >= 0 && startIndex < cssIndx)
            commentBeforeBrace = true;

        /* special handling for markdown */
        if (progLan == "markdown" && startIndex > 0)
        {
            if (text.indexOf (QRegularExpression ("^#+\\s+.*"), 0) == 0
                || text.indexOf (QRegularExpression ("^( {4,}|\\s*\\t+\\s*).*"), 0) == 0)
            {
                return; // no comment start sign inside headings or code blocks
            }
            /* no comment start sign inside footnotes, images or links */
            QRegularExpressionMatch mMatch;
            QRegularExpression mExp ("\\[\\^[^\\]]+\\]"
                                     "|"
                                     "\\!\\[[^\\]\\^]*\\]\\s*"
                                     "(\\(\\s*[^\\)\\(\\s]+(\\s+\\\".*\\\")*\\s*\\)|\\s*\\[[^\\]]*\\])"
                                     "|"
                                     "\\[[^\\]\\^]*\\]\\s*\\[[^\\]\\s]*\\]"
                                     "|"
                                     "\\[[^\\]\\^]*\\]\\s*\\(\\s*[^\\)\\(\\s]+(\\s+\\\".*\\\")*\\s*\\)"
                                     "|"
                                     "\\[[^\\]\\^]*\\]:\\s+\\s*[^\\)\\(\\s]+(\\s+\\\".*\\\")*");
            int mStart = text.indexOf (mExp, 0, &mMatch);
            while (mStart >= 0 && mStart < startIndex)
            {
                int mEnd = mStart + mMatch.capturedLength();
                if (startIndex < mEnd)
                {
                    startIndex = text.indexOf (commentStartExp, mEnd, &startMatch);
                    if (startIndex == -1) return;
                }
                mStart = text.indexOf (mExp, mEnd, &mMatch);
            }
        }
    }

    while (startIndex >= 0)
    {
        int badIndex = -1;
        int endIndex;
        /* when the comment start is in the prvious line
           and the search for the comment end has just begun... */
        if ((prevState == commState
             || prevState == commentInCssState)
            && startIndex == 0)
            /* ... search for the comment end from the line start */
            endIndex = text.indexOf (commentEndExp, 0, &endMatch);
        else
            endIndex = text.indexOf (commentEndExp,
                                     startIndex + startMatch.capturedLength(),
                                     &endMatch);

        /* skip quotations */
        QTextCharFormat fi = format (endIndex);
        if (progLan != "fountain") // in Fountain, altQuoteFormat is used for notes
        { // FIXME: Is this really needed? Commented quotes are skipped in formatting multi-line quotes.
            while (fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat)
            {
                endIndex = text.indexOf (commentEndExp, endIndex + 1, &endMatch);
                fi = format (endIndex);
            }
        }

        /* if there's a comment end ... */
        if (/*!hugeText && */endIndex >= 0 && progLan != "xml" && progLan != "yaml")
        {
            /* ... clear the comment format from there to reformat later as
               a single-line comment sign may have been commented out now */
            badIndex = endIndex + 1;
            for (int i = badIndex; i < text.length(); ++i)
            {
                if (format (i) == commentFormat || format (i) == urlFormat)
                    setFormat (i, 1, neutralFormat);
            }
        }

        int commentLength;
        if (endIndex == -1)
        {
            if ((currentBlockState() != cssBlockState
                 && currentBlockState() != cssValueState)
                || commentBeforeBrace)
            {
                setCurrentBlockState (commState);
            }
            else
                setCurrentBlockState (commentInCssState);
            commentLength = text.length() - startIndex;
        }
        else
        {
            if (/*!hugeText && */cssIndx >= startIndex && cssIndx < endIndex)
            {
                /* if '{' is inside the comment,
                   this isn't a CSS block */
                setCurrentBlockState (-1);
                setFormat (endIndex + 1, text.length() - endIndex - 1, neutralFormat);
            }
            commentLength = endIndex - startIndex
                            + endMatch.capturedLength();
        }
        //if (!hugeText)
        //{
            setFormat (startIndex, commentLength, comFormat);

            /* format urls and email addresses inside the comment */
            QString str = text.mid (startIndex, commentLength);
            int pIndex = 0;
            QRegularExpressionMatch urlMatch;
            while ((pIndex = str.indexOf (urlPattern, pIndex, &urlMatch)) > -1)
            {
                setFormat (pIndex + startIndex, urlMatch.capturedLength(), urlFormat);
                pIndex += urlMatch.capturedLength();
            }
            /* format note patterns too */
            pIndex = 0;
            while ((pIndex = str.indexOf (notePattern, pIndex, &urlMatch)) > -1)
            {
                if (format (pIndex + startIndex) != urlFormat)
                    setFormat (pIndex + startIndex, urlMatch.capturedLength(), noteFormat);
                pIndex += urlMatch.capturedLength();
            }
        //}

        startIndex = text.indexOf (commentStartExp, startIndex + commentLength, &startMatch);

        /* reformat from here if the format was cleared before */
        if (/*!hugeText && */badIndex >= 0)
        {
            for (const HighlightingRule &rule : qAsConst (highlightingRules))
            {
                if (rule.format == commentFormat)
                {
                    int INDX = text.indexOf (rule.pattern, badIndex);
                    fi = format (INDX);
                    while (fi == quoteFormat
                           || fi == altQuoteFormat
                           || fi == urlInsideQuoteFormat
                           || isMLCommented (text, INDX, commState, endIndex + endMatch.capturedLength()))
                    {
                        INDX = text.indexOf (rule.pattern, INDX + 1);
                        fi = format (INDX);
                    }
                    if (INDX >= 0)
                    {
                        setFormat (INDX, text.length() - INDX, commentFormat);
                        /* URLs and notes were cleared too */
                        QString str = text.mid (INDX, text.length() - INDX);
                        int pIndex = 0;
                        QRegularExpressionMatch urlMatch;
                        while ((pIndex = str.indexOf (urlPattern, pIndex, &urlMatch)) > -1)
                        {
                            setFormat (pIndex + INDX, urlMatch.capturedLength(), urlFormat);
                            pIndex += urlMatch.capturedLength();
                        }
                        pIndex = 0;
                        while ((pIndex = str.indexOf (notePattern, pIndex, &urlMatch)) > -1)
                        {
                            if (format (pIndex + INDX) != urlFormat)
                                setFormat (pIndex + INDX, urlMatch.capturedLength(), noteFormat);
                            pIndex += urlMatch.capturedLength();
                        }
                    }
                    break;
                }
            }
        }

        /* skip single-line comments and quotations again */
        if (format (startIndex) == commentFormat || format (startIndex) == urlFormat)
            startIndex = -1;
        fi = format (startIndex);
        while (fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat)
        {
            startIndex = text.indexOf (commentStartExp, startIndex + 1, &startMatch);
            fi = format (startIndex);
        }
    }

    /* reset the block state if this line created a next-line comment
       whose starting single-line comment sign is commented out now */
    if (currentBlockState() == nextLineCommentState
        && format (text.size() - 1) != commentFormat && format (text.size() - 1) != urlFormat)
    {
        setCurrentBlockState (0);
    }
}
/*************************/
// Handles escaped backslashes too.
bool Highlighter::textEndsWithBackSlash (const QString &text)
{
    QString str = text;
    int n = 0;
    while (!str.isEmpty() && str.endsWith ("\\"))
    {
        str.truncate (str.size() - 1);
        ++n;
    }
    return (n % 2 != 0);
}
/*************************/
// This also covers single-line quotes. It comes after single-line comments
// and before multi-line ones are formatted. "comState" is the comment state,
// whose default is "commentState" but may be different for some languages.
// Sometimes (with multi-language docs), formatting should be started from "start".
bool Highlighter::multiLineQuote (const QString &text, const int start, int comState)
{
    if (progLan == "perl")
    {
        multiLinePerlQuote (text);
        return false;
    }
//--------------------
    /* these are only for C++11 raw string literals,
       whose pattern is R"(\bR"([^(]*)\(.*(?=\)\1"))" */
    bool rehighlightNextBlock = false;
    QString delimStr;
    QTextCharFormat rFormat;
    TextBlockData *cppData = nullptr;
    if (progLan == "cpp")
    {
        rFormat = quoteFormat;
        rFormat.setFontWeight (QFont::Bold);
        cppData = static_cast<TextBlockData *>(currentBlock().userData());
        QTextBlock prevBlock = currentBlock().previous();
        if (prevBlock.isValid())
        {
            if (TextBlockData *prevData = static_cast<TextBlockData *>(prevBlock.userData()))
                delimStr = prevData->labelInfo();
        }
    }
//--------------------

    int index = start;
    bool mixedQuotes = false;
    if (progLan == "c" || progLan == "cpp"
        || progLan == "javascript" || progLan == "qml"
        || progLan == "python"
        /*|| progLan == "sh"*/ // bash uses SH_MultiLineQuote()
        || progLan == "makefile" || progLan == "cmake"
        || progLan == "lua"
        || progLan == "ruby" || progLan == "scss"
        || progLan == "yaml")
    {
        mixedQuotes = true;
    }
    QRegularExpressionMatch quoteMatch;
    QRegularExpression quoteExpression;
    if (mixedQuotes)
        quoteExpression.setPattern ("\"|\'");
    else
        quoteExpression = quoteMark;
    int quote = doubleQuoteState;

    /* find the start quote */
    int prevState = previousBlockState();
    if ((prevState != doubleQuoteState
         && prevState != singleQuoteState)
        || index > 0)
    {
        index = text.indexOf (quoteExpression, index);
        /* skip escaped start quotes and all comments */
        while (isEscapedQuote (text, index, true)
               || isInsideRegex (text, index)
               || isMLCommented (text, index, comState)) // multiline
        {
            index = text.indexOf (quoteExpression, index + 1);
        }
        while (format (index) == commentFormat || format (index) == urlFormat) // single-line and Python
            index = text.indexOf (quoteExpression, index + 1);

        /* with markdown, "`" should be distinguished from "```" for code bocks (-> highlightBlock()) */
        if (index >= 0 && progLan == "markdown")
        {
            while (index >= 0 && index == text.indexOf (QRegularExpression ("```(?!`)"), index))
            {
                index = text.indexOf (quoteExpression, index + 3);
                while (isEscapedQuote (text, index, true)
                       || isMLCommented (text, index, comState))
                {
                    index = text.indexOf (quoteExpression, index + 1);
                }
                while (format (index) == commentFormat || format (index) == urlFormat)
                    index = text.indexOf (quoteExpression, index + 1);
            }
        }

        /* if the start quote is found... */
        if (index >= 0)
        {
            if (mixedQuotes)
            {
                /* ... distinguish between double and single quotes */
                if (text.at (index) == quoteMark.pattern().at (0))
                {
                    if (progLan == "cpp" && index > start)
                    {
                        QRegularExpressionMatch cppMatch;
                        if (text.at (index - 1) == 'R' && index - 1 == text.indexOf (QRegularExpression (R"(\bR"([^(]*)\()"), index - 1, &cppMatch))
                        {
                            delimStr = ")" + cppMatch.captured (1);
                            setFormat (index - 1, 1, rFormat);
                        }
                    }
                    quoteExpression = quoteMark;
                    quote = doubleQuoteState;
                }
                else
                {
                    quoteExpression.setPattern ("\'");
                    quote = singleQuoteState;
                }
            }
        }
    }
    else // but if we're inside a quotation...
    {
        /* ... distinguish between the two quote kinds
           by checking the previous line */
        if (mixedQuotes)
        {
            quote = prevState;
            if (quote == doubleQuoteState)
                quoteExpression = quoteMark;
            else
                quoteExpression.setPattern ("\'");
        }
    }

    while (index >= 0)
    {
        /* if the search is continued... */
        if (quoteExpression.pattern() == "\"|\'")
        {
            /* ... distinguish between double and single quotes
               again because the quote mark may have changed */
            if (text.at (index) == quoteMark.pattern().at (0))
            {
                if (progLan == "cpp" && index > start)
                {
                    QRegularExpressionMatch cppMatch;
                    if (text.at (index - 1) == 'R'  && index - 1 == text.indexOf (QRegularExpression (R"(\bR"([^(]*)\()"), index - 1, &cppMatch))
                    {
                        delimStr = ")" + cppMatch.captured (1);
                        setFormat (index - 1, 1, rFormat);
                    }
                }
                quoteExpression = quoteMark;
                quote = doubleQuoteState;
            }
            else
            {
                quoteExpression.setPattern ("\'");
                quote = singleQuoteState;
            }
        }

        int endIndex;
        /* if there's no start quote ... */
        if (index == 0
            && (prevState == doubleQuoteState || prevState == singleQuoteState))
        {
            /* ... search for the end quote from the line start */
            endIndex = text.indexOf (quoteExpression, 0, &quoteMatch);
        }
        else // otherwise, search from the start quote
            endIndex = text.indexOf (quoteExpression, index + 1, &quoteMatch);

        if (delimStr.isEmpty())
        { // check if the quote is escaped
            while (isEscapedQuote (text, endIndex, false))
                endIndex = text.indexOf (quoteExpression, endIndex + 1, &quoteMatch);
        }
        else
        { // check if the quote is inside a C++11 raw string literal
            while (endIndex > -1
                   && (endIndex - delimStr.length() < start
                       || text.mid (endIndex - delimStr.length(), delimStr.length()) != delimStr))
            {
                endIndex = text.indexOf (quoteExpression, endIndex + 1, &quoteMatch);
            }
        }

        bool isQuotation = true;
        if (endIndex == -1)
        {
            if (progLan == "c" || progLan == "cpp"
                || progLan == "javascript" || progLan == "qml")
            {
                /* In c and cpp, multiline double quotes need backslash and
                   there's no multiline single quote. Moreover, In C++11,
                   there can be multiline raw string literals. */
                if (quoteExpression.pattern() == "\'"
                    || (quoteExpression == quoteMark && delimStr.isEmpty() && !textEndsWithBackSlash (text)))
                {
                    endIndex = text.size() + 1; // quoteMatch.capturedLength() is 1 here
                }
            }
            else if (progLan == "markdown")
            { // this is the main difference of a markdown inline code from a single-line quote
                isQuotation = false;
            }
        }

        int quoteLength;
        if (endIndex == -1)
        {
            if (isQuotation)
                setCurrentBlockState (quote);
            quoteLength = text.length() - index;

            /* set the delimiter string for C++11 */
            if (cppData && !delimStr.isEmpty())
            {
                /* with a multiline C++11 raw string literal, if the delimiter is changed
                   but the state of the current block isn't changed, the next block won't
                   be highlighted automatically, so it should be rehighlighted forcefully */
                if (cppData->lastState() == quote)
                {
                    QTextBlock nextBlock = currentBlock().next();
                    if (nextBlock.isValid())
                    {
                        if (TextBlockData *nextData = static_cast<TextBlockData *>(nextBlock.userData()))
                        {
                            if (nextData->labelInfo() != delimStr)
                                rehighlightNextBlock = true;
                        }
                    }
                }
                cppData->insertInfo (delimStr);
                setCurrentBlockUserData (cppData);
            }
        }
        else
            quoteLength = endIndex - index
                          + quoteMatch.capturedLength(); // 1
        if (isQuotation)
        {
            setFormat (index, quoteLength, quoteExpression == quoteMark ? quoteFormat
                                                                        : altQuoteFormat);
            /* URLs should be formatted in a different way inside quotes because,
               otherwise, there would be no difference between URLs inside quotes and
               those inside comments and so, they couldn't be escaped correctly when needed. */
            QString str = text.mid (index, quoteLength);
            int urlIndex = 0;
            QRegularExpressionMatch urlMatch;
            while ((urlIndex = str.indexOf (urlPattern, urlIndex, &urlMatch)) > -1)
            {
                 setFormat (urlIndex + index, urlMatch.capturedLength(), urlInsideQuoteFormat);
                 urlIndex += urlMatch.capturedLength();
            }
        }

        /* the next quote may be different */
        if (mixedQuotes)
            quoteExpression.setPattern ("\"|\'");
        index = text.indexOf (quoteExpression, index + quoteLength);

        /* skip escaped start quotes and all comments */
        while (isEscapedQuote (text, index, true)
               || isInsideRegex (text, index)
               || isMLCommented (text, index, comState, endIndex + 1))
        {
            index = text.indexOf (quoteExpression, index + 1);
        }
        while (format (index) == commentFormat || format (index) == urlFormat)
            index = text.indexOf (quoteExpression, index + 1);

        if (index >= 0 && progLan == "markdown")
        {
            while (index >= 0 && index == text.indexOf (QRegularExpression ("```(?!`)"), index))
            {
                index = text.indexOf (quoteExpression, index + 3);
                while (isEscapedQuote (text, index, true)
                       || isMLCommented (text, index, comState))
                {
                    index = text.indexOf (quoteExpression, index + 1);
                }
                while (format (index) == commentFormat || format (index) == urlFormat)
                    index = text.indexOf (quoteExpression, index + 1);
            }
        }
        delimStr.clear();
    }
    return rehighlightNextBlock;
}
/*************************/
// Perl's multiline quote highlighting comes here to support backquotes.
// Also see isPerlQuoted().
void Highlighter::multiLinePerlQuote (const QString &text)
{
    int index = 0;
    QRegularExpressionMatch quoteMatch;
    QRegularExpression quoteExpression ("\"|\'|`");
    int quote = doubleQuoteState;

    /* find the start quote */
    int prevState = previousBlockState();
    if (prevState != doubleQuoteState && prevState != singleQuoteState)
    {
        index = text.indexOf (quoteExpression, index);
        /* skip escaped start quotes and all comments */
        while (isEscapedQuote (text, index, true) || isInsideRegex (text, index))
            index = text.indexOf (quoteExpression, index + 1);
        while (format (index) == commentFormat || format (index) == urlFormat)
            index = text.indexOf (quoteExpression, index + 1);

        /* if the start quote is found... */
        if (index >= 0)
        {
            /* ... distinguish between the three kinds of quote */
            if (text.at (index) == quoteMark.pattern().at (0))
            {
                quoteExpression = quoteMark;
                quote = doubleQuoteState;
            }
            else
            {
                if (text.at (index) == '\'')
                    quoteExpression.setPattern ("\'");
                else
                    quoteExpression.setPattern ("`");
                quote = singleQuoteState;
            }
        }
    }
    else // but if we're inside a quotation...
    {
        /* ... distinguish between the three kinds of quote */
        quote = prevState;
        if (quote == doubleQuoteState)
            quoteExpression = quoteMark;
        else
        {
            bool backquoted (false);
            QTextBlock prevBlock = currentBlock().previous();
            if (prevBlock.isValid())
            {
                TextBlockData *prevData = static_cast<TextBlockData *>(prevBlock.userData());
                if (prevData && prevData->getProperty())
                    backquoted = true;
            }
            if (backquoted)
                quoteExpression.setPattern ("`");
            else
                quoteExpression.setPattern ("\'");
        }
    }

    while (index >= 0)
    {
        /* if the search is continued... */
        if (quoteExpression.pattern() == "\"|\'|`")
        {
            /* ... distinguish between the three kinds of quote
               again because the quote mark may have changed */
            if (text.at (index) == quoteMark.pattern().at (0))
            {
                quoteExpression = quoteMark;
                quote = doubleQuoteState;
            }
            else
            {
                if (text.at (index) == '\'')
                    quoteExpression.setPattern ("\'");
                else
                    quoteExpression.setPattern ("`");
                quote = singleQuoteState;
            }
        }

        int endIndex;
        /* if there's no start quote ... */
        if (index == 0
            && (prevState == doubleQuoteState || prevState == singleQuoteState))
        {
            /* ... search for the end quote from the line start */
            endIndex = text.indexOf (quoteExpression, 0, &quoteMatch);
        }
        else // otherwise, search from the start quote
            endIndex = text.indexOf (quoteExpression, index + 1, &quoteMatch);

        // check if the quote is escaped
        while (isEscapedQuote (text, endIndex, false))
            endIndex = text.indexOf (quoteExpression, endIndex + 1, &quoteMatch);

        int quoteLength;
        if (endIndex == -1)
        {
            setCurrentBlockState (quote);
            if (quoteExpression.pattern() == "`")
            {
                if (TextBlockData *data = static_cast<TextBlockData *>(currentBlock().userData()))
                    data->setProperty (true);
                /* NOTE: The next block will be rehighlighted at highlightBlock()
                         (-> multiLineRegex (text, 0);) if the property is changed. */
            }
            quoteLength = text.length() - index;
        }
        else
            quoteLength = endIndex - index
                          + quoteMatch.capturedLength(); // 1
        setFormat (index, quoteLength, quoteExpression == quoteMark ? quoteFormat
                                                                    : altQuoteFormat);
        QString str = text.mid (index, quoteLength);
        int urlIndex = 0;
        QRegularExpressionMatch urlMatch;
        while ((urlIndex = str.indexOf (urlPattern, urlIndex, &urlMatch)) > -1)
        {
             setFormat (urlIndex + index, urlMatch.capturedLength(), urlInsideQuoteFormat);
             urlIndex += urlMatch.capturedLength();
        }

        /* the next quote may be different */
        quoteExpression.setPattern ("\"|\'|`");
        index = text.indexOf (quoteExpression, index + quoteLength);

        /* skip escaped start quotes and all comments */
        while (isEscapedQuote (text, index, true) || isInsideRegex (text, index))
            index = text.indexOf (quoteExpression, index + 1);
        while (format (index) == commentFormat || format (index) == urlFormat)
            index = text.indexOf (quoteExpression, index + 1);
    }
}
/*************************/
// Generalized form of setFormat(), where "oldFormat" shouldn't be reformatted.
void Highlighter::setFormatWithoutOverwrite (int start,
                                             int count,
                                             const QTextCharFormat &newFormat,
                                             const QTextCharFormat &oldFormat)
{
    int index = start; // always >= 0
    int indx;
    while (index < start + count)
    {
        QTextCharFormat fi = format (index);
        while (index < start + count
               && (fi == oldFormat
                   /* skip comments and quotes */
                   || fi == commentFormat || fi == urlFormat
                   || fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat))
        {
            ++ index;
            fi = format (index);
        }
        if (index < start + count)
        {
            indx = index;
            fi = format (indx);
            while (indx < start + count
                   && fi != oldFormat
                   && fi != commentFormat && fi != urlFormat
                   && fi != quoteFormat && fi != altQuoteFormat && fi != urlInsideQuoteFormat)
            {
                ++ indx;
                fi = format (indx);
            }
            setFormat (index, indx - index , newFormat);
            index = indx;
        }
    }
}
/*************************/
// XML quotes are handled as multiline quotes for possible XML doc mistakes
// to be seen easily and also because the double quote can be written as "&quot;".
// This comes after values are formatted.
void Highlighter::xmlQuotes (const QString &text)
{
    int index = 0;
    /* mixed quotes aren't really needed here
       but they're harmless and easy to handle */
    QRegularExpressionMatch quoteMatch;
    QRegularExpression quoteExpression ("\"|&quot;|\'");
    QRegularExpression doubleQuote ("\"|&quot;");
    QRegularExpression virtualQuote ("&quot;");
    int quote = doubleQuoteState;

    /* find the start quote */
    int prevState = previousBlockState();
    if (prevState != doubleQuoteState
        && prevState != singleQuoteState)
    {
        index = text.indexOf (quoteExpression);
        /* skip all comments and all values (that are formatted by multiLineComment()) */
        while (isMLCommented (text, index, commentState)
               || format (index) == neutralFormat)
        {
            index = text.indexOf (quoteExpression, index + 1);
        }

        /* if the start quote is found... */
        if (index >= 0)
        {
            /* ... distinguish between double, virtual and single quotes */
            if (text.at (index) == '\"')
            {
                quoteExpression = quoteMark;
                quote = doubleQuoteState;
            }
            else if (text.mid (index, 6) == "&quot;")
            {
                quoteExpression = virtualQuote;
                quote = doubleQuoteState;
            }
            else
            {
                quoteExpression.setPattern ("\'");
                quote = singleQuoteState;
            }
        }
    }
    else // but if we're inside a quotation...
    {
        /* ... distinguish between the two quote kinds
           by checking the previous line */
        quote = prevState;
        if (quote == doubleQuoteState)
            quoteExpression = doubleQuote;
        else
            quoteExpression.setPattern ("\'");
    }

    while (index >= 0)
    {
        /* if the search is continued... */
        if (quoteExpression.pattern() == "\"|&quot;|\'")
        {
            /* ... distinguish between double, virtual and single
               quotes again because the quote mark may have changed */
            if (text.at (index) == '\"')
            {
                quoteExpression = quoteMark;
                quote = doubleQuoteState;
            }
            else if (text.mid (index, 6) == "&quot;")
            {
                quoteExpression = virtualQuote;
                quote = doubleQuoteState;
            }
            else
            {
                quoteExpression.setPattern ("\'");
                quote = singleQuoteState;
            }
        }

        /* search for the end quote from the start quote */
        int endIndex = text.indexOf (quoteExpression, index + 1, &quoteMatch);

        /* but if there's no start quote ... */
        if (index == 0
            && (prevState == doubleQuoteState || prevState == singleQuoteState))
        {
            /* ... search for the end quote from the line start */
            endIndex = text.indexOf (quoteExpression, 0, &quoteMatch);
        }

        if (endIndex == -1)
        {
            /* tolerate a mismatch between `"` and `&quot;` as far as possible
               but show the error by formatting `>` or `&gt;` */
            QRegularExpressionMatch match;
            int closing = text.indexOf (QRegularExpression ("(>|&gt;)"), index, &match);
            if (closing > -1)
                endIndex = closing + match.capturedLength();
        }

        int quoteLength;
        if (endIndex == -1)
        {
            setCurrentBlockState (quote);
            quoteLength = text.length() - index;
        }
        else
            quoteLength = endIndex - index
                          + quoteMatch.capturedLength();
        setFormat (index, quoteLength,
                   quoteExpression == quoteMark
                   || quoteExpression == virtualQuote ? quoteFormat : altQuoteFormat);

        QString str = text.mid (index, quoteLength);
        int urlIndex = 0;
        QRegularExpressionMatch urlMatch;
        while ((urlIndex = str.indexOf (urlPattern, urlIndex, &urlMatch)) > -1)
        {
             setFormat (urlIndex + index, urlMatch.capturedLength(), urlInsideQuoteFormat);
             urlIndex += urlMatch.capturedLength();
        }

        /* the next quote may be different */
        quoteExpression.setPattern ("\"|&quot;|\'");
        index = text.indexOf (quoteExpression, index + quoteLength);

        while (isMLCommented (text, index, commentState, endIndex + quoteMatch.capturedLength())
               || format (index) == neutralFormat)
        {
            index = text.indexOf (quoteExpression, index + 1);
        }
    }
}
/*************************/
// Check if the current block is inside a "here document" and format it accordingly.
// (Open quotes aren't taken into account when they happen after the start delimiter.)
bool Highlighter::isHereDocument (const QString &text)
{
    QTextBlock prevBlock = currentBlock().previous();
    TextBlockData *prevData = nullptr;
    if (prevBlock.isValid())
        prevData = static_cast<TextBlockData *>(prevBlock.userData());

    QTextCharFormat blockFormat;
    blockFormat.setForeground (Violet);
    QTextCharFormat delimFormat = blockFormat;
    delimFormat.setFontWeight (QFont::Bold);
    QString delimStr;
    /* Kate uses something like "<<(?:\\s*)([\\\\]{0,1}[^\\s]+)" */
    QRegularExpression delim;
    if (progLan == "sh" || progLan == "makefile" || progLan == "cmake") // "<<-" can be used instead of "<<"
        delim.setPattern ("<<-?(?:\\s*)(\\\\{0,1}[A-Za-z0-9_]+)|<<-?(?:\\s*)(\'[A-Za-z0-9_]+\')|<<-?(?:\\s*)(\"[A-Za-z0-9_]+\")");
    /*else if (progLan == "perl") // without space after "<<" and with ";" at the end
        delim.setPattern ("<<([A-Za-z0-9_]+)(?:;)|<<(\'[A-Za-z0-9_]+\')(?:;)|<<(\"[A-Za-z0-9_]+\")(?:;)");*/
    else if (progLan == "perl") // can contain spaces inside quote marks or backquotes and usually has ";" at the end
        delim.setPattern ("<<([A-Za-z0-9_]+)(?:;{0,1})|<<(?:\\s*)(\'[A-Za-z0-9_\\s]+\')(?:;{0,1})|<<(?:\\s*)(\"[A-Za-z0-9_\\s]+\")(?:;{0,1})|<<(?:\\s*)(`[A-Za-z0-9_\\s]+`)(?:;{0,1})");
    else if (progLan == "ruby")
        delim.setPattern ("<<(?:-|~){0,1}([A-Za-z0-9_]+)|<<(\'[A-Za-z0-9_]+\')|<<(\"[A-Za-z0-9_]+\")");
    else // FIXME: No language.
        delim.setPattern ("<<([A-Za-z0-9_]+)|<<(\'[A-Za-z0-9_]+\')|<<(\"[A-Za-z0-9_]+\")");
    QRegularExpression comment;
    if (progLan == "sh" || progLan == "makefile" || progLan == "cmake")
        comment.setPattern ("^#.*|\\s+#.*");
    else
        comment.setPattern ("#.*");
    int insideCommentPos = text.indexOf (comment);
    int pos = 0;

    /* format the start delimiter */
    QRegularExpressionMatch match;
    int prevState = previousBlockState();
    if ((!prevBlock.isValid()
         || (prevState >= 0 && prevState < endState))
        && (pos = text.indexOf (delim, 0, &match)) >= 0
        && !isQuoted (text, pos, true) // escaping start double quote before "$("
        /* the whole line isn't commented out */
        && (insideCommentPos == -1 || pos < insideCommentPos
            || isQuoted (text, insideCommentPos, true)))
    {
        int i = 1;
        while ((delimStr = match.captured (i)).isEmpty() && i <= 3)
        {
            ++i;
            delimStr = match.captured (i);
        }

        if (progLan == "perl")
        {
            bool ok;
            delimStr.toInt (&ok, 10);
            if (ok)
                delimStr = QString(); // don't mistake shift-left operator with here-doc delimiter
            else if (delimStr.contains ('`')) // Perl's delimiter can have backquotes
                delimStr = delimStr.split ('`').at (1);
        }

        if (!delimStr.isEmpty())
        {
            /* remove quotes */
            if (delimStr.contains ('\''))
                delimStr = delimStr.split ('\'').at (1);
            if (delimStr.contains ('\"'))
                delimStr = delimStr.split ('\"').at (1);
            /* remove the start backslash if it exists */
            if (QString (delimStr.at (0)) == "\\")
                delimStr = delimStr.remove (0, 1);
        }

        if (!delimStr.isEmpty())
        {
            int n = static_cast<int>(qHash (delimStr));
            int state = 2 * (n + (n >= 0 ? endState/2 + 1 : 0)); // always an even number but maybe negative
            if (isQuoted (text, pos, false))
            { // to know whether a double quote is added/removed before "$(" in the current line
                state > 0 ? state += 2 : state -= 2;
            }
            if (prevState == doubleQuoteState || prevState == SH_DoubleQuoteState)
            { // to know whether a double quote is added/removed before "$(" in a previous line
                state > 0 ? state += 4 : state -= 4; // not 2 -> not to be canceled above
            }
            setCurrentBlockState (state);
            setFormat (text.indexOf (delimStr, pos),
                       delimStr.length(),
                       delimFormat);

            TextBlockData *data = static_cast<TextBlockData *>(currentBlock().userData());
            if (!data) return false;
            data->insertInfo (delimStr);
            setCurrentBlockUserData (data);

            return false;
        }
    }

    if (prevState >= endState || prevState < -1)
    {
        if (!prevData) return false;
        delimStr = prevData->labelInfo();
        int l = 0;
        if (progLan == "perl" || progLan == "ruby")
        {
            QRegularExpressionMatch rMatch;
            QRegularExpression r ("\\s*" + delimStr + "(?=(\\W+|$))");
            if (text.indexOf (r, 0, &rMatch) == 0)
                l = rMatch.capturedLength();
        }
        else if (text == delimStr
                 || (text.startsWith (delimStr)
                     && text.indexOf (QRegularExpression ("\\W+")) == delimStr.length()))
        {
            l = delimStr.length();
        }
        if (l > 0)
        {
            /* format the end delimiter */
            setFormat (0, l, delimFormat);
            return false;
        }
        else
        {
            /* format the contents */
            TextBlockData *data = static_cast<TextBlockData *>(currentBlock().userData());
            if (!data) return false;
            data->insertInfo (delimStr);
            /* inherit the previous data property and open nests */
            if (bool p = prevData->getProperty())
                data->setProperty (p);
            int N = prevData->openNests();
            if (N > 0)
            {
                data->insertNestInfo (N);
                QSet<int> Q = prevData->openQuotes();
                if (!Q.isEmpty())
                    data->insertOpenQuotes (Q);
            }
            setCurrentBlockUserData (data);
            if (prevState % 2 == 0)
                setCurrentBlockState (prevState - 1);
            else
                setCurrentBlockState (prevState);
            setFormat (0, text.length(), blockFormat);

            /* also, format whitespaces */
            for (const HighlightingRule &rule : qAsConst (highlightingRules))
            {
                if (rule.format == whiteSpaceFormat)
                {
                    QRegularExpressionMatch match;
                    int index = text.indexOf (rule.pattern, 0, &match);
                    while (index >= 0)
                    {
                        setFormat (index, match.capturedLength(), rule.format);
                        index = text.indexOf (rule.pattern, index + match.capturedLength(), &match);
                    }
                    break;
                }
            }

            return true;
        }
    }

    return false;
}
/*************************/
void Highlighter::markDownFonts (const QString &text)
{
    QTextCharFormat boldFormat = neutralFormat;
    boldFormat.setFontWeight (QFont::Bold);

    QTextCharFormat italicFormat = neutralFormat;
    italicFormat.setFontItalic (true);

    QTextCharFormat boldItalicFormat = italicFormat;
    boldItalicFormat.setFontWeight (QFont::Bold);

    /* NOTE: Apparently, all browsers use expressions similar to the following ones.
             However, these patterns aren't logical. For example, escaping an asterisk
             isn't always equivalent to its removal with regard to boldness/italicity.
             It also seems that five successive asterisks are ignored at start. */

    QRegularExpressionMatch italicMatch;
    const QRegularExpression italicExp ("(?<!\\\\|\\*{4})\\*([^*]|(?:(?<!\\*)\\*\\*))+\\*|(?<!\\\\|_{4})_([^_]|(?:(?<!_)__))+_"); // allow double asterisks inside

    QRegularExpressionMatch boldcMatch;
    //const QRegularExpression boldExp ("\\*\\*(?!\\*)(?:(?!\\*\\*).)+\\*\\*|__(?:(?!__).)+__}");
    const QRegularExpression boldExp ("(?<!\\\\|\\*{3})\\*\\*([^*]|(?:(?<!\\*)\\*))+\\*\\*|(?<!\\\\|_{3})__([^_]|(?:(?<!_)_))+__"); // allow single asterisks inside

    const QRegularExpression boldItalicExp ("(?<!\\\\|\\*{2})\\*{3}([^*]|(?:(?<!\\*)\\*))+\\*{3}|(?<!\\\\|_{2})_{3}([^_]|(?:(?<!_)_))+_{3}");

    QRegularExpressionMatch expMatch;
    const QRegularExpression exp (boldExp.pattern() + "|" + italicExp.pattern() + "|" + boldItalicExp.pattern());

    int index = 0;
    while ((index = text.indexOf (exp, index, &expMatch)) > -1)
    {
        if (format (index) == mainFormat && format (index + expMatch.capturedLength() - 1) == mainFormat)
        {
            if (index == text.indexOf (boldItalicExp, index))
            {
                setFormatWithoutOverwrite (index, expMatch.capturedLength(), boldItalicFormat, whiteSpaceFormat);
            }
            else if (index == text.indexOf (boldExp, index, &boldcMatch) && boldcMatch.capturedLength() == expMatch.capturedLength())
            {
                setFormatWithoutOverwrite (index, expMatch.capturedLength(), boldFormat, whiteSpaceFormat);
                /* also format italic bold strings */
                QString str = text.mid (index + 2, expMatch.capturedLength() - 4);
                int indx = 0;
                while ((indx = str.indexOf (italicExp, indx, &italicMatch)) > -1)
                {
                    setFormatWithoutOverwrite (index + 2 + indx, italicMatch.capturedLength(), boldItalicFormat, whiteSpaceFormat);
                    indx += italicMatch.capturedLength();
                }
            }
            else
            {
                setFormatWithoutOverwrite (index, expMatch.capturedLength(), italicFormat, whiteSpaceFormat);
                /* also format bold italic strings */
                QString str = text.mid (index + 1, expMatch.capturedLength() - 2);
                int indx = 0;
                while ((indx = str.indexOf (boldExp, indx, &boldcMatch)) > -1)
                {
                    setFormatWithoutOverwrite (index + 1 + indx, boldcMatch.capturedLength(), boldItalicFormat, whiteSpaceFormat);
                    indx += boldcMatch.capturedLength();
                }

            }
            index += expMatch.capturedLength();
        }
        else ++index;
    }
}
/*************************/
void Highlighter::fountainFonts (const QString &text)
{
    QTextCharFormat boldFormat = neutralFormat;
    boldFormat.setFontWeight (QFont::Bold);

    QTextCharFormat italicFormat = neutralFormat;
    italicFormat.setFontItalic (true);

    QTextCharFormat boldItalicFormat = italicFormat;
    boldItalicFormat.setFontWeight (QFont::Bold);

    QRegularExpressionMatch italicMatch, boldcMatch, expMatch;
    static const QRegularExpression italicExp ("(?<!\\\\)\\*([^*]|(?:(?<=\\\\)\\*))+(?<!\\\\|\\s)\\*");
    static const QRegularExpression boldExp ("(?<!\\\\)\\*\\*([^*]|(?:(?<=\\\\)\\*))+(?<!\\\\|\\s)\\*\\*");
    static const QRegularExpression boldItalicExp ("(?<!\\\\)\\*{3}([^*]|(?:(?<=\\\\)\\*))+(?<!\\\\|\\s)\\*{3}");

    const QRegularExpression exp (boldExp.pattern() + "|" + italicExp.pattern() + "|" + boldItalicExp.pattern());

    int index = 0;
    while ((index = text.indexOf (exp, index, &expMatch)) > -1)
    {
        if (format (index) == mainFormat && format (index + expMatch.capturedLength() - 1) == mainFormat)
        {
            if (index == text.indexOf (boldItalicExp, index))
            {
                setFormatWithoutOverwrite (index, expMatch.capturedLength(), boldItalicFormat, whiteSpaceFormat);
            }
            else if (index == text.indexOf (boldExp, index, &boldcMatch) && boldcMatch.capturedLength() == expMatch.capturedLength())
            {
                setFormatWithoutOverwrite (index, expMatch.capturedLength(), boldFormat, whiteSpaceFormat);
                /* also format italic bold strings */
                QString str = text.mid (index + 2, expMatch.capturedLength() - 4);
                int indx = 0;
                while ((indx = str.indexOf (italicExp, indx, &italicMatch)) > -1)
                {
                    setFormatWithoutOverwrite (index + 2 + indx, italicMatch.capturedLength(), boldItalicFormat, whiteSpaceFormat);
                    indx += italicMatch.capturedLength();
                }
            }
            else
            {
                setFormatWithoutOverwrite (index, expMatch.capturedLength(), italicFormat, whiteSpaceFormat);
                /* also format bold italic strings */
                QString str = text.mid (index + 1, expMatch.capturedLength() - 2);
                int indx = 0;
                while ((indx = str.indexOf (boldExp, indx, &boldcMatch)) > -1)
                {
                    setFormatWithoutOverwrite (index + 1 + indx, boldcMatch.capturedLength(), boldItalicFormat, whiteSpaceFormat);
                    indx += boldcMatch.capturedLength();
                }

            }
            index += expMatch.capturedLength();
        }
        else ++index;
    }

    /* format underlines */
    static const QRegularExpression under ("(?<!\\\\)_([^_]|(?:(?<=\\\\)_))+(?<!\\\\|\\s)_");
    index = 0;
    while ((index = text.indexOf (under, index, &expMatch)) > -1)
    {
        QTextCharFormat fi = format (index);
        if (fi == commentFormat || fi == altQuoteFormat)
            ++index;
        else
        {
            int count = expMatch.capturedLength();
            fi = format (index + count - 1);
            if (fi != commentFormat && fi != altQuoteFormat)
            {
                int start = index;
                int indx;
                while (start < index + count)
                {
                    fi = format (start);
                    while (start < index + count
                           && (fi == whiteSpaceFormat || fi == commentFormat || fi == altQuoteFormat))
                    {
                        ++ start;
                        fi = format (start);
                    }
                    if (start < index + count)
                    {
                        indx = start;
                        fi = format (indx);
                        while (indx < index + count
                               && fi != whiteSpaceFormat && fi != commentFormat && fi != altQuoteFormat)
                        {
                            fi.setFontUnderline (true);
                            setFormat (indx, 1 , fi);
                            ++ indx;
                            fi = format (indx);
                        }
                        start = indx;
                    }
                }
            }
            index += count;
        }
    }
}
/*************************/
void Highlighter::reSTMainFormatting (int start, const QString &text)
{
    if (start < 0) return;
    TextBlockData *data = static_cast<TextBlockData *>(currentBlock().userData());
    if (data == nullptr) return;

    data->setHighlighted(); // completely highlighted
    for (const HighlightingRule &rule : qAsConst (highlightingRules))
    {
        QTextCharFormat fi;
        QRegularExpressionMatch match;
        int index = text.indexOf (rule.pattern, start, &match);
        if (rule.format != whiteSpaceFormat)
        {
            fi = format (index);
            while (index >= 0 && fi != mainFormat)
            {
                index = text.indexOf (rule.pattern, index + match.capturedLength(), &match);
                fi = format (index);
            }
        }
        while (index >= 0)
        {
            /* get the overwritten format if existent */
            QTextCharFormat prevFormat = format (index + match.capturedLength() - 1);

            setFormat (index, match.capturedLength(), rule.format);
            if (rule.pattern == QRegularExpression (":[\\w\\-+]+:`[^`]*`"))
            { // format the reference start too
                QTextCharFormat boldFormat = neutralFormat;
                boldFormat.setFontWeight (QFont::Bold);
                setFormat (index, text.indexOf (":`", index) - index + 1, boldFormat);
            }
            index += match.capturedLength();

            if (rule.format != whiteSpaceFormat && prevFormat != mainFormat)
            { // if a format is overwriiten by this rule, reformat from here
                setFormat (index, text.length() - index, mainFormat);
                reSTMainFormatting (index, text);
                break;
            }

            index = text.indexOf (rule.pattern, index, &match);
            if (rule.format != whiteSpaceFormat)
            {
                fi = format (index);
                while (index >= 0 && fi != mainFormat)
                {
                    index = text.indexOf (rule.pattern, index + match.capturedLength(), &match);
                    fi = format (index);
                }
            }
        }
    }
}

/*************************/
void Highlighter::debControlFormatting (const QString &text)
{
    if (text.isEmpty()) return;
    bool formatFurther (false);
    QRegularExpressionMatch expMatch;
    QRegularExpression exp;
    int indx = 0;
    QTextCharFormat debFormat;
    if (text.indexOf (QRegularExpression ("^[^\\s:]+:(?=\\s*)")) == 0)
    {
        formatFurther = true;
        exp.setPattern ("^[^\\s:]+(?=:)");
        if (text.indexOf (exp, 0, &expMatch) == 0)
        {
            /* before ":" */
            debFormat.setFontWeight (QFont::Bold);
            debFormat.setForeground (DarkBlue);
            setFormat (0, expMatch.capturedLength(), debFormat);

            /* ":" */
            debFormat.setForeground (DarkMagenta);
            indx = text.indexOf (":");
            setFormat (indx, 1, debFormat);
            indx ++;

            if (indx < text.count())
            {
                /* after ":" */
                debFormat.setFontWeight (QFont::Normal);
                debFormat.setForeground (DarkGreenAlt);
                setFormat (indx, text.count() - indx , debFormat);
            }
        }
    }
    else if (text.indexOf (QRegularExpression ("^\\s+")) == 0)
    {
        formatFurther = true;
        debFormat.setForeground (DarkGreenAlt);
        setFormat (0, text.count(), debFormat);
    }

    if (formatFurther)
    {
        /* parentheses and brackets */
        exp.setPattern ("\\([^\\(\\)\\[\\]]+\\)|\\[[^\\(\\)\\[\\]]+\\]");
        int index = indx;
        debFormat = neutralFormat;
        debFormat.setFontItalic (true);
        while ((index = text.indexOf (exp, index, &expMatch)) > -1)
        {
            int ml = expMatch.capturedLength();
            setFormat (index, ml, neutralFormat);
            if (ml > 2)
            {
                setFormat (index + 1, ml - 2 , debFormat);

                QRegularExpression rel ("<|>|\\=|~");
                int i = index;
                while ((i = text.indexOf (rel, i)) > -1 && i < index + ml - 1)
                {
                    QTextCharFormat relFormat;
                    relFormat.setForeground (DarkMagenta);
                    setFormat (i, 1, relFormat);
                    ++i;
                }
            }
            index = index + ml;
        }

        /* non-commented URLs */
        debFormat.setForeground (DarkGreenAlt);
        debFormat.setFontUnderline (true);
        QRegularExpressionMatch urlMatch;
        while ((indx = text.indexOf (urlPattern, indx, &urlMatch)) > -1)
        {
            setFormat (indx, urlMatch.capturedLength(), debFormat);
            indx += urlMatch.capturedLength();
        }
    }
}
/*************************/
// Check whether the start bracket/brace is escaped. FIXME: This is quite complex.
static inline bool isYamlBraceEscaped (const QString &text, const QRegularExpression &start, int pos)
{
    if (pos < 0 || text.indexOf (start, pos) != pos)
        return false;
    if (text.lastIndexOf (QRegularExpression ("(^|{|,|\\[)?[^:#]*:\\s+[^{\\[,#\\s]+\\s*\\K" + start.pattern()), pos) == pos) // inside value
        return true;
    QRegularExpressionMatch match;
    int indx = text.lastIndexOf (QRegularExpression ("[^:#\\s]+\\s*" + start.pattern() + "[^:#]*:\\s+"), pos, &match);
    if (indx > -1 && indx < pos && indx + match.capturedLength() > pos) // inside key
        return true;
    return false;
}

// Process open braces or brackets, apply the neutral format appropriately
// and return a boolean that shows whether the next line should be updated.
// Single-line comments should have already been highlighted.
bool Highlighter::yamlOpenBraces (const QString &text,
                                  const QRegularExpression &startExp, const QRegularExpression &endExp,
                                  int oldOpenNests, bool oldProperty, // old info on the current line
                                  bool setData) // whether data should be set
{
    TextBlockData *data = static_cast<TextBlockData *>(currentBlock().userData());
    if (!data) return false;

    int openNests = 0;
    bool property = startExp.pattern() == "{" ? false : true;

    if (setData)
    {
        QTextBlock prevBlock = currentBlock().previous();
        if (prevBlock.isValid())
        {
            if (TextBlockData *prevData = static_cast<TextBlockData *>(prevBlock.userData()))
            {
                if (prevData->getProperty() == property)
                    openNests = prevData->openNests();
            }
        }
    }

    QRegularExpression mixed (startExp.pattern() + "|" + endExp.pattern());
    int indx = -1, startIndx = 0;
    int txtL = text.length();
    QRegularExpressionMatch match;
    while (indx < txtL)
    {
        indx = text.indexOf (mixed, indx + 1, &match);
        while (isYamlBraceEscaped (text, startExp, indx) || isQuoted (text, indx))
            indx = text.indexOf (mixed, indx + match.capturedLength(), &match);
        if (format (indx) == commentFormat)
        {
            while (format (indx - 1) == commentFormat) --indx;
            if (indx > startIndx && openNests > 0)
                setFormat (startIndx, indx - startIndx, neutralFormat);
            break;
        }
        if (indx > -1)
        {
            if (text.indexOf (startExp, indx) == indx)
            {
                if (openNests == 0)
                    startIndx = indx;
                ++ openNests;
            }
            else
            {
                -- openNests;
                if (openNests == 0)
                    setFormat (startIndx, indx + match.capturedLength() - startIndx, neutralFormat);
                openNests = qMax (openNests, 0);
            }
        }
        else
        {
            if (openNests > 0)
            {
                indx = txtL;
                while (format (indx - 1) == commentFormat) --indx;
                if (indx > startIndx)
                    setFormat (startIndx, indx - startIndx, neutralFormat);
            }
            break;
        }
    }

    if (setData)
    {
        data->insertNestInfo (openNests);
        if (openNests == 0) // braces take priority over brackets
            property = false;
        data->setProperty (property);
        return (openNests != oldOpenNests || property != oldProperty);
    }
    return false; // don't update the next line if no data is set
}
/*************************/
// Completely commented lines are considered blank.
// It's also supposed that spaces don't affect blankness.
bool Highlighter::isFountainLineBlank (const QTextBlock &block)
{
    if (!block.isValid()) return false;
    QString text = block.text();
    if (block.previous().isValid())
    {
        if (block.previous().userState() == markdownBlockQuoteState)
            return false;
        if (block.previous().userState() == commentState)
        {
            int indx = text.indexOf (commentEndExpression);
            if (indx == -1 || indx == text.length() - 2)
                return true;
            text = text.right (text.length() - indx - 2);
        }
    }
    int index = 0;
    while (index < text.length())
    {
        while (index < text.length() && text.at (index).isSpace())
            ++ index;
        if (index == text.length()) break; // only spaces
        text = text.right (text.length() - index);
        if (!text.startsWith ("/*")) return false;
        index = 2;

        index = text.indexOf (commentEndExpression, index);
        if (index == -1) break;
        index += 2;
    }
    return true;
}
/*************************/
static inline bool isUpperCase (const QString & text)
{ // this isn't the same as QSting::isUpper()
    bool res = true;
    for (int i = 0; i < text.length(); ++i)
    {
        const QChar thisChar = text.at (i);
        if (thisChar.isLetter() && !thisChar.isUpper())
        {
            res = false;
            break;
        }
    }
    return res;
}

void Highlighter::highlightFountainBlock (const QString &text)
{
    bool rehighlightPrevBlock = false;

    TextBlockData *data = new TextBlockData;
    setCurrentBlockUserData (data);
    setCurrentBlockState (0);

    QTextCharFormat fi;

    static const QRegularExpression leftNoteBracket ("\\[\\[");
    static const QRegularExpression rightNoteBracket ("\\]\\]");
    static const QRegularExpression heading ("^\\s*(INT|EXT|EST|INT./EXT|INT/EXT|I/E|int|ext|est|int./ext|int/ext|i/e|\\.\\w)");
    static const QRegularExpression charRegex ("^\\s*@");
    static const QRegularExpression parenRegex ("^\\s*\\(.*\\)$");
    static const QRegularExpression lyricRegex ("^\\s*~");

    /* notes */
    multiLineComment (text, 0, -1, leftNoteBracket, QRegularExpression ("^ ?$|\\]\\]"), markdownBlockQuoteState, altQuoteFormat);
    /* boneyards (like a multi-line comment -- skips altQuoteFormat in notes with commentStartExpression) */
    multiLineComment (text, 0, -1, commentStartExpression, commentEndExpression, commentState, commentFormat);

    QTextBlock prevBlock = currentBlock().previous();
    QTextBlock nxtBlock = currentBlock().next();
    if (isFountainLineBlank (currentBlock()))
    {
        if (currentBlockState() != commentState)
            setCurrentBlockState (updateState); // to distinguish it
        if (prevBlock.isValid()
            && previousBlockState() != commentState && previousBlockState() != markdownBlockQuoteState
            && (isUpperCase (prevBlock.text()) || prevBlock.text().indexOf (charRegex) == 0
                || text.indexOf (heading) == 0))
        {
            rehighlightPrevBlock = true;
        }
    }
    else
    {
        if (prevBlock.isValid() && previousBlockState() != codeBlockState
            && (isUpperCase (prevBlock.text()) || prevBlock.text().indexOf (charRegex) == 0
                || text.indexOf (heading) == 0))
        {
            rehighlightPrevBlock = true;
        }

        QTextCharFormat fFormat;
        /* scene headings (between blank lines) */
        if (previousBlockState() == updateState && isFountainLineBlank (nxtBlock)
            && text.indexOf (heading) == 0)
        {
            fFormat.setFontWeight (QFont::Bold);
            fFormat.setForeground (Blue);
            setFormatWithoutOverwrite (0, text.length(), fFormat, commentFormat);
        }
        /* characters (following a blank line and not preceding one) */
        else if (previousBlockState() == updateState
                 && nxtBlock.isValid() && !isFountainLineBlank (nxtBlock)
                 && (text.indexOf (charRegex) == 0 || isUpperCase (text)))
        {
            fFormat.setFontWeight (QFont::Bold);
            fFormat.setForeground (DarkBlue);
            setFormatWithoutOverwrite (0, text.length(), fFormat, commentFormat);
            if (currentBlockState() != commentState && currentBlockState() != markdownBlockQuoteState)
                setCurrentBlockState (codeBlockState); // to distinguish it
        }
        /* transitions (between blank lines) */
        else if (previousBlockState() == updateState && isFountainLineBlank (nxtBlock)
                 && ((text.indexOf (QRegularExpression ("^\\s*>")) == 0
                      && text.indexOf (QRegularExpression ("<$")) == -1) // not centered
                     || (isUpperCase (text) && text.endsWith ("TO:"))))
        {
            fFormat.setFontWeight (QFont::Bold);
            fFormat.setForeground (DarkMagenta);
            fFormat.setFontItalic (true);
            setFormatWithoutOverwrite (0, text.length(), fFormat, commentFormat);
        }
        else
        {
            if (previousBlockState() == codeBlockState
                && currentBlockState() != commentState  && currentBlockState() != markdownBlockQuoteState)
            {
                setCurrentBlockState (codeBlockState); // to distinguish it for parentheticals
            }
            /* parentheticals (following characters or dialogs) */
            if (text.indexOf (parenRegex) == 0 && previousBlockState() == codeBlockState)
            {
                fFormat.setFontWeight (QFont::Bold);
                fFormat.setForeground (DarkGreen);
                setFormatWithoutOverwrite (0, text.length(), fFormat, commentFormat);
            }
            /* lyrics */
            else if (text.indexOf (lyricRegex) == 0)
            {
                fFormat.setFontItalic (true);
                fFormat.setForeground (DarkMagenta);
                setFormatWithoutOverwrite (0, text.length(), fFormat, commentFormat);
            }
        }
    }

    int index;

    /* main formatting */
    int bn = currentBlock().blockNumber();
    if (bn >= startCursor.blockNumber() && bn <= endCursor.blockNumber())
    {
        data->setHighlighted();
        for (const HighlightingRule &rule : qAsConst (highlightingRules))
        {
            if (rule.format == commentFormat)
                continue;
            QRegularExpressionMatch match;
            index = text.indexOf (rule.pattern, 0, &match);
            if (rule.format != whiteSpaceFormat)
            {
                fi = format (index);
                while (index >= 0 && fi != mainFormat)
                {
                    index = text.indexOf (rule.pattern, index + match.capturedLength(), &match);
                    fi = format (index);
                }
            }
            while (index >= 0)
            {
                int length = match.capturedLength();
                setFormat (index, length, rule.format);
                index = text.indexOf (rule.pattern, index + length, &match);

                if (rule.format != whiteSpaceFormat)
                {
                    fi = format (index);
                    while (index >= 0 && fi != mainFormat)
                    {
                        index = text.indexOf (rule.pattern, index + match.capturedLength(), &match);
                        fi = format (index);
                    }
                }
            }
        }
        fountainFonts (text);
    }

    /* left parenthesis */
    index = text.indexOf ('(');
    fi = format (index);
    while (index >= 0 && (fi == altQuoteFormat || fi == commentFormat))
    {
        index = text.indexOf ('(', index + 1);
        fi = format (index);
    }
    while (index >= 0)
    {
        ParenthesisInfo *info = new ParenthesisInfo;
        info->character = '(';
        info->position = index;
        data->insertInfo (info);

        index = text.indexOf ('(', index + 1);
        fi = format (index);
        while (index >= 0 && (fi == altQuoteFormat || fi == commentFormat))
        {
            index = text.indexOf ('(', index + 1);
            fi = format (index);
        }
    }

    /* right parenthesis */
    index = text.indexOf (')');
    fi = format (index);
    while (index >= 0 && (fi == altQuoteFormat || fi == commentFormat))
    {
        index = text.indexOf (')', index + 1);
        fi = format (index);
    }
    while (index >= 0)
    {
        ParenthesisInfo *info = new ParenthesisInfo;
        info->character = ')';
        info->position = index;
        data->insertInfo (info);

        index = text.indexOf (')', index +1);
        fi = format (index);
        while (index >= 0 && (fi == altQuoteFormat || fi == commentFormat))
        {
            index = text.indexOf (')', index + 1);
            fi = format (index);
        }
    }

    /* left bracket */
    index = text.indexOf (leftNoteBracket);
    fi = format (index);
    while (index >= 0 && fi != altQuoteFormat)
    {
        index = text.indexOf (leftNoteBracket, index + 2);
        fi = format (index);
    }
    while (index >= 0)
    {
        BracketInfo *info = new BracketInfo;
        info->character = '[';
        info->position = index;
        data->insertInfo (info);

        info = new BracketInfo;
        info->character = '[';
        info->position = index + 1;
        data->insertInfo (info);

        index = text.indexOf (leftNoteBracket, index + 2);
        fi = format (index);
        while (index >= 0 && fi != altQuoteFormat)
        {
            index = text.indexOf (leftNoteBracket, index + 2);
            fi = format (index);
        }
    }

    /* right bracket */
    index = text.indexOf (rightNoteBracket);
    fi = format (index);
    while (index >= 0 && fi != altQuoteFormat)
    {
        index = text.indexOf (rightNoteBracket, index + 2);
        fi = format (index);
    }
    while (index >= 0)
    {
        BracketInfo *info = new BracketInfo;
        info->character = ']';
        info->position = index;
        data->insertInfo (info);

        info = new BracketInfo;
        info->character = ']';
        info->position = index + 1;
        data->insertInfo (info);

        index = text.indexOf (rightNoteBracket, index + 2);
        fi = format (index);
        while (index >= 0 && fi != altQuoteFormat)
        {
            index = text.indexOf (rightNoteBracket, index + 2);
            fi = format (index);
        }
    }

    setCurrentBlockUserData (data);

    if (rehighlightPrevBlock)
    {
        QTextBlock prevBlock = currentBlock().previous();
        if (prevBlock.isValid())
            QMetaObject::invokeMethod (this, "rehighlightBlock", Qt::QueuedConnection, Q_ARG (QTextBlock, prevBlock));
    }
}
/*************************/
// Start syntax highlighting!
void Highlighter::highlightBlock (const QString &text)
{
    if (progLan.isEmpty()) return;

    int txtL = text.length();
    if (txtL <= 10000)
    {
        /* If the paragraph separators are shown, the unformatted text
           will be grayed out. So, we should restore its real color here.
           This is also safe when the paragraph separators are hidden. */
        setFormat (0, txtL, mainFormat);

        if (progLan == "fountain")
        {
            highlightFountainBlock (text);
            return;
        }
    }

    bool rehighlightNextBlock = false;
    int oldOpenNests = 0; QSet<int> oldOpenQuotes; // to be used in SH_CmndSubstVar() (and perl)
    bool oldProperty = false; // to be used with yaml
    QString oldLabel; // to be used with perl
    if (TextBlockData *oldData = static_cast<TextBlockData *>(currentBlockUserData()))
    {
        oldOpenNests = oldData->openNests();
        oldOpenQuotes = oldData->openQuotes();
        oldProperty = oldData->getProperty();
        oldLabel = oldData->labelInfo();
    }

    int index;
    TextBlockData *data = new TextBlockData;
    data->setLastState (currentBlockState()); // remember the last state (which may not be -1)
    setCurrentBlockUserData (data); // to be fed in later
    setCurrentBlockState (0); // start highlightng, with 0 as the neutral state

    /* set a limit on line length */
    if (txtL > 10000)
    {
        setFormat (0, txtL, translucentFormat);
        return;
    }

    /********************
     * "Here" Documents *
     ********************/

    if (progLan == "sh" || progLan == "makefile" || progLan == "cmake"
        || progLan == "perl" || progLan == "ruby")
    {
        if (isHereDocument (text))
        {
            data->setHighlighted(); // completely highlighted
            /* transfer the info on open quotes inside code blocks downwards */
            if (data->openNests() > 0)
            {
                QTextBlock nextBlock = currentBlock().next();
                if (nextBlock.isValid())
                {
                    if (TextBlockData *nextData = static_cast<TextBlockData *>(nextBlock.userData()))
                    {
                        if (nextData->openQuotes() != data->openQuotes()
                            || (nextBlock.userState() >= 0 && nextBlock.userState() < endState)) // end delimiter
                        {
                            QMetaObject::invokeMethod (this, "rehighlightBlock", Qt::QueuedConnection, Q_ARG (QTextBlock, nextBlock));
                        }
                    }
                }
            }
            return;
        }
    }
    /* just for debian control file */
    else if (progLan == "deb")
        debControlFormatting (text);

    int bn = currentBlock().blockNumber();
    bool mainFormatting (bn >= startCursor.blockNumber() && bn <= endCursor.blockNumber());
    QTextCharFormat fi;

    /************************
     * Single-Line Comments *
     ************************/

    if (progLan != "html")
        singleLineComment (text, 0);

    /* this is only for setting the format of
       command substitution variables in bash */
    rehighlightNextBlock |= SH_CmndSubstVar (text, data, oldOpenNests, oldOpenQuotes);

    /*******************
     * Python Comments *
     *******************/

    pythonMLComment (text, 0);

    /*******************************
     * XML Quotations and Comments *
     *******************************/

    if (progLan == "xml")
    {
        /* value is handled as a kind of comment */
        multiLineComment (text, 0, -1, QRegularExpression ("(>|&gt;)"), QRegularExpression ("(<|&lt;)"), xmlValueState, neutralFormat);
        /* multiline quotes as signs of errors in the xml doc */
        xmlQuotes (text);
    }
    /**************************
     * (Multiline) Quotations *
     **************************/
    else if (progLan == "sh") // bash has its own method
        SH_MultiLineQuote (text);
    else if (progLan != "diff" && progLan != "log"
             && progLan != "desktop" && progLan != "config" && progLan != "theme"
             && progLan != "changelog" && progLan != "url"
             && progLan != "srt" && progLan != "html"
             && progLan != "deb" && progLan != "m3u"
             && progLan != "reST"
             && progLan != "yaml") // yaml will be formated separately
    {
        rehighlightNextBlock |= multiLineQuote (text);
    }

    /*******
     * CSS *
     *******/

    /* helps seeing if a comment destroys a css block */
    int cssIndx = cssHighlighter (text, mainFormatting);

    /**********************
     * Multiline Comments *
     **********************/

    if (!commentStartExpression.pattern().isEmpty() && progLan != "python")
        multiLineComment (text, 0, cssIndx, commentStartExpression, commentEndExpression, commentState, commentFormat);

    /* only javascript, qml and perl */
    multiLineRegex (text, 0);
    if (progLan == "perl" && currentBlockState() == data->lastState())
        rehighlightNextBlock |= (data->labelInfo() != oldLabel || data->getProperty() != oldProperty
                                 || data->openNests() != oldOpenNests);

    /********
     * Yaml *
     ********/

    if (progLan == "yaml")
    {
        bool braces (true);
        int openNests = 0;
        QTextBlock prevBlock = currentBlock().previous();
        if (prevBlock.isValid())
        {
            if (TextBlockData *prevData = static_cast<TextBlockData *>(prevBlock.userData()))
            {
                openNests = prevData->openNests();
                if (openNests != 0) // braces take priority over brackets
                    braces = !prevData->getProperty();
            }
        }
        if (text.indexOf (QRegularExpression("^---")) == 0) // pass the data
        {
            data->insertNestInfo (openNests);
            data->setProperty (braces);
            rehighlightNextBlock |= oldOpenNests != openNests;
        }
        else // format braces and brackets before formatting multi-line quotes
        {
            if (previousBlockState() != codeBlockState || text.indexOf (QRegularExpression ("^\\S")) == 0)
            {
                if (braces)
                {
                    rehighlightNextBlock |= yamlOpenBraces (text, QRegularExpression ("{"), QRegularExpression ("}"), oldOpenNests, oldProperty, true);
                    rehighlightNextBlock |= yamlOpenBraces (text, QRegularExpression ("\\["), QRegularExpression ("\\]"), oldOpenNests, oldProperty,
                                                            data->openNests() == 0); // set data only if braces are completely closed
                }
                else
                {
                    rehighlightNextBlock |= yamlOpenBraces (text, QRegularExpression ("\\["), QRegularExpression ("\\]"), oldOpenNests, oldProperty, true);
                    rehighlightNextBlock |= yamlOpenBraces (text, QRegularExpression ("{"), QRegularExpression ("}"), oldOpenNests, oldProperty,
                                                            data->openNests() == 0); // set data only if brackets are completely closed
                }
            }
            else
            {
                data->insertNestInfo (0);
                data->setProperty (false);
            }
            if (data->openNests() == 0)
                multiLineComment (text, 0, -1, QRegularExpression (".*\\s+\\K(\\||>)\\s*$"), QRegularExpression ("^(?=\\S)"), codeBlockState, codeBlockFormat);

            rehighlightNextBlock |= multiLineQuote (text);
        }

        /* yaml Main Formatting  */
        if (mainFormatting)
        {
            data->setHighlighted();
            for (const HighlightingRule &rule : qAsConst (highlightingRules))
            {
                if (rule.format != whiteSpaceFormat
                    && previousBlockState() == codeBlockState
                    && text.indexOf (QRegularExpression ("^(?=\\S)")) != 0)
                { // a block
                    continue;
                }
                if (rule.format == commentFormat)
                    continue;

                QRegularExpressionMatch match;
                index = text.indexOf (rule.pattern, 0, &match);
                if (rule.format != whiteSpaceFormat)
                {
                    fi = format (index);
                    while (index >= 0
                           && (fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat
                               || fi == commentFormat || fi == urlFormat))
                    {
                        index = text.indexOf (rule.pattern, index + 1, &match);
                        fi = format (index);
                    }
                }
                while (index >= 0)
                {
                    int length = match.capturedLength();

                    /* check if there is a valid brace inside the regex
                       and if there is, limit the found match to it */
                    QString txt = text.mid (index, length);
                    int braceIndx = 0;
                    while ((braceIndx = txt.indexOf (QRegularExpression ("{"), braceIndx)) >= 0)
                    {
                        if (format (index + braceIndx) == neutralFormat
                            && !isYamlBraceEscaped (text, QRegularExpression ("{"), index + braceIndx))
                        {
                            txt = text.mid (index, braceIndx);
                            break;
                        }
                        ++ braceIndx;
                    }
                    braceIndx = 0;
                    while ((braceIndx = txt.indexOf (QRegularExpression ("}"), braceIndx)) >= 0)
                    {
                        if (format (index + braceIndx) == neutralFormat)
                        {
                            txt = text.mid (index, braceIndx);
                            break;
                        }
                        ++ braceIndx;
                    }
                    braceIndx = 0;
                    while ((braceIndx = txt.indexOf (QRegularExpression ("\\["), braceIndx)) >= 0)
                    {
                        if (format (index + braceIndx) == neutralFormat
                            && !isYamlBraceEscaped (text, QRegularExpression ("\\["), index + braceIndx))
                        {
                            txt = text.mid (index, braceIndx);
                            break;
                        }
                        ++ braceIndx;
                    }
                    braceIndx = 0;
                    while ((braceIndx = txt.indexOf (QRegularExpression ("\\]"), braceIndx)) >= 0)
                    {
                        if (format (index + braceIndx) == neutralFormat)
                        {
                            txt = text.mid (index, braceIndx);
                            break;
                        }
                        ++ braceIndx;
                    }
                    braceIndx = 0;
                    while ((braceIndx = txt.indexOf (QRegularExpression (","), braceIndx)) >= 0)
                    {
                        if (format (index + braceIndx) == neutralFormat)
                        {
                            txt = text.mid (index, braceIndx);
                            break;
                        }
                        ++ braceIndx;
                    }
                    length = txt.length();

                    if (length > 0)
                    {
                        fi = rule.format;
                        if (fi.foreground() == Violet)
                        {
                            if (txt.indexOf (QRegularExpression ("[-+]?\\d*\\.?\\d+(e(\\+|-)\\d+)?\\s*(?=(#|$))"), 0, &match) == 0)
                            { // format numerical values differently
                                if (match.capturedLength() == length)
                                    fi.setForeground (Brown);
                            }
                            else if (txt.indexOf (QRegularExpression ("(true|false|yes|no|TRUE|FALSE|YES|NO|True|False|Yes|No)\\s*(?=(#|$))"), 0, &match) == 0)
                            { // format booleans differently
                                if (match.capturedLength() == length)
                                {
                                    fi.setForeground (DarkBlue);
                                    fi.setFontWeight (QFont::Bold);
                                }
                            }
                        }
                        setFormat (index, length, fi);
                    }
                    index = text.indexOf (rule.pattern, index + qMax (length, 1), &match);

                    if (rule.format != whiteSpaceFormat)
                    {
                        fi = format (index);
                        while (index >= 0
                               && (fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat
                                   || fi == commentFormat || fi == urlFormat))
                        {
                            index = text.indexOf (rule.pattern, index + 1, &match);
                            fi = format (index);
                        }
                    }
                }
            }
        }
    }

    /************
     * Markdown *
     ************/

    else if (progLan == "markdown")
    {
        if (blockQuoteFormat.isValid() && codeBlockFormat.isValid())
        {
            int prevState = previousBlockState();
            /* the block quote of markdown is like a multiline comment
           but shouldn't be formatted inside a real comment */
            if (prevState != commentState)
                multiLineComment (text, 0, -1, QRegularExpression ("^>.*"), QRegularExpression ("^$"), markdownBlockQuoteState, blockQuoteFormat);
            /* the ``` code block of markdown is like a multiline comment
           but shouldn't be formatted inside a comment or block quote */
            if (prevState != commentState && prevState != markdownBlockQuoteState)
                multiLineComment (text, 0, -1, QRegularExpression ("```(?!`)"), QRegularExpression ("(?<![^\\s])```(?![^\\s])"), codeBlockState, codeBlockFormat);
            if (mainFormatting)
            {
                data->setHighlighted(); // completely highlighted
                for (const HighlightingRule &rule : qAsConst (highlightingRules))
                {
                    QRegularExpressionMatch match;
                    index = text.indexOf (rule.pattern, 0, &match);
                    if (rule.format != whiteSpaceFormat)
                    {
                        fi = format (index);
                        while (index >= 0
                               && (fi == blockQuoteFormat || fi == codeBlockFormat // the same as quoteFormat (for `...`)
                                   || fi == commentFormat || fi == urlFormat))
                        {
                            index = text.indexOf (rule.pattern, index + match.capturedLength(), &match);
                            fi = format (index);
                        }
                    }
                    while (index >= 0)
                    {
                        setFormat (index, match.capturedLength(), rule.format);
                        index = text.indexOf (rule.pattern, index + match.capturedLength(), &match);
                        if (rule.format != whiteSpaceFormat)
                        {
                            fi = format (index);
                            while (index >= 0
                                   && (fi == blockQuoteFormat || fi == codeBlockFormat
                                       || fi == commentFormat || fi == urlFormat))
                            {
                                index = text.indexOf (rule.pattern, index + match.capturedLength(), &match);
                                fi = format (index);
                            }
                        }
                    }
                }
                markDownFonts (text);
            }
        }
        /* go to braces matching */
    }

    /********************
     * reStructuredText *
     ********************/

    else if (progLan == "reST" && codeBlockFormat.isValid())
    {
        QRegularExpressionMatch match;

        /*******************************
        * reST Code and Comment Blocks *
        ********************************/
        QTextBlock prevBlock = currentBlock().previous();
        /* definitely, the start of a code block */
        if (text.indexOf (QRegularExpression ("^\\.{2} code-block::"), 0, &match) == 0)
        { // also overwrites commentFormat
            /* the ".. code-block::" part will be formatted later */
            setFormat (match.capturedLength(), text.count() - match.capturedLength(), codeBlockFormat);
            setCurrentBlockState (codeBlockState);
        }
        /* perhaps the start of a code block */
        else if (text.indexOf (QRegularExpression ("^(?!\\s*\\.{2}).*::$")) == 0)
        {
            bool isCommented (false);
            if (previousBlockState() >= endState || previousBlockState() < -1)
            {
                int spaces = text.indexOf (QRegularExpression ("\\S"));
                if (spaces > 0)
                {
                    if (TextBlockData *prevData = static_cast<TextBlockData *>(prevBlock.userData()))
                    {
                        QString prevLabel = prevData->labelInfo();
                        if (prevLabel.startsWith ("c") && text.startsWith (prevLabel.right(prevLabel.count() - 1)))
                        { // not a code block start but a comment
                            isCommented = true;
                            setFormat (0, text.count(), commentFormat);
                            setCurrentBlockState (previousBlockState());
                            data->insertInfo (prevLabel);
                        }
                    }
                }
            }
            /* definitely, the start of a code block */
            if (!isCommented)
            {
                QTextCharFormat blockFormat = codeBlockFormat;
                blockFormat.setFontWeight (QFont::Bold);
                setFormat (text.count() - 2,  2, blockFormat);
                setCurrentBlockState (codeBlockState);
            }
        }
        /* perhaps a comment */
        else if (text.indexOf (QRegularExpression ("^\\s*\\.{2}"
                                                   "(?!("
                                                       /* not a label or substitution (".. _X:" or ".. |X| Y::") */
                                                       " _[\\w\\s\\-+]*:(?!\\S)"
                                                       "|"
                                                       " \\|[\\w\\s]+\\|\\s+\\w+::(?!\\S)"
                                                       "|"
                                                       /* not a footnote (".. [#X]") */
                                                       " (\\[(\\w|\\s|-|\\+|\\*)+\\]|\\[#(\\w|\\s|-|\\+)*\\])\\s+"
                                                       "|"
                                                       /* not ".. X::" */
                                                       " (\\w|-)+::(?!\\S)"
                                                   "))"
                                                   "\\s+.*")) == 0)
        {
            bool isCodeLine (false);
            QString prevLabel;
            if (previousBlockState() == codeBlockState
                || previousBlockState() >= endState || previousBlockState() < -1)
            {
                if (TextBlockData *prevData = static_cast<TextBlockData *>(prevBlock.userData()))
                {
                    prevLabel = prevData->labelInfo();
                    if (prevLabel.isEmpty()
                        || (!prevLabel.startsWith ("c") && text.startsWith (prevLabel)))
                    { // not a commnt but a code line
                        isCodeLine = true;
                        if (prevLabel.isEmpty())
                        { // the code block was started or kept in the previous line
                            int spaces = text.indexOf (QRegularExpression ("\\S"));
                            if (spaces == -1) // spaces only keep the code block
                                setCurrentBlockState (codeBlockState);
                            else
                            { // a code line
                                setFormat (0, text.count(), codeBlockFormat);
                                if (spaces == 0) // a line without indent only keeps the code block
                                    setCurrentBlockState (codeBlockState);
                                else
                                {
                                    /* remember the starting spaces */
                                    QString spaceStr = text.left (spaces);
                                    data->insertInfo (spaceStr);
                                    /* also, the state should depend on the starting spaces */
                                    int n = static_cast<int>(qHash (spaceStr));
                                    int state = 2 * (n + (n >= 0 ? endState/2 + 1 : 0));
                                    setCurrentBlockState (state);
                                }
                            }
                        }
                        else
                        {
                            setFormat (0, text.count(), codeBlockFormat);
                            setCurrentBlockState (previousBlockState());
                            data->insertInfo (prevLabel);
                        }
                    }
                }
            }
            /* definitely a comment */
            if (!isCodeLine)
            {
                setFormat (0, text.count(), commentFormat);
                if ((previousBlockState() >= endState || previousBlockState() < -1)
                    && prevLabel.startsWith ("c") && text.startsWith (prevLabel.right(prevLabel.count() - 1)))
                {
                    setCurrentBlockState (previousBlockState());
                    data->insertInfo (prevLabel);
                }
                else
                {
                    /* remember the starting spaces (which consists of 3 spaces at least)
                       but add a "c" to its beginning to distinguish it from a code block */
                    QString spaceStr;
                    int spaces = text.indexOf (QRegularExpression ("\\S"));
                    if (spaces == -1)
                        spaceStr = "c   ";
                    else
                        spaceStr = "c" + text.left (spaces) + "   ";
                    data->insertInfo (spaceStr);
                    /* also, the state should depend on the starting spaces */
                    int n = static_cast<int>(qHash (spaceStr));
                    int state = 2 * (n + (n >= 0 ? endState/2 + 1 : 0)); // always an even number but maybe negative
                    setCurrentBlockState (state);
                }
            }
        }
        /* now, everything depends on the previous block */
        else if (prevBlock.isValid())
        {
            if (previousBlockState() == codeBlockState)
            { // the code block was started or kept in the previous line
                int spaces = text.indexOf (QRegularExpression ("\\S"));
                if (text.isEmpty() || spaces == -1) // spaces only keep the code block
                    setCurrentBlockState (codeBlockState);
                else
                { // a code line
                    setFormat (0, text.count(), codeBlockFormat);
                    if (spaces == 0) // a line without indent only keeps the code block
                        setCurrentBlockState (codeBlockState);
                    else
                    {
                        /* remember the starting spaces */
                        QString spaceStr = text.left (spaces);
                        data->insertInfo (spaceStr);
                        /* also, the state should depend on the starting spaces */
                        int n = static_cast<int>(qHash (spaceStr));
                        int state = 2 * (n + (n >= 0 ? endState/2 + 1 : 0));
                        setCurrentBlockState (state);
                    }
                }
            }
            else if (previousBlockState() >= endState || previousBlockState() < -1)
            {
                if (TextBlockData *prevData = static_cast<TextBlockData *>(prevBlock.userData()))
                {
                    QString prevLabel = prevData->labelInfo();
                    if (text.isEmpty()
                        || ((prevLabel.startsWith ("c") && text.startsWith (prevLabel.right(prevLabel.count() - 1)))
                            || text.startsWith (prevLabel)))
                    {
                        setCurrentBlockState (previousBlockState());
                        data->insertInfo (prevLabel);
                        if (prevLabel.startsWith ("c")) // a comment continues
                            setFormat (0, text.count(), commentFormat);
                        else
                            setFormat (0, text.count(), codeBlockFormat);
                    }
                }
            }
        }
        /***********************
        * reST Main Formatting *
        ************************/
        if (mainFormatting)
            reSTMainFormatting (0, text);
    }

    /*************
     * HTML Only *
     *************/

    else if (progLan == "html")
    {
        htmlBrackets (text);
        htmlCSSHighlighter (text);
        htmlJavascript (text);
        /* go to braces matching */
    }

    /*******************
     * Main Formatting *
     *******************/

    // we format html embedded javascript in htmlJavascript()
    else if (mainFormatting)
    {
        data->setHighlighted(); // completely highlighted
        for (const HighlightingRule &rule : qAsConst (highlightingRules))
        {
            /* single-line comments are already formatted */
            if (rule.format == commentFormat)
                continue;

            QRegularExpressionMatch match;
            index = text.indexOf (rule.pattern, 0, &match);
            /* skip quotes and all comments */
            if (rule.format != whiteSpaceFormat)
            {
                fi = format (index);
                while (index >= 0
                       && (fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat
                           || fi == commentFormat || fi == urlFormat
                           || fi == regexFormat))
                {
                    index = text.indexOf (rule.pattern, index + match.capturedLength(), &match);
                    fi = format (index);
                }
            }

            while (index >= 0)
            {
                int length = match.capturedLength();
                int l = length;
                /* In c/c++, the neutral pattern after "#define" may contain
                   a (double-)slash but it's always good to check whether a
                   part of the match is inside an already formatted region. */
                if (rule.format != whiteSpaceFormat)
                {
                    while (format (index + l - 1) == commentFormat
                           /*|| format (index + l - 1) == commentFormat
                           || format (index + l - 1) == urlFormat
                           || format (index + l - 1) == quoteFormat
                           || format (index + l - 1) == altQuoteFormat
                           || format (index + l - 1) == urlInsideQuoteFormat
                           || format (index + l - 1) == regexFormat*/)
                    {
                        -- l;
                    }
                }
                setFormat (index, l, rule.format);
                index = text.indexOf (rule.pattern, index + length, &match);

                if (rule.format != whiteSpaceFormat)
                {
                    fi = format (index);
                    while (index >= 0
                           && (fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat
                               || fi == commentFormat || fi == urlFormat
                               || fi == regexFormat))
                    {
                        index = text.indexOf (rule.pattern, index + match.capturedLength(), &match);
                        fi = format (index);
                    }
                }
            }
        }
    }

    /*********************************************
     * Parentheses, Braces and brackets Matching *
     *********************************************/

    /* left parenthesis */
    index = text.indexOf ('(');
    fi = format (index);
    while (index >= 0
           && (fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat
               || fi == commentFormat || fi == urlFormat
               || fi == regexFormat
               || (progLan == "sh" && isEscapedChar (text, index))))
    {
        index = text.indexOf ('(', index + 1);
        fi = format (index);
    }
    while (index >= 0)
    {
        ParenthesisInfo *info = new ParenthesisInfo;
        info->character = '(';
        info->position = index;
        data->insertInfo (info);

        index = text.indexOf ('(', index + 1);
        fi = format (index);
        while (index >= 0
               && (fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat
                   || fi == commentFormat || fi == urlFormat
                   || fi == regexFormat
                   || (progLan == "sh" && isEscapedChar (text, index))))
        {
            index = text.indexOf ('(', index + 1);
            fi = format (index);
        }
    }

    /* right parenthesis */
    index = text.indexOf (')');
    fi = format (index);
    while (index >= 0
           && (fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat
               || fi == commentFormat || fi == urlFormat
               || fi == regexFormat
               || (progLan == "sh" && isEscapedChar (text, index))))
    {
        index = text.indexOf (')', index + 1);
        fi = format (index);
    }
    while (index >= 0)
    {
        ParenthesisInfo *info = new ParenthesisInfo;
        info->character = ')';
        info->position = index;
        data->insertInfo (info);

        index = text.indexOf (')', index +1);
        fi = format (index);
        while (index >= 0
               && (fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat
                   || fi == commentFormat || fi == urlFormat
                   || fi == regexFormat
                   || (progLan == "sh" && isEscapedChar (text, index))))
        {
            index = text.indexOf (')', index + 1);
            fi = format (index);
        }
    }

    /* left brace */
    index = text.indexOf ('{');
    fi = format (index);
    while (index >= 0
           && (fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat
               || fi == commentFormat || fi == urlFormat
               || fi == regexFormat
               || (progLan == "yaml" && fi != neutralFormat)))
    {
        index = text.indexOf ('{', index + 1);
        fi = format (index);
    }
    while (index >= 0)
    {
        BraceInfo *info = new BraceInfo;
        info->character = '{';
        info->position = index;
        data->insertInfo (info);

        index = text.indexOf ('{', index + 1);
        fi = format (index);
        while (index >= 0
               && (fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat
                   || fi == commentFormat || fi == urlFormat
                   || fi == regexFormat
                   || (progLan == "yaml" && fi != neutralFormat)))
        {
            index = text.indexOf ('{', index + 1);
            fi = format (index);
        }
    }

    /* right brace */
    index = text.indexOf ('}');
    fi = format (index);
    while (index >= 0
           && (fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat
               || fi == commentFormat || fi == urlFormat
               || fi == regexFormat
               || (progLan == "yaml" && fi != neutralFormat)))
    {
        index = text.indexOf ('}', index + 1);
        fi = format (index);
    }
    while (index >= 0)
    {
        BraceInfo *info = new BraceInfo;
        info->character = '}';
        info->position = index;
        data->insertInfo (info);

        index = text.indexOf ('}', index +1);
        fi = format (index);
        while (index >= 0
               && (fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat
                   || fi == commentFormat || fi == urlFormat
                   || fi == regexFormat
                   || (progLan == "yaml" && fi != neutralFormat)))
        {
            index = text.indexOf ('}', index + 1);
            fi = format (index);
        }
    }

    /* left bracket */
    index = text.indexOf ('[');
    fi = format (index);
    while (index >= 0
           && (fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat
               || fi == commentFormat || fi == urlFormat
               || fi == regexFormat
               || (progLan == "yaml" && fi != neutralFormat)
               || (progLan == "sh" && isEscapedChar (text, index))))
    {
        index = text.indexOf ('[', index + 1);
        fi = format (index);
    }
    while (index >= 0)
    {
        BracketInfo *info = new BracketInfo;
        info->character = '[';
        info->position = index;
        data->insertInfo (info);

        index = text.indexOf ('[', index + 1);
        fi = format (index);
        while (index >= 0
               && (fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat
                   || fi == commentFormat || fi == urlFormat
                   || fi == regexFormat
                   || (progLan == "yaml" && fi != neutralFormat)
                   || (progLan == "sh" && isEscapedChar (text, index))))
        {
            index = text.indexOf ('[', index + 1);
            fi = format (index);
        }
    }

    /* right bracket */
    index = text.indexOf (']');
    fi = format (index);
    while (index >= 0
           && (fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat
               || fi == commentFormat || fi == urlFormat
               || fi == regexFormat
               || (progLan == "yaml" && fi != neutralFormat)
               || (progLan == "sh" && isEscapedChar (text, index))))
    {
        index = text.indexOf (']', index + 1);
        fi = format (index);
    }
    while (index >= 0)
    {
        BracketInfo *info = new BracketInfo;
        info->character = ']';
        info->position = index;
        data->insertInfo (info);

        index = text.indexOf (']', index +1);
        fi = format (index);
        while (index >= 0
               && (fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat
                   || fi == commentFormat || fi == urlFormat
                   || fi == regexFormat
                   || (progLan == "yaml" && fi != neutralFormat)
                   || (progLan == "sh" && isEscapedChar (text, index))))
        {
            index = text.indexOf (']', index + 1);
            fi = format (index);
        }
    }

    setCurrentBlockUserData (data);

    if (rehighlightNextBlock)
    {
        QTextBlock nextBlock = currentBlock().next();
        if (nextBlock.isValid())
            QMetaObject::invokeMethod (this, "rehighlightBlock", Qt::QueuedConnection, Q_ARG (QTextBlock, nextBlock));
    }
}

}
