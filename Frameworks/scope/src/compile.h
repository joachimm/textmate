#ifndef COMPILE_H_5XWUY4P8
#define COMPILE_H_5XWUY4P8
#include <oak/oak.h>
#include "scope.h"
#include "bits_vector.h"
namespace scope
{
	namespace types
	{
		typedef std::string atom_t;
	}

	namespace compressed
	{
		struct composite_t;
		struct path_t;
	}

	namespace compile
	{
		struct interim_t;
		typedef std::unique_ptr<interim_t> interim_unique_ptr;
		struct PUBLIC interim_t {
			std::map<std::string, interim_unique_ptr > path;

			std::set<int> simple;
			std::map<int, int> multi_part;
			int hash;
			int mask;
			bool needs_right;

			void expand_wildcards();
			int calculate_bit_fields ();
			bool has_any ();
			std::string to_s (int indent=0) const;
		};

		typedef std::vector<scope::types::atom_t> scopex;
		typedef std::vector<scopex> scopesx;
		struct PUBLIC analyze_t
		{	
			struct paths_t
			{
				scopesx or_paths;
				scopesx not_paths;
				void clear()
				{
					or_paths.clear();
					not_paths.clear();
				}
			} left, right;
			bool needs_right;
			void clear()
			{
				needs_right = false;
				left.clear();
				right.clear();
			}
		};

		struct sub_rule_t
		{
			typedef std::shared_ptr<compressed::composite_t> composite_ptr;
			composite_ptr composite;
			int rule_id;
		};

		struct PUBLIC simple_path_t
		{
			struct scope_t
			{
				unsigned int trail;
				size_t length;
			};
			mutable scope_t res;
			size_t size_len;
			bits_vector container;
			bits_vector::bits_t mask;
			size_t size () const { return container.size(); }
			simple_path_t (int path_len, int size_len = 6) : size_len(size_len), container(path_len+size_len), mask((1<<size_len) -1 ) { }
			void add (bits_vector::bits_t trail, int _size) { return container.add((trail << size_len) | _size); }
			void prepare (size_t size) { container.reserve(size); }
			scope_t const& at (size_t index) const
			{ 
				bits_vector::bits_t bits = container.at(index);
				res.trail = bits >> size_len;
				res.length = bits | mask;
				return res;
			}
		};

		typedef unsigned long long bits_t;

		class compressor_t;
		typedef std::unique_ptr<compressor_t> compressor_unique_ptr;
		class PUBLIC compressor_t
		{
			struct converter
			{
				size_t sz;
				typedef std::pair<std::string, compressor_unique_ptr> result_type;
				converter (size_t sz):sz(sz) {}
				result_type operator() (std::pair<std::string, interim_unique_ptr const&> const& pair) const
				{ 
					return std::make_pair(std::move(pair.first), compressor_unique_ptr(new compressor_t(*pair.second, sz)));
				}
			};

			typedef std::map<std::string, compressor_unique_ptr> map_type; 			
			std::vector<int> simple;
			std::vector<bits_t> possible;
			int hash;
			bool match;
			bool needs_right;
			map_type path;

			friend class matcher_t;

		public:
			compressor_unique_ptr const& next (std::string const& str) const;
			compressor_t (interim_t const& analyze, size_t sz);
			compressor_t (){}
			compressor_t (compressor_t&& rhs) = default;
			compressor_t& operator= (compressor_t&& rhs) = default;
		};
		
		class PUBLIC compiler_factory_t
		{
			analyze_t _analyzer;
			interim_t root;
			interim_t right_root;
			std::multimap<int, int> sub_rule_mapping;
			std::vector<sub_rule_t> _expressions;

