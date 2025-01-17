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

#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QRegularExpression>

namespace FeatherPad {

struct ParenthesisInfo
{
    char character; // '(' or ')'
    int position;
};

struct BraceInfo
{
    char character; // '{' or '}'
    int position;
};

struct BracketInfo
{
    char character; // '[' or ']'
    int position;
};


/* This class gathers all the information needed for
   highlighting the syntax of the current block. */
class TextBlockData : public QTextBlockUserData
{
public:
    TextBlockData() :
        Highlighted (false),
        Property (false),
        LastState (0),
        OpenNests (0),
        LastFormattedQuote (0),
        LastFormattedRegex (0) {}
    ~TextBlockData();

    QVector<ParenthesisInfo *> parentheses() const;
    QVector<BraceInfo *> braces() const;
    QVector<BracketInfo *> brackets() const;
    QString labelInfo() const;
    bool isHighlighted() const;
    bool getProperty() const;
    int lastState() const;
    int openNests() const;
    int lastFormattedQuote() const;
    int lastFormattedRegex() const;
    QSet<int> openQuotes() const;

    void insertInfo (ParenthesisInfo *info);
    void insertInfo (BraceInfo *info);
    void insertInfo (BracketInfo *info);
    void insertInfo (const QString &str);
    void setHighlighted();
    void setProperty (bool p);
    void setLastState (int state);
    void insertNestInfo (int nests);
    void insertLastFormattedQuote (int last);
    void insertLastFormattedRegex (int last);
    void insertOpenQuotes (const QSet<int> &openQuotes);

private:
    QVector<ParenthesisInfo *> allParentheses;
    QVector<BraceInfo *> allBraces;
    QVector<BracketInfo *> allBrackets;
    QString label; // A label (usually, a delimiter string, like that of a here-doc).
    bool Highlighted; // Is this block completely highlighted?
    bool Property; // A general boolean property (used with SH, Perl and YAML).
    int LastState; // The state of this block before it is highlighted (again).
    /* "Nest" is a generalized bracket. This variable
       is the number of unclosed nests in a block. */
    int OpenNests;
    /* "LastFormattedQuote" is used when quotes are formatted
       on the fly, before they are completely formatted. */
    int LastFormattedQuote;
    int LastFormattedRegex;
    QSet<int> OpenQuotes; // The numbers of open double quotes of open nests.
};
/*************************/
/* This is a tricky but effective way for syntax highlighting. */
class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    Highlighter (QTextDocument *parent, const QString& lang,
                 const QTextCursor &start, const QTextCursor &end,
                 bool darkColorScheme,
                 bool showWhiteSpace = false, bool showEndings = false);
    ~Highlighter();

    void setLimit (const QTextCursor &start, const QTextCursor &end) {
        startCursor = start;
        endCursor = end;
    }

protected:
    void highlightBlock (const QString &text);

private:
    QStringList keywords (const QString &lang);
    QStringList types();
    bool isEscapedChar (const QString &text, const int pos) const;
    bool isEscapedQuote (const QString &text, const int pos, bool isStartQuote,
                         bool skipCommandSign = false);
    bool isQuoted (const QString &text, const int index,
                   bool skipCommandSign = false);
    bool isPerlQuoted (const QString &text, const int index);
    bool isMLCommented (const QString &text, const int index, int comState = commentState,
                        const int start = 0);
    bool isHereDocument (const QString &text);
    void pythonMLComment (const QString &text, const int indx);
    void htmlCSSHighlighter (const QString &text, const int start = 0);
    void htmlBrackets (const QString &text, const int start = 0);
    void htmlJavascript (const QString &text);
    int cssHighlighter (const QString &text, bool mainFormatting, const int start = 0);
    void singleLineComment (const QString &text, const int start);
    void multiLineComment (const QString &text,
                           const int index, const int cssIndx,
                           const QRegularExpression &commentStartExp, const QRegularExpression &commentEndExp,
                           const int commState,
                           const QTextCharFormat &comFormat);
    bool textEndsWithBackSlash (const QString &text);
    bool multiLineQuote (const QString &text,
                         const int start = 0,
                         int comState = commentState);
    void multiLinePerlQuote(const QString &text);
    void xmlQuotes (const QString &text);
    void setFormatWithoutOverwrite (int start,
                                    int count,
                                    const QTextCharFormat &newFormat,
                                    const QTextCharFormat &oldFormat);
    /* SH specific methods: */
    void SH_MultiLineQuote(const QString &text);
    bool SH_SkipQuote (const QString &text, const int pos, bool isStartQuote);
    int formatInsideCommand (const QString &text,
                             const int minOpenNests, int &nests, QSet<int> &quotes,
                             const bool isHereDocStart, const int index);
    bool SH_CmndSubstVar (const QString &text,
                          TextBlockData *currentBlockData,
                          int oldOpenNests, const QSet<int> &oldOpenQuotes);

