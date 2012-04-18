#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h> // for memset

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/lexical_cast.hpp>
#include <mysql/mysql.h>

using namespace std;
using namespace boost;

#define MYSQLTHROW(fmt,...)do{char _throw_tmp[2000];snprintf(_throw_tmp,sizeof(_throw_tmp),"%s:%d "fmt,__FILE__,__LINE__,##__VA_ARGS__);fprintf(stderr,"%s\n",_throw_tmp);throw runtime_error(_throw_tmp);}while(0)

struct dont_care {
	void operator()(map<string,string>)const{}
};

template<class T>
struct query_for {
	T result;
	void operator()(map<string, string> row) {
		result = boost::lexical_cast<T>((*row.begin()).second);
	};
};

struct count_rows {
	int result;
	count_rows(): result(0) {
	}
	void operator()(map<string, string> row) {
		++result;
	}
};

struct print_rows {
	void operator()(map<string, string> row) const {
		for (map<string, string>::iterator iter = row.begin(); iter != row.end(); ++iter)
			printf("%s=%s|", (*iter).first.c_str(), (*iter).second.c_str());
		printf("\n");
	}
};

// Usage: auto_mysql c(mysql_init(NULL), safe_mysql_close);
typedef shared_ptr<MYSQL> auto_mysql;

// use w/auto_mysql
inline void
safe_mysql_close(MYSQL *handle) {
	if (handle)
		mysql_close(handle);
}

// Usage: string now(auto_mysql_stmt(c.get(), "select now()").execute(query_for<string>()).result);
class auto_mysql_stmt {
public:
    // ctor
	auto_mysql_stmt(MYSQL *handle, string sql): sql(sql), stmt(mysql_stmt_init(handle)) {
		init(handle);
	}
    // ctor
	template<class T>
	auto_mysql_stmt(MYSQL *handle, string sql, T arg): sql(sql), stmt(mysql_stmt_init(handle)) {
		init(handle);
		param_values.push_back(boost::lexical_cast<string>(arg));
	}
    // ctor
	template<class T1, class T2>
	auto_mysql_stmt(MYSQL *handle, string sql, T1 arg1, T2 arg2): sql(sql), stmt(mysql_stmt_init(handle)) {
		init(handle);
		param_values.push_back(boost::lexical_cast<string>(arg1));
		param_values.push_back(boost::lexical_cast<string>(arg2));
	}
    // ctor
	template<class T1, class T2, class T3>
	auto_mysql_stmt(MYSQL *handle, string sql, T1 arg1, T2 arg2, T3 arg3): sql(sql), stmt(mysql_stmt_init(handle)) {
		init(handle);
		param_values.push_back(boost::lexical_cast<string>(arg1));
		param_values.push_back(boost::lexical_cast<string>(arg2));
		param_values.push_back(boost::lexical_cast<string>(arg3));
	}
    // ctor
	template<class T1, class T2, class T3, class T4>
	auto_mysql_stmt(MYSQL *handle, string sql, T1 arg1, T2 arg2, T3 arg3, T4 arg4): sql(sql), stmt(mysql_stmt_init(handle)) {
		init(handle);
		param_values.push_back(boost::lexical_cast<string>(arg1));
		param_values.push_back(boost::lexical_cast<string>(arg2));
		param_values.push_back(boost::lexical_cast<string>(arg3));
		param_values.push_back(boost::lexical_cast<string>(arg4));
	}
    // ctor
	template<class T1, class T2, class T3, class T4, class T5>
	auto_mysql_stmt(MYSQL *handle, string sql, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5): sql(sql), stmt(mysql_stmt_init(handle)) {
		init(handle);
		param_values.push_back(boost::lexical_cast<string>(arg1));
		param_values.push_back(boost::lexical_cast<string>(arg2));
		param_values.push_back(boost::lexical_cast<string>(arg3));
		param_values.push_back(boost::lexical_cast<string>(arg4));
		param_values.push_back(boost::lexical_cast<string>(arg5));
	}
    // dtor
	~auto_mysql_stmt() {
		if (stmt)
			mysql_stmt_close(stmt);
	}
	// for queries
	template<class T>
	T execute(T callback) {
		input_bindings.resize(param_values.size());
		for (size_t i = 0; i < param_values.size(); ++i) {
			input_bindings[i].buffer_type= MYSQL_TYPE_STRING;
			input_bindings[i].buffer= (char *) param_values[i].data();
			input_bindings[i].buffer_length= param_values[i].size();
		}
        if (mysql_stmt_bind_param(stmt, input_bindings.data()) != 0)
			MYSQLTHROW("%s %s", sql.c_str(), mysql_stmt_error(stmt));
		if (mysql_stmt_execute(stmt) != 0)
			MYSQLTHROW("%s %s", sql.c_str(), mysql_stmt_error(stmt));
		//if (mysql_stmt_num_rows())
		unsigned int num_fields = 0;
		shared_ptr<MYSQL_RES> res(mysql_stmt_result_metadata(stmt), mysql_free_result);
		if (res.get())
			num_fields = mysql_num_fields(res.get());
		if (num_fields > 0) {
			int result;
			output_bindings.resize(num_fields);
			// 0=ok 1=error 100=no_data
			while ((result = bind_and_fetch(num_fields)) != MYSQL_NO_DATA) {
				if (result == 1)
					MYSQLTHROW("%s %s", sql.c_str(), mysql_stmt_error(stmt));
				map<string, string> row;
				for (unsigned int field = 0; field < num_fields; ++field) {
					if (output_bindings[field].buffer_length > 0) {
						MYSQL_FIELD *fields = mysql_fetch_fields(res.get());
						row[fields[field].name].resize(output_bindings[field].buffer_length);
						output_bindings[field].buffer = (void*) row[fields[field].name].data();
						if (mysql_stmt_fetch_column(stmt, &output_bindings[field], field, 0) != 0)
							MYSQLTHROW("%s %s", sql.c_str(), mysql_stmt_error(stmt));
					}
				}
				callback(row);
			}
		}
		return callback;
	}
	// for updates
	void execute() {
		execute(dont_care());
	}
private:
	string sql;
	MYSQL_STMT *stmt;
    vector<string> param_values;
    vector<MYSQL_BIND> input_bindings;
	vector<MYSQL_BIND> output_bindings;
	void init(MYSQL *handle) {
		if (stmt == 0)
			MYSQLTHROW("%s %s", sql.c_str(), mysql_error(handle));
		if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size()) != 0)
			MYSQLTHROW("%s %s", sql.c_str(), mysql_stmt_error(stmt));
	}
	int bind_and_fetch(unsigned int num_fields) {
		for (unsigned int field = 0; field < num_fields; ++field) {
			output_bindings[field].buffer_type= MYSQL_TYPE_STRING;
			output_bindings[field].buffer_length= 0; // supplied length
			output_bindings[field].length = &output_bindings[field].buffer_length; // required length
		}
		if (num_fields > 0) {
			if (mysql_stmt_bind_result(stmt, output_bindings.data()) != 0)
				MYSQLTHROW("%s %s", sql.c_str(), mysql_stmt_error(stmt));
		}
		return mysql_stmt_fetch(stmt);
	}
};

class auto_mysql_tx {
	MYSQL *handle;
public:
	auto_mysql_tx(MYSQL *handle): handle(handle) {
		if (mysql_query(handle, "begin")!=0)
			MYSQLTHROW("%s", mysql_error(handle));
	}
	~auto_mysql_tx() {
		if (mysql_query(handle, "commit")!=0)
			MYSQLTHROW("%s", mysql_error(handle));
	}
};
