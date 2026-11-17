#ifndef FOOD_SAVER_H
#define FOOD_SAVER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include "Position.h"

// Forward declarations
class Inventory;
class Warehouse;
class TaskManager;

/**
 * @brief Robust, leap-year-aware Date structure avoiding fragile string comparisons.
 */
struct Date {
    int year{2026};
    int month{11};
    int day{16};

    Date() = default;
    Date(int y, int m, int d) : year(y), month(m), day(d) {}

    static bool isLeapYear(int y) {
        return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
    }

    static int daysInMonth(int y, int m) {
        if (m < 1 || m > 12) return 0;
        static const int days[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        if (m == 2 && isLeapYear(y)) return 29;
        return days[m];
    }

    bool isValid() const {
        if (year < 2000 || year > 2100) return false;
        if (month < 1 || month > 12) return false;
        if (day < 1 || day > daysInMonth(year, month)) return false;
        return true;
    }

    static bool tryParse(const std::string& s, Date& outDate) {
        if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
        try {
            int y = std::stoi(s.substr(0, 4));
            int m = std::stoi(s.substr(5, 2));
            int d = std::stoi(s.substr(8, 2));
            Date cand(y, m, d);
            if (!cand.isValid()) return false;
            outDate = cand;
            return true;
        } catch (...) {
            return false;
        }
    }

    static Date fromString(const std::string& s) {
        Date d;
        if (tryParse(s, d)) return d;
        return Date(0, 0, 0); // Invalid marker
    }

    std::string toString() const {
        if (!isValid()) return "INVALID_DATE";
        std::ostringstream ss;
        ss << std::setfill('0') << std::setw(4) << year << "-"
           << std::setfill('0') << std::setw(2) << month << "-"
           << std::setfill('0') << std::setw(2) << day;
        return ss.str();
    }

    int toDays() const {
        int y = year;
        int m = month;
        if (m <= 2) {
            y -= 1;
            m += 12;
        }
        int a = y / 100;
        int b = a / 4;
        int c = 2 - a + b;
        int e = static_cast<int>(365.25 * (y + 4716));
        int f = static_cast<int>(30.6001 * (m + 1));
        return c + day + e + f - 1524;
    }

    int daysUntil(const Date& target) const {
        return target.toDays() - this->toDays();
    }

    bool operator<(const Date& other) const {
        if (year != other.year) return year < other.year;
        if (month != other.month) return month < other.month;
        return day < other.day;
    }
    bool operator<=(const Date& other) const {
        return (*this < other) || (*this == other);
    }
    bool operator==(const Date& other) const {
        return year == other.year && month == other.month && day == other.day;
    }
    bool operator!=(const Date& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Near-Expiry classification statuses.
 */
enum class ExpiryStatus {
    SAFE,
    NEAR_EXPIRY,
    CRITICAL,
    EXPIRED
};

std::string expiryStatusToString(ExpiryStatus status);
ExpiryStatus stringToExpiryStatus(const std::string& str);

/**
 * @brief Batch operational statuses.
 */
enum class BatchStatus {
    ACTIVE,
    RECALLED,
    DISPOSED
};

std::string batchStatusToString(BatchStatus status);
BatchStatus stringToBatchStatus(const std::string& str);

/**
 * @brief Food donation eligibility classification.
 */
enum class DonationEligibility {
    ELIGIBLE,
    INELIGIBLE,
    RESERVED
};

std::string donationEligibilityToString(DonationEligibility status);
DonationEligibility stringToDonationEligibility(const std::string& str);

/**
 * @brief Donation Request lifecycle stages.
 */
enum class DonationStatus {
    DRAFT,
    ELIGIBILITY_CHECKED,
    APPROVED,
    INVENTORY_RESERVED,
    READY_FOR_PICKUP,
    PICKED_UP,
    COMPLETED,
    REJECTED,
    CANCELLED
};

std::string donationStatusToString(DonationStatus status);
DonationStatus stringToDonationStatus(const std::string& str);

/**
 * @brief Batch-level inventory representation for expiry and donation governance.
 */
struct InventoryBatch {
    std::string batchId;
    int productId{0};
    std::string productName;
    std::string category{"Food"};
    int quantity{0};
    std::string unit{"units"};
    Date manufacturingDate{2026, 11, 1};
    Date expiryDate{2026, 11, 30};
    Position storageLocation{3, 3};
    BatchStatus batchStatus{BatchStatus::ACTIVE};
    DonationEligibility donationEligibility{DonationEligibility::ELIGIBLE};
    int reservedQuantity{0};

    int getAvailableQuantity() const {
        return std::max(0, quantity - reservedQuantity);
    }

    std::string toJSON(const Date& currentDate, int nearExpiryWindow = 7, int criticalWindow = 3) const;
    std::string toJSON() const {
        return toJSON(Date(2026, 11, 16), 7, 3);
    }
};

/**
 * @brief Detailed expiry risk evaluation for a single batch.
 */
struct ExpiryEvaluation {
    std::string batchId;
    int productId{0};
    std::string productName;
    int daysRemaining{0};
    ExpiryStatus status{ExpiryStatus::SAFE};
    double riskScore{0.0}; // 0.0 (safest) to 100.0 (critical / expired)
    std::string recommendation;

    std::string toJSON() const;
};

/**
 * @brief Specific item requested within a donation order.
 */
struct DonationItem {
    std::string batchId;
    int productId{0};
    std::string productName;
    int requestedQuantity{0};
    int reservedQuantity{0};
    int pickedQuantity{0};

    std::string toJSON() const;
};

/**
 * @brief Formal Donation Request model managing the multi-step donation lifecycle.
 */
struct DonationRequest {
    int id{0};
    std::string recipientOrganization;
    std::string contactDetails;
    std::vector<DonationItem> items;
    DonationStatus status{DonationStatus::DRAFT};
    std::string createdTimestamp{"2026-11-16 10:00:00"};
    std::string approvedTimestamp;
    std::string pickupTimestamp;
    std::string completedTimestamp;
    std::string rejectionReason;
    std::string notes;
    std::string assignedRobotId;
    Position stagingLocation{14, 13}; // Default packaging/donation station

    int getTotalRequestedQuantity() const {
        int total = 0;
        for (const auto& item : items) total += item.requestedQuantity;
        return total;
    }

    int getTotalReservedQuantity() const {
        int total = 0;
        for (const auto& item : items) total += item.reservedQuantity;
        return total;
    }

    std::string toJSON() const;
};

/**
 * @brief Comprehensive FoodSaver inventory and donation analytics.
 */
struct FoodSaverAnalytics {
    int totalFoodQuantity{0};
    int nearExpiryQuantity{0};
    int criticalExpiryQuantity{0};
    int expiredQuantity{0};
    int eligibleDonationQuantity{0};
    int reservedDonationQuantity{0};
    int completedDonationQuantity{0};
    int foodWasteQuantity{0};
    int foodSavedQuantity{0};
    double donationCompletionRate{0.0};
    double fefoFulfillmentPercentage{100.0};

    std::string toJSON() const;
};

/**
 * @brief Central FoodSaver Engine: Expiry Tracking, FEFO Dispatch & Donation Management.
 */
class FoodSaverManager {
private:
    Date systemDate{2026, 11, 16};
    int nearExpiryDays{7};
    int criticalExpiryDays{3};
    int minDonationShelfLifeDays{1};

    std::map<std::string, InventoryBatch> batches;
    std::vector<DonationRequest> donationRequests;
    int nextDonationId{1001};

    // Analytics counters
    int totalFulfillmentsCount{0};
    int fefoCompliantFulfillmentsCount{0};
    int totalDisposedWasteQuantity{0};
    int totalSavedFoodQuantity{0};

public:
    FoodSaverManager();

    // Date & Configuration Governance
    void setSystemDate(const Date& date);
    Date getSystemDate() const;
    void setThresholds(int nearExpiry, int critical, int minDonationShelfLife = 1);
    int getNearExpiryDays() const { return nearExpiryDays; }
    int getCriticalExpiryDays() const { return criticalExpiryDays; }

    // Batch Management
    bool addBatch(const InventoryBatch& batch, std::string& outError);
    bool updateBatch(const InventoryBatch& batch, std::string& outError);
    const InventoryBatch* getBatch(const std::string& batchId) const;
    InventoryBatch* getBatch(const std::string& batchId);
    bool removeBatch(const std::string& batchId);
    void clearBatches();

    std::vector<InventoryBatch> getAllBatches() const;
    std::vector<InventoryBatch> getBatchesByProduct(int productId) const;
    std::vector<InventoryBatch> getBatchesByCategory(const std::string& category) const;
    std::vector<InventoryBatch> getBatchesByStatus(BatchStatus status) const;

    // Near-Expiry Engine & Risk Evaluation
    ExpiryEvaluation evaluateBatch(const InventoryBatch& batch) const;
    std::vector<ExpiryEvaluation> evaluateAllBatches() const;

    // FEFO (First Expired, First Out) Inventory Fulfillment
    std::vector<InventoryBatch> getBatchesSortedFEFO(int productId = -1) const;
    bool fulfillOrderFEFO(int productId, int quantity, std::vector<std::pair<std::string, int>>& outAllocations, std::string& outError);

    // Donation Eligibility Engine
    bool checkDonationEligibility(const InventoryBatch& batch, std::string& outReason) const;
    std::vector<InventoryBatch> getEligibleDonationBatches() const;

    // Donation Management Lifecycle
    int createDonationRequest(const std::string& recipient, const std::string& contact,
                              const std::vector<std::pair<std::string, int>>& requestedBatchQuantities,
                              const std::string& notes, std::string& outError);

    bool checkDonationRequestEligibility(int donationId, std::string& outReport);
    bool approveDonation(int donationId, std::string& outError);
    bool rejectDonation(int donationId, const std::string& reason, std::string& outError);
    bool reserveDonationInventory(int donationId, std::string& outError);
    bool markReadyForPickup(int donationId, const std::string& assignedRobotId, std::string& outError);
    bool completeDonation(int donationId, std::string& outError);
    bool cancelDonation(int donationId, const std::string& reason, std::string& outError);

    const DonationRequest* getDonationRequest(int donationId) const;
    DonationRequest* getDonationRequest(int donationId);
    std::vector<DonationRequest> getAllDonations() const;

    // Analytics
    FoodSaverAnalytics computeAnalytics() const;

    // Initialization of Canonical Warehouse Food Stock
    void initializeDefaultFoodStock();

    // Serialization
    std::string batchesToJSON() const;
    std::string expirySummaryToJSON() const;
    std::string eligibilitySummaryToJSON() const;
    std::string donationsToJSON() const;
    std::string generateReport(const std::string& format = "json") const;
};

#endif // FOOD_SAVER_H
