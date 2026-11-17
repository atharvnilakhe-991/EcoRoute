#include "../include/FoodSaver.h"
#include <iostream>
#include <algorithm>
#include <sstream>

std::string expiryStatusToString(ExpiryStatus status) {
    switch (status) {
        case ExpiryStatus::SAFE: return "SAFE";
        case ExpiryStatus::NEAR_EXPIRY: return "NEAR_EXPIRY";
        case ExpiryStatus::CRITICAL: return "CRITICAL";
        case ExpiryStatus::EXPIRED: return "EXPIRED";
    }
    return "SAFE";
}

ExpiryStatus stringToExpiryStatus(const std::string& str) {
    if (str == "NEAR_EXPIRY") return ExpiryStatus::NEAR_EXPIRY;
    if (str == "CRITICAL") return ExpiryStatus::CRITICAL;
    if (str == "EXPIRED") return ExpiryStatus::EXPIRED;
    return ExpiryStatus::SAFE;
}

std::string batchStatusToString(BatchStatus status) {
    switch (status) {
        case BatchStatus::ACTIVE: return "ACTIVE";
        case BatchStatus::RECALLED: return "RECALLED";
        case BatchStatus::DISPOSED: return "DISPOSED";
    }
    return "ACTIVE";
}

BatchStatus stringToBatchStatus(const std::string& str) {
    if (str == "RECALLED") return BatchStatus::RECALLED;
    if (str == "DISPOSED") return BatchStatus::DISPOSED;
    return BatchStatus::ACTIVE;
}

std::string donationEligibilityToString(DonationEligibility status) {
    switch (status) {
        case DonationEligibility::ELIGIBLE: return "ELIGIBLE";
        case DonationEligibility::INELIGIBLE: return "INELIGIBLE";
        case DonationEligibility::RESERVED: return "RESERVED";
    }
    return "INELIGIBLE";
}

DonationEligibility stringToDonationEligibility(const std::string& str) {
    if (str == "ELIGIBLE") return DonationEligibility::ELIGIBLE;
    if (str == "RESERVED") return DonationEligibility::RESERVED;
    return DonationEligibility::INELIGIBLE;
}

std::string donationStatusToString(DonationStatus status) {
    switch (status) {
        case DonationStatus::DRAFT: return "DRAFT";
        case DonationStatus::ELIGIBILITY_CHECKED: return "ELIGIBILITY_CHECKED";
        case DonationStatus::APPROVED: return "APPROVED";
        case DonationStatus::INVENTORY_RESERVED: return "INVENTORY_RESERVED";
        case DonationStatus::READY_FOR_PICKUP: return "READY_FOR_PICKUP";
        case DonationStatus::PICKED_UP: return "PICKED_UP";
        case DonationStatus::COMPLETED: return "COMPLETED";
        case DonationStatus::REJECTED: return "REJECTED";
        case DonationStatus::CANCELLED: return "CANCELLED";
    }
    return "DRAFT";
}

DonationStatus stringToDonationStatus(const std::string& str) {
    if (str == "ELIGIBILITY_CHECKED") return DonationStatus::ELIGIBILITY_CHECKED;
    if (str == "APPROVED") return DonationStatus::APPROVED;
    if (str == "INVENTORY_RESERVED") return DonationStatus::INVENTORY_RESERVED;
    if (str == "READY_FOR_PICKUP") return DonationStatus::READY_FOR_PICKUP;
    if (str == "PICKED_UP") return DonationStatus::PICKED_UP;
    if (str == "COMPLETED") return DonationStatus::COMPLETED;
    if (str == "REJECTED") return DonationStatus::REJECTED;
    if (str == "CANCELLED") return DonationStatus::CANCELLED;
    return DonationStatus::DRAFT;
}

std::string InventoryBatch::toJSON(const Date& currentDate, int nearExpiryWindow, int criticalWindow) const {
    int days = currentDate.daysUntil(expiryDate);
    ExpiryStatus expStat = ExpiryStatus::SAFE;
    if (days <= 0) expStat = ExpiryStatus::EXPIRED;
    else if (days <= criticalWindow) expStat = ExpiryStatus::CRITICAL;
    else if (days <= nearExpiryWindow) expStat = ExpiryStatus::NEAR_EXPIRY;

    std::ostringstream ss;
    ss << "{"
       << "\"batchId\":\"" << batchId << "\","
       << "\"productId\":" << productId << ","
       << "\"productName\":\"" << productName << "\","
       << "\"category\":\"" << category << "\","
       << "\"quantity\":" << quantity << ","
       << "\"reservedQuantity\":" << reservedQuantity << ","
       << "\"availableQuantity\":" << getAvailableQuantity() << ","
       << "\"unit\":\"" << unit << "\","
       << "\"manufacturingDate\":\"" << manufacturingDate.toString() << "\","
       << "\"expiryDate\":\"" << expiryDate.toString() << "\","
       << "\"daysRemaining\":" << days << ","
       << "\"expiryStatus\":\"" << expiryStatusToString(expStat) << "\","
       << "\"batchStatus\":\"" << batchStatusToString(batchStatus) << "\","
       << "\"donationEligibility\":\"" << donationEligibilityToString(donationEligibility) << "\","
       << "\"storageLocation\":" << storageLocation.toJSON()
       << "}";
    return ss.str();
}

