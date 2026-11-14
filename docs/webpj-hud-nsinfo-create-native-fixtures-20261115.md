# webpj NSInfo 부모 생성 원본 관측 연결 (WB04-E493, 2026-11-15)

3.8절 이월 목록의 `hud-nsinfo-create`를 원본 10.78 독립 x86 관측 **170행**(85입력 × 두 정밀도)에 대조했다. 170행 모두 첫 실행에서 일치했고 이식 결함은 나오지 않았다. `PORT_MAP`과 모듈 머리의 "전용 PE fixture 없음/cpppj에도 직접 시험이 없다"는 오기였다. E488 에서 후속으로 남겼던 단위다.

## 비교한 것

`tests/hud-nsinfo-create-native-fixtures.test.mjs`가 cpppj `HudNsInfoCreateTests`의 입력 구성을 옮긴다. 앞 10열(입력)만 장치에 공급하고 뒤 8열(관측)은 비교에만 쓴다.

- **다섯 생성**: 숨김 생성(`createHidden`, 0xd8 할당부터)·`construct`(NSInfo)·`constructPlayer`(PlayerInfo)·`constructBattle`(BattleInfo)·`constructButton`(특수 버튼 16×15). 숨김/NSInfo 전체 생성은 50개 할당 블록(배경·명단·제목·구분선·전투 정보·플레이어 8개와 각 자식)을 만든다.
- **장치 틀**: 한 원문 할당의 raw 0x180 바이트와 형별 파생 저장소(기본/NSInfo/PlayerInfo/BattleInfo/버튼)를 같이 소유하고, 이벤트 경계마다 서로 내보내고 읽어(`export`/`import`) 같은 물리처럼 다룬다. 입력의 명시 변경(호출 번호별 `트리거/주소/값`)은 노드 raw·번역 문자열 512바이트·13개 전역 중 한 곳의 같은 물리 메모리만 바꾼다. 원문 `new` 실패 호출 번호, 번역 입력(형식 문자열·원격 서버 이름), 종류 TYPE 결과, 그리고 초기 `fill` 바이트도 입력이다.
- **비교**: 완료 여부와 반환 토큰, 13개 전역, 모든 할당 블록(토큰:요청 폭:raw 0x180 바이트 전체), 번역·서버 문자열 전체 바이트와 형식화 결과, 호출 순서 문자열(`alloc`·`base`·`background`·`scroll`·`text`·`buttonBase`·`stone`·`scrollable`·`slider`·`wrap`·`parse*`·`fit*`·`attach`·`baseAttach`·`translate`·`type`·`remote`·`format`·`assert`·`meta`), 스택 정리량(버튼 12·그 밖 4). 부분 중단 50행에서 이미 쓴 효과가 되돌아가지 않는 순서도 확인된다.

## 변형 확인

자동 변형 131개 중 103개가 잡히고 28개가 생존했다. 기존 `ns-info` 시험과 합치면 2개가 남고 둘 다 동등 변형이다: NSInfo/BattleInfo 생성 끝의 `flags | 0x800`을 `0x801`로 바꾼 것으로, 최하위 비트(표시)는 바로 앞 기본 생성(`baseConstruct` flags 1)이 이미 켜 둔다. 나머지 26개는 노드 클래스 필드 기본값(raw 로 모두 설치)과 할당 실패 분기이며 기존 시험이 잡는다.

## 남은 것

3.8절의 HUD 쪽 남은 항목은 `minimap-blit`(122행)뿐이다.
