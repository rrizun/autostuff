
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <gtest/gtest.h>
#include "auto_curl.h"
#include "auto_xml.h"

using namespace std;

class auto_xml_test: public ::testing::Test {
protected:
	virtual void SetUp() {
	}
	virtual void TearDown() {
	}
};

// ----------------------------------------------------------------------

struct MyGetTitleVisitor {
	string title;
	void operator()(string tagName, XmlDto xmlDto) {
		if (tagName=="channel")
			title = xmlDto.get("title");
	}
};

// get <rss> <channel> <title> value
TEST_F(auto_xml_test, TestGetFeed) {
	string title(XmlDto(curl_get("http://feeds.nytimes.com/nyt/rss/HomePage")).accept(MyGetTitleVisitor()).title);
	printf("holy crap! title is %s\n", title.c_str());
}

// ----------------------------------------------------------------------

struct ItemVisitor {
	void operator()(string tagName, XmlDto xmlDto) {
		if (tagName=="item") {
			string title;
			if (xmlDto.safe_get("title", &title))
				printf("holy crap! item is %s\n", title.c_str());
		}
	}
};

struct ChannelVisitor {
	void operator()(string tagName, XmlDto xmlDto) {
		if (tagName=="channel")
			xmlDto.accept(ItemVisitor());
	}
};

// get all <rss> <channel> <item> <title> values
TEST_F(auto_xml_test, TestGetItems) {
	XmlDto(curl_get("http://feeds.nytimes.com/nyt/rss/HomePage")).accept(ChannelVisitor());
}
