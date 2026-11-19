# 앱 미션 설치의 표시 바이트 (WB04-E896, 2026-11-19)

기준선 `225553df`의 `CampaignClientServices.cpp`와 현행 `0444b4b6c`의 `OriginalText.cpp`를 대조했다. 앱은 현재 번역의 `You`를 표시 바이트와 NUL로 만들어 Session 설치기에 넘기고, 설정에 쓸 때 첫 NUL 앞을 해독해 같은 `GameData`에 기록하며 현재 창 제목을 갱신한다. 한국어 번역표에 cpppj 확정본과 같은 `You` → `당신`을 추가했다. 호출 시점의 자원과 설정을 다시 조회하며 Session별 명시 장치는 우선한다.

기존 대응표의 표시 바이트 웹 제외 판정은 고정 바이트 Player·Login 기록에 맞지 않아 정정했다. 웹 표시 그리기는 기존 UTF-16을 유지하지만 이 기록 경계에서는 cpppj와 같은 정적 한국어·가나·기호와 프로세스별 동적 한자 배정 표를 사용한다. 잘린 시퀀스는 일반 원문 바이트로 보존하고, 표현 범위 밖은 거부한다. 다른 채팅·AI·화면 소비자의 바이트 연결과 일본어 UI는 아직 별도 작업이다.

실제 Tutorial1 `BridgeTheGap`의 State7→8 내부 로그인에서 웹 번역표를 사용해 `fort.mySubHandle`과 Player의 이름 바이트가 모두 `당신`으로 돌아오는 것을 확인했다. 첫 검사에서는 Player 이름 오프셋을 잘못 읽어 실패했고 원본 `+0x0c`로 바로잡은 뒤 통과했다. Node v26의 표시·설치·한국어·원문 인코딩 검사 25건, 실제 World 검사 2건, Chromium 갈무리11 메뉴·영어 대체 2건, `npm run check-assets`가 통과했다.

cpppj 원본 함수로 정적 범위 11,679자와 한자 배정 네 행을 생성해 총 11,683행을 웹 결과와 바이트별 비교했고 모두 일치했다. LF로 정규화한 TSV SHA-256은 `148b696c447278eb1dcde270891d7ebab46fbd90d44185dc5db8c4a339f90455`이며 Node 검사가 이 해시를 다시 확인한다. 최초 MSVC 빌드는 소스 코드 페이지 경고를 냈고 `/utf-8` 추가 후 경고 없이 다시 빌드했다. 원본 게임 창은 실행하지 않았다.

관측 생성 도구는 게임을 실행하지 않는 콘솔 프로그램이다. Visual Studio C++ 도구와 CMake가 있는 저장소 루트에서 다음처럼 사용한다. 인수는 생성할 TSV 경로이며 기존 파일이면 덮어쓰므로 별도 출력 경로를 지정한다.

```powershell
cmake -S webpj/tools/display-text-oracle -B extracted/web-display-oracle/build
cmake --build extracted/web-display-oracle/build --config Release
& ./extracted/web-display-oracle/build/Release/web-display-text-oracle.exe extracted/web-display-oracle/vectors.tsv
```

World 생성 요청 소비, 진행 창·나머지 실장치와 화면·입력 결선은 남아 있어 이 단위만으로 브라우저 미션 플레이를 할 수는 없다.
