# Burning 고정 연소점 선택

## 출처와 현재 결과

- 주 대상은 패치 **10.78**, 비교 대상은 CD **10.72**다. 현재/마지막 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-14**다.
- 실제 상위 함수는 `00424110/CD004a5d30`이다. 기존 Burning18/19를 보존하고 Sources 보조 **8/7**, 별도 CD TYPEflags1 **1**, 위크 마나 타입번호 **1**을 같은 정밀 프로젝트에서 읽기 전용으로 내보냈다.
- 새 원본 관찰은 **652정상(326×2)/assert0/외부 관찰 경계0**다. 새 C++2검사는 준비했고 **공유 대조는 아직 미수행**이다.
- 생성자256·Append564의 기존820 및 Notice414와 다른 입력/몸체의 후속 자료다. 이 격자로 전체 Step·캡처·첫팔레트·Draw·Host 생성/삭제를 완료했다고 표시하지 않는다.

## 실제 계산

원본은 SID 유효번호 helper를 호출하고 현재 HasHP·raw HP·TYPE 최대HP를 읽는다. 패치는 HP DWORD, CD는 signed HP WORD다. TYPE 최대HP의 위크 마나 옵션은 해당 타입158에서 최대HP를4로 나눈다. 이 옵션/번호는 `BurningSourcesState`의 같은 현재 상태를 매 호출 참조한다.

목표행은 현재 HP×5를 `최대HP−최대HP/2`로 나눈 값에서 계산하고 이미지 폭+높이의5분할에 제한한다. 최소1이며 genus400은 항상1이다. 높이3 미만이면 실제 selected 값만 갱신한다. 현재 selected가 목표와 같으면 그대로 반환한다.

늘릴 때는 폭 가장자리7칸·상한8과 높이 마지막행을 고려해 다시 목표를 제한한다. 정수난수의 최소는 포함하고 **최대는 제외**한다. 같은 게임 난수 상태를 사용하며 CRT rand와 다르다. x/y를 선택하고 현재 마스크의 한줄 아래 픽셀과 rowTaken을 검사한다. 각 대각선의6~9개 후보 중 불투명 점을 실제 Append로 추가하고 행을 표식한다. 허용된 점이4개 이상이면 selected를1 증가시킨다.

최대300번 시도하며 마지막 시도에서 목표에 도달해도 남은 횟수가0이면 폴백에 들어간다. 폴백은 마지막행을 제외한 y/x 순서에서 첫7개 불투명 점을 사용한다. **원본은 Append 실패를 무시하므로 입자 용량이 부족해도 rowTaken/selected 또는 폴백의 사용점 수가 증가할 수 있다.** 이 차이를 새 격자에서 용량0/5/15와 가득 찬 양수수명 배열로 관찰한다.

줄일 때는 현재 total 범위의 첫 source 포인터 표식을 찾아 해당 대각선 행을0으로 바꾼다. 같은 행의 source 입자만 포인터 표식/life를0으로 만든다. 좌표·속도·total과 **selected는 감소시키지 않는다.** 현재 C++도 실제 원본 쓰기만 수행한다.

원본은 source 함수의 반환 EAX를 이후에 사용하지 않는다. 패치의 디컴파일 포인터 반환에 보이는 임시값/이전 레지스터를 의미 있는 C++ 반환값으로 만들지 않는다.

## 독립 입력과 출력

작은 이미지2×2/3×3과8×8/16×12, HP0/25/50, genus400/비400, selected0/3, 전부불투명/전부투명 마스크, 세 게임난수 시드, 현재 source/가득 찬 일반입자를 교차한다. odd 최대HP101과 위크 마나0/1, 용량0/5/15, SID0/65535의 자연반환을 별도로 포함한다.

TYPE/HP/캡처 마스크·기존 source 좌표/rowTaken/입자는 현재 입력이다. 실제 protected TYPE나 프레임을 모두 다시 로드한 결과와 구분한다. 출력은 selected/total·행 표식 전체·capacity개 입자24바이트 전체·난수 전역이다. 원본 this+74/null의 두 source 포인터만1/0으로 정규화하며 다른 의미 필드는 그대로 비교한다.

각판본에서 원본 정수난수 **22,094회**, Append/Particle.New 각 **940회**, ftol **672회**가 내부에서 호출됐다. CD 위크 마나 타입번호 helper는18회다. 내부 호출 수를652의 독립 고유 입력 수에 더하지 않는다. 힙·부착·캡처·OS 호출은 없으며 미허용 외부 호출에 닿으면 실행기를 중단한다.

## C++와 남은 연결

`RawBurningSources`는 같은 `RawBurningVisual::Append`, 현재 SID 풀/타입과 `GameRandom`을 소비한다. 기존 생성자/Append/그 원본 보고서를 수정하지 않았다. 프레임이 실제 캡처를 수행한 뒤 이 선택기를 호출하는 사슬은 아직 후속이다. 진단전역이0인 정상 입력·유한 source좌표·양수최대HP·정상 마스크 범위를 대조한다. 비유한 좌표·잘못된 버퍼/0나눗셈은 C++ 보호 경계이며 이 격자의 정상 완료에 포함하지 않는다.

현재 RawBurningVisual의 실제 Warm 생성자256와 Append564는 단위12 첫 공유 대조에서 통과했다. 새652/양판본2검사는 단위14의 현재 공유 C++ 전수 대조에서 모두 통과했다(`campaign-runtime14-BurningSources_initial-tests.log`). 단위14 전체 검사는 아직 진행 중이며 이 개별 성공과 구분한다. cache0 ColorSort/gradient/buildPalette와 실제 Source 캡처·전체 Step/Draw·같은 Host factory/삭제·GameWorld 및 캠페인1-1 완주는 미완료다. 새 실제 Frame426은 단위15의 별도 준비 자료다.

자료는 `RawBurningSources.h/.cpp`, `BurningSourcesTests.cpp`, 매크로 `NETSTORM_BURNINGSOURCES_FIXTURE`, `burningsources-x86.tsv`, `recovery-burningsources-evidence.json`이다.

```text
python -X utf8 tools/decomp_burningsources_oracle.py --verify
```
