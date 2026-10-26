# 빈 DataManager 초기화와 공유 이름표 선행 관측

이 준비본은 `cpp-new-fort-469-staging` 동결 후 별도 수행한 **읽기 전용 복원 선행 연구**다. 제품/C++ 검사/빌드/GUI/보호 원본/정밀 C를 수정하지 않았다. 실제 C++ 모듈과 Storage/Client 바인딩은 아직 없다. 신규 NewFort 기능 완료로 합산하지 않는다.

현재 HJOW-Athlon과 마지막 전체 디컴파일 호스트 HJOW-Athlon/2026-10-23이 일치한다. 보호 PE는 SHA256 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`이다. 원본·클론 게임 프로세스 실행과 실제 OS 장치 호출은 모두0회다.

## 실제 부모와 새로 확인한 공유 수명

`Netstorm.c:62573`의 `0044d870`은 고정 manager의 root(+4)가 비영이면 Datamanager.cpp:0xc9 진단→그 뒤 현재 가상+4의 이름표 getter→이름0 조회→첫BYTE가0일 때까지 같은 manager의 Create(`0044d1b0`)와 다음 이름 조회 순서다. 이름표 getter는 **한 번만 호출**하며 반복은 그 반환 포인터를 고정한다. 따라서 Create 중 이름표 전역005c8aa4가 다른 표로 바뀌어도 현재 반복은 원래 표를 계속 사용한다. 두 번째 initialize는 그때의 현재 전역표를 새로 받는다.

가상+4의 실제 `004bccb0`은 상수가 아니다(`Netstorm.c:150433` 이후).

1. 전역005c8aa4가0일 때만 `operator_new(512)`를 호출하고 **반환 포인터를 전역에 먼저 쓴다**.
2. `004437f0`을 호출한다. 이 함수는 정밀 C와 실제 보호 기계어에서 빈 RET이므로 추가 효과가 없다. 관측에서는 대체하지 않고 원문을 실행했다.
3. 기본 이름 문자열 **136BYTE(135문자와 NUL)**를 복사한다. 기본15이름은 Subscriber, State, Mission, CoreData, Chaff, Badges, Territory, TypeNames, CompressedData, Technology, Money, Deck, Reserved2, Reserved3, Reserved4다.
4. 각 index0..19에 대해 `004a89c0(local256,"Terr%02d",index)`를 호출하고, 그 문자열을 전역표의 현재 NUL 뒤에 붙이고 `"/\0"`를 붙인다. 마지막에도 `"/\0"`를 한 번 더 붙인다. 실제 미변환 입력은 `/Subscriber/.../Reserved4/Terr00/.../Terr19//\0`이다.
5. 전역005c8aa4를 반환한다. 정상 신규 표의 실제 쓰기 폭은277BYTE이며512BYTE의 나머지는 건드리지 않는다. 이미 비영이면 할당/서식/복사를 생략하고 같은 현재 포인터만 반환한다.
6. 할당0은0을 공개하고 빈 base RET을 지난 뒤 **실제004bcced의 복사 접근에서 중단**된다. 정상 empty 표를 반환한 것이 아니다.

원문005c8aa4의 정밀 C 전체 참조를 재검색하면 이 getter 내부 참조들뿐이며 Session마다 reset/free하는 증거는 없다. Client 수명의 lazy owner로 만들고 Session 전환에서 새 표를 만들지 않는 것이 현재 근거에 맞는다. 호스트 종료의 RAII 반납과 원문 Session 정리는 구별해야 한다.

SNART 이름 선택은 실제 `004a8850`과 `004a8800`이다(`Netstorm.c:133319` 이후).

- 최초 BYTE가 비영이면 원래 입력의 마지막 NUL을 FF로 바꾸고 첫 FF 전의 slash만 NUL로 바꾼다. 잘못된 선행 separator는 Snart.cpp:0x46 진단 뒤 계속한다.
- 변환된 같은 표의 첫BYTE는0이므로 다음 조회·다음 initialize는 재변환하지 않는다.
- index0은 표+1, 이후 지정한 수의 NUL을 지난 위치를 돌려준다.35번째 인덱스는 마지막 빈 이름이며 initialize가 이 첫BYTE0에서 멈춘다.
- 표를 vector<string>로 나누거나 이름들을 앞에서 모두 복사하면 Create 중 **원래 표 내용 변경**을 다음 조회에서 보는 성질이 사라진다.

## Native 추가 관측

`tools/decomp_initialize.py`는 실제 `0044d870`, `004bccb0`, `004a8850`, `004a8800`, `004437f0`을 실행한다. 큰 stream Create, operator new, `Terr%02d` 서식만 명시한 외부 장치 입력이다. 따라서 실제 stream 할당/제거/목록/버퍼 초기화는 관측했다고 주장하지 않는다. 이 부분은 기존 RawDataManagerResize 검증과 다음 실제 Storage 결합 검사가 맡아야 한다.

| 파일 | 입력과 출력 |
| --- | --- |
| `fixtures/data-manager-initialize-x86.tsv` | 64행. 실제 lazy표/변환 전 One·Two표/변환 후 동일표/빈표 × root0·비영 ×1·2회 호출 ×외부 효과4종이다. manager12BYTE, 두 표512BYTE, 현재 전역 포인터와 전체 호출 순서를 보관한다. |
| `fixtures/template-section-names-x86.tsv` | 12행. 실제 lazy표/준비된 표/변환된 표 ×할당0·비영 ×1·2회 getter 호출이다. 같은 전체 BYTE와 실제 할당0 fault PC를 보관한다. |
| `initialize-native.json` | 원본·도구 SHA, 실제 실행한 보호 코드 범위 BYTE, 행 수와 외부 장치의 한계다. |

