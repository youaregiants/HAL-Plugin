#pragma once
#include <functional>
#include <string>

struct HALHTTPClient
{
  using Callback = std::function<void(bool success, std::string body)>;

  static void generate(const std::string& prompt,
                       const std::string& style,
                       const std::string& nudge,
                       const std::string& currentPatchJSON,
                       Callback callback);
};
