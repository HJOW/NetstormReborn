# webpj State 세계 정리(RawStateWorldCleanup) 이식 — WB04-E228

2026-11-10, HJOW-Athlon. 게임 상태 전환 때 실행하는 State 세계 정리 부모(004b2df0, 1256 BYTE: 전송/정지 → 저장/Player → UI/Kernel → Core 0 선택 → Pool → Player 재구성 → 모드/기한/전역)와 그 안에서 쓰는 작은 정리 몸체 둘(004328a0 PlayerNoMake 경고 표 255 DWORD·004ab940 spot 지도 65536 BYTE)을 옮겼다. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절의 "순서4 이후로 넘기는 `o/` 단위(Fort 적재·지형/섬 …)"에 속하며 기준선 뒤 변경이 없고 원본 관측이 있어 E227 팔레트 전환에 이어 옮겼다.

- cpppj 기준선 `1f00a25a`. 소스(`RawStateWorldCleanup.cpp/.h`)·시험(`StateWorldCleanupTests`·`StateStorageCleanupTests` 의 평탄 정리 두 건)·fixture(`state-world-cleanup-x86.tsv`·`state-flat-reset-x86.tsv`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 를 저장소 루트에서 확인, 출력 없음).
- 근거 커밋: 소스 `a18bf611`(2026-10-26), fixture `a18bf611`/`d207940c`(2026-10-24~26).
- 원본 fixture: `state-world-cleanup-x86.tsv` 294행(두 FPCW 관측; 정상 194·명시 중단 100). 입력 15칸(seed·reason·Client·모드 비트·editor·mission·State·known·owner·Template·header 존재 마스크·정지 깊이·defaultSp·mission·변이)과 관측 범위 선언(전역 61곳의 폭 4/8/10/88 BYTE), 출력은 상태·정지 깊이·현재 Core 포인터 정체성·전역 전체 결과·세 Player 물리 전체·TYPE+c4 float·9×20 BYTE Template header·하위 호출 trace. `state-flat-reset-x86.tsv` 16행은 경고/spot × 두 x87 상태 × 네 패턴의 전체 Adler-32 와 앞뒤 인접 16 BYTE 다.
- 공유 장치: E3 `raw-player-core-data.js`(`selectPlayerCoreData`, 0048f3b0 Core 10 BYTE 선택/정리). 시험의 Core 선택 하위 장치가 이 JS 자식을 같은 전역/캐시/Player 뷰에 직접 연결한다.

## 1. 옮긴 것

`www/core/raw-state-world-cleanup.js`(DOM 없음, 10.78 전용)

- `StateWorldCleanupEffect`: 원문의 무인자/고정 this 자식 34개(CloseAllConnections 0x4e29c0, EndConnections 0x4e2a10, ResumeGame 0x460e30, …, InterpretOptions 0x435220)의 Native 주소 상수. 함수 포인터가 아니라 식별자이며 `hooks.effect(selected)` 하나로 분배한다.
- `cleanupStateWorld(reason, globals, hooks)`(004b2df0): 부모는 전역 DWORD 를 사용 위치마다 `globals.read32` 로 다시 읽고 `write32`/`write16`/`write64Bits`(BigInt)로 같은 실제 owner 필드에 직접 쓰며, 실제 UI/Kernel/Pool/Player/Template 자식은 선택한 필수 경계로 호출한다. 핵심 순서:
  1. reason 0 이면 전송 정리 둘, 아니면 Client 전역이 0 일 때 연결 1..8 끊기, 아니면 9; 이어 10·11. fort 를 캡처한 뒤 두 전역 0 쓰기. 모드 중복(fort 면 battle 또는 metamap, 단일 모드면 둘 다) 진단 0x77a 뒤 계속.
  2. 정지 깊이가 0 이 될 때까지 (게임 재개, 음향 재개) 반복하며 깊이를 매번 다시 읽음. 첫 자식 넷, 22 DWORD(5c84e8..) 0 쓰기, Template manager 조회와 8개 전역 쓰기, manager 가 있으면 Fort 저장(저장 복귀 때만 Template 번호를 다시 읽음)과 전투 저장(State 14 조건).
  3. Player 1..8: 8개 전투 필드(0x7c..0x98)를 현재 Player base 에서 재조회해 0 쓰기, AI/사제 초기화, known 이 없으면 다른 Player 초기화, 있으면 owner 에게만 SP(editor 면 0x483d9c80 float)와 표시 갱신.
  4. Template 정리, metamap 캡처 뒤 0 쓰기와 조건부 이탈, Cursor, 기본 SP 를 signed DWORD → float32 로 TYPE+c4 에 저장, 64비트 기한 0 쓰기들, 링크/Kernel/Gump/Uber/Core 정리, 5c89b4 하위 WORD 0, known 0, **Core 0 선택** → Pool 초기화 → 상태 자식들 → Graph → Player 재구성 → Zone.
  5. editor 캡처, 직접 전역 쓰기, known 이면 SP, 표시 갱신, State/mission 캡처 뒤 모드 전역 0 쓰기, State 가 29/33 이 아니면 Core 1 선택, 설정 재해석, 카운터 +1(32비트 감김), 벽시각 + 10.0 초의 double 기한과 나머지 전역 정리(0xffffffff 네 곳).
