# 기존 Fort Editor의 최초 설치 기반과 현재 경계

2026-10-24, 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 마지막 수행 2026-10-23**이다. 기준 판본은 10.78이며 원본·CD·AGENTS는 수정하지 않는다.

## 실제 Session 연결

`CampaignInstallMode::ExistingEditor`는 생성자 입력이다. 첫 Player·지형·저장 객체가 생성되기 전에 실제 SurfaceWorld의 소유권·연결·다리 붕괴 모드와 Session의 Editor/missionPost/Fort/Objects/FinishOwner/Reward/사제 소비자를 설정한다. 00594fa0의 Mission은0, 00594fa4의 missionPost는1이다. 별도 옵션인 Player.special(0055a480)을 Editor라는 이유로1로 만들지 않는다.

Client 하나가 `FortTypeConversionStorage`의 현재 DWORD와 실제255BYTE 할당을 소유한다. 현재값 대입·본문 초기화·해제는 원문 부모의 위치에 남는다. 빈 TypeNames가 현재값을0으로 덮더라도 이전 할당을 자동 반납하지 않는다. Session마다 별도 전역이나 타입표 사본을 만들지 않는다.

일반 Fort 부모는 기존 실제 지형·Objects·Graph를 재사용하고 같은 Session의 Money·Badges·Technology를 연결한다. Money는 실제 분산 SP와 공용 난수를 사용한다. Badges는 Client의 같은6160BYTE 장부다. Technology는 선택 당시의 `TemplateWriteStream`을 고정하고 TypeNames와 원시 Contents 모두 실제 `Storage.AccessSection` 안에서 실행한다. Contents의 현재 변환표/두 TYPE 조회/QA·QB/즉시 생성·중첩 순서를 parsed 내용물로 대체하지 않는다.

지형 회복은 현재 owner의 실제120BYTE를 읽고 각 flags bit4를 회복한 뒤 같은 manager의 새 Territory에 일반 MemStream으로 쓴다. 그 뒤 현재 owner의 같은 State 첫 BYTE bit4를 지운다. Deck은 같은 ProductionDeck과 GameRandom에서 Clear/Open/Full·Append/Refresh를 실행한다.

`LoadExistingEditorWorld()`와 `LoadExistingEditorDeck()`는 설치 단계다. 공개 Snapshot의 최종 loaded 계약은 유지하고, 전체 설치 부모만 사용할 private 저장 본문을 분리했다. EditorPlayers의 마지막 맵 후처리는 실제 Pool/Finder/표면 WORD/spot/Unpop/Release/Graph를 사용한다.

## 원문 근거

- [TypeNames](cpp-fort-type-names-load-20261024.md), [실제 스트림 빌림](cpp-fort-type-names-access-proposed-20261024.md)
- [원시 Contents](cpp-fort-contents-load-20261024.md), [Session 연결 제안과 동결 경계](cpp-editor-session-children-contents-proposed-20261024.md)
- [Deck 부모](cpp-fort-deck-load-20261024.md), [Deck 복원 primitive](cpp-deck-restore-20261024.md)
- [지형 회복](cpp-fort-territory-recover-20261024.md), [Editor 맵 후처리](cpp-editor-world-finish-20261024.md)
- [기존 파일과 설치 외곽](cpp-existing-editor-install-outer-20261024.md)
- [Subscriber 원문 로더](cpp-fort-subscriber-load-20261024.md), [새 등록 ID 생성](cpp-subscriber-id-generate-20261024.md)

최초 [children patch](patches/cpp-editor-session-children-20261024.patch)와 [원시 Contents delta](patches/cpp-editor-session-contents-20261024.patch)는 저장소에도 같은 SHA로 보존한다. root의 실제 적용에는 새 Territory include 때문에 delta의 include hunk만 수동 병합했고, 이후 지형 회복·private Snapshot 연결을 별도로 더했다. 최초 제안 자체를 최종 제품 diff로 덮어쓰지 않는다.

독립 최초 관측은 변경하지 않는다. 맵 후처리의 최초 자료와 v2, 설치 외곽의 최초 관측에 포함된 부가 디버그 항목의 오측정도 별도 보존한다. 하위 몸체를 명시 관측 경계로 둔 원문 실행을 실제 전체 게임 완주로 표현하지 않는다.

## 검증 진행과 이번 단위의 범위

단위383은 최초 ExistingEditor 모드, 실제 World·Deck 단계, 원시 Fort 로더9개와 Client 변환표 소유자까지 검증한다. 신규34검사를 공용 대상에 등록했다. 전체 파일 설치 외곽, 최종 HUD/State/loaded, 메뉴 진입과 전체 저장 후 별도 Editor 재로드는 후속 단위다. 현재 단계에서 전체 Editor 설치가 완료됐다고 판정하지 않는다.

첫 제품 빌드 `extracted/cpp-editor-install-20261024-foundation-build1.log`는 공용 헤더로 발생한 큰 순차 재컴파일의 성능 전환을 위해 해당 CL 프로세스를 의도 중단했다. 종료1/MSB6006(-1)은 코드 오류로 판정하지 않는다. `foundation-build2.log`는 `CL=/MP4`로 종료0이며 C4458 두 경고를 확인해 세 지역 변수 이름을 구체화했다.

