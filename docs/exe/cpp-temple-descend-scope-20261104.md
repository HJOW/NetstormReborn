# 신전 event4의 fort territory·curServer·Descend 범위 정정

단위866 정적 분류 기록이다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**이며 원본과 정밀 산출물을 변경하지 않았다. 직전 커밋은 `238714d3 / 20261104-53 CP`이고 **제품은865/11.01 그대로**다. 제품 소스/빌드/기능·Native/게임/MP 검사는0이다. 이 기록은 함수 구현 완료가 아니다.

## 확인한 실제 경로

PATCH `004dc220`의 사건4는 count0에서 argument4를 쓰고, checking0/count<=4이면1초를 반환한다. 그 뒤 `0046f650`의 반환과 `0054db4c<<16`을 OR한 값을 두 인자에 똑같이 넣어 `00452fe0(00542744,token,token)`으로 제출하고 argument0을 쓴다. 기존 `RawAssetRegular`의 임시 이름만 보면 이를 지역 세대/애니메이션 삭제로 오인할 수 있다.

실제 의미는 다음과 같다.

| 원문 위치 | 확인한 내용 | 기존 임시 이름의 한계 |
| --- | --- | --- |
| `0054db4c` | **curServer**. `ClientMain::InterpretOptions`가 ReadInt로 갱신하고 `ClientNsInfoGlobals::serverSelection`/NSInfo reader가 같은 주소로 다룬다 | `regionGeneration`은 맵 세대가 아니다 |
| `0046f650`→`0046f5a0` | float 위치를 기존 정수 helper로 바꾸고, **inFortMode**를 요구한다. 현재 지면 WORD/SID의 island argument→exists 검사를 거쳐 섬 기록+0x10의 **signed territory BYTE**를 반환한다 | 일반 청크의 island BYTE나 단순 지역번호 조회로 대체할 수 없다 |
| `00542744` | PE 파일의 DWORD **0x02000013**. UserInput의 서버 범위 판정 뒤 `004b3760`으로 간다 | `animationType`/`removeAnimation`은 실제 소비자 이름이 아니다 |
| `004b3760`→`004b3620` | fort Descend 준비(territory/Template State/청크 저장)를 거쳐 State **19(0x13)**와 현재 서버를 예약한다 | 일반 애니메이션 삭제 콜백에 연결하면 원문과 달라진다 |

`004b3620`은 inFortMode와 유효 territory를 요구하고 read-only fort에서 Descend 경고를 기록한다. 기존 LEFT_JOBS가 미완료로 둔 메타맵 State19·Descend 부모와 같은 범위다. 외부 공식 서버를 다시 연결하라는 뜻이 아니며, 후속 구현도 로컬 TCP/IP/외부 검증 비활성화 요구를 따른다.

## 사건4의 생성 부모와 분류 한계

이번 정밀 C에서 literal event4를 `00496e80`으로 만드는 직접 호출은 `004dbac0` 한 곳을 검출했다. 실제 Vortex VT `005196f0`의 +0x6c가 이 함수이며, 현재 argument0이고 명령이 `00542990`(PE DWORD **0x030000a7**)이면 new40 성공 시 Regular4/payload0을 만든다. 같은 함수의 별도 `005428e8=0x0300007c` 가지는 battle Ascend packet9 요청이다. 두 요청을 혼동하지 않는다.

이 결과를 **단독 모드의 모든 도달이 불가능하다는 증명으로 확대하지 않는다.** `004dbac0`의 사건4 가지와 `004dc220` 자체에는 직접 inSingle 관문이 없다. 동적 Regular 생성/강제 명령/예외 상태도 이번 직접 호출 검색의 범위 밖이다. 실제 fort/메타맵의 큰 부모·입력 분배·State19가 필요한 경로라는 근거로 LEFT_JOBS 3절8번에 미완료 이월한다. 단독 Session에 임의 성공 반환·청크 island 조회·애니메이션 삭제를 연결하지 않는다.

CD 정밀 대응은 Handle `0044efd0`, float/정수 territory helper `004bdcf0`/`004bdc80`, Descend 예약/준비 `0042ffd0`/`0042feb0`를 함께 보존했다. 이번 범위는 현재 정밀 본문/PE의 정적 대조이며 CD 실행 대조가 아니다.

## 제품과 검증 범위

제품/기존 독립 fixture API를 바꾸지 않았다. 과거 `AssetRegular` fixture의 경계 관측과 bool checking 재생은 독립 부품 근거이며, 실제 Session의 Descend 연결 완료로 세지 않는다. checking 현재 뷰 자체는 이미 연결돼 있지만 실제 소비자/현재 curServer·fort territory·명령/State19 부모는 미완료다.

checking 집계 **12=부모6+부분3+MP분류3·완료부모36/부분3**은 유지한다. 기존 부모 범주의 `004a7510`과 이번 `004dc220`, 기존 부분 `004b54e0/004b8cb0`은 같은 집계 안에서 8번에 이월된 것이므로 MP분류3에 다시 더하지 않는다. 남은 함수·부모 전체를 완료로 체크하지 않는다.

보호2,761개/fixture941개는 불변이다. `0054db4c`는 PE 파일에 없는 .data BSS 영역이므로 파일 DWORD로 읽지 않고 섹션의 가상 zero 초기화로 구별했다. 세 명령 DWORD는 실제 파일 바이트다. 제품 EXE SHA-256은865의 `a7931f4b7aaa471744af77d5c171c5dc50bd4f9a0d7f4d59bb46923bc59ba8e4`(10,586,624바이트) 그대로다. 원문/정적 호출자·PE 값은 Git 제외 `extracted/cpp-resume-866/`, 상세 해시는 [검증 JSON](../../cpppj/temple-descend-scope-20261104.json)에 있다.

다음867 첫 대조 대상은 비멀티 Missile `00482340`의 현재 checking/두 isRobust 호출과 실제 Host/진단 소비자다. 다른 TYPE 번호 사본·Teleport/가상20/전체 Pop·군도/Factory/State/파일과 LEFT_JOBS 나머지는 유지한다. 전체 목표는 active다.
