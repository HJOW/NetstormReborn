# WB04-E747 개발자 객체 생성과 분산

`31h`의 `create`/`c`와 `scatter`/`sct`를 현재 Session의 Factory·Pool·CRT·World에 연결했다. `create`는 두 signed 정수를 읽은 뒤 서버에서만 원본 TYPE를 생성하고 frame DWORD를 직접 쓴 다음 좌표 `(20,20)`에 Pop한다. `scatter`는 서버에서 열 `3..30`마다 같은 TYPE을 만들고 현재 프레임 수와 15비트 CRT를 한 번씩 읽어 frame을 정한 뒤 행 `developerScatterRow`에 Pop한다. 열 전체를 끝낸 뒤 행에 2를 더한 원문 10 나머지를 같은 GameAux 장기 상태에 보관한다.

근거는 기준선 `225553df`의 `RawCampaignSession.cpp` 5094..5104와 `RawGameAuxState.h`다. 기준선 이후 해당 두 파일의 변경은 없다. 새 scatter 행 상태의 주소 `005326d8`와 초기값 `3`은 보호 원본 PE 10.78을 읽어 기존 GameAux 전체 초기값 검사에 포함했다.

독립 검사에서 입력 오류, 클라이언트 생략, signed frame 비트, NULL 슬롯, 10회 CRT 소비·frame·Pop 순서와 행 회전을 확인했다. Tutorial 1의 실제 `31h` 분배기로 TYPE154 `create` 한 개와 `scatter` 열 열 개를 검사해 Factory/Pool/World의 frame·좌표·장기 상태까지 확인했다. 기존 집중 검사 17건과 중단 후 관련 세 파일을 다시 실행한 검사 52건이 통과했다.

Node v26.13.0 전체 6,237건이 통과했고 실패·취소·건너뜀은 0건이다. 중단 후 같은 전체 회귀를 다시 실행해 6,237건 통과를 확인했다. E746의 v22 검사 뒤 두 번째 v26 전체 회귀다.

다음은 `31h`의 나머지 도움말·진단·연결 명령과 내부 로그인 서버 후반·응답 왕복이다. Native 분기와 앱 미션 연결은 남아 있다.
