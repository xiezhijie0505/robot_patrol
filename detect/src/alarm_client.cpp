#include "alarm_client.h"

#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>

#ifdef HAVE_CURL
#include <curl/curl.h>
#endif

bool PostStrangerAlarm(const std::string &base_url, float score,
                       const std::string &note,
                       const std::vector<std::string> &jpeg_paths) {
  if (jpeg_paths.empty()) {
    return false;
  }
  const std::string url = base_url + "/api/alarms";

#ifdef HAVE_CURL
  CURL *curl = curl_easy_init();
  if (!curl) {
    return false;
  }
  curl_mimepart *part;

  /* 服务一次收一张图；多张则逐张 POST */
  bool ok_all = true;
  for (size_t i = 0; i < jpeg_paths.size(); ++i) {
    curl_mime *one = curl_mime_init(curl);

    part = curl_mime_addpart(one);
    curl_mime_name(part, "type");
    curl_mime_data(part, "stranger", CURL_ZERO_TERMINATED);

    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.3f", score);
    part = curl_mime_addpart(one);
    curl_mime_name(part, "score");
    curl_mime_data(part, buf, CURL_ZERO_TERMINATED);

    std::ostringstream note_i;
    note_i << note << " shot=" << (i + 1);
    part = curl_mime_addpart(one);
    curl_mime_name(part, "note");
    curl_mime_data(part, note_i.str().c_str(), CURL_ZERO_TERMINATED);

    part = curl_mime_addpart(one);
    curl_mime_name(part, "image");
    curl_mime_filedata(part, jpeg_paths[i].c_str());
    curl_mime_filename(part, "capture.jpg");
    curl_mime_type(part, "image/jpeg");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, one);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    CURLcode res = curl_easy_perform(curl);
    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    curl_mime_free(one);
    if (res != CURLE_OK || code < 200 || code >= 300) {
      std::fprintf(stderr, "POST fail shot %zu curl=%d http=%ld\n", i + 1,
                   (int)res, code);
      ok_all = false;
    } else {
      std::printf("POST ok shot %zu http=%ld\n", i + 1, code);
    }
  }
  curl_easy_cleanup(curl);
  return ok_all;
#else
  /* 无 libcurl：调用系统 curl 命令（板子通常有） */
  bool ok_all = true;
  for (size_t i = 0; i < jpeg_paths.size(); ++i) {
    char cmd[1024];
    std::snprintf(
        cmd, sizeof(cmd),
        "curl -sS -X POST '%s' -F type=stranger -F score=%.3f "
        "-F 'note=%s shot=%zu' -F image=@'%s' >/tmp/alarm_post.json",
        url.c_str(), score, note.c_str(), i + 1, jpeg_paths[i].c_str());
    int rc = std::system(cmd);
    if (rc != 0) {
      std::fprintf(stderr, "curl cmd fail rc=%d\n", rc);
      ok_all = false;
    } else {
      std::printf("POST via curl shot %zu\n", i + 1);
    }
  }
  return ok_all;
#endif
}
