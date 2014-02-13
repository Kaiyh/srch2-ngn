
#include "PhysicalOperators.h"

using namespace std;
namespace srch2 {
namespace instantsearch {

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// PhysicalPlan Random Access Verification Term Operator ////////////////////////////

RandomAccessVerificationNotOperator::RandomAccessVerificationNotOperator() {
}

RandomAccessVerificationNotOperator::~RandomAccessVerificationNotOperator(){
}
bool RandomAccessVerificationNotOperator::open(QueryEvaluatorInternal * queryEvaluator, PhysicalPlanExecutionParameters & params){
	// open all children
	ASSERT(this->getPhysicalPlanOptimizationNode()->getChildrenCount() == 1);
	this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->open(queryEvaluator , params);
	return true;
}
PhysicalPlanRecordItem * RandomAccessVerificationNotOperator::getNext(const PhysicalPlanExecutionParameters & params) {
	return NULL;
}
bool RandomAccessVerificationNotOperator::close(PhysicalPlanExecutionParameters & params){
	// close the children
	this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->close(params);
	return true;
}

string RandomAccessVerificationNotOperator::toString(){
	string result = "RandomAccessVerificationNotOperator" ;
	if(this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode() != NULL){
		result += this->getPhysicalPlanOptimizationNode()->getLogicalPlanNode()->toString();
	}
	return result;
}


bool RandomAccessVerificationNotOperator::verifyByRandomAccess(PhysicalPlanRandomAccessVerificationParameters & parameters) {

	bool resultFromChild = this->getPhysicalPlanOptimizationNode()->getChildAt(0)->getExecutableNode()->verifyByRandomAccess(parameters);
	if(resultFromChild == true){
		return false;
	}
	// we clear everything to make sure no junk propagates up
	parameters.attributeBitmaps.clear();
	parameters.positionIndexOffsets.clear();
	parameters.prefixEditDistances.clear();
	parameters.termRecordMatchingPrefixes.clear();
	parameters.runTimeTermRecordScore = 0;
	parameters.staticTermRecordScore = 0;
	return true;
}
// The cost of open of a child is considered only once in the cost computation
// of parent open function.
PhysicalPlanCost RandomAccessVerificationNotOptimizationOperator::getCostOfOpen(const PhysicalPlanExecutionParameters & params){
	PhysicalPlanCost resultCost ;
	resultCost = resultCost + 1;
	resultCost = resultCost + this->getChildAt(0)->getCostOfOpen(params);
	return resultCost;
}
// The cost of getNext of a child is multiplied by the estimated number of calls to this function
// when the cost of parent is being calculated.
PhysicalPlanCost RandomAccessVerificationNotOptimizationOperator::getCostOfGetNext(const PhysicalPlanExecutionParameters & params) {
	return PhysicalPlanCost(); // zero cost
}
// the cost of close of a child is only considered once since each node's close function is only called once.
PhysicalPlanCost RandomAccessVerificationNotOptimizationOperator::getCostOfClose(const PhysicalPlanExecutionParameters & params) {
	PhysicalPlanCost resultCost ;
	resultCost = resultCost + 1;
	resultCost = resultCost + this->getChildAt(0)->getCostOfClose(params);
	return resultCost;
}
PhysicalPlanCost RandomAccessVerificationNotOptimizationOperator::getCostOfVerifyByRandomAccess(const PhysicalPlanExecutionParameters & params){
	PhysicalPlanCost resultCost;
	resultCost = resultCost + 1; // O(1)
	resultCost = resultCost + this->getChildAt(0)->getCostOfVerifyByRandomAccess(params);
	return resultCost;
}
void RandomAccessVerificationNotOptimizationOperator::getOutputProperties(IteratorProperties & prop){
	// TODO
}
void RandomAccessVerificationNotOptimizationOperator::getRequiredInputProperties(IteratorProperties & prop){
	// TODO
}
PhysicalPlanNodeType RandomAccessVerificationNotOptimizationOperator::getType() {
	return PhysicalPlanNode_RandomAccessNot;
}
bool RandomAccessVerificationNotOptimizationOperator::validateChildren(){
	ASSERT(this->getChildrenCount() == 1);
	PhysicalPlanNodeType childType = this->getChildAt(0)->getType();
	switch (childType) {
		case PhysicalPlanNode_RandomAccessTerm:
		case PhysicalPlanNode_RandomAccessAnd:
		case PhysicalPlanNode_RandomAccessOr:
		case PhysicalPlanNode_RandomAccessNot:
			return true;
		default:
			return false;
	}
}

}
}
