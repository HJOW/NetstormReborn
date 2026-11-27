# 웹 favicon 적용 (WB04-E894, 2026-11-19)

**현재 결론: E1446(2026-11-27)에서 실제 Chrome 탭의 시각 확인까지 완료했다.** 아래 초기 실패 기록은 그대로 보존한다.

cpppj 1323에서 추출한 `assets/Netstorm.ico`를 `webpj/www/favicon.ico`로 복사하고, `index.html`에 상대 경로 아이콘 링크를 추가했다. 하위 경로 배포에서도 링크는 문서 경로를 기준으로 해석된다.

원본 `originals/Netstorm.exe`의 PE 리소스를 `pefile`로 읽어 그룹 102·144의 디렉터리와 각각 `RT_ICON` 1·2의 이미지 전체 744·296바이트를 웹 ICO와 비교했다. 모두 같고 공통 ICO와 웹 ICO도 바이트 단위로 같다. SHA-256은 `bd4c18c5cb5f42bc49a1de0c6d6623643fd5377f575e95a1de60a05f01e4f068`이다. 원본은 수정하지 않았다.

Playwright Chromium에서 창 모드와 헤드리스로 각각 링크·HTTP 200·MIME·해시·브라우저 이미지 해독 검사를 통과했다. 자동 favicon 요청은 Playwright 페이지의 응답 이벤트로 관찰되지 않아 명시 요청으로 확인했다. 실제 탭의 아이콘을 눈으로 확인하려 했으나 컴퓨터 사용 도구가 보이는 Chromium 창에 연결할 때 창 소유자 오류를 반환했다. 탭 표시의 시각 확인은 아직 남아 있다.

2026-11-19 재시도에서 인앱 브라우저는 로컬 시작 화면을 표시했지만 화면 캡처에 탭 영역이 포함되지 않았다. 별도 Chrome 창을 열어 확인하려 하자 컴퓨터 사용 도구가 현재 URL을 확실하게 판별하지 못해 작업을 중단했다. 탭 아이콘의 시각 확인은 계속 남아 있다.

2026-11-20 HJOW-Athlon 재시도: Chrome에서 `http://localhost:9803/`의 NetStorm 탭이 실제로 열리고 접근성 트리에서 확인되었다. 그러나 Windows 창 화면 캡처는 두 차례 시간 초과했고 탭 선택도 좌표 정보를 얻지 못해 실패했다. 다른 컴퓨터 사용 경로의 캡처도 시간 초과했다. 탭 그림을 직접 볼 수 없었으므로 시각 검증을 완료 처리하지 않는다.

## E1446 실제 탭 시각 확인 완료(2026-11-27)

HJOW-Athlon의 설치된 Chrome을 computer-use의 @oai/sky로 열었다. 처음 앱 목록/실행 요청의 시간 초과를 그대로 기록했고, 이후 list_windows에 실제 새 탭 창이 나타났을 때 반환한 창 ID/앱으로 대상을 유일하게 선택했다. 새 탭에서 실제 localhost:9803 링크를 눌러 NetStorm 로딩을 기다렸다. 브라우저 전체 창 이미지에서 상단 탭 왼쪽의 **노랑/파랑 NS 아이콘**을 직접 시각 확인했다. 페이지 안 이미지·DOM·HTTP 해독 성공만으로 탭 표시를 판정한 것이 아니다.

클릭하여 시작 화면까지 적재됐으며 게임 시작 입력은 하지 않았다. 현재 웹 ICO와 공통 assets/Netstorm.ico는 여전히 동일 바이트/SHA-256이며 원본 PE 이미지 동일성은 E894 근거를 따른다. 기존 시각 확인 잔여를 닫는다. [실제 전체 창 PNG](webpj-e1446-evidence/e1446-favicon-native-chrome.png) · [현재 창/시각](webpj-e1446-evidence/e1446-favicon-native-chrome.json) · [첫 시간 초과 기록](webpj-e1446-evidence/e1446-favicon-attempt.json) · [출처/해시](webpj-e1446-evidence/provenance.json).
