
#pragma once

#include <getopt.h>
#include <map>
#include <string>

using namespace std;

struct auto_flag {
//	string name;
	int has_arg;
//	int *flag;
	int val;
	void *value;
	string help;
	auto_flag() :
			has_arg(0), val(0), value(0) {
	}
	auto_flag(int has_arg, void *value, string help) :
			has_arg(has_arg), val(0), value(value), help(help) {
	}
};

typedef map<string, auto_flag> auto_flags;

/**
 * add bool arg
 *
 * @param flags
 * @param name
 * @param value
 * @param help
 */
inline void
add_no_argument(auto_flags *flags, string name, int *value, string help="") {
	(*flags)[name] = auto_flag(no_argument, value, help);
}

/**
 * add value arg
 *
 * @param flags
 * @param name
 * @param value
 * @param help
 */
inline void
add_required_argument(auto_flags *flags, string name, string *value, string help="") {
	(*flags)[name] = auto_flag(required_argument, value, help);
}

// private
struct do_print_help {
	void operator()(pair<string, auto_flag> p)const{
		auto_flag f = p.second;
		printf("   ");
		if (f.val)
			printf("-%c, ", f.val);
		printf("--%s", p.first.c_str());
		if (f.has_arg)
			printf("=value");
		printf("       %s", f.help.c_str());
		if (f.has_arg)
			printf(" (default=%s)", (*(string *) f.value).c_str());
		printf("\n");
	}
};

/**
 * parse_auto_flags
 *
 * @return arg
 */
inline int
parse_auto_flags(auto_flags *flags, int argc, char **argv) {
	string shortopts;
	vector<option> longopts;
	for (auto_flags::iterator iter = (*flags).begin(); iter != (*flags).end(); ++iter) {
		const char *name = (*iter).first.c_str();
		int has_arg=(*iter).second.has_arg;
		int val=(*iter).second.val;
		option next = {name, has_arg, 0, val};
		longopts.push_back(next);
		if (val) {
			shortopts+=val;
			if (has_arg)
				shortopts+=":";
		}
	}
	option last = {0,0,0,0};
	longopts.push_back(last);
	int c;
	int option_index = 0;
	while ((c = getopt_long(argc, argv, shortopts.c_str(), longopts.data(), &option_index)) != -1) {
		void *value = (*flags)[longopts[option_index].name].value;
		switch (c) {
		case 0:
			printf("option %s", longopts[option_index].name);
			if (optarg)
				printf(" with arg %s", optarg);
			printf("\n");
			if (optarg == 0)
				*((int *) value) = 1;
			else
				*((string *) value) = optarg;
			break;

		case '?':
		case 'h':
			printf("Usage: %s [options]\n", argv[0]);
			for_each((*flags).begin(), (*flags).end(), do_print_help());
			exit(1);

		default:
			printf("?? getopt returned character code 0%o ??\n", c);
		}
	}
	return optind;
}

// private
struct do_print_values {
	void operator()(pair<string, auto_flag> p)const{
		auto_flag f = p.second;
		printf("%s=", p.first.c_str());
		if (f.has_arg)
			printf("%s", (*(string *) f.value).c_str());
		else
			printf("%d", (*(int *) f.value));
		printf("\n");
	}
};

/**
 * print_auto_flags
 *
 * convenience function to print the flags values to stdout
 *
 * @param flags the flags context to print
 */
inline void
print_auto_flags(auto_flags *flags) {
	for_each((*flags).begin(), (*flags).end(), do_print_values());
}
