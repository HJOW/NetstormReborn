# 시간 동기화 Poll 이식과 실제 cpppj 대조 (WB04-E517)

기준선 `225553df`의 `RawClockSynchronization::Poll`을 기존 웹 시간 동기화에
옮겼다. 같은 70개 저장 표본의 만료·유효성 검사, 빠른 절반의 왕복 시간과
offset 평균, 지연·게임 시계 DWORD 보정, 기한에 따른 첫 빈 표본 송신을
원문 순서로 수행한다. E516의 Windows CRT 순서 정렬을 실제 보정에 연결했으며,
비유한 수나 signed 64비트 밖 값을 절삭할 때 원문의 하위 DWORD 0을 유지한다.
송신 장치가 실패하면 이미 쓴 표본은 남기고 후속 카운터·기한은 바꾸지 않는다.

실제 cpppj C++ 본체를 합성 상태에 호출하는 콘솔 도구
`webpj/tools/clock-poll-observer`를 만들었다. 이 도구는 게임·네트워크·창을
사용하지 않는다. Poll에서 도달하지 않는 패킷 등록 함수의 링크 경계는
호출되면 즉시 중단하도록 했고, 시계와 송신 장치만 합성 입력으로 제공한다.
기준선과 현재 cpppj 소스가 같은 것을 확인한 뒤 다음처럼 Windows VS2022로
빌드하고 실행하면 표준 출력으로 JSON을 얻는다. 이 실행 파일은 게임 배포판에
포함하지 않는 개발·검증 도구다.

```powershell
cmake -S webpj/tools/clock-poll-observer -B extracted/webpj-oracle/build-clock-poll-observer -G "Visual Studio 17 2022" -A x64
cmake --build extracted/webpj-oracle/build-clock-poll-observer --config Release --target clock-poll-observer
& extracted/webpj-oracle/build-clock-poll-observer/Release/clock-poll-observer.exe
```

출력에서 24개 장면의 입력·출력 표본 전체(각 70×32바이트), 지연·게임 시계,
송수신 수, 기한, 벽시각 조회 횟수, 송신 연결과 32바이트 패킷을
`webpj/tests/fixtures/cpppj-clock-poll.json`에 보존했다. 웹 시험은 그 자료를
복원해 실제 C++ 결과와 대조한다. 동률 선택·NaN 정리·비유한 보정·signed
송신 수·송신 예외의 부분 효과도 별도 시험한다. 이 자료는 cpppj의 합성
관측이며 원본 10.78 기계어 fixture는 아니다. 관측 도구를 다시 실행했을 때
출력 바이트가 같았고 SHA-256은
`89d0623cb574590ba6d9479fea20b66e195bed53a47a03ce55eb8faacb99e82b`다.

초기 임시 빌드는 `/utf-8` 누락으로 실패했고, 이를 보강한 뒤 등록 함수의
링크 참조가 남아 관측하지 않는 네 함수를 명시적 중단 경계로 제공했다.
그 후 실제 Poll을 성공적으로 실행했다. `RawClockSynchronization`은 여전히
**partial**이다. 장기 고정 Native 수신기 baseline 승격, 실제 Client/Session
결선과 단독 미션 도달 검증은 남았다. Node 22·26 전체 회귀 검사를 수행했다.
