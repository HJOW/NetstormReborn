# TypeNames 로드와 별도 cleanup 복원

2026-10-24 **신규 독립 준비·미등록**이다. AGENTS/공통·닷넷 인계 상단을 다시 확인했고 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. 보호10.78 PE와 기존 정밀 C를 읽었으며 원본/CD/AGENTS/정밀 산출물은 변경하지 않았다. 게임·GUI·OS실행·공유빌드·CMake·LEFT·git 변경은 없다. 신규 격리 콘솔 검사만 실행했다.

## 최초 독립 관측

[도구](../../tools/decomp_fort_type_names_load.py)로 제품 작성 **전에** [fixture](../../cpppj/tests/fixtures/fort-type-names-load-x86.tsv)와 [보고서](../../cpppj/fort-type-names-load-native.json)를 고정했다. 제품/검사에 맞춰 재생성하지 않았다. 도구는 최초 결과 덮어쓰기를 거부하고 `--verify`가 PE/C/functions/helper/tool/fixture SHA를 확인한다.

| 항목 | 결과 |
|---|---|
|본문|004bd800~004bd939,314BYTE |
|별도 cleanup|004fb820~004fb827의8BYTE→004bcc50~004bcc56의7BYTE |
|본문 입력/관측|332입력×027f/037f 두정밀도=664관측,정밀도 차이0 |
|정상/명시 중단|626정상/38본문 중단,동일 부분상태에서 별도cleanup38정상 |
|실제 명령|2,410,126개,Select664/할당324/TYPE6596/진단178회 |
|물리 중단|count-read16/hash-read8/현재conversion쓰기2/new반환0 2 |
|callback 중단|Select/진단/new/TYPE1/TYPE2,각2 |
|fixture SHA256|`4c824ec764b85b73ef1d2387c7e6043611b767e983f66abc3f1a98da2d0ef86c` |
|보호 PE SHA256|`a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236` |

Unicorn은 위 실제 명령만 실행한다. Select0044ceb0/new004e4391/TYPE0049a840/진단004e0620은 명시 하위 장치다. hash/zero255/첫 일치/현재 전역·stream 쓰기/깊이는 실제 PE가 수행한다. 정상반환에서는 stack/FS 복원도 확인했다. 원본이 경계 검사 없이 읽거나 쓰는 곳의 물리 범위 부족/할당0은 **쓰기·읽기 직전 명시 중단**이며 원본 자체의 정상 거부나 예외라고 주장하지 않는다.

본문 중단 상태를 먼저 기록한 뒤,같은 현재 깊이에서 **예외 funclet/helper만 별도로** 시작해 cleanup 결과를 기록했다. OS CxxFrameHandler/예외 dispatch/전체 stack 전개를 실행한 것은 아니다. 본문 중단과 cleanup 뒤를 같은 성공으로 합치지 않는다.

## 원문 순서

정밀 C `Netstorm.c:151111~151196`,functions.tsv의314BYTE 및 PE를 대조했다.

1. 현재 loadingDepth005c89b8를 증가한 뒤 **고정 this의 현재 manager**를 읽어 Select(manager,"TypeNames",1)을 호출한다. 이 부모 자체에는 manager0 진단이 없다. 반환 stream 주소를 보관한다.
2. stream.data=0 또는 stream.end=0이면 **기존 typeConversion005c89bc를 free 없이0으로 덮고** 깊이를 감소해 반환한다. 이 경로에는 count/할당/TYPE 조회/기존 pointer 진단이 없다.
3. data+현재pos의 count **BYTE를 먼저 캡처**하고 pos를1 증가한다. 현재 conversion이 비영이면 Template.cpp608(`!typeConversion`) 진단한다. 복귀하면 계속한다. 진단이 포인터를0으로 바꿔도 new를 생략하지 않는다.
4. new(255)의 반환 주소를 현재 conversion에 쓰고 **반환 블록 자체**를63DWORD+WORD+BYTE로 정확히255BYTE zero한다. 할당 callback이 바꾼 기존 conversion값을 zero 대상에 사용하지 않는다.
5. **진단·할당·zero 뒤 같은 stream의 현재 data/pos**로 hash 배열 주소를 한 번 캡처한다. signed pos>end면 end=pos로 쓴다. 원문은 여기서 count×4만큼 pos를 전진하거나 end를 확장하지 않는다. count0이어도 new/zero와 현재 타입 순회는 실행된다.
6. 현재 signed typeCount00541348가 양수일 때 타입0부터 TYPE(index)+4의 첫NUL까지 이름을 읽는다. 각 BYTE를 **signed char**로 넓히고 `(문자위치&3)*8`만큼 밀어DWORD 감김 합을 만든다.20자에서 자르거나 대소문자를 정규화하지 않는다.
7. 캡처한 count개의 저장 hash를0번부터 검색해 **첫 일치 저장칸만** 갱신한다. 그 쓰기 직전에 **현재 conversion 포인터를 다시 읽어** 현재 타입 번호의 낮은BYTE를 쓴다. 이후 현재 signed typeCount를 다시 읽고 다음 타입을 진행한다. 같은 hash의 현재 타입이 여러 개면 나중 타입이 같은 첫 저장칸을 다시 덮는다. 저장쪽 두 번째 중복칸은 자동 채우지 않는다.
8. 정상 종료는 **그 시점 현재 깊이**를 하나 줄인다. 진단/TYPE callback이 깊이를 바꾸면 진입값으로 되돌리지 않는다. 별도 예외 cleanup도 현재 깊이를 하나 줄이는 동일7BYTE다.

