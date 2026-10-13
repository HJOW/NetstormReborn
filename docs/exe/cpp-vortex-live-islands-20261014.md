# Ascend와 실제 현재 섬 목록 연결

## 범위와 출처

마지막 디컴파일 수행 PC는 **HJOW-Athlon, 2026-10-14**다. 10.78의 `0046f480`, CD10.72의 `004bdba0`는 실제 IslandList의 countDWORD와 32바이트 기록을 읽어 다른 nonzero owner의 섬 수를 센다. 현재 PC에서 다시 생성한 정밀 원본 분석 자료와 기존 독립1496개 원본 입력을 사용했다. 원본 또는 복사본 게임·GUI는 실행하지 않았다.

기존 Ascend 입력 어댑터는 존재/owner만 가진8바이트 항목의 span을 받았다. 이번 overload는 같은풀의 `RawIslandRuntime`을 참조해 실제3204바이트 저장소를 직접 읽는다. 후보 판정·20개 버퍼·난수·가상 삭제의 기존 전체 몸체와 이전 span 경로는 그대로 보존한다.

## 실제 집계

- 반복마다 현재 countDWORD를 signed32로 읽는다. 음수 비트값이면 항목을 읽지 않는다.
- 현재 항목이 존재하고 owner가 요청값과 다르면 owner를 다시 읽어0을 제외한다. 존재/현재owner는 실제 DWORD이며 원래ownerBYTE와 합치지 않는다.
- 동일한 목록에 Add/Delete 또는 현재owner 쓰기가 일어나면 다음 호출에서 바로 읽는다. 삭제는 기존 runtime처럼 count를 줄이지 않는다.
- 목록과 Ascend의 pool 참조가 다르면 구성 단계에서 거부한다. runtime/hash/spot/type/random의 수명은 소비자보다 길어야 한다.

패치는 일부 첫 owner 읽기를 인라인하고 CD는 getter를 두 번 호출한다. 존재하는 정상 기록에서 두판본은 같은 입력 결과를 낸다. 물리100개 기록 밖의 원본 메모리 읽기는 복원 계약에 포함하지 않는다.

## 검사와 전체 이전 본문 보존

`VortexIslandBindingTests.cpp`의 두 검사는 양판본 기존Count40개 입력을 실제32바이트 간격에 기록하고 저장된 원본 반환값을 대조한다. 목록 전체3204바이트와 게임난수 보존도 확인한다. 다른 검사는 Add/Delete/현재ownerDWORD 변경·signed count·다른풀 거부를 실행한다. Count에 불필요한 하위효과는 호출 시 오류로 처리한다.

`analyze_vortex_live_islands_lineage.py --verify`는 기준 **5b10ce8**의 전체 h/cpp를 보존하고 새 선언·overload·참조멤버·분기만 역제거해 파일 전체 문자와 주석 제외 토큰이 같은지 확인한다. 이전 원본 및 단위09 보고서는 당시 바이트를 유지한다. 현재 소스/검사/fixture SHA는 별도 연결 근거에 보관한다.

이번 재사용은 **기존40개 입력, 새native관찰0**이다. 기존1496개를 다시 더하지 않는다. Release/집중/전체 검사의 최종 결과는 단위10 인수인계와 통합 근거에서 기록한다. 실제 GameWorld 설치와 캠페인 완주 동일성은 후속이다.
