# cpppj Gun 표적 검색·발사 허용 판정

마지막 디컴파일 수행 PC: **VM-W11-CODEX, 2026-10-13**. 원본 10.78을 우선하며 비교용 CD 10.72의 실제 다른 몸체를 별도 분기로 유지한다. 이 문서는 캠페인 1-1의 필수 전투 경계를 줄이는 독립 런타임 작업이다. 실제 미션 완주·투사체 생성·애니메이션·음향의 완료 근거가 아니다.

## 복원한 몸체

`RawGunTarget`은 `RawSquidFinder`의 같은 raw SID 풀·네 공간 해시·현재 타입을 사용한다. 후보 배열의 사본이나 새로운 임시 월드 번호를 사용하지 않는다. `GunTargetQuery`는 원본의 8DWORD 값 문맥을 이름 있는 필드로 표현한다. 현재 Pop 위치·발자국 중심·타입·공격 모드 방향·소유자·프레임이 각각 원본 순서로 보존된다.

| 동작 | 패치 10.78 | CD 10.72 |
| --- | --- | --- |
| 전체 검색 | `0046b6f0` | `00435e10` |
| Cannon finder | `0046b0f0` | `00435680` |
| Archer finder | `0046b400` | `00435a60` |
| 후보 허용 | `0046af80` | `00434b20` |
| group별 사거리·각도 판정 | `0046ac00` | `00435580` |
| 썬 캐논 현재 프레임의 발사 허용 | `0046b070` | `00434c30` |
| Cannon 방향 사정선 | `00469190` | `00434d60` |
| Archer 거리·윈드 관측각 | `0046aaf0` | `00435370` |

표적은 실제 Damageable 판정 뒤 서버 번호 대역을 통과해야 한다. 패치는 5~14999, CD는 5~5999를 거부한다. 동맹은 현재 Player+54의 비트 마스크이며 `shootMyFriends`가 켜져 있을 때만 이 배제를 해제한다. 후보의 extra bit1/bit10, 사제 수량, 발사자와 같은 Pop 위치를 원본처럼 검사한다. 사제 수량은 패치 signed DWORD/CD signed WORD 차이가 있다. 여기에 독자적인 dead/free/state 필터를 추가하지 않는다.

Archer는 원본 `ceil(center)`와 `trunc(range)`의 정수 범위에 flags5 finder를 사용한다. ceil은 **float32 0.9999를 x87에서 더한 뒤 절삭**하며 중간 float 저장이 없다. 후보에 실제 HP 플래그가 있어야 하며 썬더 아처는 flyer/balloon genus를 먼저 확인한다. 거리는 현재 확장된 발자국의 발사자 방향 경계점까지의 제곱 거리다. 지상 표적에는 1000을 더하고 더 작은 점수만 교체한다.

Cannon은 0/2/4/6 방향의 사정선마다 finder를 새로 만든다. Rain1/Thunder2 테마는 지정한 방향만 허용한다. 실제 방향별 프레임과 SHP signed short 핫스폿·타입 핫스폿·현재 픽셀 배율을 적용한다. 폭이나 높이가 2보다 작은 얇은 선은 원본 floor(minimum)/ceil(maximum) 뒤 float 경계 바이어스 0.0001/0.1로 확장한다. **두 판본 모두 계산한 사정선 사각형4DWORD를 반환한다.** 패치의 Ghidra C 출력은 원점4값으로 잘못 표현했으나 실제 반환 tail의 지역 사각형 복사와 native 관찰을 기준으로 교정했다. 디컴파일 C만을 원본 동일성 근거로 사용하지 않는다.

윈드 아처는 원본 30도 반각의 최소 circular 구간을 사용하며 구간의 양 끝을 포함한다. degree 환산의 실제 double 상수는 **57.295827908797776**이다. 360도 정규화, float 저장 위치, 사각형 거리의 패치/CD 저장 차이를 구별한다. 썬 캐논 `MayFire`는 방향별 24프레임 표의 bit8000 및 24~27 특수 프레임을 판정한 뒤 실제 `MayTarget`으로 넘어간다. 이 함수는 투사체 생성 함수가 아니다.

## 실제 연결과 남은 경계

