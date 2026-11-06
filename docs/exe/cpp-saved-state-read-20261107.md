# 1005 — 실제 startup 저장상태 reader와 질문 표시

2026-11-07, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다.10.78의004b5890/004b5ba0/004cf550·0042ce70/0048f350과 현재 부품의 연결을 대조했다.CD reader00432a10(strong0.796),startup00432d40(strong0.809),cf550→004c3670(strong1.0)을 기록하며 구현 판본은10.78이다.

ReadClientSavedState는 기존 RawSavedStateRead의 동작을 복제하지 않고 실제 사용자 홈 Saves/!state.dat 경로 고리/존재조회·VFS.Open(mode0,create0)/같은 BaseFile.Read(-1)/소멸에 연결한다.Windows10/11 Unicode 이름 때문에 원문 ANSI 대신 W속성 조회/_wfopen을 사용한다. 디렉터리도 존재 비영으로 넘기며 뒤 실제 open이 실패할 수 있다. File 지역 대여ID1은 실제 호스트 FILE 포인터를DWORD로 절삭한 값이 아니다.

파일 부재에서 scratch/Player를 읽지 않으며 존재할 때 현재 CRT 할당392BYTE를 명시 입력으로 제공한다. 이는 원본32비트 stack/24BYTE File/SEH ABI 자체 일치가 아니며 임의0패딩을 피하기 위한 호스트 경계다. 실제 fread(data,size,1)의 부분 읽기는 잔여 BYTE를 보존하고 반환을 새 길이 검사/실패로 바꾸지 않는다. reader의 서명/점이름/종료DWORD/참가자2명 관문, 파일 정상 생성 뒤 정상/선택 예외 해제와 부분 전역 효과는 기존 원문 몸체를 사용한다. 잘못된 owner/NUL/물리 범위는 기존 명시 거부를 유지한다.

같은 ClientPlayerRecords의 실제1620BYTE와 ClientCoreDataGlobals.StoreMetadata/InitializePlayerCore를 사용한다.pending 이름은 기존 SetPendingFortName helper에, 복구 owner/subscriber/이전handle은1003 ClientLocalStateGlobals의 같은 물리에 연결한다. tail은 같은 SpellCount/SourceType/GameTicks이며 별도 사본 목록/새 PlayerKnown/가짜 Ready를 만들지 않는다. 읽는 중 파일/Player/메타데이터가 부분 적용되어도 이전 상태로 되돌리지 않는다.

State1/3의 기존 startupStateCheck에 실제 reader를 공급했다. 옵션0/저장시각0 또는 엄격 경과분0..45 밖은 기존 원문 자연 반환이다. 질문 경로의 Yes/No/질문 번역은 각각 소비까지 별도 주소를 유지하고, 실제256BYTE 버튼 서식과 double로 승격한float32분/문자열의 질문 서식 뒤 원문cf550→cf270(text,0,width,height)를 같은 HTML 공장에 전달한다. 공급하지 않은 CRT 인자/범위 초과는 명시 거부하고 새 MessageBox/정지를 추가하지 않는다.

**질문 표시를 재접속 실행 완료로 세지 않는다.**StateReconnect/DelStateFile의 일반 입력 이름/소비자와004b8cb0의 서버 선택/실제 상태 요청·복구State7 Player/이름 인계는 다음 작업이다. 일반 프레임486bc0/스펠·전체 decoder·메타·Outpost/checking7·한국어 후속·Edit 시나리오 UI도 남는다.

Release 제품 빌드 종료0·경고/오류0이다. 제품11.01은11,844,096바이트, SHA-256 `63306a19b19c1251dc161d71182cb3564d72a06f3be50c5321e91aa72346680a`다. 보고서는 `cpppj/saved-state-read-20261107.json`이다. 신규 MP/단위/게임/통신 검사·fixture 생성0이며 보호2800파일은 모두 불변이다. 실제 파일 읽기/재접속 질문/버튼 동작을 실행 확인한 결과가 아니다.
