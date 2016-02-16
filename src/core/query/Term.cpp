/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// $Id: Term.cpp 3456 2013-06-14 02:11:13Z jiaying $

/*
 * The Software is made available solely for use according to the License Agreement. Any reproduction
 * or redistribution of the Software not in accordance with the License Agreement is expressly prohibited
 * by law, and may result in severe civil and criminal penalties. Violators will be prosecuted to the
 * maximum extent possible.
 *
 * THE SOFTWARE IS WARRANTED, IF AT ALL, ONLY ACCORDING TO THE TERMS OF THE LICENSE AGREEMENT. EXCEPT
 * AS WARRANTED IN THE LICENSE AGREEMENT, SRCH2 INC. HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS WITH
 * REGARD TO THE SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES AND CONDITIONS OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT.  IN NO EVENT SHALL SRCH2 INC. BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF SOFTWARE.

 * Copyright © 2010 SRCH2 Inc. All rights reserved
 */

#include <instantsearch/Term.h>

#include <map>
#include <vector>
#include <string>
#include <stdint.h>
#include "util/encoding.h"
#include <sstream>
#include "util/SerializationHelper.h"

using std::string;
using std::vector;
namespace srch2
{
namespace instantsearch
{

//TODO OPT pass string pointer in FuzzyTerm/ExactTerm constructor rather than copying. But copy terms into cache
struct Term::Impl
{

	Impl(const Term::Impl & impl){
	    this->boost = impl.boost;
	    this->similarityBoost = impl.similarityBoost;
	    this->threshold  = impl.threshold;
	    this->searchableAttributeIdsToFilter  = impl.searchableAttributeIdsToFilter;
	    this->attrOp = impl.attrOp;
	    this->type  = impl.type;
	    this->keyword  = impl.keyword;
	};
	Impl(){};
    float boost;
    float similarityBoost;
    uint8_t threshold;
    vector<unsigned> searchableAttributeIdsToFilter;
    ATTRIBUTES_OP attrOp;
    TermType type;
    string keyword;

    string toString(){
    	std::stringstream ss;
    	ss << keyword.c_str();
    	ss << type;
    	ss << boost;
    	ss << similarityBoost;
    	ss << (threshold+1) << "";
    	ss << attrOp;
    	for (unsigned i = 0; i < searchableAttributeIdsToFilter.size(); ++i)
    		ss << searchableAttributeIdsToFilter[i];
    	return ss.str();
    }

