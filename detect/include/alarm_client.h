#ifndef ROBOT_ALARM_CLIENT_H
#define ROBOT_ALARM_CLIENT_H

#include <string>
#include <vector>

/** 向 alarms HTTP 服务 POST 陌生人报警（multipart 带图） */
bool PostStrangerAlarm(const std::string &base_url, float score,
                       const std::string &note,
                       const std::vector<std::string> &jpeg_paths);

#endif
