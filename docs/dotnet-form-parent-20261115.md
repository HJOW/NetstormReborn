# dotnetpj 0.546 — 현재 Form 부모 체인과 내용물 통지

2026-11-15, HJOW-Athlon. R4-1 객체 수명 공통층의 `RawFormLinked`·`RawContentParentNotice`·`ContentNoticeVtables` 계열을 옮겼다. cpppj 제품 1313의 해당 소스와 `RawFormLinkedTests.cpp`·`CampaignNoticeTests.cpp`를 근거로 사용했다. 디컴파일이나 보호 원본 파일 변경은 없다.

## 구현

`RawFormLinked`는 부모 getter를 한 번 호출한 뒤 현재 풀 물리를 다시 빌린다. 부모 체인의 첫 자기 발견 뒤에도 끝까지 검사하며, 뒤에 범위 밖 번호가 있으면 false, 순환이면 명시 오류다. 타입과 상태 필터는 추가하지 않았다. ProcessForm의 양판본 getter는 해당 가상표를 확인한 뒤 부모 WORD 전체를 읽는다. C# 관리 풀의 CD 슬롯은 기존과 같이 50BYTE이며, 작은 독립 체인 검사는 명시 stride를 받는다.

`RawContentParentNotice`는 PATCH 83개·CD 68개 생성자 가상표와 네 내장 Form의 실제 VT+24를 그대로 사용한다. 원본 빈 통지는 내용물 인자나 부모 TYPE·상태를 읽지 않고 반환한다. 파생 통지는 전달한 실제 소비자를 호출하고, 소비자 또는 가상표가 없으면 명시 오류로 끝난다. 주소는 분배 토큰이며 호스트 함수 포인터로 실행하지 않는다.

World의 기본 `RawSquidContainedUnpop`에 현재 가상표 분배기를 연결했다. 파생 통지만 현재 World runtime의 `ContentNotice`를 요구한다. 독립 `RawSquidForms.Unpop`의 명시 통지 callback 계약은 cpppj처럼 유지했다. 전체 NativeStorage의 장기 소유권이나 세션 파생 통지 본체를 완료한 것으로 세지 않는다. `RawFormLinked`의 실제 건설 프로세스 소비자는 R6에 남는다.

## 검증

`form-linked-parent-x86.tsv`의 원본 관측 120행을 읽기 전용으로 연결했다. 자기 뒤 범위 오류, 부모 getter 중 풀 재확보, 순환, 물리 범위 실패, 양판본 부모 WORD, 빈 몸체의 바이트 불변, 워크샾 파생 인자와 미지원 가상표를 검사했다. 실제 World 공장의 `UnpopForm`으로 기본 부모 내용물 목록을 해제하는 검사도 포함했다. 신규 11검사와 기존 관련 검사를 합친 155/155가 통과했고, 누락된 `NETSTORM_DATA` 환경을 바로잡은 기술 검사도 이 실행에서 통과했다. 산출물은 `extracted/dotnet-form-parent-0546-final/`과 별도 빌드 경로 `extracted/dotnet-artifacts-0546/`다.

호스트 Release 빌드는 경고 0·오류 0이다. 재개 전체 회귀는 Assets366/366·Client일반1,232/1,232·Core2,393/2,394로 완주했다(Core17분39초, 큰 독립 저장 검사 포함). Core의 실패1은 `NETSTORM_DATA` 미지정 기술 검사이며, 올바른 데이터 경로를 설정한 이번155/155 집중 실행에서 같은 검사가 통과했다. 전체 최초 실행의 종료1을 전체 무실패 결과로 바꾸지 않는다. 실장치 오디오Explicit1은 기존처럼 미실행이다. 원시 TRX는 `extracted/dotnet-regression-0546/`에 있다.

전체 검사와 집중 검사의 동일 출력 경로가 겹쳐 집중 빌드가 DLL 잠금으로 실패했으며, 집중 검사는 분리한 산출물 경로에서 재실행해 통과했다. 두 검증 환경 실패를 제품 결함이나 성공 결과로 합산하지 않는다.

34경로 L2를 같은 fixed unix 1760000000·25ms·영어·1024×768·창 모드·음소거·빈 사용자 루트로 다시 수집했다. 일반 미션31개는 양쪽, 데모3개는 cpp만 실행했으며 총65회 모두 종료0, 수집 오류0이다. 전체 일치는 여전히0/34이고 데모 dn 진입은 미연결이다. M1 개선으로 합산하지 않는다. 대표 `thewarbegins`의 dn 2,140행은 0.545의 `f090.tsv`와 프레임 행까지 전부 동일하며 1,856개 슬롯도 그대로다. 원본1,394파일의 이름·SHA-256은 전후 동일하다. 원자료·실행 인자·로그·감사는 `extracted/dotnet-l2-0546/`에 있다(Git 제외). 이 실제 창 실행은 AGENTS의 HJOW-Athlon 예외를 적용했고 조작은 미션 진입·대기·덤프뿐이다.

## 인수인계

PORT_MAP 891행(L1 423·port 468), fixture 300개다. 기존 대응의 전체 기준 커밋·fixture 해시·floor/reflected는 변경하지 않았고 새 5쌍과 원본 관측 1개만 추가했다. R4-1의 남은 대상은 장기 NativeStorage 공장·공간·World 수명, 현재 connector·Depend/Content 파생 몸체, 제품 피해·해방·삭제·배치 상위 결선이다. 이후 R4-2 적재 중 Pop 후처리 순서를 유지한다. M1 0/34와 새 경로 플레이 불가는 유지한다.

사제 포획/희생·특정 적 공격·승리 재현과 MP 실행 검사는 하지 않았다. cpppj·LEFT_JOBS.md·AGENTS.md·originals/originalCD·README 개요·다른 세션의 webpj 변경은 수정하지 않았다.
