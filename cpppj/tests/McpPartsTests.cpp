// 4차 로컬 MCP 부품(JSON, zlib/PNG) 검사다. 게임/GUI/통신 없이 순수 함수만 실행한다.
#include "TestSupport.h"
#include "app/McpServer.h"
#include "platform/FrameRecorder.h"
#include "platform/Json.h"
#include "platform/Png.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // 압축 결과를 독립적으로 되돌리는 최소 inflate(저장/고정 Huffman)다. 제품 압축기와 코드를 공유하지 않는다.
using netstorm::platform::Json;
// 낮은 비트부터 읽는 비트 판독기다.
struct BitReader {
    const std::vector<std::uint8_t>& data; // 압축 바이트열이다.
    std::size_t position{};                // 다음 바이트 위치다.
    unsigned bit{};                        // 현재 바이트에서 읽은 비트 수다.
    // 한 비트를 읽는다.
    unsigned Bit() {
        if (position >= data.size()) throw std::runtime_error("inflate 입력 끝");
        const unsigned value = (data[position] >> bit) & 1U;
        if (++bit == 8) { bit = 0; ++position; }
        return value;
    }
    // width비트를 LSB 우선 정수로 읽는다.
    std::uint32_t Bits(unsigned width) { std::uint32_t value = 0; for (unsigned index = 0; index < width; ++index) value |= Bit() << index; return value; }
    // 다음 바이트 경계로 올린다.
    void Align() { if (bit) { bit = 0; ++position; } }
};
// 고정 Huffman 리터럴/길이 심볼 하나를 해독한다(MSB 우선 부호).
unsigned FixedSymbol(BitReader& reader) {
    std::uint32_t code = 0;
    for (unsigned length = 1; length <= 9; ++length) { // 부호를 한 비트씩 늘려 가며 표준 고정 부호 범위와 대조한다.
        code = (code << 1) | reader.Bit();
        if (length == 7 && code <= 0x17) return 256 + code;
        if (length == 8) { if (code >= 0x30 && code <= 0xbf) return code - 0x30; if (code >= 0xc0 && code <= 0xc7) return 280 + (code - 0xc0); }
        if (length == 9 && code >= 0x190 && code <= 0x1ff) return 144 + (code - 0x190);
    }
    throw std::runtime_error("고정 Huffman 부호 오류");
}
// zlib 스트림 전체를 되돌린다(Adler-32도 검증한다).
std::vector<std::uint8_t> Inflate(const std::vector<std::uint8_t>& zlib) {
    if (zlib.size() < 6 || (zlib[0] & 0x0f) != 8 || ((zlib[0] << 8) | zlib[1]) % 31 != 0) throw std::runtime_error("zlib 머리 오류");
    static constexpr std::uint16_t lengthBase[] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
    static constexpr std::uint8_t lengthExtra[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
    static constexpr std::uint16_t distanceBase[] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
    static constexpr std::uint8_t distanceExtra[] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
    BitReader reader{zlib, 2, 0};
    std::vector<std::uint8_t> out;
    bool final = false;
    while (!final) { // 블록을 마지막 블록까지 읽는다.
        final = reader.Bit() != 0;
        const auto type = reader.Bits(2);
        if (type == 0) { // 저장 블록이다.
            reader.Align();
            const auto length = static_cast<std::uint32_t>(zlib.at(reader.position) | (zlib.at(reader.position + 1) << 8));
            reader.position += 4;
            for (std::uint32_t index = 0; index < length; ++index) out.push_back(zlib.at(reader.position++));
        } else if (type == 1) { // 고정 Huffman 블록이다.
            while (true) { // 블록 끝 심볼까지 리터럴과 (길이,거리)를 해독한다.
                const auto symbol = FixedSymbol(reader);
                if (symbol == 256) break;
                if (symbol < 256) { out.push_back(static_cast<std::uint8_t>(symbol)); continue; }
                const auto code = symbol - 257;
                const auto length = lengthBase[code] + reader.Bits(lengthExtra[code]);
                std::uint32_t distanceCode = 0; // 거리 부호 5비트는 MSB 우선이다.
                for (int index = 0; index < 5; ++index) distanceCode = (distanceCode << 1) | reader.Bit();
                const auto distance = distanceBase[distanceCode] + reader.Bits(distanceExtra[distanceCode]);
                if (distance > out.size()) throw std::runtime_error("inflate 거리 범위");
                for (std::uint32_t index = 0; index < length; ++index) out.push_back(out[out.size() - distance]); // 겹치는 복사도 한 바이트씩 처리한다.
            }
        } else throw std::runtime_error("지원하지 않는 deflate 블록");
    }
    reader.Align();
    std::uint32_t adler = 0;
    for (int index = 0; index < 4; ++index) adler = (adler << 8) | zlib.at(reader.position + index);
    if (adler != netstorm::platform::Adler32(out)) throw std::runtime_error("Adler-32 불일치");
    return out;
}
// 결정적 의사 난수 바이트를 만든다(압축 불가에 가까운 입력).
std::vector<std::uint8_t> Noise(std::size_t size, std::uint32_t seed) {
    std::vector<std::uint8_t> bytes(size);
    for (auto& byte : bytes) { seed = seed * 1664525U + 1013904223U; byte = static_cast<std::uint8_t>(seed >> 24); }
    return bytes;
}
}

