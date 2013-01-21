#include "compile.h"
#include "compressed.h"
#include "types.h"

scope::compile::compressor_t::compressor_t (interim_t const& interim, size_t sz): possible(sz), path(
			boost::make_transform_iterator(interim.path.begin(), converter(sz)),
				boost::make_transform_iterator(interim.path.end(), converter(sz))
			) 
{
	size_t block_sz = sizeof(bits_t)*CHAR_BIT;
	// set the bit for each sub_rule that is affected by this scope
	iterate(o, interim.multi_part)
		possible[o->first/block_sz] = 1L << (o->first%block_sz);
	iterate(o, interim.simple)
		simple.push_back(*o);
	match = interim.simple.size() > 0;
	hash = interim.hash;
	needs_right = interim.needs_right;
}

static scope::compile::compressor_unique_ptr EMPTY;
scope::compile::compressor_unique_ptr const& scope::compile::compressor_t::next(std::string const& str) const{
	typedef map_type::const_iterator iterator;

	iterator it = this->path.find(str);
	iterator last = this->path.end();
	if(it != last) 
		return it->second;

	// We use the fact that '*' is sorted before alphabetical letters 
	if(this->path.size() > 0 && this->path.begin()->first == scope::types::atom_any)
		return this->path.begin()->second;

	return EMPTY; 
}

scope::compile::matcher_t::matcher_t(std::vector<compile::sub_rule_t> expressions, size_t blocks_needed, int sum): expressions(expressions), blocks_needed(blocks_needed), palette(blocks_needed), left(sum), right(sum) {}

void scope::compile::matcher_t::lookup (scope::compile::simple_path_t& populate, scope::types::path_ptr const& scope, const scope::compile::compressor_t& compressor, std::vector<scope::compile::bits_t>& palette, std::map<int, double>& ruleToRank, bool& needs_right) const
{
	std::vector<scope::types::scope_t>& path = scope->scopes;

	iterate(sim, compressor.simple)
		ruleToRank[*sim] = 0.0;
	size_t s = path.size();
	populate.prepare(s);

	int power = 0;
	while(s--)
	{
		const scope::compile::compressor_t* comp = &compressor;
		int j = 0;
		int sz = path[s].atoms.size();
		power += sz;
		const scope::compile::compressor_t* current;
		while(j < sz && (current = comp->next(path[s].atoms[j]).get() ))
		{
			j++;
			comp = current;
			if(comp->match)
			{
				// calculate score
				double score = 0;
				for(size_t k = 0; k < j; ++k)
					score += 1 / pow(2, power - k);
				
				iterate(id, comp->simple) {		
   		 		double& rank = ruleToRank[*id];
					rank = std::max(rank, score);
				}
			}
		}

		for(int i = 0; i < comp->possible.size();i++)
			palette[i] |= comp->possible[i];
		populate.add(comp->hash, sz);
	}
}

std::map<int, double> scope::compile::matcher_t::match (scope::context_t const& scope, const scope::compile::compressor_t& compressor, const scope::compile::compressor_t& r_compressor) const
{
	palette.assign(blocks_needed, 0); // clear
	std::map<int, double> ruleToRank; // should this be a vector, and ignore zero values?
	bool needs_right = false;
	lookup(left, scope.left.path, compressor, palette, ruleToRank, needs_right);

	if(needs_right)
		lookup(right, scope.right.path, r_compressor, palette, ruleToRank, needs_right);

	for(size_t index = 0; index < palette.size(); index++) {
		while(int sub_rule_id = ffs(palette.at(index))) {
			int real_index = sub_rule_id - 1 + index*(sizeof(scope::compile::bits_t)*CHAR_BIT);
			double score;
			
			if(expressions[real_index].composite->does_match(left, right, &score)) {
				size_t r_id = expressions.at(real_index).rule_id;
				double& rank = ruleToRank[r_id];
				rank = std::max(rank, score);
			}
			palette[index] &= ~(1<<sub_rule_id-1);
		}
	}
	return ruleToRank;
}
