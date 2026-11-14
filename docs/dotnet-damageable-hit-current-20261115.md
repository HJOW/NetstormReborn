# dotnetpj 0.533 — burning·일반 피격의 현재 부모와 판본 경계

2026-11-15, HJOW-Athlon. 직전0.532는747b97dd8다. 기준은 cpppj RawDamageableBurning.h/.cpp의a9157275900dc6537b7aa423481272be7328b8ba, RawDamageableHit.cpp의c3cfc98fa788f6229dbf5dd0b57752c90dc9f4d9와h의8b467ca03249dbc963ceaf8efdd35fa6e26986b7, RawSquidHitPoints.h/.cpp의552656c0c8bb46b48637b26a1acb59213edc1de1이다. 디컴파일 산출물은 변경하지 않았다.

## 구현

burning에 현재 실제 RawSquidReward 최대HP 부모를 받는 경계를 추가했다. 기존 RawHitPoints API는 같은 몸체로 이어진다. 현재 checking DWORD 뷰를 값 복사하며 실제 owner 주소는 빌린다. 두 번째 프로세스 진단이 복귀하면 이미 저장한 HP/최대HP/첫조회 결과는 보존하고, 이후 현재 TYPE/위치/역할은 원문 위치에서 다시 읽는다. 벌프 소리 뒤에 생성 경계를 검사하는 원문 부분효과도 유지한다. 함수 묶음 자체는 복사한다.

일반 피격은 현재 checking·owner·primary island·server 조회를 유지하며 서버 권한을 치명상에서만 읽도록 교정했다. CD의 signed WORD HP, Owner32/extra35, PATCH 전용 held/dead 진단·경고 사건/문장 선택, 레벨 보정 판본 인자를 구별한다. 공통 SetRawSquidHitPoints는 CD에서2바이트만 써 인접 stale바이트를 보존한다.

같은 풀의 실제 burning을 입력 함수 사본에 연결하는 어댑터와 현재 raw VT→실제 가상98 주소 분배를 추가했다. C#에는 아직 상위 GunHitHooks 형식이 없으므로 원문의 damage 슬롯을 Action 콜백으로 제공한다. 빌린 진입 표의 첫 일치 항목만 선택하고 다른 파생은 명시 fallback으로 넘기거나 실패한다. 총기 시스템 전체/제품 실행 결선 완료로 세지 않는다.

## 검사

기존 PATCH2923관찰과 사제 피격 기존 대조를 유지했다. 일반 피격은 CD2923행도 추가해 합계5846고유원본 입력의 결과·경계 순서·슬롯50/36바이트·사망/경고 기록을 모두 대조했다. CD 최대HP만 읽는 검사 부모는 기존 물리188칸을 유지하고 같은 앞171레코드를 실제 피격/연소에 빌린다. CD36바이트 밖 관리 패딩14바이트도 불변을 확인했다. 전체 CD 보상HP 부모를 구현한 것은 아니다. CD 지면은 실제 RawPathSurface.SurfaceAt으로 조회한다. 원본/golden/fixture는 변경하지 않았다.

추가12검사는 복귀 진단의 저장값/현재 재조회·함수 사본, 소리 뒤 미연결 생성 실패, CD signed HP/인접 바이트/진단 미조회/소리만의 경고, 비치명상 서버 미조회, 양판본의 현재VT/빌린 주소/fallback, 실제 burning 어댑터를 검증한다. 검사 작성 중 RawCheckingFlag의 throwing lambda를 명시 Func<uint>로 지정했고 RawPathPoint 입력은 실제 필드 초기화 API로 맞췄다. 제품/오라클 기대값 보정은 없다.

대표 변이는 비치명상 서버 선행 조회2/12실패와 CD HP DWORD 쓰기1/12실패로 검출했다. 각각 소스 바이트/SHA를 원복했다. 원복 뒤 전체 비증분 Release 빌드 경고0·오류0, 관련 Core99/99·Client23/23은 실패0·건너뜀0·각 종료0으로 통과했다. 이번 집중 결과를 최근 전체회귀0.529와 구별한다. 로컬 자료는 extracted/dotnet-damageable-hit-0533-mutations/와 extracted/dotnet-damageable-hit-0533-final/이다(Git제외).

## 남은 실제 조립

실제 Burning14 생성/시각/프레임/삭제와 공통 피해·삭제·Dais/Altar/총기 상위 몸체, Host의 실제 권한/장기역할 대여를 후속으로 연결한다. 이번 하위 모듈을 갱신한 것만으로 제품의 교전/월드 실행을 완료로 판정하지 않는다. 다음0.534는 같은 표시 등록에 연결할 burning 생성·입자·프레임 모듈 묶음이다.

PORT_MAP816행(L1 392+port424), fixture278개 유지. 새 외부 접속/NB 수정은 없다. AGENTS.md·보호 원본/CD·README 개요·cpppj·LEFT_JOBS.md·다른 프로젝트를 변경하지 않았다. 게임창·실제 사제 포획/희생·특정 적 공격·시나리오 승리·MP 재현은 하지 않았다. 전체 목표 active·M1전체0/34·후속 단계 잔여다.