외부 효과4종은0=변경 없음,1=root 진단에서 전역을 Alt·Last표로 교체,2=첫 Create 뒤 전역을 Alt·Last표로 교체,3=첫 Create 뒤 **원래 캡처 표**의 State→Other 또는 Two→Six 변경이다. 효과는 장치가 전달하는 명시 입력이며 제품 동작을 대신한 결과 주입이 아니다.

실제 결과에서 canonical 신규 표는 `FortTemplate::SectionNames()`와 같은35개를 같은 순서로 Create했다. 첫 Create에서 전역을 바꾼 행도 그 첫 반복은 기존35개를 모두 생성했다. One·Two표의 두 번째 이름을 변경한 행은 One→Six를 생성했다. root 진단에서 전역을 바꾼 행은 그 뒤 getter가 변경된 전역을 사용했다. 동일 getter를 두 번 호출해도512 할당과20회 서식은 처음 한 번만 있었다.

최초 생성과 `--verify` 재실행의 전체 BYTE가 일치했다.64행 fixture SHA256은 `402557fde72688da3f6f2b694767e08a711b428ab5b916aeff6aa4a818c1298d`,12행 fixture는 `af4e140f46ddaaefc7b7e6335b234065bef5d495b329c262ff025d871f51117a`다.

최초 도구 준비에서 Create의 thiscall `ret4`를 일반 cdecl 반환으로 처리한 오류를 고쳐 fixture 생성 전에 수정했다. 또한35회 원문 문자열 재순회는 Python 명령 hook과 동시 빌드 부하에서 기존 Oracle의0.5초 제한을 넘겼다. 허용 코드/입력/명령수 제한은 유지하고 이 별도 도구의 가상CPU 시간 제한만5초로 늘렸다. 제한 초과나 스택 불일치를 정상 결과로 기록하지 않는다.

재검증 명령은 다음과 같다. 생성 도구는 기존 fixture를 덮어쓰지 않으며 `--verify`는 다시 실행한 전체 BYTE를 대조한다.

```text
python extracted/cpp-data-manager-initialize-469-staging/tools/decomp_initialize.py --verify
```

## 다음 최소 C++ 단위와 소유자 계약

1. `RawTemplateSectionNames` 또는 같은 역할의 작은 모듈로004bccb0의 lazy 할당/초기277BYTE 쓰기를 복원한다. 같은 Client 전역 token/실제512BYTE owner와 현재 span을 빌리는 API가 필요하다. 이미 만들어진 표를 매번 초기화하지 않는다. 할당0은 실제 실패로 남긴다.
2. SNART 변환/이름 선택004a8800/004a8850을 공통 raw helper로 공개한다. 현재 `RawDataManagerImage.cpp`의 private `Name`에 같은 복원이 있지만 외부에서 재사용할 수 없다. 초기화 모듈과 파일 이미지 reader가 **같은 helper와 같은 Client 이름표**를 빌리도록 연결한다. 기존 reader의 동작을 임의 변경하지 않고 별도 Native 관측과 회귀를 함께 둔다.
3. `InitializeDataManagerSections(fixedManager, hooks)`는 root 현재값 진단 뒤 `names(fixedManager)`를 한 번 부르고, 그 반환 물리표를 보관하여 매 index에서 원문 이름 선택을 다시 실행해야 한다. Create에는 현재 header를 다시 읽어 고른 manager가 아니라 **진입 시 전달된 같은 manager**와 같은 이름 포인터를 넘긴다. 이름 길이를 모두 미리 계산하거나 전체 목록을 복사하지 않는다.
4. 실제 `CampaignTemplateStorage::Image::CreateSection`은 RawDataManagerResize의 기존 실제 생성 부모를 사용하므로 이를 manager token으로 직접 선택하는 좁은 바인딩이 필요하다. state_recovery가 준비 중인 `CreateManagerSection(manager,name)`는 현재 header를 다시 읽는 `CreateSection(slot,name)`과 구별하여 사용할 수 있다.
5. 새 빈 Image는 원문 ctor 뒤 header 공개→이 초기화 부모 순서를 유지한다. `Image::SyncStreams()`가 초기 목록을 임의 미리 만들지 않도록 `fileImage=true`의 빈 목록 경계를 재사용하되, 각 canonical 생성이 실제 manager current/head/노드/버퍼/metadata에 연결되는지 검사한다. 초기 빈 State 다음28BYTE 확장도 같은 owner에 수행한다.
6. 다음 C++ 검사는 위76행 Native replay와 실제 Storage의35개 순서/current, 모든 섹션 실제 비영 노드·pos0/end0·기본 버퍼 할당, 같은 Client의 파일 reader→신규 manager→다시 reader 사이의 표 포인터/변환 상태 보존을 확인해야 한다. 앞 단계에서 callback이 실패하면 이미 생성한 목록과 이름표 변환을 원문처럼 남기고 완료로 표시하지 않는다.

NewFort 부모1400행 준비본은 별도 manifest로 이미 동결했다. 이 추가 연구를 그 부모의 C++ 통과·완료 범위에 소급 합산하지 않는다.