// 표준 시험 벡터로 두 체크섬을 확인한다.
TEST_CASE(McpParts_ChecksumsMatchStandardVectors) {
    const std::string digits = "123456789", wiki = "Wikipedia";
    const std::span<const std::uint8_t> a(reinterpret_cast<const std::uint8_t*>(digits.data()), digits.size());
    const std::span<const std::uint8_t> b(reinterpret_cast<const std::uint8_t*>(wiki.data()), wiki.size());
    CHECK(netstorm::platform::Crc32(a) == 0xcbf43926U);
    CHECK(netstorm::platform::Adler32(b) == 0x11e60398U);
    CHECK(netstorm::platform::Crc32(a.subspan(4), netstorm::platform::Crc32(a.first(4))) == 0xcbf43926U); // 이어 계산도 같다.
}

// 빈 입력/한 바이트/반복/잡음/큰 반복 입력이 독립 inflate로 정확히 복원되고 반복 입력은 크게 줄어든다.
TEST_CASE(McpParts_ZlibRoundTripsAndShrinksRepeatedData) {
    const std::vector<std::vector<std::uint8_t>> inputs = {{}, {42}, std::vector<std::uint8_t>(70000, 7), Noise(20000, 1), std::vector<std::uint8_t>(300, 0)};
    for (const auto& input : inputs) { // 각 입력은 압축 뒤 원본과 바이트까지 같아야 한다.
        const auto packed = netstorm::platform::ZlibCompress(input);
        CHECK(Inflate(packed) == input);
    }
    CHECK(netstorm::platform::ZlibCompress(std::vector<std::uint8_t>(70000, 7)).size() < 1000); // 긴 같은 값 반복은 258바이트 일치로 거의 사라진다.
    std::vector<std::uint8_t> pattern; // 짧은 주기 패턴이 반복되는 입력이다.
    for (int index = 0; index < 5000; ++index) { pattern.push_back(static_cast<std::uint8_t>(index % 13)); pattern.push_back(static_cast<std::uint8_t>((index / 13) % 3)); }
    const auto packed = netstorm::platform::ZlibCompress(pattern);
    CHECK(Inflate(packed) == pattern && packed.size() < pattern.size() / 2);
}

