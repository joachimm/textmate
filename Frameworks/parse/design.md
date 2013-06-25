#The TextMate 2 parser
The parser applies the rules defined in the grammar files found in the Syntaxes folders in the bundles.
## Technical walkthrough
The parser parses line-by-line, it is supplied with a line + a stack of active rules from pervious lines.
###Entry point

    stack_ptr parse (char const* first, char const* last, stack_ptr stack, std::map<size_t, scope::scope_t>& scopes, bool firstLine, size_t i)

first and last are the start and endpoint of the underlying c char array.
stack is a stack of the active rules
scopes is an out value, mapping scope change to index in line.
   
#### Details
First the start scope is set up for index 0. This depends on if there are while patterns.