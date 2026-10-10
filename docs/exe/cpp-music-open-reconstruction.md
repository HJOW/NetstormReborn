# 음악 파일 열기·곡 길이·공개 시작 복원

2026-10-10, 마지막 디컴파일 수행 PC **HJOW-Athlon**(192.168.0.94). [musicopen 목록](../../tools/ghidra/musicopen-functions.json) **3/2/2개**를 같은 PC의 세 Ghidra 프로젝트에서 읽기 전용으로 내보냈다. 원본 게임·복사본·창·오디오 장치·작업 스레드는 실행하지 않았다.

[SoundMusic](../../cpppj/src/client/SoundMusic.h)에 `MusicChannel::Open`·`Active`, `SoundMusic::Play`를 추가했다. [MusicFileStore](../../cpppj/src/client/SoundMusicFile.h)는 실제 Winmm 음악 파일을 열린 상태로 소유하고 기존 `Read`·`Rewind`·`Close`·`Stop`에 연결한다. `ReadSoundWave`의 효과음 전체 적재와 달리 **헤더와 요청한 표본 구간만 읽는다.** 상위 음악 옵션 관리자 본체는 [다음 단계](cpp-music-selection-reconstruction.md)에서 완료했다. 두 채널/이벤트/스레드·실제 오디오/GUI 연결은 후속이다.

## 주소와 계약

| 역할 | 10.78 | CD/추가 10.37 |
|---|---|---|
| 공개 음악 시작 | `004aace0` | `004393b0` |
| 경로 선택·열기·길이 | `004aa220` | 공개 시작 내부 인라인 |
| 실제 WAVE 헤더 열기 | `004a8b50` | `004375c0` |
| 파일 경로 조회 경계 | `0041ad10` | `004bb6d0` |
| 실패 때 상위 음악 옵션 선택 | `00435200` | `00484aa0` |
| 기본/보조 디렉터리 전역 | `005c7b40` / `005c7b3c` | `0051a748` / `0051a744` |

공개 시작은 음악 준비·장치 존재·비활성·nonnull 이름을 검사한다. 통과하면 잠금 안에서 기본 경로 조회→필요 시 보조 조회→실제 헤더 열기→길이 저장, 기존 버퍼 보존 또는 고정 버퍼 생성, 중첩 잠금의 되감기/활성화를 순서대로 수행한다. loop가 0이면 flags=1, 그 외에는 3이며 쓰기 위치는 0이다. **실제 COM Play는 이후 Update의 첫 채우기에서 수행한다.**

원본은 디렉터리 포인터가 있으면 끝에 역슬래시를 무조건 붙인다. 빈 문자열 포인터와 null은 다르고, 후행 구분자가 있어도 하나 더 붙인다. **첫 조회 실패 뒤 보조 디렉터리가 null이면 기존 검색 문자열에 이름을 한 번 더 붙인다.** 예를 들어 기본=`music`, 보조=null, 이름=`track.mus`이면 두 조회는 `music\track.mus`, `music\track.mustrack.mus`다. 이 동작을 임의로 정상화하지 않았다. 두 조회 실패의 채널 0x13 로그는 null `%s` 경계의 `(null)` 문자열로 기록한다.

열기 실패 때만 이름을 ASCII 대소문자 구분 없이 `demo.mus`와 비교한다. 이미 demo라면 0을 반환하고, 그 외에는 상위 옵션 경계에 demo를 요청한 뒤 활성 비트를 반환한다. 버퍼 생성/되감기 실패에는 fallback하지 않는다. **상위 옵션 함수는 현재 곡 이름·음악 옵션·특수 곡 loop·이전 음악 정지를 함께 관리하므로 단순 재귀 Play로 대체하지 않았다.** 이 단계에서는 `MusicOpenHooks::fallback`에 명시 콜백을 남겨 독립 검사에 선택 사건/활성 반환만 공급했다. 이후 [상위 음악 선택 단계](cpp-music-selection-reconstruction.md)에서 **실제 옵션 래퍼/선택 본체를 연결하고 상위 fallback 대체 없이 원본과 대조**했다. GUI 옵션/게임 연결은 남는다.

## 실제 파일 소유권과 부분 출력

`MusicFileStore::Open`은 `mmioOpenW(MMIO_ALLOCBUF|MMIO_READ)`→WAVE RIFF→fmt→최대 18바이트 읽기→fmt Ascend→data Descend를 사용한다. 짧은 PCM fmt는 원본처럼 cbSize=0, bits=`(평균 바이트율 << 3) / (채널 수 × 샘플률)`로 보정한다. 짧은 비PCM은 원본 조건식/줄 0xeb를 보고한 뒤 진행한다.

헤더의 File은 열린 직후 바뀌며 실패해도 닫힌 토큰이 남을 수 있다. format도 읽힌 단계까지 변경하고, Length/DataOffset/Duration은 실패 전 값을 보존한다. 성공하면 data 시작에 열린 파일을 남기고 Length/DataOffset을 저장한다. 경로만 실패하면 헤더는 건드리지 않는다. 이 부분 출력과 raw96 전체의 패딩/관련 없는 필드 보존을 원본 관찰과 대조했다.

호스트 Winmm 핸들은 채널의 32비트 raw에 넣지 않는다. 단조 증가하는 토큰으로 관리하며 **닫힌 토큰을 재사용하지 않는다.** 원본의 실패 뒤 남은 토큰을 재닫아도 새 파일에 영향을 주지 않는다. 실패/예외에서 열린 파일을 반납하고, 소유자 종료는 아직 열린 파일 전체를 닫는다. 채널 콜백보다 소유자가 오래 살아야 하며 음악 Stop은 장치 Shutdown 전에 수행한다.

