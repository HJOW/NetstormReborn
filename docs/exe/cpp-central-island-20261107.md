# 990 — 중앙섬 생성과 metadata 수명

2026-11-07, HJOW-Athlon. 원문은 같은 호스트의 2026-11-03 정밀 디컴파일과 보호 PE를 읽기 전용으로 대조했다. 신규 MP/단위/게임/통신 검사는 사용자 지시대로 제외했다. 제품990의 Release 빌드는 종료0·경고/오류0이다.

## 구현하는 원문 범위

- `004b3bc0` metadata: Zone bit100이 없으면 기존 로그 뒤 반환한다. `$` 이름이면 실제 Creating Islands. 안내→로그→현재 강제 밀도 쓰기→현재 layout 조회 순서다. layout2는 고정 `\D\SURROUNDED.FORT` 지역 Template의 전체 배치/지형/Objects 뒤 centralCreated를 쓰지 않고 반환한다. 그 외는 값형 Zone6BYTE의 실제 일반 중앙섬을 만든 뒤 centralCreated1/현재 predictableCount 로그를 수행한다.
- `0040b110`: 송신에서 실제 DWORD len==-1 진단 뒤 현재 낮은WORD+10을 헤더에 쓴다. 실제 패킷을 빌리는 ObservePacket으로 등록하여 옛 인자 전용 observer의 송신 예외를 제거한다. 수신 방향은 원문 무동작이다.
- `0046e140`: 전투/위치BYTE/기존섬수/owner0 진단, 실제 원점 배치 범위, 장기 중심 X/Y 쓰기, 패턴/지형, 조건부 거주 구역, 군도 분리, Graph0을 같은 현재 월드에 연결한다. **PE의00540a30은127**이며 builder owner0으로 대체하지 않는다. 현재 invalidRegion 참조를 사용한다.
- `0046f7d0/0046ed50/0046ec00`: y행/x열로 현재 지면 TYPE와 지역0을 찾는다. 새 섬 기록은 기존 Add(0,ff) 필드만 쓰고, Native `_ftol`의 실제 raw X/Y를 시작점으로 북/동/남/서 연결 flood를 수행한다. 프레임 코드와 TYPE를 현재 조회하고 같은 raw 지역 getter/setter·bounds·worldRedraw를 쓴다. 호스트 작업 스택으로 원문 깊이 우선 순서를 유지한다. PE의 `_ftol` 호출 전 FPU 입력을 확인해 디컴파일의 잘못된 정수 인자 표현을 대체했다.
- `005424b0`: Login 지역 사본0을 제거하고 실제 SidPool의 예측 커서를 참조한다. 원문 Reset `004abb10`의 초기1 및 같은 할당 증가와 직접 공유한다. 중앙섬 로그/Player metadata/Fort packet도 같은 현재 값을 읽는다.
- 지역 SURROUNDED Template는 전역9헤더를 교체하지 않으며 같은 serializationVersion/이름표 owner를 빌린다. 기존 Native File Load와 원시 Territory/TypeNames/Objects reader를 사용한다. 생성 월드/공유 version을 되돌리지 않고 지역 manager만 호스트 수명 끝에서 반납한다. 원본 파일의 삭제 요청은 기존 보호 장치에서 거부한다.

## 빌드·한계

Release 제품은11.01, 11,621,888바이트이며 SHA-256은 `aba1b54593aa66e48ba704079b131a165478e5c19258013e87a8347d727572f5`다. 빌드 로그는 `extracted/cpp-resume-990/build.log`, 정적 PE 대조533명령은 같은 폴더의 `x86.txt`, 제품 보고서는 `cpppj/central-island-20261107.json`이다. 보호 기준 AGENTS.md·원본/CD·fixture2800파일은 모두 불변이고 신규 fixture는 없다.

정적 대조/컴파일을 LAN 동작/안정성 검증으로 표현하지 않는다. ordinary 자기 Fort, 일반 State/Commander/roster/LAN UI, 전체 파생 writer/decoder, 메타/저장/재접속/Outpost/checking8은 별도 잔여다. 현재 제품의 GUI/ZIP/전체34경로/전체검사는 반복하지 않았다.
