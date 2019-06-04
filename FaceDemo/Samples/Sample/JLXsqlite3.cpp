#include <exception>
#ifdef ANDROID
#include <logging.h>
#endif
#include <array>
#include <unistd.h>
#include <string>
#include "JLXsqlite3.hpp"
#include "JLXDefine.hpp"
#include "sqlite3.h"

#define SQL_DEBUG 0
#define LEN_FLOAT_STR 204800// max array

union FloatRand {
    struct 
    {
        unsigned long Frac:23;
        unsigned long Exp:8;
        unsigned long Signed:1;
    } BitArea;
    float Value;
};

float GetFloatRand() {
    union FloatRand r;
    r.BitArea.Signed = 0;
    r.BitArea.Exp = 1;
    r.BitArea.Frac = (rand() * rand()) % 0x800000;

    if (r.BitArea.Frac == 0x7FFFFF)
        r.BitArea.Exp = rand() % 0x7E;
    else if (r.BitArea.Frac == 0)
        r.BitArea.Exp = rand() % 0x7E + 1;
    else
        r.BitArea.Exp = rand() % 0x7F;

    return r.Value;
}

namespace MySQLite
{
    Exception::Exception(const char* aErrorMessage) :
            std::runtime_error(aErrorMessage),
            mErrcode(-1), // 0 would be SQLITE_OK, which doesn't make sense
            mExtendedErrcode(-1)
    {
    }
    Exception::Exception(const std::string& aErrorMessage) :
            std::runtime_error(aErrorMessage),
            mErrcode(-1), // 0 would be SQLITE_OK, which doesn't make sense
            mExtendedErrcode(-1)
    {
    }
    Exception::Exception(const char* aErrorMessage, int ret) :
            std::runtime_error(aErrorMessage),
            mErrcode(ret),
            mExtendedErrcode(-1)
    {
    }
    Exception::Exception(const std::string& aErrorMessage, int ret) :
            std::runtime_error(aErrorMessage),
            mErrcode(ret),
            mExtendedErrcode(-1)
    {
    }
    Exception::Exception(sqlite3* apSQLite) :
            std::runtime_error(sqlite3_errmsg(apSQLite)),
            mErrcode(sqlite3_errcode(apSQLite)),
            mExtendedErrcode(sqlite3_extended_errcode(apSQLite))
    {
    }

    Exception::Exception(sqlite3* apSQLite, int ret) :
            std::runtime_error(sqlite3_errmsg(apSQLite)),
            mErrcode(ret),
            mExtendedErrcode(sqlite3_extended_errcode(apSQLite))
    {
    }
// Return a string, solely based on the error code
    const char* Exception::getErrorStr() const noexcept // nothrow
    {
        return sqlite3_errstr(mErrcode);
    }

}  // namespace MySQLite


//=======================================================================
Mysqlite3::Mysqlite3():OpenDBStatus(false){
}
Mysqlite3::~Mysqlite3(){//in the destructor,close db
  const int ret = sqlite3_close(db);
  (void) ret;
}


int   Mysqlite3::tableLineNum = 0;//
bool  Mysqlite3::mCheckNameExist = false;

std::vector<std::string>  Mysqlite3::tableAllname;

int Mysqlite3::callback(void *NotUsed, int argc, char **argv, char **azColName){

    for(int i=0; i < argc; i++)
    {

#if SQL_DEBUG
    #ifdef ANDROID
    LOG(INFO) << "SQL callback" << azColName[i] <<  "="  << (argv[i] ? argv[i] : "NULL");
    #else
    std::cout << "SQL callback" << azColName[i] <<  "="  << (argv[i] ? argv[i] : "NULL");
    #endif
#endif
        if (strcmp(azColName[i],"num") == 0)
        {
            if (atoi(argv[i]) >= 0)
            {
                mCheckNameExist = true;
            }
        }
    }
    return 0;
}



int Mysqlite3::count_callback(void *NotUsed, int argc, char **argv, char **azColName){
    int i;
    for(i = 0; i < argc; i++){
#if SQL_DEBUG
#ifdef ANDROID
        LOG(INFO) << "SQL callback" << azColName[i] <<  "="  << (argv[i] ? argv[i] : "NULL");
#else
    std::cout << "SQL callback" << azColName[i] <<  "="  << (argv[i] ? argv[i] : "NULL");
#endif
#endif
        if (strcmp(azColName[i],"count(*)") == 0)
        {
            tableLineNum = atoi(argv[i]);
        }
    }
    return 0;
}

