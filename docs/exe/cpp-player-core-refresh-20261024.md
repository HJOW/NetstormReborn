# 현재 Core 기술과 level 하한 갱신 독립 복원

> 대상 해석 정정(후속388): PE005412ec의 실제 초기DWORD는165(daisType)다. Player152는005412b8이다. 정상VT00507950의+80(005079d0)은004478f0이며 기존RawTempleRitual::Count를 호출한다. 앞선 자기Player152의state상위3비트 level 또는RawSquidLevel::Get 재사용 제안은 이 정상대상에 적용하지 않는다. 최초Native/알고리즘은주소005412ec를명시입력으로이미사용했으므로변경하지않는다.


2026-10-24 **후속386 독립 준비**다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. PATCH10.78 **0044fc00의357BYTE**(`44fc00-44fc35;44fc40-44fd6e`)를 신규 제품 전에 PE 격리 실행으로 고정했다. 정밀 SourceMap의 CD10.72 대응은 **0045f0d0**이며 이번에는 CD 몸체를 실행하지 않았다. 공유 기존 소스·CMake·LEFT·git·공용 build/lib·GUI와 보호 원본/CD는 변경하거나 실행하지 않았다.

새 [공개 API](../../cpppj/src/o/RawPlayerCoreRefresh.h)와 [본문](../../cpppj/src/o/RawPlayerCoreRefresh.cpp), [검사](../../cpppj/tests/PlayerCoreRefreshTests.cpp)는 실제 Current Core/Technology/Finder/Pool/TYPE/가상 level 장치를 빌린다. 실제 Session이나 전체 checkpoint에는 아직 연결하지 않았다. **격리 build3 종료0·W4 관측 경고오류0, 신규4검사/실패check0**이다. 전체 사용자 게임/Editor 설치의 동일성 판정으로 확대하지 않는다.

## 제품보다 먼저 고정한 원문

[최초 도구](../../tools/decomp_player_core_refresh.py), [TSV](../../cpppj/tests/fixtures/player-core-refresh-x86.tsv), [보고서](../../extracted/cpp-player-core-refresh-20261024-native.json)는 **321입력×양x87모드=642관측·168112명령**이다. 부모 외에 실제 **48f0d0/48fc60/4b2270/4b1b20/44daa0/49a840/42b280/40eb40**, 총9몸체를 실행했다. level 가상+80과 assertion은 명시 입력/외부 경계다. 부모 결과를 제품에서 가져오거나 그 결과로 fixture를 다시 쓰지 않았다. 양 정밀도 출력 차이는0이다.

정상588·명시중단20·NULL 실제 미매핑10·논리 길이중단24를 구별한다. 기록은 **Core16BYTE×3 전체, rawPool400BYTE×2 전체, 현재 Core와 Pool/capacity/owner/TYPE/containedType/known/metamap, 하위 순서**다. 기술bit−2/−1/0/31/32/33/극값, kindWORDffff, ownerDWORD256와 playerTypeDWORD256, capacity0/1/2/3/8, free/dead, 첫째/둘째 level 반환 차이·signed극값, 현재Core/Pool/상한/타입 변경, 진단·자식 중단과 **실제 부모 재진입**을 포함한다. owner256은 관측자가 명시한 Player64KiB의 실제0물리를 읽는 입력이며 정상 게임의9Player 유효성 증거가 아니다.

**최초 논리 길이중단24행은 동일대조에서 제외한다.** Unicorn mem-write hook의 emu_stop 뒤 해당 DWORD store가 커밋되어 짧은 실제 물리 fault와 같지 않음을 관측 직후 확인했다. 최초 자료/도구는 그대로 보존한다. 별도 [guard 도구](../../tools/decomp_player_core_refresh_guard.py)·[TSV](../../cpppj/tests/fixtures/player-core-refresh-guard-x86.tsv)·[보고서](../../extracted/cpp-player-core-refresh-guard-20261024-native.json)를 역시 제품 전에 고정했다. A/B Core를 실제 한page 끝에 놓고 뒤page를 미매핑한 **20관측·628명령**이며 정상4·guard fault16이다.

