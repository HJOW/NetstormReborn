// 실제 로그의 공유 1024문자 버퍼·현재 스트림과 출력 순서를 복원한다.
namespace Netstorm.Client;

// 추적 로그 형식을 담는 네임스페이스다. 2026-11-25 R4-7c3 진단 전체 Client 연결에서 추가됐다.

// 원본 FILE*는 호스트의 실제 Stream 또는 검사의 명시 토큰으로 표현한다. null이면 스트림 없음이다.
// cpppj client/RawTraceLog.h의 TraceLogStream(uintptr_t)을 Stream 참조로 옮겼다.
public sealed class TraceLogState
{
    /// <summary>00542658의 PE 초기값 29다. 덤프 부모는 잠시 30으로 쓴다. 2026-11-25 R4-7c3 진단 전체 Client 연결에서 추가됐다.</summary>
    public int Level { get; set; } = 29;

    /// <summary>005ca8d0의 현재 Stream이다. null이면 처음 출력 때 표준 출력으로 연다. 2026-11-25 R4-7c3 진단 전체 Client 연결에서 추가됐다.</summary>
    public Stream? Stream { get; set; }

    /// <summary>005ca8d4의 현재 들여쓰기다. signed 양수일 때만 공백을 넣는다. 2026-11-25 R4-7c3 진단 전체 Client 연결에서 추가됐다.</summary>
    public uint Indent { get; set; }

    /// <summary>005ca8dc의 현재 출력 복제 관문이다. 0이 아니면 표준 오류에도 쓴다. 2026-11-25 R4-7c3 진단 전체 Client 연결에서 추가됐다.</summary>
    public uint Mirror { get; set; }

    /// <summary>005ca8d8의 현재 즉시 flush 관문이다. 0이 아니면 매 출력 뒤 flush한다. 2026-11-25 R4-7c3 진단 전체 Client 연결에서 추가됐다.</summary>
    public uint Flush { get; set; }

    /// <summary>005ca4d0의 같은 물리 버퍼다. NUL 뒤 stale 문자를 보존한다. 2026-11-25 R4-7c3 진단 전체 Client 연결에서 추가됐다.</summary>
    public char[] Buffer { get; } = new char[1024];
}

// 게임 formatter 이후의 실제 출력 장치다. 호출 뒤 현재 상태·버퍼를 다시 읽는다.
// cpppj client/RawTraceLog.h의 TraceLogHooks를 옮겼다. 2026-11-25 R4-7c3 진단 전체 Client 연결에서 추가됐다.
public sealed class TraceLogHooks
{
    /// <summary>00434f60의 trace→Chat 전달 몸체다. 조건 판단도 실제 장치가 수행한다. 2026-11-25 R4-7c3 진단 전체 Client 연결에서 추가됐다.</summary>
    public required Action<string> Console { get; init; }

    /// <summary>fprintf(FILE,"%s",현재공유버퍼)다. NULL도 이 경계로 전달한다. 2026-11-25 R4-7c3 진단 전체 Client 연결에서 추가됐다.</summary>
    public required Action<Stream, string> Write { get; init; }

    /// <summary>fflush의 현재 Stream이다. 콜백 뒤 예전 포인터를 재사용하지 않는다. 2026-11-25 R4-7c3 진단 전체 Client 연결에서 추가됐다.</summary>
    public required Action<Stream> Flush { get; init; }
}

// 원문 공유 버퍼·현재 Stream의 출력 순서를 그대로 실행한다. 2026-11-25 R4-7c3 진단 전체 Client 연결에서 추가됐다.
public static class RawTraceLog
{
    /// <summary>버퍼 크기다. 원문 1024BYTE와 같다. 2026-11-25 R4-7c3 진단 전체 Client 연결에서 추가됐다.</summary>
    public const int BufferSize = 1024;

    /// <summary>스트림이 없을 때 여는 표준 출력이다. 한 번 열어 계속 쓴다. 2026-11-25 R4-7c3 진단 전체 Client 연결에서 추가됐다.</summary>
    private static Stream? _standardOut;

    /// <summary>복제 출력이 쓰는 표준 오류다. 한 번 열어 계속 쓴다. 2026-11-25 R4-7c3 진단 전체 Client 연결에서 추가됐다.</summary>
    private static Stream? _standardError;

