# 단위355: 편집 Fort의 경제·메타 섹션 선택 연결

시작 HEAD는 `0e3a7418 / 20261023-18 CP`다. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-23**이며 기존 정밀 원문을 읽었다. 새 디컴파일/기존 디컴파일 소스 변경은 없다.

## 문제와 원본 근거

`CampaignTemplateStorage`는 실제 manager와 연결된 State·Territory 선택만 제공했다. 편집용 generic Fort 설치에 필요한 Money·Badges·Technology는 별도의 임시 스트림을 만들지 않고 같은 manager의 목록/current와 원시 섹션을 사용해야 한다.

기존 `extracted/refined/originals/Netstorm.c`에서 다음 선택 인자를 확인했다.

| 원본 10.78 | 실제 선택 | 의미 |
|---|---|---|
| `004bf340` | `0044ceb0(manager,"Money",0)` | 빈 읽기는 원본 진단을 거친다. |
| `004be8e0` | `0044ceb0(manager,"Badges",1)` | 빈 섹션도 허용하고 실제 데이터/end를 확인한다. |
| `004beee0` | `0044ceb0(manager,"Technology",1)` | 빈 섹션에서도 현재 기술 부모 조회/UI 마감은 수행한다. |

선택 본체는 이미 원본 864입력으로 검증된 `SelectDataManagerSection`을 재사용한다. 새 선택 정책이나 결과 값을 재계산하지 않는다.

## 변경

- 같은 `Image`에 Money의 pos/end와 Badges·Technology의 bytes/pos/end/present를 보유한다. 빌린 Template 헤더도 같은 객체를 반환한다.
- 비선택 getter는 manager.current를 바꾸지 않는다. 각 selector는 실제 원본 이름/모드로 manager.current를 선택하고 해당 위치만0으로 초기화한다.
- 공개한 cursor의 소비 위치는 같은 실제 노드에 반영한다. 원시 섹션 버퍼가 교체되면 새 버퍼와 현재 end/present를 다시 연결한다. 다른 섹션의 선택으로 소비 위치를 초기화하지 않는다.
- 값형 설치와 실제 파일 이미지 읽기 부모를 모두 지원한다. 원본 raw 섹션 데이터, 기존 이름 목록/생성·반납/저장 알고리즘 및 과거 fixture는 변경하지 않는다.

## 검증

빌드1에서 새 매개변수 `name`이 기존 파일명 멤버를 숨기는 C4458이 나왔다. `sectionName`으로 수정한 **최종 Release 빌드2는 종료0·경고0·오류0**이다.

두 새 검사는 실제 TEST01의 값형/파일 설치, 빌린 헤더, 같은 세 cursor/버퍼, 선택 사이 소비 위치 보존, State/Territory 선택 후 보존 및 빌림 반납을 확인했다. 명시 raw 섹션 입력을 기존 생산 로더에 전달하여 저장된 Money 값의 직접 대입 대신 **194162 SP** 정책, 두 배지의 count DWORD만 변경/패딩 보존, 빈 기술의 부모 조회·UI/로딩 깊이 복구, 빈 배지 허용 및 빈 Money 진단을 검사했다. 실제 기술 내용물 생성이나 전체 편집 세션을 이 새 검사로 완료 처리하지 않는다.

관련 검사는 **총35개·실패check0**이다. CampaignTemplates15, DataManagerSelection2(원본864입력 포함), FortMoneyLoad3, FortMetaLoad3, EditorFiles4, FortWorldLoadBinding1, TemplateLifetime1, SoloMapCatalog6이다. 로그는 `extracted/cpp-fort-streams-20261023-*`에 있다.

최종 전체 CTest1은 **2,337개·실패check0·종료0·200.27초**다. 내부 집계는 LastTest.log의 `2337 test(s), 0 failed check(s)`로 확인했다. 제품/검사 C++는 최종 빌드2와 관련/전체 검사 뒤 바꾸지 않았다. `git diff --check`, 변경 소스 UTF-8 해석 및 보호 경로 미변경을 확인했다.

## 남은 실제 연결

이번 단위는 실제 저장소/섹션 선택 연결이며 Edit 진입은 아직 `NotImplemented`다. 다음은 `RawFortWorldLoad`를 사용하는 편집용 설치 진입, Session의 실제 Money 상태·badge 장부·기술 부모/변환/UI·작업본 파일 저장 장치다.

실제 wrapper의 원본 선택 시점도 유지해야 한다. Money는 로딩 깊이 증가/Template 진단 뒤, Technology는 깊이/Template·isMine 진단과 기술 부모 조회 뒤에 섹션을 선택한다. 기존 binding이 `loader.Load(selector())`로 먼저 평가하는 지점은 실제 Session 연결 시 함께 정정해야 한다. 이번 선택 API 검사는 전체 wrapper의 callback 순서를 새로 입증한 것이 아니다.

alternate populate는 `0046e630→0046e000/0046df80→0040e130/0040e2b0`의 미복원 하위가 남아 있다. 일반 지형 생성으로 대신하지 않는다. 전체 Early Missions·캠페인·데모3개/사용자 완주·GUI/음향·다른 메뉴·배포 목표는 계속 활성이다. 원본/CD·AGENTS.md·다른 프로젝트 수정, 원본 게임 실행, 원격 push는 없다.
