# Client Core seed 소유자와 Session SP writer 접합 제안

2026-10-24 **단위386 미적용 제안**이다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. [제안 patch](../../extracted/cpp-client-core-seed-sp-20261024-proposed.patch)는 기존 `ClientRandomGlobals.h`, `RawCampaignSession.cpp`와 신규 `ClientFortCoreSeedTests.cpp` **3파일**만 포함한다. 작업은 `extracted` 사본에만 적용했으며 공유 기존 소스/CMake/LEFT/git·공용 build/lib·원본/CD·GUI에는 변경·실행이 없다.

**사본 patch check/apply 종료0, Session/신규검사 MSVC `/Zs` 종료0·관측 경고/오류0**이다. 새 검사3개는 적용 후 실행용 초안이며 **링크·실행하지 않았다**. 독립 [SP writer198관측/4검사](cpp-fort-sp-write-20261024.md)와 [CoreWrite 부모](cpp-fort-core-write-20261024.md)의 기존 검증을 이 Session 접합의 실제 실행 성공으로 합치지 않는다. 동료가 세 파일을 읽기 검토해 명백한 순서·수명 오류가 없음을 확인했다.

## 실제005c89b4의 초기값과 수명

[정적 도구](../../extracted/cpp-client-core-seed-sp-20261024-proposed/analyze_seed.py)는 보호 PE를 읽고005c89b4가 `.data`의 **raw 파일 범위 밖 virtual zero**인 것을 확인했다. 섹션 virtual779444/raw118784BYTE이며 mapped 최초DWORD는 `00000000`이다. 이 결과는 [정적 report](../../extracted/cpp-client-core-seed-sp-20261024-proposed/seed-static.json)이고 새 Native 실행 fixture가 아니다.

실제 cleanup **004b3120**은 `66 89 1d b4 89 5c 00`, 즉 `mov word ptr [005c89b4],bx`다. [복원한 공통 정리 부모](cpp-state-world-cleanup-20261024.md)의 독립294관측에서도 그 시점의 BX0과 **하위WORD만0/상위WORD 보존**을 대조했다. 원문 CoreWrite는 low16이0일 때만 준비/비영 low16 생성 후 현재 상위16을 보존한다. 최초값0을 Session마다 다시 쓰는 규칙과 다르다.

제안은 기존 Client 수명의 `ClientRandomGlobals`에 **실제 `uint32_t fortCoreSeed_{}` 한 개**를 추가한다. `FortCoreSeed()`의 const/비const는 같은 전체DWORD를 참조로 반환한다. CRT/GameRandom/GameMode/SubscriberIdInitialized와 별도 저장소이며 새 Client에서만 최초0이다. Sessionlocalseed·스크래치·저장할 때 복사값 조회·프레임별 동기화·생성/반납 reset은 추가하지 않는다. 실제 cleanup/CoreLoad/NewFort의 후속 쓰기도 이 같은 owner에 연결해야 하며 이번 patch가 그 전체 접합을 완료하지는 않는다.

`ClientRandomGlobals`의 크기가 바뀌므로 적용 후 이 헤더를 사용하는 Client/Session을 다시 빌드해야 한다. 이전 공용 정적 라이브러리와 사본 헤더를 혼합해 링크·실행하지 않았다.

## Session의 현재 소유자와 같은 SP stream

`WriteExistingEditorCoreData`는 기존 **명시 ExistingEditor/설치 중/slot0..8** 진입 관문을 유지한다. `services.randomGlobals`가 있을 때만 같은 shared owner를 캡처한 seed 참조 getter를 공급한다. 없으면 `globals.seed`를 비워 실제 부모가 그 위치에 도달할 때 필수 누락으로 실패한다. 정상 Editor/post의 자연 반환을 막는 선행 owner 검사나 새 fallback0은 없다.

