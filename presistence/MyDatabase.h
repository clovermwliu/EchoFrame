#ifndef MYFRAMEWORK2_MYDATABASE_H
#define MYFRAMEWORK2_MYDATABASE_H

#include "MyDatabase.h"
#include "dcex.h"
#include <map>
#include <vector>
#include <stdlib.h>

namespace db_util
{

/////////////////////////////////////////////////
/**
* @brief mysql操作类. 
*
*/           
/////////////////////////////////////////////////


/**
* @brief 数据库异常类
*/
struct DCMysql_Exception : public DCException
{
	DCMysql_Exception(const string &sBuffer, int err) : DCException(sBuffer, err){};
	~DCMysql_Exception() throw(){};    
};

/**
* @brief 数据库配置接口
*/
struct DCDBConf
{
    /**
    * 主机地址
    */
    string _host;

    /**
    * 用户名
    */
    string _user;

    /**
    * 密码
    */
    string _password;

    /**
    * 数据库
    */
    string _database;

    /**
     * 字符集
     */
    string _charset;

    /**
    * 端口
    */
    int _port;

    /**
    * 客户端标识
    */
    int _flag;

    /**
    * @brief 构造函数
    */
    DCDBConf()
        : _port(0)
        , _flag(0)
    {
    }

    /**
	* @brief 读取数据库配置. 
	* 
	* @param mpParam 存放数据库配置的map 
	*   	 dbhost: 主机地址
	*   	 dbuser:用户名
	*   	 dbpass:密码
	*   	 dbname:数据库名称
	*   	 dbport:端口
    */
    void loadFromMap(const map<string, string> &mpParam)
    {
        map<string, string> mpTmp = mpParam;

        _host        = mpTmp["dbhost"];
        _user        = mpTmp["dbuser"];
        _password    = mpTmp["dbpass"];
        _database    = mpTmp["dbname"];
        _charset     = mpTmp["charset"];
        _port        = atoi(mpTmp["dbport"].c_str());
        _flag        = 0;

        if(mpTmp["dbport"] == "")
        {
            _port = 3306;
        }
    }

};

/**
* @brief Mysql数据库操作类 
* 
* 非线程安全，通常一个线程一个DCMysql对象；
* 
* 对于insert/update可以有更好的函数封装，保证SQL注入；
* 
* DCMysql::DB_INT表示组装sql语句时，不加””和转义；
* 
* DCMysql::DB_STR表示组装sql语句时，加””并转义；
*/
class DCMysql 
{
public:
    /**
    * @brief 构造函数
    */
    DCMysql();

    /**
	* @brief 构造函数. 
	*  
    * @param sHost        主机IP
    * @param sUser        用户
    * @param sPasswd      密码
    * @param sDatebase    数据库
    * @param port         端口
    * @param iUnixSocket  socket
    * @param iFlag        客户端标识
    */
    DCMysql(const string& sHost, const string& sUser = "", const string& sPasswd = "", const string& sDatabase = "", const string &sCharSet = "", int port = 0, int iFlag = 0);

    /**
    * @brief 构造函数. 
    * @param tcDBConf 数据库配置
    */
    DCMysql(const DCDBConf& tcDBConf);

    /**
    * @brief 析构函数.
    */
    ~DCMysql();

    /**
	* @brief 初始化. 
	*  
    * @param sHost        主机IP
    * @param sUser        用户
    * @param sPasswd      密码
    * @param sDatebase    数据库
    * @param port         端口
    * @param iUnixSocket  socket
    * @param iFlag        客户端标识
    * @return 无
    */
    void init(const string& sHost, const string& sUser  = "", const string& sPasswd  = "", const string& sDatabase = "", const string &sCharSet = "", int port = 0, int iFlag = 0);

    /**
	* @brief 初始化. 
	*  
    * @param tcDBConf 数据库配置
    */
    void init(const DCDBConf& tcDBConf);