int Mysqlite3::name_callback(void *NotUsed, int argc, char **argv, char **azColName){
    int i;
    for(i=0; i < argc; i++){
#if SQL_DEBUG
#ifdef ANDROID
        LOG(INFO) << "SQL callback" << azColName[i] <<  "="  << (argv[i] ? argv[i] : "NULL");
#else
         std::cout <<"SQL callback" << azColName[i] <<  "="  << (argv[i] ? argv[i] : "NULL");
#endif
#endif
        if (strcmp(azColName[i],"name") == 0)
        {
            tableAllname.push_back(string(argv[i]));
        }
    }
    return 0;
}


int Mysqlite3::open(const char* dbfile) {
    const int rc = sqlite3_open(dbfile, &db);
	if(SQLITE_OK != rc)
	{
    #if SQL_DEBUG
        LOG(ERROR) << TAG << "Can't open database:" << getErrorMsg();
    #endif
         const MySQLite::Exception exception(db, rc);
         sqlite3_close(db);
         throw exception;
    }
    else
    {
    #if SQL_DEBUG
        LOG(INFO)<< TAG << " Opened  database successfully";
    #endif
       fprintf(stderr, "Opened database successfully\n");
       OpenDBStatus = true;
    }
    //检验文件是否存在或可访问
	if(access(dbfile,F_OK) != 0){
        #if SQL_DEBUG
            LOG(ERROR) << TAG << " open failed,because no file no permission,please check out";
        #endif
            exit(1);
	}
    return  0;
}

int Mysqlite3::created(){
	const int rc = sqlite3_exec(db, CREATE_TABLE, callback, 0, &zErrMsg);
   	check(rc);
#if SQL_DEBUG
    LOG(INFO) << TAG << " Table created successfully";
#endif
   	return 0; 
}

int Mysqlite3::CreateArcTable(const std::string& tablename,const std::string& sql)
{
    std::string sql_exe;

    sql_exe = sql + tablename;

    sql_exe = sql_exe + this->begin_data;
    std::string  sex_field     = "sex INT";
    std::string  name_field    = "name TEXT";
    std::string  age_field     = "age INT";
    std::string  feature_filed = "feature BLOB";

    //field type , composite
    //sex INT ,name TEXT ,age INT ,feature BLOB (girl is 0, boy is 1);
    sql_exe = sql_exe + sex_field;
    sql_exe = sql_exe + this->separat;
    sql_exe = sql_exe + name_field;
    sql_exe = sql_exe + this->separat;
    sql_exe = sql_exe + age_field;
    sql_exe = sql_exe + this->separat;
    sql_exe = sql_exe + feature_filed;
    sql_exe = sql_exe + this->data_end;

    std::cout << "CreateArcTable  sql exe : " << sql_exe << std::endl;

    const int rc = sqlite3_exec(db,sql_exe.c_str(),callback,0,&zErrMsg);
    check(rc);
    return 0;
}

int  Mysqlite3::InsertArcData(const std::string& tablename,
                              const std::string& name,const int& sex,
                              const int& age,
                              unsigned char* data,
                              size_t dataSize)
{
    if(!tablename.empty() && (data != nullptr))
    {
        std::string     sql;//待执行的sql语句..
        std::string     insert("INSERT INTO ");
        std::string     arefer("\'");

        std::string     uncertainty("?");
        std::string     value(" (sex,name,age,feature) VALUES ");


        insert = insert + tablename;
        insert = insert + value;

        insert = insert + this->begin_data;
        int     tmp_sex = sex;
        std::string sex_str = std::to_string(tmp_sex);
        int     tmp_age = age;
        std::string age_str = std::to_string(tmp_age);

        insert = insert + sex_str;
        insert = insert + this->separat;

        insert = insert + arefer;
        insert = insert + name;
        insert = insert + arefer;

        insert = insert + this->separat;

        insert = insert + age_str;
        insert = insert + this->separat;
        insert = insert + uncertainty;
        insert = insert + this->data_end;

        std::cout << "Arcsoft Insert SQL : " << insert << std::endl;
        prepare(insert);

        int rc = sqlite3_bind_blob(stmt,1,data,dataSize,NULL);

        if(rc != SQLITE_OK)
        {
            std::cout << "bind blob fail, Error code : "<< rc << std::endl;
        }
        else
        {
            sqlite3_step(stmt);//将数据写入数据库中?
            std::cout << "bind blob success " << std::endl;
        }
        sqlite3_finalize(stmt);

        return 0;//执行完毕..
    }
    return -1;
}

