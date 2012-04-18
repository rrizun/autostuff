
// thin veneer over libxml2

#pragma once

#include <stdio.h>
#include <stdlib.h>

#include <stdexcept>
#include <string>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlerror.h>

#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

using namespace std;

/**
 * type unsafe xml data transfer object
 *
 * parses xml documents and provides simplified access to tag values
 *
 * usage: e.g., get <rss> <channel> <title> value
 *
 * struct MyGetTitleVisitor {
 * 	string title;
 * 	void operator()(string tagName, XmlDto xmlDto) {
 * 		if (tagName=="channel")
 * 			title = xmlDto.get("title");
 * 	}
 * };
 *
 * string response_payload = curl_get("http://feeds.nytimes.com/nyt/rss/HomePage");
 * string rss_channel_title = XmlDto(response_payload).accept(MyGetTitleVisitor()).title;
 * printf("yeah! title is %s\n", rss_channel_title.c_str());
 */
class XmlDto {
	xmlNodePtr nodePtr;
    boost::shared_ptr<xmlDoc> doc;
    // private
	XmlDto(xmlNodePtr nodePtr): nodePtr(nodePtr) {
	}
public:
	/**
	 * public ctor
	 */
	XmlDto(string xml): nodePtr(0) {
        doc.reset(xmlReadMemory(xml.data(), xml.size(), "", 0, 0), xmlFreeDoc);
        if (doc==0)
        	throw runtime_error(xmlGetLastError()->message);
        for (xmlNodePtr childNode = doc->children; childNode != 0; childNode = childNode->next) {
    		if (childNode->type == XML_ELEMENT_NODE)
    			nodePtr = childNode;
        }
        if (nodePtr==0)
        	throw runtime_error(xmlGetLastError()->message);
	}

	/**
	 * get value
	 */
	string get(string tagName) const {
		string value;
		for (xmlNodePtr childNode = nodePtr->children; childNode != 0; childNode = childNode->next) {
			if (tagName == (const char *) childNode->name) {
				for (xmlNodePtr valueNode = childNode->children; valueNode != 0; valueNode = valueNode->next) {
					if (valueNode->content)
						value += (const char *) valueNode->content;
				}
			}
		}
		return value;
	}

	/**
	 * safe get value
	 *
	 * @return true if tagName/value was found
	 */
	bool safe_get(string tagName, string *value) const {
		int found=0;
		for (xmlNodePtr childNode = nodePtr->children; childNode != 0; childNode = childNode->next) {
			if (tagName == (const char *) childNode->name) {
				for (xmlNodePtr valueNode = childNode->children; valueNode != 0; valueNode = valueNode->next) {
					if (valueNode->content) {
						found=1;
						*value += (const char *) valueNode->content;
					}
				}
			}
		}
		return found;
	}

	/**
	 * visit immmediate children
	 */
	template<class T>
	T accept(T visitor) {
		for (xmlNodePtr childNode = nodePtr->children; childNode != 0; childNode = childNode->next) {
			string tagName((const char *) childNode->name);
			if (childNode->type == XML_ELEMENT_NODE)
				visitor(tagName, XmlDto(childNode));
		}
		return visitor;
	}
};
