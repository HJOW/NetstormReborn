using System.Buffers.Binary;
using System.Text;
using Netstorm.Core.Original;

// 같은 manager/현재 Chat 기록과 인접 지역 캡처를 사용하는 실제 채팅 제출 경계다.
namespace Netstorm.Client.Chat;

/// <summary>NUL 포함 실제 가변 본문을 동기 명령 분배기에 넘긴다.</summary>
public delegate void ChatSubmitDispatch(Span<byte> text);
/// <summary>선택된 실제 물리/할당/식별/표시 장치만 원문 위치에서 요구한다.</summary>
public sealed class ChatSubmitHooks
{
    /// <summary>같은 토큰의 실제 가변 물리를 빌리는 순수 조회다.</summary>
    public Func<uint, Memory<byte>>? Memory { get; set; }
    /// <summary>수신자 확장의 실제 BYTE 폭을 받아0/비영 토큰을 반환한다.</summary>
    public Func<uint, uint>? Allocate { get; set; }
    /// <summary>현재 이전 버퍼 토큰을 확장 복사 뒤 실제 반납한다.</summary>
    public Action<uint>? Release { get; set; }
    /// <summary>실패 할당의 Chat.cpp274 진단이며 복귀하면 원문 접근을 계속한다.</summary>
    public Action<uint>? Assertion { get; set; }
    /// <summary>같은60BYTE 지역 식별자를 실제 현재 Player/Chat으로 조회한다.</summary>
    public Func<byte[], bool>? Resolve { get; set; }
    /// <summary>채널 키를 현재 언어의 Latin1 보존 표시 BYTE 문자열로 돌려준다.</summary>
    public Func<string, string>? Translate { get; set; }
    /// <summary>현재100×48BYTE 기록을 매 행 소비 위치에서 다시 빌린다.</summary>
    public Func<ReadOnlyMemory<byte>>? Records { get; set; }
    /// <summary>빈 본문에서 수신자 갱신 뒤 원문 입력줄 표시0을 적용한다.</summary>
    public Action<uint>? InputVisible { get; set; }
    /// <summary>비어 있지 않은 같은 본문/NUL을 실제 명령 분배에 넘긴다.</summary>
    public ChatSubmitDispatch? Dispatch { get; set; }
}
/// <summary>RawChatSubmit.cpp의0042da00/0042d1b0 본체다.</summary>
public static class RawChatSubmit
{
    /// <summary>본문2048+접두2048과 인접 마지막NUL의 실제 물리 폭이다.</summary>
    public const int WorkBytes = 4097;
    /// <summary>원문00531768의 NULL 앞 여덟 키이며 빈 키도 순서대로 조회한다.</summary>
    private static readonly string[] Channels = ["", "", "", "BATTLE", "", "EVERYONE", "ALLIES", "OBSERVER"];
    /// <summary>실제로 도달한 외부 장치만 요구한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("Chat 제출의 실제 선택 장치 미연결");
    /// <summary>한 DWORD의 전체 물리를 확인하고 little endian으로 읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, uint at)
    {
        if ((ulong)at + 4 > (ulong)bytes.Length) throw new ArgumentOutOfRangeException(nameof(at), "Chat 제출 DWORD의 실제 물리 범위");
        return BinaryPrimitives.ReadUInt32LittleEndian(bytes.Slice((int)at, 4));
    }
    /// <summary>전체 DWORD 폭을 확인한 뒤 같은 네BYTE만 쓴다.</summary>
    private static void Write(Span<byte> bytes, uint at, uint value)
    {
        if ((ulong)at + 4 > (ulong)bytes.Length) throw new ArgumentOutOfRangeException(nameof(at), "Chat 제출 store의 실제 물리 범위");
        BinaryPrimitives.WriteUInt32LittleEndian(bytes.Slice((int)at, 4), value);
    }
    /// <summary>실제 같은 토큰의 물리를 매 사용 위치에서 빌린다.</summary>
    private static Memory<byte> Memory(ChatSubmitHooks hooks, uint token) => Need(hooks.Memory)(token);
    /// <summary>명시 BYTE view의 첫NUL 뒤를 해석하지 않으며 NUL 없는 길이 view도 허용한다.</summary>
    private static ReadOnlySpan<byte> Z(ReadOnlySpan<byte> text) { int end = text.IndexOf((byte)0); return end < 0 ? text : text[..end]; }
    /// <summary>실제 지역 물리의 NUL 앞을 빌리며 두 인접 캡처 사이의 이어짐도 유지한다.</summary>
    private static ReadOnlySpan<byte> Text(ReadOnlySpan<byte> bytes)
    {
        int end = bytes.IndexOf((byte)0); if (end < 0) throw new ArgumentOutOfRangeException(nameof(bytes), "Chat 제출 문자열의 실제 NUL 없음"); return bytes[..end];
    }
    /// <summary>원문 패턴의 CR/LF/TAB/SP만 공백이며 VT/FF는 포함하지 않는다.</summary>
    private static bool Space(byte value) => value is 13 or 10 or 9 or 32;
    /// <summary>앞 네 공백만 소비하고 같은 입력 view의 나머지를 반환한다.</summary>
    private static ReadOnlySpan<byte> TrimStart(ReadOnlySpan<byte> text)
    {
        int cursor = 0;
        // 고정 패턴에 지정된 공백만 건너뛴다.
        while (cursor < text.Length && Space(text[cursor])) cursor++;
        return text[cursor..];
    }
    /// <summary>최대2048BYTE와 그 다음NUL을 같은 인접 작업 공간에 쓴다.</summary>
    private static void Capture(Span<byte> work, int at, ReadOnlySpan<byte> text)
    {
        int count = Math.Min(text.Length, 2048);
        if (at < 0 || at > work.Length || count + 1 > work.Length - at) throw new ArgumentOutOfRangeException(nameof(work), "Chat 캡처의 실제 물리 범위");
        text[..count].CopyTo(work[at..]); work[at + count] = 0;
    }
    /// <summary>C locale의 ASCII 대문자만 바꾸며 다른 BYTE를 유지한다.</summary>
    private static byte Lower(byte value) => value is >= (byte)'A' and <= (byte)'Z' ? (byte)(value + 32) : value;
    /// <summary>첫NUL 앞의 길이와 원문 C locale 바이트만 비교한다.</summary>
    private static bool Equal(ReadOnlySpan<byte> first, ReadOnlySpan<byte> second)
    {
        first = Z(first); second = Z(second); if (first.Length != second.Length) return false;
        // 원문 narrow 비교처럼 비ASCII는 변환하지 않는다.
        for (int index = 0; index < first.Length; index++) if (Lower(first[index]) != Lower(second[index])) return false;
        return true;
    }
    /// <summary>빈 키도 조회하고 각 번역 callback 뒤 같은 현재 식별자 이름을 다시 읽는다.</summary>
    private static int Channel(byte[] identity, ChatSubmitHooks hooks)
    {
        // 최초 여덟 채널의 고정 순서를 보존한다.
        for (int index = 0; index < Channels.Length; index++)
        {
            string value = Need(hooks.Translate)(Channels[index]);
            if (Equal(Text(identity.AsSpan(8)), Encoding.Latin1.GetBytes(value))) return index;
        }
        return -1;
    }
    /// <summary>중복 현재 DWORD 검색→필요 시32슬롯 확장→현재 count에 추가하는 실제 배열 부모다.</summary>
    public static void AddChatRecipient(uint manager, uint subscriber, ChatSubmitHooks hooks)
    {
        Memory<byte> storage = Memory(hooks, manager);
        // 현재 signed count와 같은 배열을 매 비교 위치에서 읽는다.
        for (uint index = 0; unchecked((int)index) < unchecked((int)Read(storage.Span, 0)); index = unchecked(index + 1))
            if (Read(Memory(hooks, Read(storage.Span, 8)).Span, unchecked(index * 4)) == subscriber) return;
        if (unchecked((int)Read(storage.Span, 4)) <= unchecked((int)Read(storage.Span, 0)))
        {
            uint buffer = Need(hooks.Allocate)(unchecked(Read(storage.Span, 4) * 4 + 128));
            if (buffer == 0) Need(hooks.Assertion)(0x112);
            // count 대신 현재 capacity 전체를 복사하며 하위 호출 뒤 현재 주소를 재조회한다.
            for (uint index = 0; unchecked((int)index) < unchecked((int)Read(storage.Span, 4)); index = unchecked(index + 1))
                Write(Memory(hooks, buffer).Span, unchecked(index * 4), Read(Memory(hooks, Read(storage.Span, 8)).Span, unchecked(index * 4)));
            Need(hooks.Release)(Read(storage.Span, 8)); Write(storage.Span, 8, buffer); Write(storage.Span, 4, unchecked(Read(storage.Span, 4) + 32));
        }
        Write(Memory(hooks, Read(storage.Span, 8)).Span, unchecked(Read(storage.Span, 0) * 4), subscriber); Write(storage.Span, 0, unchecked(Read(storage.Span, 0) + 1));
    }
    /// <summary>고정 대상/본문 패턴과 두 인접 캡처를 처리하고 현재 수신자 갱신 뒤 본문을 분배한다.</summary>
    public static void SubmitChatInput(uint manager, ReadOnlySpan<byte> input, byte[] identity, Span<byte> work, ChatSubmitHooks hooks)
    {
        Memory<byte> storage = Memory(hooks, manager); Write(storage.Span, 0, 0); Write(storage.Span, 12, 0);
        if (work.Length < WorkBytes) throw new ArgumentOutOfRangeException(nameof(work), "Chat 제출의 두 인접 캡처 물리 부족");
        work = work[..WorkBytes]; input = Z(input); ReadOnlySpan<byte> source = TrimStart(input); int colon = source.IndexOf((byte)':'); uint mask = 0; work[0] = 0;
        if (colon >= 0)
        {
            Capture(work, 0, TrimStart(source[(colon + 1)..])); Capture(work, 2048, source[..colon]); RawPlayerIdentity.Initialize(identity, 0); Write(storage.Span, 12, 1); int cursor = 0;
            // 다음 위치는40BYTE 이름 절단이 아닌 원문 접두 토큰 전체 길이로 이동한다.
            for (;;)
            {
                ReadOnlySpan<byte> prefix = Text(work[2048..]); if (cursor > prefix.Length) throw new ArgumentOutOfRangeException(nameof(work), "Chat 접두 토큰의 실제 다음 위치");
                ReadOnlySpan<byte> remaining = prefix[cursor..]; int start = 0;
                // 현재 접두의 지정 공백만 건너뛴다.
                while (start < remaining.Length && Space(remaining[start])) start++;
                if (start == remaining.Length) break; int end = start;
                // 전체 토큰의 끝을 먼저 찾고 이름 복사 상한과 구별한다.
                while (end < remaining.Length && !Space(remaining[end])) end++;
                int length = end - start; Write(identity, 0, 0); Write(identity, 4, 0); identity[8] = 0; Write(identity, 48, 0);
                int copied = Math.Min(length, 40); remaining.Slice(start, copied).CopyTo(identity.AsSpan(8)); identity[8 + copied] = 0;
                if (Need(hooks.Resolve)(identity)) AddChatRecipient(manager, Read(identity, 4), hooks);
                else
                {
                    int channel = Channel(identity, hooks); Write(storage.Span, 12, 0);
                    if (channel < 0)
                    {
                        Write(storage.Span, 0, 0); mask = 0x20; int bytes = Math.Min(input.Length, 2048); input[..bytes].CopyTo(work); work[bytes..2048].Clear(); work[2047] = 0; break;
                    }
                    mask |= 1u << (channel & 31);
                }
                cursor += start + length;
            }
        }
        else { Capture(work, 0, source); mask = 0x20; }
        Write(storage.Span, 16, mask);
        if (mask != 0)
        {
            // 고정100행을 순서대로 읽으며 각 Add 뒤 다음 행은 현재 기록을 다시 조회한다.
            for (uint index = 0; index < 100; index++)
            {
                ReadOnlyMemory<byte> records = Need(hooks.Records)(); if (records.Length < 4800) throw new ArgumentOutOfRangeException(nameof(hooks), "Chat 현재100행의 실제 물리 부족");
                ReadOnlySpan<byte> row = records.Span.Slice((int)index * 48, 48); uint subscriber = Read(row, 44), flags = unchecked((uint)(int)(sbyte)row[40]);
                if (subscriber != 0 && (mask & flags) != 0) AddChatRecipient(manager, subscriber, hooks);
            }
        }
        ReadOnlySpan<byte> body = Text(work);
        if (body.IsEmpty) Need(hooks.InputVisible)(0); else Need(hooks.Dispatch)(work[..(body.Length + 1)]);
    }
}
