# 단위324: 실제 Fort 파일과 단독/편집 맵 목록 연결

작업 PC와 마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-22**로 같다. 시작 HEAD는 `448db39f`다. 기존 정밀 소스와 앞 단위의 독립 관측을 읽었으며, 새 전체 디컴파일이나 원본/클론 게임 실행은 하지 않았다.

## 실제 연결

`SoloMapCatalog`는 `RawSoloMapList`에 현재 설정, 파일 열거와 파일 내용을 연결한다. 분류용 임시 Template에서 파일 서명/전체 섹션 읽기 → 실제 `RawPlayerTemplateState::Get` → State+17을 수행한다. 수락된 이름은 별도 임시 Template에 다시 읽어 Subscriber, State 첫 BYTE, 현재 사용자와 예약 getter를 조회한다. 파일마다 분류와 목록 조회의 State 수명이 구별된다. 지형/유닛/기술 전체를 미리 파싱하지 않는다.

빈 State는 기존 실제 생성/28BYTE 쓰기 부모를, 짧은 State는 기존 실제 최소 확장/이름 교체 부모를 사용한다. 초기화되지 않는 바이트는 실제 Client에서는 기존 현재 힙 입력을 공급받는다. 검사에서는 0xa7 입력을 명시했다. 원본 x86의 미정 메모리와 현재 호스트 힙 내용이 일치한다는 뜻은 아니다.

`CampaignTemplateStorage::ReadSubscriberName`은 파싱 시점의 `fort.subscriber` 사본을 쓰지 않는다. 실제 현재 섹션을 선택하고 ID 네 BYTE를 건너뛴 뒤 signed WORD 길이와 원문을 읽으며 같은 노드의 cursor를 진행한다. 첫 NUL은 원본 문자열 소비자와 같이 처리한다. 음수 길이, 물리 원문 밖 접근, 목록의 256BYTE 출력 밖 쓰기는 호스트 예외로 거부한다.

`MakeSoloMapCatalogClientEffects`는 같은 Client의 실제 타입 개수, 현재 힙 입력, 번역을 제공한다. 현재 사용자 이름의 소유자는 별도로 현재 좁은 문자열을 공급해야 한다. 아직 UberGump가 이 목록을 소유하거나 호출하지 않으므로 Edit 메뉴가 사용 가능해졌다는 주장은 하지 않는다.

## 열거 경계에서 확인한 차이

초기 연결에서는 기존 `BaseFileSystem::Match`를 사용했다. 이후 0045a220 → 00459610/004597c0을 확인하여 **Edit 목록은 Win32 FindFirstFileA/FindNextFileA의 기본 디스크 열거**임을 확인했다. TAFF/보조 디렉터리를 합쳐 정렬하는 공용 Match와 다르다.

새 `MatchDisk`는 실제 Win32 반환 순서와 파일 이름 대소문자를 유지하며 디렉터리 속성도 별도로 제외하지 않는다. 와일드카드 없는 FileSpec은 원본처럼 존재 확인 없이 이름 하나를 반환한다. 열거는 기본 디스크만 사용한다. 이름을 선택한 뒤의 내용 읽기는 기존 디스크 → TAFF → 보조 경로 우선순위를 유지한다. cpppj가 대상으로 하는 Windows의 경계이며 다른 운영체제의 같은 동작을 가장하지 않는다.

00459c40(1)→004595a0은 마지막 확장자뿐 아니라 같은 파일 이름의 모든 점 뒤를 지운다. `map.extra.fort`의 목록 이름은 `map`이다. 이 부분도 일반 `filesystem::stem`과 다르므로 별도 검사와 같은 처리를 사용한다.

처음 관측한 합쳐진 파일 수는 465개다. 이 중 기본 디스크는 433개이며 State 종류는 0이 1개, 2가 432개다. 디스크 밖 항목을 Edit 목록에 섞지 않도록 고쳤다. 원본의 최대 50개 캐시는 그대로이므로 많은 파일이 있는 폴더의 모든 맵을 한 번에 표시하는 기능을 새로 추가하지 않았다.

## 검사와 보존 범위

검사는 원시 파일의 길이 레코드에서 독립적으로 읽은 State/Subscriber를 실제 목록 결과와 대조한다. 기본 디스크의 전 파일, 모드 0/1/2와 편집 허용 설정, 최대 50개, 현재/fallback 선택, 손상/작성자 flags, 기존 캐시 재사용과 dirty 갱신을 확인한다. TEST01은 명시 FileSpec으로도 별도 조회한다. 별도 빈 기본 폴더에서는 TAFF/보조 파일이 목록에 나타나지 않으면서 내용 읽기로는 접근되는지 확인한다.

