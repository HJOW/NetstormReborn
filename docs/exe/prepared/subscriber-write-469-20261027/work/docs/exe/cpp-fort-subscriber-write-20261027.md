# 469 준비: 실제 Fort Subscriber 저장 부모

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**으로 같다. 작업일은2026-10-27이다. 468 제품은 동결/통합 검증 중이므로 이번 작업은 `extracted/cpp-fort-subscriber-write-469-staging`에만 준비했다. 제품 src/tests/CMake/기존 fixture/원본/디컴파일 결과는 변경하지 않았다. 게임·창·OS 장치 실행과 C++ 빌드는 하지 않았다.

## 원문과 선택/캡처 순서

10.78 `004bd400(this,id,name)`은 다음 순서를 실행한다.

1. 고정 Template this의 첫 DWORD manager가0이면 Template.cpp463(`0x1cf`) 진단을 호출한다. 복귀하면 계속한다.
2. `004b38d0` 등록 helper를 호출한다. 기존 `FinishFortSubscriberRegistration`이 실제 같은 현재00540c74와 ReadInt/새 ID/설정식을 처리한다.
3. 그 뒤 고정 this의 **현재 manager를 다시 읽어** `0044d1b0(manager,"Subscriber")`를 호출한다. manager 선택을 진단 전이나 helper 전으로 앞당기지 않는다.
4. Create가 돌려준 stream에 부모 인자로 들어온 **캡처 ID 4BYTE**를 쓴다. helper가 바꾼 현재 global ID를 대신 저장하지 않는다.
5. **캡처된 같은 name 포인터**를 `004bcbf0` 문자열 writer에 넘긴다. helper가 전역 포인터00540ca8만 교체해도 이미 캡처한 포인터는 바뀌지 않는다. helper 또는 ID쓰기가 같은 버퍼의 내용/길이를 바꾸면 ID쓰기 뒤에 처음 수행하는 NUL 검색에서 현재 내용이 보인다.

`004bcbf0`은 null이면 Template.cpp155(`0x9b`) 진단 뒤 빈 이름으로 계속하고, 첫 NUL까지 길이의 하위WORD를 쓴 뒤 같은 주소의 최초 전체 길이를 NUL 없이 쓴다. 길이쓰기 callback이 이름 내용을 바꾸더라도 길이를 다시 계산하지 않는다. 기존 `SaveFortMissionName`이 이 몸체를 이미 복원했으므로 새 문자열 계산기를 만들지 않았다.

NewFort의 바깥 부모는 handle 포인터→현재 owner→현재 MySubscriber를 호출 직전에 캡처한다. 이 바깥 캡처는 별도 `cpp-new-fort-469-staging`이 담당한다. 이번 child API는 캡처 ID 값과 NUL 길이를 미리 자르지 않은 물리 span을 받는다. null data 포인터는 호출자가 nullopt로 연결한다.

## 준비한 구현과 실제 manager 장치

`RawFortSubscriberWrite`의 `SaveFortSubscriber`는 위 부모만 소유한다. 현재 manager getter, 줄 진단, 실제 등록 helper, Create, 일반 write를 선택 장치로 받고 기존 등록/문자열/MemStream을 재사용한다. 가짜 Fort 이미지나 임시 State를 대신 만들어 성공 처리하지 않는다.

`CampaignTemplateStorage::CreateManagerSection(manager,name)`은 **이미 선택한 manager 토큰**으로 실제 소유 Image를 찾아 기존 `Image::CreateSection`을 호출하고 실제 generation을 포함한 `TemplateWriteStream`을 반환한다. 기존 `CreateSection(slot,name)` 본문은 바꾸지 않았다. 슬롯을 다시 읽는 기존 API를 Create 경계에서 사용하면 callback이 바꾼 header 때문에 다른 manager가 선택될 수 있으므로 좁은 manager API를 추가했다.

동기 child 호출 동안 Create가 반환한 typed stream 하나를 지역 장치가 유지하면 뒤 ID/이름 쓰기 중 header가 다른 manager로 바뀌어도 `WriteSection(typedStream,bytes)`가 동일 노드/생성 수명에 쓴다. 다른 슬롯/새 fake manager를 만들지 않는다. 잘못된 manager나 반납된 generation은 기존 물리 장치의 예외로 중단한다.

