#include "../include/HttpHandler.h"
#include "../include/JsonParser.h"
#include "../include/Exceptions.h"
#include "../include/FoodSaver.h"
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <iomanip>

// ============================================================================
// HttpRequest & HttpResponse Implementation
// ============================================================================

std::string HttpRequest::getHeader(const std::string& name, const std::string& def) const {
    std::string lower = name;
    for (char& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    auto it = headers.find(lower);
    return (it != headers.end()) ? it->second : def;
}

bool HttpRequest::hasHeader(const std::string& name) const {
    std::string lower = name;
    for (char& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return headers.find(lower) != headers.end();
}

std::string HttpResponse::serialize() const {
    std::ostringstream ss;
    ss << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
    for (const auto& kv : headers) {
        ss << kv.first << ": " << kv.second << "\r\n";
    }
    ss << "\r\n";
    ss << body;
    return ss.str();
}

HttpResponse HttpResponse::json(int code, const std::string& statusText, const std::string& jsonBody) {
    HttpResponse r;
    r.statusCode = code;
    r.statusText = statusText;
    r.headers["Content-Type"] = "application/json; charset=utf-8";
    r.headers["Content-Length"] = std::to_string(jsonBody.size());
    r.headers["Access-Control-Allow-Origin"] = "*";
    r.headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS";
    r.headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
    r.headers["Connection"] = "close";
    r.body = jsonBody;
    return r;
}

HttpResponse HttpResponse::text(int code, const std::string& statusText, const std::string& contentType, const std::string& textBody) {
    HttpResponse r;
    r.statusCode = code;
    r.statusText = statusText;
    r.headers["Content-Type"] = contentType;
    r.headers["Content-Length"] = std::to_string(textBody.size());
    r.headers["Access-Control-Allow-Origin"] = "*";
    r.headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS";
    r.headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
    r.headers["Connection"] = "close";
    r.body = textBody;
    return r;
}

HttpResponse HttpResponse::error(int code, const std::string& statusText, const std::string& errorMsg) {
    std::string escaped;
    for (char c : errorMsg) {
        if (c == '"') escaped += "\\\"";
        else if (c == '\\') escaped += "\\\\";
        else if (c == '\n') escaped += "\\n";
        else if (c == '\r') escaped += "\\r";
        else if (c == '\t') escaped += "\\t";
        else escaped += c;
    }
    return HttpResponse::json(code, statusText, "{\"success\":false,\"error\":\"" + escaped + "\"}");
}

HttpResponse HttpResponse::notFound(const std::string& errorMsg) {
    return HttpResponse::error(404, "Not Found", errorMsg);
}

HttpResponse HttpResponse::methodNotAllowed(const std::string& allowedMethods, const std::string& path) {
    HttpResponse r = HttpResponse::error(405, "Method Not Allowed", "Method not allowed" + (path.empty() ? "" : (" on " + path)) + ".");
    r.headers["Allow"] = allowedMethods;
    return r;
}

HttpResponse HttpResponse::payloadTooLarge(const std::string& errorMsg) {
    return HttpResponse::error(413, "Payload Too Large", errorMsg);
}

HttpResponse HttpResponse::unsupportedMediaType(const std::string& errorMsg) {
    return HttpResponse::error(415, "Unsupported Media Type", errorMsg);
}

// ============================================================================
// HttpRequestHandler Implementation
// ============================================================================

HttpRequestHandler::HttpRequestHandler(WarehouseSystem& system, std::mutex& mtx, const std::string& staticDir)
    : warehouseSystem_(system), mutex_(mtx), staticDir_(staticDir) {}

std::string HttpRequestHandler::getMimeType(const std::string& path) const {
    if (path.find(".html") != std::string::npos) return "text/html; charset=utf-8";
    if (path.find(".css") != std::string::npos) return "text/css; charset=utf-8";
    if (path.find(".js") != std::string::npos) return "application/javascript; charset=utf-8";
    if (path.find(".json") != std::string::npos) return "application/json; charset=utf-8";
    if (path.find(".png") != std::string::npos) return "image/png";
    if (path.find(".svg") != std::string::npos) return "image/svg+xml";
    return "text/plain; charset=utf-8";
}

std::string HttpRequestHandler::readFileContent(const std::string& filePath) const {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::string fallback = "../" + filePath;
        std::ifstream file2(fallback, std::ios::binary);
        if (file2.is_open()) {
            std::ostringstream ss2;
            ss2 << file2.rdbuf();
            return ss2.str();
        }
        std::string fallback2 = "D:/Robot/" + filePath;
        std::ifstream file3(fallback2, std::ios::binary);
        if (file3.is_open()) {
            std::ostringstream ss3;
            ss3 << file3.rdbuf();
            return ss3.str();
        }
        return "";
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

bool HttpRequestHandler::parseRawRequest(const std::string& rawRequest, HttpRequest& req, HttpResponse& errorResponse) const {
    if (rawRequest.empty()) {
        errorResponse = HttpResponse::error(400, "Bad Request", "Empty HTTP request received.");
        return false;
    }

    size_t headerEnd = rawRequest.find("\r\n\r\n");
    size_t delimiterLen = 4;
    if (headerEnd == std::string::npos) {
        headerEnd = rawRequest.find("\n\n");
        delimiterLen = 2;
    }

    if (headerEnd == std::string::npos) {
        errorResponse = HttpResponse::error(400, "Bad Request", "Malformed HTTP request: headers delimiter not found.");
        return false;
    }

    if (headerEnd > MAX_HEADER_SIZE) {
        errorResponse = HttpResponse::error(431, "Request Header Fields Too Large", "HTTP request headers exceed maximum size (64 KB).");
        return false;
    }

    std::string headerPart = rawRequest.substr(0, headerEnd);
    std::string bodyPart = rawRequest.substr(headerEnd + delimiterLen);

    std::istringstream stream(headerPart);
    std::string requestLine;
    if (!std::getline(stream, requestLine) || requestLine.empty()) {
        errorResponse = HttpResponse::error(400, "Bad Request", "Malformed HTTP request line.");
        return false;
    }
    if (!requestLine.empty() && requestLine.back() == '\r') {
        requestLine.pop_back();
    }

    std::istringstream reqLineStream(requestLine);
    if (!(reqLineStream >> req.method >> req.path)) {
        errorResponse = HttpResponse::error(400, "Bad Request", "Malformed HTTP request line format.");
        return false;
    }
    reqLineStream >> req.version;
    if (req.version.empty()) req.version = "HTTP/1.1";

    // Split path and query parameters
    size_t qPos = req.path.find('?');
    if (qPos != std::string::npos) {
        req.queryString = req.path.substr(qPos + 1);
        req.path = req.path.substr(0, qPos);

        std::istringstream qStream(req.queryString);
        std::string param;
        while (std::getline(qStream, param, '&')) {
            size_t eqPos = param.find('=');
            if (eqPos != std::string::npos) {
                req.queryParams[param.substr(0, eqPos)] = param.substr(eqPos + 1);
            } else if (!param.empty()) {
                req.queryParams[param] = "";
            }
        }
    }

    // Parse header fields
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string headerName = line.substr(0, colon);
            std::string headerVal = line.substr(colon + 1);

            size_t nFirst = headerName.find_first_not_of(" \t");
            size_t nLast = headerName.find_last_not_of(" \t");
            if (nFirst != std::string::npos && nLast != std::string::npos) {
                headerName = headerName.substr(nFirst, nLast - nFirst + 1);
            }
            for (char& c : headerName) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

            size_t vFirst = headerVal.find_first_not_of(" \t");
            size_t vLast = headerVal.find_last_not_of(" \t");
            if (vFirst != std::string::npos && vLast != std::string::npos) {
                headerVal = headerVal.substr(vFirst, vLast - vFirst + 1);
            } else {
                headerVal = "";
            }

            req.headers[headerName] = headerVal;
        }
    }

    // Validate Content-Length if present
    if (req.headers.find("content-length") != req.headers.end()) {
        try {
            long long cl = std::stoll(req.headers["content-length"]);
            if (cl < 0) {
                errorResponse = HttpResponse::error(400, "Bad Request", "Negative Content-Length header is invalid.");
                return false;
            }
            if (static_cast<size_t>(cl) > MAX_BODY_SIZE) {
                errorResponse = HttpResponse::payloadTooLarge("Request body exceeds maximum allowed size (2 MB).");
                return false;
            }
            if (bodyPart.size() < static_cast<size_t>(cl)) {
                errorResponse = HttpResponse::error(400, "Bad Request", "Incomplete request body: received fewer bytes than Content-Length.");
                return false;
            }
            if (bodyPart.size() > static_cast<size_t>(cl)) {
                bodyPart = bodyPart.substr(0, static_cast<size_t>(cl));
            }
        } catch (...) {
            errorResponse = HttpResponse::error(400, "Bad Request", "Invalid Content-Length header.");
            return false;
        }
    } else {
        if (bodyPart.size() > MAX_BODY_SIZE) {
            errorResponse = HttpResponse::payloadTooLarge("Request body exceeds maximum allowed size (2 MB).");
            return false;
        }
    }

    req.body = std::move(bodyPart);
    return true;
}

std::string HttpRequestHandler::handleRawRequest(const std::string& rawRequest) {
    HttpRequest req;
    HttpResponse errResp;
    if (!parseRawRequest(rawRequest, req, errResp)) {
        return errResp.serialize();
    }
    HttpResponse resp = handleRequest(req);
    return resp.serialize();
}

static bool isDonationActionPath(const std::string& path, const std::string& action, int& outId) {
    std::string prefix = "/api/foodsaver/donations/";
    if (path.rfind(prefix, 0) != 0) return false;
    std::string suffix = path.substr(prefix.length());
    size_t slashPos = suffix.find('/');
    if (slashPos == std::string::npos) return false;
    std::string idStr = suffix.substr(0, slashPos);
    std::string act = suffix.substr(slashPos + 1);
    if (act != action) return false;
    try {
        outId = std::stoi(idStr);
        return true;
    } catch (...) {
        return false;
    }
}

static int extractDonationId(const HttpRequest& req) {
    std::string prefix = "/api/foodsaver/donations/";
    if (req.path.rfind(prefix, 0) == 0) {
        std::string suffix = req.path.substr(prefix.length());
        size_t slashPos = suffix.find('/');
        if (slashPos != std::string::npos) {
            std::string idStr = suffix.substr(0, slashPos);
            try { return std::stoi(idStr); } catch (...) {}
        }
    }
    if (req.queryParams.count("id")) {
        try { return std::stoi(req.queryParams.at("id")); } catch (...) {}
    }
    if (req.queryParams.count("donationId")) {
        try { return std::stoi(req.queryParams.at("donationId")); } catch (...) {}
    }
    if (!req.body.empty()) {
        std::string err;
        JsonValue root = JsonValue::parse(req.body, err);
        if (root.isObject()) {
            if (root.hasKey("id") && root["id"].isNumber()) return root["id"].asInt();
            if (root.hasKey("donationId") && root["donationId"].isNumber()) return root["donationId"].asInt();
        }
    }
    return -1;
}

HttpResponse HttpRequestHandler::handleRequest(const HttpRequest& req) {
    // 1. CORS Preflight
    if (req.method == "OPTIONS") {
        HttpResponse res;
        res.statusCode = 204;
        res.statusText = "No Content";
        res.headers["Access-Control-Allow-Origin"] = "*";
        res.headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS";
        res.headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
        res.headers["Access-Control-Max-Age"] = "86400";
        res.headers["Connection"] = "close";
        return res;
    }

    // 2. Dispatch REST API endpoints
    if (req.path == "/api/status" || req.path == "/api/state") {
        if (req.method != "GET") return HttpResponse::methodNotAllowed("GET, OPTIONS", req.path);
        return handleGetStatus(req);
    }
    if (req.path == "/api/step") {
        if (req.method != "POST") return HttpResponse::methodNotAllowed("POST, OPTIONS", req.path);
        return handlePostStep(req);
    }
    if (req.path == "/api/order") {
        if (req.method != "POST") return HttpResponse::methodNotAllowed("POST, OPTIONS", req.path);
        return handlePostOrder(req);
    }
    if (req.path == "/api/compare") {
        if (req.method != "GET") return HttpResponse::methodNotAllowed("GET, OPTIONS", req.path);
        return handleGetCompare(req);
    }
    if (req.path == "/api/obstacle/add" || req.path == "/api/obstacles/add") {
        if (req.method != "POST") return HttpResponse::methodNotAllowed("POST, OPTIONS", req.path);
        return handlePostObstacleAdd(req);
    }
    if (req.path == "/api/obstacle/remove" || req.path == "/api/obstacles/remove") {
        if (req.method != "POST") return HttpResponse::methodNotAllowed("POST, OPTIONS", req.path);
        return handlePostObstacleRemove(req);
    }
    if (req.path == "/api/reset") {
        if (req.method != "POST") return HttpResponse::methodNotAllowed("POST, OPTIONS", req.path);
        return handlePostReset(req);
    }
    if (req.path == "/api/heatmap/reset") {
        if (req.method != "POST") return HttpResponse::methodNotAllowed("POST, OPTIONS", req.path);
        return handlePostHeatmapReset(req);
    }
    if (req.path == "/api/analytics/reset") {
        if (req.method != "POST") return HttpResponse::methodNotAllowed("POST, OPTIONS", req.path);
        return handlePostAnalyticsReset(req);
    }
    if (req.path == "/api/benchmark/suite") {
        if (req.method != "GET") return HttpResponse::methodNotAllowed("GET, OPTIONS", req.path);
        return handleGetBenchmarkSuite(req);
    }
    if (req.path == "/api/benchmark/run" || req.path == "/api/benchmark") {
        if (req.method != "POST") return HttpResponse::methodNotAllowed("POST, OPTIONS", req.path);
        return handlePostBenchmarkRun(req);
    }
    if (req.path == "/api/report/csv") {
        if (req.method != "GET") return HttpResponse::methodNotAllowed("GET, OPTIONS", req.path);
        return handleGetReportCsv(req);
    }
    if (req.path == "/api/report/html") {
        if (req.method != "GET") return HttpResponse::methodNotAllowed("GET, OPTIONS", req.path);
        return handleGetReportHtml(req);
    }
    if (req.path == "/api/snapshot/save") {
        if (req.method != "POST") return HttpResponse::methodNotAllowed("POST, OPTIONS", req.path);
        return handlePostSnapshotSave(req);
    }
    if (req.path == "/api/snapshot/load") {
        if (req.method != "POST") return HttpResponse::methodNotAllowed("POST, OPTIONS", req.path);
        return handlePostSnapshotLoad(req);
    }
    if (req.path == "/api/config/load") {
        if (req.method != "POST") return HttpResponse::methodNotAllowed("POST, OPTIONS", req.path);
        return handlePostConfigLoad(req);
    }
    if (req.path == "/api/algorithms/run" || req.path == "/api/algorithm/run") {
        if (req.method != "POST" && req.method != "GET") return HttpResponse::methodNotAllowed("GET, POST, OPTIONS", req.path);
        return handleAlgorithmRun(req);
    }
    if (req.path == "/api/algorithms/compare" || req.path == "/api/algorithm/compare") {
        if (req.method != "POST" && req.method != "GET") return HttpResponse::methodNotAllowed("GET, POST, OPTIONS", req.path);
        return handleAlgorithmCompare(req);
    }
    if (req.path == "/api/algorithms/report" || req.path == "/api/algorithm/report") {
        if (req.method != "POST" && req.method != "GET") return HttpResponse::methodNotAllowed("GET, POST, OPTIONS", req.path);
        return handleAlgorithmReport(req);
    }
    if (req.path == "/api/ecoroute/optimize") {
        if (req.method != "POST" && req.method != "GET") return HttpResponse::methodNotAllowed("GET, POST, OPTIONS", req.path);
        return handleEcoRouteOptimize(req);
    }
    if (req.path == "/api/ecoroute/compare") {
        if (req.method != "POST" && req.method != "GET") return HttpResponse::methodNotAllowed("GET, POST, OPTIONS", req.path);
        return handleEcoRouteCompare(req);
    }
    if (req.path == "/api/ecoroute/analytics") {
        if (req.method != "GET") return HttpResponse::methodNotAllowed("GET, OPTIONS", req.path);
        return handleEcoRouteAnalytics(req);
    }
    if (req.path == "/api/ecoroute/robots") {
        if (req.method != "GET") return HttpResponse::methodNotAllowed("GET, OPTIONS", req.path);
        return handleEcoRouteRobots(req);
    }
    if (req.path == "/api/ecoroute/charging/recommend" || req.path == "/api/ecoroute/charging") {
        if (req.method != "POST" && req.method != "GET") return HttpResponse::methodNotAllowed("GET, POST, OPTIONS", req.path);
        return handleEcoRouteChargingRecommend(req);
    }
    if (req.path == "/api/ecoroute/report") {
        if (req.method != "POST" && req.method != "GET") return HttpResponse::methodNotAllowed("GET, POST, OPTIONS", req.path);
        return handleEcoRouteReport(req);
    }

    // FoodSaver: Near-Expiry Inventory & Donation Optimization
    if (req.path == "/api/foodsaver/batches") {
        if (req.method == "GET") return handleFoodSaverBatchesGet(req);
        if (req.method == "POST") return handleFoodSaverBatchesPost(req);
        return HttpResponse::methodNotAllowed("GET, POST, OPTIONS", req.path);
    }
    if (req.path == "/api/foodsaver/expiry") {
        if (req.method != "GET") return HttpResponse::methodNotAllowed("GET, OPTIONS", req.path);
        return handleFoodSaverExpiry(req);
    }
    if (req.path == "/api/foodsaver/eligibility") {
        if (req.method != "GET") return HttpResponse::methodNotAllowed("GET, OPTIONS", req.path);
        return handleFoodSaverEligibility(req);
    }
    if (req.path == "/api/foodsaver/donations") {
        if (req.method == "GET") return handleFoodSaverDonationsGet(req);
        if (req.method == "POST") return handleFoodSaverDonationsPost(req);
        return HttpResponse::methodNotAllowed("GET, POST, OPTIONS", req.path);
    }
    int pathDonId = -1;
    if (req.path == "/api/foodsaver/donations/approve" || isDonationActionPath(req.path, "approve", pathDonId)) {
        if (req.method != "POST") return HttpResponse::methodNotAllowed("POST, OPTIONS", req.path);
        return handleFoodSaverDonationApprove(req);
    }
    if (req.path == "/api/foodsaver/donations/reject" || isDonationActionPath(req.path, "reject", pathDonId)) {
        if (req.method != "POST") return HttpResponse::methodNotAllowed("POST, OPTIONS", req.path);
        return handleFoodSaverDonationReject(req);
    }
    if (req.path == "/api/foodsaver/donations/reserve" || isDonationActionPath(req.path, "reserve", pathDonId)) {
        if (req.method != "POST") return HttpResponse::methodNotAllowed("POST, OPTIONS", req.path);
        return handleFoodSaverDonationReserve(req);
    }
    if (req.path == "/api/foodsaver/donations/ready" || isDonationActionPath(req.path, "ready", pathDonId)) {
        if (req.method != "POST") return HttpResponse::methodNotAllowed("POST, OPTIONS", req.path);
        return handleFoodSaverDonationReady(req);
    }
    if (req.path == "/api/foodsaver/donations/complete" || isDonationActionPath(req.path, "complete", pathDonId)) {
        if (req.method != "POST") return HttpResponse::methodNotAllowed("POST, OPTIONS", req.path);
        return handleFoodSaverDonationComplete(req);
    }
    if (req.path == "/api/foodsaver/donations/cancel" || isDonationActionPath(req.path, "cancel", pathDonId)) {
        if (req.method != "POST") return HttpResponse::methodNotAllowed("POST, OPTIONS", req.path);
        return handleFoodSaverDonationCancel(req);
    }
    if (req.path == "/api/foodsaver/analytics") {
        if (req.method != "GET") return HttpResponse::methodNotAllowed("GET, OPTIONS", req.path);
        return handleFoodSaverAnalytics(req);
    }
    if (req.path == "/api/foodsaver/report") {
        if (req.method != "GET" && req.method != "POST") return HttpResponse::methodNotAllowed("GET, POST, OPTIONS", req.path);
        return handleFoodSaverReport(req);
    }

    // 3. Catch-all for unknown /api/ endpoints -> 404 JSON
    if (req.path.rfind("/api/", 0) == 0) {
        return HttpResponse::notFound("Unknown API endpoint: " + req.path);
    }

    // 4. Static web assets
    return handleStaticFile(req);
}

// ============================================================================
// Endpoint Handlers
// ============================================================================

HttpResponse HttpRequestHandler::handleGetStatus(const HttpRequest& /*req*/) {
    std::string json;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        json = warehouseSystem_.getFullStateJSON();
    }
    return HttpResponse::json(200, "OK", json);
}

