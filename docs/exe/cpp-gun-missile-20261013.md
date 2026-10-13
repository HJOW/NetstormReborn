# 썬 캐논 Missile 숫자 몸체 및 실제 생성 연결 (2026-10-13)

분석 PC/마지막 정밀 디컴파일 PC는 **HJOW-Athlon**이다. 원본 10.78과 CD판을 읽기만 했으며 원본 게임이나 OS를 실행하지 않았다. `gunmissile` 및 `gunmissile-helper`의 현재 정밀 내보내기를 사용한다.

## 실제 의미

`004571b0/CD004793d0`은 **교차하는 레인 배리어 코너**를 고르는 함수다. 표적의 발자국 중심을 반환하는 함수가 아니다. 배리어 교차가 없는 일반 발사에서는 `-10000,-10000`을 반환한다. 초기 valid 좌표 입력만 다룬 초안의 해석을 바로잡고 일반 invalid 좌표의 정상 표적 발사와 별도 배리어 코너 경로를 모두 복원/관찰한다.

일반 발사는 `incomingCorner=0`이며 실제 표적의 발자국과 이동 선분이 교차하면 현재 총의 `004d4f00/CD004432f0` 직접 피해 경계에 도달한다. 배리어 코너 발사는 `incomingCorner=1`이며 해당 좌표의 작은 사각형에 충돌해 visual 효과 `0045a750/CD0047c8a0`를 생성하고 총의 사거리 시각만 갱신한다. 이 visual 효과를 직접 피해 함수로 해석하지 않는다.

| 항목 | 10.78 | CD |
| --- | --- | --- |
| Missile ctor | `00481a50` | `0044ac70` |
| Missile frame | `00482340` | `0044b260` |
| 실제 가상 표 | `0050d630` | `00501ca0` |
| 사정선 축 정렬 | `00481750` | `0044a960` |
| 현재 중심 | `004adea0` | `004aef30` |
| 일반 표적 hit | `004d4f00` | `004432f0` |
| 사거리 시각 갱신 | `004d5000` | `004433e0` |

패치 객체는 `0x80`, CD는 `0x68`이다. 예약 종료/시작/이전 좌표는 패치 `+0x68/+0x70/+0x78`, CD `+0x50/+0x58/+0x60`에 있다. 패치의 추가 빗나감/건너뛰기 필드를 CD 객체에 삽입하지 않는다. CD Frame은 남은 시각/전체 이동시간을 float로 먼저 저장하지만 패치는 double 경과 값을 유지한다.

## 코드와 관찰

`RawMissile`은 원본 최초 썬 캐논 발사의 숫자 필드·포구 보정·예약·표시 자산 생성·소유자·Pop·비행 효과 요청·이동·표적/배리어 코너 충돌·범위 종료·피해/사거리/삭제 순서를 구현한다. `Missile`은 같은 Host/Kernel의 실제 ProcessForm 수명을 소유한다. `MakeMissileFactoryHooks`는 **실제 `SquidFactory.Create(type,2)`** 몸체를 연결하고, `CreateMissile`은 원본 flags `0x10`으로 실제 Kernel에 붙인다. `MakeGunMissileHooks`는 GunAnimation launch의 생성 경계를 이 factory에 연결한다.

`tools/decomp_gunmissile_oracle.py`는 생성자와 Frame의 실제 몸체를 제한 x86으로 실행한다. 포구·SHP·타입 query·사정선·발자국·자산 생성/소유자/Pop·비행 효과·피해·visual·사운드·삭제는 명시 관찰 경계다. 따라서 C++의 정상 대조는 각 숫자 몸체와 경계 인자/순서를 증명하며, 그 경계 자체의 실제 월드 구현 완료를 증명하지 않는다.

최종 고유 격자는 **296개 정상 반환**(판본별 ctor 36 + Frame 112)이다. 일반 직접 피해 경계는 각 판본 7회, 별도 배리어 visual 경계도 각 판본 7회 관찰한다. 앞선 valid 코너 전용 148행 생성은 이 격자에 포함되므로 별도 신규 관찰로 더하지 않는다. 원본 assert/OS 호출은 0이다. 저장물은 `cpppj/tests/fixtures/gunmissile-x86.tsv`와 `cpppj/recovery-gunmissile-evidence.json`, C++ 대조는 `MissileTests.cpp`다. 감사 명령은 `python -X utf8 tools/decomp_gunmissile_oracle.py --verify`다.

초기 대조 실패를 조사하며 사정선 관찰 경계의 반환 계약을 교정했다. 실제 `00481750/CD0044a960` 호출자는 출력 포인터를 EAX로 받아 즉시 읽는다. 경계가 좌표만 쓰고 EAX에0을 반환했던 초안에서는 주소0의0좌표를 읽어 종점을 `(1,1)`로 Clamp했다. 출력 포인터를 반환하도록 바꾸고 같은296개 입력을 재생성했으며, 교정 전 관찰을 추가 누적으로 세지 않는다.

## 실제 연결에 필요한 후속

- 정상 교차 없음의 `targetPoint`는 원본 invalid 좌표를 보존해야 한다. 임의로 표적 중심을 넣으면 피해가 발생하는 경로가 달라진다.
- 썬 캐논 포구 가상 getter는 raw 좌표를 복사하는 `0041c110`이다. 이어지는 SHP 물리 프레임의 signed 픽셀 hotspot과 타입 hotspot 보정을 둘 다 적용한다.
- `targetDimensions`는 `footX`, `footY + heightExtension`이다. Missile의 `targetBounds`는 기록한 발사 시 표적 좌표에서 해당 전체 치수를 뺀다. 기존 일반 `Footprint`의 `-1` 확장 규칙을 그대로 대입하면 안 된다.
- 실제 사정선·비행 fieryTailSystem 타입11·hitEffect·Gun 직접 피해/사거리 갱신·배리어 visual/Regular/사운드 및 삭제를 같은 월드에 연결해야 한다. 누락 callback은 예외를 유지한다.
- 반사/다른 총/패치 빗나감 경로/디버그 좌표 보정/0거리 특수 처리와 실제 게임 플레이 동일성은 아직 미완료다. 이 단위만으로 캠페인 전투 완성을 주장하지 않는다.

최신 Release에서 집중4개 검사/296개 전수 C++ 대조와 실제 Factory/공간/Kernel 삭제 수명 검사가 통과했다. 전체968개 검사도 실패0이다. 로그는 `extracted/campaign-runtime5-Missile_-final-tests.log`다. 첫 차이는 oracle ray 경계의 결과 포인터 EAX 미반환 때문에 종점이(1,1)로 clamp된 문제였으며 실제 반환 계약으로 교정했다. 같은296개 입력을 다시 관찰했고 신규 입력으로 중복 합산하지 않았다. 초기 차이 로그는 보존한다.
