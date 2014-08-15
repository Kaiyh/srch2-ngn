package com.srch2.android.http.service;

import org.json.JSONArray;
import org.json.JSONObject;

/**
 * Represents an index in the SRCH2 search server. For every index to be searched on, users of the
 * SRCH2 Android SDK should implement a separate subclass instance of this class.
 * <br><br>
 * For each implementation of this class, <b>it is necessary</b> to override:
 * <br>
 * {@link #getIndexName()}
 * <br>
 * {@link #getSchema()}
 * <br><br>
 * This class contains methods for performing CRUD actions on the index such as insertion and
 * searching; in addition, specific <code>Indexable</code> instances can be obtained from the
 * <code>SRCH2Engine</code> static method <code>getIndex(String indexName)</code> (where <code>
 * indexName</code> matches the return value of <code>getIndexName())</code> to access these
 * same methods.
 */
public abstract class Indexable {

    IndexInternal indexInternal;

    /**
     * The default value for the numbers of search results to return per search request.
     * <br><br>
     * Has the <b>constant</b> value of <code>10</code>.
     */
    public static final int DEFAULT_NUMBER_OF_SEARCH_RESULTS_TO_RETURN_AKA_TOPK = 10;


    /**
     * The default value for the fuzziness similarity threshold. Approximately one character
     * per every three characters will be allowed to act as a wildcard during a search.
     * <br><br>
     * Has the <b>constant</b> value of <code>0.65</code>.
     */
    public static final float DEFAULT_FUZZINESS_SIMILARITY_THRESHOLD = 0.65f;

    /**
     * Implementing this method sets the name of the index this <code>Indexable</code> represents.
     * @return the name of the index this <code>Indexable</code> represents
     */
    abstract public String getIndexName();


    /**
     * Implementing this method sets the schema of the index this <code>Indexable</code> represents. The schema
     * defines the data fields of the index, much like the table structure of an SQLite database table. See
     * {@link com.srch2.android.http.service.Schema} for more information.
     * @return the schema to define the index structure this <code>Indexable</code> represents
     */
    abstract public Schema getSchema();

    /**
     * Override this method to set the number of search results to be returned per query or search task.
     * <br><br>
     * The default value of this method, if not overridden, is ten.
     * <br><br>
     * This method will cause an <code>IllegalArgumentException</code> to be thrown when calling
     * {@link com.srch2.android.http.service.SRCH2Engine#initialize(Indexable, Indexable...)} and passing this
     * <code>Indexable</code> if the returned value
     *  is less than one.
     * @return the number of results to return per search
     */
    public int getTopK() {
        return DEFAULT_NUMBER_OF_SEARCH_RESULTS_TO_RETURN_AKA_TOPK;
    }

    /**
     * Set the default fuzziness similarity threshold. This will determine how many character
     * substitutions the original search input will match search results for: if set to 0.5,
     * the search performed will include results as if half of the characters of the original
     * search input were replaced by wild card characters.
     * <br><br>
     * <b>Note:</b> In the the formation of a {@link com.srch2.android.http.service.Query}, each <code>Term</code> can
     * have its own fuzziness similarity threshold value set by calling the method
     * {@link com.srch2.android.http.service.SearchableTerm#enableFuzzyMatching(float)}; by default it is disabled for terms.
     * <br><br>
     * The default value of this method, if not overridden, is 0.65.
     * <br><br>
     * This method will cause an <code>IllegalArgumentException</code> to be thrown when calling
     * {@link com.srch2.android.http.service.SRCH2Engine#initialize(Indexable, Indexable...)} and passing this <code>Indexable</code> if the
     * value returned is less than zero or greater than one.
     * @return the fuzziness similarity ratio to match against search keywords against
     */
    public float getFuzzinessSimilarityThreshold() { return DEFAULT_FUZZINESS_SIMILARITY_THRESHOLD; }

    /**
     * Inserts the specified <code>JSONObject record</code> into the index that this
     * <code>Indexable</code> represents. This <code>JSONObject</code> should be properly
     * formed and its keys should only consist of the fields as defined in the index's
     * schema; the values of these keys should match the type defined in the index's
     * schema as well.
     * <br><br>
     * After the SRCH2 search server completes the insertion task, the callback method of the
     * {@link com.srch2.android.http.service.StateResponseListener#onInsertRequestComplete(String, InsertResponse)}
     * will be triggered containing the status of the
     * completed insertion task.
     * @param record the <code>JSONObject</code> representing the record to insert
     */
    public final void insert(JSONObject record) {
        if (indexInternal != null) {
            indexInternal.insert(record);
        }
    }

