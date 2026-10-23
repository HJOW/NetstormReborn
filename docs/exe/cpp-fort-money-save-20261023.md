# Fort Money 저장의 공유 난수와 독립 원본 검증

현재 및 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. AGENTS·공통/닷넷 인수인계를 다시 읽고 보호 PE와 현재 정밀 원문을 읽기 전용으로 분석했다. 원본/CD·디컴파일·기존 코드·CMake·인수인계·공유 빌드를 변경하지 않았고 게임/GUI/OS 원본 실행을 하지 않았다.

## 원문 범위와 부모 순서

10.78의 `004832c0`(정밀 C 105807행)은 `004558c0(0x3f2)`를 **한 번** 호출한다. 그 반환 signed DWORD를 `FILD`로 읽고 `0050d7c4`의 `333.0f`(`0x43a68000`)를 더해 `FSTP DWORD`로 저장한다. 그 네 바이트를 `0044cce0(stream, &value, 4)`로 쓴다. 따라서 저장값은 현재 SP 복사가 아니라 **333.0f~1342.0f 중 하나**이고, 실제 공유 게임난수 상태가 한 번 전진한다. 유효 결과는 모두 float32로 정확히 표현되므로 두 x87 정밀도에서도 저장 비트가 같다.

단독 Template 부모는 `004beec0`(152219행)이다. 순서는 **현재 this 첫 DWORD 한 번 읽기 → `Create(manager,"Money")` → 반환된 고정 stream으로 Money 저장**이다. 이 부모는 manager 0을 별도로 진단하거나 slot을 Player0으로 clamp하거나 `isMine()`를 검사하지 않는다. Create가 manager나 공유난수 상태를 바꾸면 이미 캡처한 manager/반환 stream을 유지하며, Money는 Create가 끝난 뒤의 현재 공유난수를 소비한다.

`isMine()`는 전체 snapshot 부모의 선행 계약이다. `004bf880`(152741행)은 manager 진단 `0x50e` → 현재 battle 진단 `0x50f` → 현재 Template owner로 `isMine()` 진단 `0x510` 뒤 Technology/Deck/Objects/Badges를 거쳐 Money를 생성한다. `004bf9a0`(152782행)은 manager `0x530` → 현재 fort `0x531` → `isMine()` `0x532` 뒤 TypeNames/Technology/Objects/Badges/Deck을 거쳐 Money를 생성한다. 양쪽 모두 Money 뒤 현재 manager를 다시 읽어 Mission으로 이어진다. 이 문서는 그 전체 부모를 실행·복원했다는 뜻이 아니며 단독 Money 부모에 선행 진단을 중복 추가하지 않는다.

CD 10.72 정밀 원문의 대응 단독 부모는 `004293b0`(35716행)이며 `0044c3c0(manager,"Money") → 004146d0`이다. CD 저장 몸체(18444행)는 `005120d4` DWORD를 인라인 저장하며 패치의 공유난수 생성과 다르다. 이는 정밀 C 정적 대조이고 이번 PE 실행 fixture와 새 모듈은 **10.78 전용**이다. CD 저장의 실행 동일성 검증은 포함하지 않는다.

## 제품 API와 연결 계약

새 제품 파일은 `cpppj/src/o/RawFortMoneySave.h/.cpp`이다.

- `SaveFortMoney(stream, GameRandom&, FortMoneySaveWrite)`는 같은 실제 게임난수 객체를 소비한 뒤 4BYTE를 쓴다. 현재 SP나 별도 난수 복사본을 받지 않는다.
- `SnapshotFortMoney(slot, GameRandom&, FortMoneySaveHooks)`는 고정 this 슬롯을 `manager(slot)`에 그대로 전달하고 `create(manager,"Money")`의 반환 stream을 writer에 전달한다.
- `FortMoneySaveHooks`는 `manager(slot)`, `create(manager,name)`, `write(stream,span)` 세 경계만 갖는다. write는 같은 실제 DataManager stream의 현재16BYTE/물리버퍼로 **기존 `WriteMemStream(fields,bytes,4,memory)`**를 실행해야 한다. 캡처한 pos/data 또는 현재 선택 노드에 단순 append하는 어댑터로 대체하지 않는다.

`0044cce0`는 signed `pos+4 > alloc` 진단 뒤 현재 data/pos를 다시 읽고 실제 네 바이트를 쓴다. 이후 현재 pos/end를 읽어 pos를 4 진행시키고 signed end 비교로 갱신한다. 새 제품은 이 일반 writer를 재구현하지 않고 명시 경계로 호출한다. 검사에서는 실제 기존 `RawMemStream.cpp`를 직접 연결해 독립 PE의 일반 writer와 대조했다. 도달하지 않은 하위 경계를 미리 검사하지 않아 Create/난수 소비 뒤 누락 오류가 나도 앞선 효과를 되돌리지 않는다.

