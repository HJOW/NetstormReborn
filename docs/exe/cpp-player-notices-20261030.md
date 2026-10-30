# cpppj 프레임말 Player 결과 알림·Core 기술 합계 — 단위604

2026-10-30, HJOW-Athlon(192.168.0.94, Windows 10 19045). AGENTS.md와 두 인계·깨끗한1f8933ab를 다시 확인했다. 마지막 전체 정밀 디컴파일은 같은 PC의2026-10-23이다. 원본/CD·AGENTS·정밀 C/표는 수정하지 않았다.

사용자의 “이번 작업 완료하고 문서화 및 git 커밋까지 완료한 후 목표 잠시 일시 정지” 요청에 따라 이 단위를 검증·인계·커밋한 뒤 전체 목표를 일시 정지한다. 전체 목표를 완료한 것으로 바꾸지 않으며 다음 구현은 사용자가 재개한 뒤 진행한다.

## 복원한 실제 호출

기존 메인 루프의 미연결00490da0을 원문00439c33과 같은 Sleep 뒤 위치에 연결했다. known/battle이 아닌 메인 화면은 Session을 요구하지 않는다. 선택된 실제 Session은 같은 Client 소유 Player9기록·현재 owner/기술 부모·실제 Carrier 내용물·기존 다이스 획득/로컬 갱신 자식을 사용한다. 새 World나 Player 사본을 만들지 않는다. battle은 갱신용 사본 대신 실제 공유 ownerMode에서 읽는다.

부모는 known→battle→캡처한 Player+a0 대기 타입→+9c 기술 부모의 선행 조건을 확인하고0048fc60을 실제 호출한다. 정밀 C는 이 getter를 undefined8처럼 합쳐 두 번째 반환값을 타입처럼 표현하지만 실제 EDX는 호출 전에 Player+a0에서 읽어 보존된 값이다. getter의 실제 반환은 EAX SID다. 새 코드는 대기 타입과 SID를 명시적으로 구별한다.

- 대기 타입2/3은 Player+71 BYTE의 기존 값과 타입-1 중 큰 값을 쓴다. 값이 증가할 때만 AltarUpgrade를 Tell하고 같은 캡처 기록의+a0을 지운다.
- noIsland는 Player+6c Core 합계의 낮은 BYTE가43인지 확인하고+71을0으로 쓴다.43일 때만 보상/갱신으로 이어진다.
- nugget는+71/a0만 지우고 반환한다. 이미 지급한 SP를 중복 가산하지 않는다.
- 일반 타입은 캡처한 SID의 FindContained(mask4,pending,index0)를 확인한다. 없을 때만 호출 순간의 현재 owner와 같은 캡처 기록의 현재+a0을 다시 읽어 보상/갱신을 수행한다. 마지막에는 캡처한 기록의+a0만 지운다.

별도0042b210 Core 합계도 복원했다. Core+6 마스크의32비트를 순서대로 읽고, 각 비트에 대해 현재 signed 타입 수와 TYPE+1ec를 조회해 첫 일치의 level(+94)+1만 합산한다. 마지막 현재 Core+5 BYTE를 더한 낮은 BYTE를 반환한다. DWORD/최종BYTE 감김과 TYPE 조회 뒤 마스크·타입 수·마지막BYTE 재읽기를 유지한다.

보상은 기존 RawDaisRegular::GrantCapturedType을 공유한다.00448310은599의 실제 Core→현재 Template owner→CoreData→사용하지 않는 owner 재조회→현재 Battle/Fort snapshot과 같음을 기계어로 확인해 같은 기존 갱신을 재사용했다. 비미션 랭크/비로컬 보상·MP 안내의 기존 명시 미복원 경계를 상수 성공으로 바꾸지 않았다. 이번에 그 자식 전체를 새로 복원한 것은 아니다.

## 검증 범위

