#include <execinfo.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // memset
#include <sys/stat.h> // struct stat
// foo
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>

#include "event2/event.h"
#include "event2/bufferevent.h"

#include <google/protobuf/descriptor.h>
#include <gtest/gtest.h>

#include "auto_mysql.h"
#include "odbcdatasources.h"
// bar
#include "person.pb.h"
#include "wozconfig.pb.h"
#include "wozstatus.pb.h"

//#include <gflags/gflags.h>

using namespace std;
using namespace boost;
using namespace google;
using namespace google::protobuf;

inline string
render_si_value(double value) {
	int si_unit = 0;
	static const char *si_units[] = {"","k","M","G","T","P","E","Z","Y"};
	while (1000 <= value) {
		++si_unit;
		value /= 1000;
	}
	stringstream tmp;
	tmp << fixed << setprecision(1) << value << si_units[si_unit];
	return tmp.str();
}

//// variadic template
//void
//output() { cout << "\n"; }
//
//// variadic template
//template<class T, class ...Args>
//void
//output(T cls, Args ...args) {
//	cout << cls << " ";
//	if (sizeof...(args) == 0)
//		cout << "\n";
//	else
//		output(args...);
//}

// gets the scalar field value
static string
GetFieldValue(Message *message, const FieldDescriptor *field) {
	switch (field->cpp_type()) {
	case FieldDescriptor::CPPTYPE_BOOL:
		return boost::lexical_cast<string>(message->GetReflection()->GetBool((*message), field));
	case FieldDescriptor::CPPTYPE_INT32:
		return boost::lexical_cast<string>(message->GetReflection()->GetInt32((*message), field));
	case FieldDescriptor::CPPTYPE_UINT64:
		return boost::lexical_cast<string>(message->GetReflection()->GetUInt64((*message), field));
	case FieldDescriptor::CPPTYPE_STRING:
		return boost::lexical_cast<string>(message->GetReflection()->GetString((*message), field));
	default:
		throw runtime_error(field->name());
	}
}

// sets the scalar value/adds to the columnar value (i.e., field can be a repeated field)
static void
SetFieldValue(Message *message, const FieldDescriptor *field, const string& value) {
	switch (field->cpp_type()) {
	case FieldDescriptor::CPPTYPE_BOOL:
		if (field->is_repeated())
			message->GetReflection()->AddBool(message, field, boost::lexical_cast<bool>(value));
		else
			message->GetReflection()->SetBool(message, field, boost::lexical_cast<bool>(value));
		break;
	case FieldDescriptor::CPPTYPE_STRING:
		if (field->is_repeated())
			message->GetReflection()->AddString(message, field, value);
		else
			message->GetReflection()->SetString(message, field, value);
		break;
	default:
		throw runtime_error(field->name()+value);
	}
}

struct OWReadConfigVisitor {
	::google::protobuf::Message *config;
	OWReadConfigVisitor(::google::protobuf::Message *config): config(config) {
	}
	void operator()(map<string, string> row) {
		//ComponentURI|RowId|name|status=active|value
		string name(row["name"]);
		string value(row["value"]);

		match_results<const char *> results;
		regex re("([a-z]+_[a-zA-Z0-9]+)_([0-9]+)"); // e.g., foo_barBaz_1
		if (regex_match(name.c_str(), results, re))
			name = results[1].str(); // e.g., foo_barBaz

		const FieldDescriptor *field = config->GetDescriptor()->FindFieldByName(name);
		if (field)
			SetFieldValue(config, field, value); // works for scalars and columnars
	}
};

/**
 * reads the given iVOG_Config config
 *
 * @param dsn e.g., "OWConfig"
 * @param config
 * @param component_uri e.g., "foo/ch01/sl02/vm3/4"
 */
inline void
OWReadConfig(string dsn, ::google::protobuf::Message *config, string component_uri) {
	auto_mysql handle(OdbcNewConnection(dsn));
	auto_mysql_stmt(handle.get(), "select * from OWConfig where ComponentURI=?", component_uri).execute(OWReadConfigVisitor(config));
}