## 독립 x86 관측

새 도구 `decomp_fort_subscriber_write.py`는 보호 PE의 **004bd400/004b38d0/004bcbf0/0044cce0** 네 몸체를 실제 Unicorn CPU로 실행했다. DataManager Create, ReadInt, 새 ID 생성, 설정, 진단, sprintf는 명시 외부 장치다. 실제 게임/OS/file 저장을 실행한 관측이 아니다.

입력45개를 두 x87 control에서 실행한 **90관측·575,800명령**이다. 정상72관측, null manager의 명시 Create 거부2관측, assert/register/create/write1/write2/write3/capacity/null 중단 각2관측을 구분한다. `fault:manager0`은 실제 Create 몸체의 null 역참조를 CPU로 실행한 결과가 아니라 그 외부 경계에서 null을 거부한 상태다.

입력에는 ID0/상위32비트/-1, 등록 ReadInt/생성/false지만 현재ID 변경, null/빈이름/NUL뒤 내용/255·256·65536길이, 두 stream/현재 cursor/용량 진단, 진단·helper 뒤 manager 재조회, global 포인터 교체와 같은 버퍼 변경, ID쓰기 중 이름 변경, 길이쓰기 뒤 이름 변경, 의미 있는 중간 중단을 포함했다. 제품 결과는 fixture 생성 입력으로 사용하지 않았다.

fixture는 두 stream의8DWORD, 현재ID/manager/global handle포인터 선택, 전체 두 물리 output의 FNV1a64, 각64BYTE prefix, 실제 효과 순서를 기록한다. 전체 물리 output은 각각69,632BYTE이며 미사용 guard도 hash에 포함한다. 출력 전체를 직접 hex로 저장한 자료라고 주장하지 않는다.

최초 fixture SHA256은 **`8619983808122e17dd261c974a25906374fa46aee16293a51f119d3fad9fe973`**이다. 생성기 멤버 설명 주석만 보완한 뒤 보고서의 생성기 SHA 메타데이터를 갱신했고 fixture 바이트는 그대로 보존했다. 동일 도구 `--verify`는 PE/함수표/기반도구/현재생성기/최초fixture SHA를 확인하고 원문90관측을 실제 재실행하여 차이0을 확인했다.

## C++ 재생/실제 저장소 검사 초안

- `FortSubscriberWrite_ReplaysNative90CapturedIdAndDelayedNameWrites`는 fixture90행을 실제 `SaveFortSubscriber`, 기존 `FinishFortSubscriberRegistration`, `SaveFortMissionName`, `WriteMemStream`으로 재생한다. 입력/외부 변경만 구성하고 모든 기대는 독립 fixture의 상태/효과/전체 hash에서 읽는다.
- `FortSubscriberWrite_RealStorageKeepsChosenManagerAndCapturedPhysicalName`은 보호 TEST01의 실제 파일/일반 로더로 두 manager를 설치하고 빌린 헤더를 사용한다. helper가 현재 manager를 바꾸고 ID쓰기 뒤 header를 다시 바꾸어도 선택한 실제 Subscriber stream에 캡처 ID와 ID쓰기 뒤 바뀐 이름을 저장하는지 확인한다. 보호 원문과 다른 manager의 Subscriber는 보존한다. 새 Fort 이미지/생성자를 우회하여 NewFort 전체 성공을 합성하는 검사가 아니다.

두 C++ 검사는 아직 컴파일/실행하지 않았다. 원문 도구 검증과 실제 C++ 회귀 통과를 합산하지 않는다. root가 다음 단위에서 CMake/제품/검사에 채택하고 집중/전체/GUI를 수행해야 한다.

## 후속 연결

NewFort 부모의 subscriber callback은 owner/capturedID/capturedHandle을 그대로 child에 전달하고, 실제 같은 Client Subscriber/Config로 등록 helper를 연결해야 한다. Create/Write의 실제 저장소 장치와 양쪽 staging의 patch를 충돌 없이 합친다. TypeNames/초기화 부모와 Client의 lazy 이름표 owner, AddIsland, 전체 Native State7 및 실제 NewFort GUI는 별도 후속이다. 이번 child 관측으로 전체 편집기/미션 목표 완료를 주장하지 않는다.