`build3.log`의 제품 빌드는 성공했고 검사별 서로 다른 컴파일 설정이 마지막 검사들을 직렬 처리함을 확인했다. 정확한 해당 MSBuild 프로세스와 자식만 종료해 첫 로그/종료1을 보존했다. 처음 단일 CL PID 종료 시도는 이미 종료돼 실패했고, 이후 tree 종료의 tracker 경합 표시는 전체 관련 프로세스가 사라졌음을 다시 확인했다. 완료된 제품/중간 파일을 삭제하지 않았다.

`build4.log`는 `CL=/MP4`와 `/p:UseMultiToolTask=true /p:EnforceProcessCountAcrossBuilds=true /p:CL_MPCount=4 /p:MultiProcMaxCount=4`를 사용해 종료0/관측 경고·오류0으로 끝났다. [Microsoft의 작업 스케줄러 설명](https://devblogs.microsoft.com/cppblog/improved-parallelism-in-msbuild/)에 따라 서로 다른 컴파일 단위도 최대4개 작업으로 처리한다. 설정 전환이 명령 기록을 바꿔 추가 재컴파일을 유발했으므로 후속 빌드에도 같은 설정을 유지한다.

최초 집중검사는 신규34개 중33개가 통과했으며 실제 Editor의 저장 사제2개가 사라지는 실패1개를 발견했다. 진단 빌드에서도 실제 사제0/Player1을 관측했다. 기존 원문 oracle의 `blocked=0x54db14`와 Editor prefix의 같은 FullDeck=1을 대조해, Session이 생산 소비자에만 이 값을 쓰고 객체 삭제 억제 소비자에는 전달하지 않았음을 확인했다. 최초 실패 로그·소스와 진단 실행은 보존한다. 객체 로더와 기대값을 바꾸지 않고 첫 Editor 모드 설정에서 `objectState.fortDestroyBlocked=true`를 함께 적용했다. 독립 읽기 검토에서도 PE004b71e1의 쓰기와004bdfc3의 같은DWORD 관문, 기존 Fort1/Blocked1 관측27행의 삭제 생략을 확인했다. 현재 fullDeck는 생성자 뒤 별도 변경 경로가 없으며 추후 변경 입력을 추가하면 같은DWORD 참조/현재getter로 통합해야 한다.

최종 `build5.log`는 종료0/관측 경고·오류0이다. 신규34개와 기존 전체Snapshot3/Client파일옵션5/객체로더4를 합친 **46검사 실패0**이며 `extracted/cpp-editor-install-20261024-final-focused.json`과 각 원시 로그에 보존한다. 실제 첫 Editor에서 사제2개/소유자1·2/원본TYPE 최대HP,SP194162,고정26기술,같은 Client 장부·변환표,작업manager120BYTE 지형 회복과188TYPE FullDeck 필터 및 누락 UI의 부분효과를 확인했다.

최종 전체CTest는 **2506검사 실패check0/종료0·337.50초(전체337.56초)**다. 제품 SHA256은 `6c31bab2c03323cba008dc447435b9826a0196d0a499f6bd43d9f5775189e5e0`, 검사 SHA256은 `5032329e702439c49f23a91bb5eba1d71fdece128e08950f8f4843f2cbc56be2`다. 전체 originals1394파일이 시작과 동일하며 추가·삭제·변경0이다. 첫 감사는 Windows 기준표와 POSIX 키를 비교한 형식 오류였고 해당 보고서를 별도 보존한 뒤 같은 Windows 상대경로로 대조해 동일성을 확인했다. 원문9도구의 `--verify`, UTF8/AST/JSON·문서 링크·공백·보호 경로를 확인했다. 원시 결과는 `extracted/cpp-editor-install-20261024-*`, 최종 요약은 `-final-audit.json`에 있다. 이번 단위에서는 게임/GUI를 실행하지 않았다.

## 남은 일

Subscriber/새 ID 생성의 독립 몸체도 복원했지만 실제 Client 전역·기존 파일 설치 순서 연결은 남는다. 전체 설치 외곽의 private Snapshot/State/Notify/startupStateCheck/lastEditFort/EditorPlayers와 최종 loaded를 연결하고, 실제 변경·전체 저장·별도 Editor 재로드를 검증해야 한다. 선택정보 UI0042f540과 startupStateCheck의 미복원 경계를 빈 성공으로 통과시키지 않는다. 현재 일반 Objects 로더의 parsed 입력 경계도 원시 Technology reader와 구별한다. [실제 메뉴의 확인된 접합 지점](cpp-editor-menu-integration-boundaries-20261024.md)을 다음 구현의 기준으로 사용한다.

실제 Edit 메뉴, NewFort, 이름 입력·변경, Save as, 편집 도구와 언어별 시나리오 실행은 계속 잔여다. Early 전체·캠페인 전체·Demo3의 사용자 완주와 HUD/소리/화면 동일성도 전체 목표의 별도 검증 대상이다.