/**
 * writes the given iVOG_Status status
 *
 * @param dsn e.g., "OWZzzStatus"
 * @param status
 * @param component_uri e.g., "foo/ch01/sl02/vm3/4"
 */
inline void
OWWriteStatus(string dsn, ::google::protobuf::Message *status, string component_uri) {
	auto_mysql handle(OdbcNewConnection(dsn));
	int last_row_id(auto_mysql_stmt(handle.get(), "select coalesce(max(RowId),0) from OWStatus").execute(query_for<int>()).result);
	int new_group_id(auto_mysql_stmt(handle.get(), "select coalesce(max(GroupId),0) from OWStatus").execute(query_for<int>()).result + 1);
	for (int index = 0; index < status->GetDescriptor()->field_count(); ++index) {
		const FieldDescriptor *field = status->GetDescriptor()->field(index);
		if (status->GetReflection()->HasField(*status, field)) {
			string name(field->name());
			string value(GetFieldValue(status, field));
			string sql2("insert into OWStatus (RowId, GroupId, name, value, ComponentURI) values (?,?,?,?,?)");
			auto_mysql_stmt(handle.get(), sql2, ++last_row_id, new_group_id, name, value, component_uri).execute();
		}
	}
}

// Note- rhel54 bundles libboost 1.33, filesystem3 was introduced in libboost 1.44
#include <dirent.h>
#include <boost/filesystem.hpp>
using namespace boost::filesystem;

/**
 * Returns the cpu core count.
 */
static int
cpu_core_count() {
	int result = 0;
	regex re("cpu\\d");
	path dir("/sys/devices/system/cpu");
	for (directory_iterator iter(dir), iter_end; iter != iter_end; ++iter) {
		if (regex_search((*iter).path().c_str(), re))
			++result;
	}
	return result;
}

#include "auto_flags.h"

static auto_flags flags;

static int verbose=0;
static int version=0;
static int run_unit_tests=0;
static string listen_addr("0.0.0.0:8000"); // local http proxy listen addr
static string remote_addr("0.0.0.0:8001"); // remote dpp request addr
static string cores("0");

int
main(int argc, char **argv) {

	// name, has_arg, flag, val

	add_no_argument(&flags, "verbose", &verbose, "enable verbose output");
	add_no_argument(&flags, "version", &version, "print version");

	add_no_argument(&flags, "run-unit-tests", &run_unit_tests, "run built-in unit-tests");

	add_required_argument(&flags, "cores", &cores, "number of cpu cores to use for media processing (0=auto detect)");
	add_required_argument(&flags, "listen", &listen_addr, "the local addr to use to receive http proxy requests");
	add_required_argument(&flags, "remote", &remote_addr, "the remote addr to use to send dpp requests");

	int arg = parse_auto_flags(&flags, argc, argv);

	while (arg < argc)
		printf("extra non-option ARGV-element: %s\n", argv[arg++]);

	print_auto_flags(&flags);

	auto_mysql handle(OdbcNewConnection("OWStatus"));
	auto_mysql_stmt(handle.get(), "describe OWStatus").execute(print_rows());
//	auto_mysql_stmt(handle.get(), "select * from OWStatus where ComponentURI=?", "foo").execute(print_rows());
//
//	WozConfig config;
//	OWReadConfig("OWConfig", &config, "foo");
////	config.PrintDebugString();
////	printf("cli_filebase is %s\n", config.cli_filebase().c_str());
//
//	WozStatus status;
//	status.set_bytes_sent(123);
//	status.set_bytes_received(456);
//	OWWriteStatus("OWZzzStatus", &status, "bar");

	//	printf("the count is %d\n", auto_mysql_stmt(handle.get(), "select * from OWConfig").execute(count_rows()).result);

	boost::shared_ptr<event_base> base(event_base_new(), event_base_free);

//	if (run_unit_tests)
	{
		  ::testing::InitGoogleTest(&argc, argv);
		  return RUN_ALL_TESTS();
	}

	return event_base_dispatch(base.get());
}
