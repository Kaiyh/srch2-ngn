#include "analyzer/Normalizer.h"
#include <string.h>
#include <stdio.h>
#include <iostream>
#include "util/Assert.h"

using namespace std;
using namespace srch2::instantsearch;

void testNormalizer()

{
	string FILE_DIR = getenv("file_dir");
	Normalizer *normalizer_handler = new Normalizer(srch2::instantsearch::ONLY_NORMALIZER, FILE_DIR);
	vector<string> tokens;
	string token = "wal";
	tokens.push_back(token);
	token = "mart";
	tokens.push_back(token);
	token = "wal mart";
	tokens.push_back(token);
	token = "cheese";
	tokens.push_back(token);
	token = "factory";
	tokens.push_back(token);
	token = "star";
	tokens.push_back(token);
	token = "bucks";
	tokens.push_back(token);

    normalizer_handler->normalize(tokens);

    assert(tokens.size()==11);

}


int main(int argc, char *argv[])
{


	testNormalizer();
	cout<<"\n\nNormalizer unit tests passed!!";

	return 0;

}
