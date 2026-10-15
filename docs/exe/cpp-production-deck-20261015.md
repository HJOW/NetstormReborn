# 현재 Player의 생산 Deck과 기술 알림 — 2026-10-15

현재 PC/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-15**다. AGENTS와 양 LEFT를 다시 읽었다. 기존 full/refined 원문·보호 PE/TYPE를 읽었으며 원본 게임/GUI/OS를 실행하지 않았다. AGENTS·보호 원본·dotnetpj를 바꾸지 않았다. 이 단위는 신규 파일만 작성하며 기존 Session/Client/CMake의 편집·공용 빌드·커밋은 통합 담당자가 맡는다.

## 실제 저장소와 원문

`0044f970`은 현재 생산 가능한 건물 수를 즉석 계산하는 함수가 아니다. 현재 `templateOwner00540cac`의 **Player+0x68**가 가리키는 `0x160BYTE Deck`을 `0044f700`으로 얻고, `count+0x14c`개의 **5BYTE 항목 중 type BYTE가 전체 인자 DWORD와 같은 항목**을 찾는다. power/허용 표/상태를 Contains의 추가 필터로 쓰지 않는다. Deck의 변경은 별도 실제 생산 갱신 시점에 발생한다.

`RawProductionDeck`은 같은 `RawPlayerWorld::Records()/State()`를 참조하고 Player의 opaque DWORD와 대응하는 블록을 하나만 소유한다. 최초 할당 바이트는 필수 공급 경계로 받으며 묵시적인0채움을 넣지 않는다. 원본 생성 초기화는 정확히 **5DWORD**다. 코드상의 배열/클라이언트 포인터를 호스트 포인터로 해석하지 않는다. 소유자가 끝날 때 다른 모듈이 교체한 Player 포인터를 지우지 않고, 아직 자신의 블록을 가리키는 칸만0으로 정리한다.

| 필드 | 위치/폭 | 의미 |
|---|---|---|
| 항목 |0부터66×5BYTE| type BYTE, chance BYTE, signed power BYTE, remaining BYTE, order BYTE |
| count |`+14c DWORD`| signed 활성 개수 |
| totalChance |`+150 DWORD`| 양의 power 항목의 chance 합계 |
| remaining |`+154 DWORD`| 원본 Deck의 현재 남은수 |
| seed |`+158 DWORD`| 원본 open/추첨 상태 |
| open |`+15c DWORD`| bit1 여부 |

`0044f200`은 count→seed→chance→remaining→open을0으로 쓴다. 항목과 두 BYTE의 물리 틈은 초기화하지 않는다. `0044f230`은 count/chance/remaining만0으로 바꾼다. `0044f290`의 중복 진단→5BYTE 쓰기→DWORD 합계/개수, `0044f3d0`의 signed 음수 power→0/조건부+1/없으면chance1 항목 추가, `0044f430`의 먼저 복사→음수 power 압축을 복원했다. 잘못된 양수 개수/블록 밖 쓰기/모르는 현재 토큰은 안전한 명시 예외다.

`004929e0`은 UI 창을 직접 띄우는 함수가 아니다. 현재 `battleOwner00540c70`의 `CurrentTechnology0048fc60`과 `004acac0(mask4,type,index0)`를 호출하고 **반환 kind WORD가0일 때** 현재 부모를 다시 읽어 `004afa70(parent,type,0,4,0x10,0)`으로 만든다. 기존 `RawPlayerTechnology::Ensure`의 타입 assert와 SID 반환 판정을 그대로 대체 사용하면 type0에서 원문과 달라진다. 새 `RawIndicateTechnology`는 기존 실제 Carrier/Contained 조회에서 kind WORD를 읽고 기존 실제 `RawContainedCreate`로 전체 생성/부착을 호출한다. PlayerKnown0의 잘못된 부모 등 호스트 풀 범위 계약 밖 입력은 빈 성공으로 바꾸지 않는다.

## 같은 현재 Factory 목록의 재계산

`RawCampaignProductionBindings`는 같은 `core.pool/hash/types/bookkeeping` 및 `RawPlayerWorld`·`RawProductionDeck`·`RawTotalMade`·현재 contained 전역을 빌린다. `core.bookkeeping.factories`는 원본 `0055a4d4`의 실제 공통 PostPop/삭제 목록이다. 별도 건물/생산 bool 목록을 만들지 않는다.

`Refresh(owner)`는 `0044f7c0` 순서대로 같은 Deck을 얻고 Fort/Battle 진단을 수행한 뒤, 미션0이며 fullDeck 비영이면 기존 블록을 유지한다. 그 외에는 기존 power만-1로 바꾸고 totalChance를0으로 쓴다. 현재 `getTeleportTechFree`가 비영이면 `005412bc`의 실제 **Mog153**을 활성화한다. 설정 이름만 teleport이며 asset의 teleportEffect134와 혼동하지 않는다.

현재 Factory 목록의 owner BYTE가 전체 owner DWORD와 같은 항목만 읽는다. **flags2&0x200은 Vortex/신전**이다. 신전은 같은 TotalMade 허용 표가 `0054126c`의 현재 sunWalker133을 허용할 때만 그 항목을 활성화한다. 일반 Factory는 flags1&0x04000000일 때 기존 actual ContainedFinder로 flags0x10의 현재 kind WORD를 읽는다. **일반 Factory의 mode0은 허용 표를 조회하지만 금지된 kind도 Deck에 포함한다.** Contains에 일괄 허용 필터를 추가하지 않는다.

