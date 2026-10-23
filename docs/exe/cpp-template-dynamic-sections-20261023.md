# Template 동적 노드와 Temp 이름 교체 — 단위377 저장 기반

현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 이 조각은 `CampaignTemplateStorage.h/.cpp`와 신규 `CampaignTemplateDynamicTests.cpp`를 담당한다. 기존 보호 원본/CD·AGENTS·공용 CMake·Session/UberGump·LEFT_JOBS·git·게임/GUI를 변경하거나 실행하지 않았다. 상위 작업자가 Session의 전체 Objects 저장 부모를 연결하고 통합 빌드/검사를 담당한다.

## 동적 노드와 실제 버퍼

기존 Storage는 섹션 handle을 표준35개 이름의 위치+1로 취급하고, streamGeneration/accessing도35칸 배열로 저장했다. 따라서 원본 전체 Objects 저장의 **새 Temp 노드 생성→현재 TerrNN 반납→Temp 노드의 이름만 TerrNN으로 교체**를 실제 수명으로 표현할 수 없었다. EnsureStateSize도 원문 임시 노드를 고정 State handle로 다시 바꾸고 head/current/next를 편의 재기록했다.

현재는 각 실제60BYTE Stream에 고유 handle·비영 generation·현재 buffer binding을 둔다. 최초 파싱의 표준 위치와 런타임 노드 handle은 독립적이다. 표준 이름의 버퍼는 기존 `fort.sections[index]`를 그대로 소유하고, Temp 같은 비표준 이름은 같은 Stream의 extra 벡터가 소유한다. 최초 파싱 위치는 한 번만 목록에 연결하고, 후속 SyncStreams는 실제 head/current/next를 파일 순서로 다시 만들지 않는다.

원본 `RenameDataManagerStream`을 반드시 실행한다. 이 본체가 destination 검색→있으면 실제 목록 제거/버퍼/노드 반납→source 재검색→같은 source 노드 이름 쓰기를 수행한다. Storage는 그 **결과 source handle을 바꾸지 않고**, 완성된 벡터를 새 표준 `fort.sections` 위치로 move하여 binding만 바꾼다. std::vector의 동일 allocator move이므로 바이트 복사나 새 게임 버퍼 할당 없이 실제 data 주소/capacity를 보존한다. raw data 토큰·pos·end·capacity, source generation, 실제 목록/head/current/next도 그대로다. destination의 옛 참조만 무효화한다.

현재 handle은 재사용하지 않고 단조 증가한다. generation도 노드 생성 때 부여하며 이름 교체로 증가시키지 않는다. 같은 이름 재생성/manager 반납 뒤 옛 TemplateWriteStream은 거부하고, current 변경/이름 교체/다른 헤더의 빌림은 유효성을 바꾸지 않는다. typed State/Territory/Money/Badges/Technology cursor도 고정 handle 번호 대신 **현재 이름과 실제 소유 node handle**로 연결한다. 표준 이름을 Scratch로 바꾼 뒤 옛 State cursor가 새 이름의 pos를 덮거나 State를 되살리지 않는다.

## 실제 호출 API

| API | 현재 원문 효과 |
| --- | --- |
| `CreateSection(slot,name)` | 실제 CreateDataManagerStream으로 같은 이름의 기존 노드를 제거하고 새60BYTE/16384BYTE 노드를 목록 마지막에 만든다. Temp 등 동적 이름도 허용한다. |
| `FindSection(slot,name)` | 실제 FindDataManagerSection으로 current를 갱신한다. 누락은 current0/빈 참조이며 pos는 초기화하지 않는다. |
| `SelectSection(slot,name,writable)` | 실제 SelectDataManagerSection의 검색/원본 진단/pos0을 수행하고 같은 노드 참조를 반환한다. |
| `RenameSection(slot,destination,source)` | 실제 RenameDataManagerStream을 수행하고 같은 source node handle/generation을 반환한다. |
| `AccessSection(ref,operation)` / `WriteSection(ref,bytes)` | current와 독립적으로 보관한 실제 노드/원래 버퍼를 사용한다. Rename 후 기존 source 참조도 계속 쓸 수 있다. |
| `Template(slot).sections` / `SaveFile` | Rename이 이동한 같은 완성 allocation을 읽는다. SaveFile은 실제 manager 이름 검색으로 표준35개 섹션을 직렬화하며 current는 마지막 Terr19가 된다. |

EnsureStateSize는 기존 원본 `EnsureDataManagerStreamSize`가 만든 `T$e$m$p` 노드도 같은 동적 저장소에 두고, 원문 Rename 결과 handle을 그대로 State에 연결한다. 고정 State handle로 바꾸거나 manager.current/head/next를 덮어쓰는 후처리를 제거했다. 기존 별도 GrowState/CommitStateGrowth 호환 경계와 읽기 부모의 실제 파일/서명 소유권은 유지했다.

## 검증과 한계

신규 세 검사는 다음을 확인한다.

- Temp→Terr00의 node/generation/data 토큰/실제 주소/pos/end/capacity 유지, destination 옛 참조 거부, 실제 목록35개/Temp 소멸, 대소문자 선택/누락 current0, 빌린 헤더/manager 반납.
- 20개 Terr00..19에 Temp 생성/이름 교체를 반복하고 같은 allocation을 Template.sections와 실제 SaveFile의35개 WORD 길이/payload에서 대조한다. 파일 장치는 메모리 sink이며 원본/작업 파일을 쓰지 않는다.
- Temp→State→Scratch→State에서 동일 노드 수명과 typed cursor의 현재 이름/위치/버퍼를 확인하고, State 재생성 뒤 옛 참조가 거부되는지 확인한다.

독립 경로 `extracted/cpp-storage-dynamic-20261023-isolated`에서 새 Storage 객체를 기존 정적 라이브러리보다 먼저 링크했다. 최종 Release 컴파일은 관측 경고/오류0, 새3개와 기존 TemplateLifetime·DataManager Lifetime/Selection/Streams/Resize/Create/Read를 합쳐 **36검사/실패check0·종료0**이다. 기존 원문 fixture2550입력과 실제11파일 읽기, 짧은 State 최소 확장, State 반복 생성/tail 압축, 늦은 표준 버퍼 추가/외부 축소, 경제/메타 cursor 및 빌림 소유권을 포함한다. 실제 기존 게임/클론 GUI는 실행하지 않았다.

최종 원시는 같은 경로의 `build-final.log`·`tests-final.log`다. 공유 제품 빌드/전체 CTest와 Session 전체 Objects snapshot은 상위 단위377 결과로 구별한다. 기존 검사에서 `TemplateWriteStream.section-1`을 Fort 파일 위치로 사용한 기대와 unknown 동적 이름 거부 기대는 새 계약과 달라 상위 작업자가 이름 기반 조회로 갱신한다. 그 옛 기대에 맞추어 동적 Temp를 고정 Terr handle로 가장하지 않는다.

이 조각은 저장소의 실제 노드/할당/이름 교체 기반이다. 전체 Editor 설치/작업본 사용자 저장·불러오기·모든 Fort snapshot 부모·메인 Edit 활성화·게임 전체 동일성 완료를 뜻하지 않는다. 런타임 API의 구조 변경은 raw 쓰기 빌림 중에 거부하며, 원본40BYTE 이름/NUL·ASCII 이름 비교·DWORD 길이의 기존 물리 계약을 유지한다.