    /**
	* @brief 连接数据库. 
	*  
    * @throws DCMysql_Exception
    * @return 无
    */
    void connect();
 
    /**
    * @brief 断开数据库连接. 
    * @return 无
    */
    void disconnect();

    /**
     * @brief 获取数据库变量. 
     * 
     * @return 数据库变量
     */
    string getVariables(const string &sName);

    /**
	*  @brief 直接获取数据库指针. 
	*  
    * @return MYSQL* 数据库指针
    */
    MYSQL *getMysql();

    /**
	*  @brief 字符转义. 
	*  
    * @param sFrom  源字符串
    * @param sTo    输出字符串
    * @return       输出字符串
    */
    string escapeString(const string& sFrom);

    /**
	* @brief 更新或者插入数据. 
	*  
    * @param sSql  sql语句
    * @throws      DCMysql_Exception
    * @return
    */
    void execute(const string& sSql);

    /**
     *  @brief mysql的一条记录
     */
    class MysqlRecord
    {
    public:
        /**
		 * @brief 构造函数.
		 *  
         * @param record
         */
        MysqlRecord(const map<string, string> &record);

        /**
		 * @brief 获取数据，s一般是指数据表的某个字段名 
		 * @param s 要获取的字段
         * @return  符合查询条件的记录的s字段名
         */
        const string& operator[](const string &s) const;
        
    protected:
        const map<string, string> &_record;
    };

    /**
     * @brief 查询出来的mysql数据
     */
    class MysqlData
    {
    public:
        MysqlData() {}
        MysqlData(const MysqlData& r);
        
        /**
         * @brief 所有数据.
         * 
         * @return vector<map<string,string>>&
         */
        vector<map<string, string> >& data();

        /**
         * 数据的记录条数
         * 
         * @return size_t
         */
        size_t size();

        /**
		 * @brief 获取某一条记录. 
		 *  
		 * @param i  要获取第几条记录 
         * @return   MysqlRecord类型的数据，可以根据字段获取相关信息，
         */
        MysqlRecord operator[](size_t i);
        
        /*
         * 读写函数
         */
        void set_affect_rows(uint32_t rows) {_affect_rows = rows;}
        
        void set_errno(int32_t err) {_errno = err;}
        
        void set_error_str(const string& err_str) {_err_str = err_str;}
        
        void set_last_insert_id(uint64_t last_insert_id) {_last_insert_id = last_insert_id;}
        
        uint32_t affect_rows() const {return _affect_rows;}
        
        int32_t error() const {return _errno;}
        
        const string& error_str() const {return _err_str;}
        
        uint64_t last_insert_id() const {return _last_insert_id;}

    protected:
        vector<map<string, string> > _data;
        
        uint64_t _last_insert_id;
        
        int32_t _errno;
        
        string _err_str;
        
        int32_t _affect_rows;
        
    };

    /**
	* @brief Query Record. 
	*  
    * @param sSql sql语句
    * @throws     DCMysql_Exception
    * @return     MysqlData类型的数据，可以根据字段获取相关信息
    */
    MysqlData queryRecord(const string& sSql);

    /**
	 * @brief 定义字段类型， 
	 *  DB_INT:数字类型 
     *  DB_STR:字符串类型
     */
    enum FT
    {
        DB_INT,     
        DB_STR,    
    };

     /**
	 * 数据记录
     */
    typedef map<string, pair<FT, string> > RECORD_DATA;

    /**
	* @brief 更新记录. 
	*  
    * @param sTableName 表名
    * @param mpColumns  列名/值对
    * @param sCondition where子语句,例如:where A = B
    * @throws           DCMysql_Exception
    * @return           size_t 影响的行数
    */
    size_t updateRecord(const string &sTableName, const map<string, pair<FT, string> > &mpColumns, const string &sCondition);

    /**
	* @brief 插入记录. 
	*  
    * @param sTableName  表名
    * @param mpColumns  列名/值对
    * @throws           DCMysql_Exception
    * @return           size_t 影响的行数
    */
    size_t insertRecord(const string &sTableName, const map<string, pair<FT, string> > &mpColumns);

