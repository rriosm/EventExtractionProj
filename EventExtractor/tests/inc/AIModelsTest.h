#ifndef AIMODELSTEST_H
#define AIMODELSTEST_H

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>

extern "C" {
    #include "curl/curl.h"
}


struct Response2 {
    std::string body;
    std::string headerBuffer; // Stores raw headers to find "Retry-After"
    long statusCode = 0;
};

// --- Libcurl Callbacks ---
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp)
{
    size_t totalSize = size * nmemb;
    userp->append((char*)contents, totalSize);
    return totalSize;
}

size_t HeaderCallback(char* buffer, size_t size, size_t nitems, std::string* userdata)
{
    size_t totalSize = size * nitems;
    userdata->append(buffer, totalSize);
    return totalSize;
}

// --- Helper: Case-insensitive string search ---
bool containsCaseInsensitive(const std::string& str, const std::string& sub)
{
    auto it = std::search(
        str.begin(), str.end(),
        sub.begin(), sub.end(),
        [](char ch1, char ch2) {
        auto toUpper = [](unsigned char c) {
            return (c >= 'a' && c <= 'z') ? (c - 32) : c;
        };
            return toUpper(static_cast<unsigned char>(ch1)) == toUpper(static_cast<unsigned char>(ch2));
    }
    );
    return (it != str.end());
}


// --- Helper: Parse "Retry-After" from raw headers ---
int getRetryAfterSeconds(const std::string& headerBuffer)
{
    std::string key = "Retry-After:";
    size_t pos = headerBuffer.find(key);

    if (pos != std::string::npos) {
        // Find end of line
        size_t end = headerBuffer.find("\r\n", pos);
        if (end == std::string::npos) end = headerBuffer.length();

        std::string val = headerBuffer.substr(pos + key.length(), end - (pos + key.length()));
        // Trim whitespace
        val.erase(0, val.find_first_not_of(" \t"));
        return std::stoi(val);
    }
    return 0; // Not found
}

bool sendRequestWithRetry(const std::string& url, const std::string& apiKey, const std::string& jsonPayload) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    int maxRetries = 3;
    int retryCount = 0;
    bool success = false;

    while (retryCount <= maxRetries) {
        Response2 resp;

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        std::string keyHeader = "x-goog-api-key: " + apiKey;
        headers = curl_slist_append(headers, keyHeader.c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());

        // Register Callbacks
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp.body);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &resp.headerBuffer);

        // Perform Request
        CURLcode res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp.statusCode);
        } else {
            std::cerr << "Curl Network Error: " << curl_easy_strerror(res) << std::endl;
            curl_slist_free_all(headers);
            break;
        }

        curl_slist_free_all(headers);

        // --- STATUS CODE LOGIC (The "onReplyFinished" Logic) ---

        // 1. Success (200 OK)
        if (resp.statusCode == 200) {
            std::cout << "Success: " << resp.body << std::endl;
            success = true;
            break;
        }

        // 2. Rate Limited (429)
        else if (resp.statusCode == 429) {
            std::cout << "Rate Limited (429) on attempt " << (retryCount + 1) << std::endl;

            // A. Check for "Free Tier" vs "Paid Tier" signature in body
            if (containsCaseInsensitive(resp.body, "free_tier_requests")) {
                std::cerr << "CRITICAL: Free Tier Limit Exhausted (Limit: 0)." << std::endl;
                std::cerr << "Action: Link a billing account to upgrade to Paid Tier." << std::endl;
                break; // Don't retry if the limit is strictly 0 for the day
            }

            if (containsCaseInsensitive(resp.body, "pay_as_you_go")) {
                std::cerr << "Info: Paid Tier Burst Limit Hit. Retrying..." << std::endl;
            }

            // B. Calculate Wait Time
            int waitSeconds = getRetryAfterSeconds(resp.headerBuffer);
            if (waitSeconds == 0) {
                // Fallback: Exponential Backoff (2s, 4s, 8s...)
                waitSeconds = (1 << retryCount) * 2;
            }

            std::cout << "Waiting " << waitSeconds << " seconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(waitSeconds));

            retryCount++;
        }

        // 3. Other Errors (400, 500, etc.)
        else {
            std::cerr << "HTTP Error " << resp.statusCode << ": " << resp.body << std::endl;
            break; // Do not retry fatal errors
        }
    }

    curl_easy_cleanup(curl);
    return success;
}


int mainTest() {
    // Example Usage
    std::string apiKey = "AIzaSyBFyZN06PfrZbyQpMovaVmsd2zaVPyV7oI";
    std::string url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash-lite:generateContent";
    // Using a minimal payload for testing
    std::string payload = R"({"contents":[{"parts":[{"text":"Hello"}]}]})";

    sendRequestWithRetry(url, apiKey, payload);
    return 0;
}

#endif // AIMODELSTEST_H
