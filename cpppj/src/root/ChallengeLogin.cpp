// CCh 로그인 본체의 참가자 배치와 실제 응답/명단·주소·뉴스 순서를 복원한다.
#include "root/ChallengeDetail.h"

namespace netstorm::root { // Root1Ch가 전달한 같은 요청/이전 socket/현재 CCh를 사용한다.
using namespace challenge_detail; // 실제 wire/소유 문자열/선택 장치만 공유한다.
void ChallengeRuntime::Login(std::uint32_t challenge,std::uint32_t source,std::span<std::uint8_t> request) {
    const auto originalName=Text(request,46);
    for (std::size_t offset=0;offset<originalName.size();++offset) { //0044bb00의1..2f/3a..40/5b..60(5f제외)/7b..ff 치환을 요청 자체에 적용한다.
        auto& value=request[46+offset];if ((value>=1 && value<0x30) || (value>=0x3a && value<0x41) || (value>=0x5b && value<0x61 && value!=0x5f) || value>=0x7b) value='X';
    }
    std::unique_ptr<SocketDevice> socket;if (Read(challenges_.Bytes(challenge),4)&2U) socket=Need(hooks_.claimSocket)(Read(request,41));else socket=std::make_unique<SocketDevice>();
    const auto connection=transport_.AdoptTransferredStream(std::move(socket));const auto peer=transport_.SocketPeerAddress(connection);std::uint8_t status{};bool published=false;
    try {
        const auto attempt=[&]() -> bool { // 원문 실패 코드를 정한 뒤 실제 실패 응답/flush/close 공통 꼬리로 복귀한다.
            if (Read(challenges_.Bytes(challenge),kChallengeCounterOffset)) { status=6;return false; }
            const auto nameLength=Read(request,45,1);if (!nameLength || Text(request,46).find(static_cast<char>(0xa0))!=std::string::npos || nameLength>=23) { status=4;return false; }
            for (std::uint32_t participant=1;participant<40;++participant) if ((Read(Player(challenge,participant),0x64)&1U) && Read(Player(challenge,participant),4)==Read(request,11)) { status=3;return false; } // 현재 활성 subscriber 중복만 실패3이다.
            if (options_.autoBanCheaters && Read(request,30,1)>140) Need(hooks_.banAndSave)(Read(request,11));if (Need(hooks_.blockedSubscriber)(Read(request,11))) return false;
            if (Need(hooks_.blockedAddress)(Need(hooks_.addressText)(peer,false))) return false;
            if (options_.killLowVersions && options_.badMajorVersion>=static_cast<std::int32_t>(Read(request,4,2)) && options_.badMinorVersion>=static_cast<std::int32_t>(Read(request,6,2))) return false;
            std::uint32_t joined=1;
            for (;joined<40;++joined) { // 원문 첫 빈1..39만 선택하며 가짜0번 참가자를 생성하지 않는다.
                if (Read(Player(challenge,joined),0x64)&1U) continue;auto bytes=challenges_.Bytes(challenge);if (Read(bytes,kChallengeConnectionOffset+joined*4)) Assert("!chTransceivers[i]",2585);Put(bytes,kChallengeConnectionOffset+joined*4,connection);published=true;
                transport_.Connection(connection).sender=((Read(bytes,0)&0xffU)<<8U)|joined;transport_.SetBuffered(connection,true);InitializePlayer(challenge,joined,request);break;
            }
            if (joined>=40) { status=5;return false; }
            NotifyRoot(challenge,joined,true,static_cast<std::uint8_t>(Read(request,36,1)));LoginReply(connection,1,Read(request,11));
            auto info=Packet(0x26,0x505);auto mod=options_.mod.value_or(std::string{});mod=mod.substr(0,mod.find('\0'));Put(info,4,static_cast<std::uint32_t>(mod.size()+1),2);Put(info,6,16,1);Put(info,7,static_cast<std::uint32_t>(challenges_.Geometry().width),1);Put(info,8,static_cast<std::uint32_t>(challenges_.Geometry().height),1);PutText(info,9,mod);const auto at=9U+Read(info,4,2);Put(info,at,std::bit_cast<std::uint32_t>(challenges_.Geometry().origin[0]));Put(info,at+4,std::bit_cast<std::uint32_t>(challenges_.Geometry().origin[1]));
            for (std::size_t radius=0;radius<2;++radius) Put(info,at+8+radius*4,std::bit_cast<std::uint32_t>(challenges_.Geometry().radii[radius])); // 두 현재 float 반경을 원문 순서로 쓴다.
            Send(challenge,joined,info);
            if (static_cast<std::int32_t>(Read(request,4,2))<=options_.badMajorVersion && static_cast<std::int32_t>(Read(request,6,2))<=options_.badMinorVersion) {
                auto player=Player(challenge,joined);Put(player,0x64,Read(player,0x64)|0x400U);const auto extended=!(Read(player,0xb0,2)<7 && Read(player,0xb2,2)<2);const std::array<std::uint8_t,1> object{};
                const auto text=options_.badVersionMessage.value_or("~w~3~EYour client is obsolete.\r\nInstall the current Netstorm 11.01 build.\r\n");Message(challenge,joined,text,object,extended); // 폐기 공식 웹 링크 대신 현재 로컬 빌드 안내를 쓴다.
            }
            for (std::uint32_t battle=0;battle<10;++battle) if (Read(Battle(challenge,battle),0)&1U) { Send(challenge,joined,BattlePacket(challenge,battle,true));Send(challenge,joined,BattleDescription(challenge,battle)); } // 현재 유효 전투의 생성/설명 순서다.
            for (std::uint32_t participant=0;participant<40;++participant) { // 자기 자신을 제외한 현재 활성 참가자 전체를 번호 순서로 보낸다.
                if (participant==joined || !(Read(Player(challenge,participant),0x64)&1U)) continue;Send(challenge,joined,PlayerPacket(challenge,participant,true));const auto find=Need(hooks_.parseAddress)(options_.ipFind);const auto current=ChallengeDirectoryView(challenges_.Bytes(challenge)).Transceiver(participant);
                std::string address;if (options_.useIPReplace && Read(find,4)==Read(transport_.Connection(current).address,4)) { if (!options_.ipReplace) throw std::out_of_range("R IP 대체 문자열의 원문 NULL 읽기 거부");address=*options_.ipReplace; }else address=Need(hooks_.addressText)(transport_.Connection(current).address,false);AddressPacket(challenge,participant,address,joined);
            }
            Broadcast(challenge,PlayerPacket(challenge,joined,false));const auto current=ChallengeDirectoryView(challenges_.Bytes(challenge)).Transceiver(joined);AddressPacket(challenge,joined,Need(hooks_.addressText)(transport_.Connection(current).address,false),0xffffffffU);
            std::string flags;
            for (const auto& entry:std::array<std::pair<std::uint32_t,std::string_view>,5>{{{0x100,"DEMO"},{0x200,"PIRATE"},{0x400,"BADVER"},{0x800,"BADDIAG"},{0x1000,"BETA"}}}) if (Read(request,37)&entry.first) { if (255<=flags.size()+2+entry.second.size()) break;flags+=entry.second;flags+='.'; } //0044f820의 원문 진단 문자열 순서/한계다. 외부 검증 소비자는 아니다.
            Need(hooks_.log)(39,"Login by \""+Text(Player(challenge,joined),0x26)+"\"["+std::to_string(Read(Player(challenge,joined),4))+"]("+Need(hooks_.addressText)(peer,true)+")<"+std::to_string(joined)+">,"+std::to_string(Read(request,4,2))+"."+std::to_string(Read(request,6,2))+","+flags+",\n");
            const auto player=Player(challenge,joined);if (((Read(player,0xb0,2)==6 && Read(player,0xb2,2)>1) || Read(player,0xb0,2)>6) && messages_.news) { std::array<std::uint8_t,8> object;AllocationInput(object);Put(object,0,1,1);Put(object,4,messages_.newsChecksum);Message(challenge,joined,*messages_.news,object,true); }
            SendGuildName(challenge,Read(Player(challenge,joined),4),0xffffffffU);
            for (std::uint32_t participant=0;participant<40;++participant) if (Read(Player(challenge,participant),0x64)&1U) SendGuildName(challenge,Read(Player(challenge,0),4),source); //00451df0는 현재순회자의subscriber가아닌0번을조회하고원래source에보낸다.원문특이동작을보존한다.
            return true;
        };
        if (attempt()) return;LoginReply(connection,status,0);transport_.Flush(connection);transport_.Close(connection);
    } catch (...) { if (!published) { try { transport_.Close(connection); } catch (...) {} }throw; } // 미공개 소유권만 예외에서 반납하고 이미 공개한 현재 참가자 상태를 성공/원상복구로 합성하지 않는다.
}
}