HttpResponse HttpRequestHandler::handlePostStep(const HttpRequest& /*req*/) {
    std::string json;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        warehouseSystem_.stepSimulation();
        json = warehouseSystem_.getFullStateJSON();
    }
    return HttpResponse::json(200, "OK", json);
}

HttpResponse HttpRequestHandler::handlePostOrder(const HttpRequest& req) {
    // Validate Content-Type if present
    if (req.hasHeader("content-type")) {
        std::string ct = req.getHeader("content-type");
        for (char& c : ct) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (ct.find("application/json") == std::string::npos) {
            return HttpResponse::unsupportedMediaType("Content-Type must be application/json.");
        }
    }

    if (req.body.empty()) {
        return HttpResponse::error(400, "Bad Request", "Request body cannot be empty.");
    }

    std::string parseErr;
    JsonValue root = JsonValue::parse(req.body, parseErr);
    if (!parseErr.empty()) {
        return HttpResponse::error(400, "Bad Request", "Malformed JSON request body: " + parseErr);
    }
    if (!root.isObject()) {
        return HttpResponse::error(400, "Bad Request", "JSON payload must be a JSON object.");
    }

    // Validate productId
    if (!root.hasKey("productId")) {
        return HttpResponse::error(400, "Bad Request", "Missing required field 'productId'.");
    }
    if (!root["productId"].isNumber()) {
        return HttpResponse::error(400, "Bad Request", "Field 'productId' must be an integer.");
    }
    int productId = root["productId"].asInt();
    if (productId <= 0) {
        return HttpResponse::error(400, "Bad Request", "Field 'productId' must be a positive integer.");
    }

    // Validate quantity
    if (!root.hasKey("quantity")) {
        return HttpResponse::error(400, "Bad Request", "Missing required field 'quantity'.");
    }
    if (!root["quantity"].isNumber()) {
        return HttpResponse::error(400, "Bad Request", "Field 'quantity' must be an integer.");
    }
    int quantity = root["quantity"].asInt();
    if (quantity <= 0) {
        return HttpResponse::error(400, "Bad Request", "Quantity must be greater than zero.");
    }

    // Validate priority
    OrderPriority priority = OrderPriority::MEDIUM;
    if (root.hasKey("priority")) {
        if (!root["priority"].isString()) {
            return HttpResponse::error(400, "Bad Request", "Field 'priority' must be a string.");
        }
        std::string pStr = root["priority"].asString();
        if (pStr == "HIGH") {
            priority = OrderPriority::HIGH;
        } else if (pStr == "LOW") {
            priority = OrderPriority::LOW;
        } else if (pStr == "MEDIUM") {
            priority = OrderPriority::MEDIUM;
        } else {
            return HttpResponse::error(400, "Bad Request", "Invalid priority value: '" + pStr + "'. Must be 'LOW', 'MEDIUM', or 'HIGH'.");
        }
    }

    // Mutex-protected state mutation
    std::lock_guard<std::mutex> lock(mutex_);

    // Check catalog existence
    const Product* prod = warehouseSystem_.getInventory().getProduct(productId);
    if (!prod) {
        return HttpResponse::error(400, "Bad Request", "Product ID " + std::to_string(productId) + " not found in catalog.");
    }

    // Check available stock
    if (!warehouseSystem_.getInventory().hasStock(productId, quantity)) {
        return HttpResponse::error(400, "Bad Request", "Insufficient available stock for product ID " + std::to_string(productId) + ".");
    }

    try {
        int orderId = warehouseSystem_.createOrder(productId, quantity, priority);
        std::ostringstream resp;
        resp << "{\"success\":true,\"orderId\":" << orderId << "}";
        return HttpResponse::json(200, "OK", resp.str());
    } catch (const std::exception& ex) {
        return HttpResponse::error(400, "Bad Request", ex.what());
    }
}

