
// libcurl wrapper

#pragma once

#include <stdio.h>
#include <stdlib.h>

#include <map>
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

inline string
url_encode(string s) {
  string result;
  static const char hexAlphabet[] = "0123456789ABCDEF";
  for (unsigned i = 0; i < s.length(); ++i) {
    if (isalnum(s[i])) {
      result += s[i];
    } else if (s[i] == '-') {
      result += s[i];
    } else if (s[i] == '_') {
      result += s[i];
    } else if (s[i] == '.') {
      result += s[i];
    } else {
      result += "%";
      result += hexAlphabet[(unsigned char)(s[i]) / 16];
      result += hexAlphabet[(unsigned char)(s[i]) % 16];
    }
  }
  return result;
}

inline string
www_form_urlencoded(std::map<string, string> m) {
  string result;
  for (std::map<string, string>::iterator iter = m.begin(); iter != m.end(); ++iter) {
	if (iter != m.begin())
	  result += "&";
	result += url_encode((*iter).first);
	result += "=";
	result += url_encode((*iter).second);
  }
  return result;
}

/**
 * invoke HTTP transaction
 *
 * @param url e.g., "http://feeds.nytimes.com/nyt/rss/HomePage"
 * @return the http response payload
 */
inline string
curl_get(string url, string request_payload="") {
	stringstream in(request_payload); // request payload (e.g., POST)
	stringstream out; // response payload
	boost::shared_ptr<CURL> curl(curl_easy_init(), curl_easy_cleanup);

    VERIFY(curl_easy_setopt(curl.get(), CURLOPT_POST, 1));
    VERIFY(curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, request_payload.size()));

	VERIFY(curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str()));
	VERIFY(curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1));
	VERIFY(curl_easy_setopt(curl.get(), CURLOPT_NOSIGNAL, 1));
	long connect_timeout = 1;
	VERIFY(curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, connect_timeout));
	long readwrite_timeout = 5;
	VERIFY(curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, readwrite_timeout));
	// -k/--insecure (accept self-signed ssl certificates)
	VERIFY(curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 0));
	VERIFY(curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 0));

	VERIFY(curl_easy_setopt(curl.get(), CURLOPT_READDATA, &in));
	VERIFY(curl_easy_setopt(curl.get(), CURLOPT_READFUNCTION, read_callback));
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
