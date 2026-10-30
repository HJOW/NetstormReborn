# cpppj 공통 기술 획득과 Battle 저장 연결 — 단위598

2026-10-30, HJOW-Athlon(192.168.0.94, Windows 10 19045). 작업 전에 AGENTS.md와 두 인계 문서를 확인했다. 마지막 전체 정밀 디컴파일은 같은 PC의2026-10-23 결과다. 원본/정밀 소스는 읽기만 했으며 재디컴파일하거나 AGENTS.md를 바꾸지 않았다.

## 구현

10.78의00492a30을 `AcquireNewTechnology`로 복원하고 실제 Session에 연결했다. Help Learn과 기존 의식의 로컬 기술 획득이 같은 함수를 호출한다. 포획·희생·승리를 합성하는 경로는 추가하지 않았다.

원문은 현재 기술 부모를 한 번 조회해 버린 뒤 다시 조회한다.004acac0(parent,4,type,0)의 반환이0일 때에만 부모를 세 번째로 읽고004afa70(parent,type,0,4,0x10,0)을 실행한다. 실제 Session은 기존 Carrier 검색과 contained 생성을 사용한다. 검색 하위의 실제 반환은 일치 종류WORD이므로 SID를 비영 여부로 대신하지 않았다. 부모 함수 자체는 반환 EAX 전체를 판정한다. Native 경계의0x10000 비영 사례를 WORD로 잘라0으로 바꾸지 않는다.

기술을 이미 가지고 있어도 현재 맵 snapshot, 현재 언어 이름, `global.newTechName`, 지역256BYTE의 signed `%d` 그림 서식, `global.newTechPic`, `Tell("NewTech")`를 모두 원래 순서로 실행한다. 설정은 실제 Client Config와 알림, 안내는 공통 Tell을 사용한다. 이름은 현재 TYPE description의 번역 경계이며 원문 전체 공유 문자열 formatter의 별도 복원으로 세지 않는다.

004bfac0에는004bfb50의 선행 manager 존재 관문이 없다. 두 독립 Battle/Fort if와 각 시점의 현재 owner를 유지했다. 선택되던 Battle 저장 미연결 예외도 실제004bf880 본문에 연결했다. 공통 writer를 빌리되 Battle은 Technology→Deck→Objects→Territory→Money→Mission→CoreData→Notify, Fort는 TypeNames→Technology→Objects→Territory→Deck→Money→Mission→Badges→Notify의 서로 다른 순서를 유지한다.

CoreData writer는 기존 Editor 설치 외곽과 실제 Battle이 같은004beb30을 사용한다. manager/Metamap 진단 뒤 missionPost/editor의 자연 반환을 보존했다. 캠페인 로더가 원래 설정하는 현재 Template+8=1에 따른 Notify 반환도 그대로 사용한다. 이 변경에서 저장 관문을 강제로 켜거나 건너뛰지 않았다.

의식의 외국 SP 정규화·외국 기술 지급·로컬 특수 보상·로컬 갱신·MP 상태 경계는 여전히 잔여다. 전체 포획/희생 기능 완료로 평가하지 않는다. 도움말 외부 OS 프로토콜도 이번 범위 밖이다.

## 원문과 C++ 검증

[Native 집계](../../cpppj/new-technology-native-20261030.json)와 [관측 도구](../../tools/decomp_new_technology.py)는 보호 PE의00492a30만 실제 CPU로 실행한다. 현재 부모/검색·생성/snapshot/TYPE·이름/Config·printf/Tell은 명시 외부 경계다. 입력 타입4개, 검색 반환4개, 이름2개, 빈 이름과 각 외부 효과의 중단을 합한45사례를 독립 재관측해90관측/차이0을 얻었다. 게임이나 원격 세션을 실행한 관측이 아니다.

새 C++ 검사는45개 전체 호출 순서와 두 설정의 미작성/빈 값/첫NUL/중단 부분효과를 동결 TSV와 비교한다. 실제 Session 통합은 미션 Header 시작 기술을 공급하지 않은 빈 현재 기술 부모를 명시 입력으로 사용한다. Battle/Fort 각각 기술75 추가→같은 기술 재획득에서 실제 contained SID 유지, 저장 Technology 원문 `01 4b 04`, 두 번의 Mission 읽기와 Config/NewTech 순서를 확인한다. 파일의 TypeNames/Badges를 생략하는 Battle 분기와 CoreData의 자연 반환도 확인한다. 실제 창·음향이나 포획/희생 흐름을 실행한 검사가 아니다.

