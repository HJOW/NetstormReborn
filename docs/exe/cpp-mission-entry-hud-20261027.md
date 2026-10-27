# 미션 진입의 미니맵 중간 crop과 HUD 출력 순서

단위485는 **The War Begins 진입 직후 첫 화면에서 종료되던 오류**를 수정했다. 484의 짧은 실제 실행에서 발견했으며, 같은 스테이지를 실제 Campaign 메뉴에서 선택해도 재현됐다. 수정 후 두 진입 경로와 실제 ESC Restart가 정상 동작한다.

작업 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 기존 디컴파일·원본/CD·AGENTS·검사/fixture를 수정하지 않았다. 실제 게임 검증은 사용자 허용 범위인 격리 복사본의 짧은 진입/재시작으로 한정했고 음향 장치는 열지 않았다.

## 원인과 변경

첫 진단은 미니맵의 평면 소스 주소 **-30080**, 할당 **16384BYTE**, 현재 복사 **80×75**, 목적지 **(0,693)**다. cropX0/cropY-235의 조합이다. `ClampMinimapAttachment`는 큰 임시 높이에서 음수 crop을 만들 수 있으며, 다음 부착에서 크기가 줄어도 그 음수를0으로 바꾸지 않는 원문 동작이다. 실제 `RawMetadisplayDirty::Refresh`의 Rebuild가 현재 카메라/크기로 crop을 다시 계산한다.

원본 루프는 미니맵 Refresh 후 공통 표시를 실행한다. 그런데 호스트 `UberGump::Compose`가 브리핑/팝업/로딩 배경을 합성하며 공통 HUD를 먼저 그려, Refresh 전 중간 crop을 읽었다. 실제 HUD가 있으면 이 사전 배경 합성에서 Draw를 실행하지 않고 **기존 Renderer의 공통 Overlay로 제출**하도록 변경했다. 그려지는 트리·정렬·clip·원문 crop/블릿 계산은 유지한다. 실제 HUD가 없는 기존 표시 경로도 유지한다.

`GumpPixelTarget::RawBitmap`의 할당 밖 읽기 거부는 유지하고, 실제 주소/할당/크기/목적지를 오류에 포함했다. 할당 밖 BYTE를0으로 만들거나 crop을 임의 clamp하지 않는다. [기존 독립 블릿/guard 근거](cpp-minimap-blit-runtime-20261023.md)와 원문 계산 fixture를 그대로 재생했다.

## 검증

- Release 진단 build1/최종 build2 모두 **경고/오류0**이다. 신규 검사 없이 기존 집중 **59고유/65관측·실패0**을 확인했다. 미니맵 이름을 포함한6개가 다른 필터에서도 실행된 것을 중복 고유 검사로 세지 않았다. 최초 집계기의65고유 가정 실패/첫 집계 스크립트를 보존하고 집계만 정정했다.
- 미니맵·공통 Draw·메뉴·초기 생산/HUD·실제 섬 창·484 Guide/World 수명 검사를 통과했다. 실제3,274등록과 기존 검사/fixture 전체 바이트는 동일하다. 전체 CTest는 이번에 반복하지 않았으며 직전483의3,271개/실패0과 구별한다.
- **같은 최종 게임의 실제4실행:** 메인 메뉴, 실제 Campaign→Struggle For Freedom→The War Begins→Play Mission→수초 플레이, `--mission TheWarBegins`→수초 플레이→ESC/Game/Restart→재진입, 실제 Early1 메뉴 진입→수초 플레이→Restart→재진입을 모두 종료0/상한 미도달로 확인했다. War 직접 진입/재시작은500프레임/약12.91초, Early1은530프레임/약11.93초다. 실제 War 플레이 화면도 PNG로 확인했다.
- 원본/CD/기존 검사 **3,195파일·AGENTS 바이트동일**, 최종 소스/검사 **2,846파일 동결 동일**이다. 단위484와 이번 최초 메뉴/진단 실패를 보존했다. 원격은 다시 fetch/pull해 `Already up to date`를 확인했고 push하지 않았다.

최종 게임 SHA256은 `3786327ad5cd5b93e4b890eb1da181caa09c169d2cb41647d8b59c566b10b924`, 검사 실행 파일은 `d871a8da23a61a2b75159b90f7291d86951cded66dd06c4090a0cda77a87ade5`다. [실행 보고서](../../cpppj/mission-entry-hud-20261027.json), 원시 `extracted/cpp-mission-entry-hud-485/`을 본다. 신규 Native CPU 관측 없이 기존 원문 fixture·실제 실행 경로를 대조했다.

## 다음 작업

484의 Guide/World 분리와 이번 표시 수정을 기반으로 c2 일반 입력, 같은 작업본 namespace, Battle14의 실제 RequestFortEdit, State1 편집 복귀를 연결한다. Add Island/새 맵/저장 재진입의 짧은 실제 GUI도 확인할 수 있다. 전체 미션 자연 완주·사용자 개입·원본 전체 픽셀/음향 동일성은 이번 검증 범위가 아니다.
