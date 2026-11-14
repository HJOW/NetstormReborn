# webpj 점 명령 등록·전역 표·분배 RawDotCommandRegister/ClientDotCommandGlobals/RawDotCommandDispatch (WB04-E443, 2026-11-14)

RawMenuControls 선행 **RawDotCommandRegister 76줄·ClientDotCommandGlobals 56줄·RawDotCommandDispatch 125줄(기준선 `1f00a25a`의 .cpp/.h 줄 수, 합계 257줄, todo→port)**을 새 모듈 [`www/ui/dot-command-register.js`](../webpj/www/ui/dot-command-register.js)·[`www/ui/dot-command-globals.js`](../webpj/www/ui/dot-command-globals.js)·[`www/ui/dot-command-dispatch.js`](../webpj/www/ui/dot-command-dispatch.js)로 이식했다. 기준선 전체 소스를 읽었다. **전용 PE fixture는 없다.** cpppj에도 직접 시험이 없고 아래 검사는 기준선 소스 계약과 손 계산 바이트로 한 것이며 원본 직접 관측 수치가 아니다.

## 기준선 뒤 변경 (3.6절)

세 단위 모두 기준선 뒤 변경이 없다.

## 구현

- **표 배치(`RawDotCommandRegister`, 원문 0043f660)**: 이름·함수·표시·count가 한 전역에 인접한 2568(0xa08)바이트다: 이름 32바이트×64칸(0x000..0x7ff), 함수 식별자 DWORD(0x800), 표시 포인터 DWORD(0x900), count DWORD(0xa04). `registerDotCommand`는 진입 count(signed)가 64 이상이면 0(아무것도 쓰지 않음), 음수면 RangeError(원시 주소 감김은 host 지원 밖)다. 이름은 진입 count×32에 31바이트를 CRT `strncpy`처럼(`copyDotCommandName`: NUL 뒤 0 padding, 원본이 NUL 전에 끝나면 RangeError) 복사하고(복사 장치는 `hooks.copy`, 지정하지 않으면 기본, 명시 `null`은 Error), **복사 뒤 현재 count를 한 번만 읽어 보관**한다(복사 장치가 count를 바꿔도 같은 보관 값으로 계속). 함수 표에 식별자를, 32번째 이름 바이트에 NUL을, 표시 표에 `flag`가 비영이면 `이름표 주소(0x00531b30)+칸×32`, 0이면 빈 문자열 주소(0x00503c8b)를 쓰고, count는 현재 메모리가 아닌 보관 값+1로 저장한 뒤 다음 표시 칸을 0으로 종단한다. 모든 DWORD 접근은 실제 인접 물리를 넘으면 RangeError다.
- **`ClientDotCommandGlobals`**: PE 정적 이미지만 비영이다 — 슬롯 0 이름 `say`, 함수 `0x0042f1e0`, 표시 0은 이름표 첫 슬롯, count 1(등록은 슬롯 1부터). 표/검색 상태(`DotCommandDispatchState`: 기본 슬롯, 마지막 Regexp 그룹 3 시작 상대 위치)를 복제 없이 빌려주며 `register`가 같은 표에 위임한다.
- **`dispatchDotCommand`(원문 0043f6f0)**: 빈 입력은 1. 고정 패턴(00506a30)은 선행 space/tab/CR/LF → `.` → 이름(공백·`;`·`$` 앞까지, 80바이트 상한) → 인자(개행 포함, `;`·`$` 앞까지, 127바이트 상한, `$`로 끝나면 일치 없음)이며 실패 시 `separatorOffset`을 -1로 둔다. 일치하면 signed count 안에서 첫 중복 이름을 ASCII 대소문자 무시로 찾아(비ASCII는 변환 안 함) 앞 공백을 포함한 인자(NUL 포함)로 호출하고, 일치하지 않으면 현재 기본 슬롯에 입력 전체(NUL 포함)를 넘긴 뒤 1을 돌려준다. 호출 코드가 0이 아니면 **호출 뒤의 현재 표**에서 이름을 읽어 오류 장치에 넘긴다. 성공한 명령은 호출 뒤 **현재** `separatorOffset`(중첩 분배가 바꿀 수 있음)으로 다음 위치로 가서 `;`를 건너뛰고 남은 입력이 비면 1, 아니면 같은 방식으로 이어간다. 다음 위치가 입력 밖이거나 NUL이 없는 입력/표는 RangeError, 함수/오류 장치 누락은 도달한 위치에서만 Error다.
- 웹 적응 표기: `std::span<std::uint8_t>`→`Uint8Array`, `std::function` 기본 초기화(`copy`)는 지정하지 않으면 기본 구현, `out_of_range`→`RangeError`, `logic_error`→`Error`, 이름 입력은 바이트 배열 또는 바이트 문자열.

## 검사와 발견

새 시험 파일 1개, **14건 통과**(`dot-command.test.mjs`): 이름 복사/등록의 손 계산 바이트(슬롯 32바이트·함수/표시/count 위치)와 PE 정적 이미지의 비영 바이트 열 개, 분배는 `.help me;.QUIT`·선행 공백·기본 슬롯·오류 이름·중첩 분배 등을 값으로 확인했다.

**최초 실패와 교정(제품 결함 아님, 시험 기대값 오류 1건):** 중첩 분배가 공유 `separatorOffset`을 5로 남기면 바깥 분배는 현재 값으로 `' a;.quit'` 위치를 잡아 일치하지 않는 입력으로 기본 슬롯에 넘긴다는 점(C++ 순서를 다시 읽어 확인)을 바로잡았다.

**변형 확인:** 연산자/정수 상수 변형 191개(register 69·dispatch 105·globals 17) 중 173개를 잡았다. 처음 생존 39개 중 21개는 보강으로 잡았다: 원문 주소 식별자 값, 원본이 정확히 1바이트 짧을 때의 오류, count 0 첫 슬롯 등록, 비영 바이트 열 개, A/Z/@/[ 경계의 소문자 접기, 이름 모든 글자 비교, 검색 실패의 -1, 선행 공백 1/3칸, 구분자 앞에서 끝나는 홀수 길이 이름, 첫 슬롯(say), 입력 뒤 NUL 아닌 바이트가 더 있는 기본 슬롯 호출, 다음 위치 0, 표가 DWORD 보다 2바이트 짧은 경우. 남은 18개는 동등 변형이다: `undefined` 읽기가 같은 결과가 되는 범위 끝 변형·메시지만 다른 RangeError 경계·표 이름(31바이트)보다 긴 이름 상한(80↔81)·도달할 수 없는 signed 위치 상수·크기만 다른 임시 버퍼·count의 최하위 비트 비교·`physical(…, 1)`의 개수.

## 남은 경계

- Chat 입력이 이 분배기를 부르는 연결, 실제 명령 함수 식별자의 호스트 실행(say/flash/desc/cheatorama는 이미 이식한 `issue-commands.js`의 콜백 번호로 연결), 명령 등록 호출 위치(Client 초기화)는 별도 단위다. 앱은 아직 간소화판이며 실제 미션은 플레이할 수 없다. 원본 파일 변경과 게임/브라우저 실행은 없다.
