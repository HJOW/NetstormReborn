namespace Netstorm.Core.Original;

/// <summary>
/// 같은 Client 현재 변환표와 checking 조건만 빌리며 별도 parsed 타입 표를 만들지 않는다(cpppj <c>o/RawFortContentsLoad.h</c>의 <c>FortContentsLoadGlobals</c>).
/// </summary>
/// <remarks>2026-11-02 F2b에서 추가했다.</remarks>
public sealed class FortContentsLoadGlobals
{
    /// <summary>현재 255BYTE 할당 토큰 읽기다(원본 005c89bc). 변환 진단 복귀 뒤 다시 읽는다.</summary>
    public Func<uint>? GetConversion { get; set; }

    /// <summary>현재 255BYTE 할당 토큰 쓰기다.</summary>
    public Action<uint>? SetConversion { get; set; }

    /// <summary>005e4794의 현재 DWORD 순수 조회다. relaxed 0·현재 표 비영일 때만 필요하다.</summary>
    public Func<uint>? Checking { get; set; }
}

/// <summary>
/// 각 실제 TYPE getter 뒤 읽는 필드다(cpppj <c>o/RawFortContentsLoad.h</c>의 <c>FortContentsLoadType</c>).
/// 첫 조회 flags1과 두 번째 조회 contentListFlags를 같은 사본으로 합치지 않는다.
/// </summary>
/// <remarks>2026-11-02 F2b에서 추가했다.</remarks>
public readonly record struct FortContentsLoadType(uint Flags1, uint ContentListFlags);

/// <summary>
/// 원본이 진단 후 계속하는 두 위치다(cpppj <c>o/RawFortContentsLoad.h</c>의 <c>FortContentsLoadAssertion</c>).
/// 반환 0과 물리 거부·명시 중단은 구별한다.
/// </summary>
/// <remarks>2026-11-02 F2b에서 추가했다.</remarks>
public enum FortContentsLoadAssertion : uint
{
    /// <summary>checking·relaxed 0이며 현재 변환 BYTE가 0인 Template.cpp 진단이다.</summary>
    MissingTypeConversion = 0x79,

    /// <summary>QB WORD를 먼저 읽고 pos+2 뒤 signed pos가 end를 넘은 memstream 진단이다.</summary>
    WordPastEnd = 0x2e,
}

/// <summary>
/// 큰 하위 몸체와 실제 물리 저장소만 분리한다. 직접 읽기·현재 타입·flags 순회는 본문이 소유한다(cpppj <c>o/RawFortContentsLoad.h</c>의 <c>FortContentsLoadHooks</c>).
/// </summary>
/// <remarks>2026-11-02 F2b에서 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class FortContentsLoadHooks
{
    /// <summary>같은 반환 stream의 실제 16BYTE·버퍼를 전체 재귀 동안 동기 빌린다.</summary>
    public Action<uint, Action<Span<byte>, MemStreamHooks>>? Access { get; set; }

    /// <summary>현재 비영 토큰의 실제 255BYTE를 순수 조회한다. 0·만료·물리 부족은 성공 0으로 바꾸지 않는다.</summary>
    public RawBytesLookup? ConversionBuffer { get; set; }

    /// <summary>0049a840(kind)의 현재 두 필드다. 한 항목에서도 원문 위치에서 두 번 호출한다.</summary>
    public Func<uint, FortContentsLoadType>? Type { get; set; }

    /// <summary>004ac550(진입 부모의 raw 주소)+fc의 그때 현재 그릇 flags다.</summary>
    public Func<uint, uint>? ParentListFlags { get; set; }

    /// <summary>실제 004afa70(parent,type,quantity,listFlags,attachFlags,0)이다. 생략·parsed 목록으로 대체하지 않는다.</summary>
    public Func<uint, uint, uint, uint, uint, uint, uint>? Create { get; set; }

    /// <summary>진단 복귀 뒤 현재 전역·cursor·TYPE를 원문 위치에서 재조회한다.</summary>
    public Action<FortContentsLoadAssertion>? Assertion { get; set; }
}

