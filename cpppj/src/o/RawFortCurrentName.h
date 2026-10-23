// 원본00460980의현재Fort이름부모와004bf630의같은manager파일명setter를분리복원한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::o {
// 원본Client전역은현재getter와실제dirty DWORD참조를통해읽고쓴다. getter는부작용없는조회다.
struct FortCurrentNameGlobals {
    std::function<std::uint32_t()> owner; //00540cac의현재Template슬롯이다.본문이재조회하는시점마다읽는다.
    std::function<std::uint32_t()> fort; //00594fb8의현재Fort모드다.제목갱신뒤dirty증가와같은순서로읽는다.
    std::function<std::uint32_t()> editor; //005c85a4의현재Editor모드다.설정callback뒤에도다시읽는다.
    std::function<std::uint32_t()> mission; //00594fa0의현재Mission모드다.초기Fort/Editor관문에서만읽는다.
    std::uint32_t& dirty; //00542678의실제공유DWORD다.제목갱신뒤원본처럼한번증가한다.
};
// 파일/장치/전체Snapshot/Notify를명시경계로두며빈기본성공을제공하지않는다.
struct FortCurrentNameHooks {
    std::function<std::span<std::uint8_t>(std::uint32_t)> header; //고정9슬롯의실제20BYTE헤더다.진단callback뒤같은주소가유효해야한다.
    std::function<std::span<std::uint8_t>()> currentName; //0055b490가현재가리키는NUL쓰기버퍼다.이름값을복제하지않는다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> managerName; //헤더의실제현재manager토큰+c부터256BYTE다.토큰0/없는주소는명시거부한다.
    std::function<void()> selectFileSpec; //005327e0에고정FileSpec버퍼주소만쓴다.버퍼내용을fortSpec으로덮지않는다.
    std::function<std::span<const std::uint8_t>(std::span<const std::uint8_t>)> pathSpec; //004410f0(고정FileSpec버퍼,현재name,0,0)의NUL포함결과다.다음PathSpec전까지유효해야한다.
    std::function<void(std::uint32_t)> assertion; //39/88/Fortspec.cpp와429/430/Template.cpp의진단이다.정상반환하면그대로계속한다.
    std::function<void()> snapshot; //004bfac0의현재Battle/Fort wrapper다.현재owner선택은실제하위가수행한다.
    std::function<void(std::uint32_t)> notify; //004bd360의고정this슬롯Notify다.파일명복사직후본문이호출한다.
    std::function<bool(std::span<const std::uint8_t>,std::span<const std::uint8_t>)> samePath; //004e65ed의NUL종료대소문자무시비교다.같으면true를돌려준다.
    std::function<void(std::span<const std::uint8_t>)> deleteRequest; //0041aef0의이전경로삭제요청이다.이모듈은실제파일을조작하지않는다.
    std::function<void()> refreshTitle; //00460730의실제현재제목갱신이다.
    std::function<std::span<const std::uint8_t>(std::uint32_t)> state; //004bd550의고정this현재State다.본문은반환뒤+0x11 BYTE만읽는다.
    std::function<void(std::string_view,std::span<const std::uint8_t>)> config; //004414d0의lastMultiplayerFort/lastEditFort와그시점현재이름이다.
};
// 같은고정Template헤더의manager진단→중복진단→현재manager+c에NUL복사→자체Notify순서다.
void SetFortManagerFileName(std::uint32_t slot,std::span<const std::uint8_t> path,const FortCurrentNameHooks& hooks);
// 새이름은NUL을포함한실제원본버퍼다.원문미초기화oldpath256BYTE는명시scratch로공급하며초기화/복제하지않는다.
// 물리범위를넘으면앞서쓴BYTE/필드효과를보존한채예외로중단한다.파일확장자추가·실제삭제·Editor전체설치는범위밖이다.
void SetCurrentFortName(std::span<const std::uint8_t> name,std::span<std::uint8_t,256> oldPathScratch,const FortCurrentNameGlobals& globals,const FortCurrentNameHooks& hooks);
}
