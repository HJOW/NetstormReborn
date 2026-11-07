# 1038 — 개발자 타입·그림·그래프 덤프와 프레임 진단

10.78의31h에서 dumptypes/dts, dumptype, dumpArtData/dad, dumpgraph/dg, heaptrace/ht 다섯 명령을 실제 현재 저장소와 Client 프레임에 연결했다. 누적30개이며 남은3개는 queryDirectDraw, addLabel, getPriest다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**이다. 산출물과 보호 PE를 정적으로 읽었으며 새 디컴파일/원본 실행은 하지 않았다.

dumptypes는 원문49fd70대로 타입0을 빼고1부터 현재 표의 끝까지 순회하며 각 출력 직전의 이름·signed flags·zOrder·numFrames를 읽는다. 원문에서 detailed 인자는 읽지만 실제 덤프 함수는 사용하지 않으므로 추가 설명을 합성하지 않았다. dumptype의 실제 별칭dts는 앞 dumptypes가 먼저 처리하며 도움말의dt를 임의로 활성화하지 않았다. 기본 입력0과 %02d 형식도 보존한다.

dumpArtData는 최초 numFrames를 루프 상한으로 잡고 같은 SquidDisplayShape의 현재 signed short 핫스폿·크기·포구 값을 읽는다. 419850의 flags 배수/현재 프레임 개수·loaded·두 디버그 sentinel 관문과 원문 frameCheck 진단/상세를 유지한다. 진단 복귀 뒤 실제 현재 헤더를 조회하며 호스트 범위 밖 접근은 예외로 구별한다. dumpgraph는 실제 공유 RawGraphStorage의255행 중 선택한 행을 읽고 signed WORD surface/inUse를 출력한다.

heaptrace의 도움말은 힙 순회를 설명하지만 보호 PE의437f0은 단일RET(c3)이다. 557dfc·54dab0의 PE 초기값은 모두0이며 정밀 C에서557dfc의 쓰기는 없다. 따라서 ClientTraceGlobals에 두 현재 DWORD를 두고 ht는 플래그만 변경한다. 실제 공통 Run의 Frame 뒤·Chat 입력 전환 앞에서 원문 수준29로 allocated를 출력하며 새 힙 순회/호스트 할당 집계를 만들지 않았다. 수신 Session이 반납돼도 같은 Client 전역을 보존한다.

NetstormCpp Release 제품 빌드 종료0·경고/오류0, EXE 12,103,680바이트/SHA-256 `6390e4c380c1b18caba3bbf68f6ee62a49574542975638454fb910fa413fd4c3`다. 보호 2800파일 불변·신규fixture0이다. 신규 MP/단위/게임/통신·공중3종전용 및 사용자 지정 검사는 실행하지 않았다. 실제 명령/프레임 출력/LAN 동작 검증 완료가 아니다. 다음은31h3명령과 실제 입력·cr65h이며, 이후 LAN 나머지→한국어 후속→시나리오 UI 순서다.