fixture는 빈 data/end/기존 pointer,슬롯0/1/8,manager0,현재 count 음수/0/1/7/16/260,저장 count0/1/4/255,중복hash/현재이름/고비트 원시문자/20자초과/대소문자,두stream과서로다른data,포인터/pos/end/typeCount/manager/깊이/이름/hash의 callback 변경을 포함한다. 타입256이상은 원본처럼 낮은BYTE로 저장한다. 모든 callback의 임의 조합이나 원본 없는 물리 메모리를 정상 입력으로 확장한 검사는 아니다.

## API와 수명

[RawFortTypeNamesLoad.h](../../cpppj/src/o/RawFortTypeNamesLoad.h)/[구현](../../cpppj/src/o/RawFortTypeNamesLoad.cpp)은 다음 API를 제공한다.

```cpp
LoadFortTypeNames(slot, FortTypeNamesLoadGlobals{depth,conversion}, hooks);
UnwindFortTypeNamesLoad(globals, hooks);
```

두 전역은 실제 `uint32_t&`다. hooks는 고정 manager/Select/실제stream16BYTE/data범위/new255/현재conversion범위/현재typeCount/현재NUL이름/진단608/깊이동기화다. 도달한 필수 경계만 요구하고 빈 성공을 만들지 않는다. depth 동기화는 다른 월드 소비자에게 같은 쓰기를 전달하는 관측 경계이며 별도 게임 callback을 추가하지 않는다.

모듈은 변환표 할당을 소유하거나 free하지 않는다. 상위 Client/Storage가 실제 비영토큰→255BYTE 할당과 그 수명을 소유하고,Objects/Technology/Deck의 getter/free/clear가 **같은 conversion DWORD**를 빌려야 한다. `FortTemplate::typeConversion`의 사전 계산 배열이나 `FortMetaLoadState::hasTypeConversion` bool을 현재 포인터의 소유자로 대신하지 않는다. 빈 섹션에서 덮어쓴 기존 포인터를 이 함수가 편의상 free하는 동작도 추가하지 않는다.

선택 stream의 물리 주소는 동기 호출 동안 유지한다. data는 hash 주소를 캡처한 뒤 callback이 stream.data를 바꿔도 이전 캡처 주소의 실제 할당이 계속 유효해야 한다. TYPE 이름 span은 해당 hash 계산까지 유지하고,conversionBuffer는 그때의 실제 현재 토큰을 해석해야 한다. 토큰0/해제된주소/물리 부족은 명시 거부한다.

`LoadFortTypeNames`는 **본문 부분효과만** 보존하므로 callback/물리 거부 시 증가한 깊이도 남는다. 상위가 원본 예외 cleanup을 조합하는 경우 실패한 본문에 한 번만 `UnwindFortTypeNamesLoad`를 적용한다. 정상 감소 뒤 다시 호출하면 이중 감소다. 깊이의 시작값 복원이나 변환표·pos/end rollback은 하지 않는다. 실제 C++/OS 예외 정책과 단위 검사용 명시 중단 표식을 구별해서 연결해야 한다.

## 검사와 후속 통합

[FortTypeNamesLoadTests.cpp](../../cpppj/tests/FortTypeNamesLoadTests.cpp)는 기대열을 읽기 전에 입력열만으로 제품을 실행하고,본문의현재8필드/전체9헤더·두16BYTEstream·두1088BYTEdata·세255BYTE블록·변경가능이름두개/trace를 대조한다.38중단은 먼저 본문 부분상태를 비교한 뒤 별도 cleanup의 state/trace와 raw 불변을 비교한다. 원문 fixture 외에는 미선택 경계 미연결,기존 pointer 진단 미연결,254BYTE 할당에서 마지막BYTE 전 중단,이름NUL 누락과 명시 cleanup을 검사한다.

격리 경로는 `extracted/fort-type-names-load-20261024-isolated`다. 최초 `/std:c++20 /EHsc /utf-8 /W4` 빌드1/검사1은2검사·실패0이었다. 이후 원문 zero의DWORD/WORD/BYTE 물리 접근폭도 보존하도록 검토 보완했고,**최종 빌드2 종료0·관측 경고0,검사2개·실패0**을 확인했다. build1.log/tests1.log와build2.log/tests2.log를 모두 보존한다. 공용 빌드/기존 소스는 변경하지 않았다. 최초fixture/report/tool은 그대로이며 `--verify`로 확인한다.

후속 [기존 Editor 설치 제안](cpp-generic-editor-install-20261024.md)의 Technology prepare와 Objects/Deck의 TypeNames 경계를 이 부모에 연결한다. 실제 SelectSection(슬롯,"TypeNames",1)의 같은 stream/token/data와 현재 allocator를 공급해야 한다. 중첩 Meta loader의 catch가 자기 깊이를 복구하는 것만으로 TypeNames의 cleanup까지 대체하지 않는다. prepare 어댑터가 TypeNames 실패를 잡아 별도cleanup을 한 번 실행한 뒤 같은 예외를 전파해야 한다. 이후 동일 포인터의 해제·clear/UI 순서와 전체 Editor Player tail0046e980은 별도 부모 범위다. 이번 모듈 완료를 전체 Editor 설치·저장·GUI·사용자 완주 성공으로 확대하지 않는다.10.72 대응은 기존 정밀 C의00427e00 표기만 기록하며 이번 독립 실행은10.78 대상이다.
