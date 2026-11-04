# 단위875 — 의식 helper의 현재 사제·권한과 선택 장치

TempleRitualState가 실제 Session의 현재 사제 TYPE와 권한 주소를 빌리도록 연결했다. 의식 helper의 PATCH 효과는 도달할 때만 요구하며 Rune 생성의 물리 쓰기와 진단 정상 복귀도 원문 순서로 연결했다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**, 현재 호스트도 동일하며 원본/정밀 산출물은 변경하지 않았다.

## 구현과 범위

일반 사제 finder `004479c0`/CD `004817e0`은 각 후보에서 현재 사제 번호를 읽고, 의식 부모 `00447e40`/CD `00481fc0`은 사제 후속 처리 뒤 현재 boss를 읽는다. 새 `PriestType()/Boss()`는 실제 장기 주소를 우선하고 독립/CD 구성은 기존 필드를 사용한다. ConnectRitual에서 다른 권한 연결이 끝난 `assetState.Boss()`와 `priestState.movement.priest` 주소를 빌린다. PrepareRitual의 두 사본 갱신을 제거하고 다이스/제단/수명 모듈의 기존 fallback도 같은 현재 권한을 읽는다.

RawTempleRitual은 PATCH에서 생성 시 모든 효과를 전수 요구하지 않는다. 같은 풀/해시·TYPE/프레임/SHP/spot 크기 계약은 유지하며 도달한 callable만 복사해 호출한다. CD는 기존 생성 시 필수 효과 계약을 유지한다. Stage5는 효과 장치나 사제 슬롯을 읽기 전에 자연 반환한다.

Rune 생성 `00447ca0`은 HP→현재 가상 Stage→plain frame 쓰기→`isVoid()` 진단→extra bit1→가상 owner→새 Stage/좌표→Pop 순서다. PATCH plain 쓰기는 Count/HP 복귀 뒤에도 같은 물리 슬롯을 사용한다. 진단은 `c:\ns\o\squid.h:0x282`의 원래 표현/위치로 실제 Client 장치에 연결하고, 정상 복귀하면 같은 슬롯의 extra 쓰기와 다음 효과를 계속한다. CD의 기존 진단 예외 계약은 유지한다. Client expression 서비스 연결은 진단 UI의 전체 파일/행 표시 일치 주장이 아니다.

**Rune 생성·의식/희생·진단 경로 본체는 실행하지 않았다.** 현재 boss·물리 쓰기/진단/실제 서비스 연결은 코드/정적/컴파일 근거다. 신규 검사는 일반 조회와 효과 전 반환만 다룬다.

## 검증

제품/두 집중 대상 Release는 **1회·146.840초·종료0·경고/오류0**이다. 고정7소스와 최종 해시가 같고 추가 교정0이다. 집중 **고유22개/8묶음(새2), 첫 실패0**이다. 새 조회는 독립 번호158과 빌린 현재74를 구별해 같은 일반 finder로 사제를 찾고 전체 pool 원문을 보존했다. 새 반환 검사는 PATCH에서 빈 효과와 범위 밖 사제 SID여도 stage5에서 읽기 전에 반환하며, CD의 빈 효과 생성 거부는 유지했다.

기존 현재 checking/VT·Rune 번호/미션 상태 검사와 독립 수명1,928행의 배열/실제 Player 기록 재생도 통과했다. 수명 재생은 마지막 Damageable 진입에서 중단하는 접두이며 실제 전체 파괴/희생을 수행하지 않는다. 새 실제 Session/Native/fixture0·기존 기대값 변경0·보호2,761개/fixture941개 모두 불변이다.

제품875/11.01, EXE **10,601,472바이트**, SHA-256 `dfed059b19ab74ce858727ed282faa33e8d207ab0b54c651c084ffc7ed7e78fa`다. checking10(부모4/부분3/MP3)·완료부모38/부분3 유지이며 이 단위는 기존51함수 밖 의식 helper 연결이다. 전체 Pop·전체 동일성 완료가 아니다.

[JSON](../../cpppj/temple-helper-current-20261105.json)과 Git 제외 `extracted/cpp-resume-875/`에 원문/첫 소스/빌드/검사/보호 해시를 보존한다. 실제 게임/GUI·포획/희생/특정 적 공격/승리·신규MP/패킷/통신·공중3종 전용·물리/PCM/CD·전체71/CTest/34미션/ZIP 반복은0이다. 다음876은 다이스/제단 Regular의 선택 장치이며 의식 효과는 코드/정적으로만 다룬다. 다른 TYPE/모드·TYPE 초기화 writer·Teleport 다른 생성 함수·전체Pop/Factory/군도·메뉴/Edit/State/파일·Windows·후속 단계는 남는다.
