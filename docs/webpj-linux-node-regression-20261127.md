# 정기 Node 회귀와 Linux GUI 검사

WB04-E1397, 2026-11-27, HJOW-Athlon. E1393 제품과 이후 HTTP 캐시 검사까지 고정해 Node26/22 전체를 순차 실행했다. **두 실행 모두6,895/6,895 통과**, 실패·취소·건너뜀0이다. Node26.13.0은1342.706초, Node22.23.3은1053.522초였다. 명령은 각각 같은 `--test --test-concurrency=2 tests/*.test.mjs`다.

시작/종료 원시 해시를 대조했다. `www/netstorm.js`와 `server.js`만 LF에서 CRLF로 바뀌었고 LF로 정규화한 내용은 시작 해시와 동일하다. 나머지 고정 파일은 원시 해시도 같다. 번들3,500,854바이트와 Node 검사 코드는 변경하지 않았다. [시작 해시](webpj-e1397-evidence/freeze-start.json), [종료 판정·출처·로그 해시](webpj-e1397-evidence/provenance.json), [Node26 원문](webpj-e1397-evidence/node26.log), [Node22 원문](webpj-e1397-evidence/node22.log)을 보존했다.

Linux는 Ubuntu26.04/WSL2 x86_64·Node24.21.0 ELF·Playwright Chromium1248(Chrome for Testing156.0.8078.4)·Xvfb headed 환경이다. 필요한 브라우저 캐시만 추가했고 기존 캐시나 OS 설정을 바꾸지 않았다. Desktop Chrome 프리셋의 UA는 Windows 문자열이지만 실제 실행 바이너리는 Linux다.

기본 음향 장치에서는 WebAudio 장치 오류가 발생했다. 게임의 mute 옵션으로 우회하면 Windows에서도 동일7068픽셀 차이가 생겨 기존 오라클 조건으로 사용할 수 없었다. 따라서 검사 전용으로 **실제 네이티브 AudioContext의 표준 `sinkId: { type: 'none' }`만 선택**했다. 게임 audio 옵션은 true이며 재생기·버퍼·음향 시계·게임 시간/상태를 대체하지 않는다. 생성자의 실제 sink type을 검사하며 현재 native context 상태/시간도 관측했다. [Chrome 공식 설명](https://developer.chrome.com/blog/audiocontext-setsinkid), [Web Audio 표준](https://webaudio.github.io/web-audio-api/#dom-audiocontextoptions-sinkid)을 따른다. 물리 출력과 청취 성공의 증거는 아니다.

최종 Linux6건은 **5통과·1실패(15.4분, 종료1)**다. 원래 단언을 유지하고 trace 수집만 끈 조건이다.

| 검사 | 결과 |
| --- | --- |
| 고정 시각4:3 두 실행 |통과·C++ 화면0픽셀·90/130프레임 예상 밖 덤프 차이0|
| 고정 시각16:9 |통과·동일 기준0픽셀/차이0|
| 고정 시각16:10 |통과·동일 기준0픽셀/차이0|
| 실제1-1 저장 사본 읽기 |통과·제품 번들에서 실제1-2 World/브리핑·재실행 복원|
| 하위 경로 정적 배포/새로고침 |통과·조건부 캐시·설정 복원|
| 비고정 시각의 직접 프레임 진행 거부 |초기 clickToStart 대기60초 시간 초과·미완료|

4:3의 두 실제 캡처는 Windows의 같은 음향 조건 캡처와 인덱스 픽셀·팔레트 바이트도 동일했다. 비교에서 허용한 행은 기존 검사와 같은 환경 의존 SP/Player뿐이며 화면 마스킹은 없다. [최종 비교](webpj-e1397-evidence/capture-comparison.json), [Linux 원문](webpj-e1397-evidence/linux-final.log), [진행 저장 출처·실제 관측](webpj-e1397-evidence/linux-progress-reader.json), [Windows 단독 기준](webpj-e1397-evidence/windows-silent.log)을 참조한다.

초기 WSLg 적재/정리 시간 초과, 기본 장치 오류와 mute 조건 실패도 보존했다. trace를 켠 경로의 정리 시간 초과가 있었지만 수집기가 유일한 원인이라고 확정하지 않는다. [기본 장치](webpj-e1397-evidence/linux-audio-device.log), [Linux mute](webpj-e1397-evidence/linux-muted-attempt.log), [Windows mute](webpj-e1397-evidence/windows-muted-attempt.log). 큰 원본 캡처·덤프·실패 화면은 `webpj/extracted/e1397-*`에 유지한다.

M5 전체를 완료로 올리지 않았다. 비고정 초기 시작의 안정성과 물리 음향 확인이 남는다. 다음 단위는 사용자 요청 ESLint이며, 이번 회귀 뒤 주기는0/4·0/8로 갱신했다.
