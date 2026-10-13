# Burning 입자 추가와 만료칸 선택

- 분석·마지막 정밀 디컴파일 호스트: **HJOW-Athlon, 2026-10-14**.
- 목표10.78와 CD10.72의 실제 비교 몸체를 각각 직접 실행했다.
- 새 독립 관찰은 **564정상(282×2), assert0, 외부관찰경계0**이다. 앞 생성자256과 서로 다른 입력/몸체의 자료다.
- 기존 생성자 source/report/tool/fixture는 변경하지 않았다. **단위12 최종 공유 C++2검사/564행이 모두 통과했다.** 최종 Release 경고/오류0·고유집중42·전체1056/실패0이며 상세 인수인계 뒤 `db9b626`(`20261014-04`)로 커밋했다. 원본 보고서는 관찰 당시의 SHA/공유 검사 전 상태를 그대로 보존한다.

## 원본 몸체

실제 Burning의6필드 추가는 `00423e80/CD004a60a0`이며 공통 `Particle.New`인 `00487b40/CD0045cc00`을 부른다. 현재 total이 capacity보다 작으면 앞의 아직쓰지않은 칸을 반환하고 total을1 증가시킨다. 이미 가득찼으면 현재 앞쪽부터 만료칸을 찾고 total은 그대로 둔다. 반환된 칸에 x/y·velocityX/Y·life·source6개의 DWORD를 순서대로 쓴다. 칸이없으면 자연스럽게 실패한다.

패치10.78은 **ordered float life≤0**에서만 재사용한다. NaN은 ordered 비교를 통과하지 않는다. CD10.72는 같은 DWORD를 **signed int≤0**로 검사한다. 따라서 positive NaN은 건너뛰고 negative NaN은 재사용한다. 양수/0/음수와 양·음 quiet NaN 비트를 실제 원본 명령으로 관찰했다.

`RawBurningVisual::Append`는 같은 두 분기를 수행하고 현재6필드를 저장한다. 현재 C++ source가 원본 정상입력과 같은지 확인하는 소비자 검사에서는 생성/부착/추가확보가 호출되면 명시 예외를 낸다. 실제 원본 실행에서도 heap/Host/캡처/파일/운영체제 호출은 없다.

## 격자와 의미 경계

용량0/1/3, 현재total0..capacity, 모두양수/첫칸 또는마지막칸의0·음수·양/음NaN, 새입자life0.5/−2/NaN 및 source0/1을 교차했다. label이 아닌 실제 수명메모리가 같은 중복 프로파일을 제거하여 각판본282개다. 기존입자 좌표/속도는 명시 입력이며 첫만료칸/성공/결과count를 입력 생성기에서 계산하지 않는다.

source는 원본 this+74인 고정연소점 또는null만 공급한다. 전체24바이트 출력은 해당 두 포인터만0/1로 정규화한다. 다른임의source포인터의 의미/수명은 관찰하지 않았다. source가같은지 판정하기위해 실제주소를 호스트포인터로 호출하지 않는다.

기록은 actual 성공여부·현재total·capacity개 입자 전체6DWORD다. C++는 호스트 bool의padding을 읽지 않고 source만0/1로 직렬화한다. 실제캡처·연소점선택·particle운동/frame·색표/화면표시·효과삭제·전체전투/캠페인은 이격자의 완료 범위가 아니다.

## 자료

- 기존 생성자/입자추가 구현: `cpppj/src/o/RawBurningVisual.h/.cpp`.
- 새 `cpppj/tests/BurningParticleTests.cpp`2검사, macro `NETSTORM_BURNINGPARTICLE_FIXTURE`.
- `tools/decomp_burningparticle_oracle.py`, `cpppj/tests/fixtures/burningparticle-x86.tsv`, `cpppj/recovery-burningparticle-evidence.json`.
- `python -X utf8 tools/decomp_burningparticle_oracle.py --verify`의564행/현재전체SHA/고유입력/실제RET/외부경계0 감사는 성공했다.
- 부모 생성자자료의 `decomp_burningprocess_oracle.py --verify`도 기존256행 그대로 성공했다. 두 fixture의 입력/출력·source와 실제export/PE SHA를 각각 보존한다.

후속은 첫팔레트와 실제 피해프레임 캡처/연소점선택/particle운동 및 같은 Host 소유권과삭제의 독립 복원이다. C++ 공유 검사·공통 등록/상세LEFT/커밋은 root가 통합한다.
