# Player 작업장·그래프 배치 기준점 조회

2026-10-09. **마지막 디컴파일 수행 PC: `HJOW-Athlon`, 2026-10-09.** AGENTS.md·두 인계 문서·cpppj 내부 문서를 읽고 현재/마지막 호스트 일치를 확인했다. 새 `playeranchor` 목록 14/15/15개를 같은 PC에서 읽기 전용으로 내보냈다. 원본 게임/복사본·클론 창·OS는 실행하지 않았다.

[`RawPlayerPlacementAnchor`](../../cpppj/src/o/RawPlayerPlacementAnchor.h)은 앞 단계의 필수 Player 조회 경계를 실제 후보 수집·거리 선택·그래프·종속 조회로 채운다. 공통 `SquidPostPopState.ownerFactories`는 기존 소유자 지정/공통 postPop 장부와 공유한다. **별도 추가 목록 `0055a70c`/CD `005670e0`는 공통 factories 목록과 다른 자료**이므로 별도 `SquidPostPopList`로 받는다. 이 추가 목록의 전체 생성/삭제 수명 연결은 후속이다.

| 몸체 | 10.78 | CD·추가 10.37 |
|---|---|---|
| Player 후보 수집/반환 | `0048fdb0` | `00406f80` |
| 여러 후보의 거리 선택 | `0048fd30` | `00406ef0` |
| 실제 거리 helper | `0041cfc0` | `0043f750` |
| 좌표 / SID 그래프 조회 | `00462b80` / `00462bc0` | `0045b670` / `0045b6f0` |
| surface 지도 좌표 변환 | `0040eaf0` | 좌표 그래프에 인라인 |
| raw graph BYTE getter | `004ad390` | `004adbc0` |
| 종속 mask/kind 조회 | `004ac9f0` | `004ac560` |

## 복원 계약

좌표 그래프는 비활성이면 고정 번호 254를 돌려준다. 활성일 때 좌표에 **float 0.9999(`0x3f7ff972`)를 더한 뒤 중간 float 저장 없이 0쪽으로 절삭**한다. 0..255 밖이거나 surface SID가 0이면 254다. 표면 SID의 raw graph는 패치 +30/CD +28이다. SID 그래프는 0이면 0, surface 플래그 `flags1 & 0x800`이면 활성 상태와 무관하게 raw 번호를 사용하고 비표면은 현재 좌표를 조회한다. 패치 raw getter의 surface/현재 fence/windArcher 타입 assert와 debug SID 0 assert는 C++ 진단으로 보존한다.

디컴파일 출력에는 `fadd` 상수가 생략되어 있었다. 최초 C++ 재생의 `(255.9,255.9)` 지도 경계에서 세 판본이 불일치했다. 원본 명령과 상수 주소 `00500edc`/CD `00502418`을 확인하고 보정을 반영했다. 전체 독립 원본 fixture의 기대값은 바꾸지 않았다.

Player는 요청 좌표의 그래프를 조회하고 **현재 invalidGraph DWORD와 같은 경우** 즉시 0을 반환한다. 반환 번호 254와 invalidGraph 전역을 동일한 상수로 합치지 않는다. 소유자별 작업장 목록을 원래 순서로 읽는다. 요청 타입의 `flags2 & 6`이 있으면 후보를 허용한다. 그 외에는 기존 실제 contained 커서로 `(mask=0x10, kind=요청 타입, index=0)`을 조회한다. 종속이 있으면 별도 linked 상태도 켜고, 종속이 없을 때 현재 ignoreRestrictions가 켜져 있으면 허용한다.

허용 작업장 중 같은 그래프의 SID를 임시 목록에 넣는다. 중복을 지우지 않으며 최대 **40개**다. 가득 찬 뒤에도 원본의 종속/그래프 조회를 생략하지 않는다. 후보가 있거나 linked가 켜졌을 때만 추가 목록을 순회한다. 추가 후보는 현재 raw owner와 그래프가 모두 같은 것만 같은 임시 목록에 넣는다. 작업장의 소유자를 추가로 거르는 규칙을 만들지 않는다.

임시 후보가 0개면 0, **1개면 그 SID를 거리·소유자 재검사 없이 반환**한다. 여러 개면 현재 owner가 요청자와 같은 후보만 거리로 선택한다. 초기 최소값은 float 9999이며 유한 거리에서 `거리 < float 최소값`일 때 채택하고 그 최소값을 다시 float으로 저장한다. 같은 sqrt(5) 거리 후보 50/51의 원본 독립 관찰은 **뒤의 51을 반환**한다. float 저장의 반올림 때문에 같아 보이는 거리를 “항상 첫 후보”로 처리하거나 최소값을 double로만 유지하면 틀린다. C++ 거리 중간값은 double로 유지하고 최소값 저장 위치를 보존한다. 임의 80비트 반올림 경계 전체의 동등성을 주장하지 않는다.

