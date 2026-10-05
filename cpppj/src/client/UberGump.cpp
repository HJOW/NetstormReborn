// 원본 004d0120·004cebf0·004cf270·004ce580의 메뉴/명령 흐름을 표시 기반과 연결한다.
// 전체 Gump/StyleText 및 실제 미션 월드는 아직 복원하지 않았다. 범위: docs/exe/cpp-menu-reconstruction.md.
#include "client/UberGump.h"
#include "client/ClientMain.h"
#include "client/GifImage.h"
#include "app/InspectView.h"
#include "o/OriginalText.h"
#include "o/Template.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 원본 메인 메뉴의 돌 버튼 폭·높이·피치와 목록 행 높이.
constexpr int kButtonWidth = 75, kButtonHeight = 19, kButtonPitch = 79, kRowHeight = 18;
// 화면 조합은 GIF/VFX의 원래 팔레트 번호를 사용한다.
void Paste(IndexedImage& canvas, const IndexedImage& image, int x, int y, ScreenRect clip) {
    DrawIndexedImage(canvas.indices, static_cast<int>(canvas.width), static_cast<int>(canvas.height), image, x, y, clip);
}
// 크기 변경 없이 화면 좌표에 맞춰 질감을 반복한다.
void Tile(IndexedImage& canvas, const IndexedImage& image, ScreenRect rect) {
    // 영역 위에서 아래로 원본 크기의 타일을 반복한다.
    for (int y = rect.top; y < rect.bottom; y += static_cast<int>(image.height)) {
        // 한 줄의 마지막 타일은 판정 영역에 맞춰 자른다.
        for (int x = rect.left; x < rect.right; x += static_cast<int>(image.width)) Paste(canvas, image, x, y, rect);
    }
}
// 화면 경계를 벗어나는 UI 선은 버퍼 경계에서 자른다.
void Fill(IndexedImage& canvas, ScreenRect rect, std::uint8_t color) {
    // 각 줄의 유효 픽셀만 채운다.
    for (int y = std::max(0, rect.top); y < std::min(static_cast<int>(canvas.height), rect.bottom); ++y) {
        const int left = std::clamp(rect.left, 0, static_cast<int>(canvas.width)), right = std::clamp(rect.right, left, static_cast<int>(canvas.width));
        std::fill(canvas.indices.begin() + y * canvas.width + left, canvas.indices.begin() + y * canvas.width + right, color);
    }
}
// 돌 가장자리의 원본 관찰 색을 현재 게임 팔레트의 가장 가까운 색으로 고른다.
std::uint8_t Color(std::span<const ScreenColor> palette, int red, int green, int blue) {
    int distance = 200000, result = 0;
    // UI 가장자리 두 색에만 적용한다. GIF 팔레트를 재배치하지 않는다.
    for (std::size_t i = 0; i < palette.size(); ++i) {
        const auto c = palette[i];
        const int next = (c.red - red) * (c.red - red) + (c.green - green) * (c.green - green) + (c.blue - blue) * (c.blue - blue);
        if (next < distance) { distance = next; result = static_cast<int>(i); }
    }
    return static_cast<std::uint8_t>(result);
}
// 눌림 여부에 따라 돌 테두리의 명암 방향을 바꾼다.
void Bevel(IndexedImage& canvas, ScreenRect r, std::uint8_t light, std::uint8_t dark, bool pressed = false) {
    if (pressed) std::swap(light, dark);
    Fill(canvas, {r.left, r.top, r.right, r.top + 1}, light); Fill(canvas, {r.left, r.top, r.left + 1, r.bottom}, light);
    Fill(canvas, {r.left, r.bottom - 1, r.right, r.bottom}, dark); Fill(canvas, {r.right - 1, r.top, r.right, r.bottom}, dark);
}
// 메뉴 파일 이름을 치환하기 전까지 cur.file 참조를 보존한다.
std::string Replace(std::string text, std::string_view from, std::string_view to) {
    std::size_t pos = 0;
    // 한 번 치환한 부분을 다시 읽지 않아 자기 참조를 만들지 않는다.
    while ((pos = text.find(from, pos)) != std::string::npos) { text.replace(pos, from.size(), to); pos += to.size(); }
    return text;
}
// 설정의 UTF-8 문자열을 원본 글꼴의 Windows-1252 바이트로 되돌린다.
std::string FontBytes(std::string_view text) {
    const auto encoded = o::EncodeOriginalText(text); return std::string(encoded.begin(), encoded.end());
}
// 영어 원문을 원본 비트맵 글꼴의 전진 폭으로 줄 바꿈한다.
std::vector<std::string> Wrap(const BitmapFont& font, std::string_view text, int width) {
    std::vector<std::string> lines; std::istringstream words{std::string(text)}; std::string word, line;
    // 단어 우선으로 줄을 나누고 긴 단어는 글자 경계에서 나눈다.
    while (words >> word) {
        if (!line.empty() && font.Measure(line + " " + word) > width) { lines.push_back(line); line.clear(); }
        if (!line.empty()) line += ' ';
        // 빈 줄에서도 넘치는 단어는 한 바이트씩 나눈다.
        for (const char ch : word) {
            if (!line.empty() && font.Measure(line + ch) > width) { lines.push_back(line); line.clear(); }
            line += ch;
        }
    }
    if (!line.empty()) lines.push_back(line);
    return lines;
}
// 현재 단계에서 실제 처리 경로가 있는 메뉴 명령만 활성화한다.
bool Supported(std::string_view command) {
    const auto name = o::AsciiLower(command);
    return name == "tell" || name == "missionbegin" || name == "donothing" || name == "missionabort" || name == "leavebattle" ||
        name == "quit" || name == "menu" || name == "toggle" || name == "resolution" || name == "volume" || name == "resume";
}
// 자동 검사 보고에는 개행/탭을 제거하여 한 항목 한 줄을 유지한다.
std::string Field(std::string text) { std::replace(text.begin(), text.end(), '\t', ' '); std::replace(text.begin(), text.end(), '\n', ' '); std::replace(text.begin(), text.end(), '\r', ' '); return text; }
}

