# 실제 Dude 생성과 최초 Pop

## 현재 자료

- 주 대상은 패치 **10.78**, 비교 대상은 CD **10.72**다. 마지막 디컴파일 PC는 **HJOW-Athlon, 2026-10-14**이며 현재 PC와 같다.
- `dude-spawn-functions.json`에서 패치10/CD9 함수를 같은 정밀 프로젝트에 읽기 전용으로 내보냈다.
- 새 독립 원본 관찰은 **288정상/assert0**, 각 판본144개다. 실제 생성 조건을 통과한 **72개(각36)**에서 factory·Dude 생성자·공통 owner·Pop·Dude 최초 후처리·SetFrame·표시·공통 장부를 원본 명령으로 실행했다. 외부 대체는0이다.
- **C++ 공유 빌드/대조는 아직 미수행**이며 다음13단위의 공유 Pop 분배와 발자국0 경로 연결이 필요하다. 현재12의 Burning820·DudeTask684정상/assert96 또는 기존 Notice414에 이 원본을 더하지 않는다.

## 보호 자산과 실제 가상 표

양판본 `dude.type`은 실제 typename `Man`, description `Nimbian`이다. `guy shadow not_selectable not_real`, maxHP20, speed1.5, 발자국 **0×0**이다. A~H 각8프레임과 I00 한개로 **65클러스터**이며 그림자 레이어를 포함한 첫 SHP 블록은 **130물리프레임**이다. I00/물리64는 `blank.gif`다.

원본 실행기에는 보호 `_shapes.shp` 전체를 읽기 전용으로 연결한다. 확인한 TYPE flag/HP/발자국/I Run을 현재 입력으로 공급한다. 전체 TYPE parser를 이 단위에서 새 실행했다고 주장하지 않는다. 기존 TYPE 로더와 실제 보호 텍스트의 번호/플래그 계약을 재사용한다.

| 역할 | 패치10.78 | CD10.72 |
| --- | --- | --- |
| 생성 상위 | `00450a20` | `004e8b30` |
| Dude 생성자 | `00451450` | `004e8b10` |
| 실제 가상 표 시작 | `00508cc8` | `005072f0` |
| 최초 postPop | `00450cd0` | `004e9230` |
| 공통 owner 가상+74 | `004adf00` | `004aefa0` |
| 공통 Pop 가상+90 | `004b02d0` | `004ad490` |
| 실제 I 프레임 쓰기 | `004acee0` | `004acbc0` |

패치의 과거 `00508cd0` 표기는 가상 표 시작에서8바이트 뒤 주소였다. 실제 생성자의 대입과 PE DWORD 표를 기준으로 `00508cc8`을 사용한다. 생성 상위의 디컴파일 C에 나온 `unaff_retaddr` 표현도 사용하지 않는다. 실제 PE 스택은 **x/y/owner 세 인자**다.

## 원본 사슬

생성 상위는 signed population count가 capacity보다 작은지 확인하고 blocked가0인 경우만 실제 factory에 Dude70/flags2를 요청한다. 이후 패치는 frame DWORD+36 전체, CD는 frame BYTE+34만0으로 쓰고 실제 owner와 Pop(x,y,0)을 호출한다. 반환값은 실제 새 SID다. 이 구현은 기존 `SquidFactory`와 `SquidOwner`의 전체 몸체를 소비하며 같은 실제 Pop 연결을 요구한다.

최초 후처리는 flags1에서 raw task WORD5101을 쓰고 한도 안이면 현재 population에 SID를 추가한다. 이후 현재 TYPE의 I 첫 프레임64를 실제 SetFrame으로 지정한다. 마지막에는 항상 공통 postPop을 호출한다. `RawDudePostPop`은 같은 `SquidFrame`과 `SquidPostPop::PostPopBase`에 연결한다. list 포인터/capacity/count는 생성 한도와 최초 등록이 공유한다.

원본 격자는 capacity/count `(0,0)/(1,0)/(1,1)/(3,0)/(3,2)/(3,3)`, blocked0/1, 두 분수좌표, owner0/1/8, 후속 직접 몸체 호출0/1을 포함한다. 결과는 새 SID·population count/전체 capacity·대상 슬롯 전체·전체 해시/spot 체크섬·비용/깊이/세 통계 배열·목록·실제 표시 변경 영역이다.

**`repeat=1`은 최초 생성 뒤 flags50의 postPop 몸체를 직접 호출한 입력이다.** 실제 Activate나 Unpop/Pop 재등록을 실행한 입력과 구분한다. 따라서 해당 직접 호출은 현재 depth0에서 공통 decrement 결과 `ffffffff`를 관찰한다. 최초 생성의 정상 Activate/깊이 복원과 이 독립 비최초 몸체 관찰을 합치지 않는다. 도구 입력 설명의 “재등록”이라는 초안 표현은 이 직접 postPop 관찰을 가리키며 실제 재등록 완료의 근거가 아니다.

기존 확보 메모리에서 실제 Reset/Create로 만든 전체 초기 풀·SID 전역을 복사해 다음 독립 입력으로 복원한다. 같은 입력 준비를 반복했거나 내부 helper를 여러 번 호출했다는 이유로 새 고유 관찰 수를 늘리지 않는다.

## C++ 후속 연결

현재 새 `RawDudeSpawn.h/.cpp`와 `DudeSpawnTests.cpp`는 실제 factory/owner/Frame/표시/공통 장부를 구성한다. 매크로는 `NETSTORM_DUDESPAWN_FIXTURE`다. 공유 `SquidPostPop::SetDudePostPop`/Validate/Activate 분배와 실제 **발자국0의 빈 공간 순회**는12커밋 후 기준12 전체 본문·이전 관찰을 보존하는 별도 lineage로 추가해야 한다. 현재12 공유 파일은 수정하지 않았다.

최초 미컴파일 C++ 초안에 없던 `BaseFileSystem::Root()` 접근이 있어 명시 판본 경로 멤버로 옮겼다. 당시 production2/test/tool/fixture/report **전체6파일**을 `hist/dudespawn-initial`에 보존했다. 정확 세 변화 역변환과 fixture288·원본도구·production 불변은 `analyze_dudespawn_draft_lineage.py`/별도 보고서로 감사하며 새native0이다. 이것은 빌드 성공이나 새로운 원본 입력을 의미하지 않는다.

실제 task5103/5104의 부모 드롭·DudePath24 생성/이동·피해/삭제와 GameWorld 설치·캠페인1-1 완주는 아직 후속이다. 빈 effect로 전체 드롭 완료를 표시하지 않는다.

```text
python -X utf8 tools/decomp_dudespawn_oracle.py --verify
python -X utf8 tools/analyze_dudespawn_draft_lineage.py --verify
```
