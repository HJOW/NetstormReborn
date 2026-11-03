# 공통 Gump 부착의 현재 GT 진단

단위806은466750의 첫 상태 쓰기 전 checking/GT 진단을 실제 공통 GumpTree 부착에 연결한다. 같은 Client DWORD를 lifetime과 tree가 함께 빌리고,436d20의 상세 저장은 같은 ClientAiDebugGlobals nullable 문자열에 기록한다. 오류를 기록한 뒤에는 중단하지 않고 같은 현재 flags/부모로 부착을 계속한다. 후속 부착·정렬·이동 본문은 소스 대조에서 불변이다.

Native+20의 GT는 기존 호스트 필드 command다. signed DWORD로 해석하며1 이상이고 현재 signed BYTE 상한보다 작아야 한다. 기계어46676b는 `movsx ecx,byte ptr [54045e]`이며 보호 PE 초기 BYTE는0x43=67이다. WORD/DWORD로 넓혀 읽지 않는다. 다른 Gump 명령 이름/저장 필드의 전면 재명명은 하지 않았다.

원문432입력을 두 x87 정밀도에서 실행한864관측과 재관측864회가 바이트 일치했다. INT_MIN/MAX·음수/0/1·66/67/68,checking0/1/상위비트/FFFFFFFF, 상한 BYTE0/1/67/127/128/255, 상세 기록의 현재 필드 변경을 교차했다. 상세 선택276·첫 부착 쓰기 전 도달432다. 부착은46678f 직전 접두이고436d20은 전체 실제 writer를 실행했다.4a89c0 정수 포맷과 strdup의 확보/복사는 명시 장치다. 실제 CRT·할당 실패·전체 SEH/진단 UI·전체 부착 새 원문으로 합산하지 않는다.

새 C++ 검사는 전체0xa0BYTE·현재 checking/상세와 첫 쓰기 전 효과를 원문과 비교한다. 실제 GumpTree에서는 상세 writer가 GT/flags/부모를 바꾼 뒤 같은 저장소로 부착하는 순서와 선택된 writer 누락을 검증한다. 기존 CD 경로에는10.78의 새 GT 진단을 추가하지 않는다. 기존 세 판본 전체 트리 fixture와 실제 버튼 호스트 수명 검사도 함께 대조한다.

실제 UberGump는 기본 UI/메뉴보다 먼저 같은 checking/상세 owner를 BindChecking에 공급한다. 같은 노드 주소와 Client 상세 수명이 모든 callback/UI보다 길어야 한다. 임의 주소·노드 reset/해제·32비트ABI·전체 진단 UI는 별도 경계다. 마지막 전체 정밀 디컴파일은 HJOW-Athlon/2026-10-23이며 현재 호스트가 같고 추출물 변경은 없다.

원시 원문·빌드/검사·본문 동일성 감사는 `extracted/cpp-resume-806/`에 보관하고 최종 근거는 [JSON 보고서](../../cpppj/gump-attach-checking-20261103.json)에 기록한다. 새 게임/원본 GUI·특정 적 공격·포획/희생·시나리오 승리·공중 전용·MP/통신·물리/PCM·전체34/전체 C++/전체71대상·ZIP은 이번 범위에 없다.

Release 게임/집중 대상 빌드는 경고·오류0이고 집중19개(새3개)가 실패0이다. Native/빌드/검사 실패는0이며 기존 세판본 트리1722행과 버튼8개 수명도 통과했다. checking 잔여는37→36(부모27·부분6·MP3)이다. 최종 EXE SHA256은 `7ee1834cc31612001383ad603ba96d3865cb211f8338cab842aab594ecfc6357`이다. 기존 보호2735개 불변·새fixture1 포함2736개/fixture916개이며 다음 기준은 `extracted/cpp-resume-806/protected-after.json`이다.