std::string ExpiryEvaluation::toJSON() const {
    std::ostringstream ss;
    ss << "{"
       << "\"batchId\":\"" << batchId << "\","
       << "\"productId\":" << productId << ","
       << "\"productName\":\"" << productName << "\","
       << "\"daysRemaining\":" << daysRemaining << ","
       << "\"status\":\"" << expiryStatusToString(status) << "\","
       << "\"riskScore\":" << std::fixed << std::setprecision(1) << riskScore << ","
       << "\"recommendation\":\"" << recommendation << "\""
       << "}";
    return ss.str();
}

std::string DonationItem::toJSON() const {
    std::ostringstream ss;
    ss << "{"
       << "\"batchId\":\"" << batchId << "\","
       << "\"productId\":" << productId << ","
       << "\"productName\":\"" << productName << "\","
       << "\"requestedQuantity\":" << requestedQuantity << ","
       << "\"reservedQuantity\":" << reservedQuantity << ","
       << "\"pickedQuantity\":" << pickedQuantity
       << "}";
    return ss.str();
}

std::string DonationRequest::toJSON() const {
    std::ostringstream ss;
    ss << "{"
       << "\"id\":" << id << ","
       << "\"recipientOrganization\":\"" << recipientOrganization << "\","
       << "\"contactDetails\":\"" << contactDetails << "\","
       << "\"status\":\"" << donationStatusToString(status) << "\","
       << "\"createdTimestamp\":\"" << createdTimestamp << "\","
       << "\"approvedTimestamp\":\"" << approvedTimestamp << "\","
       << "\"pickupTimestamp\":\"" << pickupTimestamp << "\","
       << "\"completedTimestamp\":\"" << completedTimestamp << "\","
       << "\"rejectionReason\":\"" << rejectionReason << "\","
       << "\"notes\":\"" << notes << "\","
       << "\"assignedRobotId\":\"" << assignedRobotId << "\","
       << "\"stagingLocation\":" << stagingLocation.toJSON() << ","
       << "\"totalRequestedQuantity\":" << getTotalRequestedQuantity() << ","
       << "\"totalReservedQuantity\":" << getTotalReservedQuantity() << ","
       << "\"items\":[";
    for (size_t i = 0; i < items.size(); ++i) {
        if (i > 0) ss << ",";
        ss << items[i].toJSON();
    }
    ss << "]}";
    return ss.str();
}

std::string FoodSaverAnalytics::toJSON() const {
    std::ostringstream ss;
    ss << "{"
       << "\"totalFoodQuantity\":" << totalFoodQuantity << ","
       << "\"nearExpiryQuantity\":" << nearExpiryQuantity << ","
       << "\"criticalExpiryQuantity\":" << criticalExpiryQuantity << ","
       << "\"expiredQuantity\":" << expiredQuantity << ","
       << "\"eligibleDonationQuantity\":" << eligibleDonationQuantity << ","
       << "\"reservedDonationQuantity\":" << reservedDonationQuantity << ","
       << "\"completedDonationQuantity\":" << completedDonationQuantity << ","
       << "\"foodWasteQuantity\":" << foodWasteQuantity << ","
       << "\"foodSavedQuantity\":" << foodSavedQuantity << ","
       << "\"donationCompletionRate\":" << std::fixed << std::setprecision(1) << donationCompletionRate << ","
       << "\"fefoFulfillmentPercentage\":" << std::fixed << std::setprecision(1) << fefoFulfillmentPercentage
       << "}";
    return ss.str();
}

FoodSaverManager::FoodSaverManager() {
    initializeDefaultFoodStock();
}

void FoodSaverManager::setSystemDate(const Date& date) {
    if (date.isValid()) {
        systemDate = date;
    }
}

Date FoodSaverManager::getSystemDate() const {
    return systemDate;
}

void FoodSaverManager::setThresholds(int nearExpiry, int critical, int minDonationShelfLife) {
    if (nearExpiry > 0) nearExpiryDays = nearExpiry;
    if (critical > 0) criticalExpiryDays = critical;
    if (minDonationShelfLife >= 0) minDonationShelfLifeDays = minDonationShelfLife;
}