[Native 집계](../../cpppj/player-notices-native-20261030.json)와 [관측 도구](../../tools/decomp_player_notices.py)는 알림 부모118사례와 그 안의 실제0048fc60, 별도 Core 합계92사례를 두 독립 emulator에서 실행했다. 총210사례/420관측·재관측 차이0이다. 부모 안의 큰 Core 합계/내용물·보상·갱신·Tell, 별도 합계의 TYPE getter는 명시 응답 경계다. 전체9기록·현재 owner·trace와 효과 진입 중단/부분 쓰기, 하위 응답의 owner/pending 변경 및 Core 마스크/타입 수 변경을 대조한다.

최종 같은 검사 EXE의 PlayerNotification3/RitualRefresh2/NewTechnology3/PlayerCoreData3/PlayerCoreRefresh7인 고유18개는 실패0이다. 등록3506개이며 전체3506개 실행의 증거는 아니다. 최종 게임/검사 빌드는 종료0·경고/오류0이다. 같은 실제 공유 battle 필드를 읽도록 마지막으로 수정한 뒤 두 대상을 다시 빌드했으며 그 전 바이너리와 구별했다.

603 ZIP의 동일한 게임 자료를604의 새 경로에 풀고 새 게임 빌드로 Tutorial1을8.310137초 실행했다. 실제 초기 안내·ESC/F1 열기/닫기·최종phase4/정지0·같은 Tutorial1/Tutorial/3·SP 불변·시각2.384→6.039초·창전체RGB를 확인했다. 이는 새 프레임말 호출이 들어간 기본 진입/자연 반환의 검증이다. 실제 업그레이드/기술 보상 UI를 유발하거나 포획/희생을 재현한 결과가 아니다.

현재 게임 SHA256은 `e375b587b41cac2092ac4bc90ce8631bce65bf934818e9509e1aef3c40d77395`다. 새604 ZIP도 생성해 같은 게임/지정 DLL/원천 자료 바이트를 확인했다. 1376항목/게임1358·CD SMK2/DLL4이며 ZIP SHA256은 `917e7a319e47fe474efa45d10b821a27554eaeba7acf7e9b9af25f133da42875`다. 검사 도우미는 배포에서 제외했다. 실제 실행은 build/bin의 이 EXE이며 새 ZIP에서 다시 실행한 것으로 합산하지 않는다. 마지막 전체34 진입은595제품/596이다.

## 인계와 일시 정지

[실행 집계](../../cpppj/player-notices-runtime-20261030.json), 원시 `extracted/cpp-player-notices-604-20261030/`에 최초 Native TSV·두 관측·집중 검사·빌드·실행·배포·보호 해시를 남긴다. 보호2572개 중2571개/기존 fixture749는 불변이며 새2개를 더한 현재 fixture는751개다. 유일한 입력 변경은 작업 중 별도 커밋4acd05dd의 AGENTS.md FSR/FidelityFX 공식 SDK 참고 한 줄이다. 이전/현재 바이트가 각각1f8933ab/4acd05dd의 checkout과 같음을 확인했다. 이 작업은 AGENTS를 수정/복원하지 않았으며 새 지침을 읽고 보존했다. dotnetpj 인계도 바꾸지 않았다.

CD10.72는 getter00406d90/별도 Core 합계0048ce90의 정적 대응만 기록한다.00490da0의 CD 부모 대응은 이번 정밀 표에 없으며 새 알림은10.78만 허용한다.

사용자 요청으로 이 단위 뒤 목표를 일시 정지한다. 재개 시 새 프레임말 부모와 실제 보상 UI·남은 UI/Edit·Native·시나리오·파일/수명 경계에서 이어간다. 비미션 랭크/비로컬 보상·MP 재접속 저장, 보조 TAFF 등록/전체 BaseFile와 Win11/PCM/활성 사용자 전체화면·자연 완주/전체 동일성·후순위MP 무시험/4~7차는 미완료다. 포획/희생·특정 적 공격/승리 재현, 공중3종 전용/신규MP 검사는 하지 않았다.
