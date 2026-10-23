# 실제 Storage의 원문 LoadFile 연결 제안

2026-10-24, 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. AGENTS/공통·닷넷 인계를 다시 읽었다. 기존 원문/API/검사를 읽고 **extracted 사본과 독립 콘솔 빌드만** 작성·실행했다. 공용 제품/검사/CMake/LEFT/git·공유 build/library·게임/GUI·보호 원본/CD는 변경·실행하지 않았다.

## 기존 증거와 재사용

[기존 원문 Load 분석](cpp-template-load-20261021.md), `tools/cpp_template_load_audit.py`, `cpppj/template-load-static-20261021.json`, `TemplateLoadTests.cpp`의5검사를 먼저 확인했다. 이 자료는 **PE 정적 분석과 합성 hook 검사**이며 독립 x86 fixture가 아니다. 같은 부모의 새 native 관측을 중복 작성하지 않았고, 기존 `o::LoadTemplateFile`/`ReadDataManagerFile`/`ReadTemplateFileSignature`/`ReadDataManagerImage`를 그대로 호출한다. 이번 신규 native 관측0이며 아래17검사는 실제 C++ 소유 어댑터의 격리 검증이다.

제안 [patch](../../extracted/cpp-template-storage-load-20261024-proposed.patch)는 `CampaignTemplateStorage.h/.cpp`의 사본 변경과 신규 `CampaignTemplateLoadFileTests.cpp`만 포함한다. 공용 CMake 등록은 포함하지 않는다. 생성 도구는 `extracted/propose_template_storage_load_20261024.py`, 입력/사본/검증은 `extracted/cpp-template-storage-load-20261024-proposed/`에 있다.

## 공개 계약과 실제 소유권

`LoadFile(slot, FortTemplate fort, string_view name, const TemplateFileLoadHooks&)`는 원문004bf550의 별도 API다. 기존 `Install/InstallFile`의 **이전 슬롯 반납→교체 및 실패 throw** 계약은 유지한다. 신규 API는 기존 manager가 있으면 실제 +12 이름을 CRT `sameName` 경계로 비교하고, 같은 이름은 자연 반환, 다른 이름은 `wrongName` 진단 후 자연 반환한다. 기존 cursor/head/current/노드 handle/버퍼와 헤더20BYTE를 임의 초기화하지 않는다.

신규 생성은 +4=0→276BYTE 토큰 확보→실제 `Image` 생성/같은005c89c0=0→owner map 보유→기존 부모의 header.ptr 공개 순서다. 그 다음 현재 +8 비영이면 readonly 효과를 건너뛰고1로 정규화하며, 영이면 실제 파일명으로 `readOnly`를 호출한다. +12는 보존하고 +16만0으로 쓴 뒤 **현재 header.ptr**를 `ReadFile` 인자로 다시 읽는다. `read`는 실제 `ReadDataManagerFile`의 파일 장치가 선택된 자리에서만 실행한다.

`TemplateFileLoadHooks`는 `read`/`sameName`/`wrongName`/`readOnly`/`typeCount`/`tooManyTypes`/`rejectFile`/`currentSlot`/`refreshTitle`다. 장치 유무를 처음에 일괄 거부하지 않는다. 기존 이름 경로는 read/readonly/typeCount/title이 없어도 반환하고, 정상 ID나 다른 현재 슬롯처럼 선택하지 않은 효과는 요구하지 않는다. typeCount는 서명 검사 순간 호출한다. 이름 비교에 새 ASCII/로캘 정책을 몰래 추가하지 않으며 실제 Client CRT 경계를 공급해야 한다.