guard 길이9의4행에서도 Unicorn은 page 안 앞3BYTE를 쓴 뒤 fault한다. 이4행의 부분store를 C++ span 전체DWORD 검사의 동작이나 실제 CPU 하드웨어 보장이라고 주장하지 않는다. 길이0/5/6 fault12와 정상4, 총16행의 전체 물리/trace를 C++와 대조했다. 최초642 중 지원618과 guard16, **총634관측은 모든 기록 칸이 일치**한다. 제외된 논리24/부분store4도 원시 자료와 검사 분류에 남긴다. C++는 짧은 host span/NULL을 선택한 load/store 전체폭 검사에서 거부하며 쓰기 전 물리 계약을 명시한다. 고정cache C를 임의로 짧게 빌린 사례도 Native cache 할당 자체가 짧았다는 증거로 세지 않는다.

## 보존한 실제 순서

1. 실제 현재Core getter 뒤 **+6 DWORD0**을 쓴다. 다음 현재Technology 부모를 조회하고 기본true ContainedFinder를 독립 생성한다. 후보마다 **현재Pool의 flagsBYTE+20 뒤 kindWORD+18**을 읽는다. kind는 flags4가 없어도 읽는다. flags4일 때 현재 TYPE+1ec를 읽고 −1만 생략한다. 종류WORDffff를 부모에서 임의 무효처리하지 않는다.
2. 선택된 bit는 **다시 현재Core getter→Ch(같은this,bit,1)**다. bit 범위 밖이면 Ch.cpp346 진단 후 같은 캡처Core의 +6 DWORD에 **bit&31** OR를 쓴다. 진단이 현재 Core를 바꿔도 이미 넘긴 this는 바뀌지 않는다. 다음Finder 진행은 이 효과 뒤다.
3. iterator는 **index1/cachedEnd현재capacity**로 만들고 최초0040eb40에서 먼저2로 증가한다. free bit0만 cachedEnd 안에서 건너뛴다. 각 유효 관문은 현재 capacity와 현재 Pool 주소의 unsigned32 산술을 사용한다. 후속 free-skip 상한은 최초 cachedEnd를 유지한다.
4. 현재 candidate.typeBYTE가 현재 playerTypeDWORD와 같고 ownerBYTE가 현재 ownerDWORD와 같으면 **dead 검사 전에도 첫 level+80**을 호출한다. 첫 반환이 signed maximum보다 클 때 현재Pool을 재조회하고 그 같은 index의 dead bit2를 읽는다. dead가 없으면 **현재Pool의 같은 index에서 두 번째 level+80**을 호출하고 그 반환을 다시 비교하지 않고 maximum으로 채택한다. 두 번째 반환이 더 작거나 음수여도 첫 반환으로 대체하지 않는다. 각 가상호출 뒤 Pool을 다시 읽는다.
5. 최종 lower=maximum−1은 DWORD 감김 뒤 signed로 비교한다. 첫 getter의 Core+5보다 lower가 크면 두 번째 getter의 Core+5에 낮은BYTE를 쓴다. 그 외에는 **두 번째 getter Core+5를 읽고 세 번째 getter Core+5에 쓴다.** 현재Core가 바뀔 수 있어 이 중복을 줄이지 않는다. 재진입은 각 부모의 Finder/iterator/max를 별도 로컬로 유지하고 같은 현재전역/가상장치를 공유한다.

## 실제 하위 재사용과 Session 접합

`RefreshPlayerCore(hooks)`의 core는 기존 **ReadPlayerCoreData** 또는 같은 Client owner getter에 연결한다. 신규 검사도 기존 실제 `RawPlayerCoreData.cpp`를 직접 격리 링크해 진단/현재 span 재조회를 재사용한다. technologyParent는 기존 **RawPlayerWorld::CurrentTechnology**를 사용할 수 있다. 이 함수의9Player 물리 계약과 Native의 잘못된 owner 원시 주소산술은 구별해야 한다.

