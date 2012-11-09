#pragma once

#include <fstream>

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string/trim.hpp>

#include "auto_mysql.h"

using namespace std;
using namespace boost;
using namespace boost::algorithm;

struct OdbcConfig {
	string Server;
	string User;
	string Password;
	string Database;
};

/**
 * @param dsn from /etc/odbc.ini
 */
inline auto_mysql
OdbcNewConnection(string dsn) {
	string each_dsn;

	boost::regex re1("\\[([a-zA-Z0-9]+)\\]"); // [foo]
	boost::regex re2("([a-zA-Z0-9]+)\\s*=\\s*(.+)"); // bar=baz

	map<string/*each_dsn*/, OdbcConfig> dsnMap;

	string line;
	ifstream odbc_ini("/etc/odbc.ini");
	while (getline(odbc_ini, line)) {
		trim(line);

		if (line[0] == '#')
			continue;

		boost::match_results<const char *> m;

		if (boost::regex_match(line.c_str(), m, re1))
			each_dsn = m[1].str();

		if (boost::regex_match(line.c_str(), m, re2)) {
			string key(m[1].str());
			string value(m[2].str());
			if (key == "Server")
				dsnMap[each_dsn].Server = m[2].str();
			if (key == "User")
				dsnMap[each_dsn].User = m[2].str();
			if (key == "Password")
				dsnMap[each_dsn].Password = m[2].str();
			if (key == "Database")
				dsnMap[each_dsn].Database = m[2].str();
		}
	}
	auto_mysql handle(mysql_init(0), safe_mysql_close);
	if (!handle)
		throw runtime_error("OdbcNewConnection: mysql_init: dsn="+dsn);
	if (mysql_real_connect(handle.get(), dsnMap[dsn].Server.c_str(), dsnMap[dsn].User.c_str(), dsnMap[dsn].Password.c_str(), dsnMap[dsn].Database.c_str(), 0, 0, 0) == 0)
		throw runtime_error(mysql_error(handle.get()));
	return handle;
}
