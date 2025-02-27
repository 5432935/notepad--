#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "StringCopy.h"
#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "CharacterCategory.h"
#include "LexerModule.h"
#include "OptionSet.h"
#include "SubStyles.h"
#include "DefaultLexer.h"
#include "LexAccessor.h"

#include "UniConversion.h"

using namespace Scintilla;

const char styleSubable[] = { SCE_P_IDENTIFIER, 0 };

//Default = 0,//中文
//Ascii = 1,//英文
//Keyword = 2, //关键字，只有以TXT为母版的

LexicalClass lexicalClasses[] = {

#if 1
	// Lexer Python SCLEX_PYTHON SCE_P_:
	0, "SCE_TXT_DEFAULT", "default", "utf8 char",
	1, "SCE_TXT_ASCII", "Ascii", "Ascii",
	2, "SCE_TXT_KEYWORD", "keyword", "keyword",
	/*3, "SCE_P_STRING", "literal string", "String",
	4, "SCE_P_CHARACTER", "literal string", "Single quoted string",
	5, "SCE_P_WORD", "keyword", "Keyword",
	6, "SCE_P_TRIPLE", "literal string", "Triple quotes",
	7, "SCE_P_TRIPLEDOUBLE", "literal string", "Triple double quotes",
	8, "SCE_P_CLASSNAME", "identifier", "Class name definition",
	9, "SCE_P_DEFNAME", "identifier", "Function or method name definition",
	10, "SCE_P_OPERATOR", "operator", "Operators",
	11, "SCE_P_IDENTIFIER", "identifier", "Identifiers",
	12, "SCE_P_COMMENTBLOCK", "comment", "Comment-blocks",
	13, "SCE_P_STRINGEOL", "error literal string", "End of line where string is not closed",
	14, "SCE_P_WORD2", "identifier", "Highlighted identifiers",
	15, "SCE_P_DECORATOR", "preprocessor", "Decorators",
	16, "SCE_P_FSTRING", "literal string interpolated", "F-String",
	17, "SCE_P_FCHARACTER", "literal string interpolated", "Single quoted f-string",
	18, "SCE_P_FTRIPLE", "literal string interpolated", "Triple quoted f-string",
	19, "SCE_P_FTRIPLEDOUBLE", "literal string interpolated", "Triple double quoted f-string",*/
#endif
};

enum literalsAllowed { litNone = 0, litU = 1, litB = 2, litF = 4 };

// Options used for LexerPython
struct OptionsTxt {
	bool ascii;
	bool utf8;

	OptionsTxt() {
		ascii = true;
		utf8 = true;
	}

};


struct OptionSetTxt : public OptionSet<OptionsTxt> {
	OptionSetTxt() {
		DefineProperty("lexer.txt.utf8string", &OptionsTxt::utf8,
			"text Unicode string");

		DefineProperty("lexer.txt.asciistring", &OptionsTxt::ascii,
			"text literals ascii string");
	}
};

class LexTXT :public DefaultLexer
{
	WordList keywords;
	SubStyles subStyles;
	OptionsTxt options;
	OptionSetTxt osTxt;
public:
	explicit LexTXT() :
		DefaultLexer(lexicalClasses, ELEMENTS(lexicalClasses)),
		subStyles(styleSubable, 0x80, 0x40, 0) {
	}
	virtual ~LexTXT() {}

	void SCI_METHOD Release() override {
		delete this;
	}
	int SCI_METHOD Version() const override {
		return lvSubStyles;
	}
	const char *SCI_METHOD PropertyNames() override {
		return osTxt.PropertyNames();
	}
	int SCI_METHOD PropertyType(const char *name) override {
		return osTxt.PropertyType(name);
	}
	const char *SCI_METHOD DescribeProperty(const char *name) override {
		return osTxt.DescribeProperty(name);
	}
	Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;

	const char *SCI_METHOD DescribeWordListSets() override {
		return "";
	}
	Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;

	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
	void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
	void *SCI_METHOD PrivateCall(int, void *) override {
		return 0;
	}
	int SCI_METHOD LineEndTypesSupported() override {
		return SC_LINE_END_TYPE_UNICODE;
	}

	int SCI_METHOD AllocateSubStyles(int styleBase, int numberStyles) override {
		return subStyles.Allocate(styleBase, numberStyles);
	}
	int SCI_METHOD SubStylesStart(int styleBase) override {
		return subStyles.Start(styleBase);
	}
	int SCI_METHOD SubStylesLength(int styleBase) override {
		return subStyles.Length(styleBase);
	}
	int SCI_METHOD StyleFromSubStyle(int subStyle) override {
		const int styleBase = subStyles.BaseStyle(subStyle);
		return styleBase;
	}
	int SCI_METHOD PrimaryStyleFromStyle(int style) override {
		return style;
	}
	void SCI_METHOD FreeSubStyles() override {
		subStyles.Free();
	}
	void SCI_METHOD SetIdentifiers(int style, const char *identifiers) override {
		subStyles.SetIdentifiers(style, identifiers);
	}
	int SCI_METHOD DistanceToSecondaryStyles() override {
		return 0;
	}
	const char *SCI_METHOD GetSubStyleBases() override {
		return styleSubable;
	}

	static ILexer *LexerFactoryTxt() {
		return new LexTXT();
	}
};

