#include "board/library_reader.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <algorithm>
#include <array>
#include <utility>

namespace housecat::board {
namespace {

constexpr const char* kBookPath = "/library-book.txt";
constexpr const char* kBookPartPath = "/library-book.part";
constexpr const char* kBookBackupPath = "/library-book.bak";
// The reader uses a dedicated 11x18 bitmap font. Keeping logical pages short
// prevents clipped words in both the narrower portrait and shorter landscape
// reading areas.
constexpr std::size_t kCharactersPerPage = 22;
constexpr std::size_t kMaximumDownloadBytes = 1'800'000;

bool isSpace(const char value) noexcept {
    return value == ' ' || value == '\t' || value == '\r' || value == '\n';
}

}  // namespace

void LibraryReader::begin() {
    storageReady_ = LittleFS.begin(true, "/littlefs", 10, "littlefs");
    Serial.println(storageReady_ ? "[library] LittleFS ready" : "[library] LittleFS unavailable");
}

void LibraryReader::loop(const std::uint64_t nowMs) {
    auto& state = app_.mutableState().library;
    if (state.downloadRequested) {
        if (downloadNotBeforeMs_ == 0) {
            downloadNotBeforeMs_ = nowMs + 1000;
            return;
        }
        if (nowMs < downloadNotBeforeMs_) {
            return;
        }
        downloadNotBeforeMs_ = 0;
        state.downloadRequested = false;
        const auto selectedId = state.books[state.selectedBook].gutenbergId;
        bool indexed = false;
        if (storageReady_ && state.cachedBookId == selectedId) {
            indexed = buildPageIndex(kBookPath) && pageOffsets_.size() >= 2;
            if (indexed) {
                Serial.printf("[library] using cached Gutenberg %lu\n",
                              static_cast<unsigned long>(selectedId));
            }
        }
        if (!storageReady_) {
            schedule(app_.updateLibraryError("Storage is unavailable.", nowMs));
        } else if (!indexed && WiFi.status() != WL_CONNECTED) {
            schedule(app_.updateLibraryError("Connect Wi-Fi to fetch a book.", nowMs));
        } else if (!indexed && (!downloadSelectedBook() || pageOffsets_.size() < 2)) {
            schedule(app_.updateLibraryError("Book download failed. Try again.", nowMs));
        } else {
            if (!indexed) {
                state.cachedBookId = selectedId;
            }
            const auto pageCount = pageOffsets_.size() - 1;
            const auto resumePage = state.bookmarkValid && state.bookmarkedBookId == selectedId
                ? std::min(state.bookmarkedPage, pageCount - 1)
                : 0;
            schedule(app_.updateLibraryReady(loadPage(resumePage), resumePage, pageCount, nowMs));
        }
    }

    if (state.pageRequested && !state.downloadRequested) {
        const auto page = std::min(state.requestedPage, pageOffsets_.size() > 1 ? pageOffsets_.size() - 2 : 0);
        state.pageRequested = false;
        if (pageOffsets_.size() < 2) {
            schedule(app_.updateLibraryError("Open the book again.", nowMs));
        } else {
            schedule(app_.updateLibraryPage(loadPage(page), page, nowMs));
        }
    }
}

std::optional<DispatchResult> LibraryReader::takeDispatchResult() {
    auto result = pendingResult_;
    pendingResult_.reset();
    return result;
}

bool LibraryReader::downloadSelectedBook() {
    const auto& library = app_.state().library;
    const auto id = library.books[library.selectedBook].gutenbergId;
    const char* url = bookUrl(id);
    if (url == nullptr) {
        return false;
    }

    WiFiClientSecure transport;
    // The mirror carries public-domain text and no credentials. Arduino's CA
    // bundle is not available in this build, so transport encryption is used
    // without certificate pinning; metadata and links remain curated locally.
    transport.setInsecure();
    HTTPClient request;
    request.setUserAgent("HouseCat/0.1 public-domain reader");
    request.setTimeout(15000);
    if (!request.begin(transport, url)) {
        return false;
    }
    const int status = request.GET();
    const int contentLength = request.getSize();
    if (status != HTTP_CODE_OK || contentLength > static_cast<int>(kMaximumDownloadBytes)) {
        Serial.printf("[library] HTTP failure: status=%d length=%d\n", status, contentLength);
        request.end();
        return false;
    }

    (void)LittleFS.remove(kBookPartPath);
    File output = LittleFS.open(kBookPartPath, FILE_WRITE);
    if (!output) {
        request.end();
        return false;
    }
    WiFiClient* stream = request.getStreamPtr();
    std::array<std::uint8_t, 1024> buffer{};
    std::size_t written = 0;
    std::uint32_t idleSince = millis();
    while (request.connected() && (contentLength < 0 || written < static_cast<std::size_t>(contentLength))) {
        const std::size_t available = stream->available();
        if (available == 0) {
            if (millis() - idleSince > 15000U) break;
            delay(1);
            continue;
        }
        idleSince = millis();
        const std::size_t count = stream->readBytes(
            buffer.data(), std::min<std::size_t>(available, buffer.size()));
        if (count == 0 || written + count > kMaximumDownloadBytes) break;
        output.write(buffer.data(), count);
        written += count;
    }
    output.close();
    request.end();
    Serial.printf("[library] downloaded Gutenberg %lu: %u bytes\n",
                  static_cast<unsigned long>(id), static_cast<unsigned>(written));
    const bool complete = written != 0
        && (contentLength < 0 || written == static_cast<std::size_t>(contentLength));
    if (!complete) {
        (void)LittleFS.remove(kBookPartPath);
        return false;
    }

    // Validate and index the complete candidate before touching the readable
    // cache. Page offsets remain valid after the same file is renamed.
    if (!buildPageIndex(kBookPartPath) || pageOffsets_.size() < 2) {
        (void)LittleFS.remove(kBookPartPath);
        return false;
    }

    // Replace the readable cache transactionally. If the final rename fails,
    // restore the previous book so a network/storage error cannot destroy a
    // valid offline resume copy.
    (void)LittleFS.remove(kBookBackupPath);
    const bool hadCachedBook = LittleFS.exists(kBookPath);
    if (hadCachedBook && !LittleFS.rename(kBookPath, kBookBackupPath)) {
        (void)LittleFS.remove(kBookPartPath);
        return false;
    }
    if (!LittleFS.rename(kBookPartPath, kBookPath)) {
        if (hadCachedBook) {
            (void)LittleFS.rename(kBookBackupPath, kBookPath);
        }
        (void)LittleFS.remove(kBookPartPath);
        return false;
    }
    (void)LittleFS.remove(kBookBackupPath);
    return true;
}

bool LibraryReader::buildPageIndex(const char* path) {
    File input = LittleFS.open(path, FILE_READ);
    if (!input) return false;

    std::uint32_t contentStart = 0;
    std::uint32_t contentEnd = static_cast<std::uint32_t>(input.size());
    while (input.available()) {
        const String line = input.readStringUntil('\n');
        if (line.startsWith("*** START OF")) {
            contentStart = static_cast<std::uint32_t>(input.position());
            break;
        }
    }
    while (input.available()) {
        const auto lineStart = static_cast<std::uint32_t>(input.position());
        const String line = input.readStringUntil('\n');
        if (line.startsWith("*** END OF")) {
            contentEnd = lineStart;
            break;
        }
    }

    pageOffsets_.clear();
    pageOffsets_.push_back(contentStart);
    input.seek(contentStart);
    std::size_t visible = 0;
    bool previousSpace = true;
    while (input.available() && input.position() < contentEnd) {
        const char value = static_cast<char>(input.read());
        const bool space = isSpace(value);
        if (space) {
            if (!previousSpace) ++visible;
            if (visible >= kCharactersPerPage) {
                pageOffsets_.push_back(static_cast<std::uint32_t>(input.position()));
                visible = 0;
            }
        } else {
            ++visible;
        }
        previousSpace = space;
    }
    if (pageOffsets_.back() != contentEnd) pageOffsets_.push_back(contentEnd);
    input.close();
    Serial.printf("[library] indexed %u page(s)\n", static_cast<unsigned>(pageOffsets_.size() - 1));
    return pageOffsets_.size() >= 2;
}

std::string LibraryReader::loadPage(const std::size_t pageIndex) const {
    if (pageIndex + 1 >= pageOffsets_.size()) return {};
    File input = LittleFS.open(kBookPath, FILE_READ);
    if (!input || !input.seek(pageOffsets_[pageIndex])) return {};
    const auto end = pageOffsets_[pageIndex + 1];
    std::string text;
    text.reserve(kCharactersPerPage + 16);
    bool previousSpace = true;
    while (input.available() && input.position() < end) {
        const char value = static_cast<char>(input.read());
        if (isSpace(value)) {
            if (!previousSpace) text.push_back(' ');
            previousSpace = true;
        } else if (static_cast<unsigned char>(value) < 128U) {
            text.push_back(value);
            previousSpace = false;
        }
    }
    input.close();
    while (!text.empty() && text.back() == ' ') text.pop_back();
    return text;
}

const char* LibraryReader::bookUrl(const std::uint32_t gutenbergId) noexcept {
    switch (gutenbergId) {
        case 11: return "https://gutenberg.pglaf.org/cache/epub/11/pg11.txt";
        case 1342: return "https://gutenberg.pglaf.org/cache/epub/1342/pg1342.txt";
        case 1661: return "https://gutenberg.pglaf.org/cache/epub/1661/pg1661.txt";
        default: return nullptr;
    }
}

void LibraryReader::schedule(DispatchResult result) {
    pendingResult_ = std::move(result);
}

}  // namespace housecat::board
