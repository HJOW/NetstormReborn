# dotnetpj 0.542 — 공통 Form 해제의 현재 VT와 복귀 진단

2026-11-15, HJOW-Athlon. 직전0.541은64e305fa8다. 공통 Form 기준은cpppj o/SquidForms.h/.cpp09ad72b071d7864e114f671b82ec5067cbb01c62다. Factory Unpop 소비자는SquidFactory.cpp be567d46b5eca30d2a1421af78d49125add1ff6a, 실제 공통 Detach의 free 물리 쓰기는RawSquidContainedUnpop.cpp 같은 be567d46 기준으로 대조했다. 바이너리·디컴파일 산출물은 변경하지 않았다.

## 현재 공통 부모

FormUnpopHooks에 현재 VT+0 부모 getter와 정상 복귀 진단을 추가했다. Unpop은 호출 시작의 함수 사본을 유지하고 TYPE>69·유효 SID·void 세 단언을 도달한 순서대로 처리한다. 진단이 반환하면 현재 raw를 다시 읽는다. CD는 void OR 앞의 VT, PATCH는 OR 뒤의 VT를 선택하며, void 기록 뒤 현재 부모 getter를 실행하고 그 뒤 현재 부모 free를 읽는다. 부모가 free이면 링크를 유지한 채 반환한다. 아니면 실제 대여한 Detach 또는 RawSquidContainedUnpop의 공통 종단으로 간다. 부모/이웃/알림 선검사로 앞선 void·링크 변경을 취소하지 않는다.

RawSquidForms.Form의 명시 getter 검사는 raw TYPE가 아닌 실제 네 Form VT를 사용한다. 두 판본 VT 표와 지정 physical SID에 DWORD 하나만 쓰는 ConstructVtableAt을 연결했다. 기존 Patch 전용 정적 VtableFor 계약은 유지한다. 이 primitive의 CD 대조를 전체 CD 게임/관리 풀 복원 완료로 세지 않는다.

Factory.UnpopForm과 독립 ProcessHost.UnpopForm은 같은 현재 진단을 공통 부모에 전달한다. Factory는 현재 분리 함수를 빌리며 연결 반납 뒤 독립 기본 종단으로 돌아간다. 공통 ContainedDetach의 마지막 상태 BYTE는 AllocatedBytes 선검사 대신 원문 LinkRaw와 같은 RawSlot 물리에 써서 복귀 진단 뒤 free/SID0도 보존한다. Factory.Take 전체는 아직 옛 선검사 경로다. 별도 RawSquidTake 부모와 장기 Factory runtime 대여가 필요하며 이번 완료에 합산하지 않는다.

## 원본 관측과 현재 계약

form-unpop-current-x86.tsv의 PATCH864·CD864, 합계1728개 원본 관측을 연결했다. TYPE/상태/부모상태/진단복귀 프로필/SID 입력과 명시 진단·부모 getter·Detach 장치를 공급하고, 원문 사건 문자열과 대상/부모 두 슬롯 전체를 비교했다. 진단/getter/Detach는 원본 자료와 같은 외부 장치이며 실제 게임·MP·실제 자식을 실행한 것으로 세지 않는다. 기대값은 변경하지 않았다.

현재 C# 관리 풀은 판본 태그와 관계없이 50BYTE 슬롯을 쓴다. CD 관측은 실제36BYTE stride로 입력을 만들고 두 원본36BYTE를 비교하며 관리 패딩14BYTE 불변도 따로 검사한다. 최초 CD 실패는 이 입력 어댑터가50BYTE stride를 쓰던 오류였고, 원본 관측/제품 결과를 바꾸지 않고 수정했다.

추가8경우는 알림 누락 뒤 링크/void 부분 효과·payload, 복귀 진단 뒤 고친 VT/TYPE69 실제 getter, 호출 중 장치 재결합과 함수 사본, 양판본 예약 SID0의 단일 생성자 DWORD, 실제 Factory/현재 Detach/반납 뒤 기본 종단, free/SID0의 마지막 물리 상태 쓰기를 검사한다. 원본 재생2+합성8=새10검사와 기존 관련98, 집중108/108 통과했다. 기존 ContentNoticeTiming 합성 검사의 옛 무효과 선검사 가정은 현행 원문처럼 알림 앞의 void/링크 변경을 보존하도록 고쳤다. 이후 성공 입력은 명시적으로 다시 부착된 별도 입력으로 공급한다. 기존 원본 fixture300행의 기대값은 그대로 통과했다.

현재 Detach를 재조회하는 변이는 새10검사 중1개, 알림을 앞당겨 선검사하는 변이는1개, 마지막 쓰기를 AllocatedBytes로 제한하는 변이는2개 실패로 검출했다. 각 실행 뒤 제품 두 파일의 바이트/SHA-256을 복원했다. 최종 전체 비증분 Release 빌드는 경고0·오류0, 전체 Core2370/2370(16분24초)·전체 Client 일반1232/1232(2분41초)는 실패0·각 종료0으로 통과했다. Client 목록1233 중 실장치 오디오 Explicit1은 미실행이며 실제 재생 통과로 세지 않는다. 0.529의 환경 누락 후 단일 재검사와 달리 이번 Core는 NETSTORM_DATA를 지정한 한 번의 전체 실행에서 모두 통과했다. 자료는 extracted/dotnet-form-unpop-0542-initial/·-mutations/·-final/(Git 제외)에 둔다.

## 남은 연결

제품 Host Native 활성화는 아직 하지 않았다. Native Take 부모/현재 가상 해제/생성자 반환·Factory runtime 대여와 실제 World 기본 분리 소유자, 표면 connector/Native 다리·Depend/Content/Gump 현행 몸체·피해/해방·사제 상위 배치/삭제가 후속이다. 원본과 같은 새 링크 부모를 옮긴 결과를 전체 제품 삭제·플레이 완료로 세지 않는다.

PORT_MAP868행(L1 419+port449), 새 header/분리 Unpop3쌍만 추가하고 미이식 부분이 있는 기존 Forms/Factory 전체 base는 유지한다. fixture295개(신규1)이며 기존294개 값과 floor/reflected는 그대로다. 새 게임창/금지 실제 플레이/MP·보호 파일/다른 프로젝트/디컴파일 변경 없음. M1 전체0/34·목표active다.
