# 웹 일본어 선택과 실제 브라우저 검증

WB04-E1412, 2026-11-27, HJOW-Athlon. E1410의 공용 일본어 자료에 언어 선택·저장·자동 감지와 Canvas 글꼴을 연결했다. 웹 저장 번호는 English=1, Korean=2, Japanese=3이며 원본 엔진에 전달하는 일본어 번호는5다. `?lang=ja`, `ja-JP`와 `japanese`도 지원한다. Options ▸ Language 선택은 설정을 저장하고 실제 페이지를 다시 적재한다.

일본어 메뉴·브리핑·튜토리얼 안내39키·11.01 Credits·Reset All을 표시한다. 패치 Credits는 기존 웹의 HJOW 개발 및 OpenAI Codex·ChatGPT·Anthropic Claude Code 사용 표기와 `Copyright 2026 HJOW`를 유지한다. Reset All은 웹 방식의 초기화/재적재를 안내한다. 실제 HTML 게임 메뉴 단추에도 일본어 글꼴을 지정했다.

## 글꼴과 줄바꿈

AGENTS.md의 WOFF2 없는 글꼴 규칙에 따라 Noto Sans JP를 Google Fonts CDN에서 적재한다. TTF를 변환하거나 실행 파일/번들에 내장하지 않았다. 고정 CSS 주소에 사용자 글자를 보내지 않는다. 영어·한국어 선택에는 이 CDN을 요청하지 않는다. CSS와 실제 글꼴은 각각30초까지 기다리고 실패하면 시작 안내부터 영어로 계속한다. 저장된 일본어 선택은 유지하고 메뉴에서 다시 선택해 재시도할 수 있다. 실패·취소·반납 시 해당 적재가 소유한 링크를 제거한다. [외부 접속 경계와 정확한 소스 감사 예외](webpj-external-access-20261105.md#9-일본어-google-fonts-경계-2026-11-27-wb04-e1412), [Google 공식 CSS2 문서](https://developers.google.com/fonts/docs/css2)를 참조한다.

번역 자료에 나온 글자만 먼저 읽는 후보는 드문 한자 `麋`의 첫 출력에 대체 글꼴을 캐시하는 문제가 실제 Chromium에서 재현됐다. CSS가 광고한 해당 가족의 모든 부분 FontFace를 먼저 기다리도록 수정했다. 당시 관측값은248개 부분 글꼴 전부 loaded·CSS 포함249요청이며 서버 응답에 따라 달라지는 값이다. 제품 코드는 이 개수를 고정하지 않는다. 수정 전에는 첫 캐시와 뒤늦게 만든 새 글리프가 달랐고 수정 후에는 같았다. [수정 전](webpj-e1412-evidence/rare-font-before.json)·[수정 후](webpj-e1412-evidence/rare-font-after.json)는 후보 실측이며 최종 번들 검증과 구분한다.

공백 없는 긴 일본어로 `bc6mows76` 첫 안내가 잘리는 것도 재현했다. 일본어 글꼴에서만 문자 경계 줄바꿈과 시작/끝 금칙·작은 가나·결합 문자 처리를 적용했다. 영어/한국어 줄바꿈은 기존 경로를 유지한다. 일본어 글꼴 슬롯은 현재 행 높이를 사용하고 한국어 갈무리11의 크기·기준선은 유지했다.

## 최종 소스와 운영 번들 검사

각 실행은 별개 결과다. 반복 검사를 한 번의 합산 통과 수로 세지 않는다.

| 검사 | 결과 | 원문 |
| --- | --- | --- |
| 일본어/자산/기존 언어/제품/시작/대응표 집중 |69/69, 실패·취소·건너뜀0|[focused.log](webpj-e1412-evidence/focused.log)|
| 일본어와 외부 접속 정밀 감사 |12/12|[network.log](webpj-e1412-evidence/network.log)|
| 모든 부분 글꼴 적재·타 가족 제외·오류 정리 등 최종 계약 |6/6|[font-contract.log](webpj-e1412-evidence/font-contract.log)|
| 전역 ESLint·공개 자산 확인·운영 빌드 |각 종료0, 자산377개 동일|[lint](webpj-e1412-evidence/lint.log)·[자산](webpj-e1412-evidence/assets.log)·[빌드](webpj-e1412-evidence/build.log)|
| Chromium:31미션 첫 안내·실제 언어 선택/CDN·차단 대체·기존 언어 |5/5|[첫 실행](webpj-e1412-evidence/chromium-first.log)|
| Chromium: 독립 JSON 보존 후 일본어3경로 재실행 |3/3|[최종 실행](webpj-e1412-evidence/chromium-final.log)|
| 설치 Chrome·Edge·Playwright Firefox: 실제 CDN/차단 각각 |6/6|[브라우저 행렬](webpj-e1412-evidence/browser-matrix.log)|
| 기존 영어 고정 시각 L2 |1/1, 기존 덤프/화면 단언 유지|[L2](webpj-e1412-evidence/l2.log)|
| 기존 한국어 M2와 영어 대조군 |2/2, 창 밖0픽셀·상태 일치|[M2](webpj-e1412-evidence/korean-m2.log)|
| 현재 소스 재생성과 추적 번들 바이트 일치 |1/1|[bundle-fresh.log](webpj-e1412-evidence/bundle-fresh.log)|

운영 번들은3,508,533바이트, SHA-256 `5e1c8349e7e51ccfdf9d9f2a4a3cb23bd57bae0038f6583caa079da18afea35b`다. 전역 ESLint는 새 정규식의 결합 문자 경고를 실제로 차단했고 규칙 예외 없이 표현을 고쳤다. 외부 접속 감사도 처음에는 새 CSS 주소를 거부했다. 정확한 파일의 정확한 한 줄만 허용하고 다른 파일·주소·추가 네트워크 구문을 거부하는 음성 검사를 추가했다. [최초 ESLint 실패](webpj-e1412-evidence/first-lint-block.log)·[최초 감사 실패](webpj-e1412-evidence/first-external-block.log)를 보존한다. 이 실패의 번들 보존 해시를 별도로 실측하지 않았으므로 E1400의 제어된 실패 증명을 대신 확대하지 않는다.

## 실제 화면 범위와 남은 확인

31개 캠페인은 일반 운영 번들과 실제 rAF로 진입한 뒤 첫 안내창만 읽었다. Play/Start/Next·유닛 명령·목표 수행·완료값 합성을 실행하지 않았다. 브라우저 `ja-JP` 자동 감지도 이 경로로 확인했다. [31개 독립 결과](webpj-e1412-evidence/initial-missions.json)에서 모두 일본어 제목이고 창이 논리 화면 안에 있으며 최대 창 폭540px·본문 줄 초과0이다. 31개 HTML 메뉴 단추에도 Noto Sans JP가 적용됐다. [실제 선택/부분 글꼴 상태](webpj-e1412-evidence/japanese-state.json)·[차단/재시도](webpj-e1412-evidence/japanese-fallback.json)와 대표7화면을 증거 디렉토리에 보존했다.

이는 일본어 번역에 대한 모국어 사용자의 교정이나 원본과의 일본어 픽셀 동일성을 입증하지 않는다. 후반 목표 안내·전투·승리 테스트는 사용자의 범위 제한에 따라 실행하지 않았다. 전체 Node 회귀의 직전 결과 E1415는 이 변경 이전이며, E1412 완료 후 주기는 v26=1/4·v22=2/8이다. [증빙 해시와 실행 구분](webpj-e1412-evidence/provenance.json)을 참조한다.
