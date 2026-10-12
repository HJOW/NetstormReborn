# cpppj VFX 그림의 회전 없는 16.16 배율 출력

작업일 **2026-10-12**, PC **HJOW-Athlon / 192.168.0.94**. AGENTS.md와 두 LEFT_JOBS를 읽고 같은 PC의 정밀 디컴파일 기록을 확인했다. 두 정밀 프로젝트에서 관련 함수 **11/11개**를 읽기 전용으로 내보냈다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-12.**

## 구현과 원본 주소

[VFXTransform.h](../../cpppj/src/client/VFXTransform.h)·[VFXTransform.cpp](../../cpppj/src/client/VFXTransform.cpp)에 `DrawScaledVfxImage`를 추가했다. 원본 변환 커널의 **angle=0, 임시 표면 새로 작성** 분기를 복원한다. signed 16.16 x/y 배율을 사용하며 음수 배율은 해당 축을 반전한다. 다각형의 두 변을 따라가는 원본 순회를 직교 사각형의 행/열 순회로 줄였다.

| 동작 | 10.78 | CD 10.72 |
| --- | --- | --- |
| 일반 / 색 치환 변환 래퍼 | `004daaf0` / `004dab40` | `004d6540` / `004d6590` |
| 전체 변환 커널 | `004028cb` | `004662ab` |
| 기준점 주위 좌표 변환 | `00406620` | `0046a000` |
| 각도별 cos / sin 공급 | `0040653e` | `00469f1e` |
| 압축 영역 크기 / 상대 위치 | `004071f4` / `0040722a` | `0046abd4` / `0046ac0a` |
| 임시 표면 채우기 | `0040498a` | `0046836a` |
| 일반 / 색 치환 RLE 출력 | `00401d92` / `0040229f` | `00465772` / `00465c7f` |
| 현재 변환 표 설치 | `0040227e` | `00465c5e` |

두 변환 커널과 기준점 변환의 디컴파일 제어 흐름은 대응 주소를 제외하면 같다. 각도 0의 실제 ROM DWORD는 cos=**65536**, sin=**0**이다. 임의 보간이나 부동소수점 이미지 크기 반올림은 사용하지 않는다.

## 좌표와 픽셀 선택

변환 입력은 `ShapeFrame.bounds`가 아니라 **실제 압축 픽셀 영역 `rect`**다. 원본 크기 조회의 WORD 폭/높이에서 1을 뺀 값을 텍스처 끝점으로 사용한다. 프레임 상대 left/right/top/bottom을 변환한 뒤 호출 기준점을 더한다. 래퍼의 pane 원점 빼기와 커널의 원점 더하기가 상쇄되므로 표면 API는 절대 화면 기준점을 받는다.

원본 꼭짓점의 축 변환은 다음 정수 연산이다.

```text
shifted = signed32(unsigned32(relativeCoordinate) << 16)
scaled = floor((signed64(shifted) * signedScale + 0x8000) / 2^32)
destination = signed32(unsigned32(anchor) + unsigned32(scaled))
```

`0x8000`은 64비트 곱의 **하위 DWORD에 더해진다**. 통상적인 16.16 반 픽셀 반올림으로 치환하면 원본 끝점이 달라진다. signed 나눗셈의 음수 절삭을 보정해 원본 상위 DWORD와 같은 내림값을 얻는다.

행/열의 텍스처 기울기는 `signedDelta * 65536 / positiveSpan`의 **0 방향 절삭**이다. 시작 텍스처 좌표에는 `0x8000`을 더하고, 각 픽셀에서 상위 WORD를 읽는다. 클립으로 잘린 첫 행/열도 같은 기울기×생략한 간격으로 전진하며, 누산은 하위 DWORD 감김을 보존한다. 변환한 두 x/y 끝점은 모두 출력 대상에 포함된다.

- 음수 배율은 물리적으로 작은 화면 끝점에 대응하는 반대쪽 소스 끝점에서 시작한다.
- 최저/최고 y가 같으면 원본은 아무 행도 그리지 않는다. 한 행짜리 그림의 비단위 변환도 같다.
- x만 한 열로 압축되면 원본의 최저 y 동률 선택과 두 변의 `<=` 비교를 유지한다. 정상 y 방향은 마지막 소스 열, 반전 y 방향은 첫 소스 열을 선택한다.
- 정확히 x/y 배율 65536이면 일반 RLE 출력으로 조기 분기한다. 현재 C++에서는 기존 `DrawIndexedImage`를 사용한다.