HttpResponse HttpRequestHandler::handleGetCompare(const HttpRequest& req) {
    int w = 15;
    int h = 15;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        w = warehouseSystem_.getWarehouse().getWidth();
        h = warehouseSystem_.getWarehouse().getHeight();
    }
    Position start(0, 0);
    Position goal(std::max(0, w - 1), std::max(0, h - 1));

    auto isNumeric = [](const std::string& str) -> bool {
        if (str.empty()) return false;
        size_t idx = 0;
        if (str[0] == '-') idx = 1;
        if (idx >= str.size()) return false;
        for (; idx < str.size(); ++idx) {
            if (!std::isdigit(static_cast<unsigned char>(str[idx]))) return false;
        }
        return true;
    };

    if (req.queryParams.find("sx") != req.queryParams.end() || req.queryParams.find("sy") != req.queryParams.end()) {
        auto itX = req.queryParams.find("sx");
        auto itY = req.queryParams.find("sy");
        if (itX == req.queryParams.end() || itY == req.queryParams.end() || !isNumeric(itX->second) || !isNumeric(itY->second)) {
            return HttpResponse::error(400, "Bad Request", "Invalid start coordinates: sx and sy must be integers.");
        }
        try {
            int sx = std::stoi(itX->second);
            int sy = std::stoi(itY->second);
            if (sx < 0 || sy < 0 || sx >= w || sy >= h) {
                return HttpResponse::error(400, "Bad Request", "Start coordinates out of bounds (valid 0-" + std::to_string(std::max(0, w - 1)) + ").");
            }
            start = Position(sx, sy);
        } catch (...) {
            return HttpResponse::error(400, "Bad Request", "Invalid start coordinate values.");
        }
    }

    if (req.queryParams.find("gx") != req.queryParams.end() || req.queryParams.find("gy") != req.queryParams.end()) {
        auto itX = req.queryParams.find("gx");
        auto itY = req.queryParams.find("gy");
        if (itX == req.queryParams.end() || itY == req.queryParams.end() || !isNumeric(itX->second) || !isNumeric(itY->second)) {
            return HttpResponse::error(400, "Bad Request", "Invalid goal coordinates: gx and gy must be integers.");
        }
        try {
            int gx = std::stoi(itX->second);
            int gy = std::stoi(itY->second);
            if (gx < 0 || gy < 0 || gx >= w || gy >= h) {
                return HttpResponse::error(400, "Bad Request", "Goal coordinates out of bounds (valid 0-" + std::to_string(std::max(0, w - 1)) + ").");
            }
            goal = Position(gx, gy);
        } catch (...) {
            return HttpResponse::error(400, "Bad Request", "Invalid goal coordinate values.");
        }
    }

    std::string comp;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        comp = warehouseSystem_.comparePathAlgorithmsJSON(start, goal);
    }
    return HttpResponse::json(200, "OK", comp);
}

