#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include "WarehouseSystem.h"

struct HttpRequest {
    std::string method;
    std::string path;
    std::string queryString;
    std::map<std::string, std::string> queryParams;
    std::map<std::string, std::string> headers; // lower-case header names
    std::string body;
    std::string version;

    std::string getHeader(const std::string& name, const std::string& def = "") const;
    bool hasHeader(const std::string& name) const;
};

struct HttpResponse {
    int statusCode{200};
    std::string statusText{"OK"};
    std::map<std::string, std::string> headers;
    std::string body;

    std::string serialize() const;

    static HttpResponse json(int code, const std::string& statusText, const std::string& jsonBody);
    static HttpResponse error(int code, const std::string& statusText, const std::string& errorMsg);
    static HttpResponse notFound(const std::string& errorMsg = "Resource not found");
    static HttpResponse methodNotAllowed(const std::string& allowedMethods, const std::string& path = "");
    static HttpResponse payloadTooLarge(const std::string& errorMsg = "Request body exceeds maximum allowed size (2 MB).");
    static HttpResponse unsupportedMediaType(const std::string& errorMsg = "Content-Type must be application/json.");
    static HttpResponse text(int code, const std::string& statusText, const std::string& contentType, const std::string& textBody);
};

class HttpRequestHandler {
private:
    WarehouseSystem& warehouseSystem_;
    std::mutex& mutex_;
    std::string staticDir_;

    std::string escapeJson(const std::string& s) const;
    std::string getMimeType(const std::string& path) const;
    std::string readFileContent(const std::string& filePath) const;

public:
    static constexpr size_t MAX_HEADER_SIZE = 65536;        // 64 KB
    static constexpr size_t MAX_BODY_SIZE   = 2097152;      // 2 MB

    HttpRequestHandler(WarehouseSystem& system, std::mutex& mtx, const std::string& staticDir = "web");

    // Parses raw HTTP string into HttpRequest. If parsing fails, returns false and populates errorResponse.
    bool parseRawRequest(const std::string& rawRequest, HttpRequest& req, HttpResponse& errorResponse) const;

    // Dispatches a parsed request to the appropriate route handler
    HttpResponse handleRequest(const HttpRequest& req);

    // End-to-end processing of a raw HTTP request string
    std::string handleRawRequest(const std::string& rawRequest);

private:
    // Route handlers
    HttpResponse handleGetStatus(const HttpRequest& req);
    HttpResponse handlePostStep(const HttpRequest& req);
    HttpResponse handlePostOrder(const HttpRequest& req);
    HttpResponse handleGetCompare(const HttpRequest& req);
    HttpResponse handlePostObstacleAdd(const HttpRequest& req);
    HttpResponse handlePostObstacleRemove(const HttpRequest& req);
    HttpResponse handlePostReset(const HttpRequest& req);
    HttpResponse handlePostHeatmapReset(const HttpRequest& req);
    HttpResponse handlePostAnalyticsReset(const HttpRequest& req);
    HttpResponse handleGetBenchmarkSuite(const HttpRequest& req);
    HttpResponse handlePostBenchmarkRun(const HttpRequest& req);
    HttpResponse handleGetReportCsv(const HttpRequest& req);
    HttpResponse handleGetReportHtml(const HttpRequest& req);
    HttpResponse handlePostSnapshotSave(const HttpRequest& req);
    HttpResponse handlePostSnapshotLoad(const HttpRequest& req);
    HttpResponse handlePostConfigLoad(const HttpRequest& req);
    HttpResponse handleAlgorithmRun(const HttpRequest& req);
    HttpResponse handleAlgorithmCompare(const HttpRequest& req);
    HttpResponse handleAlgorithmReport(const HttpRequest& req);
    HttpResponse handleEcoRouteOptimize(const HttpRequest& req);
    HttpResponse handleEcoRouteCompare(const HttpRequest& req);
    HttpResponse handleEcoRouteAnalytics(const HttpRequest& req);
    HttpResponse handleEcoRouteRobots(const HttpRequest& req);
    HttpResponse handleEcoRouteChargingRecommend(const HttpRequest& req);
    HttpResponse handleEcoRouteReport(const HttpRequest& req);
    HttpResponse handleFoodSaverBatchesGet(const HttpRequest& req);
    HttpResponse handleFoodSaverBatchesPost(const HttpRequest& req);
    HttpResponse handleFoodSaverExpiry(const HttpRequest& req);
    HttpResponse handleFoodSaverEligibility(const HttpRequest& req);
    HttpResponse handleFoodSaverDonationsGet(const HttpRequest& req);
    HttpResponse handleFoodSaverDonationsPost(const HttpRequest& req);
    HttpResponse handleFoodSaverDonationApprove(const HttpRequest& req);
    HttpResponse handleFoodSaverDonationReject(const HttpRequest& req);
    HttpResponse handleFoodSaverDonationReserve(const HttpRequest& req);
    HttpResponse handleFoodSaverDonationReady(const HttpRequest& req);
    HttpResponse handleFoodSaverDonationComplete(const HttpRequest& req);
    HttpResponse handleFoodSaverDonationCancel(const HttpRequest& req);
    HttpResponse handleFoodSaverAnalytics(const HttpRequest& req);
    HttpResponse handleFoodSaverReport(const HttpRequest& req);
    HttpResponse handleStaticFile(const HttpRequest& req);
};

#endif // HTTP_HANDLER_H
