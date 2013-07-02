/*
 * SimpleAnalyzer.cpp
 *
 *  Created on: 2013-5-18
 */

#include "SimpleAnalyzer.h"
#include "WhiteSpaceTokenizer.h"
#include "LowerCaseFilter.h"
#include "StemmerFilter.h"
#include "StopFilter.h"
#include "SynonymFilter.h"

namespace srch2 {
namespace instantsearch {

// create operator flow and link share pointer to the data
TokenOperator * SimpleAnalyzer::createOperatorFlow() {
	TokenOperator *tokenOperator = new WhiteSpaceTokenizer();
	tokenOperator = new LowerCaseFilter(tokenOperator);
	if (this->stemmerType == ENABLE_STEMMER_NORMALIZER) {
		tokenOperator = new StemmerFilter(tokenOperator);
	}
	if (this->stopWordFilePath.compare("") != 0) {
		tokenOperator = new StopFilter(tokenOperator, this->stopWordFilePath);
	}
	if (this->synonymFilePath.compare("") != 0) {
		tokenOperator = new SynonymFilter(tokenOperator, this->synonymFilePath);
	}
	this->sharedToken = tokenOperator->sharedToken;
	return tokenOperator;
}
SimpleAnalyzer::~SimpleAnalyzer() {
	// TODO Auto-generated destructor stub
}
}
}