bool FoodSaverManager::addBatch(const InventoryBatch& batch, std::string& outError) {
    if (batch.batchId.empty()) {
        outError = "Batch ID cannot be empty";
        return false;
    }
    if (batches.find(batch.batchId) != batches.end()) {
        outError = "Duplicate batch ID: " + batch.batchId;
        return false;
    }
    if (batch.productId <= 0) {
        outError = "Invalid product ID: " + std::to_string(batch.productId);
        return false;
    }
    if (batch.quantity <= 0) {
        outError = "Quantity must be greater than zero";
        return false;
    }
    if (!batch.manufacturingDate.isValid()) {
        outError = "Invalid manufacturing date";
        return false;
    }
    if (!batch.expiryDate.isValid()) {
        outError = "Invalid expiry date";
        return false;
    }
    if (batch.expiryDate < batch.manufacturingDate) {
        outError = "Expiry date cannot be earlier than manufacturing date";
        return false;
    }

    InventoryBatch b = batch;
    std::string reason;
    if (checkDonationEligibility(b, reason)) {
        b.donationEligibility = DonationEligibility::ELIGIBLE;
    } else {
        b.donationEligibility = DonationEligibility::INELIGIBLE;
    }

    batches[b.batchId] = b;
    return true;
}

bool FoodSaverManager::updateBatch(const InventoryBatch& batch, std::string& outError) {
    auto it = batches.find(batch.batchId);
    if (it == batches.end()) {
        outError = "Batch not found: " + batch.batchId;
        return false;
    }
    if (batch.quantity < it->second.reservedQuantity) {
        outError = "Cannot reduce total quantity below reserved quantity (" + std::to_string(it->second.reservedQuantity) + ")";
        return false;
    }
    if (!batch.expiryDate.isValid()) {
        outError = "Invalid expiry date";
        return false;
    }
    if (batch.expiryDate < batch.manufacturingDate) {
        outError = "Expiry date cannot be earlier than manufacturing date";
        return false;
    }

    it->second = batch;
    std::string reason;
    if (checkDonationEligibility(it->second, reason)) {
        it->second.donationEligibility = DonationEligibility::ELIGIBLE;
    } else {
        it->second.donationEligibility = DonationEligibility::INELIGIBLE;
    }
    return true;
}

const InventoryBatch* FoodSaverManager::getBatch(const std::string& batchId) const {
    auto it = batches.find(batchId);
    if (it != batches.end()) {
        return &(it->second);
    }
    return nullptr;
}

InventoryBatch* FoodSaverManager::getBatch(const std::string& batchId) {
    auto it = batches.find(batchId);
    if (it != batches.end()) {
        return &(it->second);
    }
    return nullptr;
}

bool FoodSaverManager::removeBatch(const std::string& batchId) {
    auto it = batches.find(batchId);
    if (it != batches.end()) {
        batches.erase(it);
        return true;
    }
    return false;
}

void FoodSaverManager::clearBatches() {
    batches.clear();
}

std::vector<InventoryBatch> FoodSaverManager::getAllBatches() const {
    std::vector<InventoryBatch> list;
    list.reserve(batches.size());
    for (const auto& kv : batches) {
        list.push_back(kv.second);
    }
    return list;
}

std::vector<InventoryBatch> FoodSaverManager::getBatchesByProduct(int productId) const {
    std::vector<InventoryBatch> list;
    for (const auto& kv : batches) {
        if (kv.second.productId == productId) {
            list.push_back(kv.second);
        }
    }
    return list;
}

std::vector<InventoryBatch> FoodSaverManager::getBatchesByCategory(const std::string& category) const {
    std::vector<InventoryBatch> list;
    for (const auto& kv : batches) {
        if (kv.second.category == category) {
            list.push_back(kv.second);
        }
    }
    return list;
}

std::vector<InventoryBatch> FoodSaverManager::getBatchesByStatus(BatchStatus status) const {
    std::vector<InventoryBatch> list;
    for (const auto& kv : batches) {
        if (kv.second.batchStatus == status) {
            list.push_back(kv.second);
        }
    }
    return list;
}

ExpiryEvaluation FoodSaverManager::evaluateBatch(const InventoryBatch& batch) const {
    ExpiryEvaluation eval;
    eval.batchId = batch.batchId;
    eval.productId = batch.productId;
    eval.productName = batch.productName;
    eval.daysRemaining = systemDate.daysUntil(batch.expiryDate);

    if (eval.daysRemaining <= 0) {
        eval.status = ExpiryStatus::EXPIRED;
        eval.riskScore = 100.0;
        eval.recommendation = "EXPIRED: Remove from active circulation and record food waste disposal.";
    } else if (eval.daysRemaining <= criticalExpiryDays) {
        eval.status = ExpiryStatus::CRITICAL;
        eval.riskScore = 70.0 + (criticalExpiryDays - eval.daysRemaining) * 10.0;
        eval.recommendation = "CRITICAL: Imminent expiry within " + std::to_string(eval.daysRemaining) + " day(s). Prioritize immediate FEFO dispatch or food rescue donation.";
    } else if (eval.daysRemaining <= nearExpiryDays) {
        eval.status = ExpiryStatus::NEAR_EXPIRY;
        eval.riskScore = 40.0 + (nearExpiryDays - eval.daysRemaining) * 7.5;
        eval.recommendation = "NEAR_EXPIRY: Approaching expiry (" + std::to_string(eval.daysRemaining) + " days). Eligible for food donation partner reservation.";
    } else {
        eval.status = ExpiryStatus::SAFE;
        eval.riskScore = std::max(0.0, 40.0 - (eval.daysRemaining - nearExpiryDays) * 1.0);
        eval.recommendation = "SAFE: Fresh inventory with " + std::to_string(eval.daysRemaining) + " days remaining shelf life.";
    }

    return eval;
}

