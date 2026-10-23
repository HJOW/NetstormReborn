# 단위356: Fort 섹션 선택의 원본 호출 순서

시작 HEAD는 `1c625ee6 / 20261023-19 CP`다. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-23**이며, 단위355에서 확인한 기존 정밀 원문을 재사용했다. 디컴파일 소스 변경은 없다.

## 문제와 결과

기존 `MakeFortWorldLoadBinding` 및 `MakeFortWorldChildrenHooks`는 `loader.Load(selector())` 형태였다. C++는 로더 본문에 들어가기 전에 selector 인수를 평가하므로 원본의 로딩 깊이 증가, Template 진단 및 기술 부모 조회보다 실제 manager 선택이 먼저 실행됐다. 진단/부모 조회가 현재 섹션이나 상태를 바꾸면 잘못된 cursor를 소비할 수 있었다.

다음 실제 원본 순서에 맞춰 loader 안에서 selector를 실행한다.

| 원본 | 선택 시점 |
|---|---|
| Money `004bf340` | 검사 횟수 설정 → 깊이 증가 → Template 진단 → Money 선택0 → SP 본문 → 깊이 감소 |
| Badges `004be8e0` / CD `00428fc0` | CD만 즉시 깊이 증감 → Template 진단 → Badges 선택1 → 배지 본문 |
| Technology `004beee0` | 깊이 증가 → Template/isMine 진단 → 현재 기술 부모 조회 → Technology 선택1 → 변환/내용물/해제 → UI → 깊이 감소 |

`RawFortMoneyLoad::LoadSelected`, `RawFortMetaLoad::LoadBadgesSelected/LoadTechnologySelected`는 selector를 저장하지 않고 그 위치에서 즉시 호출한다. 실제 selector는 같은 manager 노드의 선택과 pos0 초기화를 수행해야 한다. 기존 직접 cursor 입력 API는 같은 위치에서 pos0을 수행하는 selector로 기존 몸체를 재사용한다. SP·배지·내용물 계산을 복사하거나 두 번째 구현으로 나누지 않았다.

실제 Fort binding도 새 경로를 사용한다. Meta의 현재 Template/owner 상태 공급은 wrapper 진입 전에 하고, 실제 스트림 선택은 위의 원본 시점으로 지연한다. Money의 Template 존재 등 현재 상태도 wrapper 진입 전에 공급해야 하며 selector를 상태 초기화의 대용으로 쓰지 않는다.

## 검증 범위

기존 두 판본의 Money 원문408입력, Meta 전체 장부/Technology wrapper 원문 및 Fort 부모 원문534입력을 바꾸지 않고 다시 대조한다. 새 검사는 다음을 확인한다.

- Money 진단에서 현재 cursor가 바뀌면 증가한 깊이에서 새 cursor를 선택·소비하며, 이전 cursor 위치는 보존한다.
- PATCH/CD Badges의 서로 다른 깊이 순서와 Template 진단 뒤 선택을 확인한다.
- Technology의 두 진단과 부모 조회 뒤 선택, 같은 부모 SID/flags17/변환 수명/UI/깊이를 확인한다.
- selector 실패는 이미 발생한 부모 조회 등을 되돌리지 않고 기존 호스트 예외 처리로 깊이를 복구한다. 이 검사는 원본 SEH의 모든 실패 변이를 복원했다는 뜻이 아니다.
- 누락 selector는 효과 전에 거부한다. 실제 두 판본 Fort binding 검사에서도 Money 선택 깊이2, Badges 깊이1, Technology 부모 뒤 깊이2의 순서를 확인한다.

Release 빌드1은 **종료0·경고0·오류0**이다. 새3개를 포함한 관련 **33개·실패check0**이 통과했다(Money4/Meta5/FortWorld2/Fort binding1/CampaignTemplates15/EditorFiles4/DataManagerSelection2). 원본 fixture/기대 결과는 바꾸지 않았으며 변경 C++9개가 UTF-8임을 확인했다.

최종 전체 CTest1은 **2340개·실패check0·종료0/179.76초**다. LastTest.log의 `2340 test(s), 0 failed check(s)`로 내부 집계를 확인했다. 로그는 `extracted/cpp-fort-selection-order-20261023-*`에 남겼다. 제품/검사 C++는 최종 빌드/관련·전체 검사 뒤 변경하지 않았다. 인수인계와 이 기록을 마감하여 **20261023-20 CP**로 로컬 커밋하며 전체 목표는 계속 활성이다.

## 남은 목표

이번 단위는 실제 하위 로더와 binding의 선택 순서를 수정한 것이다. Client의 Edit 진입, generic Fort 설치 전용 세션, 실제 Money/Meta 전역·장부/기술 변환/UI와 파일 저장 장치, alternate populate 및 전체 사용자 플레이 목표는 여전히 남아 있다. 테스트에서 명시한 경계 callback을 전체 실제 게임 효과의 완료로 세지 않는다. 원본/CD·AGENTS.md·기존 fixture·다른 프로젝트 수정이나 원격 push는 하지 않는다.

다음 독립 연결은 맵 목록의 가변 FileSpec 키다. 보호 PE를 pefile로 직접 읽어 `005327ab`의 `?fortSpec\0`과 포인터 `005327e0=005327ac`를 다시 확인했다. Tell의 `*SOLOLIST*`(정밀 원문159342)는 dirty1→같은9BYTE 버퍼를 `SOLOSpec\0`으로 변경→Refresh2/행 작성→dirty1→`fortSpec\0` 복구다. 현재 SoloMapCatalog의 열거/분류/별도 메타 읽기는 `fortSpec` 고정이다. 각 PathSpec 직전에 현재 키를 조회하도록 연결하며 키 변경 자체로 캐시를 자동 무효화하지 않아야 한다. 00460650/00460680/00460c40의 현재 키 getter 순서도 같은 기준이다.

이후 020000c9는 모드2/lastEditFort→pending 이름→Refresh2→LoadEditable이고,020000ca는 pending→모드0이면State6/그 외 next State1 요청이다. 둘을 즉시 월드 설치로 대체하지 않는다. 현재 이름 변경00460980은 snapshot/manager 이름 변경/기존 파일 제거 요청까지 포함하므로 pending setter00460bb0과 합치지 않는다. State1의 준비/복구004b5ca0,편집 Player 초기화004904b0,새 파일004bfba0/004b3920,현재 월드 snapshot004bfb50 및 `$Input` 컨트롤은 별도 잔여다. 보호 원본 제거 요청을 자동 실제 삭제로 연결하지 않는다.
