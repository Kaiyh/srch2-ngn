//$Id: Lookup_Test.cpp 3480 2013-06-19 08:00:34Z jiaying $

#include <instantsearch/Analyzer.h>
#include <instantsearch/Indexer.h>
#include <instantsearch/Term.h>
#include <instantsearch/Record.h>

#include "IntegrationTestHelper.h"
#include "analyzer/StandardAnalyzer.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>

#include <time.h>

using namespace std;

namespace srch2is = srch2::instantsearch;
using namespace srch2is;

unsigned mergeEveryNSeconds = 10;
unsigned mergeEveryMWrites = 5;

// Read data from file, build the index, and save the index to disk
void buildIndex(string data_file, string index_dir)
{
    /// Set up the Schema
    Schema *schema = Schema::create(srch2is::DefaultIndex);
    schema->setPrimaryKey("primaryKey");
    schema->setSearchableAttribute("description", 2);
    schema->setScoringExpression("idf_score*doc_boost");

    /// Create an Analyzer
    AnalyzerInternal *analyzer = new StandardAnalyzer(srch2::instantsearch::NO_STEMMER_NORMALIZER, "");

    /// Create an index writer
    IndexMetaData *indexMetaData = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, index_dir, "");
    Indexer *indexer = Indexer::create(indexMetaData, analyzer, schema);

    Record *record = new Record(schema);

    unsigned docsCounter = 0;
    string line;

    ifstream data(data_file.c_str());

    string recordToLookup = "";

    /// Read records from file
    /// the file should have two fields, seperated by '^'
    /// the first field is the primary key, the second field is a searchable attribute
    while(getline(data,line))
    {
        unsigned cellCounter = 0;
        stringstream  lineStream(line);
        string cell;

        while(getline(lineStream,cell,'^') && cellCounter < 3 )
        {
            if (cellCounter == 0)
            {
                record->setPrimaryKey(cell.c_str());

                if (recordToLookup == "")
                    recordToLookup = cell;
            }
            else if (cellCounter == 1)
            {
                record->setSearchableAttributeValue(0, cell);
            }
            else
            {
                float recordBoost = atof(cell.c_str());
                record->setRecordBoost(recordBoost);
            }

            cellCounter++;
        }

        indexer->addRecord(record, 0);

        docsCounter++;

        record->clear();
    }

    cout << "#Docs Read:" << docsCounter << endl;

    indexer->commit();
    indexer->save();

    cout << "Index saved." << endl;

    // test looking up a record in the index 
    ASSERT(indexer->lookupRecord(recordToLookup) == LU_PRESENT_IN_READVIEW_AND_WRITEVIEW);

    data.close();

    delete indexer;
    delete indexMetaData;
    delete analyzer;
    delete schema;
}

// Read data from file, update the index
void updateIndexAndLookupRecord(string data_file, Indexer *index)
{
    /// Set up the Schema
    Schema *schema = Schema::create(srch2is::DefaultIndex);
    schema->setPrimaryKey("primaryKey");
    schema->setSearchableAttribute("description", 2);

    Record *record = new Record(schema);

    unsigned docsCounter = 0;
    string line;

    ifstream data(data_file.c_str());

    string recordToLookup;

    /// Read one record from file to insert
    /// the file should have two fields, seperated by '^'
    /// the first field is the primary key, the second field is a searchable attribute
    if ( getline(data,line) )
    {
        unsigned cellCounter = 0;
        stringstream  lineStream(line);
        string cell;

        while(getline(lineStream,cell,'^') && cellCounter < 3 )
        {
            if (cellCounter == 0)
            {
                record->setPrimaryKey(cell.c_str());

                recordToLookup = cell;

                // test looking up a record that doesn't exist
                ASSERT(index->lookupRecord(recordToLookup) == LU_ABSENT_OR_TO_BE_DELETED);
            }
            else if (cellCounter == 1)
            {
                record->setSearchableAttributeValue(0, cell);
            }
            else
            {
                float recordBoost = atof(cell.c_str());
                record->setRecordBoost(recordBoost);
            }

            cellCounter++;
        }

        index->addRecord(record, 0);

        // test looking up a record that was just inserted and NOT merged yet
        ASSERT(index->lookupRecord(recordToLookup) == LU_TO_BE_INSERTED);

        docsCounter++;

        record->clear();
    }

    cout << "#New Docs Inserted:" << docsCounter << endl;

    sleep(11);

    // test looking up a record that was merged
    ASSERT(index->lookupRecord(recordToLookup) == LU_PRESENT_IN_READVIEW_AND_WRITEVIEW);

    cout << "Index updated." << endl;

    data.close();

    delete schema;
}

int main(int argc, char **argv)
{
    cout << "Test begins." << endl;
    cout << "-------------------------------------------------------" << endl;

    string index_dir = getenv("index_dir");
    string init_data_file = index_dir + "/init_data";
    string update_data_file = index_dir + "/update_data";

    cout << "Read init data from " << init_data_file << endl;
    cout << "Save index to " << index_dir << endl;
    cout << "Read update data from " << update_data_file << endl;

    buildIndex(init_data_file, index_dir);

    IndexMetaData *indexMetaData = new IndexMetaData( new Cache(), mergeEveryNSeconds, mergeEveryMWrites, index_dir, "");
    Indexer *index = Indexer::load(indexMetaData);

    cout << "Index loaded." << endl;

    updateIndexAndLookupRecord(update_data_file, index);
    
    delete index;
    delete indexMetaData;

    return 0;
}