std::vector<ExpiryEvaluation> FoodSaverManager::evaluateAllBatches() const {
    std::vector<ExpiryEvaluation> evals;
    evals.reserve(batches.size());
    for (const auto& kv : batches) {
        evals.push_back(evaluateBatch(kv.second));
    }
    return evals;
}

std::vector<InventoryBatch> FoodSaverManager::getBatchesSortedFEFO(int productId) const {
    std::vector<InventoryBatch> list;
    for (const auto& kv : batches) {
        if (productId > 0 && kv.second.productId != productId) continue;
        if (kv.second.batchStatus != BatchStatus::ACTIVE) continue;
        int days = systemDate.daysUntil(kv.second.expiryDate);
        if (days <= 0) continue; // Exclude expired batches from standard customer order fulfillment
        if (kv.second.getAvailableQuantity() <= 0) continue;
        list.push_back(kv.second);
    }

    std::sort(list.begin(), list.end(), [](const InventoryBatch& a, const InventoryBatch& b) {
        if (a.expiryDate != b.expiryDate) {
            return a.expiryDate < b.expiryDate;
        }
        return a.batchId < b.batchId;
    });

    return list;
}

bool FoodSaverManager::fulfillOrderFEFO(int productId, int quantity, std::vector<std::pair<std::string, int>>& outAllocations, std::string& outError) {
    if (quantity <= 0) {
        outError = "Order quantity must be positive";
        return false;
    }

    std::vector<InventoryBatch> fefoList = getBatchesSortedFEFO(productId);
    int totalAvail = 0;
    for (const auto& b : fefoList) {
        totalAvail += b.getAvailableQuantity();
    }

    if (totalAvail < quantity) {
        outError = "Insufficient unexpired stock for FEFO fulfillment (Available: " + std::to_string(totalAvail) + ", Required: " + std::to_string(quantity) + ")";
        return false;
    }

    int remaining = quantity;
    for (const auto& b : fefoList) {
        if (remaining <= 0) break;
        int take = std::min(remaining, b.getAvailableQuantity());
        batches[b.batchId].quantity -= take;
        outAllocations.push_back({b.batchId, take});
        remaining -= take;
    }

    totalFulfillmentsCount++;
    fefoCompliantFulfillmentsCount++;
    return true;
}

bool FoodSaverManager::checkDonationEligibility(const InventoryBatch& batch, std::string& outReason) const {
    if (batch.batchStatus == BatchStatus::RECALLED) {
        outReason = "INELIGIBLE: Batch is recalled for safety";
        return false;
    }
    if (batch.batchStatus == BatchStatus::DISPOSED) {
        outReason = "INELIGIBLE: Batch has been disposed";
        return false;
    }
    int days = systemDate.daysUntil(batch.expiryDate);
    if (days <= 0) {
        outReason = "INELIGIBLE: Batch expired on " + batch.expiryDate.toString();
        return false;
    }
    if (days < minDonationShelfLifeDays) {
        outReason = "INELIGIBLE: Remaining shelf life (" + std::to_string(days) + " days) below minimum donation threshold (" + std::to_string(minDonationShelfLifeDays) + " days)";
        return false;
    }
    if (batch.getAvailableQuantity() <= 0) {
        outReason = "INELIGIBLE: No unreserved available quantity";
        return false;
    }

    outReason = "ELIGIBLE: Fresh surplus food with " + std::to_string(days) + " days remaining shelf life";
    return true;
}

std::vector<InventoryBatch> FoodSaverManager::getEligibleDonationBatches() const {
    std::vector<InventoryBatch> eligible;
    for (const auto& kv : batches) {
        std::string reason;
        if (checkDonationEligibility(kv.second, reason)) {
            eligible.push_back(kv.second);
        }
    }
    return eligible;
}

