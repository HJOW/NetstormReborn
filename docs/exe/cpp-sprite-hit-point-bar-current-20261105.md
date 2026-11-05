# 선택 HP 막대의 현재 조회 순서와 공통 getter 연결 (단위901)

2026-11-05 HJOW-Athlon에서 작업했다. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다. 전체 cpppj 목표는 진행 중이다.

## 원문과 실제 표시 연결

PATCH00498220의 선택 막대는 같은 저장 TYPE this를0049fce0으로 **세 번** 읽는다. 첫 현재 HP를 읽은 뒤 첫 최대HP/2와 비교해 절반 색을 선택하고, 현재 HP를 다시 읽은 뒤 두번째 최대HP/4와 비교해1/4 색을 선택한다. 세번째 현재 HP를 읽은 뒤 세번째 최대HP를 조회해 막대004972f0의 인자로 전달한다. 현재 HP는 각 getter 호출 **전**에 저장한다. 정상 색은 첫 getter 전, 절반/1/4 색은 해당 getter 뒤의 현재 색 전역에서 읽는다. signed 나눗셈은 음수도0 방향으로 절삭한다.

CD00455f60은 첫00444720 결과가1 이상일 때만 getter를 한 번 더 호출해 두번째 결과를 최대HP로 사용한다. 첫 결과가0/음수면 최대값1이다. 이후 signed WORD HP를 읽고 원문 분기로 clamp하며 정상/절반/1/4 색은 getter들 뒤에 읽는다. 기존 단일 최대HP 캐시는 이 호출 순서를 표현하지 못했다.

새 `ReadSpriteHitPointBar`가 두 부모의 순서를 보관하며 실제 `SpriteDraw::Selection`이 호출한다. HP가 없는 분기에서는 조회를 추가하지 않는다. CD에서 첫 getter는 양수지만 두번째가0인 명시 장치 입력이면, 원문 나눗셈 실패 위치에 대응해 배경 fill 뒤 오류를 보고한다. 성공 최대값1로 바꾸거나 C++의0 나눗셈을 실행하지 않는다. 색/HP/최대값 조회 뒤 픽셀 기하는 기존 판본별 경로를 사용한다.

실제 `RawCampaignFactoryDisplay`는 공장의 `ReadSelectedHitPoints`에 저장한 TYPE this/물리 표 기준을 전달한다. 같은 현재 checking·진단·GameAux의 사제/dais/약화 전역을 사용하며 진단이 raw TYPE를 바꿔도 새 TYPE를 다시 선택하지 않는다. 기존 독립 fixture의 미연결 getter만 기존 `quarterHitPoints`/`shadowAliasType` 입력을 사용하고 동일한 부모 순서를 거친다.

전송 maxHp의158 고정 계산도 같은 공장 getter에 연결했다. 전송기는 같은 World 구현 안에서 Factory 뒤에 선언되어 먼저 파괴되고, 콜백은 같은 구현의 현재 TYPE 표를 조회한다. 이 변경은 원문 근거의 정적 연결이며 **신규 MP 기능/단위/패킷/통신 검사를 실행하지 않았다**.

## 독립 관측과 범위

[관측 도구](../../tools/decomp_sprite_hit_point_bar.py)는 각 판본의 정밀 함수 범위를 엄격 허용하고 내부 명령 경계에서 실행한다. PATCH49898c에서 시작해4972f0 호출 인자에서 정지하고, CD456789에서 시작해45680d의 기하 이전 결과에서 정지한다. 원문 부모의 signed 비교/HP 읽기/색 선택과 저장 TYPE this의 getter 호출을 실행한다.

각판본252·합계 **504행**의 최초 관측과 읽기 전용 재관측이 동일했다. INT_MIN/음수/0/양수/INT_MAX, 서로 다른 세 최대값, getter 중 raw HP 변경, 현재 색 전역 변경, CD의1~2회 조건과 WORD 해석을 구별한다. 기대값은 C++ 코드로 만들지 않았다. 최대HP 자식의 반환/정상 복귀와 HP·색 변경은 명시 장치 경계다. HP 존재 분기의 앞단·전체 Draw/실제 getter 몸체·막대 기하/픽셀 출력을 이 새 관측만으로 검증했다고 세지 않는다. 실제 getter는 기존1160행, 전체 표시 출력은 기존 동결 SpriteDraw fixture와 구별한다.

Release 제품/집중 빌드1명령은 종료0·경고/오류0이다. 소스/EOL 정리 완료→2,855소스 snapshot 저장→CMake를 같은 명령에서 순차 실행했고, 빌드 중 소스 편집/다른 빌드 시작은 없었다. 최종 소스는 빌드 전 snapshot과 같다.

새5/기존25의 **고유30사례·4명령/30실행이 첫 실행부터 실패0**이다. 새504행 전부, 실제 Draw/공장 getter의 세 진단과 저장 TYPE·HP/색 변경, CD 조건부 재조회, HP 없는 분기와 나눗셈 실패 전 출력, 미연결 선택 장치를 검사했다. 기존 SpriteDraw14/공유 getter5/공통 PostCreate6을 함께 실행했다. 기존 SpriteDraw의 세 판본별 각463행은 전체 기존 출력/슬롯 관측과 같았다.

기존 보호2,777개/957fixture가 불변이고 새fixture1개로 다음 기준은 **2,778개/958fixture**다. 기존 입력/기대값은 수정하지 않았다. 원시 로그/소스 snapshot/보호 해시/원문 C·ASM은 `extracted/cpp-resume-901/`(Git 제외·현재 PC 전용)에 보존한다. 커밋 증거는 [JSON](../../cpppj/sprite-hit-point-bar-current-20261105.json)이다. 활성74프로젝트/과거 전체·GUI·34미션·ZIP 기준은 유지한다.

제품은 **901 / 표시·파일·제품11.01**,10,719,744바이트, SHA-256 `38196e0e8a2da56403a8a8e8e28c237b288890866fa0f85f27f090cec32dbdd1`다.

## 다음 작업

RawBurningSources와 월드 설치 전 Help 등 다른 HP caller를 이어서 대조한다. 연소점의 현재 TYPE 번호/약화/진단은 장기 owner를 빌려야 하며 Session 지역 보상 객체를 수명 밖에서 캡처하지 않는다. CD Take/contained/Form·State/파일·객체UI/Edit·Windows·무검사MP와4~8차도 전체 목표에 남아 있다. checking8(부모3/부분2/MP3), 완료부모39/부분4를 유지하며 전체 HP/Factory/Pop 완료로 세지 않는다.
