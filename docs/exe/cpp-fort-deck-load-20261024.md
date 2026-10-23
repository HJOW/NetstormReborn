# Fort Deck 로드 부모004bf190

2026-10-24 신규 독립 복원이다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 기존 공유 소스·Storage·Session·CMake·LEFT·공유 build·GUI·git은 변경하지 않았다. 신규 원문 fixture를 먼저 고정한 뒤 C++를 작성했다.

## 정확한 부모 순서

`loadingDepth++` → 고정 this manager0 진단 `0x4d6` → this에서 얻은 slot1..8만 owner로, 그 밖은0 → `Deck.Get(owner)` → 반환0 진단 `0x4d8` → **현재 manager 재조회** → `Select("Deck",1)` → **캡처한 같은 Deck**의 `ClearEntries` → `Open(0)` → **현재 fullDeck DWORD** 분기다. Open은 실제 같은 게임 RNG를 소비하므로 FullDeck 경로에서도 앞선 호출을 생략할 수 없다.

fullDeck 비영이면0044fbf0 thunk→0044f680 실제 FullDeck이 선택된다. 부모는 이때 stream의 data/end를 읽지 않으며 TypeNames/변환표 해제/Refresh도 호출하지 않는다. 별도 primitive 모듈과 원문 관측이 이를 구현할 예정이고, 이 부모의 하위 hook만으로 FullDeck 본체 완료를 주장하지 않는다.

fullDeck0일 때 선택 반환 stream의 **현재 data/end가 모두 비영**인 경우에만004bd800 TypeNames를 호출한다. 그 뒤 같은 반환 stream의 현재 data/pos에서 count BYTE를 읽고, 각 항목은 **type/chance/power/remaining 네 BYTE**다. 논리 end를 반복마다 검사하지 않으며 실제 물리 범위를 넘는 입력만 호스트에서 명시 거부한다.

type<246인 경우에만 현재 conversion255BYTE를 조회한다. type0도 변환되면 추가될 수 있고,246~255는 표를 사용하지 않는다. 변환0 항목도 나머지 세 BYTE를 소비하며 추가 호출만 생략한다. 한 항목의 처음 data/pos는 뒤 세 BYTE까지 유지하고 다음 항목은 append callback 뒤 현재 data/pos/conversion을 다시 읽는다. power BYTE는 signed 인수로 미리 바꾸지 않고 DWORD로 영확장해0044f330에 전달한다.

루프 끝에는 **현재 conversion**이 비영이면 그 토큰을 free에 넘긴다. free callback이 전역을 바꾸어도 복귀 뒤 현재 conversion을 무조건0으로 쓴 다음 `Refresh(owner)`를 호출한다. 정상 마지막에 당시 현재 depth를 한 번 감소한다.

## 모듈과 장치 수명

[RawFortDeckLoad.h](../../cpppj/src/o/RawFortDeckLoad.h)·[본문](../../cpppj/src/o/RawFortDeckLoad.cpp)의 `LoadFortDeck(slot,{depth,conversion,fullDeck},hooks)`는 TypeNames와 같은 depth/conversion DWORD 참조를 빌린다. 상위 소유자는 변환표 토큰과255BYTE 할당을 유지하고 `conversionBuffer(token)`은 순수 조회로 연결한다. host pointer를 DWORD로 자르지 않는다.

`access`는 기존 Storage의 `AccessSection`과 같은 동기 fields16BYTE/버퍼 빌림 형태다. 지역 fields 사본이나 현재 선택 섹션으로 캡처된 반환 stream을 대체하지 않는다. TypeNames callback이 manager/current를 바꾸어도 이미 받은 Deck과 stream은 유지한다.

명시 하위 중단/물리 실패에서 본문은 앞선 쓰기와 증가한 depth를 유지한다. `UnwindFortDeckLoad`는 별도004fb860→004bcc50의 현재 depth 감소만 구현한다. 정상 감소와 중단 상태·별도 cleanup 결과를 구별하며 이 모듈이 전체 Windows SEH를 실행했다고 하지 않는다. 실제 C++ 외곽에서 예외 cleanup을 조립할 때 정상 경로에 중복 감소를 추가하지 않는다.

## 독립 원문과 격리 검사

[원문 도구](../../tools/decomp_fort_deck_load.py)는 보호 PE의 **본문431BYTE + 별도 cleanup15BYTE**를 실행한다. [fixture](../../cpppj/tests/fixtures/fort-deck-load-x86.tsv)는 **510입력×2정밀도=1,020행**, 정상998/명시 중단22/별도 cleanup22,532,894명령이며 정밀도 차이0이다. Select/Deck Get/Clear/Open/FullDeck/Append/TypeNames/free/진단/Refresh는 명시 외부 장치다.

최초 fixture SHA256은 `b7f42acf232eec9c919c691b42ce44b8cc17668db27bdb4406c818d45051d436`이다. [최초 보고서](../../cpppj/fort-deck-load-native.json)의 전체 근거 SHA를 `--verify`로 검사하며 제품 결과로 덮어쓰지 않는다. 하위 callback은 manager/선택 반환/현재 fullDeck/stream data·pos·end/변환표/depth를 변경하거나 중단한다. slot−2/0/1/8/9·manager0·Deck0·fullDeck 상위비트·end1/ffffffff·count255·type0/245/246/254/255·현재 free/Refresh 후 변형을 포함한다.

[FortDeckLoadTests.cpp](../../cpppj/tests/FortDeckLoadTests.cpp)는1,020행의 반환/실제 trace/현재 상태/두 stream 네 필드/별도 cleanup을 대조한다. 추가 검사에서 FullDeck의 미선택 장치 누락, Open 누락 시 Clear 뒤 중단, 논리 end를 넘긴 직접읽기의 실제 물리 끝 거부를 확인한다.

[격리 CMake](../../extracted/cpp-fort-deck-load-20261024-isolated/CMakeLists.txt)만 구성해 **최종 Release 경고·오류0,2검사/실패check0**이다. 공용 라이브러리도 링크하지 않았다. 로그와 제품/fixture SHA는 [검증 기록](../../extracted/cpp-fort-deck-load-20261024-isolated/verification.json)에 보존한다. 원본 게임·OS 함수·GUI·원본 SEH분배기는 실행하지 않았다.

## 통합에 남은 부분

TypeNames004bd800은 별도 `RawFortTypeNamesLoad`의 실제 소유자/현재 conversion과 결합한다. Editor tail0046e980은 이 부모의 범위가 아니다. `Open`은 기존 `RawDeckDraw::Open`의 같은 RNG를 재사용할 수 있지만,0044f330의 저장 복원용 네 인자 추가와0044f680의 현재 TYPE 순회는 별도 primitive 관측/모듈이 필요하다. 부모만 연결하고 이 장치들을 빈 성공으로 두지 않는다.

Storage의 실제 Deck 섹션 선택과같은 Production Deck 할당을 결합한 Editor 최초 설치는 후속이다. 현재 결과는 독립 부모 모듈이며, 사용자 Editor 진입·전체 저장/재로드·원본 전체 동일성 완료로 확대하지 않는다.