int Mysqlite3::prepare(const std::string& query){
    int rc = sqlite3_prepare_v2(db,query.c_str(),-1,&stmt,NULL);
    if(SQLITE_OK == rc){
        std::cout << "sqlite3_prepare_v2 OK " << std::endl;
        return 0;
    }else{
        std::cout << "prepare fail ,Error Code :  " << rc <<  std::endl;
        sqlite3_finalize(stmt);
    }
    return -1;
}


template<typename Ta,typename Tb,typename Tc>
int    Mysqlite3::FromStrToVector(Ta &sourceStr,Tb &feat_vector_len,Tc &vector){

    size_t comma = 0;
    size_t comma2 = 0;

    std::array<float,4096> vector_tmp;

    //LOG(ERROR) << "FromStrToVector :";
    comma   = sourceStr.find(',',0);
    float i = strtof(sourceStr.substr(0,comma).c_str(),NULL);
    vector_tmp[0] = i;

    int foundNum = 1;
    int index    = 1;
    while (comma + 1 < sourceStr.size() && foundNum < feat_vector_len)
    {
        comma2 = sourceStr.find(',',comma + 1);
        float ij = strtof(sourceStr.substr(comma + 1,comma2-comma-1).c_str(),NULL);

        vector_tmp[index] =   ij;
        comma = comma2;
        foundNum++;
        index++;
    }

    //vector = vector_tmp.data();
    vector = vector_tmp;

    //LOG(ERROR) << "FromStrToVector end --->" ;
	return 0;
}

//插入数据到数据库...
int Mysqlite3::InsertToDB(MyJLXArray& data,std::string& name,int& index){
    char    read_data[LEN_FLOAT_STR] = {0};
    for(int j = 0; j < data.size(); ++j) {
        float temp = data[j];
        char  f_str[64]={0};
        sprintf(f_str,"%f,",temp);
        strncat(read_data,f_str,strlen(f_str));
    }

    float   basic_cos = 0.0f;
    char    insert_data[LEN_FLOAT_STR + 64] = {0};

    sprintf(insert_data,"insert into feat (name,num,basic,vectors) values ('%s',%d,%f,'%s');",name.c_str(),index,basic_cos,read_data);
    const int rc = sqlite3_exec(db, insert_data, callback, 0, &zErrMsg);

#if SQL_DEBUG
    LOG(INFO)<< TAG <<" insert_data: " << insert_data;
#endif
    if( rc != SQLITE_OK )
    {
   		fprintf(stderr, "SQL error: %s\n", zErrMsg);
      	sqlite3_free(zErrMsg);
        #if SQL_DEBUG
            LOG(ERROR)<< TAG <<" SQL Error: " << zErrMsg;
        #endif
   	}
   	else
   	{
       fprintf(stdout, " insertToDB_form_csv successfully\n");
        #if SQL_DEBUG
            LOG(INFO) << TAG << "insertToDB_form_csv successfully";
        #endif
   	}
   	return 0;
}


int Mysqlite3::prepare(){
	const int   result = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
	if(SQLITE_OK != result)  
    {
#if SQL_DEBUG
    LOG(ERROR)<< TAG << " sqlite3  prepare failed";
#endif
      std::cout << getErrorMsg() << std::endl;
      sqlite3_finalize(stmt);
      return -32;
    }
    return 0;
}

int Mysqlite3::GetAllRowFromTable(){
      const int rc = sqlite3_exec(db,GET_ALL_LINE_SQL,count_callback,0,&zErrMsg);
      check(rc);
      return tableLineNum;
}

int Mysqlite3::InsertName(){
    std::map<string, int>::iterator   it; 
    for (size_t i = 0; i < tableAllname.size(); ++i)
    {
      if (it != nameMap.end())
      {
        nameMap[it->first]++;
      }
      else
      {
        nameMap.insert(std::pair<string, int>(tableAllname[i], 0));
      }
    }
    return 0;
}

int Mysqlite3:: GetUserName_FromTable(){
  tableAllname.clear();
  nameMap.clear();
  const int rc = sqlite3_exec(db,GET_ALL_NAME_SQL,name_callback,0,&zErrMsg);
  check(rc);
#if SQL_DEBUG
    LOG(INFO) << TAG << " select name from feat success";
#endif
    //Get  tableAllname 
  InsertName();
#if SQL_DEBUG
    LOG(INFO) << TAG << " "<< nameMap.size() << " GetUserName_FromTable()  end ";
#endif
  return 0;
}