HttpResponse HttpRequestHandler::handlePostObstacleAdd(const HttpRequest& req) {
    if (req.hasHeader("content-type")) {
        std::string ct = req.getHeader("content-type");
        for (char& c : ct) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (ct.find("application/json") == std::string::npos) {
            return HttpResponse::unsupportedMediaType("Content-Type must be application/json.");
        }
    }

    if (req.body.empty()) {
        return HttpResponse::error(400, "Bad Request", "Request body cannot be empty.");
    }

    std::string parseErr;
    JsonValue root = JsonValue::parse(req.body, parseErr);
    if (!parseErr.empty()) {
        return HttpResponse::error(400, "Bad Request", "Malformed JSON request body: " + parseErr);
    }
    if (!root.isObject()) {
        return HttpResponse::error(400, "Bad Request", "JSON payload must be a JSON object.");
    }

    if (!root.hasKey("x")) {
        return HttpResponse::error(400, "Bad Request", "Missing required coordinate field 'x'.");
    }
    if (!root.hasKey("y")) {
        return HttpResponse::error(400, "Bad Request", "Missing required coordinate field 'y'.");
    }
    if (!root["x"].isNumber()) {
        return HttpResponse::error(400, "Bad Request", "Field 'x' must be an integer.");
    }
    if (!root["y"].isNumber()) {
        return HttpResponse::error(400, "Bad Request", "Field 'y' must be an integer.");
    }

    int x = root["x"].asInt();
    int y = root["y"].asInt();

    int w = 15;
    int h = 15;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        w = warehouseSystem_.getWarehouse().getWidth();
        h = warehouseSystem_.getWarehouse().getHeight();
    }

    if (x < 0 || y < 0 || x >= w || y >= h) {
        return HttpResponse::error(400, "Bad Request", "Coordinates out of bounds (valid 0-" + std::to_string(std::max(0, w - 1)) + ").");
    }

    std::lock_guard<std::mutex> lock(mutex_);
    bool added = warehouseSystem_.addDynamicObstacle(Position(x, y));
    if (added) {
        std::ostringstream resp;
        resp << "{\"success\":true,\"x\":" << x << ",\"y\":" << y << ",\"message\":\"Dynamic obstacle placed safely.\"}";
        return HttpResponse::json(200, "OK", resp.str());
    } else {
        return HttpResponse::error(400, "Bad Request", "Cannot place obstacle on occupied robot, station, or impassable cell.");
    }
}

HttpResponse HttpRequestHandler::handlePostObstacleRemove(const HttpRequest& req) {
    if (req.hasHeader("content-type")) {
        std::string ct = req.getHeader("content-type");
        for (char& c : ct) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (ct.find("application/json") == std::string::npos) {
            return HttpResponse::unsupportedMediaType("Content-Type must be application/json.");
        }
    }

    if (req.body.empty()) {
        return HttpResponse::error(400, "Bad Request", "Request body cannot be empty.");
    }

    std::string parseErr;
    JsonValue root = JsonValue::parse(req.body, parseErr);
    if (!parseErr.empty()) {
        return HttpResponse::error(400, "Bad Request", "Malformed JSON request body: " + parseErr);
    }
    if (!root.isObject()) {
        return HttpResponse::error(400, "Bad Request", "JSON payload must be a JSON object.");
    }

    if (!root.hasKey("x")) {
        return HttpResponse::error(400, "Bad Request", "Missing required coordinate field 'x'.");
    }
    if (!root.hasKey("y")) {
        return HttpResponse::error(400, "Bad Request", "Missing required coordinate field 'y'.");
    }
    if (!root["x"].isNumber()) {
        return HttpResponse::error(400, "Bad Request", "Field 'x' must be an integer.");
    }
    if (!root["y"].isNumber()) {
        return HttpResponse::error(400, "Bad Request", "Field 'y' must be an integer.");
    }

    int x = root["x"].asInt();
    int y = root["y"].asInt();

    int w = 15;
    int h = 15;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        w = warehouseSystem_.getWarehouse().getWidth();
        h = warehouseSystem_.getWarehouse().getHeight();
    }

    if (x < 0 || y < 0 || x >= w || y >= h) {
        return HttpResponse::error(400, "Bad Request", "Coordinates out of bounds (valid 0-" + std::to_string(std::max(0, w - 1)) + ").");
    }

    std::lock_guard<std::mutex> lock(mutex_);
    bool removed = warehouseSystem_.removeDynamicObstacle(Position(x, y));
    if (removed) {
        std::ostringstream resp;
        resp << "{\"success\":true,\"x\":" << x << ",\"y\":" << y << ",\"message\":\"Dynamic obstacle removed successfully.\"}";
        return HttpResponse::json(200, "OK", resp.str());
    } else {
        return HttpResponse::error(400, "Bad Request", "No dynamic obstacle found at specified position.");
    }
}

HttpResponse HttpRequestHandler::handlePostReset(const HttpRequest& /*req*/) {
    std::string json;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        warehouseSystem_.resetSimulation();
        json = warehouseSystem_.getFullStateJSON();
    }
    return HttpResponse::json(200, "OK", json);
}

HttpResponse HttpRequestHandler::handlePostHeatmapReset(const HttpRequest& /*req*/) {
    std::string json;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        warehouseSystem_.resetHeatmap();
        json = warehouseSystem_.getFullStateJSON();
    }
    return HttpResponse::json(200, "OK", json);
}

HttpResponse HttpRequestHandler::handlePostAnalyticsReset(const HttpRequest& /*req*/) {
    std::string json;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        warehouseSystem_.resetAnalytics();
        json = warehouseSystem_.getFullStateJSON();
    }
    return HttpResponse::json(200, "OK", json);
}

HttpResponse HttpRequestHandler::handleGetBenchmarkSuite(const HttpRequest& /*req*/) {
    std::string res;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        res = warehouseSystem_.runBenchmarkSuiteJSON();
    }
    return HttpResponse::json(200, "OK", res);
}

HttpResponse HttpRequestHandler::handlePostBenchmarkRun(const HttpRequest& req) {
    if (req.hasHeader("content-type")) {
        std::string ct = req.getHeader("content-type");
        for (char& c : ct) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (ct.find("application/json") == std::string::npos) {
            return HttpResponse::unsupportedMediaType("Content-Type must be application/json.");
        }
    }

    std::string scenario = "all";
    int repetitions = 1;
    std::vector<std::string> algorithms = {"BFS", "Dijkstra", "A*", "SpaceTimeA*"};

    if (!req.body.empty()) {
        std::string parseErr;
        JsonValue root = JsonValue::parse(req.body, parseErr);
        if (!parseErr.empty()) {
            return HttpResponse::error(400, "Bad Request", "Malformed JSON request body: " + parseErr);
        }
        if (!root.isObject()) {
            return HttpResponse::error(400, "Bad Request", "JSON payload must be a JSON object.");
        }

        if (root.hasKey("scenario")) {
            if (!root["scenario"].isString()) {
                return HttpResponse::error(400, "Bad Request", "Field 'scenario' must be a string.");
            }
            scenario = root["scenario"].asString();
            if (scenario != "all" && scenario != "open_grid" && scenario != "obstacle_rich" &&
                scenario != "narrow_corridor" && scenario != "long_distance" && scenario != "congested_multi_robot" &&
                scenario != "empty" && scenario != "warehouse" && scenario != "narrow" && scenario != "bottleneck") {
                return HttpResponse::error(400, "Bad Request", "Unknown benchmark scenario: '" + scenario + "'. Valid: open_grid (empty), obstacle_rich (warehouse), narrow_corridor (narrow), long_distance, congested_multi_robot, all.");
            }
        }

        if (root.hasKey("repetitions")) {
            if (!root["repetitions"].isNumber()) {
                return HttpResponse::error(400, "Bad Request", "Field 'repetitions' must be an integer.");
            }
            repetitions = root["repetitions"].asInt();
            if (repetitions < 1 || repetitions > 20) {
                return HttpResponse::error(400, "Bad Request", "Repetitions must be between 1 and 20.");
            }
        }

        if (root.hasKey("algorithms")) {
            if (!root["algorithms"].isArray()) {
                return HttpResponse::error(400, "Bad Request", "Field 'algorithms' must be an array of strings.");
            }
            const auto& arr = root["algorithms"].asArray();
            if (arr.empty()) {
                return HttpResponse::error(400, "Bad Request", "Algorithms array cannot be empty.");
            }
            algorithms.clear();
            for (const auto& item : arr) {
                if (!item.isString()) {
                    return HttpResponse::error(400, "Bad Request", "Algorithms array elements must be strings.");
                }
                std::string alg = item.asString();
                if (alg != "BFS" && alg != "Dijkstra" && alg != "A*" && alg != "SpaceTimeA*" && alg != "Space-Time A*") {
                    return HttpResponse::error(400, "Bad Request", "Unknown algorithm in benchmark request: '" + alg + "'. Valid: BFS, Dijkstra, A*, SpaceTimeA*.");
                }
                algorithms.push_back(alg);
            }
        }
    }

    std::string canonicalScenario = scenario;
    if (scenario == "empty") canonicalScenario = "open_grid";
    else if (scenario == "warehouse") canonicalScenario = "obstacle_rich";
    else if (scenario == "narrow" || scenario == "bottleneck") canonicalScenario = "narrow_corridor";

    std::string res;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        res = warehouseSystem_.runCustomBenchmarkJSON(canonicalScenario, repetitions, algorithms);
    }

    int statusCode = (res.find("\"success\":false") != std::string::npos) ? 400 : 200;
    std::string statusText = (statusCode == 200) ? "OK" : "Bad Request";
    return HttpResponse::json(statusCode, statusText, res);
}

