# K1 한국어 후보 선별 병합 — 작업1270

2026-11-11 / HJOW-Athlon. **986 한국어 최종 후보(코드·배포·검사 41파일 + 별도 자산 44개)를 현행 제품에 병합하고 처음으로 컴파일·실행했다.** 후보 문서는 [FINAL_PLAN](handoffs/cpp-pause-20261106/korean/FINAL_PLAN.md)이며 후보는 제품 985 기준이라 현행(1269)과 달라 자동 덮어쓰기를 하지 않았다. 마지막 Client 디컴파일 2026-11-03/HJOW-Athlon과 현재 호스트가 같고 새 디컴파일은 없다. **K2(IME 실입력)~K5와 한국어 단계 전체는 미완료이며 MP 실행은 0이다.**

## 병합 방식

- `git apply --check`는 41개 중 5개(`package-cpppj.ps1`, `ClientMain.cpp`, `UberGump.cpp`, `src/CMakeLists.txt`, `tests/CMakeLists.txt`)에서 실패해 `--reject --ignore-space-change`로 적용하고 나머지 hunk를 직접 병합했다: ClientMain.cpp 3 hunk(초기 `extendedText=true`, `WM_IME_*`/`WM_UNICHAR` 처리, 파일 끝 `ResolveKoreanBuiltinFile`/`ReadLocalizedGameFile`/`MatchLocalizedGameFiles`), UberGump.cpp 2 hunk(include, 유니코드 클립보드 장치), 두 CMake(`DisplayTextInput`·`imm32`, 새 검사 두 파일), 배포 스크립트 3 hunk(`lang\korean\xlat.korean`과 `lang\korean\d\*.korean` 43개 동봉). `.rej` 파일은 병합 뒤 삭제했다.
- 자산 44개(`d\*.korean` 43 + `xlat.korean`)는 후보에서 `assets/lang/korean/`로 **바이트 그대로 복사**하고 SHA-256 44/44 일치를 확인했다. 현행 `xlat.korean`과의 차이는 첫 줄 UTF-8 BOM뿐이었다.

## 컴파일에서 드러난 후보의 결함 (처음 컴파일)

1. `DisplayTextInput.cpp` — `if (const auto cp=…)`의 범위가 `else`까지 이어져 같은 이름 `cp`를 다시 선언(C2373/C2440). 안쪽 변수를 `scalar`로 변경.
2. `KoreanResourcesIntegrationTests.cpp` — `ConfigInterface`는 기본 생성자가 없다(읽기 장치 필요). 빈 읽기 장치를 명시. 또 HTML 파서 검사 보조가 `condition` 효과를 연결하지 않아 실제 Help 파싱 시 "HTML 실제 효과 미연결: condition"이 발생해 `BindHtmlConditionConfig`를 연결.
3. `QuickHelpWindowTests.cpp` — 패치 적용으로 `fileSpec` 멤버가 두 번 선언되어 한 줄 제거.
4. **번역 자산의 실제 결함:** `KoreanIntegration_RealHelpAll128TopicsAndLargestBodyKeepKoreanTail`이 실제 파서 출력을 재서 가장 긴 주제 `battleOptionsHelp`의 한국어 본문이 7,940바이트에서 잘려 마지막 문단("멀티플레이어 빠른 시작으로 돌아가기" 링크)이 사라짐을 드러냈다(한글 한 글자가 표시 4바이트라 영어보다 길다; 준비본의 8,086바이트 추산은 경계 7,936을 넘는다). `help.korean`의 `Game Type` 설명 한 문단을 의미를 유지한 채 줄여(7,797바이트) 끝 안내가 남도록 했다. 이 자산은 후보 원본과 한 문단 다르다.

## 검증

Release/x64 빌드는 최초 시도 708.7초에 후보 컴파일 오류로 실패했고 위 교정 뒤 종료코드 0, 경고 0이다. `netstorm_korean_text_tests` **33/33 통과**(기존 + 새 `KoreanInput_` 11 + `KoreanIntegration_` 12, 마지막 Help 8192바이트 경계 검사 포함). `netstorm_file_tests` 639건 중 6건 실패(`CampaignMotion_*` 3, `ExistingEditorAfterCheckpoint_LiveRequestAndCommandReads`, `RawSurfaceWorld_RuntimeSharesStorageAndCurrentDispatch`, `SquidUnpop_VtableMetadata_MatchesActualCommonDisplayPaths`)이며 한국어 검사 `KoreanIntegration_ActualHelpCache…`는 통과한다. 이 6건은 한국어 변경과 무관한 이름이지만 **변경 전 기준선에서 직접 비교하지는 않았다.** `netstorm_menu_session_tests`(6건)와 `netstorm_main_popup_tests`(3건)의 실패 이름은 1269 직후와 동일하다. 최종 EXE의 정상 Tutorial4 진입·생산/건설 요청·600프레임 대기·종료 코드 0(게임 시각 13.7→24.4초)을 확인했다. 보호 원본/CD/fixture/AGENTS 2,808파일은 불변이다. 포획/희생·특정 적 공격·승리·MP 실행은 0이다.

## 남은 한국어 작업

- **K2:** 실제 물리 IME/클립보드/긴 입력은 단위 검사만 있고 OS 실입력 확인이 없다. Save as Fort의 한글 파일명 정책은 원래 ASCII/공백 정책 그대로다.
- **K3:** 미션 브리핑·팁·도움말 `.korean` 본문은 후보 번역 그대로이며 용어(AGENTS 명칭)·잘림 검수는 하지 않았다. 다른 주제도 경계에 가까울 수 있으나 128개 주제 전부 8192 미만임을 위 검사가 확인한다.
- **K4:** 한글 메인 버튼 그림·맵 목록 이동행·누락 UI 문구는 미착수.
- **K5:** 한국어 화면/배포 ZIP 실제 실행 검수는 미착수.