/// <summary>
/// 원본 004bd130의 raw count·현재 변환·TYPE·즉시 ContainedCreate와 중첩 순서를 복원한다(cpppj <c>o/RawFortContentsLoad</c>).
/// 004bd130: raw count→현재 conversion→TYPE→listFlags→즉시 Create→같은 stream 재귀다. 본문은 loadingDepth를 쓰지 않는다.
/// false는 빈 data·end·변환 0·중첩 false의 원문 반환이며 장치 누락·물리 범위는 앞선 효과를 유지한 예외다.
/// </summary>
/// <remarks>2026-11-02 F2b에서 추가했다.</remarks>
public static class RawFortContentsLoad
{
    /// <summary>첫 TYPE.flags1에서 캡처하는 세 원본 비트다.</summary>
    private const uint ContentsQa = 0x1000u;

    /// <summary>QB 비트다.</summary>
    private const uint ContentsQb = 0x2000u;

    /// <summary>container 비트다.</summary>
    private const uint ContentsContainer = 0x20000u;

    /// <summary>
    /// 도달한 누락 장치만 거부하며 빈 목록의 미선택 TYPE·Create는 요구하지 않는다.
    /// </summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식).</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <exception cref="InvalidOperationException">Fort 원시 Contents의 실제 경계가 연결되지 않았다.</exception>
    private static T NeedContents<T>(T? function) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("Fort 원시 Contents의 실제 경계 미연결");
        }
        return function;
    }

    /// <summary>
    /// 실제 현재 16BYTE의 DWORD를 비정렬 little endian으로 읽는다. 필요한 DWORD만 조합한다.
    /// </summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <exception cref="ArgumentOutOfRangeException">Fort Contents 현재 stream DWORD 물리 범위다.</exception>
    private static uint ContentsU(ReadOnlySpan<byte> bytes, int offset)
    {
        if (offset > bytes.Length || bytes.Length - offset < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), offset, "Fort Contents 현재 stream DWORD 물리 범위");
        }
        uint value = 0;
        // 현재 네 BYTE 조립값이다
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)bytes[offset + index] << (index * 8);
        }
        return value;
    }

    /// <summary>
    /// 실제 pos DWORD만 쓰고 data·end·capacity는 보존한다.
    /// </summary>
    /// <param name="fields">쓸 stream 필드다.</param>
    /// <param name="position">쓸 pos 값이다.</param>
    /// <exception cref="ArgumentOutOfRangeException">Fort Contents 현재 pos 물리 범위다.</exception>
    private static void ContentsPosition(Span<byte> fields, uint position)
    {
        if (fields.Length < 8)
        {
            throw new ArgumentOutOfRangeException(nameof(fields), fields.Length, "Fort Contents 현재 pos 물리 범위");
        }
        for (int index = 0; index < 4; index++)
        {
            fields[4 + index] = (byte)(position >> (index * 8)); // 상위 BYTE도 원문 DWORD 폭으로 대입한다
        }
    }

    /// <summary>
    /// 직접 BYTE는 논리 end를 검사하지 않고 실제 버퍼 물리 끝만 거부한다.
    /// </summary>
    /// <param name="bytes">읽을 버퍼다.</param>
    /// <param name="position">읽을 위치다.</param>
    /// <exception cref="ArgumentOutOfRangeException">Fort Contents 직접 BYTE 물리 범위다.</exception>
    private static byte ContentsByteAt(ReadOnlySpan<byte> bytes, uint position)
    {
        if (position >= (uint)bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(position), position, "Fort Contents 직접 BYTE 물리 범위");
        }
        return bytes[(int)position];
    }

    /// <summary>
    /// 현재 data·pos에서 하나 읽은 뒤 같은 현재 stream의 pos를 진행한다.
    /// </summary>
    /// <param name="fields">현재 stream 필드다.</param>
    /// <param name="memory">현재 data getter다.</param>
    private static byte ContentsByte(Span<byte> fields, MemStreamHooks memory)
    {
        uint position = ContentsU(fields, 4);
        uint data = ContentsU(fields, 0);
        ReadOnlySpan<byte> bytes = NeedContents(memory.Buffer)(data); // 한 BYTE의 실제 주소를 먼저 캡처한다
        byte value = ContentsByteAt(bytes, position);
        ContentsPosition(fields, position + 1u);
        return value;
    }

    /// <summary>
    /// QB만 현재 end를 캡처해 pos 증가 뒤 signed 진단한다. 진단이 data·pos를 바꾸어도 읽은 WORD 값은 유지한다.
    /// </summary>
    /// <param name="fields">현재 stream 필드다.</param>
    /// <param name="memory">현재 data getter다.</param>
    /// <param name="hooks">진단 경계다.</param>
    /// <exception cref="ArgumentOutOfRangeException">Fort Contents QB WORD 물리 범위다.</exception>
    private static uint ContentsWord(Span<byte> fields, MemStreamHooks memory, FortContentsLoadHooks hooks)
    {
        uint position = ContentsU(fields, 4);
        uint data = ContentsU(fields, 0);
        ReadOnlySpan<byte> bytes = NeedContents(memory.Buffer)(data); // 원문 EAX·ECX의 처음 주소다
        if (position > (uint)bytes.Length || (uint)bytes.Length - position < 2)
        {
            throw new ArgumentOutOfRangeException(nameof(position), position, "Fort Contents QB WORD 물리 범위");
        }
        ushort word = (ushort)(bytes[(int)position] | (bytes[(int)position + 1] << 8)); // 진단 앞에서 캡처한 원시 16비트다
        uint end = ContentsU(fields, 8);
        uint next = position + 2u;
        ContentsPosition(fields, next); // 원문은 end를 읽고 pos를 쓴 뒤 비교한다
        if (unchecked((int)next) > unchecked((int)end))
        {
            NeedContents(hooks.Assertion)(FortContentsLoadAssertion.WordPastEnd);
        }
        return unchecked((uint)(int)(short)word);
    }

    /// <summary>
    /// 실제 재귀의 부모·수정된 attach flags·남은 count를 독립 프레임으로 유지한다.
    /// 자식 호출이 부모의 flags·count를 덮지 않도록 각 호출값을 보관한다.
    /// </summary>
    /// <param name="Parent">진입 부모 SID 값이다.</param>
    /// <param name="Flags">수정된 attach flags다.</param>
    /// <param name="Remaining">남은 항목 수다.</param>
    private readonly record struct ContentsLoadFrame(uint Parent, uint Flags, uint Remaining);

    /// <summary>
    /// 각 재귀 진입도 현재 data·end 게이트를 다시 읽는다.
    /// </summary>
    /// <param name="frames">원문 자식 스택과 같은 독립 현재 상태다.</param>
    /// <param name="fields">현재 stream 필드다.</param>
    /// <param name="memory">현재 data getter다.</param>
    /// <param name="selected">진입할 자식 SID 값이다.</param>
    /// <param name="attachFlags">진입 attach flags다.</param>
    private static bool EnterFrame(List<ContentsLoadFrame> frames, Span<byte> fields, MemStreamHooks memory, uint selected, uint attachFlags)
    {
        if (ContentsU(fields, 0) == 0 || ContentsU(fields, 8) == 0)
        {
            return false;
        }
        byte count = ContentsByte(fields, memory);
        frames.Add(new ContentsLoadFrame(selected, attachFlags, count));
        return true;
    }

    /// <summary>
    /// 동기 access 한 번 안에서 모든 원문 재귀를 실행하며 Guard 밖에 fields·buffer를 저장하지 않는다.
    /// </summary>
    /// <param name="parent">진입 부모 SID 값이다.</param>
    /// <param name="stream">같은 반환 stream 토큰이다.</param>
    /// <param name="flags">진입 attach flags다.</param>
    /// <param name="relaxedConversion">완화 변환 여부다. 0이 아니면 변환 진단을 건너뛴다.</param>
    /// <param name="globals">같은 Client 현재 변환표·checking이다. 호출자가 소유한다.</param>
    /// <param name="hooks">큰 하위 몸체·실제 물리 저장소다.</param>
    public static bool LoadFortContents(uint parent, uint stream, uint flags, uint relaxedConversion, FortContentsLoadGlobals globals, FortContentsLoadHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        bool result = false; // 선택한 전체 원문 호출의 반환값이다
        NeedContents(hooks.Access)(stream, (fields, memory) =>
        {
            List<ContentsLoadFrame> frames = []; // 원문 자식 스택과 같은 독립 현재 상태이며 별도 parsed 목록은 만들지 않는다
            if (!EnterFrame(frames, fields, memory, parent, flags))
            {
                return;
            }
            while (frames.Count != 0)
            {
                // 한 항목을 읽고 즉시 Create한 뒤 필요하면 그 자리에서 자식 프레임을 진행한다
                ContentsLoadFrame frame = frames[^1];
                if (frame.Remaining == 0)
                {
                    frames.RemoveAt(frames.Count - 1);
                    continue;
                }
                frames[^1] = frame with { Remaining = frame.Remaining - 1 };
                frame = frames[^1];
                uint kind = ContentsByte(fields, memory); // 현재 파일 BYTE를 미리 parsed 타입으로 치환하지 않는다
                if (kind < 0xf6u)
                {
                    uint conversion = NeedContents(globals.GetConversion)(); // 원문 EAX의 첫 현재 pointer 값이며 진단을 거친 경우에만 다시 읽는다
                    if (conversion != 0)
                    {
                        if (relaxedConversion == 0 && NeedContents(globals.Checking)() != 0 && ContentsByteAt(NeedContents(hooks.ConversionBuffer)(conversion), kind) == 0)
                        {
                            NeedContents(hooks.Assertion)(FortContentsLoadAssertion.MissingTypeConversion);
                            conversion = NeedContents(globals.GetConversion)();
                        }
                        kind = ContentsByteAt(NeedContents(hooks.ConversionBuffer)(conversion), kind);
                    }
                }
                if (kind == 0)
                {
                    return;
                }
                uint savedFlags = NeedContents(hooks.Type)(kind).Flags1; // 첫 TYPE 플래그는 뒤 하위가 TYPE를 바꾸어도 QA·QB·container 판정에 유지한다
                uint quantity = 0;
                if ((savedFlags & ContentsQa) != 0)
                {
                    quantity = ContentsByte(fields, memory);
                }
                if ((savedFlags & ContentsQb) != 0)
                {
                    quantity = ContentsWord(fields, memory, hooks);
                }
                uint listFlags = NeedContents(hooks.Type)(kind).ContentListFlags; // 두 번째 TYPE 조회는 첫 조회와 구별한다
                if (listFlags == 0)
                {
                    listFlags = NeedContents(hooks.ParentListFlags)(frame.Parent);
                    if (listFlags == 0)
                    {
                        listFlags = ContentsByte(fields, memory);
                    }
                }
                listFlags &= ~0x100u;
                if ((listFlags & 0x10u) != 0)
                {
                    frames[^1] = frame with { Flags = (frame.Flags & ~8u) | 0x10u };
                    frame = frames[^1];
                }
                uint child = NeedContents(hooks.Create)(frame.Parent, kind, quantity, listFlags, frame.Flags, 0); // 원문이 실제로 Create한 뒤에만 저장된 container 비트를 본다
                if ((savedFlags & ContentsContainer) != 0 && !EnterFrame(frames, fields, memory, child, frame.Flags))
                {
                    return;
                }
            }
            result = true;
        });
        return result;
    }
}