Sci_Position SCI_METHOD LexTXT::PropertySet(const char *key, const char *val) {
	if (osTxt.PropertySet(&options, key, val)) {
		return 0;
	}
	return -1;
}

Sci_Position SCI_METHOD LexTXT::WordListSet(int n, const char *wl) {

	WordList *wordListN = 0;
	switch (n) {
	case 0:
		wordListN = &keywords;
		break;
}
	Sci_Position firstModification = -1;
	if (wordListN) {
		WordList wlNew;
		wlNew.Set(wl);
		if (*wordListN != wlNew) {
			wordListN->Set(wl);
			firstModification = 0;
		}
	}
	return firstModification;
}

const int indicatorWhitespace = 1;

inline bool IsAWordChar(int ch, bool unicodeIdentifiers) {
	if (ch < 0x80)
		return (isalnum(ch) || ch == '.' || ch == '_');

	if (!unicodeIdentifiers)
		return false;

	// Python uses the XID_Continue set from unicode data
	return IsXidContinue(ch);
}

inline bool IsAWordStart(int ch, bool unicodeIdentifiers) {
	if (ch < 0x80)
		return (isalpha(ch) || ch == '_');

	if (!unicodeIdentifiers)
		return false;

	// Python uses the XID_Start set from unicode data
	return IsXidStart(ch);
}

//只识别中文和英文两种单词的状态
void SCI_METHOD LexTXT::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
	Accessor styler(pAccess, NULL);

	const Sci_Position endPos = startPos + length;

	// Backtrack to previous line in case need to fix its tab whinging
	Sci_Position lineCurrent = styler.GetLine(startPos);
	if (startPos > 0) {
		if (lineCurrent > 0) {
			lineCurrent--;
			// Look for backslash-continued lines
			while (lineCurrent > 0) {
				Sci_Position eolPos = styler.LineStart(lineCurrent) - 1;
				const int eolStyle = styler.StyleAt(eolPos);
				if (eolStyle == SCE_P_STRING
					|| eolStyle == SCE_P_CHARACTER
					|| eolStyle == SCE_P_STRINGEOL) {
					lineCurrent -= 1;
				}
				else {
					break;
				}
			}
			startPos = styler.LineStart(lineCurrent);
		}
		initStyle = (startPos == 0 ? SCE_P_DEFAULT : styler.StyleAt(startPos - 1));
	}

	initStyle = initStyle & 31;
	if (initStyle == SCE_P_STRINGEOL) {
		initStyle = SCE_P_DEFAULT;
	}

	StyleContext sc(startPos, endPos - startPos, initStyle, styler);

	Sci_Position startIndicator = sc.currentPos;


	for (; sc.More();) {

		if (sc.state == SCE_P_IDENTIFIER) {

			//txt就三种状态、英文、中文、自定义关键字。默认是中文。
			//遇到非字符和非数字，开始检测单词,是关键字则识别为关键字;若不是关键字，则肯定是英文字符
			//关键字是英文，不是中文。

			if ((sc.ch == '.') || (!IsAWordChar(sc.ch, false))) {
				char s[1000];
				sc.GetCurrent(s, sizeof(s));
				int style = SCE_P_IDENTIFIER;
				if (keywords.InList(s)) 
		{
					style = SCE_TXT_KEYWORD;
		}
				else
		{
					//不是关键字，就是普通的英文单词
					style = SCE_TXT_ASCII;
				}
				sc.ChangeState(style);

				//下面函数运行就已经把关键字或英文给单独设置风格了。此时默认进入中文风格状态
			sc.SetState(SCE_TXT_DEFAULT);

		}
	}

		// Check for a new state starting character
		if (sc.state == SCE_TXT_DEFAULT)
	{
			//遇到下一个英文字符的时候，进入识别状态
			if (IsAWordStart(sc.ch, false)) 
	{
				sc.SetState(SCE_P_IDENTIFIER);
	}
		}
		sc.Forward();
	}
	
	//这里看起来最后一段中文，会识别不出来。因为上面的循环不能识别最后一段是中文的情况。
	//不过中文本来默认就是0，就算识别不到，本来就是默认的值。所有是没有问题的。

	styler.IndicatorFill(startIndicator, sc.currentPos, indicatorWhitespace, 0);
	sc.Complete();

}

static bool IsCommentLine(Sci_Position line, Accessor &styler) {
	Sci_Position pos = styler.LineStart(line);
	const Sci_Position eol_pos = styler.LineStart(line + 1) - 1;
	for (Sci_Position i = pos; i < eol_pos; i++) {
		const char ch = styler[i];
		if (ch == '#')
			return true;
		else if (ch != ' ' && ch != '\t')
			return false;
	}
	return false;
}

static bool IsQuoteLine(Sci_Position line, const Accessor &styler) {
	const int style = styler.StyleAt(styler.LineStart(line)) & 31;
	return false;
}


//不处理任何折叠
void SCI_METHOD LexTXT::Fold(Sci_PositionU startPos, Sci_Position length, int /*initStyle - unused*/, IDocument *pAccess) {
	return;
}

static const char *const txtWordListDesc[] = {
	0
};

LexerModule lmTxt(SCLEX_TXT, LexTXT::LexerFactoryTxt, "txt", txtWordListDesc);
