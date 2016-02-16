#ifndef _URLPARSER_H_
#define _URLPARSER_H_

namespace srch2
{
namespace httpwrapper
{

class URLParser
{
public:
    static const char queryDelimiter;
    static const char filterDelimiter;
    static const char fieldsAndDelimiter;
    static const char fieldsOrDelimiter;

    static const char* const searchTypeParamName;
    static const char* const keywordsParamName;
    static const char* const termTypesParamName;
    static const char* const termBoostsParamName;
    static const char* const fuzzyQueryParamName;
    static const char* const similarityBoostsParamName;
    static const char* const resultsToRetrieveStartParamName;
    static const char* const resultsToRetrieveLimitParamName;
    static const char* const attributeToSortParamName;
    static const char* const orderParamName;
    static const char* const lengthBoostParamName;
    static const char* const jsonpCallBackName;

    static const char* const leftBottomLatitudeParamName;
    static const char* const leftBottomLongitudeParamName;
    static const char* const rightTopLatitudeParamName;
    static const char* const rightTopLongitudeParamName;

    static const char* const centerLatitudeParamName;
    static const char* const centerLongitudeParamName;
    static const char* const radiusParamName;
    static const char* const nameParamName;
    static const char* const logNameParamName ;
    static const char* const setParamName;
    static const char* const shutdownForceParamName;
    static const char* const shutdownSaveParamName;
};

}
}


#endif //_URLPARSER_H_