begin은 기존 **RawContainedFinder::Begin/Next**를 **Begin마다 새 finder+현재SID 저장**으로 감싸 current/next를 반환하면 된다. 한 전역 finder를 재진입마다 덮지 않는다. 현재 RawContainedFinder는 같은 SidPool 객체를 고정 참조하므로 원시 Pool 교체 Native 입력 전체를 그 클래스로 표현했다고 주장하지 않는다. fixture 장치는 현재 rawPool/containedType을 읽는 하위 경계이며 부모의 Core/max 기대를 계산하지 않는다. Native에서는 그 Finder/기본true 필터 몸체까지 직접 실행했다.

pool/playerType/owner provider는 현재 실제 저장소의 **순수 조회**다. pool.bytes는 같은 SidPool.Bytes, capacity는 같은 현재 Capacity, address는 해당 물리의 안정32비트 식별자여야 한다. 임의 parsed 객체목록이나 base가 바뀌었는데 옛 bytes를 반환하는 조합은 계약 밖이다. level은 전달한 **현재 같은 raw객체**의 실제 가상+80을 실행해야 하며 상수 최대값/빈 성공을 넣지 않는다. 정상165/VT507950은 기존 **RawTempleRitual::Count(004478f0)**다. hook의 playerType 이름은 실제005412ec의현재다이스타입을뜻하며 PlayerWorldState.playerType(005412b8/152)와연결하면틀린다. technologyBit는 현재 TYPE의 +1ec DWORD를 읽고 signed비트를 반환한다.

부모가 항상value1로 호출하는 Ch42b280의 선택 부분은 이 모듈의 private `SetTechnology`에 복원했다. 독립 일반 Ch API/value0·잘못된value의 다른 진단까지 새로 완료했다고 세지 않는다. 기존 공용 Ch 몸체를 찾지 못해 이 좁은 선택 부분을 포함했으며 Native에서 실제 Ch127BYTE 전체를 실행한 value1 경로와 비교했다.

Session의 CoreWrite `hooks.refresh`는 후속 root가 이 API에 연결한다. 같은 Player/Core/Pool 수명이 필요하며 이번 모듈에 새 canonical 상태·RNG·Core 캐시를 만들지 않았다. 선택하지 않은 장치를 미리 요구하지 않고, 누락/진단/가상중단까지의 +6/기술bit 효과를 복구하지 않는다. 실제 Player를 반납하기 전 Core 선택해제 순서는 [장기 수명 설계](cpp-client-long-lifetime-next-20261024.md)와 [Native State 정리](cpp-state-world-cleanup-20261024.md)를 따른다.

## 검증과 최초 실패 보존

최초 probe1은 NULL의 미매핑 store 관측 hook이 없어 종료1이었고 fixture/report 생성 전이었다. 해당 로그를 보존한 뒤 NULL 실제 오류를 분류하여 probe2에서 최초642를 생성했다. guard probe1은 종료0이다. 최초 build1은 LF batch/잘못된 정규화 상대경로와 검사 auto 선언의 Windows unsigned형 불일치를 기록했고, batch CRLF/검사 선언만 고친 build2는 종료0이었다.

최초 test1은 owner256용 하위 Player 물리를 Native의 명시64KiB와 달리1620BYTE로 구성하여 **300check 실패**했다. 제품 부모/Native 기대는 수정하지 않고 검사 입력 물리를 맞춘 뒤 **build3/test2가 종료0·4검사/실패0**이다. 다른 작업자의 읽기 검토에서도 getter/Pool 재조회·두 번째 가상반환·진단캡처this·범위 분리에 명백한 오류를 찾지 못했다.

전체 SHA/최종로그/격리실행물은 [검증 manifest](../../extracted/cpp-player-core-refresh-20261024-isolated/verification.json)에 기록한다. 신규 공용 등록은 `src/o/RawPlayerCoreRefresh.cpp`와 `tests/PlayerCoreRefreshTests.cpp`, 매크로 **NETSTORM_PLAYER_CORE_REFRESH_FIXTURE** 및 **NETSTORM_PLAYER_CORE_REFRESH_GUARD_FIXTURE**에 두 최초TSV 절대경로를 공급한다. 기존 `RawPlayerCoreData.cpp`가 필요하다. 등록/실제 Session 배선/공용 빌드는 root 후속이며 이번에는 수행하지 않았다.
