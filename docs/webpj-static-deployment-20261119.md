# WB04-E945 하위 경로 정적 배포 검증

정적 호스트에서 `webpj/www`의 내용을 같은 디렉터리 구조로 제공한다. 하위 경로로 배포할 때는 끝에 `/`가 붙은 URL로 접속한다. 시작 모듈, 데이터 매니페스트와 나머지 자원은 그 경로를 기준으로 적재한다.

[브라우저 검사](../webpj/e2e/static-deployment.spec.js)는 테스트 전용 HTTP 서버로 정적 파일을 `/preview/netstorm/` 아래에서만 제공한다. 시작 화면까지 진입한 뒤 앱을 통해 효과음 음량을 변경하고, 같은 URL을 새로고침해 값이 복원되는지 확인한다. 하위 경로를 벗어난 요청과 브라우저의 처리되지 않은 예외도 검사한다.

Windows에서 Chromium, Chrome, Edge, Firefox가 각 1건씩 통과했다. 초기 검증에서는 Playwright의 가로채기 요청이 Chrome에서 재적재 도중 연결 재설정으로 실패했다. 테스트 서버가 앱 파일을 하위 경로에서 직접 제공하도록 바꾸고 네 브라우저를 다시 검증했다. 기본 경로 스모크와 한국어 글꼴·영어 대체·오류 진단의 Chromium 검사 4건도 통과했다.

Node v26.13.0의 최종 전체 회귀는 6,514건 통과, 실패·취소·건너뜀 0이다. 1차 회귀에서 드러난 [한국어 적재 호환 문제 두 건](webpj-browser-error-reporting-20261119.md#비동기-브라우저-경계)을 수정한 뒤 전체를 재실행했다.

Chrome과 Edge가 설치된 Windows에서 Playwright Firefox를 설치하고, `webpj` 디렉터리에서 브라우저 검사를 실행한다.

```powershell
npx playwright install firefox
$env:NETSTORM_E2E_MATRIX = '1'
npx playwright test e2e/static-deployment.spec.js --workers=1
Remove-Item Env:NETSTORM_E2E_MATRIX
```

운영 호스트의 HTTP 캐시 정책, 캐시된 이전 버전 자원과의 조합, GUI Linux, 실제 미션 완료값의 저장 수명은 별도 검증이 필요하다.
