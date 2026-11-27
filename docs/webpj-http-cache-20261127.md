# 하위 경로 정적 배포의 HTTP 캐시 검증

WB04-E1395, 2026-11-27, HJOW-Athlon. E1394의 실제 AI 플레이를 진행하는 동안 독립된 정적 서버 단위를 완료했다. 제품 게임 모듈과 번들은 E1393 그대로다.

기존 서버는 캐시 검증자를 보내지 않았다. 같은 URL을 사용하는 번들·매니페스트·자산이 배포 뒤 바뀌면 다시 확인하도록 모든 정적 응답에 `Cache-Control: no-cache`, 약한 ETag, Last-Modified를 추가했다. 재검증에 성공한 저장 응답을 다시 사용할 수 있는 정책이다. [RFC 9111 §5.2.2.4](https://www.rfc-editor.org/rfc/rfc9111.html#section-5.2.2.4).

GET/HEAD는 같은 검증자를 사용하고 If-None-Match의 약한 비교·목록·별표를 처리한다. 태그가 있으면 If-Modified-Since보다 우선하고, 일치할 때 몸체 없는304를 보낸다. 날짜는 HTTP의 초 정밀도로 비교한다. [RFC 9110 §13.1.2–3](https://www.rfc-editor.org/rfc/rfc9110.html#section-13.1.2).

ETag는 파일 크기·mtime·ctime·파일번호로 만든 약한 검증자이며 내용 해시가 아니다. 수정 시각을 보존하는 같은 크기 파일 교체도 ctime 변화로 구별했다. 정적 서버 생성기의 선택적 rootDirectory는 분리된 배포 사본 검사용이고 기본 www/기존 실행법은 같다. 파일 존재·메서드·하위 경로 검사를 먼저 수행하므로 캐시 조건이 오류를304로 숨기지 않는다.

- Node26 집중 **4/4 통과**, 실패/건너뜀0,766.148ms. 실제 HTTP에서 GET/HEAD·ETag 목록/날짜 우선순위·같은 이름/크기/mtime 배포 교체·오류/경로 경계를 확인했다. 임시 파일은 검사 전용 폴더에만 만들고 경로 확인 후 정리했다.
- Windows **Chromium·Chrome·Edge·Firefox 4/4 통과**. 실제 제품 번들을 `/preview/netstorm/`에서 적재하고 음량3을 앱 구성 쓰기로 저장한 뒤 새로고침했다. 서버가 실제 보낸200→304와 If-None-Match를 번들/매니페스트 각각 확인했으며 설정은 유지되고 경로 이탈 요청/브라우저 예외는0이다.
- 검사 후 해당 e2e의 try 본문 들여쓰기만 정돈했다. 실제 검사 코드는 같다. [로그/해시](webpj-e1395-evidence/provenance.json), [Node 원문](webpj-e1395-evidence/node.log), [네 브라우저 원문](webpj-e1395-evidence/browser.log).

다른 정적 호스트에서도 같은 이름의 파일에 긴 immutable 캐시를 붙이지 말고 매번 재검증하거나, 배포 버전별 디렉터리 URL을 새로 제공한다. 번들·매니페스트·자료는 한 배포본으로 함께 배치한다. 이 단위는 내장 서버/현재 네 Windows 브라우저의 HTTP 캐시와 설정 복원을 닫는다. GUI Linux·실제 미션 진행 저장 호환·60/120 표시·성능/메모리와 M4 사람 검수는 남는다.

재실행은 webpj에서 `node --test tests/static-server-cache.test.mjs`, 환경변수 `NETSTORM_E2E_MATRIX=1`로 `npx playwright test e2e/static-deployment.spec.js --workers=1`이다. 제품www 변경이 없으므로 번들은 다시 생성하지 않았다. E1393 전체 회귀 뒤 완료 단위1개로 주기는1/4·5/8이다.
