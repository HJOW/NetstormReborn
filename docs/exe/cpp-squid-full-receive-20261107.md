# 1021 — 전체 객체 수신·검증과 PlayerNoMake

10.78 `00433c30`의 공통35BYTE 수신을 같은 실제 Session의 Factory.Take, WORD setter, 파생 소유자, HP, FirstPop/Pop에 연결했다. 기존2fh 취소 수신 `004332c0`과 구별한다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**이며 이번에는 산출물을 읽기만 했다. CD10.72 `00487d40`은 별도 객체 폭의 비교 근거이고 PATCH35BYTE와 섞지 않는다.

Take 전 옛 free 비트를 보관하고 현재 `SupportTypeWas`를 쓴다. 좌표·추가 DWORD·상태·소유자·zOrder·프레임과004acec0의 기존2/4비트 보존을 원문 순서로 적용한다. PE의 표시 좌표는 X×16+0.5/Y×11+0.5의 ftol 하위WORD이며, HP 관문은0049a8e0이 읽는 **flags1**이다. 첫 배치만 현재 기술 장부 `00595360 + owner×16 + 10`을 검사한다. owner0/techBit−1은 원문대로 허용하고 범위 진단 복귀 뒤 마스크31을 유지한다.

첫 미보유 경고는 Player 이름 포인터 선택→현재 TYPE 설명의 언어/설정 치환→`PlayerNoMake(%s,%s)`의 실제 Tell→현재 TYPE 경고 비트 저장 순서다. Player+54의400 표식은 안내 여부와 무관하며 콜백 뒤 현재 owner를 다시 읽는다. `RawSquidName.BaseName`은 원문0049af80을 공유하고 Factory 레벨 접미사를 넣지 않는다.

수신2ch는 버전1/정책0/고정35BYTE/빈 observer로 같은 등록표와 해제 수명에 연결했다. 검증 모드는0054d4d8의 같은 장기 DWORD를 읽으며 TYPE/owner/free/dead/contained 다섯 불일치만 수준1d에 기록한 뒤0으로 만든다. 원문의 packet dead/contained 마스크2/8과 client bool1 비교를 유지한다. 검증 시작/분배 부모와 압축7ah는 후속이다.

83개 실제 자산 VT의+30/+50이 모두004ad470/004ad330임을 보호 PE에서 정적으로 대조했다. 종속 플래그 진단이 정상 복귀한 예외 경로는004ad330의 진단→좌표→자신의 내용물 UI→공통 부착/현재 링크/실제 Pop 깊이·파생 PostPop에 연결했다. 실패 때 앞선 부분 쓰기를 되돌리지 않는다.

Release 제품 빌드는 종료0·경고/오류0이다. 제품1021/11.01은11,968,000바이트이며 SHA-256은 `4252a0234d57628ad1fbe8cfb133515ec0938e7532a9814ec2fd6e527bb89d5e`다. 신규 멀티플레이·단위·게임·통신 검사와 fixture 생성은0이다. AGENTS.md·원본/CD/fixture2800파일은 불변이다. 컴파일·정적 대조 결과를 LAN 동작 검증으로 세지 않는다.

다음은7ah 압축 decoder/현재 수신 owner 캐시와 검증 진입, 파생 writer/decoder 및 재동기화 부모다. 기존 writer의 고정 헤더 선언27과 반환35는 이번 단위에서 수정하지 않았다. 공식 서버·유효성 검사는 연결하지 않는다.
