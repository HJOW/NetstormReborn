# 숫자 송금 확인의 현재 checking과 대상 수명

단위829의 완료 기록이다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**이며 직전828은 `1e7bf89e / 20261104-10 CP`다. 제품829/11.01이며 숫자 확인 부분004c9350을 완료 집계해 checking18=부모11+부분4+MP분류3이다. 완료부모31/완료부분2다.

원문004c9350은 현재 편집 문자를256BYTE로 복사하고 첫LF/끝NUL과 CRT 변환 뒤 양수 금액을 판정한다. PlayerKnown이면 첫 현재 SP, 다음 현재 SP와 Editor를 비교한 뒤 창+d4의 **현재 대상**을 저장한다. 현재 Player+50의0x20이 있으면 AI 가지를 우선하며 현재 checking이 비영이고 missionPost가0이면 `inSingle`, `.\Ui.cpp`, **0x61c(1564)** 진단을 호출한다. C 디컴파일에는 줄 번호가 빠졌으나 실제 기계어 PUSH로 확인했다. 진단이 돌아오면 DWORD neg→signed float 차감과 저장 대상/금액의 AI 지갑 가산을 계속하고1을 반환한다. 진단이나 차감이 창+d4를 바꿔도 대상을 다시 읽지 않는다.

기존 SendMoneyHooks.single은 이름과 달리005e4794 checking이며 이전 관측기/fixture의 single 열도 같은 주소다. fixture 열/기대값은 바꾸지 않고 코드 필드만checking으로 명확히 했다. 실제 MoneyInputHooks는 상수1 대신 같은 Client AiDebug.Checking을 읽고 stderr 전용 진단 대신 같은 services.ai.assertion에 연결한다. 고정 명령도 이 동일 현재 값/장치를 사용하며 All에는 기존 무진단 정책을 유지한다. 신규 MP 검사는 하지 않는다.

숫자 ConfirmSendMoney와 SaveFortNameDialogContext.sendMoney는 현재 대상DWORD를 const참조로 빌린다. 실제 AcceptSaveFort→UberEditor→Session의 경로가 자금 판정 뒤 처음 읽고 Transfer의 값 인자로 저장하도록 연결했다. 값을 일찍 복사하던 UI lambda도 const참조로 바꿨다. 실제 연결 플레이어/송신 가지 구현을 검증 완료로 확대하지 않는다.

독립 Windows x86 관측은 원문004c9350만 실행하며 CRT/CopyText/SP·진단·차감/AI지갑은 명시 외부 경계다. 연결 Player/패킷 가지에는 도달 시 실패 장치를 두고 AI/미선택432행만 실행했다. checking0/상위비트/ffffffff·mission/known·AI 우선·0/음수/공백/개행/정수·두SP/Editor 및 SP/진단/차감 뒤 현재 대상·checking 변경을 포함한다. 진단112행이며 재관측432행이 정확한 바이트로 일치했다. fixture SHA-256은 `ee558e75e4547bd7991219d21da58c236818becdc1ca2c8120474ded5127b229`다.

최초 프로브 컴파일은 성공했지만 독립 payload 시작00410000 아래의0040ec50을 직접 연결한 방식은 실행 접근 위반3221225477이었다. 최초 로그를 보존하고 해당 GetSp/차감은 원문 부모의 CALL 피연산자만 연결하도록 관측기를 교정했다. 실패 실행의 fixture는 없으며 제품/원본/기존 fixture를 바꾸지 않았다. 두 프로브 컴파일은 종료0이다. 이 관측 범위 밖의 payload/SEH·원문 손상 문자/스택·전체진단UI와 정확한 CD 숫자 확인 대응은 별도다. 자동 match.tsv의 CD 같은004c9350 행을 패치004c9350의 대응이라고 단정하지 않는다.

원시는 `extracted/cpp-resume-829/`다. 실제 게임/GUI·포획/희생/특정 적 공격/승리·사용자 확인·새 MP/공중3종 검사·물리/PCM·전체71/34미션/CTest/ZIP은 실행하지 않는다. 최종 집중9개(새4)가 실패0이며 Release 빌드3회/프로브2회는 종료0·경고/오류0이다. 처음 Session 집중3개에서3단언 실패는 Fort 로드만으로 AI 참여가 준비됐다는 검사 가정이었다. 실제 기본 AI 설정/생성자와 같은 Client의 명시 local/known 입력으로 준비를 고쳤으며 숫자700 두 번과 고정300 한 번의 실제 SP·지갑을 확인했다. 최초 실패 로그와 기존 fixture를 유지했다. 기존2755개 불변·새 fixture 포함2756개/936개다. 제품 EXE SHA-256은 `b8cfd29b41f31e05b36c0034478046f81ec0347d5275264a1f68923372611934`다. 마지막 빌드 뒤 수정한 UberEditor.inc 한 줄의 CR만 제거했으며 의미 변경은 없다. [829 JSON](../../cpppj/money-confirm-checking-20261104.json)에 모든 결과/보호 해시를 기록했다. 다음830에서는004d62b0의 세 checking 지점에 대한 기존 Guide/Console 분류를 원문 고정 송금 가지와 다시 대조한다.
