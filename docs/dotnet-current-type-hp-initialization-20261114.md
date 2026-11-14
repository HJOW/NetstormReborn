# dotnetpj 0.528 — 현재 TYPE HP·공장 초기화·제품 보상 연결

2026-11-14, HJOW-Athlon. cpppj 기준 `5207aab92`의 `o/RawRiftHitPoints.h/.cpp`, `o/SquidReward.h/.cpp`의 HP 부분, `o/SquidFactory.cpp`의 PostCreate/PostTake와 선택 HP 부모를 이식했다. 디컴파일·원본/golden 재생성은 하지 않았다.

## 구현

`RawRiftHitPoints`는 checking→현재 HP 플래그→번호→현재 다이스 예외→복귀 가능한 진단→현재 약화→번호→현재 사제TYPE→선택된 물리 칸의 현재 HP 순서를 보존한다. CD는 PATCH 진단을 읽지 않는다. 음수 HP의 1/4 나눗셈은 0 방향으로 절삭한다. 선택되지 않은 번호·진단 장치를 요구하지 않는다.

포인터 차이는 같은 배열을 빌리는 `ArraySegment<OriginalTypeRecord>`의 소유자·시작 위치로 표현한다. 같은 할당의 겹치는 뷰만 허용하며 음수 번호도 DWORD로 보존한다. C# 클래스의 값이나 중복 참조로 배열 주소를 추측하지 않는다. 기록의 필드 갱신과 관리 배열 칸의 레코드 교체 모두 같은 물리 칸 조회로 처리한다. 기존 공장 `ReadSelectedHitPoints`도 이 공통 몸체를 사용한다.

보상 HP는 현재 TYPE 뷰와 사제TYPE/다이스/약화 주소·실제 진단을 빌린다. 실패한 다른 표 연결은 기존 대여를 보존하고, null은 생성 시 표로 복귀한다. 비용·보상 나머지를 전체 현행화한 것은 아니므로 기존 SquidReward PORT_MAP 기준 커밋을 올리지 않았다. CD 보상 생성자 전체 복원도 잔여다.

공장 공통 PostCreate/PostTake는 두 입구와 마지막 진단의 정상 복귀, 현재 슬롯/타입 재조회, 저장한 최초 TYPE·base, 서버 관문과 판본별 owner/zOrder·HP 쓰기 폭을 보존한다. PostCreate는 별도 free/dead/contained 선행 거부로 진단을 대신하지 않는다. HP 진단 실패 때 앞서 쓴 owner/zOrder와 아직 쓰지 않은 HP를 구별한다. 전체 Take/Create의 잔여 판본 분기를 완료로 세지 않는다.

실제 HostSession의 지형·요새 공장과 보상에 같은 장기 GameAux의 사제TYPE·다이스·약화를 연결했다. checking도 값 사본 대신 같은 AiChecking의 현재 DWORD를 읽는다. 제품의 실제 공장 생성과 보상 조회가 같은 HP 결과를 쓰는지 실자산 검사로 확인했다.

## 원본 대조와 검사

- 기존 원본 `rifttype-hit-points-x86.tsv` 1,160관찰(양판본580씩), `factory-initialization-current-x86.tsv` 576관찰(양판본288씩)을 연결했다. 반환·getter 수·진단 식/경로/줄/당시 슬롯·최종 슬롯·현재 전역·풀 수량을 대조했다. golden을 변경하지 않았다.
- 원문 CD 슬롯은36BYTE, 기존 C# 풀은 판본과 관계없이50BYTE 패딩을 둔다. 첫 검사에서 이 폭을 구별하지 않아 CD 비교가 실패했다. 하네스를 원문36BYTE 전체+나머지14BYTE 불변 검사로 교정했다. CD 타입 입력도 기존 공장의188칸 padded API를 사용하며, 이것을171칸 CD 공장 전체 복원으로 세지 않는다.
- 현재 표의 시작 위치 이동을 무시한 대표 변이는 당시 새5검사 중4건이 검출했다. 원본 바이트를 복원한 뒤 공통 몸체를 공장과 공유하도록 확장하고 다시 검증했다.
- 최초 전체 회귀는 Assets366·Client1217 통과, Core2201 중2200 통과/오류 문구 호환1건 실패였다. 기존 검사의 “보상 HP” 기대를 유지하고 실제 오류 문구에 그 접두를 보존했다. 검사를 약화하지 않았다.
- 최종 제품의 전체 비증분 Release 빌드는 경고0·오류0, 최종 관련 Core370/370·Client 전체1218/1218 통과(실패0·각 종료0)다. Core370에는 새 두 fixture, 공장·HP·보상·사제·Carrier·월드·메타데이터가 포함된다. 최종 Core 전수를 다시 완주한 것으로 합산하지 않는다. Windows 실장치 Explicit 검사는 기존 정책대로 미실행이다.

fixture LF정규화 SHA-256:

| 파일 | SHA-256 |
| --- | --- |
| rifttype-hit-points-x86.tsv | `58197e3d412b3a01482039fe00930b831ce302ab63c6de319e6bdd7121d5d9e1` |
| factory-initialization-current-x86.tsv | `77bf5a1893a8078b6b2b4113ab49315ce8e746d9105ae314a3f8a5282e74cea2` |

PORT_MAP807행(L1 387+port420), 공유fixture278개다. 새10차 NB·외부 접속·기존 코드 은퇴는 없다.

## 실제 진입과 보호

HJOW-Athlon 예외PC에서 thewarbegins·savetheisland를 cpp/dn 각120프레임 단순 진입·대기했다(최종4회 종료0). 원본1394파일의 전후 SHA-256과 이름이 동일하다. 최종 dn 덤프2133/2898행은 각각0.512 최종 기준선과 frame행 하나 외에 바이트 동일하다. 현재 cpp와의 L2 전체 일치는 여전히0/34이며, 수집 도구 종료1은 미연결 차이를 뜻한다. 게임 비정상 종료와 혼동하지 않는다.

로컬 자료는 `extracted/dotnet-l2-0528/`(공장 연결 전), `extracted/dotnet-l2-0528-final/`(최종), `extracted/rift-hit-points-0528-original.bin`(변이 전 바이트)에 있다(Git 제외). AGENTS.md·원본/CD·README 개요·cpppj·LEFT_JOBS.md·webpj를 수정하지 않았다. 사제 포획/희생·특정 적 공격·시나리오 승리·MP 재현은 하지 않았다.

## 다음

0.529는 같은 실제 HP/공장을 바탕으로 제품 HostSession의 사제 월드·Pop/Unpop/Repop·공유 프로세스·소리·피해/해방을 연결한다. 지금의 `RawPriestWorldBindings` 부품과 장기 owner는 준비됐으나 제품의 전체 사제 경계는 아직 연결하지 않았다. 일반 지역 정책·AI 시작·다른 파생 후처리·M1전체0/34와 이후 R5~R8은 계속 남는다. 목표는 active다.