    /**
	* @brief 替换记录. 
	*  
    * @param sTableName  表名
    * @param mpColumns   列名/值对
    * @throws            DCMysql_Exception
    * @return            size_t 影响的行数
    */
    size_t replaceRecord(const string &sTableName, const map<string, pair<FT, string> > &mpColumns);

    /**
	* @brief 删除记录.  
	*  
    * @param sTableName   表名
    * @param sCondition   where子语句,例如:where A = B
    * @throws             DCMysql_Exception
    * @return             size_t 影响的行数
    */
    size_t deleteRecord(const string &sTableName, const string &sCondition = "");

    /**
	* @brief 获取Table查询结果的数目. 
	*  
    * @param sTableName 用于查询的表名
    * @param sCondition where子语句,例如:where A = B
    * @throws           DCMysql_Exception
    * @return           size_t 查询的记录数目
	*/
    size_t getRecordCount(const string& sTableName, const string &sCondition = "");

    /**
	* @brief 获取Sql返回结果集的个数. 
	*  
    * @param sCondition where子语句,例如:where A = B
    * @throws           DCMysql_Exception
    * @return           查询的记录数目
	*/
    size_t getSqlCount(const string &sCondition = "");

    /**
	 * @brief 存在记录. 
	 *  
     * @param sql  sql语句
     * @throws     DCMysql_Exception
     * @return     操作是否成功
     */
    bool existRecord(const string& sql);

    /**
	* @brief 获取字段最大值. 
	*  
    * @param sTableName 用于查询的表名
    * @param sFieldName 用于查询的字段
    * @param sCondition where子语句,例如:where A = B
    * @throws           DCMysql_Exception
    * @return           查询的记录数目
	*/
    int getMaxValue(const string& sTableName, const string& sFieldName, const string &sCondition = "");

    /**
	* @brief 获取auto_increment最后插入得ID. 
	*  
    * @return ID值
	*/
    long lastInsertID();

    /**
	* @brief 构造Insert-SQL语句. 
	*  
    * @param sTableName  表名
    * @param mpColumns  列名/值对
    * @return           string insert-SQL语句
    */
    string buildInsertSQL(const string &sTableName, const map<string, pair<FT, string> > &mpColumns);

    /**
	* @brief 构造Replace-SQL语句. 
	*  
    * @param sTableName  表名
    * @param mpColumns 列名/值对
    * @return           string insert-SQL语句
    */
    string buildReplaceSQL(const string &sTableName, const map<string, pair<FT, string> > &mpColumns);

    /**
	* @brief 构造Update-SQL语句. 
	*  
    * @param sTableName  表名
    * @param mpColumns   列名/值对
    * @param sCondition  where子语句
    * @return            string Update-SQL语句
    */
    string buildUpdateSQL(const string &sTableName,const map<string, pair<FT, string> > &mpColumns, const string &sCondition);

    /**
     * @brief 获取最后执行的SQL语句.
     * 
     * @return SQL语句
     */
    string getLastSQL() { return _sLastSql; }

	/**
	 * @brief 获取查询影响数
	 * @return int
	 */
	 size_t getAffectedRows();
protected:
    /**
	* @brief copy contructor，只申明,不定义,保证不被使用 
    */
    DCMysql(const DCMysql &tcMysql);

    /**
    * 
    * @brief 只申明,不定义,保证不被使用
    */
    DCMysql &operator=(const DCMysql &tcMysql);


private:

    /**
    * 数据库指针
    */
    MYSQL 		*_pstMql;

    /**
    * 数据库配置
    */
    DCDBConf   _dbConf;
    
    /**
    * 是否已经连接
    */
    bool		_bConnected;

    /**
     * 最后执行的sql
     */
    string      _sLastSql;
  
};

}
#endif //_MoguMYSQL_H