읽기 성공 뒤 현재 header.ptr가 비영인 경우에만 현재 슬롯을 재조회하고 해당 슬롯이면 **파일 부모의 중간 제목**을 갱신한다. 이후 Subscriber/config 뒤 외곽의 두 번째 Title은 별개다. `read`가 빈 할당을 반환하거나 `rejectFile`이 정상 복귀하여 raw reader가 false를 반환하면 **그 시점 현재 header.ptr**를 저장해 실제 base Destroy→동일 저장 handle free→ptr0만 수행한다. +4/+8/+12/+16과 callback 변경은 보존한다. 파일 장치/진단/본문 throw는 정상 false로 바꾸지 않고 공개된 manager/서명/이미 붙인 섹션을 유지한다.

`fort`는 호출자가 같은 파일에서 이미 얻은 보조 metadata다. 이번 어댑터는 새 parser를 추가하지 않는다. raw 섹션은 기존 원문 reader가 실제로 할당하며 `fort.sections`를 파싱 사본으로 복구하지 않는다. 실제 파일 입력과 보조 metadata의 동일성은 호출자 계약이다. 새 Editor ctor 지연/전체 파일 해석 경계는 [Session 설치 후속](cpp-existing-editor-session-install-next-20261024.md)에서 구별한다.

## 다른 현재 manager와 섹션 병합

readonly callback이 header.ptr를 다른 살아 있는 실제 manager로 바꾸면 그 manager가 reader의 대상이다. 기존 `ReadDataManagerImage`는 비어 있지 않은 manager에 대한 병합도 구현하므로 재사용한다. 새 노드는 handle별 extra 벡터에 실제 할당하고, 같은 이름의 기존 노드는 **원문이 제거를 선택할 때까지** 그대로 둔다. 새 빈 섹션이면 임시 노드만 반납하고 기존 노드/할당/pos를 보존한다. 새 비빈 섹션이면 기존 제거→새 append 후 같은 할당을 canonical 위치로 move한다.

정상 반환/host 예외에서 실제 head/next에 이미 붙은 노드만 같은 순서로 공개한다. current/pos/세대는 덮어쓰지 않는다. 미완료 임시 노드를 임의 append하거나 중간 상태를 전체 완료로 바꾸지 않는다. 실행 중인 manager 자체를 callback이 실제 반납한 뒤 접근하는 잘못된 수명은 지원 계약이 아니다. header 포인터 변경과 현재 객체 파괴는 구별한다. header에서 분리된 원래 할당은 Storage owner map이 호스트 종료 또는 명시 반납까지 유지하며 새 슬롯에 자동 재귀속시키지 않는다.

## 검증 결과와 제한

격리 최종 빌드5 **종료0/경고0/오류0**, 신규9+기존 TemplateLoad5+동적 Storage3 = **17검사/실패0**이다. 생성 공개 전후 헤더/공유 version, 실제 이름과 current/title 순서, readonly 선행 비영, 정상 false 정리, 서명 거부/예외 구별, 부분 Subscriber 노드, callback의 다른 manager 병합, 같은 State 할당/세대/pos 보존, 이전 Subscriber 참조 거부, 실제 SaveFile의 같은 파일 BYTE 왕복을 확인했다.

공유 라이브러리를 사용하지 않고 Storage 사본/기존 raw 하위와 기존 Template.cpp를 직접 컴파일했다. Template.cpp의 비선택 파서 종속성을 해결하는 데 필요한 **기존 RiftTypeTable의 네 메서드 몸체를 수정 없이 추출**해 격리 링크에만 포함했다. `isolated/prepare_sources.py`와 `name-hash-source.json`이 출처/정확한 몸체 SHA를 기록한다. 새 함수 대체/계산 기대/native oracle을 만든 것이 아니다.

최초 빌드1·2·3·4의 링크 종속 실패 로그와 최종 build5/run1 로그는 보존한다. 이 결과는 실제 파일 출처/CRT stat0041af80·파일 삭제 장치·전체 Editor UI/State/loaded·사용자 저장/재로드 성공의 검증이 아니다. 실제 Client의 source2/4 readonly 및 디스크 stat의 정확한 공급은 별도 필수 장치이며 Editor bool이나 단순 존재 검사로 채우지 않는다.
