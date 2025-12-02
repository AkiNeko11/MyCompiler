#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdio.h>

#include "parser.hpp"
#include "lexer.cpp"

using namespace std;


class Parser
{
    public:
    Parser(fstream &file);
    void parse();
    private:
    fstream &file;
};

Parser::Parser(fstream &file):file(file)
{
}

void Parser::parse()
{
    Lexer lexer(file);
    lexer.start();
}
