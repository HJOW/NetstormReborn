# webpj ESLint와 번들 생성 관문

WB04-E1400, 2026-11-27, HJOW-Athlon. 사용자 요청으로 작성한 JavaScript 전체에 ESLint를 설정하고 **webpack이 컴파일·출력 정리 전에 검사를 완료하도록** 연결했다. 오류나 경고가 있으면 실패하고 기존 번들을 보존한다.

프로젝트 루트에서 실행한다.

```powershell
npm --prefix webpj ci
npm --prefix webpj run lint
npm --prefix webpj run build
npm --prefix webpj run build:dev
```

ESLint10.15.0·@eslint/js10.0.1·globals17.14.0을 개발 의존성과 lockfile에 고정했다. Node 지원 범위는 해당 도구의 최소 버전에 맞춰22.13 이상인22.x 또는24 이상이다. 게임 배포 자료에 ESLint를 포함하지 않는다.

[flat 설정](../webpj/eslint.config.cjs)은 공식 권장 규칙을 사용한다. 브라우저 ES 모듈, Node CommonJS/ES 모듈, Playwright의 브라우저 콜백 환경을 나눴다. 생성 번들·복사한 게임 자료·오라클 fixture·로컬 조사/검사 산출물은 제외한다. 사용하지 않는 원본 콜백 인수와 잡은 예외 이름은 허용하지만, 미정의 이름·중복 선언·정규식/수치 오류 등 다른 권장 규칙은 유지한다. 사용하지 않는 disable 주석도 오류다. [ESLint 설정 문서](https://eslint.org/docs/latest/use/configure/configuration-files)를 따른다.

[선행 관문](../webpj/tools/eslint-before-build.js)은 compiler의 `beforeRun`과 `watchRun`에서 현재 Node로 ESLint CLI를 셸 없이 실행한다. `eslint . --max-warnings 0`을 기다려 일반 lint 명령과 기존 예외 정리 판정까지 동일하게 적용한다. 운영·개발·직접 webpack CLI/API·자동 번들 재생성에 적용되며 watch 재컴파일 때도 실행한다. [webpack compiler hooks](https://webpack.js.org/api/compiler-hooks/)의 비동기 훅을 사용한다.

`ensure-bundle`은 ESLint 설정·기존 예외 파일·선행 관문·package.json/lockfile도 최신 여부 판정에 포함한다. 최신 번들을 그대로 쓰는 경우에는 새 생성이 없으므로 기존 크기 검사만 한다. 다시 생성할 때는 webpack의 같은 선행 검사가 실행된다.

기존 미사용 선언은 [eslint-suppressions.json](../webpj/eslint-suppressions.json)에 **126파일·177건**만 고정했다. 원본 이식 필드/초기값·검사 장치의 수명을 일괄 변경하지 않도록 세 정리 규칙에 한정한 초기 예외다. 규칙을 끄지 않아 새 파일과 기존 건수를 넘는 지적은 실패한다.

| 규칙 | 기존 건수 |
| --- | ---: |
| no-unused-vars |90|
| no-useless-assignment |51|
| no-unused-private-class-members |36|

이 예외는 심벌별 기록이 아니라 파일·규칙별 건수다. 같은 파일에서 기존 지적을 고치면서 같은 규칙의 새 지적을 추가해 총건수가 같으면 구별하지 못하므로, 수정한 기존 지적의 예외도 줄인다. [공식 bulk suppressions](https://eslint.org/docs/latest/use/suppressions)를 사용한다.

```powershell
npm --prefix webpj run lint -- --prune-suppressions
npm --prefix webpj run lint
```

미정의 이름·중복 필드 같은 다른 규칙은 이 예외에 넣지 않았다. 실제로 중복된 동일 초기값 필드5개를 정리하고, 예외를 변환하는 세 경로에는 cause를 보존했다. Regular 실패 로그가 원래 게임 예외를 덮지 않게 하는 동작은 유지하면서 finally 안의 throw를 밖으로 옮겼다. 빈 정리 catch에는 이유를 썼다. 제어문자 필터7곳은 검사 목적을 명시한 줄별 예외이며, 수치 대조/경계 검사9개는 같은 double 반올림을 Number 문자열 변환으로 명시했다. 기대값/공백 정규식의 불필요한 상수식도 정리했다.

검증을 모두 마쳤다. [출처·버전·해시](webpj-e1400-evidence/provenance.json)와 아래 원문을 보존했다.

| 검사 | 결과 |
| --- | --- |
| 전체 npm lint |통과·오류/경고0|
| Node26 집중 검사 |181/181 통과|
| 운영 번들 생성/크기 |통과·3,500,786바이트·한도9,900,000바이트|
| 실제 운영/개발/자동 경로 실패 차단 |3/3·오류/경고에서 종료2·기존 번들 SHA/수정 시각 동일|
| 정상 개발 번들·Node22 동일성/관문 |개발 빌드 통과·Node22 검사3/3·운영 번들 바이트 동일|
| 실제 앱 초기 화면/진행 저장 복원 |Chromium2/2·초기 L2 0픽셀·제품 번들의 실제1-2 복원|

ESLint 적용 전 E1397의 양쪽 Node 전체6,895/6,895 결과는 이전 소스의 회귀 이력이며 이번 변경 뒤 전체를 다시 실행한 결과로 세지 않는다. 이 변경의 최종 검사 범위는 위 표를 따른다.

[집중181건](webpj-e1400-evidence/focused.log), [Node22](webpj-e1400-evidence/node22.log), [브라우저](webpj-e1400-evidence/browser.log), [운영 빌드](webpj-e1400-evidence/production.log), [개발 빌드](webpj-e1400-evidence/development.log), [세 실패 경로의 산출물 비교](webpj-e1400-evidence/build-gate-results.json)를 참조한다. 실패 검사용 임시 파일은 제거했고 정상 운영 번들을 유지했다. 정기 회귀 뒤 완료1개이므로 주기는1/4·1/8이다.

E1406 후속에서 설정만 바뀌어 번들 내용이 같은 경우도 정상 생성 시 출력 시각을 갱신하도록 보완했다. 검사 실패 때의 SHA/mtime 보존은 세 경로로 재확인했다. [자동 갱신 반복 방지](webpj-webpack-fresh-mtime-20261127.md).
