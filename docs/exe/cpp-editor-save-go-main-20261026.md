# SaveGoMain 실제 저장 선택과 next3 예약 — 467

- 작업 PC/날짜: HJOW-Athlon, 2026-10-26.
- 마지막 전체 정밀 디컴파일: HJOW-Athlon / 2026-10-23. 디컴파일 소스와 보호 원본은 변경하지 않았다.
- 이번 하위 단위는 원본 SaveGoMain 일반 입력·현재 저장 선택·next3 writer를 고정하고 제품 연결을 준비했다. **최종 C++ 빌드·집중 검사·GUI/State3 통합 결과는 root 검증 뒤 기록한다. 현재 이 문서만으로 실행 통과나 자연 메인 복귀를 주장하지 않는다.**
- 사전 원문/후속 State3 조사: [연결 재개점](cpp-editor-save-go-main-next-20261026.md).

## 독립 원본 관측을 먼저 생성

`tools/decomp_editor_save_go_main.py`는 게임/OS/창 없이 보호10.78 PE의 일반 입력 Guide/Gump 관문부터 SaveGoMain 분기와 `004bfb50`, `004b3540`을 실제 CPU로 실행한다. 큰 Battle snapshot `004bf880`과 FullFort snapshot `004bf9a0`만 선택된 하위 장치로 대체하여 this/현재 변경/명시 중단을 기록한다. 실제 Guide 가상 입력과 Gump 입력의 결과도 입력 장치로 제공한다.

원문 명령 DWORD `00542a20=020000cb`와 전체 입력 사건을 직접 사용했다. first와 second가 다른 입력을 넣어 **second/event.y**가 저장 여부를 정함을 대조한다. No0·비영1/ffffffff·뗌·상위 입력 플래그, Battle/Fort의 네 조합, Template owner0/1/2/8, manager0, Battle 하위가 owner/fort/header/next를 바꾼 경우와 두 하위의 명시 중단을 두 x87 제어어로 관측했다.

- fixture: `cpppj/tests/fixtures/editor-save-go-main-x86.tsv`, **166행**.
- 실제 정상 사건 끝 도달 및 스택 복구: **162회**. 명시 선택 하위 중단: **4회**. 부분 중단을 정상 반환에 합산하지 않는다.
- fixture SHA256: `6dce0bf479869104e0f85809b1c841655f79ea572e1a7486004f3a14b54e177e`.
- 원본 PE SHA256: `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`.
- 실행 보고서: `cpppj/editor-save-go-main-native-20261026.json`.
- native1은 신규 생성 성공, native2의 `--verify`는 같은166행을 새 CPU 실행하여 전바이트 동일함을 확인했다. 각 원시는 `extracted/cpp-editor-save-go-main-467-native1.log`, `...native2.log`에 있다.
- 보고서에는 도구/기존 입력 관문 도구 SHA, 실제 허용 코드의 범위/바이트 수/SHA와 실행 후 불변 확인을 보존한다. 기존 Native fixture는 수정하지 않았다.

## 원문 EAX/owner 재조회 위치

`004bfb50`은 최초 현재 owner를 읽어 manager 첫 DWORD를 확인하고, manager0이면 돌아간다. 그 최초 owner는 Battle 호출까지 유지된다. **Battle 하위를 실제로 실행했을 때만 복귀 직후 owner를 다시 읽고, 그 뒤 Fort 모드를 읽는다.** Battle이 생략되면 Fort에는 최초 owner를 넘긴다. Fort 호출 직전에 무조건 owner를 새로 읽는 구현은 이 원문 조회 순서와 다르다.

Battle 하위가 현재 manager를0으로 바꿔도 최초 존재 관문을 반복하지 않는다. 선택된 Fort 하위가 자신의 진단/물리 계약을 처리한다. 큰 하위가 중단되면 next3 writer를 실행하지 않는다. 같은 하위가 next를 임시로 바꾸고 정상 복귀하면 원문 최종 writer가3으로 덮어쓴다.

## 제품 코드 연결