등록할 검사 파일은 `cpppj/tests/FortMoneySaveTests.cpp`, fixture 매크로는 `NETSTORM_FORT_MONEY_SAVE_FIXTURE`이며 값은 `cpppj/tests/fixtures/fort-money-save-x86.tsv`의 절대 경로다. CMake/실제 Session 연결은 이번 하위 작업에서 변경하지 않았다.

## 독립 근거와 실제 실행 결과

`tools/decomp_fort_money_save.py`는 제품 C++ 소스나 바이너리를 읽지 않고 보호 10.78 PE의 다음 네 함수 전체를 격리 x86로 실행한다.

| 함수 | 실제 실행 범위 | 책임 |
|---|---|---|
| Money | `004832c0~004832f1` | 난수 호출, signed 변환,333가산,float32,4BYTE쓰기 호출 |
| Template 부모 | `004beec0~004beed3` | 현재manager/Create/반환stream |
| 공유 게임난수 | `004558c0~004558e7` | 0시드 대체,32비트 전진,1010 나머지 |
| 일반 stream writer | `0044cce0~0044cd3d` | signed진단,현재재조회,실제복사,pos/end |

Create와 진단 보고는 명시 외부 경계다. 원본의 정상 RET/callee-save/스택 및 x87 제어어/TOP·실행 원문 불변을 검사한다. 쓰기를 격리 스택/두stream/두물리버퍼/공유난수에 제한하며 그 밖의 코드 진입과 쓰기는 거부한다. 물리 범위를 벗어난 원본 메모리 접근·OS/할당·실제 파일·전체 snapshot 부모·게임 UI는 실행하지 않는다.

독립 고유 입력 **1,105개**, 두 x87 제어어 `0x27f/0x37f`로 **2,210회** 실행했다. **2,198회 정상 반환,12회 명시 부분 중단,137,678개 실제 명령**이다. 가능한 Money float 비트 **1,010종 전부**를 실제 관측했다. 시드0/부호비트/최대값, 슬롯 -2/-1/0/1/8/9/10와 owner 일치/불일치, manager0, 두 stream, signed end/capacity, Create/쓰기/진단 뒤 manager/난수/data/pos/end/capacity 변경을 포함한다. 최초 dry-run은 진단 식을 추정해 거부했으며 실제 PE 문자열 `pos+length <= alloc`를 직접 읽어 수정한 뒤 fixture를 처음 생성했다. 그 선행 실패 설명은 `extracted/cpp-fort-money-save-20261023-isolated/oracle-first-dry-run.txt`에 보존했다.

최초 fixture는 `cpppj/tests/fixtures/fort-money-save-x86.tsv`, native 보고서는 `cpppj/fort-money-save-native.json`이다. 앞11열은 명시 입력이고 뒤10열은 실제 PE 반환·난수·manager·두16BYTE stream·두64BYTE 버퍼·trace·스택·명령 수 관측이다. 파일이 있으면 덮어쓰기를 거부한다. `--dry-run`은 원본 전수를 다시 실행하고 파일을 쓰지 않으며, `--verify`는 최초 PE/정밀 C/도구/fixture SHA만 다시 확인한다. 검증 후에도 최초 fixture는 같았다.

- 보호 PE SHA-256: `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`
- 현재 정밀 C SHA-256: `af7cbf0b68e9d8e5aeb7068382c88d66201d3906c2926c54909ae14de4ad5646`
- 최초 fixture SHA-256: `8e00dee550343283727a3c3b8188a509ed00533545883f4223106fa39a11beb3`

자체 경로 `extracted/cpp-fort-money-save-20261023-isolated`에서 MSVC19.51 x64 `/std:c++20 /utf-8 /W4 /WX /O2`로 새 모듈·새 검사와 기존 일반 stream/게임난수를 격리 컴파일했다. **빌드 종료0/관측 경고0,3검사/실패check0**이다. 독립1105입력 전수, 하위 누락의 선행 효과·slot 비clamp·manager0, 동일 GameRandom과 SP저장소의 미초기화/배열 보존을 검사했다. 실행 파일 SHA-256은 `f8d7925e131a2d855b7a09559dd00194ecf209d4d5ca3c3eba21099a1a712e9f`이며 `build.log`, `tests.log`, `tests.json`, 사용한 response/main/fixture 헤더를 같은 경로에 남겼다. Python AST/UTF-8와 공백 검사도 통과했다.

이 결과는 새 Money 모듈과 독립 원문 범위의 대조다. 실제 Session/전체 snapshot 파일 저장·재로드·generic Editor·GUI 및 CD 복원·전체 사용자 완주의 증거로 확대하지 않는다.
