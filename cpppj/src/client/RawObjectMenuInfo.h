// 원본10.78 객체 메뉴의 제목/정보4인자와 현재 상태 경고를 복원한다.
#pragma once
#include "client/RawObjectMenu.h"

namespace netstorm::client {
// 원본 전역은 실제 사용 위치에서 다시 읽는다. TYPE/소유자/count/SID와 섞지 않는다.
struct ObjectMenuInfoState {
    std::uint32_t altar{164},nugget{154}; // 5412e8/5412c0: 제단의count감소와너깃소유자0 예외다.
    std::uint32_t invisible{117},paralyzed{118}; // 54122c/541230의 현재 경고 그림 TYPE다.
    std::function<std::uint32_t()> localOwner,editor,missionPost; // 540c70/5c85a4/594fa4의 현재DWORD다.
};
// 제목 작성기와 번역/순수서식/설정치환·현재 게임 조회는 같은 Client/Session의 실제 소비자다.
struct ObjectMenuInfoHooks {
    std::function<std::uint32_t(o::Sid)> count; // 현재 객체의 실제 가상+80이다.
    std::function<std::string(std::uint32_t)> description,ownerName; // 49af80의 번역 설명/484480의owner이름이다.
    std::function<std::string(std::string_view)> translate,substitute; // 4de9a0과441740(1024,0,0,0)이다.
    std::function<std::string(std::string_view,std::span<const PriestMenuArgument>)> print; // 번역을 추가하지 않는4a89c0 고정%s/%d서식이다.
    std::function<void(std::string_view)> title,header; // 477ee0/478020의 같은controlled 메뉴 작성기다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> alliance; // 현재표의owner행/localOwner열DWORD다.
    std::function<bool(o::Sid)> groundImmobile; // 실제492090이며 포획 상태 문구를 선택한다.
    std::function<std::uint8_t(std::uint32_t)> ownerStatus; // 실제Player9×180의현재+50 BYTE다.
};
// TYPE+aa 클래스는 실제정의의23바이트이하문자열이며 같은고정주소배열을 빌린다.
class RawObjectMenuInfo {
public:
    // 현재10.78풀/188TYPE/클래스표와모든참조는이몸체보다오래살아야한다.
    RawObjectMenuInfo(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,
        std::span<const std::string> classes,ObjectMenuInfoState& state,ObjectMenuInfoHooks hooks);
    // 4ad780: type/count/owner/sid의4인자를각각보존하며-1 SID는편집좌표를생략한다.
    void Describe(std::uint32_t type,std::uint32_t count,std::uint32_t owner,std::uint32_t sid) const;
    // 4ada70: owner/TYPE선행snapshot→가상count→Describe→현재extra경고→포획문구다.
    void Summary(o::Sid object) const;
    // 42a2f0: 실제492090과현재미션/소유자상태로Not Connected!/Capturable!을작성한다.
    void CaptureStatus(o::Sid object) const;
    // 실제4a8980의0..9 로마숫자와범위밖sentinel바이트다. 번역하지않는다.
    static std::string LevelName(std::uint32_t number);
    // 실제4a8940이참조하는8문자열의원문이다. 번역은원본호출위치에서수행한다.
    static std::string_view ThemeName(std::int32_t number);
private:
    std::span<const std::uint8_t> Raw(o::Sid object) const; // 원본이읽는물리슬롯범위만검사한다.
    const o::RiftTypeRecord& Type(std::uint32_t number) const; // 현재TYPE참조를반환한다.
    std::string_view Class(std::uint32_t number) const; // 현재클래스의첫NUL과23바이트계약을보존한다.
    std::string Print(std::string_view pattern,std::initializer_list<PriestMenuArgument> arguments) const; // 원본256바이트서식결과다.
    static std::string Text(std::string value,std::size_t capacity); // 첫NUL과원본버퍼범위를검사한다.
    static std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset); // 비정렬DWORD를읽는다.
    static std::int32_t Truncate(float value); // 원본_ftol signed64의하위signed DWORD다.
    const o::SidPool& pool_; // 같은50바이트실제객체풀이다.
    std::span<const o::RiftTypeRecord> types_; // 같은현재188TYPE다.
    std::span<const std::string> classes_; // 별도TYPE나파생슬롯복사본이아닌누락필드의현재문자열표다.
    ObjectMenuInfoState& state_; // 실제전역getter/경고TYPE를빌린다.
    ObjectMenuInfoHooks hooks_; // 도달한실제경계만요구한다.
};
// 현재controlled제목/머리를실제원본작성기에연결하며번역/서식/게임조회는보존한다.
void BindObjectMenuInfoControls(ObjectMenuInfoHooks& hooks,const MenuGumpControls& controls);
}
