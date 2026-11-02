using Netstorm.Core.Simulation;

namespace Netstorm.Core.Original;

/// <summary>
/// 본체가 선택하는 원본 용량·목록 플래그 진단이다. 반환 뒤 이미 쓴 자료를 되돌리지 않는다(cpppj <c>o/RawFortContentSave.h</c>의 <c>FortContentSaveAssertion</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-5에서 추가했다.</remarks>
public enum FortContentSaveAssertion
{
    /// <summary>일반 쓰기 용량 진단이다(원본 36행).</summary>
    WriteCapacity,

    /// <summary>목록 플래그 진단이다(원본 278행).</summary>
    ListFlags,
}

/// <summary>
/// 각 재귀 호출의 독립 finder 현재·다음 수명이다. current는 원문이 다시 읽는 DWORD다(cpppj <c>o/RawFortContentSave.h</c>의 <c>FortContentSaveCursor</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-5에서 추가했다.</remarks>
public sealed class FortContentSaveCursor
{
    /// <summary>Begin·Next가 반환한 같은 현재 후보다. 다른 콜백 뒤에도 다시 조회한다.</summary>
    public Func<uint>? Current { get; set; }

    /// <summary>004b1b20: 앞서 저장한 next부터 진행하고 같은 현재 후보를 갱신한다.</summary>
    public Action? Next { get; set; }
}

/// <summary>
/// 각 GetType 호출이 그 시점에 조회한 원본 세 DWORD다(cpppj <c>o/RawFortContentSave.h</c>의 <c>FortContentSaveType</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-5에서 추가했다.</remarks>
public struct FortContentSaveType
{
    /// <summary>원본 +e8이다. 해당 항목 끝까지 보존한다.</summary>
    public uint Flags1;

    /// <summary>원본 +100이다.</summary>
    public uint ContentListFlags;

    /// <summary>원본 +fc이다.</summary>
    public uint ContainerListFlags;

    /// <summary>
    /// 세 DWORD로 형식을 만든다.
    /// </summary>
    /// <param name="flags1">원본 +e8이다.</param>
    /// <param name="contentListFlags">원본 +100이다.</param>
    /// <param name="containerListFlags">원본 +fc이다.</param>
    public FortContentSaveType(uint flags1, uint contentListFlags, uint containerListFlags)
    {
        Flags1 = flags1;
        ContentListFlags = contentListFlags;
        ContainerListFlags = containerListFlags;
    }
}

/// <summary>
/// 실제 월드 검색·가상 판정과 메모리 빌림만 공급한다. 직렬화 규칙은 본체에서 실행한다(cpppj <c>o/RawFortContentSave.h</c>의 <c>FortContentSaveHooks</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-5에서 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class FortContentSaveHooks
{
    /// <summary>004b2270: 호출마다 독립 finder를 만들고 처음 후보까지 진행한다.</summary>
    public Func<uint, FortContentSaveCursor>? Begin { get; set; }

    /// <summary>005c847c의 현재 물리 SID 개수다. 후보 0에서는 조회하지 않는다.</summary>
    public Func<uint>? PoolLimit { get; set; }

    /// <summary>후보에 대한 실제 가상+60이며 반환 뒤 현재 후보를 다시 읽는다.</summary>
    public Func<uint, bool>? Robust { get; set; }

    /// <summary>현재 후보의 +c QA·+12 kind·+14 flags를 같은 raw 슬롯에서 빌린다.</summary>
    public RawReadOnlyBytesLookup? Form { get; set; }

    /// <summary>0049a840: 캡처한 kind WORD로 현재 타입을 조회한다.</summary>
    public Func<uint, FortContentSaveType>? Type { get; set; }

    /// <summary>004ac550: 고정 부모 슬롯의 현재 타입을 조회한다.</summary>
    public Func<uint, FortContentSaveType>? ParentType { get; set; }

    /// <summary>원본 36·278행 진단이며 선택된 경계만 요구한다.</summary>
    public Action<FortContentSaveAssertion>? Assertion { get; set; }

    /// <summary>보관한 stream 필드·실제 버퍼의 동기적 빌림이다. 원본 외부 효과는 아니다.</summary>
    public Action<uint, Action<Span<byte>, MemStreamHooks>>? Access { get; set; }
}

