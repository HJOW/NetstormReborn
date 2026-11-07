# 1037 — 개발자 설정 명령과 현재 저장소 연결

이번 /goal로 이전 일시 정지를 해제하고 cpppj 작업을 재개했다. AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md 재독, git clean·fetch 확인 뒤 시작했다. 현재 호스트와 마지막 전체 정밀 디컴파일 호스트는 **HJOW-Athlon / 2026-11-03**으로 같다. 원본 실행·디컴파일 산출물 변경은 없었다.

31h의 setsubid/ssi, setbridgekillperiod/sbkp, settracelevel/stl 세 명령을 연결했다. 누적25개 접합이며 남은8개는 dumptypes, dumptype, heaptrace, dumpArtData, dumpgraph, queryDirectDraw, addLabel, getPriest다. 실제 입력과 cr65h 소비자는 여전히 후속 대상이다.

setsubid는 기존 공개 ChangeFortSubscriber의 몸체를 Impl 공용 함수로 옮겨 UI와 명령이 같은004b32f0을 호출하게 했다. 현재 Template manager·fort 관문, handle 복사·설정·제목 갱신, My ID 쓰기·Subscriber writer·변경 통지의 순서를 유지한다. 파싱 실패 때도 현재 ID/handle 로그는 출력한다. 숫자는 signed DWORD 해석/unsigned 비트 저장을 구별한다. 원문 스택 버퍼 손상은 복제하지 않고 NUL/호스트 물리 범위를 검사한다.

sbkp는 %f로 읽은32비트 값을 같은 RawSurfaceStorage.bridgeDecayPeriod에 먼저 쓴다. 원문 수준29의 두 줄 로그 뒤 현재 주기와 현재 게임 프레임 시각을 읽어 기존 ResetStateBridgeDecayScan(00422490)을 호출한다. 이 스캔·주기는 실제 Client 프레임과 State 정리가 공유하는 저장소다. 음수 등 입력을 임의로 clamp하지 않으며 미기입 stack 자체는 명시 예외로 거부한다. 정밀 C의 시각 float 표기 대신 기존 PE 대조를 마친 reset 몸체의 double 연산을 보존했다.

stl은 두번째 토큰을 정수로 먼저 바꾸지 않고 같은 Config SetString(004414d0)의 tracelevel에 전달한다. 실제 Client의 onChanged가 기존 로그 필터를 즉시 읽으므로 별도 필터 사본을 추가하지 않았다.

처음 전체 솔루션 빌드는 제품 라이브러리 컴파일 이후 기존 검사 소스들까지 다시 컴파일하므로 Ctrl+C로 중단하고 모든 cmake/MSBuild/cl 자식 종료를 확인했다. 그 원시는 build-full-cancelled.log에 보존했다. 이어 게임 대상 NetstormCpp만 빌드해 종료0·경고/오류0을 확인했다. 전체 빌드 성공으로 기록하지 않는다.

제품 Release 빌드 종료0·경고/오류0, EXE 12,098,560바이트/SHA-256 `a5c6e179a96e394c13a6ea87e268b9c1f2516b6603cd134a23ec4ad59828247e`다. 보호 AGENTS·원본/CD/fixture 2800파일 불변이며 신규fixture0이다. 신규 MP/단위/게임/통신·공중3종전용 및 지정된 사용자 검사는 실행하지 않았다. 실제 명령/LAN 동작 검증 완료를 뜻하지 않는다. 다음은 잔여31h 진단 명령과 입력·cr65h이며 이후 LAN 나머지→한국어 후속→시나리오 UI 순서를 유지한다.
