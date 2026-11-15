# dotnetpj 0.547 — 감시 Form의 현재 공장·생성·가상 삭제

2026-11-15, HJOW-Athlon. R4-1의 `RawDependCreate` 계열과 `DependFormHost` 현재 몸체를 이식했다. 기준은 cpppj 제품1313의 `o/RawDependCreate.cpp/.h`·`DependFormHost.cpp/.h`와 `DependCreateCurrentTests.cpp`, World의 실제 `BindCreation`·`BindUnpop` 호출이다.

## 구현

두 감시 Form은 고정 VT·타입1의 직접 할당 대신 같은 현재 공장으로 생성한다. 각 Create 직전에 장기 `DependFormType` DWORD를 따로 읽고, 첫 할당이나 진단이 바꾼 번호·생성자·반환 객체의 현재 가상표를 보존한다. PATCH와 CD의 WORD 기록 및 첫 VT 저장 시점 차이를 그대로 옮겼다. 첫 부착 뒤 두 번째 VT를 다시 읽으며, 선택된 ProcessForm의 별도 부착 접두가 없으면 앞선 두 할당·링크를 남기고 명시 실패한다. 생성 부모에 원문에 없는 free/dead 사전 거부를 추가하지 않는다.

`DependFormHost`는 현재 TYPE가 달라도 실제 감시 VT인 이전 객체를 삭제할 수 있다. 검색·제거는 현재 전체 DWORD와 원문의 Form 후보 필터를 사용한다. `BindVirtualDestroy`·검색 직전 진단·공장 Unpop 연결을 옮겼고, 장치가 없는 독립 경로는 기존 공통 삭제를 사용한다. `RawSurfaceWorld`는 같은 공장과 장기 `RawGameAuxState.DependFormTypeBox`를 연결한다. 일반 World 초기화로 번호를1에 되돌리지 않는다. 전체 NativeStorage 장기 수명은 여전히 후속 대상이다.

## 검증

`depend-create-parent-current-x86.tsv`의 PATCH810·CD810, 합계1,620개 최초 PE 관측을 읽기 전용으로 연결했다. 명시 생성/진단/부착 장치, 번호와 VT 변경 입력, 사건 순서, 두 슬롯 전체를 대조했다. CD 관측에는 실제36BYTE stride를 사용한다. 실제 공장 관련 CD 합성 검사는 현재 관리 공장의188칸 하네스와 알려진 CD 생성자 주소로 검사한 것이며, CD171타입 월드나 CD게임 전체 지원을 뜻하지 않는다.

추가 실제 검사에서는 다른 풀 바인딩 거부, 선택된 Process 접두 부재 뒤 할당/링크 보존, TYPE1의 현재 Gump 생성자 사용, World 첫 진단이7→8로 바꾼 두 생성과 이전 클래스 재귀 삭제, uint최댓값에서 첫 할당 뒤 범위 실패, 일반 초기화의 현재 타입 보존, Native 검색·공장 해제·같은 공통 삭제 깊이를 확인했다. 신규8검사와 기존 관련 검사를 합쳐 **149/149** 통과했다. 호스트 Release 빌드는 경고0·오류0이다. 전체 회귀는0.546의 결과를 유지하고 이번에는 영향 범위를 집중 검사했다.

최초137검사의 실패5는 CD171칸을 아직188칸 계약인 관리 공장에 공급한 새 검사 입력2건과, 기존 합성 검사의 빈 생성자 입력3건이다. 실제 입력을 보완한 다음148검사에서는 혼합 체인의 빈 감시 생성자 입력1건과 free 부모 선행 거부를 기대하던 옛 합성 검사1건이 남아 교정했다. 기존 원본 fixture 기대값은 수정하지 않았다. 최초 실패와 최종 TRX는 `extracted/dotnet-depend-0547-initial/`·`-final/`·`-final2/`에 보존했다.

34경로 L2는 cpp 바이너리 SHA가0.546과 같은 것을 확인해 cpp34덤프를 재사용하고 dn31경로만 새로 실행했다. 신규31회 모두 종료0·수집 오류0이며 원본1,394파일 전후 이름·SHA가 동일하다. dn31경로의 슬롯·그래프·난수 등 모든 상태 행은0.546과 같다. 9경로는 파일 전체가 동일하고22경로는 `frame` 한 줄만 다르다(미션 적재 중 호스트 Update 횟수90~94의 차이). 이 메타데이터 차이는 지우지 않고 원자료에 보존했다. cpp와의 전체 일치는 여전히0/34, 데모dn3경로는 미연결이다. 자료는 `extracted/dotnet-l2-0547/`이며 재사용 출처·바이너리/덤프 SHA는 각cpp폴더의 `reuse.json`에 있다.

## 인수인계

PORT_MAP894행(L1 424·port470), fixture301개다. 새3쌍과 기존 `DependFormHost.cpp` 대응의 기준만 갱신했고, 다른 기존 대응·fixture해시·floor/reflected는 유지했다. 이 단위는 R4-1의 감시 Form 계열 완료이며 R4-1 전체나 M1 완료는 아니다. 장기 NativeStorage, 현재 connector, Content 정렬·통지·수명 몸체, 제품 피해/해방·삭제/배치 상위 결선 뒤 R4-2로 진행한다.

사제 포획/희생·특정 적 공격·승리 재현과 MP 실행 검사는 하지 않았다. cpppj·LEFT_JOBS.md·AGENTS.md·originals/originalCD·README 개요와 다른 세션의 webpj 변경은 수정하지 않았다.
