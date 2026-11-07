# 1061 — 비Session Fort의 등록 ID 저장

1060에 남긴 현재 manager/fort가 살아 있으나 Session이 없을 때의 등록 동기화 오류를 해소했다. 실제 제품은 **1061 / 11.01**, EXE 12,365,824바이트, SHA-256 `fcf0ad4132dfa548b01bf56fa2ea8036f0d82486e1660a060b5e4bb783a33b3a`다. 직전 CP 커밋은 `9b2a0580 / 20261107-89 CP`다. 멀티플레이 동작 검증 완료나 전체 목표 완료를 뜻하지 않는다.

## 같은 실제 저장소와 원문 부모

`UberGump::SynchronizeRegisteredSubscriber`가 Session을 요구하지 않고 `SynchronizeClientRegisteredSubscriber`를 호출한다. Client 수명의 실제 사용자/handle/CRT/설정과 기존 Template9를 빌린다. 새 World/Session이나 임시 Fort 사본을 만들지 않는다. manager→fort 관문은 기존004b32f0 raw 부모가 소유한다. 관문 통과 뒤 현재 MySubscriber 쓰기→현재 owner/handle 조회→004bd400→변경 뒤 현재 owner의004bd360 순서다. 이름 NULL이므로 handle 복사·fort.mySubHandle 쓰기·제목 변경을 추가하지 않는다.

004bd400은 같은 실제 manager에서 Subscriber 섹션을 생성하고 고정 인자 ID/현재 이름을 기존 writer로 쓴다. 등록 helper는 같은 Client 설정의 ReadInt/ApplyScriptConfig·최초시드 DWORD/장기 CRT·실제 time32/clock을 사용한다. 등록으로 현재 전역이 바뀌어도 부모의 고정 인자 ID와 manager 재조회 경계를 보존한다. 실패 전 이미 수행한 쓰기를 되돌리지 않는다.

004bd360 몸체를 `NotifyPlayerTemplateChanged`로 공유했다. 기존 Session의 `RawPlayerTemplateState::Changed`도 같은 부모를 호출하고, 현재 멤버 callback은 선택 시 호출해 진입 시점의 사본으로 고정하지 않는다. 파일명 첫 BYTE 진단→isMine/+12 진단·재조회/자연 반환→현재 미션 진단→manager 진단→preventSave 자연 반환→실제 파일 저장 순서를 유지한다. 비Session 경로의 미션은 missionPost(594fa4)가 아니라 실제 SurfaceStorage.links.mission(594fa0)이다.

파일 저장은 기존 CampaignTemplateStorage.SaveFile의 이미지 생성→현재 readOnlyForts/로컬 owner/숨김9BYTE 관문→권한/쓰기→할당 반납을 사용한다. 숨김은 같은 missionOptions 배열+3e의9BYTE이며 사본이 아니다. TYPE 개수도 같은 Client 표면 저장소의 현재 값을 읽는다. 실제 권한/쓰기가 선택될 때만 기존 EnsureEditorFortFiles와 사용자 작업 namespace/보호 정책을 빌린다. 원본/CD 쓰기를 허용하지 않는다. 기존 이미지 진단 경계는 Session 저장과 같은 명시 오류이며 전체 진단 UI 복원으로 계산하지 않는다.

## 설정 도구 명령의 성격

PE 명령 표00542b90에서005428e0의 실제 이름은 **AccountSetup**, 값은 **0200007a**다. 원문004d7ac3의 push 문자열은 **register**, 다음 Tell 문자열은 **Setup**이다. 004d5070은 setup.exe/setup\setup.exe/setup\debug\setup.exe 세 후보를 찾고 선택된 실행 파일에 register를 넘긴다. 인자가 비영인 다른 분기는 도구 활성 표식 확인→Tell Setup→400h 송신이다.

이 소비자를 일반 LAN 시작 명령으로 해석하지 않는다. **폐기된 외부 계정 등록/검증 프로그램 실행은 사용자 지침에 따라 연결하지 않는다.** 구 setup.exe나 원본 R을 실행하지 않았고 새 서버 실행 장치에도 이를 대신 넣지 않았다. 로컬 Fort 등록 ID 동기화와 구 외부 계정 등록을 구별해 [외부 기능 목록](cpp-external-services-20261101.md)에 기록했다. 이 분석으로1060의 미분류 도구 일반 명령 잔여를 외부 등록 경계로 분류했으며 외부 등록을 복구할 후속 작업으로 남기지 않는다.

## 확인과 다음 범위

첫 Release/x64 빌드는 종료0이다. 검토에서 기존 Session의 callback 현재 조회를 보완한 뒤 최종 게임 Release도 종료0·경고/오류0이다. 새 단위/MP/게임/통신/원본R/실제 파일 저장/메시지 송신 검사는 모두0이다. 보호2800파일 불변·새fixture0이며 전체 검사/34미션/ZIP을 반복하지 않았다. 원시 함수 사본/명령 표 분석과 빌드 로그는 Git 제외 `extracted/cpp-resume-1061/`, 보고서는 `cpppj/registered-fort-subscriber-20261107.json`이다.

다음은 새 R.exe/OS 실행·실제256칸 전송/Root·CS·17h/1dh 응답, 수동 검색 UI·State28~33/메타·LAN/Outpost/checking5다. 이후 한국어 후속→Edit 시나리오UI다. 전체 목표는 미완료이며 MP 검사는 계속 제외한다. 마지막 전체 클라이언트 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**, R 전용은 **2026-11-07 / HJOW-Athlon**이고 이번에는 다시 수행하지 않았다.