최초 통합 검사는 보호 시작 Fort의 저장 기술26개가 실행 중 목록과 같다고 가정해 두 모드/두 안내에서 각각 실패했다. 별도 Python Fort 파서로 파일의26개를 확인했다. 독립 Session에는 시작 기술 Header가 없으므로 현재 목록은 비어 있었다. 현재 목록을 명시 직렬화한 기준0과 획득 후 고정 원문을 비교하도록 검사만 고쳤다. 제품 게임 코드는 이 실패 때문에 바꾸지 않았다. 최초 실행 두 필터의 같은 실패는 최종 성공 수에 합산하지 않는다.

최종 집중 검사는 중복 필터1개를 제외한71개/실패0이다. 새3개와 저장 부모/CoreWrite/내용물/Help/기술/Editor 설치·checkpoint/Player Core·seed를 포함하며 등록은3490개다. 전체3490개를 실행한 결과는 아니다. 최종 Release 게임/검사 빌드와 기본 배포 빌드는 경고·오류0이다.

## CD 10.72 기록

정적 대응은 기술 획득0040e300, 현재 부모00406d90, 실제 기술 추가0040e240, 현재 snapshot004299d0, Battle00429880, Fort00429920, 이름00444cb0이다.10.72 획득 부모는 첫 getter 뒤 별도0040e240을 호출하고 TYPE stride0x1d4의 이름을 읽는다. snapshot의 두 독립 if는 같은 구조다. 이 판본을10.78 구현에 임의 설치하지 않고 명시 거부한다. CD 실행/복원 완료 근거로 세지 않는다.

## 배포와 실행 범위

새 배포본은 `extracted/cpp-new-technology-598-20261030/NetstormReborn-cpppj-598.zip`이다. 기본 스크립트의 CMake 의존성 확인과 동일한4개 병렬 빌드 옵션을 사용했다.1376항목/게임1358개, CD SMK2개와 런타임 DLL4개를 포함하며 설정·검사 사본은 포함하지 않는다. 한글 경로에 푼 실행 파일은 최종 빌드와 같다.

게임 SHA-256은 `13f35ef00c963c1a9b88a00e93fdf56d9aefecfe8c1e0b170b890d7be5499900`, ZIP은 `3a68ff02c7003cb1c271cadafe5b680cfb9a93207fd5b7fb370b1db6d133b5a8`이다. 실제 실행 집계와 보호/소스 바이트 확인은 [런타임 집계](../../cpppj/new-technology-runtime-20261030.json)를 따른다.

같은 ZIP에서 푼 실행 파일의 Tutorial1은23.665914초/종료0, 실제 플레이 phase4/정지0/Session/로딩 해제·게임 시각21.520초 증가와 실제 창 전체RGB 비교를 통과했다.480프레임/40fps 상한/창1024×768이며 게임 시각·판정은 바꾸지 않았다.

Edit는72.971011초/종료0으로 TEST02→시험 전투→Edit 두 왕복을 통과했다. 실제 시작2/편집 복귀2, 로컬 SP12345·AI2 Off1의 본체 부재와 이름/동맹 유지·AI3 존재·원문 브리핑을 확인했다.597의 별도 Native 기대값을 그대로 재사용한 인접 회귀이며 새 설정 기대값 관측으로 중복 합산하지 않는다. 원시 returned 화면도 확인했다.

최초 Edit 실행0.926328초/종료1은 이전 작업 공간 장부를 다른 절대 경로로 복사한 검사 준비 오류였다. 원래 root identity 진단을 우회하지 않고 자료를 보존했다. 다음에는 game만 신규 경로로 복사하고 시나리오 텍스트만 처음 이관할 source에 공급했다. 새 작업 공간/장부는 제품이 직접 생성했다. 실제 실행은 최초 실패1＋최종 성공2의 총3회다. 검사 사본의 텍스트만 바꾸었으며 보호 원본/ZIP 내부 자료를 수정하지 않았다.

기존 검사의 작업 CRLF와 Git LF 차이는 해당 테스트를 UTF-8/LF로 맞추고 속성을 고정한 뒤 다시 빌드했다. 최종 검사 실행 파일에서71개를 다시 확인하며 이전 실행을 더해 검사 수를 부풀리지 않는다. 최초 Native TSV는 -text로 보존한다.

이번 제품의 전체34경로/전체C++/Win11·실제 PCM/활성 사용자 전체화면·자연 완주/전체 동일성은 미검증이다. 이전595 제품의 전체34 진입은596의 별도 근거다. 이번 실제 실행에서 기술 획득 UI를 보았다고 주장하지 않는다. 금지된 포획/희생·특정 적 공격/시나리오 승리, 공중3종 전용, 신규MP 검사를 하지 않는다. 목표는 계속 진행 중이며 남은 UI/Edit·Native 선택·시나리오 조건·의식 보상과 후순위MP 무시험/4~7차를 완료해야 한다.
