# webpj 메인 메뉴 배경을 Kencloud 구름 캐시로 (WB04-E1298, 2026-11-24)

[LEFT_JOBS](../LEFT_JOBS.webpj.md#next) 5-A '메인 메뉴 Kencloud'를 닫았다. 메인 메뉴 구름 배경을 정적 구름 GIF 타일에서 원본 `DrawKencloud`가 그리는 Kencloud 캐시로 바꿔 M1 오라클의 구름 차이 37,054픽셀(첫 실행 환영 창·메인 메뉴·Campaign·Demo·Credits·Help 팝업·Options 팝업) 및 General Help의 33,164픽셀이 **0**이 됐다.

## 원인

- 원본(cpppj `ClientMain.cpp` → `Client::ChangeFortPalette` → `TransitionFortPalette`)은 메인 메뉴 상태 설치에서 구름 파일을 읽어 512×512 캐시를 만들고(`BuildKencloud`, 게임 난수 801회) `cloudSpeed`번 `StepKencloud`를 돌린다. 한 번 이동마다 `phaseRows[phase]`로 시작하는 16행 간격의 행을 한 바이트 왼쪽으로 민다. 화면은 이 캐시를 카메라 원점 0으로 타일 복사한다(`DrawKencloud`, 투명 없음).
- 웹은 E1280부터 같은 State 3 설치에서 `_changeClientFortPalette`로 이 캐시를 이미 만들고 있었지만(난수 소비는 맞음) 메인 메뉴는 캐시를 쓰지 않고 구름 GIF 원본을 타일로 깔았다. 그래서 이동으로 밀린 행들이 어긋났다(차이는 31행 안팎 주기로 2~3행씩, 모든 메뉴 장면에서 같은 37,054픽셀). [dotnetpj가 같은 원인을 먼저 찾았다](dotnet-l4a-menu-cloud-20261111.md).

## 변경

- `www/core/client-kencloud-globals.js`: `cacheImage()` — 캐시가 있으면 `{ width: 512, height: 512, indices(캐시 바이트의 살아 있는 보기), opacity(전부 1) }`(IndexedSurface 그림 형식)를, 없으면 null을 돌려준다.
- `www/ui/main-menu.js`: `MainMenuScreen`이 선택 옵션 `cloudSource`를 받아 `render`에서 캐시가 있으면 그것을, 없으면 구름 GIF를, 그것도 없으면 0으로 채운다.
- `www/netstorm.js`: 메인 메뉴를 만들 때 `cloudSource: () => this._clientKencloud?.cacheImage() ?? null`을 연결한다.
- `tools/headless.mjs`: 구름 배경 분류용 `background()`도 같은 선택을 쓴다. M1 시나리오 두 파일(`m1-scenarios.json`, `candidate-225553df/m1-scenarios.json`)의 구름 기록(37,054·33,164)을 0으로 바꿨다. 영역 기록(Campaign 창 크기·Options 팝업 위치, cpppj 12차·후순위 항목의 차이)은 그대로이다.
- 미션 쪽 구름은 이미 같은 캐시를 `clouds.draw`로 그리고 있어 바뀌지 않았다.

## 검증

- M1 8장면(headless 앱 실행) 판정: **모든 비교가 기록과 일치**. 구름 차이 0, 전체 차이는 기록한 두 영역(campaign 19,997·options-popup 23,609)뿐이다. 변경 전에는 모든 장면이 37,054(General Help 33,164)였다.
- 새 단위 시험 `ClientKencloudGlobals_CacheImage`: 만들기 전 null, 만든 뒤 512×512·투명 없음, 이동 전에는 구름 GIF와 바이트 같음, `step()` 뒤에는 GIF와 달라지고 처음 얻은 보기도 같은 메모리를 가리킴.
- 새 e2e `e2e/menu-clouds.spec.js`(Chromium 4:3): 제목·버튼이 닿지 않는 왼쪽 띠(x<150)의 모든 픽셀이 구름 캐시와 같고 구름 GIF 타일과는 일부 다름을 확인한다(구름 GIF로 되돌리면 실패).
- 관련 Node 시험 293건(cpp-oracle·메뉴·도움말 계열)과 e2e(스모크·title·첫 실행·메뉴 구름) 통과. 번들(`www/dist`)을 다시 만들어 함께 커밋했다.

## 판정·대응표

- 두 행의 남은 이유가 이 항목뿐이라 `ClientRendererFrame.cpp`(394줄)와 `UberGump.cpp`(2,714줄)를 `partial` → `port`로 올렸다(메모의 E890~E984 시기 "잔여" 문구는 E951~E1264 앱 결선과 E1263 점검이 모두 닫았다). partial은 7 → 5행, 남은 줄 수는 11,425 → 8,317이다.