    /**
     * Inserts the set of records represented as <code>JSONObject</code>s contained in the
     * specified <code>JSONArray records</code>. Each <code>JSONObject</code> should be properly
     * formed and its keys should only consist of the fields as defined in the index's
     * schema; the values of these keys should match the type defined in the index's
     * schema as well. The <code>JSONArray records</code> should also be properly formed
     * and contain only the set of <code>JSONObect</code>s that represent the records to be
     * inserted.
     * <br><br>
     * After the SRCH2 search server completes the insertion task, the callback method of the
     * {@link com.srch2.android.http.service.StateResponseListener#onInsertRequestComplete(String, InsertResponse)}
     * will be triggered containing the status of the
     * completed insertion task.
     * @param records the <code>JSONArray</code> containing the set of <code>JSONObject</code>s
     *                representing the records to insert
     */
    public final void insert(JSONArray records) {
        if (indexInternal != null) {
            indexInternal.insert(records);
        }
    }

    /**
     * Updates the specified <code>JSONObject record</code> into the index that this
     * <code>Indexable</code> represents. This <code>JSONObject</code> should be properly
     * formed and its keys should only consist of the fields as defined in the index's
     * schema; the values of these keys should match the type defined in the index's
     * schema as well. If the primary key supplied in the <code>record</code> does not
     * match the primary key of any existing record, the <code>record</code> will be
     * inserted.
     * <br><br>
     * After the SRCH2 search server completes the update task, the callback method of the
     * {@link com.srch2.android.http.service.StateResponseListener#onUpdateRequestComplete(String, UpdateResponse)}
     * will be triggered containing the status of the
     * completed update task.
     * @param record the <code>JSONObject</code> representing the record to upsert
     */
    public final void update(JSONObject record) {
        if (indexInternal != null) {
            indexInternal.update(record);
        }

    }

    /**
     * Updates the set of records represented as <code>JSONObject</code>s contained in the
     * specified <code>JSONArray records</code>. Each <code>JSONObject</code> should be properly
     * formed and its keys should only consist of the fields as defined in the index's
     * schema; the values of these keys should match the type defined in the index's
     * schema as well. The <code>JSONArray records</code> should also be properly formed
     * and contain only the set of <code>JSONObect</code>s that represent the records to be
     * updates. If the primary key of any of the supplied records in the <code>JSONArray</code>
     * does not match the primary key of any existing record, that record will be
     * inserted.
     * <br><br>
     * After the SRCH2 search server completes the update task, the callback method of the
     * {@link com.srch2.android.http.service.StateResponseListener#onUpdateRequestComplete(String, UpdateResponse)}
     *  will be triggered containing the status of the
     * completed update task.
     * @param records the <code>JSONArray</code> containing the set of <code>JSONObject</code>s
     *                representing the records to upsert
     */
    public final void update(JSONArray records) {
        if (indexInternal != null) {
            indexInternal.update(records);
        }
    }

    /**
     * Deletes from the index this record with a primary
     * key matching the value of the <code>primaryKeyOfRecordToDelete</code>. If no record with
     * a matching primary key is found, the index will remain as it is.
     * <br><br>
     * After the SRCH2 search server completes the deletion task, the callback method of the
     * {@link com.srch2.android.http.service.StateResponseListener#onDeleteRequestComplete(String, DeleteResponse)}
     * will be triggered containing the status of the
     * completed deletion task.
     * @param primaryKeyOfRecordToDelete the primary key of the record to delete
     */
    public final void delete(String primaryKeyOfRecordToDelete) {
        if (indexInternal != null) {
            indexInternal.delete(primaryKeyOfRecordToDelete);
        }
    }



