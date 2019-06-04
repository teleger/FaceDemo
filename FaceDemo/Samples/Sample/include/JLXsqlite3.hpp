#ifndef _JLXSQLITE3_HPP
#define _JLXSQLITE3_HPP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "sqlite3.h"
#include "JLXDefine.hpp"

#ifdef ANDROID
#include <android/log.h>
#include <logging.h>

#define  JLX_SQLITE_LOG    "JLX_SQlite_JNI"

#define  SQLITE_ALOG(...)  __android_log_print(ANDROID_LOG_INFO,JLX_SQLITE_LOG,__VA_ARGS__)
#define  SQLITE_ELOG(...)  __android_log_print(ANDROID_LOG_ERROR,JLX_SQLITE_LOG,__VA_ARGS__)

#endif


namespace MySQLite
{

/**
 * @brief Encapsulation of the error message from SQLite3, based on std::runtime_error.
 */
    class Exception : public std::runtime_error
    {
    public:
        /**
         * @brief Encapsulation of the error message from SQLite3, based on std::runtime_error.
         *
         * @param[in] aErrorMessage The string message describing the SQLite error
         */
        explicit Exception(const char* aErrorMessage);
        explicit Exception(const std::string& aErrorMessage);

        /**
         * @brief Encapsulation of the error message from SQLite3, based on std::runtime_error.
         *
         * @param[in] aErrorMessage The string message describing the SQLite error
         * @param[in] ret           Return value from function call that failed.
         */
        Exception(const char* aErrorMessage, int ret);
        Exception(const std::string& aErrorMessage, int ret);

        /**
          * @brief Encapsulation of the error message from SQLite3, based on std::runtime_error.
          *
          * @param[in] apSQLite The SQLite object, to obtain detailed error messages from.
          */
        explicit Exception(sqlite3* apSQLite);

        /**
         * @brief Encapsulation of the error message from SQLite3, based on std::runtime_error.
         *
         * @param[in] apSQLite  The SQLite object, to obtain detailed error messages from.
         * @param[in] ret       Return value from function call that failed.
         */
        Exception(sqlite3* apSQLite, int ret);

        /// Return the result code (if any, otherwise -1).
        inline int getErrorCode() const noexcept // nothrow
        {
            return mErrcode;
        }

        /// Return the extended numeric result code (if any, otherwise -1).
        inline int getExtendedErrorCode() const noexcept // nothrow
        {
            return mExtendedErrcode;
        }

        /// Return a string, solely based on the error code
        const char* getErrorStr() const noexcept; // nothrow

    private:
        int mErrcode;         ///< Error code value
        int mExtendedErrcode; ///< Detailed error code if any
    };

}  // namespace MySQLite


using namespace std;
const static std::string TAG= "MySQlite3";
class Mysqlite3
{
public:
	Mysqlite3();
	~Mysqlite3();
public:
	int    open(const char* dbfile);
	int    created();

    int     InsertToDB(MyJLXArray& data,std::string& name,int& index);

    int     InsertArcData(const std::string& tablename,
						 const std::string& name,
						 const int& sex,const int& age,
						 unsigned char* data,size_t dataSize);
	int 	CreateArcTable(const std::string& tablename,const std::string& sql);
	int 	prepare(const std::string& query);

template<typename Ta,typename Tb,typename Tc>
	int    FromStrToVector(Ta	&sourceStr,Tb &feat_vector_len,Tc &vector);

    int    ReadDBtoText(std::vector<std::string> &filename,
                        int &feat_vector_len,
						std::vector<MyJLXArray> &db_feat_vectors);

    int    prepare();
	int    GetAllRowFromTable();
	int    GetUserName_FromTable();
	int    DeleteFromTableWhereName(const char* name);

	int    GetNamelist(std::vector<std::string>& out);
	void   comp_vector(std::vector<int>& data,const char* name);
	int    InsertName();
    bool   OpenDBStatus;
	bool   CheckNameExist(const char* name);
public:
	std::vector<std::string>     db_feat_name;
public:
    static int   tableLineNum;//
    static bool  mCheckNameExist;
    static std::vector<std::string>  tableAllname;
    static int  callback(void *NotUsed, int argc, char **argv, char **azColName);
    static int  name_callback(void *NotUsed, int argc, char **argv, char **azColName);
    static int  count_callback(void *NotUsed, int argc, char **argv, char **azColName);
public:
	inline int getnameNum() const {
		return nameMap.size();
	}
protected:
	char 	*zErrMsg = 0;
	sqlite3_stmt *stmt;

	//新建表,四个字段
	const char*  CREATE_TABLE = "CREATE TABLE IF NOT EXISTS feat(name TEXT,num INT,basic FLOAT,vectors TEXT);";
	const char*  GET_ALL_LINE_SQL = "select count(*) from feat;";
	const char*  GET_ALL_NAME_SQL = "select name from feat;";
	const char*  DeleteUser_SQL   = "delete from feat where name=";
	const char*  DeleteAll_SQL    = "delete from feat;";
	const char*  ExistUserSureSQL = "select num from feat where name=";//if num >= 0
	const char * query = "SELECT * FROM feat;";//from feat(table)
	std::map<string, int> nameMap;

	const std::string CREATE_TABLE_PRE  ="CREATE TABLE IF NOT EXISTS ";//+ table name, + other ..
	const std::string QUERY_PRE			="SELECT * FROM ";//+ table name

	const std::string separat  			= ",";
	const std::string begin_data        = "(";
	const std::string data_end 			= ");";

private:
    inline void check(const int& aRet) const
    {
        if (SQLITE_OK != aRet)
        {
            throw MySQLite::Exception(db, aRet);
        }
    }
    inline const char* getErrorMsg() const noexcept // nothrow
	{
    	return sqlite3_errmsg(db);
	}
private:
	sqlite3 *db;
};

#endif