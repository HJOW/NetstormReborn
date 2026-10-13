# Burning의 실제 프레임 소비자

현재 PC와 마지막 정밀 디컴파일 PC는 **HJOW-Athlon**이다. 주 대상은 패치 **10.78**이며 CD **10.72**의 실제 주소/차이도 함께 복원했다. 단위14의 Sources652/ShapeToBuffer164/DudeSpace36 자료는 동결하고 이 프레임은 **단위15**의 별도 범위로 준비했다.

## 새 몸체와 호출 순서

`o/RawBurningFrame`은 패치 **00424530** / CD **004a60e0**을 담당한다. `client/MakeBurningFrameCapture`가 기존 실제 `RawShapeToBuffer`를 캡처 훅에 연결한다. 같은 pool의 `RawBurningVisual::Append`와 `RawBurningSources::Select`, 같은 게임 난수 상태, 기존 `DirtyRegions` 소비자를 재사용한다. 생성자에서 미초기화인 +40/+44는 새 `BurningFrameFields`에 분리했으며 단위12 생성자 필드를 수정하지 않았다.

1. capacity0은 다른 작업 없이 반환한다.
2. stage0은 실제 timer reset → 캡처 준비 **00423d50/CD004a5c70** → stage1이다. 캡처 준비는 기존 이미지/행 반납, 용량 전체 입자 zero/total0, 실제 현재 ShapeToBuffer, `(width+height)/5` 행 zero, selected0 순서다.
3. stage1은 현재 raw x/y와 실제 픽셀 배율로 사각형 네 좌표를 같게 만들고 signed 부모 깊이−1, off1, stage2를 쓴다.
4. stage2는 이전 사각형을0 방향 정수화한 뒤 카메라를 빼고 다시 정수화하여 현재 viewport와 겹침을 검사한다. 화면 밖이면 off1만 쓰고 필요할 때 실제 sound-stop helper를 호출한다.
5. 화면에 다시 들어오면 lastTime을 now로 맞춰 첫 delta0을 유지한다. 실제 timer, Sources, **capacity 전체**의 입자 순회를 실행한다. 새 뒤쪽 입자는 같은 프레임에 다시 처리될 수 있다.
6. source는 float 난수 `[0,1.4)`와 delta를 비교하고 실제 Append에 속도 `(0,−2)`, 수명 `[4,9)`, sourcefalse를 전달한다. 이동 입자는 life 감소, 바람 추가, x/y 이동, 경계 포함 순서다.
7. 실제 경계 보정 `right+2/bottom+1/left≤right−17/top−2` 뒤 이전·새 영역을 각각 viewport로 clipping하여 기존 dirty 표에 순서대로 등록한다.

## 명령에서 확인한 판본 차이

- timer **00418430/CD0045e2b0**은 `(now−lastTime)*10`을 float delta와 별도 배율 float로 저장한다. elapsed는 **이전 double 그대로 + delta float**다. Ghidra의 이전 elapsed float 절삭 표기는 실제 `fadd qword ptr`와 다르므로 따르지 않았다.
- 패치 이동 vx는 `fst DWORD`로 저장하면서 넓은 x87 합을 남겨 x갱신에 사용한다. CD는 `fstp DWORD` 뒤 저장된 float를 다시 읽어 사용한다.
- 패치는 ordered float life>0, CD는 같은 비트의 signed DWORD>0으로 활성 입자를 고른다. 양수 quiet NaN은 CD에서 이동하며 패치에서는 건너뛴다. 음수 NaN/음수0 및 양수 inf도 전체 출력에서 구분했다.
- 표시는 raw 깊이 BYTE 패치+35/CD+33, 현재 HP는 패치 DWORD+26/CD signed WORD+26다. Dais의 HasHP extra bit1은 패치+40/CD+35다. 이번 독립 격자는 실제 Dude70이므로 Dais 분기 실행을 새 근거로 세지 않는다.

## 독립 원본 관찰과 현재 C++ 출처

`tools/decomp_burningframe_oracle.py`가 두 보호 PE에서 **각213/총426 고유 입력 정상 반환, assert0**을 확보했다. 각 판본에서 실제 Frame213, Capture212, timer96, Sources96, sound-stop48에 도달했다. 실제 Append 호출은 패치70/CD238이다. heap 요청 **424회씩**만 격리 allocator 경계이며 Frame/Capture/Sources/Append/RLE/dirty를 대체한 횟수는0이다. 소리48회는 핸들0의 원본 helper 자연 반환이다. 실제 OS/게임/GUI 실행은 없다.

출력은 전체 사각형/깊이/total, double 시각·elapsed와 두 float, 단계, 캡처 메타데이터와 전체 픽셀, 모든 행 표식/selected/off, 용량 전체24바이트 입자, 난수 상태, 전체 dirty 표/flags와 full 전역, heap 요청크기다. 원본 actual_calls는 경로 증거로 별도 저장하며 C++는 나머지15개 의미 필드를 대조한다. Source 포인터0/this+74만0/1로 정규화한다. 기존 독립 Capture164/Sources652/Append564 관찰을 새426에 더하지 않는다.

최초426의 전체 보고서/source/test/tool/fixture 및 관련 실행기·정밀 내보내기와 실패 smoke까지 **91파일**을 `recovery-history/burningframe-before-extra-offset`에 보존했다. 초안의 Dais extra 위치를 정확히 교정하고, 미컴파일 검사에 기존 생성자 계약의 reserve 훅을 준비했다. 최초 보고서와 fixture/원본 출력/Native 도구는 불변이다. 첫 draft successor 도구/보고서/test 원문도 별도 보존했다. 현재 source/test는 `tools/analyze_burningframe_draft_lineage.py`의 **schema2**, 전체 이전 production/test 정확 역변환과 SHA로 연결한다. **새 원본 입력0**이다.

```text
python -X utf8 tools/analyze_burningframe_draft_lineage.py --verify
```

최초 Native 도구의 직접 `--verify`는 당시 production SHA를 확인하므로 교정된 현재 source를 보고 실패하는 것이 맞다. 현재 감사는 위 successor가 보존한 최초426의 모든 SHA와 현재 역변환을 함께 확인한다.

## 공유 등록과 남은 연결

새 production은 `src/o/RawBurningFrame.cpp`, `src/client/BurningFrameCapture.cpp`이다. 새 검사는 `tests/BurningFrameTests.cpp`의2개이며 `NETSTORM_BURNINGFRAME_FIXTURE`를 `tests/fixtures/burningframe-x86.tsv`에 연결한다. 현재 **C++ 공유 빌드/대조는 미실행**이다. parent가 단위15에서 등록/빌드한다.

이번 원본 환경은 이미 확보된 표시 객체/capacity12 입자, 현재 보호 Dude TYPE/SHP, 현재 진단 전역0을 명시 입력으로 공급한다. 첫 팔레트 cache0/전체 생성자·Kernel 부착·Draw·Host 삭제를 새 완료로 세지 않는다. 실제 nonzero 소리 정지, 재캡처 free, mapped owner 색표/추가 가상 그림, 실제 palette/Draw/수명과 캠페인 월드 연결은 후속이다. nonzero sound에서 실제 stopSound가 없으면 C++는 해당 도달을 예외로 거부한다. 좌표와 시각의 관찰 범위는 현재 격자의 유한 값이다.

첫 smoke의 `setup` 이름이 기존 PE 환경 준비 메서드와 충돌한 개발 실패는 전체 초안5파일과 실제 실패 로그로 보존했다. `setup_frame`으로 분리한 뒤 정상426을 얻었으며 실패 smoke/중복 진단 재방문을 정상 고유 수에 더하지 않았다.