    /*
     * Serialization Scheme :
     * | boost | similarityBoost | threshold | searchableAttributeIdToFilter | type | keyword |
     */
    void * serializeForNetwork(void * buffer){
    	buffer = srch2::util::serializeFixedTypes(boost, buffer);
    	buffer = srch2::util::serializeFixedTypes(similarityBoost, buffer);
    	buffer = srch2::util::serializeFixedTypes(threshold, buffer);
    	buffer = srch2::util::serializeVectorOfFixedTypes(searchableAttributeIdsToFilter, buffer);
    	buffer = srch2::util::serializeFixedTypes((uint32_t)attrOp, buffer);
    	buffer = srch2::util::serializeFixedTypes((uint32_t)type, buffer);
    	buffer = srch2::util::serializeString(keyword, buffer);

    	return buffer;
    }
    void * deserializeForNetwork(void * buffer){
    	buffer = srch2::util::deserializeFixedTypes(buffer, boost );
    	buffer = srch2::util::deserializeFixedTypes(buffer,similarityBoost);
    	buffer = srch2::util::deserializeFixedTypes(buffer,threshold);
    	buffer = srch2::util::deserializeVectorOfFixedTypes(buffer, searchableAttributeIdsToFilter);
    	uint32_t intVar = 0;
    	buffer = srch2::util::deserializeFixedTypes(buffer,intVar);
    	attrOp = (ATTRIBUTES_OP)intVar;
    	buffer = srch2::util::deserializeFixedTypes(buffer,intVar);
    	type = (TermType)intVar;
    	buffer = srch2::util::deserializeString(buffer,keyword);

    	return buffer;
    }
    unsigned getNumberOfBytesForNetwork(){
    	unsigned numberOfBytes = 0;
    	numberOfBytes += srch2::util::getNumberOfBytesFixedTypes(boost);
    	numberOfBytes += srch2::util::getNumberOfBytesFixedTypes(similarityBoost);
    	numberOfBytes += srch2::util::getNumberOfBytesFixedTypes(threshold);
    	numberOfBytes += srch2::util::getNumberOfBytesVectorOfFixedTypes(searchableAttributeIdsToFilter);
    	numberOfBytes += srch2::util::getNumberOfBytesFixedTypes((uint32_t)attrOp);
    	numberOfBytes += srch2::util::getNumberOfBytesFixedTypes((uint32_t)type);
    	numberOfBytes += srch2::util::getNumberOfBytesString(keyword);

    	return numberOfBytes;
    }
};

Term::Term(const string &keywordStr, TermType type, const float boost, const float fuzzyMatchPenalty, const uint8_t threshold)
{
    impl = new Impl;
    impl->keyword = keywordStr;
    impl->type = type;
    impl->boost = boost;
    impl->similarityBoost = fuzzyMatchPenalty;
    impl->threshold = threshold;
    impl->attrOp = ATTRIBUTES_OP_OR;
}

Term::Term(const Term & term){
	impl = new Impl(*(term.impl));
}

Term::~Term()
{
    if (impl != NULL) {
        delete impl;
    }
}

float Term::getBoost() const
{
    return impl->boost;
}

void Term::setBoost(const float boost)
{
    if (boost >= 1 && boost <= 2)
    {
        impl->boost = boost;
    }
    else
    {
        impl->boost = 1;
    }
}

float Term::getSimilarityBoost() const
{
    return impl->similarityBoost;
}

void Term::setSimilarityBoost(const float similarityBoost)
{
    if (similarityBoost >= 0.0 && similarityBoost <= 1.0)
    {
        impl->similarityBoost = similarityBoost;
    }
    else
    {
        impl->similarityBoost = 0.5;
    }
}

void Term::setThreshold(const uint8_t threshold)
{
    switch (threshold)
    {
        case 0: case 1: case 2:    case 3:    case 4:    case 5:
            impl->threshold = threshold;
            break;
        default:
            impl->threshold = 0;
    };
}

uint8_t Term::getThreshold() const
{
    return impl->threshold;
}

string *Term::getKeyword()
{
    return &(impl->keyword);
}

string *Term::getKeyword() const
{
    return &(impl->keyword);
}

TermType Term::getTermType() const
{
    return impl->type;
}

void Term::addAttributesToFilter(const vector<unsigned>& searchableAttributeId, ATTRIBUTES_OP attrOp)
{
    this->impl->searchableAttributeIdsToFilter = searchableAttributeId;
    this->impl->attrOp = attrOp;
}

ATTRIBUTES_OP Term::getFilterAttrOperation() {
	return this->impl->attrOp;
}

vector<unsigned>& Term::getAttributesToFilter() const
{
    return this->impl->searchableAttributeIdsToFilter;
}

string Term::toString(){
	return this->impl->toString();
}

void * Term::serializeForNetwork(void * buffer){
	return impl->serializeForNetwork(buffer);
}
void * Term::deserializeForNetwork(Term & term, void * buffer){
	return term.impl->deserializeForNetwork(buffer);
}
unsigned Term::getNumberOfBytesForSerializationForNetwork(){
	return impl->getNumberOfBytesForNetwork();
}
////////////////////////

Term* ExactTerm::create(const string &keyword, TermType type, const float boost, const float similarityBoost)
{
    return new Term(keyword, type, boost, similarityBoost, 0);
}

Term* FuzzyTerm::create(const string &keyword, TermType type, const float boost, const float similarityBoost, const uint8_t threshold)
{
    return new Term(keyword, type, boost, similarityBoost, threshold);
}

}}

