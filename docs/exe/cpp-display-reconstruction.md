# cpppj raw 공통 표시 갱신 복원

2026-10-06. raw Pop·Unpop의 공통 `update88/update8c`를 표시 활성 상태에서 복원하고 기존 Renderer의 변경 표에 연결했다. 공통 postPop의 영역/소유자/생산 효과와 실제 GameWorld의 raw SID 연결은 아직 남았다.

## 원본 근거

| 경로 | 패치 10.78 | CD 10.72 |
|---|---|---|
| 공통 update88 | `004ad500` | `004ae380` |
| 공통 update8c→가상 update88 | `004ad670` | `004ae3e0` |
| main/shadow 변경 등록 | `004990d0` | `00456a00` |
| SHP 경계 | `00498ff0` | `00456930` |
| 변경 표 | `00497580` | `00457320` |

읽기 전용 헤드리스 Ghidra에서 패치 10개/CD 7개 몸체를 새로 내보냈다. `extracted/display/<판본>/creation.c`, `functions.tsv`와 완료 로그를 확인했으며 프로젝트 변경은 버렸다. 기존 타입 조회/생성/공간 몸체도 실제 기계어로 실행한다. 원본 게임 프로세스·OS API는 실행하지 않는다.

SHP table이 가리키는 VFX 레코드 앞에는 36바이트의 Squid 헤더가 있다. 앞 두 float는 hash level의 cell 크기이고, VFX 앞 12바이트의 signed short 네 개는 표시 폭/높이/hotspot이다. VFX 압축 픽셀 영역과 다르다. 원본 경계는 `trunc(x*16+0.5)-cameraX`, `trunc(y*11+0.5)-cameraY`에서 Q16 hotspot을 빼며, 폭/높이에 Q16 크기와 1을 더한다. 곱셈의 low DWORD와 산술 shift도 유지했다.

main 경계는 viewport의 각 모서리로 먼저 clamp한다. extra bit 4가 켜져 있으면 폭 19 미만에서 양쪽 9, 항상 양쪽 3과 위쪽 여유를 추가한다. 위쪽 여유는 **패치 15픽셀/CD 9픽셀**이다. 확장 뒤 다시 viewport로 자르지 않는다. 타입 flags1 `0x40000`이면 `frameCount+frame`의 그림자도 갱신하며 그림자에는 확장을 적용하지 않는다. dirty 표의 100항목 넘침은 전체 갱신을 켜고 후속 표시를 생략한다.

패치는 signed DWORD frame, CD는 byte frame이다. 패치는 일반 경로에도 frame 범위·shape null·타입 첫 DWORD의 해제 흔적 검사를 유지한다. CD에는 같은 조기 반환이 없다. CD null shape에서 선택 확장을 거쳐 생기는 영역도 원본대로 보존했다. debug/assert UI와 손상된 CD 물리 프레임 접근은 재현하지 않는다.

## C++ 연결

- `o::SquidDisplay`는 raw 슬롯과 타입/SHP 목록을 읽고 공통 갱신을 `SquidDisplaySink`로 전달한다. `o/`는 `client/`를 포함하지 않는다.
- Pop은 공간 머리 등록 뒤 firstPop/void 해제 전에, Unpop은 void/spot/체인 제거 뒤 표시를 호출한다. 생성자에 표시 대상 포인터를 생략하면 이전 억제 경로를 유지한다. 기존 기계어 fixture는 그대로 통과한다.
- `client::SquidRenderer`는 실제 `Renderer::Invalidate`로 전달한다. `ShapeDatabase::SquidMetrics`와 어댑터의 `Shapes`는 실제 자산 헤더를 타입 번호 70부터 연결한다. 순수 VFX에 없는 추가 헤더를 추측하지 않는다.
- 미지원 vtable·판본 혼용·손상된 물리 프레임은 공간 쓰기 전에 거부한다. 표시 경로를 켜도 미복원 섬/다리/건물 부착·파생 후처리를 허용하지 않는다.