int FoodSaverManager::createDonationRequest(
    const std::string& recipient,
    const std::string& contact,
    const std::vector<std::pair<std::string, int>>& requestedBatchQuantities,
    const std::string& notes,
    std::string& outError
) {
    if (recipient.empty()) {
        outError = "Recipient organization name cannot be empty";
        return -1;
    }
    if (requestedBatchQuantities.empty()) {
        outError = "Donation request must contain at least one item";
        return -1;
    }

    DonationRequest req;
    req.id = nextDonationId++;
    req.recipientOrganization = recipient;
    req.contactDetails = contact;
    req.status = DonationStatus::DRAFT;
    req.createdTimestamp = systemDate.toString() + " 10:00:00";
    req.notes = notes;
    req.stagingLocation = Position(14, 13); // PK02 / Donation staging cell

    for (const auto& pair : requestedBatchQuantities) {
        const std::string& bId = pair.first;
        int qty = pair.second;
        if (qty <= 0) {
            outError = "Requested quantity must be positive for batch: " + bId;
            return -1;
        }
        auto it = batches.find(bId);
        if (it == batches.end()) {
            outError = "Batch ID not found: " + bId;
            return -1;
        }

        std::string eligReason;
        if (!checkDonationEligibility(it->second, eligReason)) {
            outError = "Batch " + bId + " is ineligible for donation: " + eligReason;
            return -1;
        }

        if (it->second.getAvailableQuantity() < qty) {
            outError = "Requested quantity exceeds available stock for batch " + bId + " (" + std::to_string(qty) + " > " + std::to_string(it->second.getAvailableQuantity()) + ")";
            return -1;
        }

        DonationItem item;
        item.batchId = bId;
        item.productId = it->second.productId;
        item.productName = it->second.productName;
        item.requestedQuantity = qty;
        item.reservedQuantity = 0;
        item.pickedQuantity = 0;
        req.items.push_back(item);
    }

    donationRequests.push_back(req);
    return req.id;
}

bool FoodSaverManager::checkDonationRequestEligibility(int donationId, std::string& outReport) {
    DonationRequest* req = getDonationRequest(donationId);
    if (!req) {
        outReport = "Donation request not found";
        return false;
    }

    bool allEligible = true;
    std::ostringstream ss;
    ss << "Donation #" << donationId << " Eligibility Assessment:\n";

    for (const auto& item : req->items) {
        auto it = batches.find(item.batchId);
        if (it == batches.end()) {
            allEligible = false;
            ss << " - Batch " << item.batchId << ": FAILED (Batch no longer exists)\n";
            continue;
        }
        std::string reason;
        bool elig = checkDonationEligibility(it->second, reason);
        if (!elig || it->second.getAvailableQuantity() < item.requestedQuantity) {
            allEligible = false;
            ss << " - Batch " << item.batchId << ": FAILED (" << reason << ", Avail: " << it->second.getAvailableQuantity() << ")\n";
        } else {
            ss << " - Batch " << item.batchId << ": PASSED (" << reason << ")\n";
        }
    }

    if (allEligible && req->status == DonationStatus::DRAFT) {
        req->status = DonationStatus::ELIGIBILITY_CHECKED;
    }
    outReport = ss.str();
    return allEligible;
}

bool FoodSaverManager::approveDonation(int donationId, std::string& outError) {
    DonationRequest* req = getDonationRequest(donationId);
    if (!req) {
        outError = "Donation request not found";
        return false;
    }
    if (req->status != DonationStatus::DRAFT && req->status != DonationStatus::ELIGIBILITY_CHECKED) {
        outError = "Cannot approve donation in status: " + donationStatusToString(req->status);
        return false;
    }

    req->status = DonationStatus::APPROVED;
    req->approvedTimestamp = systemDate.toString() + " 11:00:00";
    return true;
}

bool FoodSaverManager::rejectDonation(int donationId, const std::string& reason, std::string& outError) {
    DonationRequest* req = getDonationRequest(donationId);
    if (!req) {
        outError = "Donation request not found";
        return false;
    }
    if (req->status == DonationStatus::COMPLETED || req->status == DonationStatus::CANCELLED) {
        outError = "Cannot reject donation in status: " + donationStatusToString(req->status);
        return false;
    }

    // Release any reservations if already reserved
    if (req->status == DonationStatus::INVENTORY_RESERVED || req->status == DonationStatus::READY_FOR_PICKUP) {
        for (auto& item : req->items) {
            if (batches.find(item.batchId) != batches.end()) {
                batches[item.batchId].reservedQuantity = std::max(0, batches[item.batchId].reservedQuantity - item.reservedQuantity);
            }
            item.reservedQuantity = 0;
        }
    }

    req->status = DonationStatus::REJECTED;
    req->rejectionReason = reason.empty() ? "Operator rejected" : reason;
    return true;
}