HttpResponse HttpRequestHandler::handleGetReportCsv(const HttpRequest& /*req*/) {
    std::string csv;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        csv = warehouseSystem_.generateCSVReport();
    }
    return HttpResponse::text(200, "OK", "text/csv; charset=utf-8", csv);
}

HttpResponse HttpRequestHandler::handleGetReportHtml(const HttpRequest& /*req*/) {
    std::string html;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        html = warehouseSystem_.generateHTMLReport();
    }
    return HttpResponse::text(200, "OK", "text/html; charset=utf-8", html);
}

HttpResponse HttpRequestHandler::handlePostSnapshotSave(const HttpRequest& req) {
    if (req.hasHeader("content-type")) {
        std::string ct = req.getHeader("content-type");
        for (char& c : ct) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (ct.find("application/json") == std::string::npos) {
            return HttpResponse::unsupportedMediaType("Content-Type must be application/json.");
        }
    }

    if (req.body.empty()) {
        return HttpResponse::error(400, "Bad Request", "Request body cannot be empty.");
    }

    std::string parseErr;
    JsonValue root = JsonValue::parse(req.body, parseErr);
    if (!parseErr.empty()) {
        return HttpResponse::error(400, "Bad Request", "Malformed JSON request body: " + parseErr);
    }
    if (!root.isObject() || !root.hasKey("filepath") || !root["filepath"].isString()) {
        return HttpResponse::error(400, "Bad Request", "Missing required string field 'filepath'.");
    }

    std::string filepath = root["filepath"].asString();
    if (filepath.empty()) {
        return HttpResponse::error(400, "Bad Request", "Field 'filepath' cannot be empty.");
    }

    std::string errMsg;
    bool ok = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ok = warehouseSystem_.saveToFile(filepath, errMsg);
    }

    if (ok) {
        return HttpResponse::json(200, "OK", "{\"success\":true,\"message\":\"Snapshot saved successfully to " + filepath + "\"}");
    } else {
        return HttpResponse::error(400, "Bad Request", errMsg.empty() ? ("Failed to save snapshot to " + filepath) : errMsg);
    }
}

HttpResponse HttpRequestHandler::handlePostSnapshotLoad(const HttpRequest& req) {
    if (req.hasHeader("content-type")) {
        std::string ct = req.getHeader("content-type");
        for (char& c : ct) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (ct.find("application/json") == std::string::npos) {
            return HttpResponse::unsupportedMediaType("Content-Type must be application/json.");
        }
    }

    if (req.body.empty()) {
        return HttpResponse::error(400, "Bad Request", "Request body cannot be empty.");
    }

    std::string parseErr;
    JsonValue root = JsonValue::parse(req.body, parseErr);
    if (!parseErr.empty()) {
        return HttpResponse::error(400, "Bad Request", "Malformed JSON request body: " + parseErr);
    }
    if (!root.isObject() || !root.hasKey("filepath") || !root["filepath"].isString()) {
        return HttpResponse::error(400, "Bad Request", "Missing required string field 'filepath'.");
    }

    std::string filepath = root["filepath"].asString();
    if (filepath.empty()) {
        return HttpResponse::error(400, "Bad Request", "Field 'filepath' cannot be empty.");
    }

    std::string errMsg;
    bool ok = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ok = warehouseSystem_.loadFromFile(filepath, errMsg);
    }

    if (ok) {
        return HttpResponse::json(200, "OK", "{\"success\":true,\"message\":\"Snapshot loaded successfully from " + filepath + "\"}");
    } else {
        return HttpResponse::error(400, "Bad Request", errMsg.empty() ? ("Failed to load snapshot from " + filepath) : errMsg);
    }
}

HttpResponse HttpRequestHandler::handlePostConfigLoad(const HttpRequest& req) {
    if (req.hasHeader("content-type")) {
        std::string ct = req.getHeader("content-type");
        for (char& c : ct) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (ct.find("application/json") == std::string::npos) {
            return HttpResponse::unsupportedMediaType("Content-Type must be application/json.");
        }
    }

    if (req.body.empty()) {
        return HttpResponse::error(400, "Bad Request", "Request body cannot be empty.");
    }

    std::string parseErr;
    JsonValue root = JsonValue::parse(req.body, parseErr);
    if (!parseErr.empty()) {
        return HttpResponse::error(400, "Bad Request", "Malformed JSON request body: " + parseErr);
    }
    if (!root.isObject()) {
        return HttpResponse::error(400, "Bad Request", "JSON payload must be a JSON object.");
    }

    if (!root.hasKey("layoutPath") || !root["layoutPath"].isString() ||
        !root.hasKey("productsPath") || !root["productsPath"].isString() ||
        !root.hasKey("robotsPath") || !root["robotsPath"].isString()) {
        return HttpResponse::error(400, "Bad Request", "Missing required configuration path fields: layoutPath, productsPath, robotsPath.");
    }

    std::string layout = root["layoutPath"].asString();
    std::string products = root["productsPath"].asString();
    std::string robots = root["robotsPath"].asString();

    std::string errMsg;
    bool ok = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ok = warehouseSystem_.loadConfiguration(layout, products, robots, errMsg);
    }

    if (ok) {
        return HttpResponse::json(200, "OK", "{\"success\":true,\"message\":\"Configuration loaded successfully.\"}");
    } else {
        return HttpResponse::error(400, "Bad Request", errMsg.empty() ? "Failed to load configuration." : errMsg);
    }
}

static bool parseLabParams(const HttpRequest& req,
                          int gridW, int gridH,
                          std::string& algorithm,
                          Position& start,
                          Position& goal,
                          std::vector<Position>& customObstacles,
                          std::vector<Position>& removedObstacles,
                          std::vector<std::pair<Position, double>>& customWeights,
                          std::string& format,
                          std::string& errorMsg)
{
    algorithm = "A*";
    start = Position(0, 0);
    goal = Position(std::max(0, gridW - 1), std::max(0, gridH - 1));
    format = "json";

    auto isNumeric = [](const std::string& str) -> bool {
        if (str.empty()) return false;
        size_t idx = 0;
        if (str[0] == '-') idx = 1;
        if (idx >= str.size()) return false;
        for (; idx < str.size(); ++idx) {
            if (!std::isdigit(static_cast<unsigned char>(str[idx]))) return false;
        }
        return true;
    };

    // 1. Query parameters
    if (req.queryParams.find("algorithm") != req.queryParams.end()) {
        algorithm = req.queryParams.at("algorithm");
    } else if (req.queryParams.find("algo") != req.queryParams.end()) {
        algorithm = req.queryParams.at("algo");
    }

    if (req.queryParams.find("format") != req.queryParams.end()) {
        format = req.queryParams.at("format");
    }

    if (req.queryParams.find("sx") != req.queryParams.end() && req.queryParams.find("sy") != req.queryParams.end()) {
        std::string sxStr = req.queryParams.at("sx");
        std::string syStr = req.queryParams.at("sy");
        if (isNumeric(sxStr) && isNumeric(syStr)) {
            start = Position(std::stoi(sxStr), std::stoi(syStr));
        } else {
            errorMsg = "Invalid start coordinates: sx and sy must be integers.";
            return false;
        }
    }

    if (req.queryParams.find("gx") != req.queryParams.end() && req.queryParams.find("gy") != req.queryParams.end()) {
        std::string gxStr = req.queryParams.at("gx");
        std::string gyStr = req.queryParams.at("gy");
        if (isNumeric(gxStr) && isNumeric(gyStr)) {
            goal = Position(std::stoi(gxStr), std::stoi(gyStr));
        } else {
            errorMsg = "Invalid goal coordinates: gx and gy must be integers.";
            return false;
        }
    }

    // 2. JSON Body if present
    if (!req.body.empty()) {
        std::string jsonErr;
        JsonValue root = JsonValue::parse(req.body, jsonErr);
        if (root.isObject()) {
            if (root.hasKey("algorithm") && root["algorithm"].isString()) {
                algorithm = root["algorithm"].asString();
            } else if (root.hasKey("algo") && root["algo"].isString()) {
                algorithm = root["algo"].asString();
            }

            if (root.hasKey("format") && root["format"].isString()) {
                format = root["format"].asString();
            }

            if (root.hasKey("start") && root["start"].isObject()) {
                const auto& sObj = root["start"];
                if (sObj.hasKey("x") && sObj.hasKey("y")) {
                    start = Position(sObj["x"].asInt(), sObj["y"].asInt());
                }
            } else if (root.hasKey("sx") && root.hasKey("sy")) {
                start = Position(root["sx"].asInt(), root["sy"].asInt());
            }

            if (root.hasKey("goal") && root["goal"].isObject()) {
                const auto& gObj = root["goal"];
                if (gObj.hasKey("x") && gObj.hasKey("y")) {
                    goal = Position(gObj["x"].asInt(), gObj["y"].asInt());
                }
            } else if (root.hasKey("gx") && root.hasKey("gy")) {
                goal = Position(root["gx"].asInt(), root["gy"].asInt());
            }

            if (root.hasKey("customObstacles") && root["customObstacles"].isArray()) {
                for (const auto& item : root["customObstacles"].asArray()) {
                    if (item.isObject() && item.hasKey("x") && item.hasKey("y")) {
                        customObstacles.push_back(Position(item["x"].asInt(), item["y"].asInt()));
                    }
                }
            }

            if (root.hasKey("removedObstacles") && root["removedObstacles"].isArray()) {
                for (const auto& item : root["removedObstacles"].asArray()) {
                    if (item.isObject() && item.hasKey("x") && item.hasKey("y")) {
                        removedObstacles.push_back(Position(item["x"].asInt(), item["y"].asInt()));
                    }
                }
            }

            if (root.hasKey("customWeights") && root["customWeights"].isArray()) {
                for (const auto& item : root["customWeights"].asArray()) {
                    if (item.isObject() && item.hasKey("x") && item.hasKey("y")) {
                        double cost = item.hasKey("cost") ? item["cost"].asDouble(1.0) : 1.0;
                        customWeights.push_back({Position(item["x"].asInt(), item["y"].asInt()), cost});
                    }
                }
            }
        }
    }

    if (start.x < 0 || start.y < 0 || start.x >= gridW || start.y >= gridH) {
        errorMsg = "Start coordinates out of bounds (valid 0-" + std::to_string(std::max(0, gridW - 1)) + ").";
        return false;
    }
    if (goal.x < 0 || goal.y < 0 || goal.x >= gridW || goal.y >= gridH) {
        errorMsg = "Goal coordinates out of bounds (valid 0-" + std::to_string(std::max(0, gridW - 1)) + ").";
        return false;
    }

    return true;
}

