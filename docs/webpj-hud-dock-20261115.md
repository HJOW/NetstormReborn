# webpj HUD 구분선·도킹 HudDock (WB04-E470, 2026-11-15)

기준선 `1f00a25a`의 `client/HudDock.cpp/.h`(280줄)를 `www/ui/hud-dock.js`로 옮겼다. 원본 HUD의 구분선(divider, 0x11c BYTE 노드) 생성·슬레이브 등록·재배치와 HUD 본체의 표시 모드·채팅 토글/크기·지도 영역 갱신이다. Uber와 생산 패널이 공유하는 `HudDividerGump`와 `HudDock` 두 클래스를 포함한다.

## 이식한 것

- `HudDividerNode`(확장 노드: axis·signed 제한 네 값·슬레이브 10칸 `[serial, 관계 플래그]`)와 `HudDockPart`(루트·세로·지도 구분선·채팅 구분선·지도·생산·채팅·지식 8종).
- `HudDividerGump(globals, color, hooks)`: 004c6c50 `construct`(signed→float32 배경 생성 뒤 vtable `0x514fd8`·플래그 `0x60200`·색 signed BYTE·axis를 쓰고 슬레이브 20DWORD만 지우며 제한은 보존, 활성 등록 1), 004c4790 `registerSlave`(첫 빈 칸·중복 허용, 가득 차면 진단 83 뒤에도 자식에 `0x40000`), 004c48f0 `attach`(popup 정리→마스터 확인→기본 부착→폭 연동 `0x10`→이동 사건 방송→슬레이브 10칸을 현재 serial로 재조회하며 관계 플래그 1/2/4/8 순서로 재배치).
- `HudDock(nodes, globals, color, hooks)`: 위 세 몸체를 위임하고, 004c7160 `registerNormalLayout`(열 번 등록·세 divider 제한·축 비트 보존), 004c6530 `handle`(release 비트 제거 뒤 채팅 토글 02000088·크기 02000005·HUD 모드 0200003d의 1/3/4·이동 0200003a/크기 변경 02000046의 지도 영역과 좌하단 영역 갱신, 나머지는 기본 입력 위임)을 수행한다. 직계 토큰은 객체 또는 함수(HudDockPart → 현재 토큰) 형태이며 캐시하지 않고, 전역은 복사하지 않고 같은 live 객체에 쓴다.
- 원문 정밀도: 폭/높이는 확장 값에 현재 scale을 곱한 뒤 한 번만 float32로 저장하고(`FST field→FMUL scale`), `right`/`bottom`/이전 간격 합은 slave 계산이 끝날 때까지 배정도로 유지하며 attach 인자만 float32로 줄인다. 표시 플래그는 unsigned로 유지한다.

## 기준선 뒤 판정 (3.6절)

`1f00a25a` 뒤 `HudDock.cpp/.h`·fixture·cpppj 시험에 대한 변경이 없다(base `1f00a25a`).

## 검증

- 원본 10.78 독립 관측 **2,150행**(`hud-dock-geometry-x86.tsv`, 진입 construct 80·register 34·reflow 1,572·hud 464, 각 이름 두 번 관측)의 노드 8개 0x11c raw 전체·22DWORD 전역·호출 순서·반환 EAX·스택 크기가 **첫 실행에서 모두 일치**했고 중단 효과 6행도 일치했다. 입력은 앞 6열(노드 raw·전역·인자·callback 효과)만 읽고 기대 열은 비교에만 쓴다(cpppj `HudDockTests`와 같은 방식). manifest `hud-dock-geometry`(20.5MB, 2,151줄)로 연결했고 기존 fixture를 수정하거나 다시 실행하지 않았다.
- 독립 시험 10건: 확장 노드 기본값/종류 번호, 생성자 처리기 검사와 노드 토큰 두 형태, 표준 배치 등록(순서·제한·기존 축 비트 보존), divider 생성의 배경 인자·색 부호 확장·슬레이브만 지움, 슬레이브 등록의 구멍·중복·가득 참(진단 83·진단 경계 없음)·저장소 불일치, 폭 연동 비트, HUD 모드의 플래그 비트 보존·release 비트·무한 좌표 ftol·좌하단 영역 지움, 경계 오류, 슬레이브 재배치 정밀도(관계 1/2/4/8/0x48/0x22 각각 입력 탐색으로 고른 값), 재부착 호출 순서.
- 변형 확인: 자동 변형 184개 중 원본 관측만으로 생존이 48개였으나(표준 배치 상수·진단/경계 조건·플래그 비트 보존·정밀도 등 관측이 닿지 않는 입력) 독립 시험으로 모두 잡았다. 수동 정밀도/호출 순서 변형 15개도 보강 뒤 잡혔고 남은 하나(`Math.fround(x)` 덧붙임)는 계약상 x가 이미 float32이므로 동등 변형이다.
- 첫 실행에서 어긋난 곳은 없었다(관측 2,150행 일치). 모듈 결함이 없었으며 독립 시험의 첫 실패 세 건은 시험 기대값(`Move`의 이동 비트 소거·토큰 0 오류 위치·구분선 오류 문구) 실수였다.

## 남은 것

`HudDividerInput`(004c49f0/004c4b00 입력·방송, fixture `hud-divider-input-x86.tsv` 13,399행)과 `UberHud.inc`·`UberRootGump`·`RawUberRootControls`·생산 패널 연결은 후속 단위다. 이 모듈의 `nodes` 함수 형태가 실제 Uber 필드를 가리키도록 그때 연결한다.