// 인덱스 PNG의 서명/IHDR/PLTE/IEND 구조, CRC, 압축 해제한 필터0 행이 입력과 일치한다.
TEST_CASE(McpParts_IndexedPngHasValidStructureAndPixels) {
    constexpr std::uint32_t width = 37, height = 19, pitch = 40; // 행 간격이 폭보다 큰 화면을 흉내 낸다.
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(pitch) * height, 0xee);
    for (std::uint32_t y = 0; y < height; ++y) for (std::uint32_t x = 0; x < width; ++x) pixels[y * pitch + x] = static_cast<std::uint8_t>((x * 7 + y * 3) & 0xff);
    std::array<std::uint8_t, 768> palette{};
    for (std::size_t index = 0; index < 768; ++index) palette[index] = static_cast<std::uint8_t>(index * 5);
    const auto png = netstorm::platform::EncodePngIndexed(width, height, pitch, pixels, palette);
    const std::uint8_t signature[8] = {0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a};
    CHECK(png.size() > 8 && std::memcmp(png.data(), signature, 8) == 0);
    std::size_t at = 8;
    std::vector<std::string> types;
    std::vector<std::uint8_t> idat;
    std::vector<std::uint8_t> header, plte;
    while (at + 12 <= png.size()) { // 청크를 순서대로 읽으며 각 CRC를 검증한다.
        const std::uint32_t length = (png[at] << 24) | (png[at + 1] << 16) | (png[at + 2] << 8) | png[at + 3];
        const std::string type(reinterpret_cast<const char*>(&png[at + 4]), 4);
        const std::span<const std::uint8_t> body(&png[at + 8], length);
        const std::uint32_t stored = (png[at + 8 + length] << 24) | (png[at + 9 + length] << 16) | (png[at + 10 + length] << 8) | png[at + 11 + length];
        CHECK(stored == netstorm::platform::Crc32(std::span<const std::uint8_t>(&png[at + 4], 4 + length)));
        types.push_back(type);
        if (type == "IDAT") idat.insert(idat.end(), body.begin(), body.end());
        if (type == "IHDR") header.assign(body.begin(), body.end());
        if (type == "PLTE") plte.assign(body.begin(), body.end());
        at += 12 + length;
    }
    CHECK(at == png.size() && types == (std::vector<std::string>{"IHDR", "PLTE", "IDAT", "IEND"}));
    CHECK(header.size() == 13 && header[0] == 0 && header[1] == 0 && header[2] == 0 && header[3] == width); // 폭은 빅 엔디언 네 바이트다.
    CHECK(header[4] == 0 && header[5] == 0 && header[6] == 0 && header[7] == height && header[8] == 8 && header[9] == 3 && header[10] == 0 && header[11] == 0 && header[12] == 0); // 높이, 비트 깊이 8, 인덱스 색, 압축/필터/인터레이스 0이다.
    CHECK(plte.size() == 768 && std::equal(plte.begin(), plte.end(), palette.begin()));
    const auto raw = Inflate(idat);
    CHECK(raw.size() == static_cast<std::size_t>(width + 1) * height);
    for (std::uint32_t y = 0; y < height; ++y) { // 모든 행은 필터 0이고 화면 픽셀과 같아야 한다(행 간격 여백은 제외).
        CHECK(raw[y * (width + 1)] == 0 && std::equal(pixels.begin() + y * pitch, pixels.begin() + y * pitch + width, raw.begin() + y * (width + 1) + 1));
    }
    bool rejected = false; // 화면 버퍼가 모자라면 거부한다.
    try { static_cast<void>(netstorm::platform::EncodePngIndexed(width, height, pitch, std::span<const std::uint8_t>(pixels).first(100), palette)); } catch (const std::invalid_argument&) { rejected = true; }
    CHECK(rejected);
}

// RGBA PNG는 색 종류 6이고 압축 해제한 행이 입력과 같다.
TEST_CASE(McpParts_RgbaPngRoundTripsPixels) {
    const auto rgba = Noise(5 * 3 * 4, 9);
    const auto png = netstorm::platform::EncodePngRgba(5, 3, rgba);
    CHECK(png[25] == 6 && png[24] == 8); // IHDR의 비트 깊이와 색 종류 위치다.
    std::size_t at = 8; std::vector<std::uint8_t> idat;
    while (at + 12 <= png.size()) {
        const std::uint32_t length = (png[at] << 24) | (png[at + 1] << 16) | (png[at + 2] << 8) | png[at + 3];
        if (std::string(reinterpret_cast<const char*>(&png[at + 4]), 4) == "IDAT") idat.insert(idat.end(), png.begin() + at + 8, png.begin() + at + 8 + length);
        at += 12 + length;
    }
    const auto raw = Inflate(idat);
    CHECK(raw.size() == (5 * 4 + 1) * 3);
    for (std::size_t y = 0; y < 3; ++y) CHECK(raw[y * 21] == 0 && std::equal(rgba.begin() + y * 20, rgba.begin() + (y + 1) * 20, raw.begin() + y * 21 + 1));
}