HttpResponse HttpRequestHandler::handleAlgorithmRun(const HttpRequest& req) {
    int w = 15;
    int h = 15;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        w = warehouseSystem_.getWarehouse().getWidth();
        h = warehouseSystem_.getWarehouse().getHeight();
    }

    std::string algorithm;
    Position start(0, 0);
    Position goal(0, 0);
    std::vector<Position> customObstacles;
    std::vector<Position> removedObstacles;
    std::vector<std::pair<Position, double>> customWeights;
    std::string format;
    std::string errorMsg;

    if (!parseLabParams(req, w, h, algorithm, start, goal, customObstacles, removedObstacles, customWeights, format, errorMsg)) {
        return HttpResponse::error(400, "Bad Request", errorMsg);
    }

    std::string resultJson;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        resultJson = warehouseSystem_.runAlgorithmLabJSON(algorithm, start, goal, customObstacles, removedObstacles, customWeights);
    }

    return HttpResponse::json(200, "OK", resultJson);
}

HttpResponse HttpRequestHandler::handleAlgorithmCompare(const HttpRequest& req) {
    int w = 15;
    int h = 15;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        w = warehouseSystem_.getWarehouse().getWidth();
        h = warehouseSystem_.getWarehouse().getHeight();
    }

    std::string algorithm;
    Position start(0, 0);
    Position goal(0, 0);
    std::vector<Position> customObstacles;
    std::vector<Position> removedObstacles;
    std::vector<std::pair<Position, double>> customWeights;
    std::string format;
    std::string errorMsg;

    if (!parseLabParams(req, w, h, algorithm, start, goal, customObstacles, removedObstacles, customWeights, format, errorMsg)) {
        return HttpResponse::error(400, "Bad Request", errorMsg);
    }

    std::string resultJson;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        resultJson = warehouseSystem_.compareAlgorithmsLabJSON(start, goal, customObstacles, removedObstacles, customWeights);
    }

    return HttpResponse::json(200, "OK", resultJson);
}

HttpResponse HttpRequestHandler::handleAlgorithmReport(const HttpRequest& req) {
    int w = 15;
    int h = 15;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        w = warehouseSystem_.getWarehouse().getWidth();
        h = warehouseSystem_.getWarehouse().getHeight();
    }

    std::string algorithm;
    Position start(0, 0);
    Position goal(0, 0);
    std::vector<Position> customObstacles;
    std::vector<Position> removedObstacles;
    std::vector<std::pair<Position, double>> customWeights;
    std::string format;
    std::string errorMsg;

    if (!parseLabParams(req, w, h, algorithm, start, goal, customObstacles, removedObstacles, customWeights, format, errorMsg)) {
        return HttpResponse::error(400, "Bad Request", errorMsg);
    }

    std::string report;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        report = warehouseSystem_.generateAlgorithmReport(start, goal, format, customObstacles, removedObstacles, customWeights);
    }

    if (format == "csv") {
        return HttpResponse::text(200, "OK", "text/csv; charset=utf-8", report);
    }
    return HttpResponse::json(200, "OK", report);
}

static bool parseEcoRouteParams(const HttpRequest& req,
                                int gridW, int gridH,
                                Position& start, Position& goal,
                                std::string& robotType,
                                int& payload,
                                std::string& format,
                                std::string& robotId,
                                std::string& errorMsg) {
    start = Position(0, 0);
    goal = Position(gridW - 1, gridH - 1);
    robotType = "PickerRobot";
    payload = 0;
    format = "json";
    robotId = "";

    // 1. Query parameters
    if (req.queryParams.count("start_x") && req.queryParams.count("start_y")) {
        try {
            start = Position(std::stoi(req.queryParams.at("start_x")), std::stoi(req.queryParams.at("start_y")));
        } catch (...) {
            errorMsg = "Invalid query parameter for start position.";
            return false;
        }
    } else if (req.queryParams.count("sx") && req.queryParams.count("sy")) {
        try {
            start = Position(std::stoi(req.queryParams.at("sx")), std::stoi(req.queryParams.at("sy")));
        } catch (...) {
            errorMsg = "Invalid query parameter for start position.";
            return false;
        }
    }

    if (req.queryParams.count("goal_x") && req.queryParams.count("goal_y")) {
        try {
            goal = Position(std::stoi(req.queryParams.at("goal_x")), std::stoi(req.queryParams.at("goal_y")));
        } catch (...) {
            errorMsg = "Invalid query parameter for goal position.";
            return false;
        }
    } else if (req.queryParams.count("gx") && req.queryParams.count("gy")) {
        try {
            goal = Position(std::stoi(req.queryParams.at("gx")), std::stoi(req.queryParams.at("gy")));
        } catch (...) {
            errorMsg = "Invalid query parameter for goal position.";
            return false;
        }
    }

    if (req.queryParams.count("robotType")) robotType = req.queryParams.at("robotType");
    else if (req.queryParams.count("type")) robotType = req.queryParams.at("type");

    if (req.queryParams.count("payload")) {
        try { payload = std::stoi(req.queryParams.at("payload")); } catch (...) {}
    }

    if (req.queryParams.count("format")) format = req.queryParams.at("format");
    if (req.queryParams.count("robotId")) robotId = req.queryParams.at("robotId");

    // 2. JSON Body if present
    if (!req.body.empty()) {
        std::string jsonErr;
        JsonValue root = JsonValue::parse(req.body, jsonErr);
        if (root.isObject()) {
            if (root.hasKey("robotType") && root["robotType"].isString()) {
                robotType = root["robotType"].asString();
            } else if (root.hasKey("type") && root["type"].isString()) {
                robotType = root["type"].asString();
            }

            if (root.hasKey("payload") && root["payload"].isNumber()) {
                payload = root["payload"].asInt(0);
            }

            if (root.hasKey("format") && root["format"].isString()) {
                format = root["format"].asString();
            }

            if (root.hasKey("robotId") && root["robotId"].isString()) {
                robotId = root["robotId"].asString();
            }

            if (root.hasKey("start") && root["start"].isObject()) {
                const auto& sObj = root["start"];
                if (sObj.hasKey("x") && sObj.hasKey("y")) {
                    start = Position(sObj["x"].asInt(), sObj["y"].asInt());
                }
            } else if (root.hasKey("sx") && root.hasKey("sy")) {
                start = Position(root["sx"].asInt(), root["sy"].asInt());
            }

            if (root.hasKey("goal") && root["goal"].isObject()) {
                const auto& gObj = root["goal"];
                if (gObj.hasKey("x") && gObj.hasKey("y")) {
                    goal = Position(gObj["x"].asInt(), gObj["y"].asInt());
                }
            } else if (root.hasKey("gx") && root.hasKey("gy")) {
                goal = Position(root["gx"].asInt(), root["gy"].asInt());
            }

            if (root.hasKey("position") && root["position"].isObject()) {
                const auto& pObj = root["position"];
                if (pObj.hasKey("x") && pObj.hasKey("y")) {
                    start = Position(pObj["x"].asInt(), pObj["y"].asInt());
                }
            }
        }
    }

    if (start.x < 0 || start.y < 0 || start.x >= gridW || start.y >= gridH) {
        errorMsg = "Start coordinates out of bounds (valid 0-" + std::to_string(std::max(0, gridW - 1)) + ").";
        return false;
    }
    if (goal.x < 0 || goal.y < 0 || goal.x >= gridW || goal.y >= gridH) {
        errorMsg = "Goal coordinates out of bounds (valid 0-" + std::to_string(std::max(0, gridW - 1)) + ").";
        return false;
    }

    return true;
}

HttpResponse HttpRequestHandler::handleEcoRouteOptimize(const HttpRequest& req) {
    int w = 15;
    int h = 15;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        w = warehouseSystem_.getWarehouse().getWidth();
        h = warehouseSystem_.getWarehouse().getHeight();
    }

    Position start(0, 0);
    Position goal(0, 0);
    std::string robotType = "PickerRobot";
    int payload = 0;
    std::string format = "json";
    std::string robotId;
    std::string errorMsg;

    if (!parseEcoRouteParams(req, w, h, start, goal, robotType, payload, format, robotId, errorMsg)) {
        return HttpResponse::error(400, "Bad Request", errorMsg);
    }

    std::string resultJson;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        resultJson = warehouseSystem_.optimizeEcoRouteJSON(start, goal, robotType, payload);
    }

    return HttpResponse::json(200, "OK", resultJson);
}