// 원본 GIF는 색 표의 RGB를 쓰지 않고 파일의 번호를 화면 팔레트에 바로 쓴다(00419ec0→004dae60).
UberGump::UberGump(Client& client) : client_(client) {
    auto& config = client_.Configuration();
    title_ = DecodeGif(client_.Files().Read(config.PathSpec("BackgroundGifSpec", "titleMenu")));
    clouds_ = DecodeGif(client_.Files().Read(config.PathSpec("CloudFileSpec")));
    const auto& type = client_.Assets().Find("fortGump");
    // 원본 타입의 클러스터 이름으로 질감·모서리·체크 표시를 고른다.
    for (const char* name : {"A00", "A01", "I00", "I01", "I02", "I03", "J02"}) {
        const auto frame = client_.Assets().FrameIndex(type, name, 0);
        decorations_.emplace(name, client_.Assets().Shapes().Decode(type.block, frame));
    }
    MainMenu(); Compose(); client_.ShowScene();
}
// 불완전한 검사 어댑터 타입은 이 구현 파일에서 파괴한다.
UberGump::~UberGump() = default;
// 캠페인 이름의 설정 객체를 보존하여 officalN.title 같은 참조를 복원한다.
o::Config& UberGump::Script(std::string_view name) {
    const auto key = o::AsciiLower(name);
    if (const auto found = scripts_.find(key); found != scripts_.end()) return *found->second;
    auto script = std::make_unique<o::Config>(client_.Configuration().Registry(), key);
    const auto path = client_.Configuration().PathSpec("missionSpec", key);
    script->LoadBytes(client_.Files().Read(path)); script->Set("fileName", key);
    const auto [it, inserted] = scripts_.emplace(key, std::move(script)); (void)inserted;
    return *it->second;
}
// @/% 목록은 원본 파일 이름으로 정렬하고 '--' 제목은 선택할 수 없는 구분선으로 둔다.
void UberGump::ExpandMenus(DialogPage& page) {
    std::vector<DialogItem> items;
    // 일반 항목은 유지하고 파일 목록 항목만 확장한다.
    for (auto item : page.items) {
        if (item.menu && !item.label.empty() && (item.label[0] == '@' || item.label[0] == '%')) {
            // 빈 사용자 목록은 빈 메뉴다. 원본에 없는 캠페인을 만들지 않는다.
            for (const auto& path : client_.Files().Match(item.label.substr(1))) {
                const auto name = std::filesystem::path(path).stem().string(); auto& script = Script(name);
                const auto title = script.Get(name + ".title").value_or(name);
                auto entry = item; entry.label = title; entry.action.argument = Replace(item.action.argument, "{cur.file}", name);
                entry.enabled = !title.starts_with("--") && Supported(entry.action.command); items.push_back(std::move(entry));
            }
        } else { item.enabled = item.enabled && Supported(item.action.command); items.push_back(std::move(item)); }
    }
    page.items = std::move(items);
}
// Tell Blank는 현재 창을 닫는다. 미션 브리핑의 A./A1.은 현재 미션 파일에서 찾는다.
void UberGump::Tell(std::string target, bool briefing) {
    if (o::AsciiLower(target) == "blank" || target.empty()) {
        if (mission_) { state_.phase = ClientPhase::Mission; pageName_ = "mission"; popup_.clear(); page_ = {}; client_.Pause(false); }
        else MainMenu();
        rebuild_ = true; return;
    }
    std::optional<std::string> body;
    const auto split = target.find('.');
    if (split != std::string::npos && split + 1 < target.size()) {
        auto& script = Script(target.substr(0, split));
        if (const auto section = script.Section(target.substr(split + 1))) body = std::string(*section);
    } else if (mission_ && (briefing || state_.phase == ClientPhase::Briefing)) body = mission_->Section(target);
    if (!body) {
        if (const auto section = Script("tell").Section(target)) body = std::string(*section);
    }
    if (!body) throw std::runtime_error("Missing dialog section: " + target);
    const auto protectedBody = Replace(*body, "{cur.file}", "__CURRENT_FILE__");
    page_ = ParseDialogPage(Replace(client_.Configuration().SubstituteLocal(protectedBody), "__CURRENT_FILE__", "{cur.file}"));
    ExpandMenus(page_);
    if (page_.items.empty()) page_.items.push_back({"OK", {"DoNothing", "0"}});
    state_.phase = briefing ? ClientPhase::Briefing : ClientPhase::Dialog;
    pageName_ = target; popup_.clear(); opened_ = client_.Time().wall; rebuild_ = true;
    if (mission_) client_.Pause(true);
}
// 메인 메뉴로 돌아올 때 미션 설정 객체도 등록 목록에서 제거한다.
void UberGump::MainMenu() {
    client_.GetRenderer().SetScene({}); client_.GetRenderer().SetBackground(nullptr);
    preview_.reset(); mission_.reset(); fortObjects_ = 0; briefingSections_.clear(); briefingIndex_ = 0;
    state_.phase = ClientPhase::MainMenu; pageName_ = "main"; popup_.clear(); page_ = {}; input_.Cancel();
    client_.Pause(false); client_.Title(kWindowTitle); rebuild_ = true;
}
// 실제 미션 객체 생성 전, 원본 스크립트와 요새 구조의 로드 실패를 먼저 검출한다.
void UberGump::BeginMission(std::string name) {
    preview_.reset(); mission_.reset();
    mission_ = std::make_unique<MissionScript>(client_.Configuration(), name);
    if (!mission_->Loaded()) throw std::runtime_error("Missing mission: " + name);
    const auto type = o::AsciiLower(mission_->MissionType());
    if (type != "tutorial" && type != "battle") throw std::runtime_error("Mission type pending: " + mission_->MissionType());
    const auto fort = o::FortTemplate::Parse(client_.Files().Read(mission_->FortPath()), client_.Assets().TypeTable());
    // 전체 요새 레코드를 세어 자동 검사에서 미션 선택과 파일 로드를 함께 확인한다.
    const auto count = [this](const o::FortChunkSection& section) { for (const auto& chunk : section.chunks) fortObjects_ += chunk.objects.size(); };
    fortObjects_ = 0; count(fort.chaff);
    // 플레이어 영역의 저장 오브젝트를 포함한다.
    for (const auto& section : fort.territories) count(section);
    briefingSections_.clear(); briefingIndex_ = 0;
    // 원본 초기 브리핑은 A.이며 A1. 등은 본문 Tell 명령이 넘긴다.
    if (mission_->Section("A.")) briefingSections_.push_back("A.");
    state_.phase = ClientPhase::LoadingMission; pageName_ = "loading"; popup_.clear(); page_ = {}; rebuild_ = true;
    client_.Title(mission_->Get("title").value_or(name)); client_.Pause(true);
}
// 첫 브리핑을 표시하거나 브리핑 종료 뒤 기존 정적 요새 표시기에 연결한다.
void UberGump::AdvanceBriefing() {
    if (briefingIndex_ < briefingSections_.size()) { Tell(briefingSections_[briefingIndex_++], true); return; }
    state_.phase = ClientPhase::Mission; pageName_ = "mission"; page_ = {}; popup_.clear(); client_.Pause(false);
    preview_ = std::make_unique<app::InspectView>(client_, mission_->Name(), false);
    client_.GetRenderer().SetBackground(nullptr); preview_->Ready(client_); rebuild_ = true;
}
// 현재 복원 범위에서 의미가 있는 원본 옵션을 구성한다. 소리 장치와 전체화면 항목은 후속이다.
void UberGump::OpenMenu(std::string name) {
    popup_ = o::AsciiLower(name); page_ = {}; auto& config = client_.Configuration();
    // 목록 행에는 돌 버튼과 달리 누름 즉시 실행 규칙을 적용한다.
    const auto add = [this](std::string label, std::string command, std::string argument, bool checked = false, bool enabled = true) {
        page_.items.push_back({std::move(label), {std::move(command), std::move(argument)}, true, checked, enabled});
    };
    if (popup_ == "options") {
        add("Direct Draw / Full Screen", "", "", false, false);
        add("Resolution >", "Menu", "resolution");
        add("Sound On", "Toggle", "sound", config.GetInt("sound") != 0);
        add("Play Music", "Toggle", "music", config.GetInt("music") != 0);
        add("Wind Noise", "Toggle", "playWind", config.GetInt("playWind") != 0);
        add("Speaker Swap L/R", "Toggle", "swapLeftRightSpeakers", config.GetInt("swapLeftRightSpeakers") != 0);
        add("Sound Effect Volume >", "Menu", "soundvolume"); add("Music Volume >", "Menu", "musicvolume");
        add("Edge Scroll in Fullscreen", "Toggle", "edgeScroll", config.GetInt("edgeScroll") != 0);
        add("Auto-Demo", "Toggle", "autoDemo", config.GetInt("autoDemo") != 0);
        add("Tell Tips at Startup", "Toggle", "tellTips", config.GetInt("tellTips") != 0);
        add("Pass Server Diagnostic", "", "", false, false);
    } else if (popup_ == "resolution") {
        add("640 by 480", "Resolution", "640", client_.GetScreen().Width() == 640);
        add("800 by 600", "Resolution", "800", client_.GetScreen().Width() == 800);
        add("1024 by 768", "Resolution", "1024", client_.GetScreen().Width() == 1024);
    } else if (popup_ == "soundvolume" || popup_ == "musicvolume") {
        const std::string key = popup_ == "soundvolume" ? "soundVolume" : "musicVolume";
        // 원본 음량 단계는 1~5다. 장치 연결 전에 설정 저장을 검사할 수 있다.
        for (int i = 1; i <= 5; ++i) add("Volume " + std::to_string(i), "Volume", key + "=" + std::to_string(i), config.GetInt(key) == i);
    } else if (popup_ == "help") {
        add("About NetStorm", "Tell", "About");
    } else if (popup_ == "game") {
        add("Leave Battle", "Tell", "LeaveNormal");
    } else throw std::runtime_error("Unknown popup: " + name);
    opened_ = client_.Time().wall; rebuild_ = true;
}
// 현재 입력 단계에서는 객체를 파괴하지 않고 다음 프레임의 State로 넘긴다.
void UberGump::Event(InputEvent event) {
    // 원본 ESC 글자 사건. 초기 브리핑은 확인 버튼으로만 닫는다.
    if ((event.code & 0xffffu) == 0x1b && (event.code & InputCode::kRelease) == 0) {
        if (state_.phase == ClientPhase::Briefing || state_.phase == ClientPhase::LoadingMission) return;
        if (!popup_.empty()) state_.Post({"Menu", "close"});
        else if (mission_ && state_.phase == ClientPhase::Mission) state_.Post({"Menu", "game"});
        else if (state_.phase == ClientPhase::Dialog) state_.Post({"Tell", "Blank"});
        return;
    }
    if (!popup_.empty() && (event.code & 0x03ffffffu) == InputCode::kLeftButton &&
        (event.code & InputCode::kRelease) == 0 && !GumpInput::Contains(popupRect_, {event.x, event.y})) {
        state_.Post({"Menu", "close"}); return;
    }
    const auto old = input_.Pressed(); const auto activated = input_.Event(event);
    if (old != input_.Pressed()) rebuild_ = true;
    if (activated && *activated >= 0 && static_cast<std::size_t>(*activated) < actions_.size()) {
        // 원본 004cebf0은 메인 메뉴 버튼 사건을 받자마자 시작 표시를 지운다.
        if (state_.phase == ClientPhase::MainMenu && popup_.empty()) client_.ClearFullScreenState();
        state_.Post(actions_[static_cast<std::size_t>(*activated)]);
    }
}
// 네이티브 입력 큐의 버튼 사건과 폴링 좌표를 같은 Gump 판정기에 전달한다.
bool UberGump::Input() {
    // 사건 순서를 유지한다. 첫 상태 전환만 다음 프레임에 예약된다.
    while (!client_.Input().Empty()) Event(client_.Input().Pop(false));
    const auto mouse = client_.Poll(InputCode::kLeftButton);
    if (input_.Move({mouse.x, mouse.y}, (mouse.code & InputCode::kOutside) == 0)) rebuild_ = true;
    if (rebuild_) Compose(false);
    return quit_;
}
// 예약 명령은 메시지/입력보다 앞서 실행한다(원본 004b88c0 자리).
void UberGump::Tick() {
    const bool loading = state_.phase == ClientPhase::LoadingMission;
    if (const auto action = state_.Take()) Execute(*action);
    else if (loading) AdvanceBriefing();
    if (page_.timeoutSeconds > 0 && client_.Time().wall - opened_ >= page_.timeoutSeconds) {
        state_.Post(page_.timeout); page_.timeoutSeconds = 0;
    }
    if (rebuild_) Compose();
}
// 선택한 원본 명령의 작은 부분집합. 지원하지 않는 항목은 활성화하지 않는다.
void UberGump::Execute(const DialogAction& action) {
    const auto command = o::AsciiLower(action.command);
    if (command == "quit") { quit_ = true; return; }
    if (command == "tell") { Tell(action.argument, state_.phase == ClientPhase::Briefing); return; }
    if (command == "missionbegin") { BeginMission(action.argument); return; }
    if (command == "missionabort" || command == "leavebattle") { MainMenu(); return; }
    if (command == "donothing" || command == "resume") {
        if (state_.phase == ClientPhase::Briefing) AdvanceBriefing(); else Tell("Blank"); return;
    }
    if (command == "menu") {
        if (action.argument == "close") { popup_.clear(); page_ = {}; rebuild_ = true; }
        else OpenMenu(action.argument);
        return;
    }
    if (command == "toggle") {
        auto& config = client_.Configuration(); config.SetInt(action.argument, config.GetInt(action.argument) == 0 ? 1 : 0);
        popup_.clear(); page_ = {}; rebuild_ = true; return;
    }
    if (command == "volume") {
        const auto equal = action.argument.find('='); client_.Configuration().SetInt(action.argument.substr(0, equal), o::ConfigParseLong(action.argument.substr(equal + 1)));
        popup_.clear(); page_ = {}; rebuild_ = true; return;
    }
    if (command == "resolution") {
        const int width = o::ConfigParseLong(action.argument), height = width == 640 ? 480 : width == 800 ? 600 : 768;
        client_.ChangeResolution(width, height); popup_.clear(); page_ = {}; rebuild_ = true; return;
    }
    throw std::runtime_error("Command pending: " + action.command);
}
// 배경 합성과 글자 목록 제출은 입력 판정과 분리한다. 누름 그림 갱신 중에는 캡처를 유지한다.
void UberGump::Compose(bool controls) {
    const int width = client_.GetScreen().Width(), height = client_.GetScreen().Height();
    const ScreenRect all{0, 0, width, height};
    auto canvas = std::make_shared<IndexedImage>(); canvas->width = width; canvas->height = height;
    canvas->indices.resize(static_cast<std::size_t>(width) * height); canvas->opacity.assign(canvas->indices.size(), 255);
    Tile(*canvas, clouds_, all);
    std::vector<RenderText> text; std::vector<GumpControl> regions;
    const auto& font = client_.Fonts().Get(0); const auto& body = client_.Fonts().Get(5);
    const auto palette = client_.GetScreen().Palette();
    const auto light = Color(palette, 191, 178, 139), dark = Color(palette, 49, 44, 36);
    if (mission_ && preview_) {
        // 실제 월드 복원 전의 정적 장면을 배경에 합성한다. 메뉴는 그 위에 그린다.
        client_.GetRenderer().SetBackground(canvas); preview_->BuildScene(client_);
        canvas = std::make_shared<IndexedImage>(client_.GetRenderer().SceneImage());
    }
    if (controls) { labels_.clear(); actions_.clear(); input_.Cancel(); }
    // 같은 순서의 영역/라벨/명령을 제출한다. 그림만 갱신할 때도 번호가 유지된다.
    const auto control = [&](const DialogItem& item, ScreenRect r) {
        const int id = static_cast<int>(regions.size()); regions.push_back({id, r, item.menu, item.enabled, item.instant});
        if (controls) { labels_.push_back(item.label); actions_.push_back(item.action); }
        return id;
    };
    // 돌 버튼은 누르는 동안만 명암과 글자 위치를 바꾼다.
    const auto button = [&](const DialogItem& item, int x, int y, int w) {
        const auto label = FontBytes(item.label);
        const int id = control(item, {x, y, x + w, y + kButtonHeight + 1});
        const bool down = input_.Pressed() == id;
        const ScreenRect r{x, y, x + w, y + kButtonHeight}; Tile(*canvas, decorations_.at("A00"), r); Bevel(*canvas, r, light, dark, down);
        const int shift = down ? 1 : 0;
        text.push_back({&font, label, x + (w - font.Measure(label)) / 2 + shift, y + (kButtonHeight - font.Height()) / 2 + shift,
            item.enabled ? static_cast<std::uint8_t>(255) : dark, 0, {1, 1}, true, false});
    };
    if (state_.phase == ClientPhase::MainMenu || (!mission_ && !popup_.empty())) {
        Paste(*canvas, title_, (width - 640) / 2, (height - 480) / 2, all);
        const int x = width / 2 - 156, y = height / 2 - 73;
        const std::string campaign = Script("tell").Section("UCampaign") ? "UCampaign" : "Campaign";
        // 원본 10.78의 8개 버튼. 미복원 기능도 원본 자료의 창을 통해 상태를 알린다.
        const std::array<DialogItem, 8> buttons{{
            {"Campaign", {"Tell", campaign}}, {"Multiplayer", {"Tell", "NotImplemented"}}, {"Demo", {"Tell", "DEMOVILLE"}}, {"Help", {"Menu", "help"}},
            {"Edit", {"Tell", "NotImplemented"}}, {"Credits", {"Tell", "Credits"}}, {"Options", {"Menu", "options"}}, {"Quit", {"Quit", "0"}}}};
        // 펼침 메뉴 동안 밑에 보이는 버튼은 입력 영역에 넣지 않는다.
        for (std::size_t i = 0; i < buttons.size(); ++i) button(buttons[i], x + static_cast<int>(i % 4) * kButtonPitch, y + static_cast<int>(i / 4) * 23, kButtonWidth);
    }
    if (state_.phase == ClientPhase::Mission && popup_.empty()) {
        // 원본 Game 메뉴의 연결점. 선택/이동 등 실제 게임 UserInput은 다음 단계다.
        button({"Game", {"Menu", "game"}}, 4, 4, kButtonWidth);
        client_.GetRenderer().SetBackground(canvas); client_.GetRenderer().SetScene({}, std::move(text));
    } else {
        if (!popup_.empty()) {
            if (controls) { regions.clear(); labels_.clear(); actions_.clear(); }
            else regions.clear();
            int menuWidth = 140;
            // 원본 글꼴 폭과 체크 표시 여백으로 목록의 폭을 정한다.
            for (const auto& item : page_.items) menuWidth = std::max(menuWidth, font.Measure(FontBytes(item.label)) + 28);
            const int menuHeight = static_cast<int>(page_.items.size()) * kRowHeight + 8;
            const int x = std::clamp(width / 2 + 2, 0, width - menuWidth), y = std::clamp(height / 2 - 27, 0, height - menuHeight);
            popupRect_ = {x, y, x + menuWidth, y + menuHeight}; Tile(*canvas, decorations_.at("A01"), popupRect_); Bevel(*canvas, popupRect_, light, dark);
            int row = y + 4;
            // 목록은 버튼 뗌을 기다리지 않고 누름에 실행한다.
            for (const auto& item : page_.items) {
                const int id = control(item, {x + 4, row, x + menuWidth - 4, row + kRowHeight});
                if (input_.HoveredMenu() == id) Fill(*canvas, {x + 4, row, x + menuWidth - 4, row + kRowHeight}, dark);
                if (item.checked) Paste(*canvas, decorations_.at("J02"), x + 5, row + 3, all);
                text.push_back({&font, FontBytes(item.label), x + 20, row + 1, item.enabled ? static_cast<std::uint8_t>(255) : dark, 0, {1, 1}, true, false}); row += kRowHeight;
            }
        } else if (state_.phase == ClientPhase::Dialog || state_.phase == ClientPhase::Briefing) {
            if (!page_.background.empty()) {
                auto it = decorations_.find(page_.background);
                if (it == decorations_.end()) {
                    // 원본 00419ec0도 파일이 없으면 배경을 만들지 않는다(nsstart 등은 배포 자료에 없을 수 있다).
                    if (const auto bytes = client_.Files().TryRead(client_.Configuration().PathSpec("BackgroundGifSpec", page_.background)))
                        it = decorations_.emplace(page_.background, DecodeGif(*bytes)).first;
                }
                if (it != decorations_.end()) Paste(*canvas, it->second, (width - static_cast<int>(it->second.width)) / 2, (height - static_cast<int>(it->second.height)) / 2, all);
            }
            const int contentWidth = std::min(480, width - 64), panelWidth = contentWidth + 32;
            std::vector<RenderText> lines; int contentHeight = 0, menuRows = 0, totalButtonWidth = 0, buttons = 0;
            // 제목·본문은 원본 슬롯 3/5와 원본 스타일 캐시로 측정한다.
            for (const auto& p : page_.paragraphs) {
                const auto& f = p.heading ? client_.Fonts().Get(3) : client_.Fonts().Get(5, p.italic ? FontStyle::Italic : p.bold ? FontStyle::Bold : FontStyle::Normal);
                // 줄마다 같은 원본 스타일을 보존한다.
                for (const auto& line : Wrap(f, FontBytes(p.text), contentWidth)) { lines.push_back({&f, line, 0, contentHeight, 255, 0, {1, 1}, true, false}); contentHeight += f.Height(); }
                contentHeight += 8;
            }
            // 목록과 하단 버튼의 실제 높이를 더한다.
            for (const auto& item : page_.items) {
                if (item.menu) ++menuRows; else { ++buttons; totalButtonWidth += std::max(kButtonWidth, font.Measure(FontBytes(item.label)) + 16) + 4; }
            }
            const int panelHeight = std::min(height - 32, contentHeight + menuRows * kRowHeight + (buttons ? 27 : 0) + 32);
            const int left = (width - panelWidth) / 2, top = (height - panelHeight) / 2;
            const ScreenRect panel{left, top, left + panelWidth, top + panelHeight}; Tile(*canvas, decorations_.at("A00"), panel); Bevel(*canvas, panel, light, dark);
            const int footerTop = top + panelHeight - 16 - (buttons ? 27 : 0) - menuRows * kRowHeight;
            // 본문이 긴 원본 문서는 패널 영역까지만 표시한다. 스크롤/이미지/링크는 후속이다.
            for (auto line : lines) if (top + 16 + line.y + line.font->Height() <= footerTop) { line.x += left + 16; line.y += top + 16; text.push_back(std::move(line)); }
            int row = footerTop;
            // 캠페인 제목/완료 체크/잠금 조건은 원본 스크립트 값을 그대로 사용한다.
            for (const auto& item : page_.items) if (item.menu) {
                const int id = control(item, {left + 16, row, left + panelWidth - 16, row + kRowHeight});
                if (input_.HoveredMenu() == id) Fill(*canvas, {left + 16, row, left + panelWidth - 16, row + kRowHeight}, dark);
                if (item.checked) Paste(*canvas, decorations_.at("J02"), left + 17, row + 3, panel);
                text.push_back({&font, FontBytes(item.label), left + 32, row + 1, item.enabled ? static_cast<std::uint8_t>(255) : dark, 0, {1, 1}, true, false}); row += kRowHeight;
            }
            int x = (width - totalButtonWidth + 4) / 2;
            // 하단 버튼은 라벨 폭에 맞춰 나란히 놓는다.
            for (const auto& item : page_.items) if (!item.menu) { const int w = std::max(kButtonWidth, font.Measure(FontBytes(item.label)) + 16); button(item, x, top + panelHeight - 16 - kButtonHeight, w); x += w + 4; }
            // 원본 장식 모서리 네 프레임을 확대하지 않고 붙인다.
            for (int corner = 0; corner < 4; ++corner) {
                const auto& image = decorations_.at("I0" + std::to_string(corner));
                Paste(*canvas, image, corner % 2 ? panel.right - static_cast<int>(image.width) : panel.left, corner / 2 ? panel.bottom - static_cast<int>(image.height) : panel.top, panel);
            }
        } else if (state_.phase == ClientPhase::LoadingMission) {
            text.push_back({&body, "Starting Mission...", (width - body.Measure("Starting Mission...")) / 2, height / 2, 255});
        }
        client_.GetRenderer().SetBackground(canvas); client_.GetRenderer().SetScene({}, std::move(text));
    }
    if (controls) input_.SetControls(std::move(regions));
    rebuild_ = false;
}
// 자동 검사는 라벨 좌표를 읽고 실제 버튼 입력을 보낸다. 명령을 직접 호출하지 않는다.
std::optional<ScreenPoint> UberGump::ControlPoint(std::string_view label) const {
    // 동일 라벨의 첫 영역을 선택한다.
    for (const auto& control : input_.Controls()) if (labels_.at(static_cast<std::size_t>(control.id)) == label)
        return ScreenPoint{(control.rect.left + control.rect.right) / 2, (control.rect.top + control.rect.bottom) / 2};
    return {};
}
// 실제 메뉴 상태·활성 조건·원본 미션 로드 결과를 한 파일에 기록한다.
std::string UberGump::Report() const {
    std::ostringstream out;
    out << "phase\t" << static_cast<int>(state_.phase) << "\npage\t" << Field(pageName_) << "\npopup\t" << Field(popup_) << "\n";
    out << "size\t" << client_.GetScreen().Width() << '\t' << client_.GetScreen().Height() << "\n";
    out << "paused\t" << client_.Paused() << "\nsound\t" << client_.Configuration().GetInt("sound") << "\n";
    out << "fullscreenMarker\t" << client_.Files().TryRead("fullscreenStateFile.dat").has_value() << "\n";
    out << "musicVolume\t" << client_.Configuration().GetInt("musicVolume") << "\nsoundVolume\t" << client_.Configuration().GetInt("soundVolume") << "\n";
    if (mission_) out << "mission\t" << Field(mission_->Name()) << '\t' << Field(mission_->MissionType()) << '\t' << fortObjects_ << "\n";
    // 그리기와 같은 판정 표를 기록한다.
    for (const auto& control : input_.Controls()) {
        const auto& label = labels_.at(static_cast<std::size_t>(control.id));
        const auto item = std::find_if(page_.items.begin(),page_.items.end(),[&label](const DialogItem& value) { return value.label == label; });
        const bool checked = item != page_.items.end() && item->checked;
        out << "control\t" << Field(label) << '\t' << control.enabled << '\t' << control.menu << '\t'
            << control.rect.left << '\t' << control.rect.top << '\t' << control.rect.right << '\t' << control.rect.bottom << '\t' << checked << "\n";
    }
    return out.str();
}
}