`MakeGunTargetHooks`는 풀 일치를 확인해 `GunProcessHooks.findTarget`과 `shoot`를 실제 검색·`MayFire`로 연결한다. 그 밖의 hooks는 보존한다. `shoot`의 이름은 기존 API와의 연결명이며 이 모듈에서 생성 성공을 꾸며 내지 않는다. 실제 투사체 생성은 Gun AnimationProcess의 후속 몸체로 이어가야 한다. `GunTargetType`에는 기존 `RiftTypeRecord`에 아직 없는 range/theme/방향별 L~O 프레임/확장 높이 자료가 필요하다. 현재 실제 타입 자산 로더·미션 월드에 이를 조립하는 일은 부모 작업의 후속이다.

진단 표시가 켜져 있을 때는 `drawRange`가 필수다. 미연결 효과는 예외이며 성공하는 noop로 바꾸지 않는다. 기본 진단 표시가 꺼진 원본 자연 분기에서는 출력 hook을 요구하지 않는다. 현재 shape 물리 프레임/정상 타입·소유자·유한 좌표·방향 범위를 벗어나는 손상 입력은 명시 호스트 예외로 처리한다.

## 검증 상태

`tools/decomp_guntarget_oracle.py`가 보호 PE를 격리 Unicorn x86 메모리에 매핑한다. Ghidra의 정확한 함수 범위만 실행하며 실제 finder·플래그/소유자 getter·SHP lookup·사거리·사각형·각도·CRT fmod를 실행한다. 진단 rectangle의 출력만 실제 인자로 관찰한다. 원본 게임·OS·창·투사체·음향은 실행하지 않는다.

독립 입력은 판본별 1,727개, 합계 **3,454개**다. 각 입력을 x87 53/64비트 정밀도에서 반복해 같은 반환·사정선·진단 요청임을 검사하며 이 반복을 고유 관찰 수에 더하지 않는다. 최초 전체 생성은 정상 반환 6,908개/assert0을 확인했다. 입력에는 서버 SID 경계·동맹/친구 발사·현재 수량·네 방향·테마의 고정 방향·28프레임 발사 허용·지상 점수·확장 발자국·소수 거리 경계·같은 거리와 finder 입력 순서·진단 출력이 있다.

C++ 집중 **3개/실패0**을 확인했다. 전체 3,454개 재생·풀 혼용/활성 효과 미연결 거부·실제 `RawGunProcess.Frame` stage7의 성공/거부 소비자 연결이다. 실제 parentBlocked raw bit20/현재 8DWORD shot 문맥은 같은 풀을 사용한다. 소비자 검사의 발사 간격·감시 제거·전송은 명시 관찰 경계이며 Kernel/실제 프로세스 Form 수명·투사체/애니메이션 완료를 주장하지 않는다. 성공한 판정은 stage2/표적 유지/시각12.25, 잘못된 썬 캐논 프레임은 실제 ClearTarget/stage1/표적0/감시 제거·재전송 요청으로 이어진다.

이번 집중 검사의 Release 타깃 빌드는 경고·오류0이다. 마지막 native 재생성과 current SHA 감사도 완료했다. 고유 3,454개/정밀도 반복 정상 반환6,908개/assert0이며 native의 유일한 대체 경계는 진단 `drawRange`다. 최종 fixture를 다시 읽은 C++ 집중3개도 실패0이다. 로그는 `extracted/guntarget-build-agent-corners.log`, `extracted/guntarget-native-frozen-final.log`, `extracted/guntarget-native-frozen-verify.log`, `extracted/guntarget-focused-agent-frozen-final.log`다. 부모가 공통 게임 타깃·전체 검사·LEFT_JOBS·작업 단위 커밋을 기록한다. 스모크의 메모리 매핑 누락, 사정선 반환/최대 경계·incoming corner clamp 해석, 장면의 미등록 후보 타입 초기화·예약 꼬리/서버·예측 Take 계약은 독립 실제 결과를 기준으로 교정했고 교정 전 로그를 보존했다. 원본의 관찰 입력을 client60으로 치환하지 않았다. 보호 원본·AGENTS.md·dotnetpj·DirectDraw는 변경하지 않는다.
