#include <theme/theme.h>
#include <oak/duration.h>

class ThemeTests : public CxxTest::TestSuite
{
	int repeat = 10000;
	public:
	struct pathx_t;
	struct pathx_t {
		std::map<std::string, pathx_t> _path;
		bool match;
		int possible;
		int route;
		pathx_t& operator[](std::string str) {
			return _path[str];
		}
		pathx_t& set(bool b,int m, int r) {
			match = b;
			possible = m;
			route = r;
			return *this;
		}

		std::string to_s () const {
			std::string res = "< match=";
			res+= (match?"true":"false" );
			std::stringstream sstream;
			sstream << "\n  route=0x" << std::hex << route << "\n  possible=0x" << std::hex << possible;
			std::string result = sstream.str();
			res+= result;
			
			res+= "\n {";
			iterate(it, _path)
				res += it->first+"= "+ it->second.to_s();
		  
			res += "\n}>";
			return res;
		}
	};

	pathx_t* next(std::string& str, pathx_t& path) {
		std::map<std::string, pathx_t>::iterator it = path._path.find(str);
		if(it != path._path.end()) 
			return &it->second;
		it = path._path.find("*");
		if(it != path._path.end()) 
			return &it->second;
		
		return NULL; 
	}

	void test_scope_theme ()
	{
		static scope::scope_t const textScope = "text.html.markdown meta.paragraph.markdown markup.bold.markdown";
		static scope::selector_t const matchingSelectors[] =
		{
			"text.* markup.bold", // 0
			"text markup.bold",
			"markup.bold",
			"text.html meta.*.markdown markup",
			"text.html meta.* markup",
			"text.html * markup", // 5
			"text.html markup",
			"text markup",
			"markup",
			"text.html",
			"text" //10
		};
		// [text,markup,meta,*][bold,html,*][markdown]
		double total = 0.0;
		oak::duration_t timer1;

		for(size_t times = 0 ; times < repeat ;times++){
			total = 0.0;
		std::multimap<double, std::string> ordering;
		for(size_t i = 0; i < sizeofA(matchingSelectors); ++i)
		{
			double rank;
			if(matchingSelectors[i].does_match(textScope, &rank))
				ordering.insert(std::make_pair(rank, to_s(matchingSelectors[i])));
		}
		
		iterate(it, ordering) {
			// printf("item: %.4f %s\n", it->first, it->second.c_str());
			total += it->first;
		}
	   }
		printf("%.4f seconds to computed oldstyle:%.1f\n", timer1.duration(), total);
		
	}
	struct rules_t {
		struct internal_t {
			struct pattern_t{
				int pattern;
				int mask;
				int size;
			};
			std::vector<pattern_t>  matchingSelectors;
			pattern_t operator[](size_t idx) { return matchingSelectors[idx];}
			internal_t& add(int p, int m, int s) {
				matchingSelectors.push_back(pattern_t());
				matchingSelectors.back().pattern = p;
				matchingSelectors.back().mask = m;
				matchingSelectors.back().size = s;
				
				return *this;
			}
			internal_t& add(int p, int m) {
				return add(p, m, route_length(p));
			}			
			size_t size() {
				return matchingSelectors.size();
			}
		} internal;
		std::map<size_t, internal_t> matchingSelectors;
		