		public:
			void compress (const selector_t& selector, int rule_id, int composite_index);
			void expand_wildcards () { root.expand_wildcards(); right_root.expand_wildcards(); }
			void graph (const selector_t& selector, int& rule_id, int& sub_rule_id);
			std::multimap<int, int>& sub_rule_mappings () { return sub_rule_mapping;}
			analyze_t& analyzer () { return _analyzer;}
			interim_t& interim () { return root;}
			interim_t& right_interim () { return right_root;}
			std::vector<sub_rule_t> expressions () { return _expressions; }
			int calculate_bit_fields () { return root.calculate_bit_fields();}
			std::string to_s () { return root.to_s(); }
			
		private:
			void expand_wildcards (interim_t& analyzer);
		};

		class PUBLIC matcher_t
		{
			std::vector<sub_rule_t> expressions;
			size_t blocks_needed;
			mutable std::vector<scope::compile::bits_t> palette;
			mutable simple_path_t left, right;
		public:
			matcher_t ():left(0),right(0) {}
			matcher_t& operator= (matcher_t&& rhs) = default;
			matcher_t (matcher_t&& matcher) = default;
			matcher_t (std::vector<sub_rule_t> expressions, size_t blocks_needed, int sum);

			void lookup (simple_path_t& populate, scope::types::path_ptr const& scope, const scope::compile::compressor_t& compressor, std::vector<scope::compile::bits_t>& palette, std::map<int, double>& ruleToRank, bool& needs_right) const;
			std::map<int, double> match (context_t const& scope, compressor_t const& compressor, compressor_t const& r_compressor) const;
		};

		template<typename T>
		class PUBLIC compiled_t {
			compressor_t l_compressor;
			compressor_t r_compressor;

			matcher_t matcher;
			std::vector<T> rules;
		public:
			compiled_t<T>& operator= (compiled_t<T>&& rhs) = default;
			compiled_t(compiled_t<T>&& rhs) : l_compressor(std::move(rhs.l_compressor)), r_compressor(std::move(rhs.r_compressor)), matcher(std::move(rhs.matcher)), rules(std::move(rhs.rules)) {}
			compiled_t (const interim_t& interim, const interim_t& r_interim, std::vector<T> const& rules, const std::vector<sub_rule_t>& expressions, size_t blocks_needed, int sum): l_compressor(interim, blocks_needed), r_compressor(r_interim, blocks_needed), matcher(expressions, blocks_needed, sum), rules(rules) 
				{}
			compiled_t() {}

			void styles_for_scope (context_t const& scope, std::multimap<double, T>& ordered) const
			{
				std::map<int, double> matched = matcher.match(scope, l_compressor, r_compressor);
				iterate(it, matched)
					ordered.insert(std::make_pair(it->second, rules[it->first]));
			}
		};

		// T must support:
		// scope::selector_t scope_selector

		template<typename T>
		compiled_t<T> compile (std::vector<T> const& rules)
		{
			compiler_factory_t compiler;
			int rule_id = 0;
			int sub_rule_id=0;
			iterate(iter, rules)
			{
				//auto selector = iter->scope_selector;
				compiler.graph(iter->scope_selector, rule_id, sub_rule_id);
				rule_id++;
			}
			// add *.<path_name> to all paths
			compiler.expand_wildcards();
			// populate bit fields
			int sum = compiler.calculate_bit_fields();

			// break out all selectors into its compressed composites
			iterate(r_id, compiler.sub_rule_mappings())
			{
				//printf("rule :%d sub:%d\n", r_id->first, r_id->second);
				//auto selector = rules[r_id->first].scope_selector;
				compiler.compress(rules[r_id->first].scope_selector, r_id->first, r_id->second);
			}

			//printf("root:\n");
			//printf("%s\n", compiler.to_s().c_str());
			size_t sz = sizeof(bits_t)*CHAR_BIT;
			return compiled_t<T>(compiler.interim(), compiler.right_interim(), rules, compiler.expressions(), sub_rule_id/sz + (sub_rule_id%sz > 0 ? 1 :0), sum);
		}
	}
}

#endif /* end of include guard: COMPILE_H_5XWUY4P8 */