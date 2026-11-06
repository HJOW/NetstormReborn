# 1003 — 일반 로그인 완료의 실제 저장상태 writer

2026-11-07, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다.10.78의004b54e0·0040ee90/0040ec50·0041b4c0/0041a910/0041b410을 보호 PE/정밀 소스와 대조했다.CD writer 대응은00432770(strong0.552)이며 구현 판본은10.78이다.

RawSavedStateWrite는 missionPost 또는 미PlayerKnown이면 파일 장치를 요구하지 않고 반환한다. 그 밖에는 실제 SP offset/추적 offset 비교→현재 fallback 진단값→남은 검사횟수/현재 로컬 SP>7000→현재 첫 분산배열/첫 순열WORD-1을 원문 순서로 검사한다. 조건별 현재 fallback을 다시 읽고 남은 횟수를 DWORD 감김으로 감소시킨다. 진단은 기존 실제 ScrambledSpStore·사제 patch/Client PatchDiagnostic을 공유한다. 오프라인 서버 검증을 복원하거나 정합성을 가짜 성공으로 대체하지 않았다.

원문 경로 고리004b5470의 현재 선택 뒤 사용자 홈 Saves/!state.dat에 기존 VFS.Open(mode2,create1)/실제 Unicode CRT를 연결한다. 같은 BaseFile의 개별 fwrite(data,size,1)/offset-1을 사용하고 부분 fwrite/fclose 반환으로 새 실패 분기를 추가하지 않는다. 파일 정상 생성 뒤 선택된 예외/정상 종료에서 해당 File을 한 번 반납한다. 원문32비트24BYTE File/SEH ABI 자체 일치는 호스트 경계다. 사용자 홈 상위 디렉터리는 기존 UserData 초기화가 마련하며 설치/원본 폴더에 쓰지 않는다.

서명0xa004e→0으로 채운 현재Fort256BYTE→현재 ownerDWORD→현재 벽시각 정수DWORD→현재 SP float의 정수DWORD→owner1..8의 번호/ID/이름30/주소30/flags/관전자/Core10→SpellCount/SourceType/GameTicks→현재 LoginEnd!=0을 순서대로 기록한다. Native PE4b5674는 wall을,4b56b4는 Get이 출력한 SP float를 x87에서_ftol로 보낸다. 디컴파일의 누락된 x87 입력을 게임시각 복사로 오해하지 않았다. 전체 요청976BYTE, Player당86BYTE다.

현재 owner를 기록하기 전에00542528을 쓰고 해당 write 뒤0054252c/이전handle005c84c0을 갱신한다. 같은 Client 수명에 실제 초기ffffffff인 두DWORD와 NUL 포함64BYTE 호스트 이전이름 물리를 마련했다. 각 Player base는 해당 반복 시작에 조회해 실제 일곱 쓰기 뒤 같은 flags8을 센다. checking의 점 이름(f79)·서버수<=1(fa3) 진단은 원문처럼 복귀 후 계속한다. 이름 NUL/물리 범위를 넘는 원본 미정 접근은 명시 거부한다.

기존 로그인 완료 타이머의 saveReconnect를 이 실제 writer에 연결했다. **1004 정정:** 서버 Begin은 먼저LoginEnd0을 쓰므로 뒤 writer의 마지막DWORD는0이다. 클라이언트의 현재LoginEnd가 비영인 선택은1이며 타이머가 뒤에0을 쓴다. writer 코드는 현재값을 사용했고 앞 설명만 과도하게 일반화했다. 주기004b6730은1004에서 연결했으며 실제 파일 reader/재접속 시작·일반 프레임/스펠·saveBattle/서버 이전은 아직 남는다.0044fa00의 랭크 소비자도 별도 후속이며 파일 writer의 구현을 전체 재접속 완료로 세지 않는다.

checking 부분 잔여004b54e0의 전체 쓰기/두 진단 몸체와 실제 타이머 caller를 연결했으므로 잔여는7(부모3/부분1/MP3), 완료부모39/부분5=44+잔여7/기준51로 갱신했다. 신규 MP 동작 검증 완료라는 뜻이 아니다. 최초 patch의 CMake 문맥 불일치는 파일 변경 없이 교정했다.

Release 제품 빌드 종료0·경고/오류0이다. 제품11.01은11,820,544바이트, SHA-256 `ca5d6eaba54663b097d1e951ddf8df4e6b4b21a2966c29072edc58dc5aafda2b`다. 보고서는 `cpppj/saved-state-write-20261107.json`이다. 신규 MP/단위/게임/통신 검사·fixture 생성0이며 보호2800파일은 모두 불변이다. 실제 저장/재실행/재접속 동작을 실행 확인한 결과가 아니다.