		internal_t& operator[](size_t idx) {
			return matchingSelectors[idx];
		}
	};
	void test_scope_theme_benchmark ()
	{
		rules_t matchingSelectors;
		// path - mask
		//"text.* markup.bold", 
		matchingSelectors[0].add(3<<4|1<<1,7<<4|1<<1).add( 5<<4|3<<1, 7<<4|7<<1);
		//"text markup.bold"
		matchingSelectors[1].add(3<<4, 7<<4).add( 5<<4|3<<1, 7<<4|7<<1);
		//"text.html meta.*.markdown markup"
		matchingSelectors[3].add(3<<4|5<<1, 7<<4|7<<1).add( 7<<4|1<<1|1<<0, 7<<4|1<<1|1<<0).add( 5<<4, 7<<4);
		//"text.html meta.* markup"
		matchingSelectors[4].add(3<<4|5<<1, 7<<4|7<<1).add( 7<<4|1<<1, 7<<4|1<<1).add( 5<<4, 7<<4);
		//"text.html * markup" 
		matchingSelectors[5].add(3<<4|5<<1, 7<<4|7<<1).add( 1<<4, 1<<4).add( 5<<4,7<<4);
		 //"text.html markup"
		matchingSelectors[6].add(3<<4|5<<1, 7<<4|7<<1).add( 5<<4, 7<<4);
		//"text markup"
		matchingSelectors[7].add(3<<4, 7<<4).add( 5<<4, 7<<4); 
		// [text 3<<4,markup 5<<4,meta 7<<4,* 1<<4][bold 3<<1,html 5<<1,* 1<<1][markdown 1]
		
		static std::string const selectorStrings[] =
		{
			"text.* markup.bold", // 0
			"text markup.bold",
			"markup.bold",
			"text.html meta.*.markdown markup",
			"text.html meta.* markup",
			"text.html * markup", // 5
			"text.html markup",
			"text markup",
			"markup",
			"text.html",
			"text" //10
		};
		
		pathx_t root;
		root["text"].set(true,1<<1|1<<7, 3<<4)["html"]
			.set(true,1<<0|1<<3|1<<4|1<<5|1<<6|  1<<1|1<<7, 3<<4|5<<1);//1<<0*
		root["text"]["*"].set(false,1<<0  |1<<1|1<<7, 3<<4|1<<1);
		int help = 1 << 3 | 1 << 4 | 1 << 5 | 1 << 6| 1<<7;
		root["markup"].set(true, help, 5<<4)["bold"]
			.set(true,1<<0|1<<1|help, 5<<4|3<<1);
		root["*"].set(false,1<<5,1<<4);
		root["meta"]["*"].set(false,1<<4,7<<4|1<<1)["markdown"].set(false,1<<3  |1<<4, 7<<4|1<<1|1<<0);
		
		auto p = std::vector<std::vector<std::string> >();
		{
			std::string const cl[] = {"text","html","markdown"};
			p.push_back(std::vector<std::string> ());
			iterate(c, cl)
				p.back().push_back(*c);				
		}
		
		{
			std::string const cl[] = {"meta","paragraph","markdown"};
			p.push_back(std::vector<std::string> ());
			iterate(c, cl)
				p.back().push_back(*c);				
		}
		
		{
			std::string const cl[] = {"markup","bold","markdown"};
			p.push_back(std::vector<std::string> ());
			iterate(c, cl)
				p.back().push_back(*c);				
		}

		//f// printf(stderr, "pathx: %s\n", root.to_s().c_str());
		double total = 0.0;		
		oak::duration_t timer2;
		
		int computed = 0;
		for(size_t times = 0 ; times < repeat ;times++){
			total = 0.0;
			
			size_t s = p.size();
			std::multimap<double, std::string> ordering;
			rules_t::internal_t v;
			int power = 0;
			
		while(s--)
		{
			pathx_t* path = &root;
			int j = 0;
			int sz = p[s].size();
			power += sz;
						
			while(pathx_t* current = next(p[s][j], *path))
			{
				j++;
				path = current;
				if(path->match)
				{
					double score = 0;
					for(size_t k = 0; k < j; ++k)
						score += 1 / pow(2, power - k);
					ordering.insert(std::make_pair(score, p[s][j-1]));
				}
			}
			computed = computed | path->possible;
			v.add(path->route, 0, sz);
			
		}
		/*
		iterate(it, v) {
			// printf("vector item: %#x \n", *it);
		}
		*/
		
		while(int idx = ffs(computed)) {
			// printf("ffs %d %s\n",idx, selectorStrings[idx-1].c_str());
			
			double rank;
			if(does_match(v, matchingSelectors[idx - 1],&rank) ) {
				ordering.insert(std::make_pair(rank, selectorStrings[idx-1]));
				// printf("---ffs %d\n matched",idx);

			}
			computed &= ~(1<<idx-1);
		}
		
		iterate(it, ordering) {
			// printf("item: %.4f %s\n", it->first, it->second.c_str());
			total += it->first;
		}
	   		
		}
		
		printf("%.4f seconds to computed path_x:%.1f \n", timer2.duration(), total);
	}
	static size_t route_length(const int& route) {
		if(route & 2){ // 1<<1
			if(route & 1) {
				return 3;
			}
			return 2;
		}
		return 1;
	}
	bool does_match (rules_t::internal_t& path, rules_t::internal_t& scopes, double* rank ) const
	{

		size_t i = path.size(); // “source.ruby string.quoted.double constant.character”
		size_t j = scopes.size();      // “string > constant $”
		size_t s = i;
		// printf("path size: %zu\n", i);
		// printf("size: %zu\n", j);

		double score = 0;
		double power = 0;
		while(j <= i && j)
		{
			// if(anchor_to_bol)
			// if(anchor_to_next)
			power += path[s-i].size;//route_length(path[s-i]);
			// printf("scope:%#x path:%#x mask:%#x masked:%#x i=%zu j=%zu\n ", scopes[j-1].pattern, path[s-i], scopes[j-1].mask, path[s-i] & scopes[j-1].mask, i, j);

			if(scopes[j-1].pattern  == (path[s-i].pattern & scopes[j-1].mask))
			{
				for(size_t k = 0; k < scopes[j-1].size; ++k)
					score += 1 / pow(2, power - k);
				--j;
			}
			--i;
		}

		// if(anchor_to_eol)
		if(j == 0 && rank)
			*rank = score;
		return j == 0;
	}
	
};