- `resetStatePlayerNoMakeWarnings(warnings: Uint32Array)`(004328a0): 첫 255 DWORD 만 0, 이웃은 보존, 짧으면 RangeError. `resetStateSpotMap(spots: Uint8Array)`(004ab940): 65536 BYTE 만 0, 짧으면 RangeError.
- 도달한 장치(전역 접근자·hooks·`effect` 등)가 없으면 기본 성공값 대신 그 위치에서 TypeError 로 거부해 앞선 효과를 남긴다. 부분 중단(하위 장치가 던진 예외)은 삼키지 않고 그대로 전파한다.

## 2. 검증

`tests/state-world-cleanup.test.mjs`(5건)

- 원본 관측 재생 A: 294행 전부(정상 194·중단 100). 행마다 새 장치(전역 DWORD/QWORD 저장소·세 Player 물리·Template header·Core 캐시/외부 10 BYTE)를 만들어 전체를 대조한다. 같은 호출 번호의 변이 입력(현재 값 교체·Player base 교체·Core 포인터 교체·중단)을 포함한다.
- 원본 관측 재생 B: `state-flat-reset` 16행 전부(전체 Adler-32 입력/출력과 앞뒤 인접 16 BYTE).
- cpppj 계약 포팅 2건: 선택된 Core 정리/Player 재생성 경계가 없으면 앞선 효과(known 0, 5c89b4 하위 WORD 0, Core 포인터 유지, Player 재구성·Pool 초기화 미도달, trace 가 `C:43b540:` 로 끝남)만 남고 TypeError, 짧은 호스트 물리는 거부하고 입력이 불변이며 정확한 범위만 쓴다.
- 웹 계약 1건(원본 fixture 에 Fort·전투 동시 저장에서 Fort 저장 장치가 Template 을 바꾸는 행이 없어 추가): 바뀐 Template 번호 2 가 전투 저장 인자로 전달됨.

최초 실패와 교정: 모듈은 처음부터 310행이 모두 통과했다. 모듈 사본을 바꿔 돌려 비교가 작동하는지 확인했다. 확인 뒤 원본으로 되돌렸다(diff 일치). 처음 변형 53건 중 2건이 살아남아 한 건은 시험을 보강했고 한 건은 동등 변형으로 판정했다.

- 실패한 변형: reason/Client 분기 반전, 연결 번호·진단 줄 번호 변경, 연결 10/11 제거, fort 캡처를 늦춤·모드 중복식의 `||`/`&&` 교환, 정지 반복 순서·구성, 22 DWORD 개수 ±1, Template manager 조건·Fort 저장 조건·전투 저장 조건(State 14)·Template 재조회 제거, Player 범위(0 시작·8 끝)·필드 범위(첫/끝)·기록 폭, known/owner 분기 반전, editor SP 비트·조건·표시 갱신 제거, metamap 이탈 조건, 기본 SP 부호 처리·TYPE 저장 제거, 25·5c89b4·known 0 쓰기 제거, Core 선택 인자·시점(Pool 뒤로 이동), Pool 인자, 54c8a0 1·59a8b0 1·editor/state 캡처 시점·mission 쓰기·29/33 조건, 카운터 증가·기한 초(10.0)·기한 쓰기 제거·마지막 전역 0xffffffff, 두 번째 MapMode 제거, 평탄 정리 길이 검사·채움 범위 ±1(각 1건).
- 적용되지만 통과한 변형(동등 변형): ① metamap 캡처를 `54c8a0` 0 쓰기 뒤로 옮김 — 두 주소가 다르고 둘 사이에 호출되는 장치가 없어 같은 값이다. ② 전투 저장 인자를 Fort 저장 복귀 때만이 아니라 항상 현재 Template 번호로 재조회 — Fort 저장이 호출되지 않으면 첫 읽기와 전투 저장 사이에 Template 번호를 바꿀 수 있는 장치가 없어 같은 값이다.

## 3. 경계와 남은 것

- 실제 Client/Session 에 연결하지 않았다. 전송·UI·Kernel·Pool·Player·Template·Gump·Zone 등 실제 자식 몸체와 호출 지점(게임 상태 전환)은 순서4·5 몫이다(PORT_MAP session 0). 시험은 하위 장치를 명시 관측 장치로 두고 Core 선택만 E3 의 실제 JS 자식을 연결한다.
- CD 10.72 대응(0042f5c0)은 같은 몸체로 가장하지 않으며 웹 범위 밖이다.

## 4. 다음 시작점

WB04-E229 는 3.1절에서 고른다. 기준선 뒤 변경이 없는 Fort/섬 단위(`SquidSpatial`·`RawTerrainGrowth`·`RawManaBoltRegular`·`RawTotalMade`)를 권한다. `RawTerrainBitmap` 은 시험이 에디터 섬 미리보기의 SHP 래스터(`client/EditorIslandBitmap`, 웹 제외)를 쓰므로 그 래스터 도구를 정할 때 함께 판단한다.