일반 Factory의 `+b4`는 실제 최종 VT `00509148/00509210`에서 같은 `00453f60`을 호출한다. 이12BYTE는 full/refined 함수 표에서 누락됐지만 보호 PE의 실제 바이트 `8b01ff908000000083c002c3`와 가상표를 직접 확인했다. 몸체는 현재 가상+80의 level에2를 더한다. 새 연결은 해당 두 VT만 허용하며 다른 가상 몸체를 성공 기본값으로 돌리지 않는다. 각 distinct kind마다 남은자리를 한 번 줄이고, 중복 제품도 power는 실제 반복 횟수만큼 더한다.

마지막에는 음수 power를 압축하고 signed power가 양수인 항목의 BYTE chance만 DWORD 감김으로 합산한다. 원본 mode0에서 빈 문자열에 호출하는 `004735c0`은 실제로 빈 문자열을 유지하고 게임/표시 효과를 내지 않는다. C++의 계산 경로는 그 결과와 동일하며 native 관찰에서는 해당 함수의 전체 원문까지 실행했다.

## 독립 원본 관찰과 검사

- `decomp_production_deck.py`: **248개 고유 입력·두 정밀도·496정상 반환**. Init/Clear/Get/Contains/Append/Activate/Compact 및 Indicate의 전체 상위와 현재 Technology/kind WORD/기본 true 필터를 실행했다. CRT new와 assert, 기존 `004afa70` 생성은 명시 ABI 관찰 경계다. 실제 생성/부착 하위 전체를 이 관찰 수에 합산하지 않는다.
- `decomp_production_refresh.py`: **242개 고유 입력·두 정밀도·484정상 반환**. `0044f7c0/00475530/004752b0(mode0)`·빈 Mana 문자열·실제 가상 capacity와 작은 하위는 모두 원문이다. 설정값과 assert만 명시 외부 경계다. 정상 반환/보존 레지스터/x87 균형, 전체 Deck 및 풀/TYPE/Player/금지 표 불변을 확인했다. 두 도구의 `--verify`가 종료0이다.
- `ProductionDeckTests.cpp`: 저장된 두 fixture를 보호188TYPE/실제 RawSurfaceWorld·Player·Factory·ContentFormHost·SP 통지 접두에 대조하는2개 검사, 현재 Player별 수명/허용 표와 Factory 변경/물리 경계의3개 검사를 작성했다. 실제 생성/부착은 빈 callback으로 성공시키지 않는다. kind256 등 원본 생성 관찰 경계 이후의 호스트 TYPE 범위 오류는 명시 예외로 구분한다.

통합 담당자가 CMake에 등록한 뒤 첫 집중 검사에서는5개가 모두 실제 표면 월드 생성자의 `표면 삭제 연결 객체 타입 범위 오류`로 중단됐다. 검사 장면이 빈 `BridgeConnectState`를 공급해 connectorType0이 된 입력 전제 오류이며, `ProductionLinks()`가 실제 Session과 같은 bridge82/island94/stalag95/connector155/noIsland157 전역을 공급하도록 교정했다. 제품의 범위 검사나 fixture 대조를 약화하지 않았고 초기 로그 `extracted/goal-fort-options-production-focused.log`를 보존한다. 재빌드/재검사 결과는 통합 담당자의 후속 기록을 따른다.

그 다음 집중 검사의 재계산2개에서294CHECK가 실패한 원인은 새 입력의 신전 타입을129로 잘못 지정한 것이었다. 보호 TYPE에서 rainVortex는124이고129는 rainCannon이다. 검사와 native의 현재 노드/합성 TYPE를124로 교정해242개를 두 정밀도로 다시 실행했으며484정상 반환과 `--verify`가 통과했다. 최종 Deck/반환/이벤트 fixture는 교정 전과 바이트 단위로 같아 SHA `46828b77fdbe491d6ed8f06fa815a86c7fde5d60ad5cd3d124c98c14957f0c7d`를 유지한다. 제품 몸체/대조 조건의 변경은0이고 새 고유 native를 더하지 않는다. 초기 fixture/report/집중 로그는 `extracted/production-refresh-before-actual-vortex124*`에 별도 보존했다.

Native의 Factory/내용물/Deck/해시 없는 목록 상태는 합성 입력이며 실제 Pop/제작/운반/GUI 트랜잭션을 수행한 것으로 계산하지 않는다. C++ 현재 Factory 목록 입력도 실제 Factory/Content 생성자 뒤 raw 연결/등록을 명시하며 그 자체를 자연 생산 완료라고 주장하지 않는다.

root 통합에서 같은 Session의 실제 Player/금지표/세 카운터/Factory/기술 내용물을 연결하고, 현재 Factory 저장 공간이0이던 실제 결함을 원본500DWORD 예약으로 교정했다. 새 생산5개와 실제 Session2개를 포함한 관련 집중42개 실패0이다. 근거와 전체 회귀 최종 결과는 [중단 인수인계](cpp-pause-handoff-20261015.md)를 따른다.

실제 표시 Gump의 `0047d890` 명령→현재 GumpInput/dirty 처리와 생산 메뉴 생성 수명은 미완료다. 기존 UberGump는 Construct 행이 아직 비활성이다. 이 계산 모듈/Session 연결만으로 Early Missions나 캠페인의 생산 UI/자연 진행을 완료로 표시하지 않는다.
