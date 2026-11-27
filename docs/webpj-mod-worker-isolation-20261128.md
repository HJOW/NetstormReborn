# WB04-E1478 격리 Worker와 단일 번들

WB 기준 구현에 불투명 `sandbox=allow-scripts` iframe과 그 안에서 생성하는 전용 Blob Worker를 추가했다. 모드마다 별도 SES Compartment/동결 intrinsic을 사용하며, 공개 API 이외에 DOM·네트워크·타이머·호스트 난수·추가 컴파일 장치를 주지 않는다. 정적 상대 모듈과 순환 그래프는 패키지 내부에서만 해석한다. 설치 검증은 Worker의 Acorn 분석만 실행하며 엔트리를 평가하지 않는다. 이 단위는 실행기 기반이며 실제 게임 루프/Options 자동 연결은 다음 단위다. DN 구현 중지는 유지한다.

부모의 실제 시간 타이머가 등록/사건 한 번을1000ms로 감시한다. 초과·예외·규약 오류는 해당 Worker만 비활성화하고 대기 Promise를 거절한다. 종료 중계 확인 또는100ms 최종 정리에서 iframe/포트/콜백/타이머를 반납하고 늦은 응답은 무시한다. SDK 자체 시작은 별도10초 상한이다. `timeLimit`과 명시 반납의`disposed`를 구분한다. 웹의 강제 힙/스택 바이트 제한이 없다는 공통 프로필은 유지한다.

webpack은 선행 전체 ESLint 뒤 자식 compilation에서 SDK를 묶어 최종 JS 앞 한 줄의 base64 데이터로 넣는다. 메인 realm에서 SES를 실행하지 않으며 Worker JS를 별도 배포/요청하지 않는다. Native debug는 생산 번들을 텍스트로 읽어 같은 봉투만 해독한다. 개발 소스 맵은 추가한 한 행을 보정한다. 새 플러그인도 자동 번들 갱신의 입력이다. 직접/전이29패키지의 실제 설치 버전·라이선스/NOTICE 원문은 `webpj/www/licenses/mod-worker/index.json`과 하위에 동봉했다. `node webpj/tools/mod-worker-licenses.cjs`로 lock 설치본에서 재생성한다.

검증:

- Node26 집중/연관69/69(공개 API·호스트·입력·패키지/저장소·새 실행기6·대응표·ESLint 관문), Node22 새6/6. 작은 실제 webpack에서 운영/개발 단일JS·메인 realm 미실행·소스 맵 이동·import되지 않은 Worker lint 거부·SDK 컴파일 실패시 이전 바이트/mtime 보존을 확인했다.
- 최초 새 검사5/6은 컴파일 오류가`stats.hasErrors` 대신 Promise 거절로 전달되는 경로를 검사 코드가 예상하지 못한 실패였다. 같은 실패/보존 단언을`assert.rejects`로 정정한 뒤6/6이다. 최초 원문을 보존했다.
- 실제 Chromium/Firefox 최초2/2 뒤 확장2/2에서 공통 패키지5경계 출력 일치, 순환 모듈, import 문구 문자열 값 보존,21개 외부 권한 전역 부재, API 함수 생성자3개/Math.random 차단을 확인했다. 등록/사건 무한루프는`timeLimit`, 대기 중 반납은`disposed`, 페이지 heartbeat는 계속 응답했다. 한 모드 예외 뒤 다른 Worker의 정상 등록이 가능했다. iframe CSP의`connect-src none`은 신뢰 시험 Worker의 fetch도 막았고 실제 로컬 서버 도착0건·종료 후 iframe0개였다. 게임 플레이/승리 검사는 없다.
- 최종 전체 ESLint 선행 운영 빌드 종료0(90.060초), 단일JS5,780,002바이트/9,900,000한도 통과. 별도 현재 소스 재번들1/1(69.298초)로 바이트 동일성을 확인했다. 로그/해시는[증빙](webpj-e1478-evidence/provenance.json)에 기록한다.

제약: Endo의 공식 값 보존 변환을 사용하지만, 검열 패턴(`import(`·HTML 주석 경계)이 들어간 **tagged template raw**는 라이브러리에서 변환하지 않아 SES가 실행시`scriptError`로 거절할 수 있다. raw 값을 바꾸는 우회 변환으로 통과했다고 주장하지 않는다. Worker 내부 OOM에서 페이지 생존이나 DN 적합성도 증명하지 않았다. 실제 시계 장벽/Session 사건·오류 모드 저장 비활성화·옵션 설치/삭제/ResetAll은 다음 단위에서 연결한다.

완료 후 정기 주기는v26=2/4·v22=2/8이고 다음 전체 예상6994다. 기존 사용자 이월1-3/1-4/1-6 및 후반 목표/승리, 물리60/120Hz·청취, Linux 과거 별도 실패 한계는 유지한다.
