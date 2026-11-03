# ContentForm 부착 부모의 현재 상태와 포획 안내 연결

단위 814는 `004aec10`의 부분 공백을 실제 Session까지 연결했다. 기존 세션은 ContentFormHost에 state와 포획 통지를 전달하지 않았다. 이제 같은 `core.ownerMode.battle`, Client의 현재 checking DWORD와 진단 서비스, 원문 포획 안내 부모를 제공한다. 포획 자체의 기존 경로는 변경하지 않았다.

10.78의 앞단 두 진단은 checking과 무관하다. 부모 dead/free이면 0xbe6, 자신 dead/free이면 0xbe7의 식을 원문 순서로 전달하고 복귀 뒤 현재 두 슬롯을 다시 판정한다. 상태가 여전히 dead/free이면 쓰기 없이 반환하고, 콜백이 고쳤으면 기존 정렬/링크/Pop/부모 통지를 계속한다. CD `004afff0`은 같은 상태에서 진단 없는 반환을 유지한다. 원문 정렬 삽입부터 일반 부모 통지까지의 본문은 불변이다.

일반 통지 뒤 현재 내용물 TYPE·부모 genus·전투 상태를 원문 순서로 읽는다. checking이면 isValid(0xc2b), 다시 현재 checking이면 구조 연결 isRobust(0xc2c)를 진단한다. 여기의 isRobust는 목록 flags의 Robust가 아니라 `004ae080`의 Linked다. 진단이 돌아오면 최종 현재 유효성/연결을 다시 확인한 뒤 원소유자 하위7비트로 안내한다. 기존 SID/type/vtable/순환/할당 슬롯 보호는 유지한다. 공개 API 밖의 메모리나 원문 가상 표를 임의로 허용하지 않는다. 포획 콜백의 사전 요구는 원문 위치인 부착/일반 통지 뒤의 선택 지점으로 옮겼다.

새 `RawPriestTakenNotice`는 10.78 `004927f0`을 복원한다. 같은 실제 사제 참여를 먼저 갱신하고 현재 포획자가 로컬이면 반환한다. 이전 소유자가 로컬이면 ourPriestCaptured.wav와 WarnYourPriestTaken, 중립이면 음향 없이 WarnNeutralPriestTaken, 그 외에는 enemyPriestCaptured.wav와 WarnPriestTaken이다. 재생 인자는 기존 전역 음향의 0/0/0/1/0이다.

안내 제한은 게임 시각이 아니라 `00460d70`의 timeGetTime 기반 벽시각이다. 원문은 제한 중에도 음향을 요청하고 마지막에 현재 시각+20초를 다시 저장한다. 이전 owner별9개 기한은 Client에서 최초0으로 만들고 월드 대여/반납 때 지우지 않는다. 현재 부모 owner를 보관한 다음 이전 이름→포획자 이름 순으로 같은 현재 Player/Chat Identity를 읽어 `Warn...(이전owner,포획자이름,이전이름,포획자색,이전색)`을 실제 Tell에 넘긴다. PE 어셈블리의 push 순서와 고정 색9개·20.0·서식 문자열을 읽어 확인했다. 지역256BYTE와 owner0..8 범위 밖은 호스트 경계로 거부한다. CD 안내 `0040df80`은 이번에 새 구현하지 않았다.

Release 빌드/재링크3회가 모두 종료0·경고/오류0이다. 정적 대조에서 clock 호출이 끝난 뒤 현재 기한을 읽는 순서도 명시하고 해당 부모/게임/집중 대상을 다시 빌드했다. 최종 제품의 같은 집중4개도 다시 통과했다. 집중 **4개(새3개), 실패0**은 일반 생산 내용물의 진단 복귀·미복구 반환·CD 분기·현재 전투/checking 뷰와 Client 기한의 대여 수명, 기존 일반 너겟 SP/지갑 회귀를 검사했다. **포획 안내 본체 실행과 실제 포획 재현은 하지 않았다.** 포획이 포함된 기존 전체 fixture도 재생하지 않았고 새 Native/fixture는 없다. 새 안내의 실행 결과·음향 청취·UI는 사용자 확인 범위다. 실제 게임/GUI·희생·공격/승리·공중3종 전용·MP/통신·물리/PCM·전체 CTest/71대상·34경로·ZIP도 실행하지 않았다.

마지막 전체 정밀 디컴파일은 HJOW-Athlon / 2026-10-23이며 현재 호스트와 같다. 산출물은 변경하지 않았다. checking 잔여는 **29 = 부모21 + 부분5 + MP3**이다. 보호2,739개/fixture919개가 모두 불변이며 다음 기준은 `extracted/cpp-resume-814/protected-after.json`이다. EXE SHA-256은 `045f7e89b4a34ea060905beaa06f3fe0abc3cdea3a9510bd3559d4376f0f889f`다. 원문/어셈블리 발췌·정적 감사·빌드/검사 원시는 `extracted/cpp-resume-814/`, 커밋 근거는 [JSON 보고서](../../cpppj/content-attach-parent-20261103.json)다. 전체 목표는 진행 중이며 다음은 나머지 객체 메뉴/커서와 Graph 부모5개의 현재 checking/소비자다.