`hooks.sp`는 Create가 반환한 opaque token1과 실제 `TemplateWriteStream`의 manager/section을 확인한다. 그 같은 참조를 **`CampaignTemplateStorage::AccessSection`의 동기 본문**에 넘기고 받은 fields16BYTE·기존 MemStreamHooks를 그대로 신규 `WriteFortSp`에 전달한다. nextRandom은 기존 `s.random.Next(limit)`이며 services owner가 있으면 이미 Client의 같은 GameRandom을 참조한다. 결과88BYTE를 별도 배열에서 만든 뒤 일반 WriteSection으로 다시 복사하지 않는다. span/물리 buffer hook를 AccessSection 밖에 저장하거나 반환하지 않는다.

기존 CoreWrite의 일반10BYTE/seed2BYTE 쓰기와 현재 seed.low16 관문·시각 기반 prepareRandom은 그대로다. 새로운 공개 API나 전체 Session용 임시 seed를 만들지 않는다. CoreRefresh **`hooks.refresh`는 계속 비어 있으며**, 선택된 일반 직렬화는 Create 뒤 그 실제 경계에서 중단한다. 별도로 준비한0044fc00 독립 모듈과 실제 Client Core/Player 수명 접합은 후속이다. 따라서 이 patch만으로 선택 직렬화·전체 Snapshot·Editor loaded가 완료됐다고 표현하지 않는다.

## 신규 검사 초안의 범위

새 `ClientFortCoreSeedTests.cpp`의 세 검사는 적용 후 공용 검사에 등록할 초안이다.

- 실제 `Client`를 Run 없이 만들고 seed 최초0·const/비const 같은 주소·별도 CRT/Game/ID/모드·Config 변경 뒤 전체DWORD 보존을 확인한다.
- 두 실제 `RawCampaignSessionServices::randomGlobals` 필드가 Client 반납 뒤에도 같은 seed 주소/owner를 유지하고, 다음 새 Client만 새0을 갖는지 확인한다. 이것은 서비스 소유권 검사이며 두 실제 Session 설치 실행의 alias 증거로 세지 않는다.
- 실제 Client seed·실제 `CampaignTemplateStorage`·동결 CoreWrite 부모를 사용해 **CoreRefresh 누락은 실제 CoreData Create 뒤**에서 중단하고 seed 조회/일반쓰기/SP를 앞당기지 않는지 확인한다. 명시 비Editor 입력의 부분효과 검사이며 전체 직렬화 성공이 아니다.

구문 검사는 제안 사본 헤더를 먼저 포함하고 공유 헤더는 읽기만 했다. VC14.51.36231/SDK10.0.26100.0의 `/W4 /permissive- /utf-8 /Zs` 단일 cl이며 obj·실행파일·공용 lib를 만들지 않았다. [syntax 결과](../../extracted/cpp-client-core-seed-sp-20261024-proposed/syntax-result.json)와 로그를 보존한다.

## 적용 순서와 충돌 범위

기준은 단위385의 실제 checkpoint/CoreWrite 접합이 들어간 작업본이며 [baseline](../../extracted/cpp-client-core-seed-sp-20261024-proposed/baseline.json)의 두 공유 파일 SHA가 정확한 기준이다. 선행 `RawFortSpWrite.h/.cpp`와 그 독립198관측 자료가 필요하다. 이 patch를 적용한 뒤 root가 제품 source와 신규3검사를 공용 등록·재빌드·실행해야 한다. CoreRefresh를 같은 함수에 연결하는 다음 delta는 이 patch 뒤의 문맥을 기준으로 준비한다.

ColorSort 고정 입력15파일 patch와 Client Chat/Core owner의 기존2파일 patch는 경로 집합이 겹치지 않는다. 실제 변경 대상은 **ClientRandomGlobals.h/RawCampaignSession.cpp**뿐이며 ClientMain/Screen/UberGump/표시기는 이 제안에서 수정하지 않는다. 서로의 최초 patch/Native 자료를 바꾸어 합치지 않았다. 최종 patch·사본·정적근거·검증 SHA와 범위는 [manifest](../../extracted/cpp-client-core-seed-sp-20261024-proposed/verification.json)에 고정한다.
