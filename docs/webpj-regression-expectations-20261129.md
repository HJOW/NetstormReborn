# 최근 변경의 회귀 검사 기대값 교정 — WB04-E1493

2026-11-29, HJOW-Athlon. E1490의 정기 Node26 전체 회귀에서 최근 E1487·E1488 변경을 반영하지 않은 검사 4건을 발견했다. 실패 세 파일을 별도 실행해 24건 중 20통과·4실패를 재현한 뒤 교정했고 24/24 통과했다. 첫 전체는 실패 확인 뒤 중단했으며 전수 통과 결과로 세지 않는다.

| 검사 | 원인 | 교정 |
| --- | --- | --- |
| `campaign-request-frame.test.mjs`의 실제 앱 로그인 | CSS1366×768을 논리 화면 크기로도 기대 | 실제 표시 크기는 그대로 두고 논리1280×720, 지도·구름·dirty·화면 버퍼·피치·수명/재배치 검사를 같은 논리 크기로 일관되게 대조 |
| `korean.test.mjs`의 번역표 전수 | 별도 언어 표의 Load/Copy Settings도 Xlat 키로 요구 | 기존 Xlat 누락 검사 유지, JSON 두 행은 실제 한국어 메뉴의 정확한 라벨·명령을 별도로 확인 |
| 같은 파일의 Options 구조/번역 | JSON 두 행과 실제 한국어 언어 인자 누락 | `language:korean`과 번역 함수를 함께 제공하고 기존 라벨·새 두 라벨·명령/구조·음량 하위 체크를 모두 대조 |
| `mod-panel.test.mjs`의 마지막 행 | 모드/ResetAll이 끝에서 두 행이라고 기대 | 실제 사용자 요구 순서 Mods → ResetAll → LoadSettings → CopySettings와 원본 비교 메뉴의 제외를 세 언어에서 확인 |

이어서 Node26 전수를 끝까지 실행한 결과 7,046건 중7,045통과·1실패(951.475초)였다. 추가 실패는 `raw-campaign-session-load.test.mjs`의 월드 저장소 준비가 실제 Config 없이 초기화기를 호출한 것이다. E1487에서 추가된 `bindTutorialUnitCosts`가 Config를 요구하는 실제 시작 순서에 맞춰 `GameData`를 먼저 설치하고, 원래 TYPE 비용이 `global.crossbowCost`에 기록되는지도 확인한다. 첫 전수 로그는 `webpj/extracted/webpj-e1493-node26-full.log`에 유지한다. Node22는 이 실패 뒤 시작하지 않았다.

준비 교정 뒤 해당 파일 전체 **129/129**가 통과했다(184.933초, `webpj/extracted/webpj-e1493-load-fixture-focused.log`). 해당 파일 lint도 통과했다. 첫 전수 입력2,280개와 최종 재실행 입력을 대조해 이 테스트 파일 하나만 바뀐 것을 확인했다.

제품 코드·번역 자산·오라클을 검사 기대값에 맞춰 바꾸지 않았다. 실제 창과 논리 화면의 구분·한국어 표시·명령 순서·실제 초기화 선행 조건에 대한 단언을 유지했다.

집중 결과: `webpj/extracted/webpj-e1490-failures-focused.log`(수정 전20/24), `webpj/extracted/webpj-e1493-regression-focused.log`(수정 후24/24). 전역 lint/build는 E1492, 이후 변경한 검사·범위 도구는 `webpj/extracted/webpj-e1493-e1494-lint.log`에서 확인한다.

## 전체 회귀

E1492 제품 보완과 위 검사 교정, E1494의 공용 아군 AI 검사 범위 소스를 고정하고 Node26 → Node22를 순차 실행한다. 두 버전 전체는 동시에 실행하지 않는다. 처음에 새로 추가된 부모 소실4건·검사 범위2건도 포함한다. 작성 입력의 SHA-256은 `webpj/extracted/webpj-e1493-input-before.json`에 기록하고 종료 후 다시 확인한다.

Config 준비 교정 뒤 최종 전수 로그는 `webpj/extracted/webpj-e1493-node26-final.log`, `webpj/extracted/webpj-e1493-node22-final.log`다. 두 실행 모두 종료0이며 최종 작성 입력2,280개는 두 버전 종료까지 원시 SHA-256 변경0이다. 입력 범위는 웹 작성 JS/MJS/CJS·JSON·HTML·CSS·TSV와 운영 번들이다. 공개 복사 자산·기존 fixture/라이선스·로컬 산출물은 제외하고 기존 fixture는 전수의 manifest 검사에서 확인한다. 전체 입력 원문은 `webpj/extracted/webpj-e1493-input-final-{before,after}.json`, [최종 결과/입력 지문](webpj-e1493-evidence/validation.json)에 요약을 보존했다.

| 실행 환경 | 최종 결과 | 시간 |
| --- | --- | --- |
| Node26.13.0 | **7,046/7,046**, 실패/취소/건너뜀/todo0 | 877.123초 |
| Node22.23.3 | **7,046/7,046**, 실패/취소/건너뜀/todo0 | 843.407초 |

E1492·E1494 브라우저 검사는 Chromium156.0.8078.4(headless), Windows10 빌드19045, CSS/논리1024×768·DPR1, 고정25ms·영어·음향 끔 환경이다. 물리 표시·청취 검증과 구분한다.

운영 번들은 E1492의5,866,967바이트·SHA-256 `3cb6137503d6607ba918373351e8b2c1caeb24d58b85b6fcb4442d198b8690b3`를 유지한다. 수정한4개 검사 파일과 관련 범위 도구 lint도 통과했다. E1494 완료 소스까지 포함한 이번 전수 종료 뒤 주기는 v26=0/4·v22=0/8이다. 수동/물리 확인·미승리·DN 대조·자료 대기 항목은 [E1494](webpj-autoplay-scope-20261129.md)의 미완료를 유지하며, 사용자는1-3/1-4를 다른 AI 진행 조건으로 계속 시도하도록 지시했다.