// JSON은 중첩 값/이스케이프/대리쌍/정수와 실수를 보존해 왕복한다.
TEST_CASE(McpParts_JsonParsesAndDumpsDeterministically) {
    const auto parsed = Json::Parse(R"({"b":[1,-2,3.5,true,false,null,"x\ny\u00e9\ud83d\ude00"],"a":{"k":"v"},"n":12345678901234})");
    CHECK(parsed.has_value());
    CHECK((*parsed)["a"]["k"].AsString() == "v" && (*parsed)["n"].AsInt() == 12345678901234LL);
    const auto& list = (*parsed)["b"].Items();
    CHECK(list.size() == 7 && list[0].AsInt() == 1 && list[1].AsInt() == -2 && list[2].AsDouble() == 3.5 && list[3].AsBool() && !list[4].AsBool(true) && list[5].IsNull());
    CHECK(list[6].AsString() == std::string("x\ny\xc3\xa9\xf0\x9f\x98\x80")); // \u00e9와 대리쌍 U+1F600이 UTF-8로 풀린다.
    const auto dumped = parsed->Dump();
    // 키 사전순 출력의 접두부다. raw 문자열을 CHECK의 문자열화 대상에서 분리한다.
    constexpr char expectedPrefix[] = R"({"a":{"k":"v"},"b":[1,-2,3.5,true,false,null,"x\ny)";
    CHECK(dumped.starts_with(expectedPrefix));
    const auto again = Json::Parse(dumped);
    CHECK(again && again->Dump() == dumped);
    Json built;
    built.Set("text", "a\"b\\c\t").Set("n", 5);
    Json oneItem = Json::MakeArray();
    oneItem.Push(1);
    built.Set("list", oneItem);
    // 따옴표·역슬래시·탭의 JSON 이스케이프 기대값이다. 매크로 밖에서 원문 바이트를 유지한다.
    constexpr char expectedDump[] = R"({"list":[1],"n":5,"text":"a\"b\\c\t"})";
    CHECK(built.Dump() == expectedDump);
    CHECK(!built["missing"].AsBool(false) && built["missing"].IsNull() && built["text"].AsInt(7) == 7); // 종류가 다른 읽기는 fallback이다.
}

// 잘못된 문법, 잡문자, 너무 깊은 중첩, 짝 없는 대리쌍은 거부한다.
TEST_CASE(McpParts_JsonRejectsMalformedInput) {
    for (const char* bad : {"", "{", "[1,]", "{\"a\":}", "{\"a\" 1}", "nul", "1 2", "\"abc", "\"\\q\"", "\"\\ud83d\"", "\"\\ude00\"", "[1,2", "{\"a\":1,}", "01x"}) // 각 입력은 값으로 읽히면 안 된다.
        CHECK(!Json::Parse(bad).has_value());
    CHECK(Json::Parse(" [ 1 , { \"a\" : [ ] } ] ").has_value()); // 공백은 허용한다.
    CHECK(!Json::Parse(std::string(100, '[') + std::string(100, ']')).has_value()); // 깊이 64를 넘는다.
    CHECK(Json::Parse(std::string(60, '[') + std::string(60, ']')).has_value());
    CHECK(!Json::Parse("\"a\nb\"").has_value()); // 이스케이프 없는 제어문자다.
}

// Base64는 표준 시험 벡터(패딩 포함)와 일치한다.
TEST_CASE(McpProtocol_Base64MatchesStandardVectors) {
    const auto encode = [](const std::string& text) { return netstorm::app::McpBase64({reinterpret_cast<const std::uint8_t*>(text.data()), text.size()}); };
    CHECK(encode("") == "" && encode("f") == "Zg==" && encode("fo") == "Zm8=" && encode("foo") == "Zm9v" && encode("foob") == "Zm9vYg==" && encode("Man") == "TWFu");
}

