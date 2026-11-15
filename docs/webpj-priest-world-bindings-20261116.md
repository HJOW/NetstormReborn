# 사제 월드 바인딩 본체 이식 (WB04-E563)

`client/RawPriestWorldBindings.cpp/.h`(기준선 `225553df`, 합계 511줄)의 나머지 본체와
`RawPriestWorldState`를 `webpj/www/core/raw-priest-world-bindings.js`로 옮겼다.
이동 역할 선택은 기존 E351의 `raw-movement-process-types.js`를 그대로 공유한다.
기준선과 현행 cpppj 소스의 차이는 없다. cpppj 소스와 보호 원본은 수정하지 않았다.

Session 생성자가 Hit/Archer 연결 전에 이 몸체를 무조건 생성하므로 6단계에서 4-D C 구획으로
앞당겼다. 아직 앱이나 실제 Session에는 연결하지 않았다(PORT_MAP `partial` → `port`).

## 이식 범위

- 같은 Pool/TYPE/프레임/spot/hash0와 HP·생성·소유자·공통 후처리·프레임 지정기를 공유한다.
  `surfaces`는 `hash.view(0)` 자체여야 한다. 사본이나 다른 풀의 하위 모듈은 구성 전에 거부한다.
- 사제 상태·소유자·보호막 조회/생성 → 실제 J/E 낙하 프레임 → Carrier 지면 조회/후처리 →
  목록/회복 접두와 전체 사제 후처리 → Carrier/사제 삭제 준비 → 보호막 후처리/정기 →
  내용물 조회·회복 → 사제 생성 순서로 구성한다. 모두 성공한 뒤 공통 후처리에 등록한다.
- `bindProcesses`·`bindDamageable`·`bindPlacement`는 같은 풀의 첫 연결만 허용한다.
  `validateReady`는 첫 전투 틱 전에 세 연결을 검사한다. 삭제 목록을 바꾸기 전 하위 검증과
  실제 삭제 순서를 유지한다.
- 로딩 깊이·로컬 Player·독립 권한 setter, 같은 실제 boss의 네 소비자 대여,
  같은 Player9 물리 바이트 범위 대여를 옮겼다. getter와 내용물 뷰를 권한 owner별 WeakMap에
  보관해 다음 Session도 같은 Client 주소를 다시 빌린다. 다른 owner 교체는 거부한다.
- 사제 Regular는 실제 TYPE/VT를 대조하고 602 회복·603 낙하를 선택한다. 다른 고유 사건은
  fallback이 필요하다. 보호막은 원문 PATCH처럼 VT만 대조한다. TYPE 이름으로 분배하지 않는다.
- `damageableRefresh`는 free → 현재 TYPE 범위 → numeric `damageEffect` 순서로 조회한다.
  0이면 후속 장치를 요구하지 않고, 비영 효과는 원래 외부 burning 장치를 요청한다.
- Native 장기 몸체 네 종류(보호막 조회·Carrier·사제 삭제 준비·보호막 정기)는 명시 공급 시
  같은 풀을 검사해 그대로 빌린다. 실제 Storage가 이 몸체를 설치하는 상위 연결은 남았다.
- `dispose()`는 Kernel 정리 뒤 사제/보호막 후처리 등록과 Session Player9 대여만 반납한다.
  Client의 목록·권한·이동 역할 상태를 새로 초기화하지 않는다.

`ContainedFinderState.bossOwner`는 현재 공유 owner의 정체성을 검사하는 읽기 접근자다.
기존 검색/쓰기 규칙을 바꾸지 않는다.

## 검증

`webpj/tests/priest-world-support.mjs`는 cpppj `RawPriestWorldBindingsTests.cpp`의
PATCH `PriestWorld` 장면을 같은 raw 구성요소로 옮긴다. TYPE/프레임/SHP는 그 cpppj 장면의
합성 입력이며 기대값은 원문 검사의 숫자다. 실제 생성·Pop/Unpop·프레임·Form 분배·ProcessHost·
Kernel·삭제 장부·SP 저장소를 사용한다. 전송과 소리/UI는 외부 요청 관찰 장치다.

`raw-priest-world-bindings.test.mjs` 11건:

- cpppj PATCH 조립 검사 8건: fullHP/재Pop/회복 예약, 보호막/낙하/삭제,
  다른 풀·중복 연결·미복원 사건 거부, 사제 생성 원소유자 WORD,
  절반 HP 회복과 numeric damageEffect, 공유 boss 대여, Player9 대여 거부,
  현재 신전 DWORD를 읽는 실제 회복과 보호막 삭제.
- 웹 계약 3건: 필수 효과·하위 풀·장기 몸체·미연결 지연 장치,
  같은 장기 몸체 대여/소유자 우회/현재 TYPE/정리, 다음 Session 권한 재대여.

전용 원본 x86 조립 fixture는 없다. 기존 E351 `pathprocesstype-x86.tsv`의 PATCH 376행은
`raw-path-start.test.mjs`에서 계속 대조한다(SHA-256
`4a5da6130b1c840c4a6387efc64671f535c46251ccc529510f5386d0e9bcbb6f`).
cpppj의 별도 이동 선택 fixture 조립은 그 기존 대조와 중복이라 새 복사본을 만들지 않았다.
타입 로더 numeric damageEffect 검사는 기존 `type-record-complete` 범위다.

최초 장면 구성 중 `SquidPop`의 실제 전송 미연결 오류가 발생했다. C++ 독립 검사와 웹 Pop의
장치 연결 경계 차이여서 시험 장면에 전송 요청 관찰기를 연결했다. 기대값은 바꾸지 않았다.
추가 검토에서 SHP 크기 반환이 배열임을 확인해 배열 인자로 고쳤고, 받침 surface 플래그를
원문 `0x800`으로 맞췄다. 첫 집중 검사 10건과 다음 Session 대여 보강 뒤 11건 모두 통과했다.

집중 실행의 모듈 범위 coverage: 줄 99.61%, 분기 94.85%, 함수 88.57%.
미도달 선택 장치/범위 거부 일부는 실제 게임 관측으로 세지 않는다.
전체 Node v22.23.3·v26.13.0 각각 **5,305건 통과, 실패·skip 0**. 관련 사제/이동 선택 검사 26건, 대응표 검사 7건, manifest 검사 3건도 통과했다. 대응표 도구 오류 0이다.
검사 로그는 extracted/wb-e563-node22.log·wb-e563-node26.log·wb-e563-related.log에 있다(Git 제외). Node v24는 이번에 실행하지 않았으며 마지막 전체 검사는 E513의 4,973건이다.

## 남은 경계와 다음 시작점

- 실제 Client/Storage 장기 사제 상태와 기본 몸체 설치, Session 생성자·Load 연결은 4-D D·E 구획이다.
- `tests/CampaignInstallSupport.h`의 전체 설치 장면은 Hit·Archer 등 선행 바인딩이 더 필요하다.
  기존 `campaign-asset-support.mjs`의 자산 전용 장면을 완성된 전체 InstallScene으로 부르지 않는다.
- 다음은 4-D C 구획 `RawCampaignHitBindings`, 이어서 생성자가 요구하는 Archer·Motion·Path 등이다.
- 브라우저/오라클 실행·L2·실제 미션·자연 완주 검사는 이번 단위에서 수행하지 않았다.
