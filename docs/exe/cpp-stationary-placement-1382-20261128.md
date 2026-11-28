# 정지 커서의 에너지·공간 재판정 — CP1382

2026-11-28 webpj의 제네레이터 완성 뒤에도 에너지 부족이 남는다는 제보와 cpppj 동시 점검 요청에 따라 구현했다. 작업 PC는 HJOW-Athlon이며 LEFT_JOBS.md의 마지막 디컴파일 PC와 같다. 기존 디컴파일 자료와 원본 파일은 변경하지 않았다.

## 원인과 수정

`RawCursorPosition::Update`는 커서의 월드 좌표가 바뀌는 경우에만 미리보기와 배치 판정을 갱신한다. `UserInput::UpdateCursor`가 매 프레임 같은 마우스 위치를 전달해도 제네레이터의 공급 장부 변화가 반영되지 않았다.

게임 조립 계층인 `RawCampaignCursor::Update`에서 이전/현재 월드 좌표가 같으면 기존 `GumpsChanged`를 호출해 에너지·공간·GUI 판정과 이전/새 경고 범위를 갱신한다. 이동했을 때는 기존 원본 위치 몸체의 갱신을 사용하므로 중복 조회하지 않는다. 잠금/빈 커서는 기존 가드로 제외하며 미리보기 재생성·비용 차감·송신을 추가하지 않는다.

## 검증

새 `CampaignSession_StationaryCursorRefreshesActualEnergyProviders` 검사는 창 없이 실제 GameWorld/Session과 원본 TYPE/SHP·저장 지형을 사용한다. 검사 초기 공급 장부에서 저장 지도의 다른 공급원을 잠시 제외해 새 공급원 하나의 유무를 분리한다(종료 때 기존 항목 복원). 실제 완성 Rain 제네레이터를 등록한 뒤 건설 가능한 Wind 제네레이터 위치를 정하고, 공급원 제거 → 같은 마우스 위치에서 에너지 부족 → 같은 자리에 완성 공급원 Pop 등록 → 이동 없이 허용 복구를 확인한다. 실제 에너지·공간 함수나 TYPE의 요구 에너지는 바꾸지 않는다. 커서 좌표·미리보기 SID 목록/개수·SP·송신 불변도 확인한다.

첫 집중 21건에서는 19통과/2실패였다. 새 검사의 외부 `unitLost` 관찰 장치 누락과 기존 GUI 검사의 옛 정지 커서 동작 단언을 보완했다. 다음 실행은 20통과/1실패였으며 저장 지도의 다른 공급원이 같은 위치를 계속 지원해 에너지 부족이라는 초기 조건이 성립하지 않은 것이었다. 이를 위의 명시 공급 장부 입력으로 분리했다. 제품 로직을 검사에 맞춰 바꾸지 않았으며 두 실패 로그도 보존한다.

최종 Release 라이브러리/게임/검사 빌드 종료 0, `--filter Cursor` 집중 **21/21 통과·실패 0**이다. 게임 `cpppj/build/bin/Release/Netstorm.exe`의 SHA-256은 `03ab2a316231dfbcb3cb877b2903991cbbed75d5df40ba731f9acf7370145dec`이다. 실제 제네레이터 건설 애니메이션 전 과정과 물리 마우스 조작을 실행한 검사는 아니다. 최신 배포 ZIP1378은 이전 제품이며 이번에는 다시 만들지 않는다.

빌드 로그는 `extracted/cursor-refresh-cpp-build-20261128.log`·`cursor-refresh-cpp-product-build-20261128.log`·`cursor-refresh-cpp-tests-isolated-build-20261128.log`, 최종 검사는 `extracted/cursor-refresh-cpp-tests-isolated-20261128.log`다. 앞선 `cursor-refresh-cpp-tests-20261128.log`·`cursor-refresh-cpp-tests-final-20261128.log`는 위에서 설명한 검사 보완 전 실패 기록이다.
