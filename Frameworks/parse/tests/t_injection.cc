#include "support.cc"
#include <test/bundle_index.h>

static bundles::item_ptr InjectionTestGrammarItem;

static class InjectionGrammarFixture : public CxxTest::GlobalFixture
{
public:
	bool setUpWorld()
	{
		static std::string InjectionTestLanguageGrammar =
			"{	injections = {"
			"		string.quoted = {"
			"			name = 'todo';"
			"			match = '\\btodo\\b';"
			"		};"
			"		string.quoted.embed = {"
			"			name = 'source.ruba';"
			"			patterns = ( );"
			"		};"
			"	};"
			"	patterns = ("
			"		{	name = 'string.quoted.embed';"
			"			begin = '\"';"
			"			end = '\"';"
			"			patterns = ("
			"				{	name = 'constant.character.escape.untitled';"
			"					match = '\\.';"
			"				},"
			"			);"
			"		},"
			"	);"
			"}";
		test::bundle_index_t bundleIndex;
		InjectionTestGrammarItem           = bundleIndex.add(bundles::kItemTypeGrammar, InjectionTestLanguageGrammar);
		return bundleIndex.commit();
	}

} Injection_grammar_fixture;

class InjectionsTests : public CxxTest::TestSuite
{
public:
	void test_injections ()
	{
		auto grammar = parse::parse_grammar(InjectionTestGrammarItem);
		auto mark = markup(grammar, " \" todo \" ");
		TS_ASSERT_EQUALS(mark, "«￿» «string.quoted.embed»\"«source.ruba» «todo»todo«/todo» \"«/source.ruba»«/string.quoted.embed» «/￿»");
	}

};