## 투명색과 같은 UI 표면 연결

원본은 임시 표면을 **255**로 채운 뒤 일반/색 치환 RLE를 풀고, 변환 중 값 255를 건너뛴다. 따라서 비단위 변환에서는 원본의 불투명 255와 **색 치환 결과가 255인 픽셀**도 대상 색을 유지한다. 배율이 정확히 1인 일반 분기는 opacity를 사용하므로 255도 불투명하게 출력한다.

C++는 같은 결과를 해제 이미지의 opacity/팔레트 번호에서 직접 샘플링한다. 확대 이미지나 임시 전체 표면을 따로 할당하지 않는다. 입력 opacity가 0이면 치환 표를 조회하지 않으며, 불투명 입력은 색 치환 후 255를 검사한다.

`GumpPixelTarget::ScaledImage`는 기존 그림/선과 같은 픽셀 저장소와 **현재 VFX 클립**을 사용한다. `MarkupPixelTarget`은 해제 SHP 캐시에서 같은 프레임을 조회해 scaled 요청의 기준점/배율/현재 변환 표를 전달한다. Markup이 생성하는 회전 인자는 0이므로 이 경로에서 배율 출력을 처리한다. 회전 요청은 기존 shape 훅으로 넘기며 없는 경우 해당 경계에서 거부한다.

## 지원 범위와 남은 작업

원본 signed WORD/16.16 경로가 정상 버퍼를 읽는 범위에 한정한다. 해제 이미지의 폭/높이는 **1~32768**, 프레임 상대 좌표와 변환 끝점은 **-32768~32767**, 변환 후 각 축 간격은 **32767 이하**다. 실제 프레임 영역/해제 크기/투명 저장소와 대상 용량을 먼저 검사한다. 범위 밖 입력은 예외로 거부한다. 원본의 감긴 좌표가 임시 버퍼 밖을 읽는 동작은 재현하지 않는다.

후속은 **회전 다각형**, 임시 표면 재사용 flag 2, 원본 그림 변환 표의 공급, 대괄호 그림/옵션 파서, StyleText 다중 줄/줄바꿈/스크롤과 메뉴/Form·입력·자동 raw UI 연결이다. **Client/UberGump는 기존 메뉴 어댑터이며 raw 자동 GUI와 건설·미션 완주는 미완료다.**

## 빌드와 근거

게임 라이브러리/`NetstormCpp` **Release 빌드 경고 0·오류 0**. SOURCE_MAP 생성은 내용 차이가 없었다. **새 테스트 추가/실행·전체 CTest·독립 원본 픽셀 대조·게임/GUI 실행은 미수행**이다. 구현의 동일성은 정적 근거에 기반하며 실제 원본/C++ 픽셀 대조는 남아 있다.

원본 관찰 **523,634개**, C++ 대조 **523,374개**, CTest **854개 통과**는 앞 단계 기록이다. 이번 분석에서 이 수치를 늘리지 않았다.

자료: `cpppj/recovery-vfxscale-evidence.json`, `tools/analyze_vfxscale.py`, `tools/ghidra/vfxscale-functions.json`, `extracted/vfxscale/`, `extracted/vfxscale-originals-export.log`, `extracted/vfxscale-originalCD-retry-export.log`, `extracted/vfxscale-build.log`. 초기 CD 내보내기는 Ghidra 스크립트 번들 로드 오류로 실패했고 이후 다시 내보내 **11개 모두 완료**했다. 정적 보고서는 실제 함수 범위/명령/호출·반환/ROM과 현재 코드 SHA를 기록한다. 앞 단계 Markup 보고서는 당시 연결 코드 기록이며 이번 변경은 새 VFX 보고서를 따른다.

```powershell
python -X utf8 tools/analyze_vfxscale.py
cmake --build cpppj/build --config Release --target netstorm NetstormCpp --parallel 2
```

원본 두 바이너리 SHA는 기존 기록과 같다. AGENTS.md·보호 원본·dotnetpj 변경, 커밋/푸시 없음.
