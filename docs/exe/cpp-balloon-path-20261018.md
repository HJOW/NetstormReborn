# 벌룬42의 이륙·착륙과 그림자 수명 (2026-10-18)

실제 벌룬111과 에어 쉽141의42 이동 생성/시작, 고도·화염/착륙과5040c0 그림자 수명을 같은 Session에 연결했다. 두 자산은 같은42b010 생성자/5040c0 가상 표를 사용한다. 원본 게임·OS·창을 실행하지 않았고 보호PE/아카이브는 읽기 전용으로 사용했다. 현재/마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-16**이며 기준커밋은e7d9ff9다.

## 실제 경로와 그림자

42의48be70/50e2b8/48acd0/488d00은 고도44의 이륙·수평·착륙 꼭짓점을 만든다. 기존39의 두 꼭짓점 Init과 append/현재0 쓰기 몸체를 공유한다. 48ad60은 별도의20ms 예약이나 사제 freeze를 추가하지 않고 현재Step 뒤의 단계/시각에서 고도를 계산한다. 남은 시각과 최소 구간 길이의float 저장, signed CRT절삭, 고도0의balloonFlame.wav, 같은좌표에서 고도 쓰기 생략과 효과 뒤 현재좌표/보존 고도를 유지한다. 완료count 슬롯과 최초단계-1 인접 시각도 물리 원문 범위 안에서 읽는다. Attach는 같은42 제거 후 실제Host에 부착하고 도착은 기존48b340/429800을 사용한다.

42ac90은 실제 그림자 생성/재귀Pop/현재WORD재독/프레임·좌표/깊이와Damageable 후처리를 실행한다. 42ad64~42ad9d의 고도 배율에는 중간float 저장이 없다. 두 좌표fstp에서만 반올림하도록 정적 근거로 교정했고 기존233원문을 보존한 별도256고도 관측을 추가했다. 마지막check는 최초flag를 보존한다. 첫C++그림자 대조의17실패는 디컴파일의 임시변수명을 합친 해석 오류였으며 원문/기대값을 유지한 채 원본 결과로 교정했다. 42ae80은 그림자를 먼저 가상삭제한 뒤 기존 실제Carrier 전처리의Damageable/현재boss/내용물/485e40을 사용한다. 일반Carrier/다른플라이어 지원을 대신 적용하지 않는다.

## 독립 관측과 실제 검사

- 경로688입력/2740최상위정상반환(생성1376/Init288/frame1076)/14,038,870명령. 그림자233입력/466정상반환/32,326명령. 별도고도256입력/512정상반환/51,836명령. 세 관측 모두53·64차이0이다. 전체경로100080BYTE/부모50BYTE 또는그림자150BYTE/현재전역·효과와GP/retN/canary/SEH/x87/선택범위를 대조했다. 각 관측의 큰 경계는 JSON에 명시한다.
- 관측기 초기화/Init의 실제ret14h·5인자/부모head7 입력을 바로잡는 초기 실패 로그를 보존했다. 원본 assertion을 성공으로 막지 않았다. 첫C++재생 전 세fixture를 각각 동결했고 기존233원문은 추가고도 관측과 별도로 유지했다.
- 실제1-1 Session 두 개에서 벌룬111/에어 쉽141의 첫그림자 생성→실제42 시작/직렬화→전체Kernel 이륙·고도·착륙→그림자 우선삭제를 검사했다. 실제Prepare 시각으로 연속FrameTime을 공급하고 현재고도/그림자깊이·프레임/SP/도착위치/수명·깊이를 읽었다. 결과 전체는 세션 JSON의integrationLog에 보존한다. owner1/좌표와 시작 직후 같은 시각의 첫Kernel tick은 명시입력이다.
- 최종Release goal-balloon-path-20261018-build4.log 경고0·오류0, 집중2개/실제통합1개, **전체1643개 실패0 / 211.68초**(CTest 211.72초). 감사86도구75통과/기존11실패집합동일/새0. 기존클론진입5PASS와추가11단계페이지진입PASS/originals_unchanged=true. 기존73보고서310현재핀만 갱신하고 역치환으로 과거전체JSON 보존을 확인한다. 첫build1의비공개Validate 접근 오류는 새로운부모의순수검사와상위공통ValidateBase로 해결했다.

## 목표11개 UI 진입 추가 확인

`python tools/cpp_goal_stage_entry_smoke.py --output extracted/goal-balloon-path-20261018-goal-pages3`의 실제11개 진입이 모두 통과했고 originals_unchanged=true다. Early1/3/4/5는MORE→OK, Early2는MORE×2→OK, Early6은MORE×3→OK, 캠페인두개와GuardMyBack/DissolvedAlliance는Play Mission을 사용한다. DemoStormRages는처음부터실제phase4였다. 현재 UI 보고서의 버튼만 선택하고 마지막 클릭 다음 프레임의phase4/paused0/소유자1사제/현재raw깊이0을검사한다. 알려진접두만새클론에서재생하며내부페이지/미션상태를직접쓰지않는다. 초기고정버튼/입력소비전보고/마지막OK누락의검사실패도별도폴더로보존했다.

실제세션의첫미연결은검사장치unitLost관찰콜백누락이었다. 기존Client연결(CampaignClientServices.cpp)을확인했고새검사에서부모한개통지를검증했다. SessionMissing진단에는최초연결위치를추가해실제미연결하위를구별했다.

## 남은 범위

이번검사는 로드된세션의 명시명령/좌표다. 사용자UI 선택/로그인/자연명령·전체왕복/11스테이지 완주를 증명하지 않는다. 실제38/41/187 전용Halt, Client명령소비, AI간헐천/전략·생산/경제와 비미션40e050, 전체11스테이지 자연플레이·승패·저장·다음미션/동일성이 남아 있다.

[세션 근거](../../cpppj/campaign-balloon-path.json), [경로 원문](../../cpppj/balloon-path.json), [그림자 원문](../../cpppj/balloon-body.json), [추가 고도 원문](../../cpppj/balloon-body-height.json). 로그 접두는extracted/goal-balloon-path-20261018-다. 인계 뒤 로컬커밋20261018-12 CP 벌룬의 이륙 착륙 경로와 그림자 수명 복원으로 기록하며 원격push하지 않는다.
