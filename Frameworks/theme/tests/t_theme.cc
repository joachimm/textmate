#include <theme/theme.h>
/*
template<typename T>
std::vector<std::vector<std::string> >& operator<<(std::vector<std::vector<std::string> >& v, T i) {
	v.push_back(i);
	return v;
}
*/
class ThemeTests : public CxxTest::TestSuite
{
	public:
	struct pathx_t;
	struct pathx_t {
		std::map<std::string, pathx_t> _path;
		bool match;
		pathx_t& operator[](std::string str) {
			return _path[str];
		}
		pathx_t& set(bool b) {
			match = b;
			return *this;
		}
	} root;

	pathx_t& next(std::string& str, pathx_t& path) {
		std::map<std::string, pathx_t>::iterator it = path._path.find(str);
		if(it != path._path.end()) 
			return it->second;
		it = path._path.find("*");
		if(it != path._path.end()) 
			return it->second;
		
		return NULL; 
	}

	void test_scope_theme ()
	{
		static scope::scope_t const textScope = "text.html.markdown meta.paragraph.markdown markup.bold.markdown";
		static scope::selector_t const matchingSelectors[] =
		{
			"text.* markup.bold",
			"text markup.bold",
			"markup.bold",
			"text.html meta.*.markdown markup",
			"text.html meta.* markup",
			"text.html * markup",
			"text.html markup",
			"text markup",
			"markup",
			"text.html",
			"text"
		};

		std::multimap<double, std::string> ordering;
		for(size_t i = 0; i < sizeofA(matchingSelectors); ++i)
		{
			double rank;
			if(matchingSelectors[i].does_match(textScope, &rank))
				ordering.insert(std::make_pair(rank, to_s(matchingSelectors[i])));
		}
		
		fprintf(stderr, "scope: %s\n", to_s(textScope).c_str());		
		iterate(it, ordering)
			fprintf(stderr, "rank: %f %s\n", it->first, it->second.c_str());
		
		std::string const li1[] = {"text","html","markdown"};
		std::string const li2[] = {"meta","paragraph","markdown"};
		std::string const li3[] = 	{"markup","bold","markdown"};

		iterate(cl, li) {
			p.push_back(std::vector<std::string> >);
			iterate(c, cl)
				p.back().push_back(c);				
		}
		
		root["text"].set(true)["html"].set(true);
		root["text"]["*"].set(true);
		root["markup"].set(true)["bold"].set(true);;
		root["*"];
		root["meta"]["*"].set(true)["markdown"].set(true);
		
		size_t s = p.size();
		for(int i = 0; i < s; i++ )
		{
			pathx_t path = root;
			int j = 0;
			while(pathx_t* current = next(p[i][j], path))
			{
				path = current;
			}
		}

	}
	
};