bool Mysqlite3::CheckNameExist(const char* name){
    mCheckNameExist = false;
    if (name != NULL)
    {
        std::string  exist_sql = string(ExistUserSureSQL) + string("\"") + string(name) + string("\";");
        const int rc = sqlite3_exec(db,exist_sql.c_str(),callback,0,&zErrMsg);
        check(rc);
#if SQL_DEBUG
        LOG(INFO)<< TAG <<" exist Sqlite from feat success " << name;
#endif
    }
    else
    {
#if SQL_DEBUG
        LOG(ERROR) << TAG << " SQL   name error";
#endif
    }
    return  mCheckNameExist;
}

int Mysqlite3::DeleteFromTableWhereName(const char* name){
    if (strcmp(name,"deleteall") == 0){
        std::string  sql = string(DeleteAll_SQL);
        const int rc = sqlite3_exec(db,sql.c_str(),callback,0,&zErrMsg);
        check(rc);
    #if SQL_DEBUG
        LOG(INFO) << TAG << " Delete  feat success" << name;
    #endif
    }
    if (CheckNameExist(name)){
        std::string  sql = string(DeleteUser_SQL) + string("\"") + string(name) + string("\";");
        const int rc = sqlite3_exec(db,sql.c_str(),callback,0,&zErrMsg);
        check(rc);
    #if SQL_DEBUG
        LOG(INFO) << TAG << " Delete name from feat success" << name;
    #endif
    }else{
        #ifdef ANDROID
        LOG(ERROR) << TAG << "Delete name ,not exist";
        #else
        std::cout << TAG << "Delete name ,not exist";
        #endif
    }
    return 0;
}

//从数据库中读取 vectors 并  放入 db_feat_vectors  这个 数据类型中...
int Mysqlite3::ReadDBtoText(std::vector<std::string> &filename,
                            int &feat_vector_len,
                            std::vector<MyJLXArray> &db_feat_vectors){

    int  index = 0;
    db_feat_name.clear();
    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *get_stmt_name = (const char *)sqlite3_column_text(stmt, 0);//得到 name
        if(get_stmt_name != NULL){
            if(string(get_stmt_name).size() > 0){
                db_feat_name.push_back(string(get_stmt_name));
            }
            #if SQL_DEBUG
                LOG(INFO)<< TAG << " get_stmt_name : "<< string(get_stmt_name);
            #endif
        }

        filename.at(index) = string(get_stmt_name);//从数据库中导出 名字

        const char *vectors = (const char *)sqlite3_column_text(stmt, 3);//第三个字段  下标从0 开始
        #if SQL_DEBUG
            //LOG(INFO) << "get_stmt_num vectors: " << vectors;
        #endif
        try
        {
            if (vectors != NULL) {
                MyJLXArray v_ptr;//
                std::string tempstring(vectors);
                FromStrToVector(tempstring,feat_vector_len,v_ptr);
                #if SQL_DEBUG
                    LOG(INFO) << TAG <<"###from_str_to_vector end###";
                #endif
                if (db_feat_vectors.size() >= index)
                {
                    db_feat_vectors[index] = v_ptr;//?
                }
                else
                {
                    db_feat_vectors.push_back(v_ptr);
                }
            }
        }
        catch(std::exception e)
        {
            #ifdef ANDROID
            LOG(ERROR) << TAG << " thrown exception  : "<< e.what();
            #else
            std::cout << TAG << " thrown exception  : "<< e.what();
            #endif
        }
        index++;
    }
    #if SQL_DEBUG
        LOG(INFO) << TAG << " readDb_TEXT_to_char end##";
    #endif
    return  0;
}

int   Mysqlite3::GetNamelist(std::vector<std::string>& out){
    std::map<string, int>::iterator     it;
    for (it = nameMap.begin(); it != nameMap.end(); it++)
    {
        out.push_back(it->first);
        //LOG(ERROR) << "namemap : " << it->first;
    }
    return 0;
}

void  Mysqlite3::comp_vector(std::vector<int>& data,const char* name){
    #if SQL_DEBUG
        LOG(INFO) << TAG << " compare vector size :" <<  tableAllname.size() << " name: " << name;
    #endif
      for (int index = 0; index < tableAllname.size(); index++)
      {
          if( strcmp(tableAllname.at(index).c_str(),name) == 0)
          {
             data.push_back(index);
            #if SQL_DEBUG
                LOG(INFO) << TAG << " find index, data push_back  ok++: "<< index;//2018-0602-@leger.z
            #endif
          }
      }
}