NaN 비교는 판본마다 다르다. 패치는 x87 C0/C2를 함께 검사해 unordered를 제외하지만, CD는 C0만 검사하여 거리나 최소값이 NaN이어도 채택한다. 디컴파일 C의 `<`만 따르면 CD가 틀리므로 원본 명령과 독립 NaN fixture를 따라 분리했다. 최초 공용 `<` 구현에서 CD/추가 PE 재생이 실패한 뒤 이 차이를 반영했다.

`MakePlayerAnchorQuery`는 같은 SID 풀의 Locate를 기존 `RawCanonPlacementPermission` 필수 조회에 연결한다. `Inspect`는 같은 실행의 임시 후보 목록도 돌려주며 별도 캐시를 만들지 않는다. 원본 밖 SID/소유자/타입·손상된 목록·비유한 그래프 절삭은 실제 접근 시 진단한다. 무효 그래프·빈 작업장 목록에서 쓰지 않는 자료는 미리 읽지 않는다.

## 독립 원본 검증과 한계

[`decomp_playeranchor_oracle.py`](../../tools/decomp_playeranchor_oracle.py)는 세 실제 PE 각각 **1,598개**, 총 **4,794개**를 두 x87 정밀도 `0x027f/0x037f`로 실행했다. 판본별 collector 1,296개·좌표 그래프 60개·SID 그래프 180개·거리 선택 62개다. 반환 DWORD·원본 임시 목록의 순서/중복/상한·raw 전체/surface 지도 checksum을 대조한다. 기대 반환/후보 목록을 Python/C++의 배치 규칙으로 계산하지 않는다.

Player·거리 선택/실제 sqrt helper·그래프·현재 타입 flags·contained Begin/Next/기본 true 필터는 **전체 실제 몸체를 정상 반환까지 실행**한다. 세 판본/두 정밀도의 관찰은 모두 일치한다. 정상 cdecl ESP·보존 레지스터·FS 예외 체인 원상 복구·x87 깊이/제어·허용 코드/쓰기·원본 입력 보존·assert/OS 0회·SHA/행/실제 진입과 임시 메모리 확보/반납 균형을 감사한다. [근거 JSON](../../cpppj/recovery-playeranchor-evidence.json)에 기록했다. 누적 독립 fixture 입력은 **332,917개**다.

**대체 경계는 임시 160바이트 배열의 정상 메모리 확보/반납뿐**이다. 패치는 CRT 확보 `004e4391`/반납 `004e4354`, CD는 확보 `004f1650`/배열 소멸 진입 `00407c50`에서 대체한다. CD의 실제 정리 래퍼 `004071d0`/`004071d8`는 실행한다. 원본 CRT 메모리 부족·예외 전파·게임/GUI/OS 동작은 검사하지 않는다. 원본 함수에 읽기 자료와 정상 확보를 공급한 검증이며 전체 MayPlace 실행 근거는 아니다.

새 C++ 검사는 세 PE 전체 재생과 두 판본의 **실제 SquidOwner 작업장 등록/제거→Player 조회→후보 권한→지형 누적** 연결을 포함한다. 소유자 변경 뒤 즉시 조회 결과가 바뀌고, 종속 linked가 켜지면 그래프가 다른 작업장 뒤에도 추가 목록을 검사하며, contained 타입 전역이 바뀌면 다음 호출에서 이를 다시 읽는다. 자료/풀/목록/SID 0 진단과 조기 경로도 검사한다.

## 다음 단계와 재현

후속 [실제 주변 표면 finder/권한 누적](cpp-canon-surrounding-reconstruction.md)은 완료했다. 다음은 최종 표면 소유 관계·특수 지역·거부 조건과 별도 추가 목록의 공간 수명 연결이다. 이어 사제 Pop·보호막 생성/회복 예약·Carrier 상태 검사, raw GUI·건설·경제·전투·승패를 진행한다. **전체 MayPlace와 실제 미션 완주는 아직 미완료**다. 여러 판본 실제 자산 전수·창/픽셀·최대 지도 회귀는 계속 인계한다.

```powershell
tools/ghidra/export_functions.ps1 -Name playeranchor
python -X utf8 tools/decomp_playeranchor_oracle.py
python -X utf8 tools/decomp_playeranchor_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
```

Release 경고/오류 **0**, CTest 내부 **406개·실패 0**(113.03초), 근거 감사 **59종 모두 통과**했다. 완료 범위와 다음 단계는 [LEFT_JOBS.md](../../LEFT_JOBS.md)를 따른다. 로그는 `extracted/cpp-playeranchor-export.log`, `cpp-playeranchor-oracle-final.log`, `cpp-playeranchor-verify-final.log`, `cpp-playeranchor-build-final.log`, `cpp-playeranchor-ctest-final.log`, `cpp-playeranchor-audits-final.log`다. 기존 fixture/감사 도구·보호 파일·dotnetpj는 변경하지 않았다.
