# Deck Open·저장 Append·FullDeck 본체

2026-10-24 신규 독립 복원이다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. [Deck 부모](cpp-fort-deck-load-20261024.md)의 최초 fixture를 먼저 동결한 뒤 별도 primitive fixture를 고정하고 제품을 작성했다. 기존 공유 파일·공용 CMake·LEFT·공유 build·GUI·git은 변경하지 않았다.

## 실제 여섯 몸체와 API

[RawDeckRestore.h](../../cpppj/src/o/RawDeckRestore.h)·[본문](../../cpppj/src/o/RawDeckRestore.cpp)은 다음 세 진입을 제공한다.

| API | 원문/동작 |
|---|---|
|`OpenRestoredDeck(raw,random,seed)`|0044f250의 기존 `RawDeckDraw::Open`을 같은 실제352BYTE/공유 RNG에 직접 적용한다. seed0일 때만 `Next(10000)`을 한 번 소비하며 seed/open을 쓴다.|
|`AppendRestoredDeckEntry(raw,type,chance,power,remaining,hooks)`|0044f330→0044f160의 저장 네 인자 추가다. 입력을미리BYTE로 자르지 않는다. 현재 count/진단 뒤 실제5BYTE Entry와 현재 집계 쓰기를 직접 수행한다.|
|`LoadFullDeck(raw,random,{typeCount,nuggetType,geyserType},hooks)`|0044f680→0044f290→0044f160의 실제 FullDeck이며 세 집계DWORD 초기화·현재 TYPE 순회·같은 `Next(5)`·실제 기본 Entry 추가까지 포함한다. 큰 Append를 빈 hook으로 남기지 않는다.|

FullDeck의 TYPE 첫 조회는 **group+0x9c**, group10이면 두 번째 조회의 **flags2+0xec/bit0x100**을 사용한다. `RiftTypeRecord.flags1`은+0xe8이므로 대신 연결하면 안 된다. group10/flags2 조건에서 제외될 타입이라도 현재 **너겟005412c0(통상154)** 또는 **간헐천00541240(통상122)**이면 포함한다. 이 두 값은 포함 예외이며 반대 의미의 제외 목록이 아니다.

`hooks.type(index)`는 같은 현재 `RiftTypeRecord`의 `{group,flags2}`를 반환한다. 동일index도 원문이 두 번 부르면 두 번 조회한다. typeCount는 최초와 매반복 끝에서 현재 signed DWORD를 다시 읽으며, TYPE callback이 count를0으로 바꿔도 이미 시작한 한 타입은 끝까지 처리한다. same RNG를 빌리고 별도 난수열/임의 chance를 만들지 않는다.

## 저장 폭과 진단 뒤 현재 값

두 Append는 중복 순회에서 진단이 바꾼 현재 count를 매번 다시 읽는다. 용량 진단 뒤에도 count를 다시 읽어 Entry 주소를 정한다.0044f160은 **power BYTE→type BYTE→chance BYTE→remaining BYTE→draw-count BYTE0** 순서로 먼저 쓴 다음 type/chance/**signed power**/remaining 절단 진단을 순서대로 한다. 뒤 진단은 이전 callback이 바꾼 현재 Entry를 다시 읽는다.

부모의 파일 power는 DWORD로 영확장되므로 저장BYTE128은 인수128로 들어오고, Entry의 signed BYTE−128과 달라 원문 power 진단이 날 수 있다. 인수 `0xffffff80`은 같은 signed BYTE와 일치한다. 진단을 숨기기 위해 loader에서 power를 미리 signed 변환하지 않는다.

Entry 진단이 바꾼 현재 chance 합계/count를 다시 읽어 처음 캡처한 chance/1을 더한다. count66~69의 Entry 주소가 실제352BYTE 안에 있으면 원문의 metadata 겹침 쓰기까지 보존한다. 그보다 벗어나는 실제 물리 접근은 앞선 진단/변경 뒤 호스트 오류로 거부하며, 이를 원문 자체의 안전 검사라고 하지 않는다. 기존 `RawDeckDraw::Append`의 사전 count 캐시를 이 신규 경로에 재사용하지 않았다.

## 최초 독립 관측과 검증

[원문 도구](../../tools/decomp_deck_restore.py)는 **0044f250/0044f330/0044f680/0044f290/0044f160/004558c0 여섯 실제 몸체**만 실행한다. TYPE 주소 getter와 진단만 명시 경계이며 RNG/Entry/하위 Append는 실제 명령이다. 부모431BYTE는 이 fixture에 섞지 않는다.

[최초 fixture](../../cpppj/tests/fixtures/deck-restore-x86.tsv)는 **430입력×2=860행**, 정상844/명시 중단16,658,526명령, 정밀도 차이0이다. Open의 seed0/비영·RNG0/상위비트, 네 인자의0/127/128/255/256/음수 비트, 중복/용량과 현재count 변경, metadata 겹침, Full 타입수−1/0/1/6/66/67/260·group/flags2/예외타입·TYPE callback 변경을 포함한다.

fixture SHA256은 `0222adba1b6ab80ef308aca07adadfdaf7baf8300ae0af5dd186594190ebcc0a`이며 [최초 보고서](../../cpppj/deck-restore-native.json)의 전체 SHA를 `--verify`로 보존한다. 최초 도구/fixture의 `excludedA/B` 입력열 이름은 비선택 조건의 **너겟/간헐천 포함 예외**를 뜻하는 당시 관측 식별자다. 제품 API는 실제 타입 이름으로 구별했다.

[DeckRestoreTests.cpp](../../cpppj/tests/DeckRestoreTests.cpp)는860행의 전체352BYTE/공유 RNG 상태/현재 전역/TYPE·진단 trace/부분중단을 대조한다. 추가 검사에서 비영seed의 무소비·seed0의 실제 소비, 정확한 signed power DWORD, Entry 쓰기 뒤 진단 미연결 오류,352BYTE 물리 끝 거부, 타입수0의 미선택 장치 누락을 확인한다.

[격리 CMake](../../extracted/cpp-deck-restore-20261024-isolated/CMakeLists.txt)는 신규 모듈/기존 실제 RawDeckDraw·SpStore 소스만 컴파일하고 공용 라이브러리도 링크하지 않는다. API 필드명을+0xec의 `flags2`와 실제 너겟/간헐천으로 검토 정리한 **최종 빌드3 Release 경고·오류0,2검사/실패check0**다. 최초 빌드/검사도 보존하며 [검증 기록](../../extracted/cpp-deck-restore-20261024-isolated/verification.json)에 최종 SHA를 기록한다.

부모의 clear/Get은 같은 실제 `RawProductionDeck::ClearEntries/Get`, Open은같은RNG·`Bytes(token)`, append/full은 이 모듈의 같은 `Bytes(token)`으로 연결한다. TypeNames/현재conversion은 별도 모듈 소유권을 공유한다. 실제 Session/Storage 바인딩·전체 Editor 최초 설치·사용자 저장·EditorPlayers tail0046e980·GUI 플레이 완료를 이 독립 결과로 주장하지 않는다.10.72는 이 신규 fixture의 실행 범위 밖이다.
