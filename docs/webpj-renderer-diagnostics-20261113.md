# webpj RendererDiagnostics — Renderer 후단 진단 Hash/Fire/Spot/Board (WB04-E384, 2026-11-13, 5-A 열세 번째 단위)

5-A 렌더러 프레임 본체 중 client **RendererDiagnostics**(cpppj `client/RendererDiagnostics.cpp/.h`, 기준선 108줄)를 `render/raw-renderer-diagnostics.js` 로 옮겼다. 기준선 뒤 cpppj 변경이 없다(base `1f00a25a`). 원본 Renderer(10.78 `436450`)가 플래그에 따라 월드 위에 덧그리는 네 개발용 후단(`DrawHash`·`DrawFire`·`DrawSpot`·`DrawBoard`)과 그 7/16/19 키 분배이며, 앞서 옮긴 E380 `DrawWater` 의 가시 영역(`drawWaterVisibleArea`)·finder 영역(`drawWaterFinderArea`)·네 선(`drawWaterRectangle`)을 그대로 재사용한다. 기본 UI 나 임의 진단 활성화는 없고 현재 플래그만 읽는다.

## 1. 규칙

- **`RawRendererDiagnostics.draw(kind, renderer, snapshot, width, height, flag, fireMode, worldCells, background, hooks)`:** `flag` 가 0 이면 카메라나 장치를 요구하지 않고 끝난다(원문은 0 관문이 viewport 나눗셈/물리 배열보다 먼저). 현재 참조(`flag`·`fireMode`·`worldCells`·`background`)는 `{ value }` 상자이다.
  - **Hash:** 네 단계(1/2/4/16)마다 가시 영역을 칸 단위로 나누고 `min(값÷단계, 세계 칸÷단계−1)`(아래 한계 0 은 추가하지 않음)로 자른 칸마다 두 꼭짓점(`imul` 하위 값→float32)을 snapshot 으로 투영해 clip → 사각형(색 0). Present 하지 않고 `background` 에 1 을 쓴다.
  - **Fire:** 가시 영역을 기존 `RawSquidFinder` 로 훑어 TYPE `flags1` 의 `0x800` 객체마다 — 모드 0 은 graph BYTE(0=색 0, 무효 그래프=색 7, 그 밖은 현재 팔레트 수의 나머지)로 clip 한 사각형과 안쪽 두 겹을 같은 색으로, 모드 비영은 8방향마다 현재 거리 WORD 배열을 다시 읽어 중심 칸과 이웃 칸(x+dx, y+dy)이 같을 때만 연결선(시작 −8,−6에서 방향 증분 ×(7,5), 색 7)을 그린다. 끝에 Present.
  - **Spot / Board:** x 바깥/y 안쪽 순서(오른쪽·아래 포함)로 칸마다 `spots[y*256+x]` 를 읽는다. Board 는 비트 8(색 5)/비트 6(색 1)의 세 겹. Spot 은 지도 안 칸의 해시 머리 표면 BYTE(지도 밖 x<0·y<0·>255 는 SID 0 슬롯)를 먼저 읽고, 비트 0x40(색 7)·0x80(색 0, 둘 다면 진단 0xfe)으로 6겹 — 앞 3겹은 선택 색, 뒤 3겹은 표면 BYTE 비트 0 이 있을 때 현재 회색(색 8). 끝에 Present.
  - 모든 정수 산술은 DWORD 감김(`add`/`sub`), 색은 매 조회마다 현재 배열에서 다시 읽는다. 범위 밖 접근은 `RangeError`, 선택한 효과에만 장치를 요구한다.
- **`dispatchRendererDiagnosticKey(event, special, specialAuxiliary, state, positionLog)`:** 7(보조 특수일 때 Fire 토글)·0x10(특수일 때 Shift 없으면 Spot 토글+redraw 1, Shift 면 기존 좌표 로그 지연 호출)·0x13(특수일 때 Board 토글) 키만 고르고 release 는 소비만 한다. Fire/Board 는 꺼질 때만 redraw 를 쓴다.

## 2. 웹 범위

