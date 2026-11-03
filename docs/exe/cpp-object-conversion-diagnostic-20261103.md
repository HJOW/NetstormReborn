# 오벨리스크/스펠 전환의 실제 타입 변경 진단 복귀

단위 812는 오벨리스크↔스펠 전환의 실제 Session 타입 변경 진단을 현재 Client 서비스에 연결했다. 원문 `004b1620`은 isVoid(0x22d)·가로 발자국(0x22e)·세로 발자국(0x22f) 진단에서 돌아오면 TYPE BYTE와 실제 생성자를 계속 적용한다. 기존 SquidFactory.Retype은 이를 복원했으나 Session의 callback이 모든 진단을 즉시 예외로 중단하고 있었다.

`MakeObjectConversionHooks`가 같은 풀의 실제 Factory.Retype과 프레임 DWORD(+36)·QA WORD(+12)를 제공한다. 식/파일/행은 원문대로 전달하고 실제 Session은 현재 `services.ai.assertion`을 소비 시점에 읽는다. 서비스가 복귀하면 생성자·기본 프레임/QA·Repop을 계속한다. 장치가 없으면 선택된 진단 위치에서만 예외이며 정상 입력에 불필요하게 요구하지 않는다. 다른 Factory 풀/판본은 연결 시 거부한다.

기존 ObjectConversionBody의 17VT 분배와 RawObjectConversion.Command 본문, Factory.Retype 본문은 바꾸지 않았다. Session의 Unpop/Repop·defaultFrame·공통 부모 callback도 원문 위치와 기존 내용을 보존했다. 프레임 쓰기는 Frame.Set 호출로 바꾸지 않고 직접 DWORD를 쓰며 QA는 하위 WORD만 갱신한다. 현재 제품의 실제 Retype 명령 소비자는 Session 한 곳이고 RawSurfaceWorld의 ObjectConversionBody 참조는 읽기 전용 예약 관측이다.

집중 **6개(새3개)가 실패0**이다. 새 바인더로 517의 독립 원문 타입 변경128행과 두 가상 입력36행을 재생해 전체50BYTE 슬롯·진단/공간/부모 사건·반환값을 비교했다. 기존 직접 재생도 유지해 고유164행을 두 경로에서 총328회 C++ 대조했다. Unpop 뒤 QA/TYPE 재조회·release/상위 비트·원문 생성자 없는 fallback도 포함한다. 추가 검사는 진단 없는 정상 경로, 선택한 장치 누락의 쓰기 전 중단, 세 진단의 원문 식/파일/행 순서와 복귀, 서로 다른 풀 거부, 프레임/QA의 정확한 쓰기 폭과 다른 바이트 보존을 확인했다.

Release 게임/집중 대상 빌드1회가 경고·오류0이다. 새 Native 관측은 없고 기존 두 fixture의 SHA를 [517 보고서](../../cpppj/object-conversion-native-20261028.json)와 대조했다. 실제 Client 연결은 소스/빌드 근거이며 공간/상위 입력은 기존 명시 장치다. 실제 게임·우클릭/전환 GUI·사제 포획/희생·공격/승리·공중 3종 전용·MP/통신·물리/PCM·전체 CTest/71대상·34경로·ZIP은 실행하지 않았다. 모든 타입/소유자/상태별 메뉴·명령 효과와 전체 동일성을 완료한 것은 아니다.

마지막 전체 정밀 디컴파일은 HJOW-Athlon / 2026-10-23이고 현재 호스트가 같다. 산출물 변경은 없다. 세 진단은 checking과 무관하므로 checking 잔여는30(부모21·부분6·MP3)을 유지한다. 보호2,739개/fixture919개가 전부 불변이고 새 fixture는 없다. 다음 기준은 `extracted/cpp-resume-812/protected-after.json`이다. EXE SHA-256은 `5f02c9a61963e7b012013d35437073c923c5c4a6594aa12f7d270dcc78583281`다. 원시 빌드/검사/본문 불변 감사는 `extracted/cpp-resume-812/`, 커밋되는 근거는 [JSON 보고서](../../cpppj/object-conversion-diagnostic-20261103.json)다.

인계서 3절에 남아 있던 808 당시의 일시중지 안내도 현재 사용자 `/goal` 진행 상태와 완료 경계(809·810→811)에 맞게 고쳤다. 원본과 AGENTS.md는 수정하지 않았다. 다음은 우선순위1 객체 메뉴·커서의 나머지 상태별 효과/수명이며 전체 목표는 진행 중이다.