원본 버퍼를 넘는 조회 문자열/NUL은 예외로 거부한다. 헤더의 0 분모·불완전 fmt·signed IO 범위 밖 길이/위치·소유하지 않는 추가 코덱 자료도 안전한 실패로 처리한다. 이러한 정의되지 않은 입력의 호스트 방어를 native 동치로 주장하지 않는다. resolver의 UTF-8 문자열은 Windows 유니코드 경로로 전달하며 임시 비ASCII 음악 이름도 확인했다. 현재 파일/COM 토큰 표는 단일 스레드 전용이다.

## 곡 길이의 x87 정밀도

Ghidra의 10.78 출력에는 중간 float 캐스트가 있지만 실제 명령에는 float32 저장이 없다. 10.78은 `length / (bits × 0.125) / rate / channels`, CD는 `length / (((bits × 0.125) × channels) × rate)`의 x87 순서다. 샘플률은 uint32로 해석하며 10.78의 음수 signed 로드 뒤 `4294967296` 보정도 관찰했다.

두 제어값 `0x027f`(53비트), `0x037f`(64비트)의 관찰을 모두 저장한다. **C++ double 계산은 53비트 관찰과 전체 raw96를 정확히 비교한다.** 10.78의 유효 합성 PCM **72개**는 두 제어값에서 duration 저장값이 **1 ULP** 차이 났다. CD/추가 10.37의 이번 입력에는 차이가 없었다. 64비트 중간 연산까지 모두 비트 단위로 같은 구현은 아니다. 차이를 감추거나 Ghidra의 float32 캐스트를 기대값으로 쓰지 않았다. 실제 음악 9개는 모두 22050 Hz·stereo·16비트이며 아래 자산 검사는 C++ 파일/표본 검사다.

## 독립 관찰과 검증

[실행기](../../tools/decomp_musicopen_oracle.py)의 독립 입력은 **세 판본 각각 624개, 총 1,872개**다. 두 제어값에서 공개 시작 **3,744회**, 실제 헤더 **3,234회**, 초기 실제 Lookup까지 root **11,232회**가 정상 반환했다. 기본/보조 null·빈/후행 구분자·조회 실패·demo 비교·준비/장치/활성·PCM fmt 16/18·홀수 청크·단/다중 채널·일반/높은 unsigned 샘플률·bits·data 길이·헤더/생성 HRESULT/출력/seek 실패·기존 버퍼·loop를 교차했다.

파일 조회/상위 옵션 callback·COM·Winmm 파일/청크·잠금/기록·패치 CRT 로캘·초기 배치만 명시 대체한다. 경로 문자열 구성/헤더/길이/공개 시작과 기존 생성/되감기/helper는 실제 원본 명령이다. **앞 단계에서 정적 대조만 했던 CD 버퍼 생성과 시작도 이번 공개 시작 입력에서 실제로 실행했다.** [fixture](../../cpppj/tests/fixtures/musicopen-x86.tsv)는 사건/반환·53/64비트 raw96·파일 위치/열림을 기록한다. [근거](../../cpppj/recovery-musicopen-evidence.json)는 SHA/정확한 전체 입력·행·진입/반환·ABI/스택/보존 레지스터/SEH/x87·허용 쓰기·잠금 균형·OS/예기치 않은 assert 0을 감사한다. 기존 fixture/감사 도구는 수정하지 않았다.

[SoundMusicOpenTests.cpp](../../cpppj/tests/SoundMusicOpenTests.cpp)의 새 검사 7개는 실제 Winmm 임시 파일로 모든 native 행을 대조한다. 공개 Play→실제 파일→첫 링 채우기→파일 끝 반복/한 번 재생 EOF→Unlock/Stop, 헤더 부분 출력·만료 토큰 재닫기·호스트 방어·UTF-8 경로/파일 콜백 보존도 확인한다. 일반 CTest에는 원본 자산/PE/Python/오디오 장치가 필요 없다.

선택 읽기 전용 검사 `--inspect-music-files originals`로 기존 **9개 MUS·126,276,008 표본 바이트**의 헤더/곡 길이·앞/뒤 최대 256바이트·되감기·닫기를 확인했다. 표본 전체를 적재하지 않았고 원본 파일은 변경하지 않았다. 전체 음원을 듣거나 오디오 장치로 재생한 검사는 아니다.

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name musicopen
python -X utf8 tools/decomp_musicopen_oracle.py
python -X utf8 tools/decomp_musicopen_oracle.py --verify
cmake --build cpppj/build --config Release --parallel 4
cpppj/build/bin/Release/netstorm_tests.exe --filter SoundMusicOpen_
cpppj/build/bin/Release/netstorm_tests.exe --inspect-music-files originals
ctest --test-dir cpppj/build -C Release --output-on-failure
```

누적 독립 관찰 **437,605개**. 최종 회귀/감사 결과는 [LEFT_JOBS.md](../../LEFT_JOBS.md)에 기록한다. 상위 음악 옵션 본체는 [후속 단계](cpp-music-selection-reconstruction.md)에서 완료했다. 다음은 두 채널·이벤트/작업 스레드 초기화/종료·장치/파일 토큰 표 동기화와 GUI 옵션/클라이언트 연결이다. 실제 소리 청취·GUI 건설·경제·전투·AI·승패도 남아 미션 완주는 아직 불가능하다. 영어 원본 글꼴·outpost/LAN 3차·한국어/D2Coding/화면 요구사항/MCP 4차 순서를 유지한다.
