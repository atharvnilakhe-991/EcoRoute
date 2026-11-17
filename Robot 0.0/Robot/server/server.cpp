#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <thread>
#include <vector>
#include <mutex>
#include <cctype>
#include "../core/include/WarehouseSystem.h"
#include "../core/include/HttpHandler.h"

static WarehouseSystem g_warehouse(15, 15);
static std::mutex g_systemMutex;
static HttpRequestHandler g_httpHandler(g_warehouse, g_systemMutex, "web");

bool sendAll(SOCKET sock, const std::string& data) {
    int totalSent = 0;
    int len = static_cast<int>(data.size());
    while (totalSent < len) {
        int sent = send(sock, data.c_str() + totalSent, len - totalSent, 0);
        if (sent <= 0) return false;
        totalSent += sent;
    }
    return true;
}

bool readHttpRequestFromSocket(SOCKET clientSocket, std::string& rawRequest, std::string& errorResponse) {
    std::string received;
    char buffer[4096];
    size_t headerEnd = std::string::npos;
    size_t delimLen = 4;

    // 1. Read header until \r\n\r\n (or \n\n) or 64 KB limit
    while (headerEnd == std::string::npos) {
        int bytes = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytes <= 0) break;
        received.append(buffer, bytes);

        headerEnd = received.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            delimLen = 4;
        } else {
            headerEnd = received.find("\n\n");
            if (headerEnd != std::string::npos) delimLen = 2;
        }

        if (headerEnd == std::string::npos && received.size() > HttpRequestHandler::MAX_HEADER_SIZE) {
            errorResponse = HttpResponse::error(431, "Request Header Fields Too Large",
                "HTTP request headers exceed maximum size (64 KB).").serialize();
            return false;
        }
    }

    if (headerEnd == std::string::npos) {
        if (received.empty()) return false; // Clean disconnect
        errorResponse = HttpResponse::error(400, "Bad Request",
            "Incomplete HTTP request headers.").serialize();
        return false;
    }

    // 2. Parse Content-Length from header section
    std::string headerSection = received.substr(0, headerEnd);
    std::string lowerHeaders = headerSection;
    for (char& c : lowerHeaders) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    size_t contentLength = 0;
    bool hasContentLength = false;

    size_t clPos = lowerHeaders.find("content-length:");
    if (clPos != std::string::npos) {
        size_t lineEnd = headerSection.find("\n", clPos);
        std::string valStr = headerSection.substr(clPos + 15,
            (lineEnd == std::string::npos ? headerSection.size() : lineEnd) - (clPos + 15));
        if (!valStr.empty() && valStr.back() == '\r') valStr.pop_back();

        size_t first = valStr.find_first_not_of(" \t");
        size_t last = valStr.find_last_not_of(" \t");
        if (first != std::string::npos && last != std::string::npos) {
            try {
                long long clVal = std::stoll(valStr.substr(first, last - first + 1));
                if (clVal < 0) {
                    errorResponse = HttpResponse::error(400, "Bad Request",
                        "Negative Content-Length header is invalid.").serialize();
                    return false;
                }
                contentLength = static_cast<size_t>(clVal);
                hasContentLength = true;
            } catch (...) {
                errorResponse = HttpResponse::error(400, "Bad Request",
                    "Invalid Content-Length header format.").serialize();
                return false;
            }
        }
    }

    if (contentLength > HttpRequestHandler::MAX_BODY_SIZE) {
        errorResponse = HttpResponse::payloadTooLarge(
            "Request body exceeds maximum allowed size (2 MB).").serialize();
        return false;
    }

    // 3. Read body until Content-Length is satisfied
    size_t bodyStart = headerEnd + delimLen;
    size_t currentBodyLen = received.size() - bodyStart;

    while (hasContentLength && currentBodyLen < contentLength) {
        int bytes = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            errorResponse = HttpResponse::error(400, "Bad Request",
                "Client disconnected before sending complete request body.").serialize();
            return false;
        }
        received.append(buffer, bytes);
        currentBodyLen += bytes;

        if (currentBodyLen > HttpRequestHandler::MAX_BODY_SIZE) {
            errorResponse = HttpResponse::payloadTooLarge(
                "Request body exceeds maximum allowed size (2 MB).").serialize();
            return false;
        }
    }

    rawRequest = std::move(received);
    return true;
}

void handleClient(SOCKET clientSocket) {
    std::string rawRequest;
    std::string errorResponse;
    if (!readHttpRequestFromSocket(clientSocket, rawRequest, errorResponse)) {
        if (!errorResponse.empty()) {
            sendAll(clientSocket, errorResponse);
        }
        closesocket(clientSocket);
        return;
    }

    std::string responseStr = g_httpHandler.handleRawRequest(rawRequest);
    sendAll(clientSocket, responseStr);
    closesocket(clientSocket);
}

int main() {
    std::cout << "Starting Robot Warehouse HTTP REST API Server on port 8080..." << std::endl;

    g_warehouse.initializeDefaultWarehouse();

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }
#endif

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket." << std::endl;
        return 1;
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed on port 8080." << std::endl;
        closesocket(serverSocket);
        return 1;
    }

    if (listen(serverSocket, 10) == SOCKET_ERROR) {
        std::cerr << "Listen failed." << std::endl;
        closesocket(serverSocket);
        return 1;
    }

    std::cout << "==========================================================" << std::endl;
    std::cout << "  Warehouse Server running at: http://localhost:8080" << std::endl;
    std::cout << "  Serving Web UI and REST API" << std::endl;
    std::cout << "==========================================================" << std::endl;

    while (true) {
        sockaddr_in clientAddr{};
        int clientLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);
        if (clientSocket != INVALID_SOCKET) {
            std::thread(handleClient, clientSocket).detach();
        }
    }

    closesocket(serverSocket);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
