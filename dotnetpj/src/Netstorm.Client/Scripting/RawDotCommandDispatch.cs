using System.Buffers.Binary;

// 원문 고정 점 명령 문법과 현재 공유 검색 상태를 같은 가변 입력에 적용한다.
namespace Netstorm.Client.Scripting;

/// <summary>실제 함수 식별자와 NUL 포함 가변 인자를 동기 호출하는 장치다.</summary>
public delegate uint DotCommandInvoke(uint callback, Span<byte> argument);
/// <summary>현재 표의 같은 읽기 뷰와 결과를 받는다. 콜백 중 원문 표 변경도 뷰에서 보인다.</summary>
public delegate void DotCommandError(ReadOnlySpan<byte> name, uint result);
/// <summary>선택된 명령/비영 오류의 실제 장치이며 미선택 경계는 요구하지 않는다.</summary>
public sealed class DotCommandDispatchHooks
{
    /// <summary>현재 함수와 실제 입력/지역 인자에 대한 동기 장치다.</summary>
    public DotCommandInvoke? Invoke { get; set; }
    /// <summary>callback 뒤의 현재 이름과 반환 전체 DWORD를 실제 오류 출력에 전달한다.</summary>
    public DotCommandError? Error { get; set; }
}
/// <summary>RawDotCommandDispatch.cpp의0043f6f0과 원문 고정 Regexp 문법이다.</summary>
public static class RawDotCommandDispatch
{
    /// <summary>이름표와 인접한 callback 배열의 실제 위치다.</summary>
    private const uint CallbacksOffset = 0x800;
    /// <summary>원문 이름/인자를 가변 입력 source의 위치와 복사 상한으로 나타낸다.</summary>
    /// <param name="NameStart">source 안 이름 시작이다.</param><param name="NameLength">최대80BYTE다.</param>
    /// <param name="ArgumentStart">앞 공백도 포함하는 인자 시작이다.</param><param name="ArgumentLength">최대127BYTE다.</param>
    private readonly record struct Match(int NameStart, int NameLength, int ArgumentStart, int ArgumentLength);
    /// <summary>현재 실제 DWORD의 폭을 확인하고 x86 little endian으로 읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, uint at)
    {
        if ((ulong)at + 4 > (ulong)bytes.Length) throw new ArgumentOutOfRangeException(nameof(at), "점 명령 분배 DWORD의 실제 물리 범위");
        return BinaryPrimitives.ReadUInt32LittleEndian(bytes.Slice((int)at, 4));
    }
    /// <summary>실제 NUL까지의 가변 물리를 읽기 뷰로 빌리며 인접 슬롯도 같은 물리에서 읽는다.</summary>
    private static ReadOnlySpan<byte> Text(ReadOnlySpan<byte> bytes, uint at)
    {
        if (at >= bytes.Length) throw new ArgumentOutOfRangeException(nameof(at), "점 명령 문자열의 시작 물리 범위");
        ReadOnlySpan<byte> tail = bytes[(int)at..]; int end = tail.IndexOf((byte)0);
        if (end < 0) throw new ArgumentOutOfRangeException(nameof(bytes), "점 명령 문자열의 실제 NUL 없음");
        return tail[..end];
    }
    /// <summary>원문 패턴에 적힌 SP/TAB/CR/LF 네BYTE만 공백이다.</summary>
    private static bool Space(byte value) => value is 32 or 9 or 13 or 10;
    /// <summary>C locale의 ASCII 대문자만 소문자로 바꾸며 다른 BYTE는 유지한다.</summary>
    private static byte Lower(byte value) => value is >= (byte)'A' and <= (byte)'Z' ? (byte)(value + 32) : value;
    /// <summary>원문 이름은 길이와 C locale 바이트의 대소문자만 비교한다.</summary>
    private static bool Equal(ReadOnlySpan<byte> first, ReadOnlySpan<byte> second)
    {
        if (first.Length != second.Length) return false;
        // 비ASCII 바이트도 변환하지 않고 끝까지 비교한다.
        for (int index = 0; index < first.Length; index++) if (Lower(first[index]) != Lower(second[index])) return false;
        return true;
    }
    /// <summary>실패 표식부터 쓰고 고정 이름/인자 캡처와 실제 종료 상대 위치를 갱신한다.</summary>
    private static Match? Search(ReadOnlySpan<byte> text, DotCommandDispatchState state)
    {
        state.SeparatorOffset = -1; int cursor = 0;
        // 패턴 맨 앞의 네 공백만 소비한다.
        while (cursor < text.Length && Space(text[cursor])) cursor++;
        if (cursor == text.Length || text[cursor++] != '.') return null;
        int start = cursor;
        // 이름에는 점/콜론/VT/FF를 허용하며 지정 종료 BYTE만 제외한다.
        while (cursor < text.Length && !Space(text[cursor]) && text[cursor] != ';' && text[cursor] != '$') cursor++;
        if (cursor == start) return null;
        int nameLength = Math.Min(cursor - start, 80), argument = cursor;
        // 인자는 개행도 포함하며 세미콜론/달러 앞에서 멈춘다.
        while (cursor < text.Length && text[cursor] != ';' && text[cursor] != '$') cursor++;
        if (cursor < text.Length && text[cursor] == '$') return null;
        state.SeparatorOffset = cursor; return new(start, nameLength, argument, Math.Min(cursor - argument, 127));
    }
    /// <summary>현재 callback DWORD를 실제 장치 요구보다 먼저 읽는다.</summary>
    private static uint Invoke(ReadOnlySpan<byte> table, uint index, Span<byte> argument, DotCommandDispatchHooks hooks)
    {
        uint callback = Read(table, unchecked(CallbacksOffset + index * 4));
        return (hooks.Invoke ?? throw new InvalidOperationException("점 명령 실제 함수 장치 미연결"))(callback, argument);
    }
    /// <summary>비영 코드에서만 callback 뒤 현재 이름을 읽고 실제 오류 장치를 선택한다.</summary>
    private static void Error(ReadOnlySpan<byte> table, uint index, uint result, DotCommandDispatchHooks hooks)
    {
        if (result == 0) return;
        ReadOnlySpan<byte> name = Text(table, unchecked(index * 32));
        (hooks.Error ?? throw new InvalidOperationException("점 명령 오류 출력 장치 미연결"))(name, result);
    }
    /// <summary>같은 현재 표/가변 입력/공유 캡처로 각 명령과 중첩 callback 뒤의 나머지를 분배한다.</summary>
    public static uint DispatchDotCommand(Span<byte> table, DotCommandDispatchState state, Span<byte> input, DotCommandDispatchHooks hooks)
    {
        int cursor = 0; if (Text(input, 0).IsEmpty) return 1;
        Span<byte> argument = stackalloc byte[128];
        // 원문 지역128BYTE를 재사용하며 실제 작성한 캡처+NUL만 장치에 노출한다.
        for (;;)
        {
            ReadOnlySpan<byte> source = Text(input, (uint)cursor); Match? match = Search(source, state); bool found = false; uint index = 0;
            if (match is { } selected)
            {
                // signed count를 현재 인접 표에서 매 비교마다 읽고 첫 중복 이름을 고른다.
                while (unchecked((int)Read(table, RawDotCommandRegister.CountOffset)) > unchecked((int)index))
                {
                    if (Equal(Text(table, unchecked(index * 32)), source.Slice(selected.NameStart, selected.NameLength))) { found = true; break; }
                    index = unchecked(index + 1);
                }
            }
            if (!found)
            {
                uint result = Invoke(table, state.FallbackIndex, input.Slice(cursor, source.Length + 1), hooks);
                Error(table, state.FallbackIndex, result, hooks); return 1;
            }
            Match captured = match!.Value; source.Slice(captured.ArgumentStart, captured.ArgumentLength).CopyTo(argument); argument[captured.ArgumentLength] = 0;
            uint code = Invoke(table, index, argument[..(captured.ArgumentLength + 1)], hooks); Error(table, index, code, hooks);
            long next = (long)cursor + state.SeparatorOffset;
            if (next < 0 || next >= input.Length) throw new ArgumentOutOfRangeException(nameof(input), "점 명령 다음 구분자의 실제 입력 물리 범위");
            cursor = (int)next; if (input[cursor] == ';') cursor++;
            if (Text(input, (uint)cursor).IsEmpty) return 1;
        }
    }
}
