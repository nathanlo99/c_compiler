wlp: *.cpp
	g++ -Wall -Wextra -Werror -pedantic -std=c++2a ast_node.cpp parse_node.cpp wlp.cpp lexer.cpp parser.cpp -g -fsanitize=address,undefined -o wlp