    void markDownFonts (const QString &text);
    void fountainFonts (const QString &text);
    void reSTMainFormatting (int start, const QString &text);
    void debControlFormatting (const QString &text);

    bool isEscapedRegex (const QString &text, const int pos);
    bool isEscapedPerlRegex (const QString &text, const int pos);
    bool isInsideRegex (const QString &text, const int index);
    bool isInsidePerlRegex (const QString &text, const int index);
    void multiLineRegex (const QString &text, const int index);
    void multiLinePerlRegex (const QString &text);
    int findDelimiter (const QString &text, const int index,
                       const QRegularExpression &delimExp, int &capturedLength) const;

    bool yamlOpenBraces (const QString &text,
                         const QRegularExpression &startExp, const QRegularExpression &endExp,
                         int oldOpenNests, bool oldProperty,
                         bool setData);

    bool isFountainLineBlank (const QTextBlock &block);
    void highlightFountainBlock (const QString &text);

    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    /* Multiline comments: */
    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression;

    QTextCharFormat mainFormat; // The format before highlighting.
    QTextCharFormat neutralFormat; // When a color near that of mainFormat is needed.
    QTextCharFormat commentFormat;
    QTextCharFormat quoteFormat; // Usually for double quote.
    QTextCharFormat altQuoteFormat; // Usually for single quote.
    QTextCharFormat urlInsideQuoteFormat;
    QTextCharFormat urlFormat;
    QTextCharFormat blockQuoteFormat;
    QTextCharFormat codeBlockFormat;
    QTextCharFormat whiteSpaceFormat; // For whitespaces.
    QTextCharFormat translucentFormat;
    QTextCharFormat regexFormat;

    /* Programming language: */
    QString progLan;

    QRegularExpression quoteMark;
    QColor Blue, DarkBlue, Red, DarkRed, Verda, DarkGreen, DarkGreenAlt, DarkMagenta, Violet, Brown, DarkYellow;

    /* The start and end cursors of the visible text: */
    QTextCursor startCursor, endCursor;

    /* Block states: */
    enum
    {
        commentState = 1,

        /* Next-line commnets (ending back-slash in c/c++): */
        nextLineCommentState,

        /* Quotation marks: */
        doubleQuoteState,
        singleQuoteState,

        SH_DoubleQuoteState,
        SH_SingleQuoteState,
        SH_MixedDoubleQuoteState,
        SH_MixedSingleQuoteState,

        /* Python comments: */
        pyDoubleQuoteState,
        pySingleQuoteState,

        xmlValueState,

        /* Markdown: */
        markdownBlockQuoteState,

        /* Markdown and reStructuredText */
        codeBlockState,

        /* Regex inside JavaScript, QML and Perl: */
        regexSearchState, // search and replace (only in Perl)
        regexState,
        regexEndState, // the line ends with a JS regex (+ spaces)

        /* HTML: */
        htmlBracketState,
        htmlStyleState,
        htmlStyleDoubleQuoteState,
        htmlStyleSingleQuoteState,
        htmlCSSState,
        htmlCSSCommentState,
        htmlJavaState,
        htmlJavaCommentState,

        /* CSS: */
        cssBlockState,
        commentInCssState,
        cssValueState,

        /* Used to update the format of the next line (as in JavaScript): */
        updateState,

        endState // 28

        /* For here-docs, state >= endState or state < -1. */
    };
};

}

#endif // HIGHLIGHTER_H