// initialize/ping/tools/list/알림/오류 응답의 JSON-RPC 모양과 프로토콜 버전 협상을 확인한다.
TEST_CASE(McpProtocol_InitializeListPingAndErrors) {
    using netstorm::app::McpProtocol;
    const McpProtocol protocol;
    const auto none = [](netstorm::app::McpToolCall) { CHECK(false); }; // 이 검사의 메시지는 어떤 도구 호출도 만들면 안 된다.
    auto reply = protocol.Handle(*Json::Parse(R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-03-26","capabilities":{}}})"), none);
    CHECK(reply && (*reply)["id"].AsInt() == 1 && (*reply)["jsonrpc"].AsString() == "2.0");
    CHECK((*reply)["result"]["protocolVersion"].AsString() == "2025-03-26" && (*reply)["result"]["serverInfo"]["name"].AsString() == "netstorm-cpp" && (*reply)["result"]["capabilities"].Has("tools"));
    reply = protocol.Handle(*Json::Parse(R"({"jsonrpc":"2.0","id":"a","method":"initialize","params":{"protocolVersion":"1999-01-01"}})"), none);
    CHECK(reply && (*reply)["id"].AsString() == "a" && (*reply)["result"]["protocolVersion"].AsString() == "2024-11-05"); // 모르는 버전은 기본 버전으로 응답한다.
    CHECK(!protocol.Handle(*Json::Parse(R"({"jsonrpc":"2.0","method":"notifications/initialized"})"), none)); // id 없는 알림은 응답하지 않는다.
    reply = protocol.Handle(*Json::Parse(R"({"jsonrpc":"2.0","id":3,"method":"ping"})"), none);
    CHECK(reply && (*reply)["result"].IsObject() && (*reply)["result"].Members().empty());
    reply = protocol.Handle(*Json::Parse(R"({"jsonrpc":"2.0","id":4,"method":"tools/list"})"), none);
    CHECK(reply);
    std::vector<std::string> names;
    for (const auto& tool : (*reply)["result"]["tools"].Items()) { names.push_back(tool["name"].AsString()); CHECK(tool["inputSchema"]["type"].AsString() == "object" && !tool["description"].AsString().empty()); }
    CHECK(names == (std::vector<std::string>{"netstorm_status", "netstorm_report", "netstorm_screenshot", "netstorm_input", "netstorm_wait_frames", "netstorm_record_start", "netstorm_record_stop", "netstorm_quit"}));
    reply = protocol.Handle(*Json::Parse(R"({"jsonrpc":"2.0","id":5,"method":"no/such"})"), none);
    CHECK(reply && (*reply)["error"]["code"].AsInt() == -32601 && !(*reply).Has("result"));
    reply = protocol.Handle(Json(5), none);
    CHECK(reply && (*reply)["error"]["code"].AsInt() == -32600); // 객체가 아닌 메시지는 잘못된 요청이다.
}

// tools/call은 유효한 도구만 대기열에 넣고(id/인자 보존) 모르는 도구는 즉시 오류로 답한다.
TEST_CASE(McpProtocol_ToolCallsAreQueuedOrRejected) {
    using netstorm::app::McpProtocol;
    using netstorm::app::McpToolCall;
    const McpProtocol protocol;
    std::vector<McpToolCall> queued;
    const auto enqueue = [&](McpToolCall call) { queued.push_back(std::move(call)); };
    CHECK(!protocol.Handle(*Json::Parse(R"({"jsonrpc":"2.0","id":7,"method":"tools/call","params":{"name":"netstorm_input","arguments":{"actions":[{"op":"click","arg":"Edit"}],"wait_frames":2}}})"), enqueue));
    CHECK(queued.size() == 1 && queued[0].id.AsInt() == 7 && queued[0].name == "netstorm_input" && queued[0].arguments["wait_frames"].AsInt() == 2 && queued[0].arguments["actions"].Items().size() == 1);
    CHECK(!protocol.Handle(*Json::Parse(R"({"jsonrpc":"2.0","id":8,"method":"tools/call","params":{"name":"netstorm_status"}})"), enqueue));
    CHECK(queued.size() == 2 && queued[1].arguments.IsObject() && queued[1].arguments.Members().empty()); // 인자가 없으면 빈 객체다.
    const auto reply = protocol.Handle(*Json::Parse(R"({"jsonrpc":"2.0","id":9,"method":"tools/call","params":{"name":"evil_tool","arguments":{}}})"), enqueue);
    CHECK(reply && (*reply)["error"]["code"].AsInt() == -32602 && queued.size() == 2);
    const auto text = McpProtocol::TextResult(Json(10), "hello", true);
    CHECK(text["result"]["isError"].AsBool() && text["result"]["content"].Items()[0]["text"].AsString() == "hello" && text["id"].AsInt() == 10);
    const std::vector<std::uint8_t> png{1, 2, 3};
    const auto image = McpProtocol::ImageResult(Json(11), "s", png);
    const auto& items = image["result"]["content"].Items();
    CHECK(items.size() == 2 && items[1]["type"].AsString() == "image" && items[1]["mimeType"].AsString() == "image/png" && items[1]["data"].AsString() == "AQID");
}