- 앱 수명에 연결하지 않았다(`PORT_MAP` session 0). 호출자인 `Renderer` 본체·`ClientRendererFrame` 이 다음 5-A 항목이다.
- cpppj `RendererDiagnostics_DisabledAndLongOwnerNoSyntheticActivation` 의 장기 저장소(`ClientGameplayGlobals` 의 `DrawHash` 등 다섯 주소 해석) 부분은 그 전역 소유자를 옮기는 단계에서, `RendererDiagnostics_SharedPathWorkSurvivesSearchOwnerReplacement`(거리표를 두 탐색기가 이어 빌림)는 웹 `RawPathSearch`(E334) 쪽 시험이다.

## 3. 검사 (`tests/raw-renderer-diagnostics.test.mjs` 15건)

- **원본 독립 관측(10.78):** `renderer-diagnostics-x86.tsv` **252행**(Hash·Fire 모드 0/1·Spot·Board × flag 0/비영, 여섯 합성 객체를 실제 네 단계 해시에 걸고 순회는 제품이 함 — 96×80 전체 표면(행 padding 100)·최종 clip·배경 쓰기·호출 순서 Begin/Clip/Rect/Line/Lock/Unlock/Present/Assert)과 `renderer-diagnostic-keys-x86.tsv` **576행**(키 7/16/19, 수식·outside 마스크, 정지/Console 상태, DWORD 토글·redraw·Shift-P 좌표 로그)이 **첫 실행에서 전부 일치**했다.
- **관측이 가린 경계를 독립 입력으로 보강(변형 확인에서 발견):** 원본 관측은 spot 을 균일하게 채우고(칸 위치·행 alias·배열 경계가 안 보임), level-0 칸의 객체 표면 BYTE 가 모두 0 이고(Spot 의 회색 뒤 세 겹이 한 번도 안 나옴), 가시 영역이 작아(해시 칸 한계·지도 밖 칸·SID 0 표면이 안 보임) 변형이 많이 살아남았다. 이를 직접 만든 입력으로 채웠다 — 비균일 spot 의 `y*256+x` 위치와 65535/65536·−1/0 경계·행 alias, Spot 회색 3겹과 지도 안 경계 칸(0·255)/밖(256)의 표면 출처, Hash 의 세계 칸 한계(250.5 시점에서 36+9+4+1 칸, 세계 64 에서 4칸), 상수 비트(`flags1` 의 0x800 외 비트·spot 0x40/0x80·Board 8/6), 색 배열 길이 경계, graph 나머지, 8방향 이웃 칸이 각각 한 방향만 선을 만듦.
- 계약: flag 0 은 카메라/장치를 요구하지 않음, 선택한 효과에만 장치를 요구, Hash 는 Present 없이 배경에 1, 나머지는 Present 한 번, 색은 매번 현재 배열에서 읽음, 키 분배의 소비/redraw/Shift 분기와 release.

## 4. 변형 확인과 남은 경계

- 자동 연산자·상수 변형 193개와 손으로 쓴 구조 변형 53개(+보강 뒤 재확인 3개)를 적용했다. **처음에는 기준 시험이 깨진 상태로 변형을 돌려 0 생존이 나왔으나 기준이 실패하면 모든 변형이 거짓으로 "잡힘"이 되므로 무효였고**(기준 통과를 먼저 확인한 뒤 다시 돌렸다), 유효한 1차에서 54개 생존이 나와 위 보강 시험으로 대부분 잡았다.
- 남은 동등/도달 불가 변형: ① Fire 연결선의 좌표 진단(`cx<0||cy<0||cx>256||cy>256` → 진단 0x1f9) 전부 — finder 가 영역 안 객체만 돌려줘 정상 입력으로는 도달하지 않는 방어 진단, ② 객체 TYPE 범위 검사(`raw[10] >= types.length`) — finder 가 먼저 같은 오류로 거부하는 중복 방어, ③ Spot 에서 색 번호 `index` 의 값(7/0)과 `surface`·`selected` 초기값 — Spot 은 `index` 의 부호만 쓰고 Board 는 `selected`/`surface` 를 쓰지 않음.
- 다음 5-A 항목: 렌더러 프레임 본체 — `Renderer`(partial 494)·`ClientRendererFrame`(396)·`ScreenPresentation`·`Screen`(partial adapt 818).