    /// <summary>같은 공유 버퍼의 첫 NUL까지 빌린다. 이전 출력 문자열의 사본을 재사용하지 않는다.</summary>
    /// <param name="state">현재 로그 상태다.</param>
    /// <returns>현재 공유 버퍼 문자열이다.</returns>
    private static string Current(TraceLogState state)
    {
        ArgumentNullException.ThrowIfNull(state);
        int end = Array.IndexOf(state.Buffer, '\0');
        if (end < 0)
        {
            throw new InvalidOperationException("trace 로그 공유 버퍼 NUL 없음");
        }

        return new string(state.Buffer, 0, end);
    }

    /// <summary>원문 004c2630/004c2600/004c2610의 실제 전역 읽기·쓰기 순서다.</summary>
    /// <param name="state">같은 주소에서 읽고 쓰는 실제 전역이다.</param>
    /// <param name="level">출력 수준이다. 음수이거나 현재 수준보다 크면 버린다.</param>
    /// <param name="formatted">서식 변환된 입력이다. 첫 NUL 뒤는 버린다.</param>
    /// <param name="prefixBytes">원문 format의 선행 색상 쌍 BYTE 수다. 짝수이며 '~' 쌍이어야 한다.</param>
    /// <param name="hooks">같은 수명의 실제 출력 장치다.</param>
    public static void Write(TraceLogState state, int level, string formatted, int prefixBytes, TraceLogHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(formatted);
        ArgumentNullException.ThrowIfNull(hooks);
        if (level < 0 || level > state.Level)
        {
            return;
        }

        int nul = formatted.IndexOf('\0');
        if (nul >= 0)
        {
            formatted = formatted.Substring(0, nul);
        }

        if (prefixBytes < 0 || prefixBytes > formatted.Length || (prefixBytes & 1) != 0)
        {
            throw new ArgumentException("trace 로그 format 접두 범위", nameof(prefixBytes));
        }

        // 선행 색상 쌍을 확인하는 반복문이다.
        for (int i = 0; i < prefixBytes; i += 2)
        {
            if (formatted[i] != '~')
            {
                throw new ArgumentException("trace 로그 format 접두 쌍", nameof(prefixBytes));
            }
        }

        uint spaces = unchecked((int)state.Indent) > 0 ? state.Indent : 0u;
        if (spaces >= BufferSize || formatted.Length >= BufferSize - (int)spaces)
        {
            throw new InvalidOperationException("trace 로그 원문1024BYTE 범위");
        }

        string line = formatted.Substring(0, prefixBytes) + new string(' ', (int)spaces) + formatted.Substring(prefixBytes);
        line.CopyTo(0, state.Buffer, 0, line.Length);
        state.Buffer[line.Length] = '\0';
        if (state.Stream is null)
        {
            _standardOut ??= Console.OpenStandardOutput();
            state.Stream = _standardOut;
        }

        int end = line.Length;
        bool stripped = false;
        // 끝 개행 전부를 지우는 반복문이다.
        while (end > 0 && (state.Buffer[end - 1] == '\r' || state.Buffer[end - 1] == '\n'))
        {
            state.Buffer[--end] = '\0';
            stripped = true;
        }

        if (stripped)
        {
            state.Buffer[end] = '\n';
            state.Buffer[end + 1] = '\0';
        }

        hooks.Console(Current(state));
        if (state.Mirror != 0 && state.Stream is not null)
        {
            _standardError ??= Console.OpenStandardError();
            hooks.Write(_standardError, Current(state));
        }

        hooks.Write(state.Stream!, Current(state));
        if (state.Flush != 0)
        {
            hooks.Flush(state.Stream!);
        }
    }

    /// <summary>004c2610: 현재 Stream이 0이 아닐 때만 같은 실제 flush 장치를 부른다.</summary>
    /// <param name="state">같은 주소에서 읽는 실제 전역이다.</param>
    /// <param name="hooks">같은 수명의 실제 출력 장치다.</param>
    public static void Flush(TraceLogState state, TraceLogHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (state.Stream is not null)
        {
            hooks.Flush(state.Stream);
        }
    }
}
