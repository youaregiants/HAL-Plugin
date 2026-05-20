#import "HALHTTPClient.h"
#import <Foundation/Foundation.h>

static NSString* const kServerURL = @"https://62-238-41-219.sslip.io/api/generate-patch";
static NSString* const kAPIKey    = @"bd639f6ed47b51a5acfd42960abde05a0a995913e13d092de3fa424c11a545ba";

void HALHTTPClient::generate(const std::string& prompt,
                              const std::string& style,
                              const std::string& nudge,
                              const std::string& currentPatchJSON,
                              Callback callback)
{
  NSString* nsPrompt = [NSString stringWithUTF8String:prompt.c_str()];
  NSString* nsStyle  = [NSString stringWithUTF8String:style.c_str()];

  NSMutableDictionary* body = [NSMutableDictionary dictionary];
  body[@"description"] = nsPrompt;
  body[@"style_hint"]  = nsStyle.length > 0 ? (id)nsStyle : [NSNull null];

  if (!nudge.empty()) {
    body[@"nudge"] = [NSString stringWithUTF8String:nudge.c_str()];
  }

  if (!currentPatchJSON.empty()) {
    NSData* patchData = [[NSString stringWithUTF8String:currentPatchJSON.c_str()]
                          dataUsingEncoding:NSUTF8StringEncoding];
    NSError* parseErr = nil;
    id patchObj = [NSJSONSerialization JSONObjectWithData:patchData options:0 error:&parseErr];
    if (patchObj && !parseErr)
      body[@"current_patch"] = patchObj;
  }

  NSError* jsonErr = nil;
  NSData* bodyData = [NSJSONSerialization dataWithJSONObject:body options:0 error:&jsonErr];
  if (!bodyData) {
    callback(false, "JSON serialization error");
    return;
  }

  NSURL* url = [NSURL URLWithString:kServerURL];
  NSMutableURLRequest* req = [NSMutableURLRequest requestWithURL:url
                                                     cachePolicy:NSURLRequestReloadIgnoringLocalCacheData
                                                 timeoutInterval:30.0];
  req.HTTPMethod = @"POST";
  req.HTTPBody   = bodyData;
  [req setValue:@"application/json" forHTTPHeaderField:@"Content-Type"];
  [req setValue:[@"Bearer " stringByAppendingString:kAPIKey] forHTTPHeaderField:@"Authorization"];

  NSURLSessionDataTask* task =
    [[NSURLSession sharedSession] dataTaskWithRequest:req
                                    completionHandler:^(NSData* data, NSURLResponse* resp, NSError* err) {
      if (err || !data) {
        std::string msg = err ? [err.localizedDescription UTF8String] : "No data";
        callback(false, msg);
        return;
      }

      NSHTTPURLResponse* http = (NSHTTPURLResponse*)resp;
      if (http.statusCode < 200 || http.statusCode >= 300) {
        std::string msg = "HTTP " + std::to_string((int)http.statusCode);
        if (data.length > 0 && data.length < 512)
          msg += ": " + std::string((const char*)data.bytes, data.length);
        callback(false, msg);
        return;
      }

      std::string json((const char*)data.bytes, data.length);
      callback(true, json);
    }];

  [task resume];
}