생성 파일 검사에서는 빈/짧은 State, 전체 지형 해석이 필요 없는 파일, 종류1의 편집 허용, 현재 원문 Subscriber 변경, NUL과 signed 길이 경계, 분류와 목록 읽기 사이 파일 소실을 확인한다. 소실된 파일은 원본 flags1과 붉은 행을 남긴다. 생성/제거는 빌드 아래 검사 전용 파일에만 수행한다. 보호 Fort 원문은 다시 읽어 같음을 확인한다.

최종 빌드/집중 검사/전체 회귀 결과와 SHA는 [실행 근거](../../cpppj/solo-map-catalog-20261022.json)에 기록한다. 첫 빌드의 검사 헤더 누락과 다음 빌드의 명시 BYTE 변환 경고는 수정했다. 열거 경계를 고치기 전 결과를 최종 제품 검증으로 사용하지 않는다.

최종 Release 빌드5는 **종료0·경고0·오류0**다. 새 검사 **6개·실패check0**, 전체 **2,255개·실패check0·exit0**이며 실제 검사182.97초/CTest전체182.99초였다. 큰 의존 빌드 중 추가한 전체 점 제거 처리는 마지막 증분 빌드5에서 다시 컴파일했다. 디스크433개와 별도 빈 기본 폴더에서의 내용 읽기465개, TEST01 종류2/flags0을 확인했다. 원시 실행 기록은 `extracted/cpp-solo-map-catalog-20261022-{build*,tests*,full*,cases}.log`에 보존한다.

## 다음 실제 연결

020000c9 진입 → `lastEditFort`를 pending 이름에 복사 → 목록 mode2 갱신 → `LoadEditable`의 `*SOLOLIST*` 행, 020000ca 선택을 실제 UberGump 입력에 연결해야 한다. 현재 `Edit → Tell,NotImplemented`는 그대로다. 생성/저장/불러오기/Test Battle/Return to Editing도 남았다.

00460bb0은 현재 fort가 아니라 pending 이름 0055b494에 쓴다. 선택 뒤 004b3560은 상태 전환 요청 005c8878=1을 남긴다. 실제 처리기는 **004b6dd0**의 상태1/2/3/6 분기다. 편집 진입은 00594fb8=1과 함께 005c85a4(편집),00594fa4(미션 후처리 억제)를 설정한다. 00594fb8만 바꾸는 것으로 편집 전환을 대신할 수 없다.

이 분기는 기존 Template의 State dirty 비트를 켜고 Notify/저장을 요청한다. 보호 원본을 직접 수정해서는 안 되므로 실제 편집 작업본/저장 경계를 먼저 마련해야 한다. 004b5ca0(파일 준비/State/Subscriber),004b5ec0(Territory 전투 비트 복구),004b5f70(기존 전체 월드 로드),004904b0(Human/Computer 표시·편집 기술)을 기존 몸체와 연결할 필요가 있다. 004904b0은 현재 이름을 번역된 `Player`로 바꾸며 컴퓨터 이름/기술도 초기화한다.

현재 `RawCampaignWorldRuntime::Impl`은 `RawFortWorldLoad`를 소유하지만 generic Fort의 money/badges/technology/populate 콜백을 명시 미연결 예외로 두고 있다. 단순히 같은 로더의 Load를 호출하면 편집 월드가 완성되지 않는다. 기존 `MakeFortWorldLoadHooks`와 Meta/Money 소비자를 같은 Core에 조립해야 한다. 현재 `RawCampaignSession::Impl::handle`도 세션별 64BYTE이므로 원본의 장기 현재 이름 00540c78/00540ca8과 공유할 소유 수명을 정해야 한다.

본 단위는 파일 목록 기반 연결이다. 마무리 중 사용자가 `/goal`의 캠페인 범위를 전체로 확대했다. 현재 목표는 Early Missions 전체·캠페인 전체·데모3개와 Edit/custom 시나리오의 동일한 동작이다. 기존15개 미션 초기 진행은 부분 검증 범위다. 전체 미션 완주, GUI/음향 동일성, 편집 월드와 사용자 시나리오의 전체 수명은 계속 미완료다.
