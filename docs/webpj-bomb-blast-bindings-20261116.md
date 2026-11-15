# 폭발 효과·추가 그림 공용 바인딩 (WB04-E583)

`225553df` RawBombBlastBindings.cpp/.h 전체45줄을 이식했다. Session1427 이후의 파편
캡처가 사용하는 DrawDevastationAdditional이 미이식이라 0.5절 선행 의존성 규칙에 따라
순서7에서 앞당겼다. Session 생성자 자체의 다음 시작점은 계속1427이다.

## 이식 범위

`drawDevastationAdditional`은 원문41e3d0을 공유하는16VT만 받는다. 현재 raw TYPE·전체
signed 프레임 DWORD·입력 signed x/y를 평문 Draw 소비자에 전달한다. free/dead/abstract나
프레임 부호 필터를 추가하지 않는다. 이 함수의 증거는 그림 요청이며 RLE 전체 픽셀 출력은
호출자의 기존 VFX Draw/ShapeToBuffer 검증 범위다.

`makeCampaignBombBlastHooks`는 같은 World/Hit/BombTargets Pool을 구성 때 검사한다.
반경은502370 표만 실제 현재 DefaultRadius에 전달한다. 피해는 같은 가상98 분배의 함수를
값으로 보관하고, 삭제는 같은 World.Destroy의 bool 반환을 원문대로 무시한다. 지면 효과는
같은 Hit.Impact.Ground를, 끝 칸은 같은 Core.BridgeEvents.ConvertEnd를 호출한다.
큰 피해/지면/다리 몸체를 다시 구현하거나 별도 월드를 만들지 않는다.

구성 시 피해 함수는 값 캡처하고 World/Hit/좌표 및 실제 BridgeEvents 객체는 빌린다.
대상들의 수명은 반환 콜백보다 길어야 한다. TYPE/프레임/좌표를 전달할 때 C++ int32,
DWORD/BYTE/float 인수 폭을 유지한다. 원문이 무시하는 table 인자를 새 자격 관문으로
추가하지 않는다.

## 근거와 검사

CPP 소스/헤더는 기준선 이후 HEAD 변경0이다. CPP 수정은 없다. CampaignSessionTests의
CampaignDevastation_AdditionalDrawNative32와 ClientRegionOwnerTests의 현재 주문 추가
그림 전달 계약을 읽었다. 기존 RawBombBlast/BombTargets의 원본 자격/효과 관측과 실제
Hit 계약은 집중/전체 회귀에서 유지한다.

기존 원본 fixture `cpppj/tests/fixtures/devastation-draw-x86.tsv`32행(53/64 결과)을 처음
웹 검사와 manifest에 연결했다. 새 원본 실행/관측은 없다. LF 정규화 SHA-256은
`db3d3d73bce0f29c91e6b97976cfafb66efb74a8e3de2af9a91ff6155b26d8f1`,3,030BYTE·33줄이다.
fixture 마지막 변경은380cb7dd2, CPP 근거 마지막 변경은 cpp44a48b1e9/h380cb7dd2다.
추적 목록은804항목(original-x86766·screenshot13·source10·independent9·cpppj-oracle5·
original-binary1)이다. `extracted/wb-e583-manifest.log`는 Git 제외다.

신규9검사는 다음을 확인한다.

- 원본32입력의 현재 TYPE/전체 프레임/좌표와53/64 trace 전체 일치.
- 정확한16VT·현재 TYPE/signed 프레임·free/dead 무필터와 미지원/누락 Draw 오류.
- 같은 실제 World/Hit/좌표 Pool 계약과 다른 소유자 거부.
- 실제 기본 반경의 현재 float 표/정수화와502370 분배 관문.
- 실제 가상98의 같은 HP 변경과 원문이 무시하는 table 인자.
- 실제 공유 Game 난수 진행과 같은 Impact의 충돌 효과 억제.
- 실제3×3 받침 Graph의 hard 다리 삭제 false와 반환값 무시/상태 보존.
- 같은 실제 BridgeEvents로 float32 인자 전달과 원문 비J/K 방향의 자연 반환.
- 기존 RawBombBlast.Apply가 실제 HP1,000 감소·지면 효과·공유 난수로 이어지는 전체 체인.

최초9검사는7통과/2실패였다. 삭제 검사는 Graph 표면5개 이상이라는 원문 거부 조건을
빠뜨렸다. 현재 hard 프레임만 고쳐도 실패했고, 실제3×3 noIsland 받침을 적재해 조건을
충족한 뒤 통과했다. 끝 칸 검사는 장기 World 권한 변경이 독립 typed BridgeEventState의
초기 true까지 바꾼다고 가정했다. CPP도 두 상태를 구별하므로 실제 다리/기존 방향의 자연
반환을 선택해 같은 이벤트 객체 호출을 검증했다. 제품 몸체나 단언은 약화하지 않았다.

Node22(v22.23.3)·Node26(v26.13.0)의 집중27검사는 모두 통과했다. 신규 모듈 줄100%·
분기94.12%·함수100%다. 실제 Hit가 반환하는 피해 함수를 가짜 빈 함수로 바꾸어 누락
방어 분기를 억지 실행하지 않았다. 로그 `extracted/wb-e583-focus3-node22.log`·
`extracted/wb-e583-focus3-node26.log`는 Git 제외다.

Node22·Node26 전체 각각5,548건이 통과했고 실패·취소·건너뜀은0이다. 전체 로그는
`extracted/wb-e583-node22.log`·`extracted/wb-e583-node26.log`다(Git 제외).
`node webpj/tools/port-map.mjs`는1,160행·오류0이다(`extracted/wb-e583-port-map-pre.log`).
이식 상태는 port842·partial34·todo130·
제외154이며 client port300/40,503줄·전체 port111,687줄이다. 현재4-D는 선행 항목 추가로
29/44이며(보관 완료13개 포함, 현행 체크 목록16/31) 전체 Session은 partial·session/oracle0이다.

World/HP/좌표는 실제 보호 자산으로 구성한 기존 owner를 사용한다. 고정 수치 입력의 소스
계약 검사이며 원본/클론 게임·브라우저·오라클·디컴파일·실제 주문 시전·공중3종 전용
새 생성/행동·새 원격 멀티플레이 실행은 하지 않았다.

## 다음 시작점

1427의 소유자색 표·BridgeDraw/ShapeToBuffer 파편 캡처·CampaignParticles/EvilRay 장치를
계속 연결한다. 웹 GameAssets에 실제 GamePalette 연결이 없어 기존 팔레트 로더와 같은
자산 소유 관계도 보완해야 한다. 전체 Session 모듈·Load/로그인/프레임과 앱 연결은 잔여다.
