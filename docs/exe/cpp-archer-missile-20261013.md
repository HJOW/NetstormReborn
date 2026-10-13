# 썬 디스크 발사기 투사체 (2026-10-13)

분석 및 마지막 정밀 디컴파일 PC는 **HJOW-Athlon**이다. 보호 원본 10.78 및 CD 10.72를 읽기만 했으며 게임/OS를 실행하지 않았다. 현재 정밀 `gunarcher`, `gunmissile`, `gunmissile-helper` 내보내기를 사용한다.

## 실제 이름과 경로

원본 타입 번호71의 내부 이름 `sunArcher`는 화면의 **Sun Disc Thrower(썬 디스크 발사기)**다. 이 경로의 투사체는 `sunDisc`83이며 속도35·비행 효과 타입0이다. 썬 캐논128의 `display80/speed20/effect11`과 구별한다. 별도 Arrow를 만드는 경로로 해석하지 않는다.

`RawMissile`에 실제 group0 부모71 분기를 추가했다. 부모 포구의 raw 좌표를 사용하고 썬 캐논의 SHP/type hotspot 보정·사정선 축 정렬을 생략한다. 방향을 가진 sunDisc 표시 자산은 frame1을 쓰며 실제 `Factory.Create(type,2)`·소유자·Pop·같은 Host/Kernel 수명을 기존 Missile adapter로 연결한다. 사운드 ID는 현재 타입 자료에서 읽는다.

| 항목 | 10.78 | CD 10.72 |
| --- | --- | --- |
| 실제 Missile ctor | `00481a50` | `0044ac70` |
| 실제 Missile Frame | `00482340` | `0044b260` |
| 원본 객체 크기 | `0x80` | `0x68` |
| group0 코너 position | 요청 코너 | 요청 코너 |
| group0 코너 endpoint | 요청 코너 | 표적 발자국 치수로 보정한 중심 |
| `+32/+36` | group0에서 기존 바이트 보존 | speed/allowance 필드 |

10.78 ctor의 `+32/+36` 초기화는 **group1 코너 분기 내부**에 있다. group0을 추가하면서 그 초기화를 모든 코너에 적용했던 C++ 초안을 교정했다. CD10.72 group0 코너는 요청 코너를 position에 기록하지만 endpoint는 처음 계산한 표적 중심을 유지한다. 예제 입력의 position은 양판본 `(100,90)`이며 endpoint는 10.78 `(100,90)`, CD `(99,88.5)`다. 그 차이가 예약 종료 시각에도 반영된다. 원본의 차이를 임의로 일치시키지 않는다.

코너 Frame의 범위 종료는 10.78 group0과 CD에서 일반 거리 여유4 분기를 사용한다. 10.78 썬 캐논 group1의 별도 거리 비교를 보존한다. 피해·충돌 visual·사운드·삭제는 원본과 같은 순서의 명시 하위 경계이며 시각 효과 누락 callback은 예외다.

CD10.72 Frame은 충돌과 사거리 종료가 같은 틱에 겹쳐도 표시 자산의 nonzero `impactSound`를 요청한다. 10.78의 사거리 종료 시 사운드 생략 조건을 CD에 적용했던 초안을 두 번째 C++ 대조에서 교정했다. 근거는 CD `0044ba81` 및 `run7-ArcherMissile_-fixed.log`이며 첫 ctor 교정 직후의 실제 소스/보고서는 `hist/archermissil-56f6b`에 별도로 보존했다.

## 독립 원본 관찰과 기존 근거 보존

`tools/decomp_archermissile_oracle.py`는 **양판본 1,888개 정상 반환**, 판본마다 ctor48 + Frame896을 실제 같은 PE 몸체에 실행한다. 사거리9/40, 발사/충돌 사운드0/nonzero, 표적 상태·이동시간·배리어 코너를 교차한다. 원본 assert/OS 호출은 0이다. 중심·포구·타입 query·자산/공간·사운드·피해/충돌 visual·삭제는 명시 관찰 경계다. 각 하위 경계의 실제 월드 몸체 완료를 이 숫자에 포함하지 않는다.

05 commit **4e532fd**의 `RawMissile.cpp/.h`와 기존 `gunmissile-x86.tsv` blob SHA를 새 보고서의 `baseline`에 보존했다. 기존 썬 캐논296개 fixture와 보고서는 변경하지 않았다. 새 근거는 `archermissile-x86.tsv`, `archermissile.json`, `ArcherMissileTests.cpp`다.

첫 C++ 대조의 코너 차이12checks 로그는 `extracted/run7-ArcherMissile_-first.log`에 있다. C++ ctor 두 분기와 CD 충돌 사운드 조건을 교정하고 같은1,888개 입력을 다시 관찰했다. 신규 고유 입력은 **0**이다. 재생성 fixture SHA는 root가 사전 실제 보고서 tool 출력에서 관찰한 SHA와 동일하다. 교정 분기를 역변환한 C++도 사전 SHA와 동일함을 `tools/analyze_archermissile_correction.py`가 검사한다. 보존본은 `cpppj/hist/archermissil-d6700`에 있다. **이전 전체 보고서 바이트 snapshot은 보존하지 못했다.** 사전 tool 출력의 SHA·현재 fixture 동일성·역변환 C++ SHA를 통해 교정 경로를 감사하며, 이전 보고서 전체를 복원했다고 주장하지 않는다.

감사 명령은 다음과 같다.

```powershell
python -X utf8 tools/decomp_archermissile_oracle.py --verify
python -X utf8 tools/analyze_archermissile_correction.py --verify
python -X utf8 tools/decomp_gunmissile_oracle.py --verify
```

## 남은 실제 월드 연결

교정 후 공유 Release에서 **ArcherMissile 두 검사/1,888행 및 기존 썬 캐논4개 회귀/296행이 모두 통과**했다. **최종 Release 두 타깃 종료0·경고/오류0, 07 집중 고유13개와 전체983개 검사/실패0**이다. `run7-ArcherMissile_-final.log`와 `run7.json`이 최종 근거다. 회전/각속도 발사 애니메이션은 별도08의 새 `RawArcherAnimation` 단위로 준비하며 07 완료 관찰에 포함하지 않는다.

실제 타입 방향/물리 프레임·사운드 로딩, TargetPoint의 일반 invalid 코너, 같은 월드 공간·일반 피해자 HP·충돌 visual 하위와 미션 승패까지 연결해야 한다. 독립 투사체 관찰만으로 캠페인1-1의 전투나 완주 동일성이 완료됐다고 주장하지 않는다.
