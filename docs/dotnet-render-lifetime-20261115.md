# dotnetpj 0.534 — 공통 표시 등록·수집·실제 프로세스 수명

2026-11-15, HJOW-Athlon. 직전0.533은ad4aab0ab다. 기준은 cpppj RawConstructionLifetime.h/.cpp·RawRenderCollection.cpp의3e6286eb8bab56d6cbf26b35d567ab410ac1afde, RawRenderCollection.h의8f4a4d25fdb933c52f2404f5739dd1d0c4b41d8b, RenderProcessSystem.h/.cpp의c258c7f3723d1c32fbbfdf39fca4a6f5f0fc3db9와 RawConstructionVisual.h의5389347d381dbdfbbbfc67f540ed3d5bf702f412이다. 디컴파일 산출물을 변경하지 않았다.

## 구현과 범위

공통 RawRenderRegistry를 이식했다. 같은 원문 목록의 첫 빈 칸·40칸 확장·원본32비트 확보량·등록 뒤 부착·SID0의 signed깊이·부착 뒤 진입 부모의 현재깊이·정확한 토큰 삭제와 base의-12345표식을 보존한다. 함수 묶음과 초기 배열은 원문 값처럼 복사하고 실제 owner의 수명은 빌린다. 입력 필드를 살아 있는 다른 등록으로 재사용하거나 중복/잘못된 삭제를 하는 경우 기존 목록을 보존하며 명시 실패한다.

RawConstructionLifetime의 전체 버퍼 소멸·등록 어댑터도 연결했다. 캡처/출력/입자를 순서대로 반납한 뒤 공통 등록/base를 정리하고 flags bit0의 객체만 추가 반납한다. 공유 팔레트/카운터를 임의 초기화하지 않는다. 원문의 vector 반납은 C# 배열 참조를 빈 배열로 교체하는 방식이며, 명시 반납 콜백은 동일한 원문 순서를 관찰한다. 생성 어댑터는 입력 함수 사본에서 attach만 바꾸고 부착 후 현재 부모/영역/깊이를 돌려준다. 필요한 ConstructionVisualFields/Particle/Hooks와 실제 배율/역할/팔레트 상태를 추가했다. RawConstructionVisual의 생성/Seed/프레임 몸체는 아직 이식 범위 밖이다.

RawRenderCollection은 실제 같은 풀/목록/현재 배율의 Bounds/Intersects/Collect를 실행한다. PATCH double역수 곱셈과 CD 직접 나눗셈, NaN 교차 분기, parent head만의 원문 조회, limit 도달 뒤 조회 중단, kind3의 y+0.001와 하위WORD 깊이·Tail 보존을 유지했다. 기존16바이트 RenderCollectionEntry를 그대로 쓴다. Renderer의 정렬/출력은 별도 범위다.

RenderProcessSystem은 하나의 등록 목록/토큰을 모든 효과가 공유하게 한다. pending→ready 전에는 수집/출력하지 않고 같은 Host/풀과 현재 배율을 유지한다. Draw는 각 차례 현재 배열을 다시 읽고 콜백은 값 사본으로 호출하여 자기삭제·새 등록/확장이 가능하다. 오래된 토큰은 새 슬롯의 다른 효과를 출력하지 않는다. 실제 Kernel/파생 버퍼 소유권은 효과 모듈에 남는다. 삭제 영역은 원문 CRT의 선행 정수화·top 이외의 중간float저장 순서를 보존한다.

## 원본 대조와 실제 수명 검사

세 판본(originals/CD/10.37)의 등록·버퍼소멸 전체1377행, 영역/교차/수집4197행, 기본0x027f 정밀도의 삭제영역48행으로 총5622고유관찰이 일치했다. rendercollection의 정렬 Order108행은 상위 Renderer가 미이식이라 제외하고, 삭제영역0x037f 비교48행은 제품 기본53비트 검증에 합산하지 않는다. fixture3개 전체5778행의 SHA와 행 수는 등록했으며 원본/golden/기대값은 바꾸지 않았다.

추가5검사는 실제 공장/Form 부착→Kernel실행→Host.Kill→파생Dispose→목록/토큰 반납, ready 전 미공개, 출력 중 자기삭제와41개 신규등록/80칸 확장, 현재배율/limit/미완성null, 확보실패/함수사본·목록불변·잘못된삭제, 실제 건설 등록 어댑터를 확인한다. 최초 추가 검사2실패는 클라이언트 검사풀에서 서버 SID를 확보하려던 준비 오류다. 부모를 실제 client정책으로 확보하도록 교정한 뒤5/5통과했다. 제품·기대값 변경은 없다. 게임창은 띄우지 않았다.

생성 중 필드 노출과 Draw 길이 선행 고정의 대표 변이는 각각 집중5검사 중1실패로 검출했고 원본 바이트/SHA를 복원했다. 원복 뒤 전체 비증분Release빌드 경고0·오류0, 최종 관련 Core158/158·Client23/23은 실패0·건너뜀0·각 종료0으로 통과했다. 최근 전체회귀0.529와 이번 집중 결과를 구별한다. 로컬 자료는 extracted/dotnet-render-lifetime-0534-mutations/와 extracted/dotnet-render-lifetime-0534-final/이다(Git제외).

## 남은 실제 조립

제품 HostSession의 공통 표시 owner와 Burning14 생성/캡처/입자/프레임/삭제, 건설 파생 생성/프레임을 후속 결선한다. 합성 검사가 실제 Form/Kernel을 사용하더라도 제품 미션 화면/플레이 동일성까지 완료한 것으로 세지 않는다. 다음0.535는 공통 등록에 올릴 burning 생성/입자/대상 조회 묶음이며 아직 없는 RawSidRobust 상세 진단 부모도 같은 현재checking 계약으로 함께 이식한다.

PORT_MAP826행(L1 398+port428), fixture281개다. 원문 헤더의 의미필드/함수묶음만 옮긴 RawConstructionVisual 행은 port이며 전체 생성 복원 완료를 뜻하지 않는다. 전체 floor/reflected 커밋은 올리지 않았다. 새 외부 접속/NB 수정은 없다. 보호 원본/CD·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md·다른 프로젝트는 변경하지 않았다. 실제 포획/희생·특정 적 공격·승리·MP 재현은 제외했다. 목표 active·M1전체0/34·후속 단계 잔여다.
