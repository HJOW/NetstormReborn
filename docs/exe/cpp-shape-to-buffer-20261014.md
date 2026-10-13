# 실제 메모리 프레임 캡처

## 현재 자료

- 주 대상은 패치 **10.78**, 비교 대상은 CD **10.72**다. 현재/마지막 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-14**다.
- 새 독립 캡처 관찰은 **164정상/assert0**이며 패치80/CD84다. 이전 DudeSpawn288·Burning warm생성자256·Append564·Sources652는 재사용하는 하위 근거로만 연결하고 새 관찰 수에 더하지 않는다.
- 실제 `ShapeToBuffer`→현재 TYPE/header→투명 버퍼→plain VFX RLE→원래 메모리 문맥20바이트 복원→TYPE 추가 기준점을 원본 몸체로 실행했다. **캡처/RLE 대체0**, 외부 경계는 요청 크기와 격리 반환을 기록하는 픽셀 heap만 있다.
- C++ 새 `RawShapeToBuffer`는 실제 보호 SHP와 기존 `DrawShape` 몸체를 재사용한다. **공유 빌드/C++2검사의 전수 대조는 아직 미수행**이다.

| 기능 | 패치10.78 | CD10.72 |
| --- | --- | --- |
| 캡처 상위 | `004a7db0` | `0045b100` |
| TYPE getter | `004ac550` | `004abf50` |
| main frame header | `00419850` | 상위 인라인 |
| plain 그리기 wrapper | `004daa70` | `004d64e0` |
| 실제 VFX plain RLE | `00401d92` | `00465772` |

원본은 Squid 추가 헤더의 width/height/hotspot을 fixed16 배율로 곱한다. 새 버퍼 전체를0xff로 채우고 메모리 pane을 해당 버퍼로 잠시 바꿔 실제 plain RLE를 그린다. RLE 스트림을 새 크기에 맞춰 확대하는 경로는 아니다. 그리기를 마친 뒤 원래 문맥을 복원하고 옵션이 켜졌을 때 TYPE 기준점만 결과에 추가한다. C++에서는 명시 pane과 현재 벡터 버퍼를 같은 기존 VFX 몸체에 넘기므로 전역 화면 문맥을 바꾸지 않는다.

## 격자와 보호 자산

양판본은 보호 Dude70의 main frame **0/1/7/32/64**, fixed16 배율 **32768/65536/98304/131072**, TYPE 기준점 보정0/1, 현재 정상/dead를 교차한 각80입력을 실행한다. TYPE의 추가 기준점은 현재 입력−2/3이다. owner0/plain, 실제 가상 표와 같은 protected SHP를 공급한다. 슬롯50의 raw 상태/frame은 명시 현재 입력이며 이번 관찰이 source의 전체 생성/삭제를 새로 실행했다는 뜻은 아니다.

I64의 보호 레코드는 특수 영역을 가진 빈 프레임이다. 원본은 캡처 헤더7×7/기준점13×7을 반환하고 실제 RLE는 화면 밖 영역을 거부해 전체49바이트가 투명으로 남는다. C++에서도 같은 raw 헤더와 VFX 클리핑을 재사용하며 디코딩한 그림이 있다고 만들지 않는다.

CD의 추가4입력은 source free, 가상 표0, 해제 표식dddddddd, frame255이다. 모두 실제 **8×8/기준점4×4/전체64바이트0xff** fallback으로 정상 반환한다. 패치는 그 상태에서 원본 진단을 요구하므로 정상 캡처164에 진단 입력을 섞지 않는다. 현재 C++의 패치 invalid 상태도 명시 예외다.

결과는 메타데이터 네 정수·전체 픽셀 버퍼·실제 heap 요청 크기·원래 문맥20바이트 복원이다. 버퍼 체크섬만 비교하지 않는다. 실제 TYPE/물리 SHP·PE·불연속 함수 범위와 기존 C++ VFX 몸체의 전체 SHA를 보고서에 기록한다.

## 실행 환경과 남은 범위

VFX asm은 실제 `push ds/pop es`를 실행한다. 초기 Unicorn의 null selector를 그대로 재적재하면 스택 주소가16비트로 잘리거나 descriptor가 사라져 정상 반환 전 실패했다. 새 oracle은 현재 입력 준비 뒤 **4GB/32비트 code/data GDT**를 명시 공급한다. 코드/자료/스택의 평면 의미를 복원한 것이며 원본 명령을 건너뛰지 않는다. 해당 실패 시도를 정상/신규 완료 관찰로 세지 않는다. 게임/운영체제/창/DirectDraw 호출은0이다.

owner 색표가 필요한 mapped RLE와 추가 가상 그림은 현재 C++에서 호출 전에 거부한다. 일반 자산의 실제 owner 색표/건설 추가 그림, Burning 전체 Frame/Draw/첫 팔레트/Host 수명 및 캠페인1-1 월드는 후속이다. 이 단위는 실제 프레임 캡처 하위 경계를 닫는 자료이며 전체 Burning 효과 완료를 뜻하지 않는다.

## 자료와 등록

- production: `cpppj/src/client/RawShapeToBuffer.h/.cpp`.
- test: `cpppj/tests/ShapeToBufferTests.cpp`의 양판본2검사.
- fixture: `shape-to-buffer-x86.tsv`, 매크로 **`NETSTORM_SHAPETOBUFFER_FIXTURE`**.
- oracle/report: `tools/decomp_shape_to_buffer_oracle.py`, `cpppj/shape-to-buffer.json`.
- readOnly helper: `burning-frame-helper`9/7, `burning-frame-paint-helper`7/6, `burning-frame-raster-helper`6/4. 기존18/19·Sources8/7 및 별도 CD1/1 자료를 덮어쓰지 않았다.

```text
python -X utf8 tools/decomp_shape_to_buffer_oracle.py --verify
```

위164고유입력/전체픽셀/현재 SHA 감사는 성공했다. 단위14에서 등록한 양판본2검사가 현재 공유 C++ 전수 대조에서 모두 통과했다(`run14-ShapeToBuffer_first.log`). 단위14 전체 결과는 아직 진행 중이며 이 개별 성공을 전체 완료로 표시하지 않는다. 실제 BurningFrame426의 새로운 소비자 대조는 단위15 범위다.