HttpResponse HttpRequestHandler::handleEcoRouteCompare(const HttpRequest& req) {
    int w = 15;
    int h = 15;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        w = warehouseSystem_.getWarehouse().getWidth();
        h = warehouseSystem_.getWarehouse().getHeight();
    }

    Position start(0, 0);
    Position goal(0, 0);
    std::string robotType = "PickerRobot";
    int payload = 0;
    std::string format = "json";
    std::string robotId;
    std::string errorMsg;

    if (!parseEcoRouteParams(req, w, h, start, goal, robotType, payload, format, robotId, errorMsg)) {
        return HttpResponse::error(400, "Bad Request", errorMsg);
    }

    std::string resultJson;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        resultJson = warehouseSystem_.compareEcoRoutesJSON(start, goal, robotType, payload);
    }

    return HttpResponse::json(200, "OK", resultJson);
}

HttpResponse HttpRequestHandler::handleEcoRouteAnalytics(const HttpRequest& /*req*/) {
    std::string resultJson;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        resultJson = warehouseSystem_.getEcoAnalyticsJSON();
    }
    return HttpResponse::json(200, "OK", resultJson);
}

HttpResponse HttpRequestHandler::handleEcoRouteRobots(const HttpRequest& /*req*/) {
    std::string resultJson;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        resultJson = warehouseSystem_.getEcoRobotsJSON();
    }
    return HttpResponse::json(200, "OK", resultJson);
}

HttpResponse HttpRequestHandler::handleEcoRouteChargingRecommend(const HttpRequest& req) {
    Position start(-1, -1);
    std::string robotId;

    // Check query or body for robotId or position
    if (req.queryParams.count("robotId")) robotId = req.queryParams.at("robotId");
    if (req.queryParams.count("id")) robotId = req.queryParams.at("id");

    if (req.queryParams.count("x") && req.queryParams.count("y")) {
        try {
            start = Position(std::stoi(req.queryParams.at("x")), std::stoi(req.queryParams.at("y")));
        } catch (...) {}
    }

    if (!req.body.empty()) {
        std::string jsonErr;
        JsonValue root = JsonValue::parse(req.body, jsonErr);
        if (root.isObject()) {
            if (root.hasKey("robotId") && root["robotId"].isString()) robotId = root["robotId"].asString();
            else if (root.hasKey("id") && root["id"].isString()) robotId = root["id"].asString();

            if (root.hasKey("position") && root["position"].isObject()) {
                const auto& pObj = root["position"];
                if (pObj.hasKey("x") && pObj.hasKey("y")) {
                    start = Position(pObj["x"].asInt(), pObj["y"].asInt());
                }
            } else if (root.hasKey("x") && root.hasKey("y")) {
                start = Position(root["x"].asInt(), root["y"].asInt());
            }
        }
    }

    std::string resultJson;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        resultJson = warehouseSystem_.recommendChargingStationJSON(robotId, start);
    }

    return HttpResponse::json(200, "OK", resultJson);
}

HttpResponse HttpRequestHandler::handleEcoRouteReport(const HttpRequest& req) {
    int w = 15;
    int h = 15;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        w = warehouseSystem_.getWarehouse().getWidth();
        h = warehouseSystem_.getWarehouse().getHeight();
    }

    Position start(0, 0);
    Position goal(0, 0);
    std::string robotType = "PickerRobot";
    int payload = 0;
    std::string format = "json";
    std::string robotId;
    std::string errorMsg;

    if (!parseEcoRouteParams(req, w, h, start, goal, robotType, payload, format, robotId, errorMsg)) {
        return HttpResponse::error(400, "Bad Request", errorMsg);
    }

    std::string report;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        report = warehouseSystem_.generateEcoReport(start, goal, robotType, payload, format);
    }

    if (format == "csv") {
        return HttpResponse::text(200, "OK", "text/csv; charset=utf-8", report);
    }
    return HttpResponse::json(200, "OK", report);
}

// ============================================================================
// FoodSaver: Route Handlers
// ============================================================================

HttpResponse HttpRequestHandler::handleFoodSaverBatchesGet(const HttpRequest& /*req*/) {
    std::string json;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        json = warehouseSystem_.getFoodBatchesJSON();
    }
    return HttpResponse::json(200, "OK", json);
}

HttpResponse HttpRequestHandler::handleFoodSaverBatchesPost(const HttpRequest& req) {
    if (req.body.empty()) {
        return HttpResponse::error(400, "Bad Request", "Request body cannot be empty.");
    }
    std::string parseErr;
    JsonValue root = JsonValue::parse(req.body, parseErr);
    if (!parseErr.empty() || !root.isObject()) {
        return HttpResponse::error(400, "Bad Request", "Malformed JSON request body: " + parseErr);
    }
    if (!root.hasKey("batchId") || !root["batchId"].isString() || root["batchId"].asString().empty()) {
        return HttpResponse::error(400, "Bad Request", "Missing or invalid 'batchId'.");
    }
    if (!root.hasKey("productId") || !root["productId"].isNumber() || root["productId"].asInt() <= 0) {
        return HttpResponse::error(400, "Bad Request", "Missing or invalid 'productId' (must be > 0).");
    }
    if (!root.hasKey("quantity") || !root["quantity"].isNumber() || root["quantity"].asInt() <= 0) {
        return HttpResponse::error(400, "Bad Request", "Missing or invalid 'quantity' (must be > 0).");
    }
    if (!root.hasKey("expiryDate") || !root["expiryDate"].isString()) {
        return HttpResponse::error(400, "Bad Request", "Missing or invalid 'expiryDate'.");
    }
    std::string expStr = root["expiryDate"].asString();
    Date exp = Date::fromString(expStr);
    if (!exp.isValid()) {
        return HttpResponse::error(400, "Bad Request", "Invalid expiry date format. Expected YYYY-MM-DD.");
    }

    std::string mfgStr = root.hasKey("manufacturingDate") && root["manufacturingDate"].isString() ? root["manufacturingDate"].asString() : "";
    Date mfg;
    if (!mfgStr.empty()) {
        mfg = Date::fromString(mfgStr);
        if (!mfg.isValid()) {
            return HttpResponse::error(400, "Bad Request", "Invalid manufacturing date format. Expected YYYY-MM-DD.");
        }
        if (exp < mfg) {
            return HttpResponse::error(400, "Bad Request", "Manufacturing date cannot be later than expiry date.");
        }
    } else {
        mfg = exp;
    }

    InventoryBatch batch;
    batch.batchId = root["batchId"].asString();
    batch.productId = root["productId"].asInt();
    batch.productName = root.hasKey("productName") && root["productName"].isString() ? root["productName"].asString() : ("Product " + std::to_string(batch.productId));
    batch.quantity = root["quantity"].asInt();
    batch.category = root.hasKey("category") && root["category"].isString() ? root["category"].asString() : "General";
    batch.unit = root.hasKey("unit") && root["unit"].isString() ? root["unit"].asString() : "units";
    batch.manufacturingDate = mfg;
    batch.expiryDate = exp;
    if (root.hasKey("storageLocation") && root["storageLocation"].isObject()) {
        const auto& sl = root["storageLocation"];
        batch.storageLocation = Position(sl.hasKey("x") ? sl["x"].asInt() : 3, sl.hasKey("y") ? sl["y"].asInt() : 4);
    } else {
        batch.storageLocation = Position(3, 4);
    }

    std::string addErr;
    bool ok = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ok = warehouseSystem_.getFoodSaver().addBatch(batch, addErr);
    }
    if (!ok) {
        return HttpResponse::error(400, "Bad Request", addErr);
    }
    return HttpResponse::json(201, "Created", "{\"success\":true,\"message\":\"Batch added successfully\",\"batch\":" + batch.toJSON() + "}");
}

HttpResponse HttpRequestHandler::handleFoodSaverExpiry(const HttpRequest& /*req*/) {
    std::string json;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        json = warehouseSystem_.getNearExpiryJSON();
    }
    return HttpResponse::json(200, "OK", json);
}

HttpResponse HttpRequestHandler::handleFoodSaverEligibility(const HttpRequest& /*req*/) {
    std::string json;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        json = warehouseSystem_.getDonationEligibilityJSON();
    }
    return HttpResponse::json(200, "OK", json);
}

HttpResponse HttpRequestHandler::handleFoodSaverDonationsGet(const HttpRequest& /*req*/) {
    std::string json;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        json = warehouseSystem_.getDonationsJSON();
    }
    return HttpResponse::json(200, "OK", json);
}