bool FoodSaverManager::reserveDonationInventory(int donationId, std::string& outError) {
    DonationRequest* req = getDonationRequest(donationId);
    if (!req) {
        outError = "Donation request not found";
        return false;
    }
    if (req->status != DonationStatus::APPROVED && req->status != DonationStatus::ELIGIBILITY_CHECKED && req->status != DonationStatus::DRAFT) {
        outError = "Cannot reserve inventory in status: " + donationStatusToString(req->status);
        return false;
    }

    // Atomic Pre-Validation: Verify all items can be reserved
    for (const auto& item : req->items) {
        auto it = batches.find(item.batchId);
        if (it == batches.end()) {
            outError = "Atomic reservation failed: Batch " + item.batchId + " no longer exists";
            return false;
        }
        int days = systemDate.daysUntil(it->second.expiryDate);
        if (days <= 0) {
            outError = "Atomic reservation rejected: Batch " + item.batchId + " has expired";
            return false;
        }
        if (it->second.getAvailableQuantity() < item.requestedQuantity) {
            outError = "Atomic reservation failed: Insufficient stock for batch " + item.batchId + " (Required: " + std::to_string(item.requestedQuantity) + ", Available: " + std::to_string(it->second.getAvailableQuantity()) + ")";
            return false;
        }
    }

    // Execute atomic reservation across all batches
    for (auto& item : req->items) {
        batches[item.batchId].reservedQuantity += item.requestedQuantity;
        item.reservedQuantity = item.requestedQuantity;
    }

    req->status = DonationStatus::INVENTORY_RESERVED;
    return true;
}

bool FoodSaverManager::markReadyForPickup(int donationId, const std::string& assignedRobotId, std::string& outError) {
    DonationRequest* req = getDonationRequest(donationId);
    if (!req) {
        outError = "Donation request not found";
        return false;
    }
    if (req->status != DonationStatus::INVENTORY_RESERVED) {
        outError = "Cannot mark ready for pickup from status: " + donationStatusToString(req->status);
        return false;
    }

    req->assignedRobotId = assignedRobotId;
    req->status = DonationStatus::READY_FOR_PICKUP;
    req->pickupTimestamp = systemDate.toString() + " 14:00:00";
    return true;
}

bool FoodSaverManager::completeDonation(int donationId, std::string& outError) {
    DonationRequest* req = getDonationRequest(donationId);
    if (!req) {
        outError = "Donation request not found";
        return false;
    }
    if (req->status != DonationStatus::READY_FOR_PICKUP && req->status != DonationStatus::PICKED_UP && req->status != DonationStatus::INVENTORY_RESERVED) {
        outError = "Cannot complete donation from status: " + donationStatusToString(req->status);
        return false;
    }

    // Deduct stock permanently from batches
    int donatedTotal = 0;
    for (auto& item : req->items) {
        auto it = batches.find(item.batchId);
        if (it != batches.end()) {
            it->second.quantity = std::max(0, it->second.quantity - item.requestedQuantity);
            it->second.reservedQuantity = std::max(0, it->second.reservedQuantity - item.reservedQuantity);
        }
        item.pickedQuantity = item.requestedQuantity;
        item.reservedQuantity = 0;
        donatedTotal += item.requestedQuantity;
    }

    totalSavedFoodQuantity += donatedTotal;
    req->status = DonationStatus::COMPLETED;
    req->completedTimestamp = systemDate.toString() + " 16:30:00";
    return true;
}

bool FoodSaverManager::cancelDonation(int donationId, const std::string& reason, std::string& outError) {
    DonationRequest* req = getDonationRequest(donationId);
    if (!req) {
        outError = "Donation request not found";
        return false;
    }
    if (req->status == DonationStatus::COMPLETED) {
        outError = "Cannot cancel already completed donation";
        return false;
    }

    // Release reserved inventory if reservation occurred
    if (req->status == DonationStatus::INVENTORY_RESERVED || req->status == DonationStatus::READY_FOR_PICKUP || req->status == DonationStatus::PICKED_UP) {
        for (auto& item : req->items) {
            auto it = batches.find(item.batchId);
            if (it != batches.end()) {
                it->second.reservedQuantity = std::max(0, it->second.reservedQuantity - item.reservedQuantity);
            }
            item.reservedQuantity = 0;
        }
    }

    req->status = DonationStatus::CANCELLED;
    req->rejectionReason = reason.empty() ? "Cancelled by operator" : reason;
    return true;
}

const DonationRequest* FoodSaverManager::getDonationRequest(int donationId) const {
    for (const auto& r : donationRequests) {
        if (r.id == donationId) return &r;
    }
    return nullptr;
}

DonationRequest* FoodSaverManager::getDonationRequest(int donationId) {
    for (auto& r : donationRequests) {
        if (r.id == donationId) return &r;
    }
    return nullptr;
}

std::vector<DonationRequest> FoodSaverManager::getAllDonations() const {
    return donationRequests;
}

