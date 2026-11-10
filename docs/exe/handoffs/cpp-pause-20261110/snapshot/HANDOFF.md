# NoWorld Snapshot 후보 최종 인계 / 사용자 중단 반영

2026-11-10 / HJOW-Athlon. 부모 제품1265 debugBurst 단위와 별개인 **제품1266 이후 접합 후보**다. 사용자의 현재 작업 마감·문서·커밋 뒤 목표 일시중지 지시에 따라 새 구현 확장을 중단하고 현재 후보/검증/장부만 마감했다. 자식은 제품/LEFT_JOBS/docs/AGENTS/원본/CD/fixture를 수정하거나 커밋하지 않았다.

## 동결 파일

실제 경로는 `H:/Workspace/git/NetstormReborn/extracted/cpp-resume-1265-candidate/`다.

| 파일 | 최종 SHA-256 |
| --- | --- |
| cpppj/src/client/NativeFortSnapshot.cpp | bb4cc9a0a78a2b16dfac0f345f51fbdadef17d512ccddcd7adabc1c325bec173 |
| cpppj/src/client/NativeFortSnapshot.h | c60631169ceed21c096686566d3dc182f1c27fad61d86ba6223814326c263d2e |
| cpppj/src/client/NativeFortPreparation.cpp | 1841b8c87735fa4f2a1878ba0337c6624cd3c8b116c33bd3f6b81027d3551ba2 |
| cpppj/src/client/NativeFortPreparation.h | e675dbfb3576d12bc41c8212fd00022606503b2f7bcbee41fe3e46f0c6dbfef0 |

위 네 파일은 UTF-8/LF다. 새 helper/생성자·멤버/함수/반복문의 한국어 설명을 보강했고 실제 진단 문자열만 원문 영어를 사용한다. 모든 선언/정의는 후보 구문 검사에서 확인됐으며 옛 m2c2처럼 닫히지 않은 namespace나 선언-only method는 없다.

## 검증과 원시

- **최종 단일 후보 구문 검사 종료0:** `check-candidate-final.cmd`, `candidate-syntax-final.log`, `candidate-syntax-final-exit.txt`. MSVC `/std:c++20 /utf-8 /EHsc /Zs`로 Snapshot/Preparation 두 cpp만 확인했다. 구문 검사 전 네 소스 해시(`candidate-source-final-before-syntax.json`)와 이후 실제 바이트가 일치했다.
- 제품 CMake/빌드/링크·게임·MP·패킷·단위 검사는 모두0이다. 구문 검사 통과를 제품 동작 통과로 세지 않는다.
- 첫 Snapshot 구문 검사에서 옛 WIP의 `o/RawCarrierDropPoint.h`가 현행에 없다는 C1083을 확인하고 client include/namespace로 교정했다. tool 관측은 `snapshot-syntax-first-observation.txt`다.
- 이후 두 구문 명령이 같은 로그 경로를 잠가 후속 redirection이 실패한 관측은 `syntax-concurrency-observation.txt`다. 해당 후속 명령의 종료0은 검증으로 세지 않았다. 모든 이전 핸들이 끝난 뒤 위 최종 별도 로그의 단일 명령을 수행했다.
- **정적 원문:** `original-fragments.c`·`original-functions.asm.txt`·`native-function-ledger.json`에 기존 정밀 생성 C와 보호 PE51함수의 inclusive 범위/각 범위 바이트 SHA/소스 줄/호출자/후보 대응을 보존했다. 새 디컴파일/원문 실행은0이며 마지막 Client2026-11-03/HJOW-Athlon을 재사용한다. `diagnostic-map.json`50행에 표현식·파일·행/원본 함수를 연결했다. `candidate-manifest.json`은 코드/증거 해시와 검증·한계를 묶는다.

## 재개 직후의 정확한 접합

1. 사용자 재개 뒤 AGENTS/LEFT_JOBS와 현재 제품을 다시 읽는다. 코드 네 파일을 작은 hunk로 제품에 넣고 두 cpp만 CMake 등록한다. 기존 NativeFortSaveVirtual·기존 o 순수 부모·현행 Client/Template/Storage 장기 owner는 그대로 쓴다.
2. UberGump에서는 기존 `templateStorage_`, `*missionOptions_`, `*missionRestrictions_`, `FortMissionSaveEffects()`, `editorCatalog_->Files()`를 인자로 넘긴다. 구체 호출 예시는 README다. typed/CD 기존 경로는 별도로 유지한다.
3. Native SaveGoMain/TestBattle의 snapshot은 `SnapshotNativeCurrent(...,true)`로 연결해 원문004bfb50의 현재 owner→manager0 자연 반환을 먼저 보존한다. 무조건 전체 현재칸004bfac0은 false다. 두 wrapper를 합치지 않는다.
4. Native JoinTestBattle 준비는 `PrepareNativeFortDescend(client_,*templateStorage_,*rawButtons_,event.y,snapshotCallback)`으로 교체하고 마지막 callback만 `SnapshotNativeFort(...,slot)`로 연결한다. signed y/준비가 이름검사보다 먼저인 순서와 기존1bh/현재 Source를 유지한다. 준비-prefix는 next19를 쓰지 않으며 Descend의 기존 꼬리는 별도다.
5. 현재 이름/편집복귀/SetFortType·StateWorldCleanup의 Native snapshot/Notify caller도 같은 실제 API에 연결한다. 이름의 state getter는 `GetNativeFortTemplateState(client_,templateStorage_,slot)`이다. Notify는 `NotifyNativeFortTemplate`만 호출해 전체 snapshot을 추가하지 않는다.
6. 제품 링크/허용된 일반 Edit 왕복·실제 사용자 홈 저장·읽기 전용 설치 검증과 인계/커밋은 부모의 후속 단위다. MP/Join/특정 공격·포획·희생·승리·사용자 직접 플레이 제외는 계속 유지한다.

## 구현에서 원문으로 교정한 사항

- 옛 WIP의 CurrentTechnology는 잘못된 Player+6c였다. 실제0048fc60/RawPlayerWorld의 **known0→0, 현재 Player+9c DWORD**로 고쳤다.
- Factory mode0 앞에는00475530의 flags1 `0x04000000` 관문이 있다. 그 뒤에만 실제+b4·내용물·Allowed 조회·같은 장기 Deck 활성화를 선택한다. 임시 RawProductionDeck을 만들지 않는다.
- 00432430의 Territory record 조회는 높이0에서도 최초(0,0) selector/진단을 먼저 수행하고 현재 높이를 다시 읽는다. 단순 y/x for-loop helper로 대체하지 않았다.
- RegionAllowed는 현재 editor/missionPost와 signed originalOwnerBYTE/currentOwnerDWORD를 구분한다. version/scope/두 mask는 같은 Template SerializationGlobalsOwner에서 직접 빌린다.
- CoreData/Battle snapshot도 실제 Core·기술·다이스 count·같은 CRT/GameRandom·FortCoreSeed/SP writer를 연결했다. 이전 Session/World·권한/PlayerKnown·성공 상수를 합성하지 않는다.

**남은 한계:** 이 후보는 M6-n 전체 NoWorld/모든 process VT 수명 감사 마감이 아니다. 제품 접합/링크/허용 저장·GUI는 미실행이고 모든 원문32비트 heap/SEH/임의 외부 메모리 입력의 동일성은 주장하지 않는다. 원문4bd540 wrapper의 생성 C 본문은 현행 정밀자료에 없어 독립 함수로 장부에 더하지 않았으며 Notify는 기존 RawPlayerTemplateState.Changed/NotifyPlayerTemplateChanged를 재사용한다.