HttpResponse HttpRequestHandler::handleFoodSaverDonationsPost(const HttpRequest& req) {
    if (req.body.empty()) {
        return HttpResponse::error(400, "Bad Request", "Request body cannot be empty.");
    }
    std::string parseErr;
    JsonValue root = JsonValue::parse(req.body, parseErr);
    if (!parseErr.empty() || !root.isObject()) {
        return HttpResponse::error(400, "Bad Request", "Malformed JSON request body: " + parseErr);
    }

    std::string recipient = root.hasKey("recipient") && root["recipient"].isString() ? root["recipient"].asString() : "City Food Bank";
    std::string contact = root.hasKey("contact") && root["contact"].isString() ? root["contact"].asString() : "";
    std::string notes = root.hasKey("notes") && root["notes"].isString() ? root["notes"].asString() : "";

    std::vector<std::pair<std::string, int>> requestedItems;
    if (root.hasKey("items") && root["items"].isArray()) {
        for (const auto& it : root["items"].asArray()) {
            if (it.isObject() && it.hasKey("batchId") && it.hasKey("quantity")) {
                std::string bId = it["batchId"].asString();
                int q = it["quantity"].asInt();
                if (!bId.empty() && q > 0) {
                    requestedItems.emplace_back(bId, q);
                }
            }
        }
    } else if (root.hasKey("batchId") && root.hasKey("quantity")) {
        std::string bId = root["batchId"].asString();
        int q = root["quantity"].asInt();
        if (!bId.empty() && q > 0) {
            requestedItems.emplace_back(bId, q);
        }
    }

    if (requestedItems.empty()) {
        return HttpResponse::error(400, "Bad Request", "Donation request must specify at least one batch item with quantity > 0.");
    }

    std::string err;
    int donationId = -1;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        donationId = warehouseSystem_.getFoodSaver().createDonationRequest(recipient, contact, requestedItems, notes, err);
    }
    if (donationId < 0) {
        return HttpResponse::error(400, "Bad Request", err);
    }

    return HttpResponse::json(201, "Created", "{\"success\":true,\"donationId\":" + std::to_string(donationId) + ",\"message\":\"Donation request created successfully.\"}");
}

HttpResponse HttpRequestHandler::handleFoodSaverDonationApprove(const HttpRequest& req) {
    int id = extractDonationId(req);
    if (id <= 0) return HttpResponse::error(400, "Bad Request", "Missing or invalid donation ID.");
    std::string err;
    bool ok = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ok = warehouseSystem_.getFoodSaver().approveDonation(id, err);
    }
    if (!ok) return HttpResponse::error(400, "Bad Request", err);
    return HttpResponse::json(200, "OK", "{\"success\":true,\"donationId\":" + std::to_string(id) + ",\"status\":\"APPROVED\",\"message\":\"Donation approved.\"}");
}

HttpResponse HttpRequestHandler::handleFoodSaverDonationReject(const HttpRequest& req) {
    int id = extractDonationId(req);
    if (id <= 0) return HttpResponse::error(400, "Bad Request", "Missing or invalid donation ID.");
    std::string reason = "Rejected by warehouse coordinator";
    if (!req.body.empty()) {
        std::string parseErr;
        JsonValue root = JsonValue::parse(req.body, parseErr);
        if (root.isObject() && root.hasKey("reason") && root["reason"].isString()) {
            reason = root["reason"].asString();
        }
    }
    if (req.queryParams.count("reason")) reason = req.queryParams.at("reason");

    std::string err;
    bool ok = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ok = warehouseSystem_.getFoodSaver().rejectDonation(id, reason, err);
    }
    if (!ok) return HttpResponse::error(400, "Bad Request", err);
    return HttpResponse::json(200, "OK", "{\"success\":true,\"donationId\":" + std::to_string(id) + ",\"status\":\"REJECTED\",\"message\":\"Donation rejected.\"}");
}

HttpResponse HttpRequestHandler::handleFoodSaverDonationReserve(const HttpRequest& req) {
    int id = extractDonationId(req);
    if (id <= 0) return HttpResponse::error(400, "Bad Request", "Missing or invalid donation ID.");
    std::string err;
    bool ok = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ok = warehouseSystem_.getFoodSaver().reserveDonationInventory(id, err);
    }
    if (!ok) return HttpResponse::error(400, "Bad Request", err);
    return HttpResponse::json(200, "OK", "{\"success\":true,\"donationId\":" + std::to_string(id) + ",\"status\":\"INVENTORY_RESERVED\",\"message\":\"Donation inventory reserved atomically.\"}");
}

HttpResponse HttpRequestHandler::handleFoodSaverDonationReady(const HttpRequest& req) {
    int id = extractDonationId(req);
    if (id <= 0) return HttpResponse::error(400, "Bad Request", "Missing or invalid donation ID.");
    std::string robotId = "";
    if (!req.body.empty()) {
        std::string parseErr;
        JsonValue root = JsonValue::parse(req.body, parseErr);
        if (root.isObject() && root.hasKey("robotId") && root["robotId"].isString()) {
            robotId = root["robotId"].asString();
        }
    }
    if (req.queryParams.count("robotId")) robotId = req.queryParams.at("robotId");

    std::string err;
    bool ok = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (robotId.empty()) {
            const auto& rList = warehouseSystem_.getRobots();
            for (const auto& r : rList) {
                if (r->getStatus() == RobotStatus::IDLE) {
                    robotId = r->getId();
                    break;
                }
            }
            if (robotId.empty() && !rList.empty()) {
                robotId = rList[0]->getId();
            }
        }
        ok = warehouseSystem_.getFoodSaver().markReadyForPickup(id, robotId, err);
    }
    if (!ok) return HttpResponse::error(400, "Bad Request", err);
    return HttpResponse::json(200, "OK", "{\"success\":true,\"donationId\":" + std::to_string(id) + ",\"status\":\"READY_FOR_PICKUP\",\"assignedRobotId\":\"" + robotId + "\",\"message\":\"Donation items staged and ready for pickup.\"}");
}

HttpResponse HttpRequestHandler::handleFoodSaverDonationComplete(const HttpRequest& req) {
    int id = extractDonationId(req);
    if (id <= 0) return HttpResponse::error(400, "Bad Request", "Missing or invalid donation ID.");
    std::string err;
    bool ok = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ok = warehouseSystem_.getFoodSaver().completeDonation(id, err);
    }
    if (!ok) return HttpResponse::error(400, "Bad Request", err);
    return HttpResponse::json(200, "OK", "{\"success\":true,\"donationId\":" + std::to_string(id) + ",\"status\":\"COMPLETED\",\"message\":\"Donation completed and inventory deducted.\"}");
}

HttpResponse HttpRequestHandler::handleFoodSaverDonationCancel(const HttpRequest& req) {
    int id = extractDonationId(req);
    if (id <= 0) return HttpResponse::error(400, "Bad Request", "Missing or invalid donation ID.");
    std::string reason = "Cancelled by user";
    if (!req.body.empty()) {
        std::string parseErr;
        JsonValue root = JsonValue::parse(req.body, parseErr);
        if (root.isObject() && root.hasKey("reason") && root["reason"].isString()) {
            reason = root["reason"].asString();
        }
    }
    if (req.queryParams.count("reason")) reason = req.queryParams.at("reason");

    std::string err;
    bool ok = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ok = warehouseSystem_.getFoodSaver().cancelDonation(id, reason, err);
    }
    if (!ok) return HttpResponse::error(400, "Bad Request", err);
    return HttpResponse::json(200, "OK", "{\"success\":true,\"donationId\":" + std::to_string(id) + ",\"status\":\"CANCELLED\",\"message\":\"Donation cancelled.\"}");
}

HttpResponse HttpRequestHandler::handleFoodSaverAnalytics(const HttpRequest& /*req*/) {
    std::string json;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        json = warehouseSystem_.getFoodSaverAnalyticsJSON();
    }
    return HttpResponse::json(200, "OK", json);
}

HttpResponse HttpRequestHandler::handleFoodSaverReport(const HttpRequest& req) {
    std::string format = "json";
    if (req.queryParams.count("format")) format = req.queryParams.at("format");
    if (!req.body.empty()) {
        std::string parseErr;
        JsonValue root = JsonValue::parse(req.body, parseErr);
        if (root.isObject() && root.hasKey("format") && root["format"].isString()) {
            format = root["format"].asString();
        }
    }
    std::string report;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        report = warehouseSystem_.generateFoodSaverReport(format);
    }
    if (format == "csv") {
        HttpResponse res = HttpResponse::text(200, "OK", "text/csv; charset=utf-8", report);
        res.headers["Content-Disposition"] = "attachment; filename=\"foodsaver_report.csv\"";
        return res;
    }
    return HttpResponse::json(200, "OK", report);
}

HttpResponse HttpRequestHandler::handleStaticFile(const HttpRequest& req) {
    if (req.method != "GET" && req.method != "HEAD") {
        return HttpResponse::methodNotAllowed("GET, HEAD", req.path);
    }

    // Path traversal check
    if (req.path.find("..") != std::string::npos) {
        return HttpResponse::text(403, "Forbidden", "text/plain; charset=utf-8", "403 Forbidden");
    }

    std::string filePath = staticDir_ + req.path;
    if (req.path == "/" || req.path.empty()) {
        filePath = staticDir_ + "/index.html";
    }

    std::string content = readFileContent(filePath);
    if (content.empty()) {
        std::string fallbackPath = "../" + filePath;
        std::string fallbackContent = readFileContent(fallbackPath);
        if (!fallbackContent.empty()) {
            filePath = fallbackPath;
            content = fallbackContent;
        }
    }
    if (!content.empty()) {
        return HttpResponse::text(200, "OK", getMimeType(filePath), (req.method == "HEAD") ? "" : content);
    } else {
        return HttpResponse::text(404, "Not Found", "text/plain; charset=utf-8", "404 Not Found: " + filePath);
    }
}