- `client/EditorSaveGoMain.h/.cpp`의 `kSaveGoMainCommand=020000cb`, `DispatchEditorSaveGoMain`은 대상/뗌 관문 → y 비영의 실제 저장 → 정상 복귀 뒤 `nextState=3`만 수행한다. No0은 저장 callback이 없어도 정상이며 다른 명령은 false다. 원본에 없는 추가 Editor/모드 관문을 넣지 않는다.
- 같은 모듈의 `SnapshotCurrentMap`은 최초 owner/header 존재와 위의 Battle/Fort/owner 재조회 순서를 소유한다. 실제 존재값을 합성하지 않는다.
- `RawCampaignSession::SaveCurrentMap`은 설치된 실제 Session의 Template9 header 첫 DWORD·현재 TemplateOwner·같은 Surface Battle/Fort를 조회한다. Fort 하위는 기존 `SnapshotFort(slot)` 전체 부모를 재사용한다. **선택된 Battle 하위는 `004bf880 미연결` 경계에서 명시 중단한다.** 일반 Editor의 실제 Battle0은 자연스럽게 이 하위를 생략하며, 일반 전투 저장까지 완료했다고 주장하지 않는다.
- 현재 파일 쓰기는 기존 checkpoint가 빌린 `BindFortFileSave` → 실제 Template Notify/이미지/저장 → `EditorFiles::Writer`다. TypeNames/기술·Objects·Territory·Deck·Money·Mission·Badges 및 마지막 Notify의 기존 전체 순서를 유지한다. 새 직접 파일 writer나 원본 경로 저장을 추가하지 않았다.
- `readOnlyForts`·파일명·header·hidden·Mission 관문과 EditorFiles의 보호 경로/권한 재검사를 유지한다. Session이 저장하는 동안 같은 Client/Template9/EditorFiles 참조가 살아 있어야 한다.

## 집중 검사 준비

`EditorSaveGoMainTests.cpp`의 신규2사례는 독립166행 전체의 실제 조회 trace·Template180BYTE·현재 owner/모드·next·부분 중단을 재생하고, No/뗌/다른 명령의 장치 생략과 선택된 callback 누락을 확인한다. 현재 fixture의 코드·반환 관측을 C++ 기대값으로 다시 계산하지 않는다.

`CampaignSaveGoMainTests.inc`의 신규3사례는 기존 checkpoint TU의 실제 Client/작업본/로컬 양끝/State 공통 확정 도우미를 재사용한다. 기존 검사 본문을 바꾸지 않고 같은 TU 끝에 포함할 수 있게 준비했다.

1. 실제 설치 이전 거부 → 공통 확정 후 Editor 품질 변경 → No0 파일/RNG 불변 → Yes1 전체 Fort 파일 쓰기/재파싱/같은 live 섹션 일치를 검사한다.
2. 실제 manager 첫 DWORD0이면 Battle 선택/파일 쓰기를 생략하고, 원래 실제 header를 복구한 뒤 선택된 Battle 미연결이 정확히 중단하며 이전 next/파일을 유지하는지 확인한다.
3. 같은 Client readOnlyForts1은 전체 snapshot/난수/섹션 생성 뒤 실제 파일쓰기만 자연 생략하고, 저장 부모 정상 반환 뒤 next3은 예약되는지 확인한다.

이 검사들은 **아직 이 문서 작성 시점에 빌드/실행하지 않았다.** root가 최종 제품으로 실행·통합한 뒤 결과를 기록해야 한다. 작은 저장 부모 검사는 자연 State3 정리/메인 메뉴 표시/재진입의 완료 증거가 아니다. 실제 Event/명령 표와 자연 State3 prefix·공통144BYTE 확정·다음 Poll4는 별도 통합 소유자가 연결하고 같은 최종 제품에서 검증한다.

## 467 최종 통합 결과

Source6 집중181고유/189관측·신규29·전체C++3,194개·실패0, 같은 게임 GUI8·자연Early2→Tutorial3를 통과했다. 본문의 준비/연결 지침은 이 통합 제품에 적용됐다. [최종 문서](cpp-editor-menu-runtime-integration-20261026.md)와 [보고서](../../cpppj/editor-menu-runtime-integration-20261026.json)를 기준으로 통과 범위와 남은 전체 목표를 구별한다.
