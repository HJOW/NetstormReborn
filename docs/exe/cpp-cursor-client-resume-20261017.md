# cpppj 단위138 — 직전 객체 메뉴 수정의 재검증

작업 PC와 마지막 정밀 디컴파일 PC는 **HJOW-Athlon**, 디컴파일 날짜는 **2026-10-16**이다. 기준 커밋은 `8c10b36`이다. 사용자의 목표 재개 지시로 단위137에서 보류했던 검증을 수행한다. 새 디컴파일, 원본 실행 및 전체화면 DirectDraw 수정은 없다. 제품 코드와 기존 기대값은 수정하지 않았다.

## 확인한 결과

- Release 재빌드가 종료0으로 완료됐다. 로그는 `extracted/goal-menu-resume-20261017-build.log`다.
- 마지막 fixture 수정 뒤 `CampaignObjectMenus_` 3개가 모두 통과했다. 로그는 `extracted/goal-menu-resume-20261017-focus.log`다.
- 최신 실행 파일의 클론 창 회귀 5회가 모두 종료0이다. 캠페인1-1·1-2는 브리핑→Play Mission→월드, tutorial1·2·3은 브리핑까지 확인한다. `extracted/goal-menu-resume-20261017-entry/results.json`의 `originals_unchanged`는 true다. 자연 완주 검증은 아니다.

원본 근거 도구를 기존 보고서의 의존성까지 넓혀 57종 실행했다. 처음에는 Python에 Unicorn이 없어 관련 모듈을 설치했으며, 최종 UTF-8 실행 결과는 **46종 성공·11종 실패**다. `extracted/goal-menu-resume-20261017-verify2.json`에 각 명령·종료 코드·원문 출력을 남겼다. 이전 단위의 48종 성공을 이번 결과로 복사하지 않는다.

실패는 아래 기존 자료 문제이며 제품 수정으로 발생한 실패가 아니다. 이번에는 기준값이나 역사 자료를 바꿔 통과시키지 않았다.

| 도구 | 실패 지점 |
|---|---|
| analyze_campaign_target_path | RawGunTarget.h의 과거 SHA |
| decomp_chat_channels | decomp_priestmenu.py의 과거 SHA |
| decomp_constructionarrival_oracle | RawConstructionArrival.cpp의 과거 SHA |
| decomp_factorypostpop_oracle | RawFactoryPostPop.h의 과거 SHA |
| decomp_guntarget_oracle | extracted/pathmath/originals/creation.c 부재 |
| decomp_menulayout_oracle | recovery-menulayout-observations.tsv의 SHA |
| decomp_metadisplaydirty_oracle | extracted/metadisplaydirty/originals/creation.c 부재 |
| decomp_pathdata_oracle / decomp_pathprepare_oracle | RawPathData.h의 과거 SHA |
| decomp_pathruntime_oracle | RawPathRuntime.h의 과거 SHA |
| decomp_unpop_oracle | UnpopVtables.inc의 과거 SHA |

전체 CTest는 **1,565개 검사·실패0·종료0·230.66초**로 완료됐다(`extracted/goal-menu-resume-20261017-ctest.log` 및 `cpppj/build/Testing/Temporary/LastTest.log`)다. 단위137의 마지막 수정 뒤 전체 검증 보류를 해소했다. 다음 구현은 원본 공통 객체 메뉴 Open `004adb30`과 위치 `00425f90`을 출발점으로 한다. 실제+a8 작성 체인의 성공을 공통+ac·제목·객체 Form 부착·Client 명령 소비자의 완료로 확대하지 않는다.
