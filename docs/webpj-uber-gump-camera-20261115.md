# webpj UberGump 카메라 연결부 UberGumpCamera (WB04-E469, 2026-11-15)

기준선 `1f00a25a`의 `client/UberGumpCamera.cpp`(83줄)를 `www/ui/uber-gump-camera.js`로 옮겼다. `UberGump`의 멤버 다섯 개(`PollEdgeScroll`·`CommitCameraNow`·`CenterViewSpot`·`CenterViewPoint`·`ApplyCameraFrame`)가 이미 이식된 계산 본체를 현재 Client의 화면·렌더러·월드·버튼 장치에 연결하는 부분이다. 5-B 카메라 부품의 마지막 `client/` 카메라 단위다.

## 이식한 것

- `UberGumpCamera(host)` 클래스: 아직 이식되지 않은 `UberGump`/`Client`의 현재 장치를 빌려 주는 **host 계약**을 받는다(`client`·`buttons()`·`world()`·`gameMenuState()` 함수, `pointerBoundary`·`worldRedraw`·`rebuild` 공유 객체/상자). 계약이 부족하면 만들기 전에 `TypeError`다. 모든 장치 조회는 C++처럼 호출 시점의 현재 값을 다시 읽는다. 계약 전체는 모듈 머리 주석에 있으며 이후 `ClientGameplayGlobals`·`Client`·`GameWorld`를 옮길 때 이 계약에 맞춘다.
- `pollEdgeScroll(mouse)`: 설정 `edgeScroll`(초기 1)·`edgeScrollSpeed`(초기 20)를 현재 값을 기본 입력으로 읽는다(키 없음/같은 값은 유지). 다른 앱이 활성일 때는 켜짐을 0으로 `processEdgeScroll`에 준다. 초기화 비트는 `GumpPointerBoundaryState`와, 최초 시각(005cafc8)은 이 객체의 같은 멤버와 getter/setter 객체로 공유한다(복사본이 아님). 요청 훅은 현재 `rendererView`·`target`·화면 높이·`metaRebuild`로 `requestCameraDelta`(497e00)를 부른다.
- `commitCameraNow()`: 월드가 있으면 지도 영역, 없으면 전체 화면을 viewport로 쓰고(사각형 **값 복사**), 화면 폭/높이/모드 플래그는 호출 시작의 **사본**을 `commitCameraTarget`에 건넨다. 시차 분모와 마우스는 장치 효과 뒤에도 현재 값을 읽게 getter로 넘긴다. 커서 잠금/해제는 설치된 월드의 활성 커서가 있으면 그것을, 없으면 같은 DWORD만 증감하며 깊이 0 잠금과 해제 결과 0은 원문 경계 오류로 거부한다(감소가 먼저 일어남). 소프트웨어 마우스는 4a41c0 호출 지점의 현재 플래그에 비트가 있을 때만 핫스폿을 뺀 현재 마우스로 커서를 강제 갱신하고 비활성 앱은 그림이 없다. 비DIB 시차 장치는 원문 부모가 선택하지 않으므로 호출 시 오류다.
- `centerViewSpot(text)`·`centerViewPoint(x, y)`: ViewSpot 문자열을 `parseViewSpot`으로 읽고 433aa0 즉시 이동(`centerImmediately`)을 같은 `commitCameraNow`와 `worldRedraw`/`metaRebuild` 상자로 부른다. 현재 원점이 바뀐 경우에만 월드에 카메라 적용을 공개하고 재구성을 예약한다.
- `applyCameraFrame()`: 진단·화면 크기(호출 시작의 사본 상자)·세계 칸 수로 `presentCameraTarget`을 부르고(flip→두 목표 제한→적용), 원점이나 Q16 배율이 바뀐 경우에만 월드에 공개·재구성 예약한다.

## 기준선 뒤 판정 (3.6절)

`2e5e1c67`(20261108-20 "실제 메타 Poll과 공통 Renderer 및 카메라 장치 연결")이 `CommitCameraNow`의 clip 계산에 메타 대여(`gameplay.Borrowed()`와 owner challenge)일 때 `rawButtons_->MapViewRect()`를 쓰는 분기를 더했다. 온라인 메타 Session 몫이라 **① 웹 제외**다(대응표 base `2e5e1c67`). 그 밖의 `UberGump.h`/`UberGump.cpp` 변경은 이 파일이 아닌 허브 몫이며 `UberGump.cpp` partial 행이 판정한다.

## 검증

- 원본 10.78 관측 fixture는 이 연결부에 없다(cpppj에도 직접 시험이 없다). 계산 본체 다섯 개는 각자의 원본 관측 시험이 이미 대조한다: edge-scroll 135행·camera-center·497960 106+168행·Present·`parseViewSpot` 34행. 여기서는 **연결**(훅·전역·상태 공유·변경 때만 공개·호출 순서)만 새 독립 시험 15건으로 고정했다. 실제 `ClientCameraGlobals`·`Renderer`와 가짜 화면/버튼/월드로 구성하며, 시험이 호출 순서(설정 읽기→좌버튼 폴링→팝업 닫기→Shift 폴링, 잠금→재그리기→화면 이동→해제, flip→한계 적용→commit)와 변경 영역 병합 결과까지 직접 단언한다.
- 첫 실행에서 어긋난 곳은 시험 기대값 세 곳(변경 영역 병합 규칙 497580, `initializeNativeRenderer`가 worldRedraw를 1로 쓰는 초기값, 최초 시각 지움 위치)뿐이었고 모듈은 고치지 않았다.
- 변형 확인: 자동 변형 43개 중 첫 라운드 생존 3개(배율 위치 읽기 오프셋·두 축 `||`)를 낮은 바이트만 바뀐 배율과 한 축만 바뀐 이동 시험으로 보강해 모두 잡았다. 수동 의미 변형 40여 개(장치 사본/현재값 읽기·부호·깊이·지도 영역 값 복사·snapshot과 Renderer view 구분·설정 현재값 유지·상자 대체 등)도 보강 뒤 모두 잡혔다. 남은 동등 변형은 네 가지다: 켜짐 DWORD의 `>>> 0`↔`| 0`(`processEdgeScroll`이 다시 `>>> 0`을 적용), `screen.flags = flags` 덧쓰기(무효), `addBackgroundDirty` 플래그(비DIB 시차 장치가 항상 오류라 도달 불가), `addDirty` 플래그(본체가 항상 0을 넘김).

## 남은 것

실제 `UberGump`/`Client`/`GameWorld`/`ClientGameplayGlobals` 조립은 5-C·4-D 이후이며 이 모듈의 host 계약(`gameplay().cursor()`·`selection()`·`patchDiagnostic()`·`worldCells()`·`world().session().cursor()`·`mapView()`·`cameraApplied()` 등)을 그때 실제 객체가 만족해야 한다. 메타 대여의 지도 영역은 웹 제외다.