## 검증과 재현

[독립 실행 기록](../../cpppj/recovery-display-evidence.json), [기대값](../../cpppj/tests/fixtures/display-x86.tsv), [실행 도구](../../tools/decomp_display_oracle.py), [C++ 검사](../../cpppj/tests/DisplayTests.cpp).

```powershell
python -X utf8 tools/decomp_display_oracle.py
python -X utf8 tools/decomp_display_oracle.py --verify
cmake --build cpppj/build --config Release --parallel 4
ctest --test-dir cpppj/build --build-config Release --output-on-failure
cpppj/build/bin/Release/NetstormCpp.exe --inspect-assets originals
cpppj/build/bin/Release/NetstormCpp.exe --inspect-assets originalCD --cd
python -X utf8 tools/cpp_renderer_smoke.py
python -X utf8 tools/cpp_world_smoke.py
```

새 x86은 두 판본×x87 53/64비트의 4시퀀스, **1,728회**다: 직접 공통 표시 400, 경계 368, 실제 Pop 480, Unpop 480. 대체 함수/assert 도달 0. 준비 Reset/Create 각 4회와 내부 함수 도달은 1,728회에 더하지 않는다. 실제 update88은 각 판본 680회, update8c는 각 340회였다. 경계/dirty 내부 도달은 패치 812/544, CD 870/566회이며 상위 호출 안에 포함된다.

클리핑·폭 18/19 경계·signed hotspot·Q16 네 배율·소수/음수 좌표·그림자/abstract 레이어·null·패치 invalid frame/해제 흔적·표시 억제·raw old/new 위치·변경 표 넘침을 포함한다. 모든 dirty 항목의 좌표/플래그와 Pop/Unpop 슬롯 전체 바이트를 비교한다. 입력 타입/SHP는 합성 준비 자료이며 실제 파일 로더·파생 생성자를 이 실행에서 복원했다는 뜻은 아니다.

Release 경고/오류 0, CTest 내부 **120개 검사·실패 0**. raw→실제 Renderer 어댑터 검사는 Pop 위치와 Unpop→재등록한 이전/새 위치의 부분 Draw/Present 픽셀을 확인한다. 실제 자산 읽기 검사는 패치 3,783/CD 3,167개, 합계 **6,950개** Squid 표시 헤더를 연결했다. 순수 VFX 거부와 잘못된 표시 연결의 사전 거부도 검사했다.

클론 renderer/world GUI 회귀를 이번 PC에서 다시 실행했다. Renderer 글꼴·커서·화면 비교와 기존 GameWorld의 1-1/TEST01 선택·이동·정지·카메라·재진입을 확인한다. 결과는 `extracted/cpp-renderer-smoke/report.json`, `extracted/cpp-world-smoke/report.json`에 있다. 도구가 허용된 설정 파일을 복구하고 원본 파일 해시를 확인했다. 이 GUI는 **기존 임시 GameWorld** 검사이고 raw SID 월드 연결의 증명이 아니다.

누적 **52,484개**는 제한 x86 기대값 입력 수다. 기존 878개 공간 전이는 의존성 계약 대체, 480개 수명 입력은 접두 구간, 4개 해시 초기화는 할당 없는 경로다. 생성자 359행/vtable 151행은 호출 수와 별도다. 이 수치는 미션 완주나 게임 전체 완성도가 아니다.

## 다음 작업

공통 postPop의 실제 영역/소유자/생산·dirty/grid 변경, 표면 부착 프레임/이웃 통지와 섬/다리/건물/파생 가상 효과, 삭제/의존 객체/참조 수명과 SID 소진/Take 목록을 이어 복원한다. 이후 raw SID·표면·실제 프레임을 GameWorld에 연결하고 다리 배치/Construction·경제·전투·AI·승패를 진행한다.
