# 스커미시 구분선과 Options 선택 아이콘 통일 (WB04-E1489)

2026-11-29 사용자 요청의 webpj 두 항목을 반영했다. Campaign의 세 캠페인 묶음과 Skirmish 사이에 구분선을 넣었고, 스커미시 AI 수·개별 난이도·동맹·금지 유닛의 선택 상태를 메인 Options와 같은 메뉴 아이콘으로 표시한다.

## 변경

- `www/assets/product.js`: 웹에서 사용자 제작 캠페인(`offical6`)을 제외할 때 파일 목록의 끝 구분선(`offical5sep`)도 잘리는 것이 원인이었다. 스커미시 행 직전에 원본 `offical5sep.<현재 언어>` 파일 목록을 추가해 기존 구분선 자산·Tell 경로를 재사용한다. 영어·한국어·일본어를 지원하며 스커미시를 끈 판본은 기존 본문을 유지한다. 중복 처리도 한 번만 반영한다.
- `www/core/skirmish-setup.js`: 별표 및 `[X]`/`[ ]` 라벨 대신 `$Checked=라벨,Tell,대상,선택값,1`을 생성한다. 기존 `TellWindow` → `MenuList.addChecked` → `resources.marker`가 Options의 J02 선택 아이콘을 그대로 그린다. 금지 유닛의 세 언어 안내도 아이콘 설명으로 바꿨다.
- `$Checked`의 쉼표 필드와 충돌하지 않도록 생성 대상은 `SkirmishSet(diff;2;3)`처럼 세미콜론으로 나눈다. 대상 해석기는 기존 쉼표 방식도 허용하며 저장 형식은 바뀌지 않는다. cpppj CP1387과 같은 계약이다.
- Tell 메타는 `$Menu`를 `$Checked`보다 먼저 추출하므로 금지 유닛 화면의 다음/이전/모두 해제도 선택값 0인 `$Checked`로 만든다. 유닛 뒤의 원래 순서를 유지하며 이 행에는 아이콘을 그리지 않는다.

## 검증

- Node v26.13.0 집중 검사 65/65: 제품·스커미시 규칙/설정 UI·fort/Outpost·HTML 텍스트·Tell 섹션·embedded Tell. 실제 세 언어 최종 Campaign 메뉴에서 구분선 2개와 스커미시 바로 위 구분선, 스커미시 꺼짐의 끝 구분선 제거, 설정 선택·저장을 확인했다.
- 기존 공통 HTML 메타/위젯·Options 메뉴 및 아이콘 검사 27/27, 운영 번들 신선도/동일 설정 재빌드 바이트 대조 1/1. Node 합계 93/93 통과.
- `npm run lint`와 `npm run build` 통과. 운영 번들은 5,866,770바이트이며 9,900,000바이트 크기 관문을 통과했다.
- Chromium `e2e/skirmish.spec.js` 4/4 통과(Windows/HJOW-Athlon, Node v26.13.0, 논리 1024×768, 35.1초): 영어 선택·설정 저장/재진입·금지 목록 순서, 한국어 번역, 일본어 구분선/선택, `?skirmish=0`의 끝 구분선 제거를 확인했다. 기존 실제 메뉴 입력을 사용하며 미션은 시작하지 않았다.
- 실행 화면은 Git 제외 `webpj/test-results/skirmish-기본-판본에서-Campaign-의-Skirmish-설정-화면-흐름과-저장이-동작한다-chromium/`의 `campaign-separator.png`·`skirmish-selection-icons.png`·`skirmish-ban-icons.png`에 남겼다.
- 세 화면의 육안 확인에서 스커미시 바로 위 구분선과 Hard/Ally·금지 유닛 두 개의 기존 원형 선택 아이콘을 확인했다.
- 대응표의 `SkirmishSetup.cpp`는 CP1387 `738ac924`의 ② 반영으로 기록했다. 기존 후속 판정 대상 `RawGameMenus.cpp`·`RawProductionControls.cpp`는 이번 범위에 포함하지 않았다.
- `node tools/port-map.mjs`는 1,160행 오류 0으로 끝났으며 `SkirmishSetup.cpp`는 판정 필요 목록에 남지 않았다. 기존 두 이월 파일과 제외 분류 `ScreenResolutions.h`만 변경 목록에 나온다.

변경 범위는 메뉴 표시와 선택 명령의 필드 구분이다. 미션 시작/난수/AI 진행 규칙, 사용자 제작 캠페인 제외 정책은 기존 동작을 유지한다. 기존 원본 오라클은 덮어쓰지 않았다.