// 녹화기는 캡처 주기에 맞는 프레임만 순번 PNG로 저장하고 manifest/ffconcat의 시각과 길이가 일치한다.
TEST_CASE(McpParts_FrameRecorderWritesPngSequenceManifestAndConcat) {
    using netstorm::platform::FrameRecorder;
    using netstorm::platform::FrameRecorderOptions;
    const auto directory = std::filesystem::path(__FILE__).parent_path().parent_path() / "build/test-data" / ("recorder-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    FrameRecorder recorder;
    FrameRecorderOptions options;
    options.directory = directory;
    options.fps = 10;
    std::string error;
    CHECK(recorder.Start(options, error) && recorder.Running());
    CHECK(!recorder.Start(options, error)); // 이미 녹화 중이면 거부한다.
    constexpr std::uint32_t width = 8, height = 6, pitch = 10;
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(pitch) * height, 0);
    std::array<std::uint8_t, 768> palette{};
    for (std::size_t index = 0; index < 768; ++index) palette[index] = static_cast<std::uint8_t>(index);
    const auto offer = [&](double time, std::uint8_t fill) { std::fill(pixels.begin(), pixels.end(), fill); return recorder.Offer(width, height, pitch, pixels, palette, time); };
    CHECK(offer(0.00, 1));   // 첫 프레임은 항상 받는다.
    CHECK(!offer(0.03, 2));  // 주기(0.1초) 전이다.
    CHECK(offer(0.10, 3));
    CHECK(offer(0.21, 4));
    CHECK(!offer(0.25, 5));
    const auto result = recorder.Stop();
    CHECK(!recorder.Running() && result.frames == 3 && result.dropped == 0 && result.width == width && result.height == height);
    for (int index = 0; index < 3; ++index) { // 프레임 파일은 PNG 서명으로 시작해야 한다.
        char name[32]; std::snprintf(name, sizeof(name), "frame_%06d.png", index);
        std::ifstream file(directory / name, std::ios::binary);
        char signature[4]{};
        file.read(signature, 4);
        CHECK(file.good() && static_cast<unsigned char>(signature[0]) == 0x89 && signature[1] == 'P' && signature[2] == 'N' && signature[3] == 'G');
    }
    CHECK(!std::filesystem::exists(directory / "frame_000003.png"));
    std::ifstream manifestFile(result.manifest, std::ios::binary);
    const std::string manifestText((std::istreambuf_iterator<char>(manifestFile)), std::istreambuf_iterator<char>());
    const auto manifest = Json::Parse(manifestText);
    CHECK(manifest && (*manifest)["fps"].AsInt() == 10 && (*manifest)["frames"].AsInt() == 3 && (*manifest)["width"].AsInt() == width && (*manifest)["height"].AsInt() == height);
    const auto& times = (*manifest)["timestamps_ms"].Items();
    CHECK(times.size() == 3 && times[0].AsInt() == 0 && times[1].AsInt() == 100 && times[2].AsInt() == 210);
    CHECK(std::abs((*manifest)["duration_seconds"].AsDouble() - 0.31) < 1e-6); // 마지막 프레임은 한 주기 동안 표시한다.
    std::ifstream concatFile(result.concat, std::ios::binary);
    const std::string concat((std::istreambuf_iterator<char>(concatFile)), std::istreambuf_iterator<char>());
    CHECK(concat.starts_with("ffconcat version 1.0\n") && concat.find("file frame_000000.png\nduration 0.100000\n") != std::string::npos);
    CHECK(concat.find("file frame_000001.png\nduration 0.110000\n") != std::string::npos && concat.find("file frame_000002.png\nduration 0.100000\nfile frame_000002.png\n") != std::string::npos);
    CHECK(!offer(1.0, 6)); // 멈춘 뒤에는 받지 않는다.
    std::string badError;
    FrameRecorder rejected;
    FrameRecorderOptions bad;
    bad.directory = directory / "never";
    bad.fps = 0;
    CHECK(!rejected.Start(bad, badError) && !badError.empty() && !std::filesystem::exists(bad.directory)); // fps 범위를 벗어나면 폴더도 만들지 않는다.
}