FoodSaverAnalytics FoodSaverManager::computeAnalytics() const {
    FoodSaverAnalytics a;
    for (const auto& kv : batches) {
        const auto& b = kv.second;
        a.totalFoodQuantity += b.quantity;
        a.reservedDonationQuantity += b.reservedQuantity;

        int days = systemDate.daysUntil(b.expiryDate);
        if (days <= 0) {
            a.expiredQuantity += b.quantity;
            a.foodWasteQuantity += b.quantity;
        } else if (days <= criticalExpiryDays) {
            a.criticalExpiryQuantity += b.quantity;
        } else if (days <= nearExpiryDays) {
            a.nearExpiryQuantity += b.quantity;
        }

        std::string reason;
        if (checkDonationEligibility(b, reason)) {
            a.eligibleDonationQuantity += b.getAvailableQuantity();
        }
    }

    int approvedOrHigherCount = 0;
    int completedCount = 0;
    for (const auto& r : donationRequests) {
        if (r.status == DonationStatus::APPROVED ||
            r.status == DonationStatus::INVENTORY_RESERVED ||
            r.status == DonationStatus::READY_FOR_PICKUP ||
            r.status == DonationStatus::PICKED_UP ||
            r.status == DonationStatus::COMPLETED) {
            approvedOrHigherCount++;
        }
        if (r.status == DonationStatus::COMPLETED) {
            completedCount++;
            a.completedDonationQuantity += r.getTotalRequestedQuantity();
        }
    }

    a.foodSavedQuantity = std::max(totalSavedFoodQuantity, a.completedDonationQuantity);
    a.foodWasteQuantity = std::max(a.foodWasteQuantity, totalDisposedWasteQuantity + a.expiredQuantity);

    // Safe division for completion rate
    if (approvedOrHigherCount > 0) {
        a.donationCompletionRate = (static_cast<double>(completedCount) / approvedOrHigherCount) * 100.0;
    } else {
        a.donationCompletionRate = 0.0;
    }

    // Safe division for FEFO fulfillment percentage
    if (totalFulfillmentsCount > 0) {
        a.fefoFulfillmentPercentage = (static_cast<double>(fefoCompliantFulfillmentsCount) / totalFulfillmentsCount) * 100.0;
    } else {
        a.fefoFulfillmentPercentage = 100.0;
    }

    return a;
}

void FoodSaverManager::initializeDefaultFoodStock() {
    batches.clear();
    std::string err;

    // Shelf Aisle 1 (x=3, y=3..10)
    // 1. Organic Whole Milk - Imminent Expiry (2 days remaining -> CRITICAL)
    InventoryBatch b1;
    b1.batchId = "FB-201-01";
    b1.productId = 201;
    b1.productName = "Organic Whole Milk 1L";
    b1.category = "Dairy";
    b1.quantity = 25;
    b1.unit = "bottles";
    b1.manufacturingDate = Date(2026, 11, 4);
    b1.expiryDate = Date(2026, 11, 18); // 2 days left
    b1.storageLocation = Position(3, 4);
    addBatch(b1, err);

    // 2. Sourdough Sandwich Loaf - Near Expiry (4 days remaining -> NEAR_EXPIRY)
    InventoryBatch b2;
    b2.batchId = "FB-202-01";
    b2.productId = 202;
    b2.productName = "Artisan Sourdough Loaf";
    b2.category = "Bakery";
    b2.quantity = 30;
    b2.unit = "loaves";
    b2.manufacturingDate = Date(2026, 11, 14);
    b2.expiryDate = Date(2026, 11, 20); // 4 days left
    b2.storageLocation = Position(3, 5);
    addBatch(b2, err);

    // 3. Crisp Gala Apples - Near Expiry (6 days remaining -> NEAR_EXPIRY)
    InventoryBatch b3;
    b3.batchId = "FB-203-01";
    b3.productId = 203;
    b3.productName = "Crisp Gala Apples Bag";
    b3.category = "Produce";
    b3.quantity = 40;
    b3.unit = "bags";
    b3.manufacturingDate = Date(2026, 11, 10);
    b3.expiryDate = Date(2026, 11, 22); // 6 days left
    b3.storageLocation = Position(3, 7);
    addBatch(b3, err);

    // Shelf Aisle 2 (x=6, y=3..10)
    // 4. Greek Style Yogurt - Safe (12 days remaining -> SAFE)
    InventoryBatch b4;
    b4.batchId = "FB-204-01";
    b4.productId = 204;
    b4.productName = "Greek Style Yogurt 500g";
    b4.category = "Dairy";
    b4.quantity = 35;
    b4.unit = "tubs";
    b4.manufacturingDate = Date(2026, 11, 10);
    b4.expiryDate = Date(2026, 11, 28); // 12 days left
    b4.storageLocation = Position(6, 5);
    addBatch(b4, err);

    // 5. Plant-Based Protein Bowls - Safe (45 days remaining -> SAFE)
    InventoryBatch b5;
    b5.batchId = "FB-205-01";
    b5.productId = 205;
    b5.productName = "Plant-Based Protein Bowl";
    b5.category = "Prepared Foods";
    b5.quantity = 50;
    b5.unit = "packs";
    b5.manufacturingDate = Date(2026, 11, 1);
    b5.expiryDate = Date(2026, 12, 31); // 45 days left
    b5.storageLocation = Position(6, 7);
    addBatch(b5, err);

    // Shelf Aisle 3 (x=9, y=3..10)
    // 6. Whole Wheat Penne - Safe (180 days remaining -> SAFE)
    InventoryBatch b6;
    b6.batchId = "FB-206-01";
    b6.productId = 206;
    b6.productName = "Whole Wheat Penne 500g";
    b6.category = "Packaged Goods";
    b6.quantity = 80;
    b6.unit = "boxes";
    b6.manufacturingDate = Date(2026, 10, 1);
    b6.expiryDate = Date(2027, 5, 15);
    b6.storageLocation = Position(9, 5);
    addBatch(b6, err);

    // Shelf Aisle 4 (x=11, y=3..10)
    // 7. Spoiled Baby Spinach - Expired (3 days expired -> EXPIRED)
    InventoryBatch b7;
    b7.batchId = "FB-207-01";
    b7.productId = 207;
    b7.productName = "Baby Spinach Clamshell";
    b7.category = "Produce";
    b7.quantity = 15;
    b7.unit = "packs";
    b7.manufacturingDate = Date(2026, 11, 1);
    b7.expiryDate = Date(2026, 11, 13); // Expired 3 days ago
    b7.storageLocation = Position(11, 6);
    addBatch(b7, err);
}

