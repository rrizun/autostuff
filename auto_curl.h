
// libcurl wrapper

#pragma once

#include <stdio.h>
#include <stdlib.h>

#include <stdexcept>
#include <string>
#include <sstream>

#include <curl/curl.h>

#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

using namespace std;

#define VERIFY(e) e

// private
inline size_t
read_callback(void *ptr, size_t size, size_t nmemb, void *stream) {
  stringstream* in = (stringstream *) stream;
  (*in).read((char*) ptr, size *nmemb);
  return (*in).gcount();
}

// private
inline size_t
write_callback(void *ptr, size_t size, size_t nmemb, void *stream) {
  stringstream* out = (stringstream *) stream;
  (*out).write((char*) ptr, size *nmemb);
  return size * nmemb;
}

/**
 * invoke HTTP GET
 *
 * @param url e.g., "http://feeds.nytimes.com/nyt/rss/HomePage"
 * @return the http response payload
 */
inline string
curl_get(string url) {
//	stringstream in; // request payload (e.g., POST)
	stringstream out; // response payload
	boost::shared_ptr<CURL> curl(curl_easy_init(), curl_easy_cleanup);

	VERIFY(curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str()));
	VERIFY(curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1));
	long signal = 1;
	VERIFY(curl_easy_setopt(curl.get(), CURLOPT_NOSIGNAL, signal));
	long connect_timeout = 2;
	VERIFY(curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, connect_timeout));
	long readwrite_timeout = 5;
	VERIFY(curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, readwrite_timeout));
	// -k/--insecure (accept self-signed ssl certificates)
	VERIFY(curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 0));
	VERIFY(curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 0));

//	VERIFY(curl_easy_setopt(curl.get(), CURLOPT_READDATA, &in));
//	VERIFY(curl_easy_setopt(curl.get(), CURLOPT_READFUNCTION, read_callback));
	VERIFY(curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &out));
	VERIFY(curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, write_callback));

	CURLcode curlCode(curl_easy_perform(curl.get()));
	if (curlCode == 0) {
		long responseCode;
		if (curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &responseCode) == 0) {
			if (responseCode < 400)
				return out.str();
			throw runtime_error(boost::lexical_cast<string>(responseCode)+out.str()); // app layer error
		}
	}
	throw runtime_error(curl_easy_strerror(curlCode)); // transport layer error
}
