//$Id: ConfigManager.h 2013-07-5 02:11:13Z iman $

#include "ConfigManager.h"

#include <algorithm>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include "util/DateAndTimeHandler.h"

using namespace std;
namespace po = boost::program_options;

namespace srch2 {
namespace httpwrapper {

const char * ignoreOption = "IGNORE";

ConfigManager::ConfigManager(std::string& configFile) {
    this->configFile = configFile;
}

void ConfigManager::loadConfigFile(){
    std::cout << "Reading config file: " << this->configFile 
              << std::endl;

    po::options_description config("Config");

    config.add_options()
    //("customer-name", po::value<string>(), "customer name") // REQUIRED
    ("write-api-type", po::value<bool>(), "write-api-type. Kafka or http write") // REQUIRED
    ("index-type", po::value<int>(), "index-type") // REQUIRED
    ("data-source-type", po::value<bool>(), "Data source type")

    ("kafka-consumer-topicname", po::value<string>(),"Kafka consumer topic name") // REQUIRED
    ("kafka-broker-hostname", po::value<string>(), "Hostname of Kafka broker") // REQUIRED
    ("kafka-broker-port", po::value<uint16_t>(), "Port of Kafka broker") // REQUIRED
    ("kafka-consumer-partitionid", po::value<uint32_t>(),"Kafka consumer partitionid") // REQUIRED
    ("kafka-consumer-read-buffer", po::value<uint32_t>(),"Kafka consumer socket read buffer") // REQUIRED
    ("kafka-ping-broker-every-n-seconds", po::value<uint32_t>(),"Kafka consumer ping every n seconds") // REQUIRED
    ("merge-every-n-seconds", po::value<unsigned>(), "merge-every-n-seconds") // REQUIRED
    ("merge-every-m-writes", po::value<unsigned>(), "merge-every-m-writes") // REQUIRED
    ("number-of-threads", po::value<int>(), "number-of-threads")

    ("listening-hostname", po::value<string>(), "port to listen") // REQUIRED
    ("listening-port", po::value<string>(), "port to listen") // REQUIRED
    ("doc-limit", po::value<uint32_t>(), "document limit") // REQUIRED
    ("memory-limit", po::value<uint64_t>(), "memory limit") //REQUIRED
    ("license-file", po::value<string>(),"File name with path to the srch2 license key file") // REQUIRED
    ("trie-bootstrap-dict-file", po::value<string>(),"bootstrap trie with initial keywords") // REQUIRED
    //("number-of-threads",  po::value<int>(), "number-of-threads")
    ("cache-size", po::value<unsigned>(), "cache size in bytes") // REQUIRED

    ("primary-key", po::value<string>(), "Primary key of data source") // REQUIRED
    ("is-primary-key-searchable", po::value<int>(), "If primary key searchable")

    ("attributes-search", po::value<string>(),"Attributes/fields in data for searching") // REQUIRED
    ("attributes-search-default", po::value<string>(),"Default values for searchable attributes")
    ("attributes-search-required", po::value<string>(),"Flag to indicate if searchable attributes are nullable")
    ("non-searchable-attributes", po::value<string>(),"Attributes/fields in data for sorting and range query")
    ("non-searchable-attributes-types", po::value<string>(),"Types of attributes/fields in data for sorting and range query")
    ("non-searchable-attributes-default", po::value<string>(),"Default values of attributes/fields in data for sorting and range query")
    ("non-searchable-attributes-required", po::value<string>(),"Flag to indicate if a non-searchable attribute is required.")

    // facet information
    ("facet-enable", po::value<int>(),
            "Attributes which are used to create facets.")("facet-attributes",
            po::value<string>(), "Attributes which are used to create facets.")(
            "facet-attribute-type", po::value<string>(),
            "Type of facet to be created on each attribute.") // 0 : simple, 1 : range
    ("facet-attribute-start", po::value<string>(),
            "The start value of range attributes.")("facet-attribute-end",
            po::value<string>(), "The start value of range attributes.")(
            "facet-attribute-gap", po::value<string>(),
            "The start value of range attributes.")

    ("attribute-record-boost", po::value<string>(), "record-boost")(
            "attribute-latitude", po::value<string>(),
            "record-attribute-latitude")("attribute-longitude",
            po::value<string>(), "record-attribute-longitude")(
            "record-score-expression", po::value<string>(),
            "record-score-expression")("attribute-boosts", po::value<string>(),
            "Attributes Boosts")("search-response-format", po::value<int>(),
            "The result formatting of json search response. 0 for rid,edit_dist,matching_prefix. 1 for rid,edit_dist,matching_prefix,mysql_record")(
            "attributes-to-return", po::value<string>(),
            "Attributes to return in the search response")(
            "search-response-JSON-format", po::value<int>(),
            "search-response-JSON-format")

    ("query-tokenizer-character", po::value<char>(),"Query Tokenizer character")
    ("allowed-record-special-characters",po::value<string>(), "Record Tokenizer characters")
    ("default-searcher-type", po::value<int>(), "Searcher-type")
    ("default-query-term-match-type", po::value<int>(),"Exact term or fuzzy term")
    ("default-query-term-type",po::value<int>(), "Query has complete terms or fuzzy terms")
    ("default-query-term-boost", po::value<int>(),"Default query term boost")
    ("default-query-term-similarity-boost",po::value<float>(), "Default query term similarity boost")(
    "default-query-term-length-boost", po::value<float>(),"Default query term length boost")
    ("prefix-match-penalty",po::value<float>(), "Penalty for prefix matching")
    ("support-attribute-based-search", po::value<int>(),"If support attribute based search")
    ("default-results-to-retrieve",po::value<int>(), "number of results to retrieve")
    ("default-attribute-to-sort", po::value<string>(),"attribute used to sort the results")
    ("default-order",po::value<int>(), "sort order")
    ("default-spatial-query-bounding-square-side-length",po::value<float>(), "Query has complete terms or fuzzy terms")

    //("listening-port", po::value<string>(), "HTTP indexDataContainer listening port")
    //("document-root", po::value<string>(), "HTTP indexDataContainer document root to put html files")
    ("data-file-path", po::value<string>(), "Path to the file") // REQUIRED if data-source-type is 0s
    ("index-dir-path", po::value<string>(), "Path to the index-dir") // DEPRECATED
    ("access-log-file", po::value<string>(),"HTTP indexDataContainer access log file") // DEPRECATED
    ("log-level", po::value<int>(), "srch2 log level")
    ("error-log-file",po::value<string>(), "HTTP indexDataContainer error log file") // DEPRECATED
    ("default-stemmer-flag", po::value<int>(), "Stemming or No Stemming")
    ("stop-filter-file-path", po::value<string>(),"Stop Filter file path or IGNORE")
    ("synonym-filter-file-path",po::value<string>(), "Synonym Filter file path or IGNORE")
    ("default-synonym-keep-origin-flag", po::value<int>(),"Synonym keep origin word or not")
    ("stemmer-file",po::value<string>(), "Stemmer File")
    ("install-directory",po::value<string>(), "Install Directory");

    fstream fs(configFile.c_str(), fstream::in);
     po::variables_map vm_config_file;
     po::store(po::parse_config_file(fs, config), vm_config_file);
     po::notify(vm_config_file);

     bool configSuccess = true;
     std::stringstream parseError;
     this->parse(vm_config_file, configSuccess, parseError);

     if (!configSuccess)
     {
         cout << "Error while reading the config file" << endl;
         cout << parseError.str() << endl;  // assumption: parseError is set properly
         exit(-1);
     }
}

void ConfigManager::kafkaOptionsParse(const po::variables_map &vm,
        bool &configSuccess, std::stringstream &parseError) {
    if (vm.count("kafka-consumer-topicname")
            && (vm["kafka-consumer-topicname"].as<string>().compare(
                    ignoreOption) != 0)) {
        kafkaConsumerTopicName = vm["kafka-consumer-topicname"].as<string>();
    } else {
        configSuccess = false;
        return;
    }

    if (vm.count("kafka-broker-hostname")) {
        kafkaBrokerHostName = vm["kafka-broker-hostname"].as<string>();
    } else {
        parseError << "kafka-broker-hostname is not set.\n";
    }

    if (vm.count("kafka-broker-port")) {
        kafkaBrokerPort = vm["kafka-broker-port"].as<uint16_t>();
    } else {
        parseError << "kafka-broker-port is not set.\n";
    }

    if (vm.count("kafka-consumer-partitionid")) {
        kafkaConsumerPartitionId =
                vm["kafka-consumer-partitionid"].as<uint32_t>();
    } else {
        parseError << "kafka-consumer-partitionid is not set.\n";
    }

    if (vm.count("kafka-consumer-read-buffer")) {
        writeReadBufferInBytes =
                vm["kafka-consumer-read-buffer"].as<uint32_t>();
    } else {
        parseError << "kafka-consumer-read-buffer is not set.\n";
    }

    if (vm.count("kafka-ping-broker-every-n-seconds")) {
        pingKafkaBrokerEveryNSeconds =
                vm["kafka-ping-broker-every-n-seconds"].as<uint32_t>();
    } else {
        parseError << "kafka-ping-broker-every-n-seconds is not set.\n";
    }
}

void ConfigManager::parse(const po::variables_map &vm, bool &configSuccess,
        std::stringstream &parseError) {
    if (vm.count("license-file")
            && (vm["license-file"].as<string>().compare(ignoreOption) != 0)) {
        licenseKeyFile = vm["license-file"].as<string>();
    } else {
        parseError << "License key is not set.\n";
        configSuccess = false;
        return;
    }

    if (vm.count("listening-hostname")) {
        httpServerListeningHostname = vm["listening-hostname"].as<string>();
    } else {
        configSuccess = false;
        return;
    }

    if (vm.count("listening-port")) {
        httpServerListeningPort = vm["listening-port"].as<string>();
    } else {
        configSuccess = false;
        return;
    }

    if (vm.count("doc-limit")) {
        documentLimit = vm["doc-limit"].as<uint32_t>();
    } else {
        configSuccess = false;
        return;
    }


	if (vm.count("memory-limit")) {
		memoryLimit = vm["memory-limit"].as<uint64_t>();
	} else {
		//configSuccess = false;
		//return;
	}

	if (vm.count("index-type")) {
		indexType = vm["index-type"].as<int>();

		switch (indexType)
		{
			case 0:
			{
				if (vm.count("default-searcher-type")) {
					searchType = vm["default-searcher-type"].as<int>();
				}else {
					//parseError << "SearchType is not set. Default to 0.\n";
					searchType = 0;
				}
			}
			break;
			case 1:
			{
				if (vm.count("attribute-latitude") && (vm["attribute-latitude"].as<string>().compare(ignoreOption) != 0)) {
					attributeLatitude =  vm["attribute-latitude"].as<string>();
				}
				else {
					parseError << "Attribute-latitude is not set.\n";
					configSuccess = false;
				}

				if (vm.count("attribute-longitude") && (vm["attribute-longitude"].as<string>().compare(ignoreOption) != 0)) {
					attributeLongitude =  vm["attribute-longitude"].as<string>();
				}else {
					parseError << "Attribute-longitude is not set.\n";
					configSuccess = false;
				}

				if (vm.count("default-spatial-query-bounding-square-side-length")) {
					defaultSpatialQueryBoundingBox =  vm["default-spatial-query-bounding-square-side-length"].as<float>();
				}else {
					defaultSpatialQueryBoundingBox = 0.2;
					parseError << "default-spatial-query-bounding-square-side-length is not set.\n";
				}
				searchType = 2;
			}
			break;
			default:
				parseError << "Index type is not set. Default to 0.\n";
				searchType = 0;
				//configSuccess = false;
				//return;
		}
	}else {
		parseError << "Index type is not set.\n";
		configSuccess = false;
		return;
	}

	if (vm.count("search-response-JSON-format")) {
		searchResponseJsonFormat = vm["search-response-JSON-format"].as<int>();
	}else {
		searchResponseJsonFormat=0;
	}

	if (vm.count("primary-key") && (vm["primary-key"].as<string>().compare(ignoreOption) != 0)) {
		primaryKey = vm["primary-key"].as<string>();
	}else {
		parseError << "primary-key is not set.\n";
		configSuccess = false;
		return;
	}

    vector<string> searchableAttributesVector;
    if (vm.count("attributes-search")
            && (vm["attributes-search"].as<string>().compare(ignoreOption) != 0)) {
        boost::split(searchableAttributesVector,
                vm["attributes-search"].as<string>(), boost::is_any_of(","));
    } else {
        parseError << "attributes-search is not set.\n";
        configSuccess = false;
        return;
    }

    vector<string> searchableAttributesDefaultVector(
            searchableAttributesVector.size());
    if (vm.count("attributes-search-default")
            && (vm["attributes-search-default"].as<string>().compare(
                    ignoreOption) != 0)) {
        boost::split(searchableAttributesDefaultVector,
                vm["attributes-search-default"].as<string>(),
                boost::is_any_of(","));
    } else {
        std::fill(searchableAttributesDefaultVector.begin(),
                searchableAttributesDefaultVector.end(), "");
        parseError << "OPTIONAL: Attributes Search Default is not set.\n";
    }

    vector<bool> searchableAttributesRequiredFlagVector(
            searchableAttributesVector.size());
    if (vm.count("attributes-search-required")
            && (vm["attributes-search-required"].as<string>().compare(
                    ignoreOption) != 0)) {
        vector<string> temp(searchableAttributesVector.size());
        boost::split(temp, vm["attributes-search-required"].as<string>(),
                boost::is_any_of(","));
        for (unsigned iter = 0; iter < temp.size(); iter++) {
            searchableAttributesRequiredFlagVector[iter] = atoi(
                    temp[iter].c_str());
        }
    } else {
        std::fill(searchableAttributesRequiredFlagVector.begin(),
                searchableAttributesRequiredFlagVector.end(), false); // in default nothing is required
        parseError << "OPTIONAL: Attributes Search required flag is not set.\n";
    }

    bool attrBoostParsed = false;
    if (vm.count("attribute-boosts")
            && (vm["attribute-boosts"].as<string>().compare(ignoreOption) != 0)) {
        vector<string> values;
        boost::split(values, vm["attribute-boosts"].as<string>(),
                boost::is_any_of(","));
        if (values.size() == searchableAttributesVector.size()) {
            for (unsigned iter = 0; iter < values.size(); iter++) {
                searchableAttributesInfo[searchableAttributesVector[iter]] =
                        pair<bool, pair<string, pair<unsigned, unsigned> > >(
                                searchableAttributesRequiredFlagVector[iter],
                                pair<string, pair<unsigned, unsigned> >(
                                        searchableAttributesDefaultVector[iter],
                                        pair<unsigned, unsigned>(0,
                                                (unsigned) atoi(
                                                        values[iter].c_str()))));

            }
            attrBoostParsed = true;
        } else {
            parseError
                    << "OPTIONAL: Number of attributes and attributeBoosts do not match.\n";
        }
    } else {
        parseError << "OPTIONAL: Attributes Boosts is not set.\n";
    }

    // give each searchable attribute an id based on the order in the info map
    // should be consistent with the id in the schema
    unsigned idIter = 0;
    map<string, pair<bool, pair<string, pair<unsigned, unsigned> > > >::iterator searchableAttributeIter =
            searchableAttributesInfo.begin();
    for (; searchableAttributeIter != searchableAttributesInfo.end();
            searchableAttributeIter++) {
        searchableAttributeIter->second.second.first = idIter;
        idIter++;
    }

    if (!attrBoostParsed) {
        for (unsigned iter = 0; iter < searchableAttributesVector.size();
                iter++) {
            searchableAttributesInfo[searchableAttributesVector[iter]] = pair<
                    bool, pair<string, pair<unsigned, unsigned> > >(
                    searchableAttributesRequiredFlagVector[iter],
                    pair<string, pair<unsigned, unsigned> >(
                            searchableAttributesDefaultVector[iter],
                            pair<unsigned, unsigned>(iter, 1)));

        }

        parseError << "All Attributes Boosts are set to 1.\n";
    }


    if (vm.count("non-searchable-attributes")
            && (vm["non-searchable-attributes"].as<string>().compare(
                    ignoreOption) != 0)) {
        vector<string> attributeNames;
        boost::split(attributeNames,
                vm["non-searchable-attributes"].as<string>(),
                boost::is_any_of(","));

        vector<bool> attributeIsRequired;
        if (vm.count("non-searchable-attributes-required")
                && (vm["non-searchable-attributes-required"].as<string>().compare(
                        ignoreOption) != 0)) {
            vector<string> attributeTypesTmp;
            boost::split(attributeTypesTmp,
                    vm["non-searchable-attributes-required"].as<string>(),
                    boost::is_any_of(","));
            for (unsigned iter = 0; iter < attributeTypesTmp.size(); iter++) {
                if (attributeTypesTmp[iter].compare("0") == 0) {
                    attributeIsRequired.push_back(false);
                } else if (attributeTypesTmp[iter].compare("1") == 0) {
                    attributeIsRequired.push_back(true);
                } else {
                    parseError
                            << "Non-searchable-attributes-required only accepts 0 and 1.\n";
                    configSuccess = false;
                    return;
                }
            }

            if (attributeIsRequired.size() != attributeNames.size()) {
                parseError
                        << "Non-searchable related options were not set correctly.\n";
                configSuccess = false;
                return;
            }

        } else { // default value is false
            attributeIsRequired.insert(attributeIsRequired.begin(),
                    attributeNames.size(), false);
        }

        if (vm.count("non-searchable-attributes-types")
                && (vm["non-searchable-attributes-types"].as<string>().compare(
                        ignoreOption) != 0)
                && vm.count("non-searchable-attributes-default")
                && (vm["non-searchable-attributes-default"].as<string>().compare(
                        ignoreOption) != 0)) {
            vector<srch2::instantsearch::FilterType> attributeTypes;
            vector<string> attributeDefaultValues;


            boost::split(attributeDefaultValues,
                    vm["non-searchable-attributes-default"].as<string>(),
                    boost::is_any_of(","));
            {
            	for(vector<string>::iterator defaultValue = attributeDefaultValues.begin();
            			defaultValue != attributeDefaultValues.end() ; ++defaultValue){
					if(srch2is::DateAndTimeHandler::verifyDateTimeString(*defaultValue , srch2is::DateTimeTypePointOfTime)){
						long timeValue = srch2is::DateAndTimeHandler::convertDateTimeStringToSecondsFromEpoch(*defaultValue);
						std::stringstream buffer;
						buffer << timeValue;
						*defaultValue = buffer.str();
					}
            	}
            }

            {
                vector<string> attributeTypesTmp;
                boost::split(attributeTypesTmp,
                        vm["non-searchable-attributes-types"].as<string>(),
                        boost::is_any_of(","));
                for (unsigned iter = 0; iter < attributeTypesTmp.size();
                        iter++) {
                    if (attributeTypesTmp[iter].compare("int") == 0)
                        attributeTypes.push_back(
                                srch2::instantsearch::ATTRIBUTE_TYPE_UNSIGNED);
                    else if (attributeTypesTmp[iter].compare("float") == 0)
                        attributeTypes.push_back(
                                srch2::instantsearch::ATTRIBUTE_TYPE_FLOAT);
                    else if (attributeTypesTmp[iter].compare("text") == 0)
                        attributeTypes.push_back(
                                srch2::instantsearch::ATTRIBUTE_TYPE_TEXT);
                    else if (attributeTypesTmp[iter].compare("time") == 0)
                        attributeTypes.push_back(
                                srch2::instantsearch::ATTRIBUTE_TYPE_TIME);
                }
            }


            if (attributeNames.size() == attributeTypes.size()
                    && attributeNames.size() == attributeDefaultValues.size()) {

                for (unsigned iter = 0; iter < attributeNames.size(); iter++) {

                    /// map<string, pair< srch2::instantsearch::FilterType, pair<string, bool> > >
                    nonSearchableAttributesInfo[attributeNames[iter]] =
                            pair<srch2::instantsearch::FilterType,
                                    pair<string, bool> >(attributeTypes[iter],
                                    pair<string, bool>(
                                            attributeDefaultValues[iter],
                                            attributeIsRequired[iter]));
                }
            } else {
                parseError
                        << "Non-searchable attribute related options were not set correctly.\n";
                configSuccess = false;
                return;
            }

        } else {
            parseError
                    << "Non-searchable attribute related options were not set correctly.\n";
            configSuccess = false;
            return;
        }
    }

    /*
     // facet information
     ("facet-enable", po::value<int>(), "Attributes which are used to create facets.")
     ("attribute-facet-type", po::value<int>(), "Type of facet to be created on each attribute.") // 0 : simple, 1 : range
     ("attribute-facet", po::value<string>(), "Attributes which are used to create facets.")
     ("attribute-facet-start", po::value<string>(), "The start value of range attributes.")
     ("attribute-facet-end", po::value<string>(), "The start value of range attributes.")
     ("attribute-facet-gap", po::value<string>(), "The start value of range attributes.")
     */

    if (vm.count("facet-enable")){
        facetEnabled = vm["facet-enable"].as<int>();
    } else {
        facetEnabled = false;
    }

    if (facetEnabled) {

        if (vm.count("facet-attributes")&& (vm["facet-attributes"].as<string>().compare(
                ignoreOption) != 0)) {
            boost::split(facetAttributes, vm["facet-attributes"].as<string>(),
                    boost::is_any_of(","));
            for(vector<string>::iterator facetAttribute = facetAttributes.begin() ;
                    facetAttribute != facetAttributes.end() ; ++facetAttribute){
                if(nonSearchableAttributesInfo.find(*facetAttribute) == nonSearchableAttributesInfo.end()){
                    parseError << "Facet attribute is not declared as a non-searchable attribute. Facet disabled.\n";
                    facetEnabled = false;
                }
            }
        } else {
            parseError << "Not enough information for facet. Facet disabled.\n";
            facetEnabled = false;
        }

        // now all the other options are required.
        if (vm.count("facet-attribute-type")&& (vm["facet-attribute-type"].as<string>().compare(
                ignoreOption) != 0)) {
            vector<string> facetTypesStringVector;
            boost::split(facetTypesStringVector,
                    vm["facet-attribute-type"].as<string>(),
                    boost::is_any_of(","));
            for (vector<string>::iterator type = facetTypesStringVector.begin();
                    type != facetTypesStringVector.end(); ++type) {
                int typeInteger = atoi(type->c_str());
                if (typeInteger != 0 && typeInteger != 1) {
                    typeInteger = 0; // zero means Categorical, 1 means Range
                    parseError << "Facet types should be either 0 or 1. Facet disabled.\n";
                }
                facetTypes.push_back(typeInteger);
            }
            if(facetTypes.size() != facetAttributes.size()){
                parseError << "facet-attribute-type should contain the same number of values as facet-attributes. Facet disabled.\n";
                facetEnabled = false;
            }
        } else {
            parseError << "Not enough information for facet. Facet disabled.\n";
            facetEnabled = false;
        }

        if (vm.count("facet-attribute-start")&& (vm["facet-attribute-start"].as<string>().compare(
                ignoreOption) != 0)) {
            boost::split(facetStarts, vm["facet-attribute-start"].as<string>(),
                    boost::is_any_of(","));
            if(facetStarts.size() != facetAttributes.size()){
                parseError << "facet-attribute-start should contain the same number of values as facet-attributes. Facet disabled.\n";
                facetEnabled = false;
            }

            // now use the date/time class to parse the values
            for(vector<string>::iterator startTextValue =facetStarts.begin() ;
            		startTextValue != facetStarts.end() ; ++startTextValue){
            	string attributeName = facetAttributes.at(std::distance(facetStarts.begin() , startTextValue));
            	srch2::instantsearch::FilterType attributeType ;
            	if(nonSearchableAttributesInfo.find(attributeName) != nonSearchableAttributesInfo.end()){
            		attributeType = nonSearchableAttributesInfo.find(attributeName)->second.first;
            	}else{
                    parseError << "Facet attribute is not declared as a non-searchable attribute. Facet disabled.\n";
                    facetEnabled = false;
            	}
            	if(attributeType == srch2is::ATTRIBUTE_TYPE_TIME){
					if(srch2is::DateAndTimeHandler::verifyDateTimeString(*startTextValue , srch2is::DateTimeTypePointOfTime)
							|| srch2is::DateAndTimeHandler::verifyDateTimeString(*startTextValue , srch2is::DateTimeTypeNow) ){
						long timeValue = srch2is::DateAndTimeHandler::convertDateTimeStringToSecondsFromEpoch(*startTextValue);
						std::stringstream buffer;
						buffer << timeValue;
						*startTextValue = buffer.str();
					}else{
	                    parseError << "Facet attribute start value is in wrong format.Facet disabled.\n";
	                    facetEnabled = false;
					}
            	}
            }
        } else {
            parseError << "Not enough information for facet. Facet disabled.\n";
            facetEnabled = false;
        }
        if (vm.count("facet-attribute-end")&& (vm["facet-attribute-end"].as<string>().compare(
                ignoreOption) != 0)) {
            boost::split(facetEnds, vm["facet-attribute-end"].as<string>(),
                    boost::is_any_of(","));
            // now use the date/time class to parse tha values
            for(vector<string>::iterator endTextValue =facetEnds.begin() ;
            		endTextValue != facetEnds.end() ; ++endTextValue){
            	string attributeName = facetAttributes.at(std::distance(facetEnds.begin() , endTextValue));
            	srch2::instantsearch::FilterType attributeType ;
            	if(nonSearchableAttributesInfo.find(attributeName) != nonSearchableAttributesInfo.end()){
            		attributeType = nonSearchableAttributesInfo.find(attributeName)->second.first;
            	}else{
                    parseError << "Facet attribute is not declared as a non-searchable attribute. Facet disabled.\n";
                    facetEnabled = false;
            	}
            	if(attributeType == srch2is::ATTRIBUTE_TYPE_TIME){
            		// If it's a time attribute, the input value must be compatible with our format.
    				if(srch2is::DateAndTimeHandler::verifyDateTimeString(*endTextValue , srch2is::DateTimeTypePointOfTime)
    						|| srch2is::DateAndTimeHandler::verifyDateTimeString(*endTextValue , srch2is::DateTimeTypeNow) ){
    					long timeValue = srch2is::DateAndTimeHandler::convertDateTimeStringToSecondsFromEpoch(*endTextValue);
            			std::stringstream buffer;
            			buffer << timeValue;
            			*endTextValue = buffer.str();
            		}else{
	                    parseError << "Facet attribute end value is in wrong format.Facet disabled.\n";
	                    facetEnabled = false;
					}
            	}

            }
            if(facetEnds.size() != facetAttributes.size()){
                parseError << "facet-attribute-end should contain the same number of values as facet-attributes. Facet disabled.\n";
                facetEnabled = false;
            }
        } else {
            parseError << "Not enough information for facet. Facet disabled.\n";
            facetEnabled = false;
        }
        if (vm.count("facet-attribute-gap")&& (vm["facet-attribute-gap"].as<string>().compare(
                ignoreOption) != 0)) {
            boost::split(facetGaps, vm["facet-attribute-gap"].as<string>(),
                    boost::is_any_of(","));
            // now use the date/time class to parse tha values
            for(vector<string>::iterator gapTextValue =facetGaps.begin() ;
            		gapTextValue != facetGaps.end() ; ++gapTextValue){
            	string attributeName = facetAttributes.at(std::distance(facetGaps.begin() , gapTextValue));
            	srch2::instantsearch::FilterType attributeType ;
            	if(nonSearchableAttributesInfo.find(attributeName) != nonSearchableAttributesInfo.end()){
            		attributeType = nonSearchableAttributesInfo.find(attributeName)->second.first;
            	}else{
                    parseError << "Facet attribute is not declared as a non-searchable attribute. Facet disabled.\n";
                    facetEnabled = false;
            	}
            	if(attributeType == srch2is::ATTRIBUTE_TYPE_TIME){
    				if(!srch2is::DateAndTimeHandler::verifyDateTimeString(*gapTextValue , srch2is::DateTimeTypeDurationOfTime) ){
	                    parseError << "Facet attribute end value is in wrong format.Facet disabled.\n";
	                    facetEnabled = false;
					}
            	}

            }
            if(facetGaps.size() != facetAttributes.size()){
                parseError << "facet-attribute-gap should contain the same number of values as facet-attributes. Facet disabled.\n";
                facetEnabled = false;
            }
        } else {
            parseError << "Not enough information for facet. Facet disabled.\n";
            facetEnabled = false;
        }
        if(facetEnabled){
            for(vector<int>::iterator facetType = facetTypes.begin() ;
                    facetType != facetTypes.end() ; ++facetType){
                if(*facetType == 0){ // Categorical
                    if(facetStarts.at(std::distance(facetTypes.begin() , facetType)).compare("-") != 0){
                        parseError << "Facet start value should be - for Categorical facets. Value changed.\n";
                        facetStarts.at(std::distance(facetTypes.begin() , facetType)) = "";
                    }
                    if(facetEnds.at(std::distance(facetTypes.begin() , facetType)).compare("-") != 0){
                        parseError << "Facet end value should be - for Categorical facets. Value changed.\n";
                        facetEnds.at(std::distance(facetTypes.begin() , facetType)) = "";
                    }
                    if(facetGaps.at(std::distance(facetTypes.begin() , facetType)).compare("-") != 0){
                        parseError << "Facet gap value should be - for Categorical facets. Value changed.\n";
                        facetGaps.at(std::distance(facetTypes.begin() , facetType)) = "";
                    }
                }else{
                    if(facetStarts.at(std::distance(facetTypes.begin() , facetType)).compare("-") == 0){
                        parseError << "Facet start value should not be - for Categorical facets. Facet disabled.\n";
                        facetEnabled = false;
                    }
                    if(facetEnds.at(std::distance(facetTypes.begin() , facetType)).compare("-") == 0){
                        parseError << "Facet end value should not be - for Categorical facets. Facet disabled.\n";
                        facetEnabled = false;
                    }
                    if(facetGaps.at(std::distance(facetTypes.begin() , facetType)).compare("-") == 0){
                        parseError << "Facet gap value should not be - for Categorical facets. Facet disabled.\n";
                        facetEnabled = false;
                    }
                }
            }
        }

    }

    recordBoostAttributeSet = false;
    if (vm.count("attribute-record-boost")
            && (vm["attribute-record-boost"].as<string>().compare(ignoreOption)
                    != 0)) {
        recordBoostAttributeSet = true;
        attributeRecordBoost = vm["attribute-record-boost"].as<string>();
    } else {
        //std::cout << "attribute-record-boost and topk-ranking-function were not set correctly.\n";
    }

    if (vm.count("record-score-expression")) {
        scoringExpressionString = vm["record-score-expression"].as<string>();
    } else {
        //std::cout << "record-score-expression was not set correctly.\n";
        scoringExpressionString = "1";
    }

    if (vm.count("search-response-format")) {
        searchResponseFormat = (ResponseType)vm["search-response-format"].as<int>();
    } else {
        searchResponseFormat = RESPONSE_WITH_RECORD; //in-memory
    }

    if (searchResponseFormat == 2 && vm.count("attributes-to-return")
            && (vm["attributes-to-return"].as<string>().compare(ignoreOption)
                    != 0)) {
        boost::split(attributesToReturn,
                vm["attributes-to-return"].as<string>(), boost::is_any_of(","));
    }

    if (vm.count("data-source-type")) {
        dataSourceType =
                vm["data-source-type"].as<bool>() == 1 ?
                        FILEBOOTSTRAP_TRUE : FILEBOOTSTRAP_FALSE;
        if (dataSourceType == FILEBOOTSTRAP_TRUE) {
            if (vm.count("data-file-path")) {
                filePath = vm["data-file-path"].as<string>();
            } else {
                parseError << "Path of data file is not set.\n";
                configSuccess = false;
                dataSourceType = FILEBOOTSTRAP_FALSE;
            }
        }
    } else {
        dataSourceType = FILEBOOTSTRAP_FALSE;
    }

    if (vm.count("write-api-type")) {
        writeApiType =
                vm["write-api-type"].as<bool>() == 0 ?
                        HTTPWRITEAPI : KAFKAWRITEAPI;
        switch (writeApiType) {
        case KAFKAWRITEAPI:
            this->kafkaOptionsParse(vm, configSuccess, parseError);
            break;
        case HTTPWRITEAPI:
            break;
        }
    } else {
        parseError << "WriteAPI type Set. Default to HTTPWRITEAPI.\n";
        writeApiType = HTTPWRITEAPI;
    }

    if (vm.count("trie-bootstrap-dict-file")) {
        trieBootstrapDictFile = vm["trie-bootstrap-dict-file"].as<string>();
    } else {
        trieBootstrapDictFile = std::string("");
    }

    if (vm.count("index-dir-path")) {
        indexPath = vm["index-dir-path"].as<string>();
    } else {
        parseError << "Path of index files is not set.\n";
        configSuccess = false;
    }
	if(vm.count("log-level")){
		loglevel = static_cast<Logger::LogLevel> (vm["log-level"].as<int>());
	}else{
		//default level is INFO
		loglevel = srch2::util::Logger::SRCH2_LOG_INFO;
	}

    if (vm.count("access-log-file")) {
        httpServerAccessLogFile = vm["access-log-file"].as<string>();
    } else {
        parseError << "httpServerAccessLogFile is not set.\n";
    }


    if (vm.count("error-log-file")) {
        httpServerErrorLogFile = vm["error-log-file"].as<string>();
    } else {
        parseError << "httpServerErrorLogFile is not set.\n";
    }

    if (vm.count("allowed-record-special-characters")) {
        allowedRecordTokenizerCharacters =
                vm["allowed-record-special-characters"].as<string>();
        if (allowedRecordTokenizerCharacters.empty())
            std::cerr
                    << "[Warning] There are no characters in the value allowedRecordTokenizerCharacters. To set it properly, those characters should be included in double quotes such as \"@'\""
                    << std::endl;
    } else {
        allowedRecordTokenizerCharacters = string("");
        //parseError << "allowed-record-special-characters is not set.\n";
    }

    if (vm.count("is-primary-key-searchable")) {
        isPrimSearchable = vm["is-primary-key-searchable"].as<int>();
    } else {
        isPrimSearchable = 0;
        //parseError << "IfPrimSearchable is not set.\n";
    }

    if (vm.count("default-query-term-match-type")) {
        exactFuzzy = (bool) vm["default-query-term-match-type"].as<int>();
    } else {
        exactFuzzy = 0;
        //parseError << "default-fuzzy-query-term-type is not set.\n";
    }

    if (vm.count("default-stemmer-flag")) {
        int temp = vm["default-stemmer-flag"].as<int>();
        if (temp == 1) {
            stemmerFlag = true;
        } else {
            stemmerFlag = false;
            if (temp != 0) {
                cerr << "default-stemmer-flag only accepts '0' or '1'" << endl
                        << "   '0' for disabling the stemmer" << endl
                        << "   '1' for enabling the stemmer" << endl
                        << "   Now it is disabled by default" << endl;
            }
        }
    } else {
        stemmerFlag = false;
    }

    if (vm.count("stemmer-file")) {
        stemmerFile = vm["stemmer-file"].as<string>();
    }

    if (vm.count("install-directory")) {
        installDir = vm["install-directory"].as<string>();
    }

    if (vm.count("stop-filter-file-path")
            && (vm["stop-filter-file-path"].as<string>().compare(ignoreOption)
                    != 0)) {
        stopFilterFilePath = vm["stop-filter-file-path"].as<string>();
    } else {
        stopFilterFilePath = "";
    }

    if (vm.count("synonym-filter-file-path")
            && (vm["synonym-filter-file-path"].as<string>().compare(
                    ignoreOption) != 0)) {
        synonymFilterFilePath = vm["synonym-filter-file-path"].as<string>();
    } else {
        synonymFilterFilePath = "";
    }

    if (vm.count("default-synonym-keep-origin-flag")) {
        int temp = vm["default-synonym-keep-origin-flag"].as<int>();
        if (temp == 0) {
            synonymKeepOrigFlag = false;
        } else {
            synonymKeepOrigFlag = true;
            if (temp != 1) {
                cerr
                        << "default-synonym-keep-origin-flag only accepts '0' or '1'"
                        << endl << "   '0' for not keeping the original word"
                        << endl << "   '1' for keeping the original word too"
                        << endl
                        << "   Now the original words will be kept by default"
                        << endl;
            }
        }
    } else {
        synonymKeepOrigFlag = true;
    }

    if (vm.count("default-query-term-type")) {
        queryTermType = (bool) vm["default-query-term-type"].as<int>();
    } else {
        queryTermType = 0;
        //parseError << "default-query-term-type is not set.\n";
    }

    if (vm.count("default-query-term-boost")) {
        queryTermBoost = vm["default-query-term-boost"].as<int>();
    } else {
        queryTermBoost = 1;
        //parseError << "Default query term boost.\n";
    }

    if (vm.count("default-query-term-similarity-boost")) {
        queryTermSimilarityBoost = vm["default-query-term-similarity-boost"].as<
                float>();
    } else {
        queryTermSimilarityBoost = 0.5;
        //parseError << "Default query term similarity boost.\n";
    }

    if (vm.count("default-query-term-length-boost")) {
        queryTermLengthBoost =
                vm["default-query-term-length-boost"].as<float>();
        if (!(queryTermLengthBoost > 0.0 && queryTermLengthBoost < 1.0)) {
            queryTermLengthBoost = 0.5;
        }
    } else {
        queryTermLengthBoost = 0.5;
        //parseError << "Default query term length boost of 0.5 set.\n";
    }

    if (vm.count("prefix-match-penalty")) {
        prefixMatchPenalty = vm["prefix-match-penalty"].as<float>();
        if (!(prefixMatchPenalty > 0.0 && prefixMatchPenalty < 1.0)) {
            prefixMatchPenalty = 0.95;
        }
    } else {
        prefixMatchPenalty = 0.95;
    }

    if (vm.count("support-attribute-based-search")) {
        supportAttributeBasedSearch =
                (bool) vm["support-attribute-based-search"].as<int>();
    } else {
        supportAttributeBasedSearch = false;
        //parseError << "\n";
    }

    if (supportAttributeBasedSearch && searchableAttributesInfo.size() > 31) {
        parseError
                << "To support attribute-based search, the number of searchable attributes cannot be bigger than 31.\n";
        configSuccess = false;
        return;
    }

    if (vm.count("default-results-to-retrieve")) {
        resultsToRetrieve = vm["default-results-to-retrieve"].as<int>();
    } else {
        resultsToRetrieve = 10;
        //parseError << "resultsToRetrieve is not set.\n";
    }

//	if (vm.count("default-attribute-to-sort")) {
//		attributeToSort = vm["default-attribute-to-sort"].as<string>();
//		if(! nonSearchableAttributesInfo[attributeToSort].second.second){
//	        parseError << "Default attribute to sort is not specified as sortable in non-searchable-attributes-sort.\n";
//	        configSuccess = false;
//	        return;
//		}
//	}else {
//		if (isDefaultSortAttributeNeeded){
//	        parseError << "Some attributes are sortable but no default sortable attribute is specified.\n";
//	        configSuccess = false;
//	        return;
//		}
//
//		attributeToSort = "";
//		//parseError << "attributeToSort is not set.\n";
//	}
    if (vm.count("default-attribute-to-sort")
            && (vm["default-attribute-to-sort"].as<string>().compare(
                    ignoreOption) != 0)) {

        string attributeToSortTmp =
                vm["default-attribute-to-sort"].as<string>();
        attributeToSort = -1;
        int i = 0;
        for (map<string,
                pair<srch2::instantsearch::FilterType, pair<string, bool> > >::const_iterator iter =
                nonSearchableAttributesInfo.begin();
                iter != nonSearchableAttributesInfo.end(); ++iter) {
            if (attributeToSortTmp.compare(iter->first) == 0
                    && iter->second.second.second) {
                attributeToSort = i;
            }
            i++;
        }
        if (attributeToSort == -1) {
            parseError
                    << "Default attribute to be used for sort is not set correctly.\n";
            configSuccess = false;
            return;
        }
        // 	map<string, pair< srch2::instantsearch::FilterType, pair<string, bool> > > nonSearchableAttributesInfo;
    } else {

//        if (isDefaultSortAttributeNeeded) {
//            parseError
//                    << "Default attribute to be used for sort is not specified.\n";
//            configSuccess = false;
//            return;
//        }
        attributeToSort = 0;
    }

    if (vm.count("default-order")) {
        ordering = vm["default-order"].as<int>();
    } else {
        ordering = 0;
        //parseError << "ordering is not set.\n";
    }

    if (vm.count("merge-every-n-seconds")) {
        mergeEveryNSeconds = vm["merge-every-n-seconds"].as<unsigned>();
        if (mergeEveryNSeconds < 10)
            mergeEveryNSeconds = 10;
        std::cout << "Merge every " << mergeEveryNSeconds << " second(s)"
                << std::endl;

    } else {
        parseError << "merge-every-n-seconds is not set.\n";
        configSuccess = false;
    }

    if (vm.count("merge-every-m-writes")) {
        mergeEveryMWrites = vm["merge-every-m-writes"].as<unsigned>();
        if (mergeEveryMWrites < 10)
            mergeEveryMWrites = 10;
    } else {
        parseError << "merge-every-m-writes is not set.\n";
    }

    if (vm.count("number-of-threads")) {
        numberOfThreads = vm["number-of-threads"].as<int>();
    } else {
        numberOfThreads = 1;
        parseError << "number-of-threads is not specified.\n";
    }

    unsigned minCacheSize = 50 * 1048576; // 50MB
    unsigned maxCacheSize = 500 * 1048576; // 500MB
    unsigned defaultCacheSize = 50 * 1048576; // 50MB
    if (vm.count("cache-size")) {
        cacheSizeInBytes = vm["cache-size"].as<unsigned>();
        if (cacheSizeInBytes < minCacheSize || cacheSizeInBytes > maxCacheSize)
            cacheSizeInBytes = defaultCacheSize;
    } else {
        cacheSizeInBytes = defaultCacheSize;
    }

    if (not (this->writeReadBufferInBytes > 4194304
            && this->writeReadBufferInBytes < 65536000)) {
        this->writeReadBufferInBytes = 4194304;
    }
}

ConfigManager::~ConfigManager() {

}

const std::string& ConfigManager::getCustomerName() const {
	return kafkaConsumerTopicName;
}

uint32_t ConfigManager::getDocumentLimit() const {
	return documentLimit;
}

uint64_t ConfigManager::getMemoryLimit() const {
	return memoryLimit;
}

uint32_t ConfigManager::getMergeEveryNSeconds() const {
	return mergeEveryNSeconds;
}

uint32_t ConfigManager::getMergeEveryMWrites() const {
	return mergeEveryMWrites;
}

int ConfigManager::getIndexType() const {
	return indexType;
}

const string& ConfigManager::getAttributeLatitude() const {
	return attributeLatitude;
}

const string& ConfigManager::getAttributeLongitude() const {
	return attributeLongitude;
}

float ConfigManager::getDefaultSpatialQueryBoundingBox() const {
	return defaultSpatialQueryBoundingBox;
}

int ConfigManager::getNumberOfThreads() const {
	return numberOfThreads;
}

DataSourceType ConfigManager::getDataSourceType() const {
	return dataSourceType;
}

WriteApiType ConfigManager::getWriteApiType() const {
	return writeApiType;
}

const string& ConfigManager::getIndexPath() const {
	return indexPath;
}

const string& ConfigManager::getFilePath() const {
	return this->filePath;
}

const string& ConfigManager::getPrimaryKey() const {
	return primaryKey;
}
const map<string, pair<bool, pair<string, pair<unsigned, unsigned> > > > * ConfigManager::getSearchableAttributes() const {
    return &searchableAttributesInfo;
}

const map<string, pair<srch2::instantsearch::FilterType, pair<string, bool> > > * ConfigManager::getNonSearchableAttributes() const {
    return &nonSearchableAttributesInfo;
}

const vector<string> * ConfigManager::getAttributesToReturnName() const {
    return &attributesToReturn;
}

bool ConfigManager::isFacetEnabled() const {
    return facetEnabled;
}
const vector<string> * ConfigManager::getFacetAttributes() const {
    return &facetAttributes;
}
const vector<int> * ConfigManager::getFacetTypes() const {
    return &facetTypes;
}
const vector<string> * ConfigManager::getFacetStarts() const {
    return &facetStarts;
}
const vector<string> * ConfigManager::getFacetEnds() const {
    return &facetEnds;
}

const vector<string> * ConfigManager::getFacetGaps() const {
    return &facetGaps;
}

/*const vector<unsigned> * ConfigManager::getAttributesBoosts() const
 {
 return &attributesBoosts;
 }*/

string ConfigManager::getInstallDir() const {
	return installDir;
}

bool ConfigManager::getStemmerFlag() const {
	return stemmerFlag;
}

string ConfigManager::getStemmerFile() const {
	return stemmerFile;
}

string ConfigManager::getSynonymFilePath() const {
	return synonymFilterFilePath;
}

bool ConfigManager::getSynonymKeepOrigFlag() const {
	return synonymKeepOrigFlag;
}

string ConfigManager::getStopFilePath() const {
	return stopFilterFilePath;
}

const string& ConfigManager::getAttributeRecordBoostName() const {
	return attributeRecordBoost;
    }

/*string getDefaultAttributeRecordBoost() const
 {
 return defaultAttributeRecordBoost;
 }*/

const std::string& ConfigManager::getScoringExpressionString() const {
	return scoringExpressionString;
}

int ConfigManager::getSearchResponseJSONFormat() const {
	return searchResponseJsonFormat;
}

const string& ConfigManager::getRecordAllowedSpecialCharacters() const {
	return allowedRecordTokenizerCharacters;
}

int ConfigManager::getSearchType() const {
	return searchType;
}

int ConfigManager::getIsPrimSearchable() const {
	return isPrimSearchable;
}

bool ConfigManager::getIsFuzzyTermsQuery() const {
	return exactFuzzy;
}

bool ConfigManager::getQueryTermType() const {
	return queryTermType;
}

unsigned ConfigManager::getQueryTermBoost() const {
	return queryTermBoost;
}

float ConfigManager::getQueryTermSimilarityBoost() const {
	return queryTermSimilarityBoost;
}

float ConfigManager::getQueryTermLengthBoost() const {
	return queryTermLengthBoost;
}

float ConfigManager::getPrefixMatchPenalty() const {
	return prefixMatchPenalty;
}

bool ConfigManager::getSupportAttributeBasedSearch() const {
	return supportAttributeBasedSearch;
}

ResponseType ConfigManager::getSearchResponseFormat() const {
	return searchResponseFormat;
}

const string& ConfigManager::getAttributeStringForMySQLQuery() const {
	return attributeStringForMySQLQuery;
}

const string& ConfigManager::getLicenseKeyFileName() const {
	return licenseKeyFile;
}

const std::string& ConfigManager::getTrieBootstrapDictFileName() const {
	return this->trieBootstrapDictFile;
}

const string& ConfigManager::getHTTPServerListeningHostname() const {
	return httpServerListeningHostname;
}

const string& ConfigManager::getHTTPServerListeningPort() const {
	return httpServerListeningPort;
}

const string& ConfigManager::getKafkaBrokerHostName() const {
	return kafkaBrokerHostName;
}

uint16_t ConfigManager::getKafkaBrokerPort() const {
	return kafkaBrokerPort;
}

const string& ConfigManager::getKafkaConsumerTopicName() const {
	return kafkaConsumerTopicName;
}

uint32_t ConfigManager::getKafkaConsumerPartitionId() const {
	return kafkaConsumerPartitionId;
}

uint32_t ConfigManager::getWriteReadBufferInBytes() const {
	return writeReadBufferInBytes;
}

uint32_t ConfigManager::getPingKafkaBrokerEveryNSeconds() const {
	return pingKafkaBrokerEveryNSeconds;
}

int ConfigManager::getDefaultResultsToRetrieve() const {
	return resultsToRetrieve;
}

int ConfigManager::getAttributeToSort() const {
	return attributeToSort;
}

int ConfigManager::getOrdering() const {
	return ordering;
}

bool ConfigManager::isRecordBoostAttributeSet() const {
	return recordBoostAttributeSet;
}

const string& ConfigManager::getHTTPServerAccessLogFile() const {
	return httpServerAccessLogFile;
}

const Logger::LogLevel& ConfigManager::getHTTPServerLogLevel() const
{
	return loglevel;
}

const string& ConfigManager::getHTTPServerErrorLogFile() const {
	return httpServerErrorLogFile;
}

unsigned ConfigManager::getCacheSizeInBytes() const {
	return cacheSizeInBytes;
}

}
}