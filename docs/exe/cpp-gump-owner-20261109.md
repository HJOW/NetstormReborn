# 작업1222 — Gump Form과 실제 UI 등록표의 장기 연결

Storage 기본 Gump 삭제와 실제 UI 등록표를 같은 현재 공장/TYPE·물리·가상 삭제에 연결한다. **제품1222의 구현·제한된 검증을 마감**했다. 전체 목표는 미완료다.

Native Session은 장기 Gump 본체를 빌린다. Gump TYPE는 기존 Storage `containedGumpType`의 같은 DWORD이며 Native Player/contained 통지도 같은 주소를 사용한다. Gump Host가 보관하던 사용하지 않는 TYPE 사본은 제거했다. 독립 typed Host/Session의 기존 값형 계약은 유지한다.

첫 메뉴 전에 실제 RawButtonControls의 등록표 통지와 그 저장소 pin을 weak로 연결한다. 현재 World의 명시 UI 소비자는 실제 전체 Runtime pin을 보존하며, Session 밖에서는 같은 실제 UI 등록표를 사용한다. 선택된 owner와 함수 사본을 호출 종료까지 유지하고 미연결 비영 UI를 성공으로 대체하지 않는다.

메뉴·생산의 Form 생성/삭제와 현재 풀/카메라 투영은 Native에서 Session을 임시 생성하지 않는다. 메뉴의 첫 Reference 인자도 생성 callback보다 먼저 현재 Host의 전체 TYPE를 읽는다. 실제 RawButtonControls/GumpLifetime 등록표·트리·지연 삭제 큐와 Floating SP Reference→owner/Attach→가상 UI Kill→Form 재삭제를 비MP로 검사한다. 새 글꼴 캐시는 보호 원본 밖에 둔다.

Gump 통지의 실제4ae9d0은 owner DWORD를 먼저 읽고0이면 반환한다. 비영일 때 현재 부모 WORD를 저장한 뒤464190의 현재 등록표/가상+4를 부르고 Form을 다시 읽지 않는다. pre/post는 실제 공통 깊이를 반납하며 직접 Form 삭제로 UI 통지를 새로 발생시키지 않는다. 원문 양판본 VT/전체 확보 함수와 짧은 PE 몸체는 `extracted/cpp-resume-1222/`에 보존했다.

현재와 마지막 정밀 Client 양판본2026-11-03/R전용2026-11-07은 HJOW-Athlon이다. 새 디컴파일·원문 변경은 없다. 새 MP·공중3종 전용·포획희생·특정공격·승리 검사는 제외하고 관련 비MP 집중·단순 생산/건설 요청·대기·Edit 왕복을 확인했다. 확정 결과는 아래와 같다.

## 확정 결과와 최초 실패의 정정

최초 빌드6331은483.235초/종료0, 최종 빌드16573은36.422초/종료0, 경고·오류0,17소스 고정이다. **비MP397/397**(기존371+신규16+기존 관련10)이 최종37665/66.794초/종료0으로 통과했다. 실제 등록표 재사용·지연 Kill/scalar·저장소 pin·현재 World 통지/만료·전체 DWORD·현재 공장/TYPE·가상 삭제·부모 종속 통지와 Floating SP의 Reference→owner/Attach→UI Kill→같은 Form 재삭제를 확인했다. 기존 Take600입력·메뉴308행도 직접 대조했다.

최초 집중88971은396/397이었다. 새 Take 검사를 client SID·다른 기존 타입·부모 WORD0 기대값으로 잘못 구성했다. 원문600입력과 기존 연결 검사에 맞춰 동일 현재TYPE7·flags0 지정SID·부모 WORD 보존으로 정정하고 범위/head/void·contained/owner/SID/타입 단언을 유지했다. 제품 본문·원문 fixture는 바꾸지 않았다. 최초 소스/고정 해시/빌드/397결과/실패 로그·교정 근거는 원시 first-*에 남겼다.

Tutorial34985는 TYPE75/owner1 실제 생성·요청0→1→600프레임13.581→24.085초 대기/015PNG 확인 후 종료0이다. 같은 EXE/800×600 설정 Edit58488은 TEST01↔Main2회/11관측 종료0이다. 보호 원본/CD/fixture·AGENTS2801파일과 EXE 바이트는 불변이며 모든 시작 빌드·집중·GUI 핸들이 종료했다. MP/공중3종 전용·포획희생·특정공격·승리 검사는 하지 않았다.

양판본22진입 요청 중 독립 전체 C/PE는14개, pre/post·알림의 짧은 실제 PE 몸체는8개다. +1c와+78은 같은 효과이며 반환 정리 규약만 다르다. 독립 C metadata8개 미확보를 전체 함수 복원 완료로 합산하지 않는다.

기본 Native Form 삭제는 **3/4종(Process/Depend/Gump)** 연결이며 Content1종·Regular48VT/23함수·큰 AI·배치/Fort 상위 Finish·전수 caller/owner·M6-l/m·M7/M8·K/S/B의16상위계획·등록34버그가 남는다. 전체 NoWorld/게임 동일성/전체 솔루션 통과를 주장하지 않는다.

## 사용자 요청에 따른 목표 일시 중지

사용자는 현재1222까지만 완료하고 문서 반영·로컬 커밋 후 목표를 일시 중지하도록 요청했다. 이 단위를 마감한 뒤 다음 제품 단위는 시작하지 않는다. 재개 지시가 오면 LEFT_JOBS.md의 M6-n Content Form 장기 연결부터 진행한다. 전체 목표는 미완료다.

- Netstorm.exe: 13971456 bytes, SHA256 `a03f1e80dd7bad63cebf2578d9a8a496c8b74e527d4d4917b0917198805f56a1`.
- R.exe: 677376 bytes, SHA256 `0a3fa33576f05323b00bef994f2d0b0c5b12e9f5e8d147716280c1441e633ded`.

[현행 인계](../../LEFT_JOBS.md)·[확정 보고서](../../cpppj/gump-owner-20261109.json).
