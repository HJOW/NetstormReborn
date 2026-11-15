# dotnetpj 0.564 — 현재 Gun 계열과 진단 복귀

2026-11-15, HJOW-Athlon. cpppj 제품1313의 제네레이터·Gun4종·Gun 장식 현재 본문을 맞췄다. R4-1 진행 중이며 M1은0/34, 새 경로는 플레이할 수 없다.

## 구현

PATCH의 이식한 부모는 전체 사전 검사 대신 원문이 도달한 장치만 요구한다. 제네레이터는 최초 현재 theme을 Damageable보다 먼저 읽는다. 각도 Gun은 현재 각도→실제 프레임 조회→그래프 설정 뒤 현재 서버/extra를 읽는다. 썬 디스크 발사기는 제거 효과가 권한을 바꾸어도 이미 선택한 생성 분기를 취소하지 않는다. 장식은 Regular 확보 실패 때 생성자를 요구하지 않는다.

생성 뒤 현재 로딩·편집·checking DWORD를 읽고 원문 Gunprocess.cpp 식/행을 그대로 전달한다. checkForms는 실제 bool 판정이며 실패 때 이미 선택한 진단은 getter가 checking을 꺼도 유지한다. returning assertion은 후속 현재 로딩/TYPE/HP를 읽어 진행한다. 캐논은 최초 아님에도 마지막 TYPE 접근을 유지하며 DWORD 백업과 WORD 저장/현재 다리 통지는 같은 물리 슬롯에서 실행한다. 콜백이 free로 바꿔도 앞선 물리 쓰기를 취소하지 않는다.

RawCheckingFlag로 실제 전체 DWORD를 연결하며 독립 상태도 지원한다. hook은0.563의 함수 값 사본을 유지한다. Vortex 현재 본문·Native 장기 자원 설치·실제 Client 자산뷰/호스트 전환은 잔여다. RawAssetPostPop 전체 기준은 올리지 않았다.

## 검증

최종 **Core306/306·Client120/120**. 원본 `gun-postpop-checking-x86.tsv` **1,344행**을 읽기 전용으로 연결하여 생성/제거 시점 전역 변경, checking 판정/실패 진단, 진단 중단/복귀와 최종 raw·전역을 대조했다. 기존 자산818행 직접/가상/Player 기록 대조도 유지했다. 추가 현재 진입 검사는 미최초/잘못된 TYPE의 미도달 장치, getter/setter·생성 누락 시 앞선 효과, 공유 권한 재조회/free, extra/비서버 관문, 선택 생성 유지, 장식 예약 실패, 현재 다리 WORD 통지/복귀, 생성 뒤 현재 로딩/HP 백업, 선택 진단 누락을 다룬다.

자료는 `extracted/dotnet-asset-gun-0564-final/`·`-client/`와 초기 로그다. 전체 회귀 기준은0.546을 유지하고0.566에 다음 회귀를 수행한다. 사용자 제외 플레이와 MP 실행은 하지 않았다.

L2는 cpp34 SHA 확인 재사용·dn일반31 신규 실행 종료0·수집 오류0·원본1,394파일 전후 동일이다. 0.563 대비 상태 차이0, 파일 전체 동일9·frame행만 차이22경로다. **일치0/34·데모dn3 미연결**이다. 자료 `extracted/dotnet-l2-0564/`.

PORT_MAP949행(L1437·port512), 대응cpp416단위, fixture314개다. 부분 대응2행과 진단fixture1개를 추가했고 기존 fixture 기대값/해시·순서와 floor/reflected는 유지했다. R4 누락137단위/13,873줄·R6 누락382단위/45,090줄은 그대로다. 다음은 Vortex 현재 본문과 Native 장기 조립이다.
