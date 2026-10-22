// 원본 네 함수의 독립 전체80BYTE·패킷64BYTE·부모 출력·반환 길이와 제품 상태 모듈을 대조한다.
#include "RawSceneSupport.h"
#include "o/RawWindAviaryState.h"
#include <filesystem>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 공용 DWORD 입력 helper 두 번으로 불투명64비트 필드를 준비한다. 기대 결과를 계산하지 않는다.
void WindInput64(std::span<std::uint8_t> raw,std::size_t offset,std::uint64_t bits) {
    Put(raw,offset,static_cast<std::uint32_t>(bits));Put(raw,offset+4,static_cast<std::uint32_t>(bits>>32));
}
// 같은 입력 바이트만 준비한다. 기대 출력은 원본 fixture에서 읽고 제품 결과를 쓰지 않는다.
void CheckWindState(char kind,std::size_t expected) {
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/wind-aviary-state-x86.tsv").string();std::size_t count{};
    // 각 고유 입력의 전체 결과와 미작성 주변을 함께 대조한다.
    for (const auto& row:LoadFixture(path.c_str()).rows) {
        CHECK(row.size()==6);if (row.size()!=6 || row[0]!=std::string(1,kind)) continue;
        const auto args=Split(row[1],',');WindAviaryProcessFields fields;fields.bytes.fill(0xa5);auto& raw=fields.bytes;
        WindInput64(raw,16,std::bit_cast<std::uint64_t>(123.25));Put(raw,24,7);WindInput64(raw,32,0x7ff0000000000001ULL);WindInput64(raw,40,0x8000000000000000ULL);Put(raw,48,0xfedcba98U);Put(raw,60,77);Put(raw,64,50);
        std::array<std::uint8_t,64> packet;packet.fill(0xa7);std::uint32_t parent=0xdadadada;std::uint16_t result{};
        if (kind=='C') {
            CHECK(args.size()==3);if (args.size()!=3) continue;raw.fill(static_cast<std::uint8_t>(Number(args[2])));
            RawWindAviaryState::Initialize(fields,Number(args[0])!=0,std::array<double,3>{0,100.25,-5.25}.at(Number(args[1])));
        } else if (kind=='W') {
            CHECK(args.size()==5);if (args.size()!=5) continue;std::fill_n(raw.begin(),8,static_cast<std::uint8_t>(Number(args[4])));raw[8]=static_cast<std::uint8_t>(Number(args[0]));Put(raw,64,Number(args[3]));
            result=RawWindAviaryState::WriteProcess(fields,packet,Number(args[1])!=0,Number(args[2])!=0,0x43,0x49);
        } else {
            CHECK(args.size()==2);if (args.size()!=2) continue;raw.fill(static_cast<std::uint8_t>(Number(args[1])));
            // 원본 호출자가 공급한 불투명 패킷 바이트를 같은 순서로 준비한다.
            for (std::size_t index=0;index<packet.size();++index) packet[index]=static_cast<std::uint8_t>(index*17+3);
            packet[8]=static_cast<std::uint8_t>(Number(args[0]));parent=RawWindAviaryState::ReadProcess(fields,packet);
        }
        CHECK(Hex(raw)==row[2]);CHECK(Hex(packet)==row[3]);CHECK(parent==Number(row[4]));CHECK(result==Number(row[5]));++count;
    }
    CHECK(count==expected);
}
}
// 생성자가 초기화하지 않는 서버별 상태와 padding까지 원문18행과 비교한다.
TEST_CASE(WindAviaryState_ConstructorNative) { CheckWindState('C',18); }
// 모든 dirty BYTE와 별도 부모 경계의 원문1108행을 비교한다.
TEST_CASE(WindAviaryState_WriterNative) { CheckWindState('W',1108); }
// 수신 부분 갱신과 원문768행의 미작성 필드를 비교한다.
TEST_CASE(WindAviaryState_ReaderNative) { CheckWindState('R',768); }
// 손상 host 길이를 첫 쓰기 전에 거부하고 기본 프로토콜은 실제 PE의01/74를 따른다.
TEST_CASE(WindAviaryState_HostBoundsAndDefaultProtocol) {
    WindAviaryProcessFields fields;fields.bytes.fill(0xa5);const auto before=fields.bytes;std::array<std::uint8_t,64> packet{};
    CHECK(Throws([&] { RawWindAviaryState::WriteProcess(fields,std::span(packet).first(2),true,true); }));CHECK(fields.bytes==before);
    CHECK(Throws([&] { RawWindAviaryState::ReadProcess(fields,std::span(packet).first(8)); }));CHECK(fields.bytes==before);
    packet[8]=0x3f;CHECK(Throws([&] { RawWindAviaryState::ReadProcess(fields,std::span(packet).first(44)); }));CHECK(fields.bytes==before);
    RawWindAviaryState::WriteProcess(fields,packet,false);CHECK(packet[0]==1 && packet[1]==0x74);
}