/// <summary>
/// 개수 BYTE→robust 통과 항목→QA BYTE/WORD→목록 플래그→중첩 순서로 같은 stream에 쓴다.
/// 호출 스택 대신 명시 프레임을 사용하며 순환 부모는 물리 손상으로 거부한다(cpppj <c>o/RawFortContentSave</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-5에서 추가했다.</remarks>
public static class RawFortContentSave
{
    /// <summary>
    /// 실제 선택한 효과가 없으면 앞선 부분 쓰기를 보존한 채 해당 지점에서 실패한다.
    /// </summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식).</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <exception cref="InvalidOperationException">Fort 내용물 저장의 실제 경계 미연결이다.</exception>
    private static T Need<T>(T? function) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("Fort 내용물 저장 실제 경계 미연결");
        }
        return function;
    }

    /// <summary>
    /// 비정렬 BYTE·WORD·DWORD를 읽는다. 원본 물리 슬롯 밖은 성공으로 처리하지 않는다.
    /// </summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <param name="width">읽을 너비(BYTE 수)다.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        if (offset > bytes.Length || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), offset, "Fort 내용물 저장의 원문 필드 범위");
        }
        uint value = 0;
        // 지정 폭의 낮은 바이트부터 조합한다
        for (int index = 0; index < width; index++)
        {
            value |= (uint)bytes[offset + index] << (index * 8);
        }
        return value;
    }

    /// <summary>
    /// 같은 stream의 지정 DWORD만 쓴다.
    /// </summary>
    /// <param name="bytes">쓸 16바이트 필드다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        if (offset > bytes.Length || bytes.Length - offset < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), offset, "Fort 내용물 저장의 stream 필드 범위");
        }
        // 인접 필드는 보존한다
        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>
    /// 원본 header가 예약한 당시의 data+offset을 고정한다. 이후 현재 data 변경과 별개다.
    /// </summary>
    private struct CountPosition
    {
        /// <summary>실제 포인터를 자르지 않는 버퍼 토큰이다.</summary>
        public uint Data;

        /// <summary>그 안의 개수 BYTE 위치다.</summary>
        public uint Offset;
    }

    /// <summary>
    /// 원문 header는 용량 진단 없이 pos·end를 먼저 진행한 다음 보관한 주소에 0을 쓴다.
    /// </summary>
    /// <param name="stream">쓸 stream 토큰이다.</param>
    /// <param name="hooks">실제 저장소·쓰기 경계다.</param>
    private static CountPosition ReserveCount(uint stream, FortContentSaveHooks hooks)
    {
        CountPosition count = default;
        Need(hooks.Access)(stream, (fields, memory) =>
        {
            count.Data = Read(fields, 0);
            count.Offset = Read(fields, 4);
            uint next = count.Offset + 1;
            Put(fields, 4, next);
            if (unchecked((int)Read(fields, 8)) < unchecked((int)next))
            {
                Put(fields, 8, next);
            }
            Span<byte> countBuffer = Need(memory.Buffer)(count.Data);
            if (count.Offset >= (uint)countBuffer.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(stream), count.Offset, "Fort 내용물 저장의 개수 BYTE 물리 범위");
            }
            countBuffer[(int)count.Offset] = 0;
        });
        return count;
    }

    /// <summary>
    /// robust 통과 직후 원래 개수 위치를 BYTE로 증가시킨다. 256번째 항목은 0으로 감긴다.
    /// </summary>
    /// <param name="count">고정한 개수 BYTE 주소다.</param>
    /// <param name="stream">쓸 stream 토큰이다.</param>
    /// <param name="hooks">실제 저장소·쓰기 경계다.</param>
    private static void Increment(CountPosition count, uint stream, FortContentSaveHooks hooks)
    {
        Need(hooks.Access)(stream, (fields, memory) =>
        {
            Span<byte> incrementBuffer = Need(memory.Buffer)(count.Data);
            if (count.Offset >= (uint)incrementBuffer.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(stream), count.Offset, "Fort 내용물 저장의 해제된 개수 위치");
            }
            incrementBuffer[(int)count.Offset]++;
        });
    }

    /// <summary>
    /// 현재 data·pos는 용량 진단 뒤 다시 읽고 이미 캡처한 값만 원본 BYTE·WORD 폭으로 쓴다.
    /// </summary>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓸 너비(BYTE 수)다.</param>
    /// <param name="stream">쓸 stream 토큰이다.</param>
    /// <param name="hooks">실제 저장소·쓰기 경계다.</param>
    private static void Append(uint value, uint width, uint stream, FortContentSaveHooks hooks)
    {
        Need(hooks.Access)(stream, (fields, memory) =>
        {
            if (unchecked((int)Read(fields, 12)) < unchecked((int)(Read(fields, 4) + width)))
            {
                Need(hooks.Assertion)(FortContentSaveAssertion.WriteCapacity);
            }
            Span<byte> appendBuffer = Need(memory.Buffer)(Read(fields, 0));
            uint position = Read(fields, 4);
            // 원본 little endian으로 쓴다
            if (position > (uint)appendBuffer.Length || width > (uint)appendBuffer.Length - position)
            {
                throw new ArgumentOutOfRangeException(nameof(stream), position, "Fort 내용물 저장의 실제 출력 범위");
            }
            for (uint index = 0; index < width; index++)
            {
                appendBuffer[(int)position + (int)index] = (byte)(value >> ((int)index * 8));
            }
            uint next = Read(fields, 4) + width;
            Put(fields, 4, next);
            if (unchecked((int)Read(fields, 8)) < unchecked((int)next))
            {
                Put(fields, 8, next);
            }
        });
    }

    /// <summary>원본의 재귀별 finder·고정 부모·개수 위치를 동적 프레임으로 보관한다.</summary>
    private sealed class SaveFrame
    {
        /// <summary>현재 타입 fallback에 사용하는 진입 당시 부모다.</summary>
        public uint Parent;

        /// <summary>이 목록의 진입 당시 개수 BYTE 주소다.</summary>
        public CountPosition Count;

        /// <summary>자식이 끝날 때까지 살아 있는 독립 현재·다음 검색기다.</summary>
        public FortContentSaveCursor Cursor = new();
    }

    /// <summary>
    /// 명시 프레임은 원본의 깊이 우선 순서와 자식 복귀 뒤 부모 Next 호출을 보존한다.
    /// </summary>
    /// <param name="parent">진입 부모 SID다.</param>
    /// <param name="stream">쓸 stream 토큰이다.</param>
    /// <param name="hooks">실제 검색·판정·쓰기 경계다.</param>
    public static void SaveFortContents(uint parent, uint stream, FortContentSaveHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        List<SaveFrame> frames = [];
        HashSet<uint> active = [];
        // 순환 목록을 원본의 스택 소진 대신 물리 손상으로 보고한다
        Action<uint> enter = selected =>
        {
            if (!active.Add(selected))
            {
                throw new InvalidOperationException("Fort 내용물 저장의 순환 부모");
            }
            CountPosition count = ReserveCount(stream, hooks);
            FortContentSaveCursor cursor = Need(hooks.Begin)(selected);
            frames.Add(new SaveFrame { Parent = selected, Count = count, Cursor = cursor });
        };
        enter(parent);
        // 현재 자식 목록이 끝나면 부모의 보존된 next부터 계속한다
        while (frames.Count != 0)
        {
            SaveFrame frame = frames[^1];
            uint candidate = Need(frame.Cursor.Current)();
            if (candidate == 0 || candidate >= Need(hooks.PoolLimit)())
            {
                active.Remove(frame.Parent);
                frames.RemoveAt(frames.Count - 1);
                if (frames.Count != 0)
                {
                    Need(frames[^1].Cursor.Next)();
                }
                continue;
            }
            if (Need(hooks.Robust)(candidate))
            {
                Increment(frame.Count, stream, hooks);
                uint kind = Read(Need(hooks.Form)(Need(frame.Cursor.Current)()), 0x12, 2);
                uint flags = Need(hooks.Type)(kind).Flags1;
                Append(kind, 1, stream, hooks);
                if ((flags & 0x1000U) != 0)
                {
                    Append(Read(Need(hooks.Form)(Need(frame.Cursor.Current)()), 0xc, 1), 1, stream, hooks);
                }
                if ((flags & 0x2000U) != 0)
                {
                    Append(Read(Need(hooks.Form)(Need(frame.Cursor.Current)()), 0xc, 2), 2, stream, hooks);
                }
                uint required = Need(hooks.Type)(kind).ContentListFlags;
                // 두 번째 조회도 첫 kind WORD를 사용하고 현재 metadata를 다시 읽는다
                if (required == 0)
                {
                    required = Need(hooks.ParentType)(frame.Parent).ContainerListFlags;
                }
                if (required == 0)
                {
                    Append(Read(Need(hooks.Form)(Need(frame.Cursor.Current)()), 0x14, 1), 1, stream, hooks);
                }
                else if ((Read(Need(hooks.Form)(Need(frame.Cursor.Current)()), 0x14, 2) & required) != required)
                {
                    Need(hooks.Assertion)(FortContentSaveAssertion.ListFlags);
                }
                if ((flags & 0x20000U) != 0)
                {
                    uint nested = Need(frame.Cursor.Current)();
                    enter(nested);
                    continue;
                }
            }
            Need(frame.Cursor.Next)();
        }
    }
}