std::string FoodSaverManager::batchesToJSON() const {
    std::ostringstream ss;
    ss << "[";
    bool first = true;
    for (const auto& kv : batches) {
        if (!first) ss << ",";
        first = false;
        ss << kv.second.toJSON(systemDate, nearExpiryDays, criticalExpiryDays);
    }
    ss << "]";
    return ss.str();
}

std::string FoodSaverManager::expirySummaryToJSON() const {
    auto evals = evaluateAllBatches();
    std::ostringstream ss;
    ss << "{\"systemDate\":\"" << systemDate.toString() << "\","
       << "\"nearExpiryThresholdDays\":" << nearExpiryDays << ","
       << "\"criticalThresholdDays\":" << criticalExpiryDays << ","
       << "\"evaluations\":[";
    for (size_t i = 0; i < evals.size(); ++i) {
        if (i > 0) ss << ",";
        ss << evals[i].toJSON();
    }
    ss << "]}";
    return ss.str();
}

std::string FoodSaverManager::eligibilitySummaryToJSON() const {
    auto eligible = getEligibleDonationBatches();
    std::ostringstream ss;
    ss << "{\"eligibleCount\":" << eligible.size() << ",\"eligibleBatches\":[";
    for (size_t i = 0; i < eligible.size(); ++i) {
        if (i > 0) ss << ",";
        ss << eligible[i].toJSON(systemDate, nearExpiryDays, criticalExpiryDays);
    }
    ss << "]}";
    return ss.str();
}

std::string FoodSaverManager::donationsToJSON() const {
    std::ostringstream ss;
    ss << "[";
    for (size_t i = 0; i < donationRequests.size(); ++i) {
        if (i > 0) ss << ",";
        ss << donationRequests[i].toJSON();
    }
    ss << "]";
    return ss.str();
}

std::string FoodSaverManager::generateReport(const std::string& format) const {
    if (format == "csv") {
        std::ostringstream ss;
        ss << "BatchID,ProductID,ProductName,Category,Quantity,Available,ExpiryDate,DaysRemaining,RiskStatus,Eligibility\n";
        for (const auto& kv : batches) {
            const auto& b = kv.second;
            int days = systemDate.daysUntil(b.expiryDate);
            ExpiryEvaluation eval = evaluateBatch(b);
            ss << b.batchId << ","
               << b.productId << ",\""
               << b.productName << "\",\""
               << b.category << "\","
               << b.quantity << ","
               << b.getAvailableQuantity() << ","
               << b.expiryDate.toString() << ","
               << days << ","
               << expiryStatusToString(eval.status) << ","
               << donationEligibilityToString(b.donationEligibility) << "\n";
        }
        return ss.str();
    }

    std::ostringstream ss;
    ss << "{"
       << "\"systemDate\":\"" << systemDate.toString() << "\","
       << "\"analytics\":" << computeAnalytics().toJSON() << ","
       << "\"batches\":" << batchesToJSON() << ","
       << "\"donations\":" << donationsToJSON()
       << "}";
    return ss.str();
}
