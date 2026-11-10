//현재 Client 전역을 각 원문 위치에서 다시 조회하며 loaded/World/Meta 선행 자격을 더하지 않는다.
#include "client/NativeFortPreparation.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/CampaignTemplateStorage.h"
#include "client/RawButtonControls.h"
#include "client/RawSurfaceWorld.h"
#include "o/RawFortTerritoryRecover.h"
#include "o/RawFortTerritorySave.h"
#include "o/RawMemStream.h" //같은DataManager가선택한현재stream의일반Write를사용한다.
#include <bit>
#include <span>
#include <stdexcept>
#include <vector>

namespace netstorm::client { //Template/Pool/지도/화면 장치는 같은 Client 수명이며 임시 실행기/새 World를 만들지 않는다.
namespace { //원문이 선택한 필드의 host 실제 물리 범위만 확인한다.
// bytes의 at에서 실제 비정렬 DWORD 하나를 읽는다. 선택 필드가 물리 범위 밖이면 앞선 효과를 유지한 채 중단한다.
std::uint32_t ReadFortWord(std::span<const std::uint8_t> bytes,std::size_t at) {
    if (at>bytes.size() || bytes.size()-at<4) throw std::out_of_range("Native Fort 현재 DWORD의 물리 범위 부족");
    std::uint32_t result{};for (std::size_t byte=0;byte<4;++byte) result|=std::uint32_t(bytes[at+byte])<<(byte*8); //비정렬 DWORD4BYTE만 읽는다.
    return result;
}
// 지정 Template9 슬롯에서 요청한 현재 헤더 DWORD를 읽는다. manager나 읽기 전용 여부를 미리 복제하지 않는다.
std::uint32_t CurrentTemplateField(CampaignTemplateStorage& templates,std::uint32_t slot,std::size_t offset) {
    if (slot>=9) throw std::out_of_range("Native Fort 현재 Template9의 선택 물리 범위 부족");return ReadFortWord(templates.Headers(),slot*20U+offset);
} //고정 this 슬롯을 보존하고 header DWORD는 요청 위치마다 새로 읽는다.
// 고정 슬롯의 Territory를 현재 stream에서120BYTE 읽고 원문의 pos/end 진단을 실행한다.
void ReadNativeTerritory(Client& client,CampaignTemplateStorage& templates,std::uint32_t slot,std::span<std::uint8_t,120> output) {
    if (!CurrentTemplateField(templates,slot,0)) client.SurfaceStorage()->AssertDisplay("tm","\\Ns\\O\\Template.cpp",0x22e);
    auto& stream=templates.SelectTerritory(slot);const auto position=stream.position; //진단 뒤 같은 고정 슬롯의 현재 manager/lookup이 선택한 stream을 보존한다.
    if (position>stream.bytes.size() || stream.bytes.size()-position<output.size()) throw std::out_of_range("Native Fort Territory 선택120BYTE의 물리 범위 부족");
    for (std::size_t byte=0;byte<output.size();++byte) output[byte]=stream.bytes[position+byte]; //원문 DWORD30개가 읽는 같은120BYTE만 복사한다.
    stream.position=position+120U;if (std::bit_cast<std::int32_t>(stream.end)<std::bit_cast<std::int32_t>(stream.position)) client.SurfaceStorage()->AssertDisplay("pos<=end","c:\\ns\\o\\memstream.h",0x2e);
} //output은호출자가보장한실제120BYTE이므로원문NULL목적진단은선택되지않는다.
// 같은 실제 Template의 Territory Create/현재 stream 재조회/일반120BYTE writer를 연결한다.
void WriteNativeTerritory(Client& client,CampaignTemplateStorage& templates,std::uint32_t slot,o::FortTerritoryRecords records) {
    std::vector<TemplateWriteStream> streams; //현재조회때발급한실제stream수명을일반Write정상복귀까지보관한다.
    o::FortTerritoryWriteHooks hooks;
    hooks.manager=[&](auto selected) { return CurrentTemplateField(templates,selected,0); };
    hooks.assertion=[&](auto line) { client.SurfaceStorage()->AssertDisplay(line==521 ? "tm":"terrList","\\Ns\\O\\Template.cpp",line); };
    const auto currentManager=[&](auto manager) { if (manager!=hooks.manager(slot)) throw std::logic_error("Native Fort 고정 슬롯의 현재 manager 불일치"); }; //현재 manager0의 물리 거부는 TemplateStorage가 소유한다.
    hooks.create=[&](auto manager,auto name) { currentManager(manager);static_cast<void>(templates.CreateSection(slot,name)); };
    hooks.current=[&](auto manager) { currentManager(manager);streams.push_back(templates.CurrentWriteStream(slot));return static_cast<std::uint32_t>(streams.size()); };
    hooks.write=[&](auto token,auto source,auto length) {
        if (!token || token>streams.size()) throw std::out_of_range("Native Fort Territory의 선택 stream 물리 범위 부족");
        templates.AccessSection(streams[token-1],[&](auto fields,const auto& memory) { o::WriteMemStream(fields,source ? std::span<const std::uint8_t>(*source):std::span<const std::uint8_t>{},length,memory); });
    }; //원문의120BYTE일반Write/현재stream재조회와기존부분효과를그대로공유한다.
    o::WriteFortTerritoryRecords(slot,records,hooks);
}
// 같은 장기 섬 기록의 signed count와 지정 섬의 존재 DWORD만 현재 시점에 조회한다.
bool CurrentIslandExists(const RawSurfaceStorage& storage,std::int32_t island) {
    const auto records=std::span<const std::uint8_t>(storage.islands.records);if (island<0 || island>=std::bit_cast<std::int32_t>(ReadFortWord(records,0))) return false;
    if (island>=100) throw std::out_of_range("Native Fort 현재 섬100기록의 물리 범위 부족");return ReadFortWord(records,4+static_cast<std::size_t>(island)*32+4)!=0;
} //원문의signed count/범위관문뒤존재DWORD만읽으며Exists를새로쓰기하지않는다.
// 현재 ChunkMap 유효/좌표 진단 후 기존 가장자리 clamp selector를 호출한다.
o::ChunkEntry& SelectNativeChunk(Client& client,RawSurfaceStorage& storage,std::int32_t x,std::int32_t y) {
    if (!storage.chunks.Mode()) client.SurfaceStorage()->AssertDisplay("isValid()","\\Ns\\O\\Chunkmap.cpp",0xb7);
    if (x<0 || y<0 || x>=storage.chunks.XLength() || y>=storage.chunks.YLength()) client.SurfaceStorage()->AssertDisplay("cc.x>=0 && cc.y>=0 && cc.x<xLen && cc.y<yLen","\\Ns\\O\\Chunkmap.cpp",0xb8);
    return storage.chunks.At(x,y); //두진단의정상복귀뒤현재길이로clamp하는기존같은물리selector를사용한다.
}
// 첫 진단 전에 받은6BYTE 값 인자를 고정하고 현재 지도 전체의 같은 섬 record와 생산 dirty를 갱신한다.
void UpdateNativeIslandRecord(Client& client,RawButtonControls& controls,std::int32_t island,std::span<const std::uint8_t,6> record) {
    const std::array<std::uint8_t,6> value{record[0],record[1],record[2],record[3],record[4],record[5]}; //원문값인자DWORD/WORD는첫진단전에고정한다.
    const auto storage=client.SurfaceStorage();if (!CurrentIslandExists(*storage,island)) storage->AssertDisplay("islandList.exists(islandId)","\\Ns\\O\\Chunkmap.cpp",0x139);
    std::int32_t x=0,y=0;auto* chunk=&SelectNativeChunk(client,*storage,0,0); //원문은Y길이관문전첫selector의진단도실행한다.
    if (storage->chunks.YLength()>0) {
        do { //원문행우선pointer진행과각선택뒤현재길이재조회를보존한다.
            if (std::bit_cast<std::int8_t>(chunk->island)==island) chunk->record=value;
            x=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(x)+1U);
            if (storage->chunks.XLength()<=x) { x=0;y=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(y)+1U); }
            chunk=y<storage->chunks.YLength() ? &SelectNativeChunk(client,*storage,x,y):nullptr;
        } while (y<storage->chunks.YLength());
    }
    controls.RefreshProduction(); //현재실제Gump입력Route를사용하며생산Deck/World를새로만들지않는다.
}
}
//4b3620의경고와진단은반환한위치에서이어가고이름검증/next19/네트워크는실행하지않는다.
std::array<std::uint8_t,6> PrepareNativeFortDescend(Client& client,CampaignTemplateStorage& templates,RawButtonControls& controls,std::int32_t territory,const std::function<void(std::uint32_t)>& snapshot) {
    if (CurrentTemplateField(templates,client.GameplayGlobals()->player.templateOwner,8)) client.WriteDiagnosticLog(29,"WARNING: You cannot descend with a read-only fort.\n");
    if (!client.SurfaceStorage()->ownerMode.fort) client.SurfaceStorage()->AssertDisplay("inFortMode",".\\State.cpp",0x91b);
    if (territory==client.LocalStateGlobals()->InvalidTerritory()) client.SurfaceStorage()->AssertDisplay("terrId != invalidTerrId",".\\State.cpp",0x91c);
    client.LocalStateGlobals()->BattleTerritory()=std::bit_cast<std::uint32_t>(territory);
    std::array<std::uint8_t,120> records{};ReadNativeTerritory(client,templates,client.GameplayGlobals()->player.templateOwner,records);
    if (territory<0 || territory>=20) throw std::out_of_range("Native Fort 현재 지역20기록의 선택 물리 범위 부족");const auto at=static_cast<std::size_t>(territory)*6;records[at+1]|=4U;
    const std::array<std::uint8_t,6> record{records[at],records[at+1],records[at+2],records[at+3],records[at+4],records[at+5]}; //섬검색전의DWORD/WORD값인자를고정한다.
    const auto storage=client.SurfaceStorage();const auto island=o::FindFortTerritoryIsland(storage->islands.records,storage->ownerMode.fort,std::bit_cast<std::int32_t>(client.GameplayGlobals()->player.battleOwner),territory);
    UpdateNativeIslandRecord(client,controls,island,record);
    WriteNativeTerritory(client,templates,client.GameplayGlobals()->player.templateOwner,records);
    if (!snapshot) throw std::logic_error("Native Fort 전체 snapshot의 실제 장기 저장 owner 미연결");snapshot(client.GameplayGlobals()->player.templateOwner); //앞선경고/진단/현재지역/지도/120BYTE쓰기뒤에만실제하위를요구한다.
    return {records[at],records[at+1],records[at+2],records[at+3],records[at+4],records[at+5]};
}
}
