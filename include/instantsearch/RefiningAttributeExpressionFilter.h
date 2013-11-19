//$Id: NonSearchableAttributeExpressionFilter.h 3456 2013-07-10 02:11:13Z Jamshid $

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


#ifndef __CORE_POSTPROICESSING_REFININGATTRIBTEEXPRESSIONFILTER_H__
#define __CORE_POSTPROICESSING_REFININGATTRIBTEEXPRESSIONFILTER_H__

#include "instantsearch/ResultsPostProcessor.h"
#include "instantsearch/QueryEvaluator.h"
#include "instantsearch/TypedValue.h"

namespace srch2
{
namespace instantsearch
{
class RefiningAttributeExpressionFilterInternal;

class RefiningAttributeExpressionEvaluator
{
public:
	virtual bool evaluate(std::map<std::string, TypedValue> & refiningAttributeValues) = 0 ;
	virtual ~RefiningAttributeExpressionEvaluator(){};
};

class RefiningAttributeExpressionFilter : public ResultsPostProcessorFilter
{
public:
	RefiningAttributeExpressionFilter();
	void doFilter(QueryEvaluator *queryEvaluator, const Query * query,
			QueryResults * input, QueryResults * output);
	~RefiningAttributeExpressionFilter();
	// this object is allocated and de-allocated ourside this class.
	RefiningAttributeExpressionEvaluator * evaluator;

private:
	RefiningAttributeExpressionFilterInternal * impl;
};

}
}
#endif // __CORE_POSTPROICESSING_REFININGATTRIBTEEXPRESSIONFILTER_H__

