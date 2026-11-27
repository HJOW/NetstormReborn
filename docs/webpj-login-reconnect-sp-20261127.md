# Login의 현재 재접속SP 지정 훅

WB04-E1393, 2026-11-27, HJOW-Athlon. CP1375(`84182644a`)의 잔여 훅을 반영하고 정기 전체 회귀와 변경 감사를 마쳤다.

기존 RawLocalMissionState의 State13은 현재 재접속 표식(+0x84)이 있을 때만 현재 signed금액(+0x88)을float로 변환해 `setReconnectSp`를 호출한다. 웹 Login 앞단에도 같은 훅을 연결했다. 현재Editor 정책이면194162.0f, 그 밖에는 받은float 금액을 선택한다. 현재PlayerKnown이 참일 때만 현재battleOwner의 같은 분산SP에32비트로 쓰고, known 여부와 무관하게HUD를 갱신한다.

기존 정수용 `setPlayerSpAmount`를 재사용해 금액을 signed int로 다시 좁히지 않았다. `0x7fffffff`의float 변환은2147483648이며 양수float비트`4f000000`을 유지한다. 소수·음수의float도 보존한다. Editor/known/owner를 오래된 사본으로 캐시하지 않는다.

이 훅을 복원한 것이 LAN 저장fort복구나 외부 수송 UI의 구현을 뜻하지 않는다. 기존 State8의 재접속 연결 제외 관문은 유지한다. 웹 단독도 사용하는 현재 State 부모의 선택/부작용만 복원했으며 외부 도메인/IP 접속은 추가하지 않았다.

- Login 앞단·원본 LocalMissionState 관측 **59/59 통과**, 실패/건너뜀0(7.955초).
- 실제 Owner의현재SP·다른지갑보존·현재owner변경·Editor양방향·unknown/RNG무효과·필수refresh누락시SP쓰기선행·반납거부를 확인했다.
- 실제Fort/State13 부모에서도 표식1일 때 훅과양수float 금액이 선택되고 표식0일 때 호출되지 않는다. LAN 실행/시나리오 목표 주입으로 검사하지 않았다.
- webpack **3,500,854바이트**. 빌드가 종료0으로 완료된 뒤 검사를 시작했다.
- Node **26.13.0 전체 6,891/6,891 통과**, 실패/취소/건너뜀0, **734.138초**. 검사 중 제품 소스와 Node 검사를 고정했다. `webpj/extracted/webpj-e1393-node26.log`에 전체 결과를 보존했다.
- 실제 Chromium 서비스 연결 **1/1 통과**(44.7초). 구름 갱신·HTML 전역·Factory 연구 도움말·연결 상태 삭제·빈 State 바이트를 확인했다.
- 실제 비고정 시각의 Tutorial2 건설 **1/1 통과**(약1.3분). 마우스로 지정한 위치에서 사제가 신전과 워크샾 건설을 완료하고 단계C로 진행했다. E1392의 큰 epoch 값으로도 실제 건설 기한이 진행됨을 확인했다. 진입용 튜토리얼 체크값은 UI 검사 준비값이므로 자연 완주/진행 저장의 근거로 사용하지 않는다. 포획·희생·공격·승리는 시험하지 않았다.

RawCampaignSessionLogin.inc는 기존base`a3bea51fa` 뒤3줄 추가 전체를 반영해`84182644a`로 갱신했다. E1381의 변경9행과 기준선 밖 신규10파일 감사도 이 단위에서 다시 확인했다. 후속 기능/웹 제외 판정을 기존1,007단위의 완료율에 섞지 않는다.

변경 감사의 오류/기존 행 변경/삭제는 모두0이다. 대응표1,160행은 port275/session732/제외153이며 partial/todo0이다. 고정 이식 대상1,007단위와 기준선225553df는 유지했다. 신규10파일의 개별 판정과 대응표의 정규 UTF-8/LF SHA-256은 [감사 자료](webpj-e1393-evidence/cpp-delta-judgments.json)에 보존했다.

| 대응표 밖 파일 | 웹 판정 |
| --- | --- |
| EmbeddedBitmapFont.cpp/.h | 외부TTF 비트맵 판독이라는 Native 경계다. 웹의 외부WOFF2·Canvas/갈무리11 적응과 기존 글꼴 크기·일반체·고해상도층 사용자 결정으로 대응한다. |
| KoreanFontSettings.h 및 별도 목록 밖 KoreanFontSettings.cpp.in | 글꼴 family/파일명/라이선스 생성 상수다. 웹의 korean-font-config.js와 prepare-assets가 대응하며 글꼴 파일은 별도 배포한다. |
| ResetAllSettings.cpp/.h | 기존 E863/E1251의 실제 확인창·게임 소유 localStorage 초기화·destroy/재로딩으로 대응한다. |
| ManaPictureData.inc / TextPictureResources.h | E1336의 8,016바이트 자료·마나 별칭·CD25프레임 보충으로 반영했다. 이번 `make-mana-picture-data.mjs --check`와 `prepare-assets.js --check`도 종료0·333자료/목록 일치다. |
| TutorialObjective.cpp/.h / TutorialObjectiveCatalog.inc | 필수 추가 기능으로 E1386~E1389의 조건40단계/문구39키·표시·수명·검증을 마쳤다. 일본어39키는 명시한13차 언어 작업에 남는다. |

새파일은 다음 재기준선 때 등록하며, 개별 판정을 생략한 채 이식 완료량에 더하지 않는다. 일본어·M4/M5·허용 캠페인 목표 수행 등 인계 문서의 나머지 작업은 계속 유효하다. 정기 주기는 E1393에서0/4·4/8이며 다음 E1397에 Node26/22를 순차 검사한다.