    /**
     * Does a basic search on the index that this <code>Indexable</code> represents. A basic
     * search means that all distinct keywords (delimited by white space) of the
     * <code>searchInput</code> are treated as fuzzy, and the last keyword will
     * be treated as fuzzy and prefix.
     * <br><br>
     * For more configurable searches, use the
     * {@link com.srch2.android.http.service.Query} class in conjunction with the {@link #advancedSearch(Query)}
     * method.
     * <br><br>
     * When the SRCH2 server is finished performing the search task, the method
     * {@link com.srch2.android.http.service.SearchResultsListener#onNewSearchResults(int, String, java.util.HashMap)}
     *  will be triggered. The
     * <code>HashMap</code> argument will contain the search results in the form of <code>
     * JSONObject</code>s as they were originally inserted (and updated).
     * <br><br>
     * This method will throw an exception is the value of <code>searchInput</code> is null
     * or has a length less than one.
     * @param searchInput the textual input to search on
     */
    public final void search(String searchInput) {
        if (indexInternal != null) {
            indexInternal.search(searchInput);
        }
    }

    /**
     * Does an advanced search by forming search request input as a {@link com.srch2.android.http.service.Query}. This enables
     * searches to use all the advanced features of the SRCH2 search engine, such as per term
     * fuzziness similarity thresholds, limiting the range of results to a refining field, and
     * much more. See the {@link com.srch2.android.http.service.Query} for more details.
     * <br><br>
     * When the SRCH2 server is finished performing the search task, the method
     * {@link com.srch2.android.http.service.SearchResultsListener#onNewSearchResults(int, String, java.util.HashMap)}
     * will be triggered. The argument
     * <code>HashMap</code> will contain the search results in the form of <code>
     * JSONObject</code>s as they were originally inserted (and updated).
     * <br><br>
     * This method will throw an exception if the value of <code>query</code> is null.
     * @param query the formation of the query to do the advanced search
     */
    public final void advancedSearch(Query query) {
        if (indexInternal != null) {
            indexInternal.advancedSearch(query);
        }
    }

    /**
     * Retrieves a record from the index this <code>Indexable</code> represents if the primary
     * key of any record in the index matches the value of <code>primaryKeyOfRecordToRetrieve</code>.
     * When the SRCH2 search server completes the retrieval task, the method
     * {@link com.srch2.android.http.service.StateResponseListener#onGetRecordByIDComplete(String, GetRecordResponse)}
     * will be triggered. The record retrieved will be in
     * the form of a <code>JSONObject</code> inside the <code>response GetRecordResponse</code>.
     * <br><br>
     * This method will than an <code>IllegalArgumentException</code>if the value of
     * <code>primaryKeyOfRecordToRetrieve</code> is null or has a length less than one.
     * @param primaryKeyOfRecordToRetrieve the primary key
     */
    public final void getRecordbyID(String primaryKeyOfRecordToRetrieve) {
        if (indexInternal != null) {
            indexInternal.getRecordbyID(primaryKeyOfRecordToRetrieve);
        }
    }

    public static final String INDEX_INFO_LAST_MERGE_TIME_NOT_SET = "0";
    public static final int INDEX_INFO_STATE_NOT_SET = -1;






    private int numberOfDocumentsInTheIndex = INDEX_INFO_STATE_NOT_SET;
    final void setRecordCount(int latestRecordCount) {
        numberOfDocumentsInTheIndex = latestRecordCount;
    }
    public final int getRecordCount() {
        return numberOfDocumentsInTheIndex;
    }

    private int numberOfSearchRequests = INDEX_INFO_STATE_NOT_SET;
    final void setSearchRequestCount(int latestSearchRequestCount) {
        latestSearchRequestCount = numberOfSearchRequests;
    }
    final void incrementSearchRequestCount() { ++numberOfSearchRequests; }
    public final int getSearchRequestCount() {
        return numberOfSearchRequests;
    }



    private int numberOfWriteRequests = INDEX_INFO_STATE_NOT_SET;
    final void setWriteRequestCount(int latestWriteRequestCount) {
        numberOfWriteRequests = latestWriteRequestCount;
    }
    public final int getWriteRequestCount() {
        return numberOfWriteRequests;
    }

    private String lastMergeTime = INDEX_INFO_LAST_MERGE_TIME_NOT_SET;
    final void setLastMergeTime(String latestMergeTime) {
        lastMergeTime = latestMergeTime;
    }
    public final String getLastMergeTime() {
        return lastMergeTime;
    }

    final void updateFromInfoResponse(InternalInfoResponse ir) {
        numberOfDocumentsInTheIndex = ir.numberOfDocumentsInTheIndex;
        numberOfSearchRequests = ir.numberOfSearchRequests;
        numberOfWriteRequests = ir.numberOfWriteRequests;
        lastMergeTime = ir.lastMergeTime;
    }
}
