using static Netstorm.Core.Original.RawStorageBytes;

namespace Netstorm.Core.Original;

/// <summary>
/// 10.78 뱃지 저장 부모와 같은 28개 실제 장부의 첫 일치 count getter를 복원한다(cpppj <c>o/RawFortBadgesSave.h</c>의 상수·getter).
/// </summary>
/// <remarks>2026-11-03 F2c-6에서 추가했다.</remarks>
public static class FortBadgeSaveShape
{
    /// <summary>00593160..00594970의 실제 기록 수다.</summary>
    public const int RecordCount = 28;

    /// <summary>type와 count 사이 212바이트를 포함하는 실제 기록 폭이다.</summary>
    public const int RecordStride = 220;

    /// <summary>각 기록에서 전체 count DWORD가 있는 위치다.</summary>
    public const int CountOffset = 216;
}

/// <summary>
/// 원본 Template.cpp 0x412와 인라인 MemStream 0x24를 구별한다(cpppj <c>o/RawFortBadgesSave.h</c>의 <c>FortBadgesSaveAssertion</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-6에서 추가했다.</remarks>
public enum FortBadgesSaveAssertion : uint
{
    /// <summary>새 Badges 섹션 생성 전에 같은 this의 manager 0을 진단한다.</summary>
    MissingTemplate = 0x412,

    /// <summary>초기 count 바이트를 제외한 각 type·count 바이트의 용량 진단이다.</summary>
    WriteCapacity = 0x24,
}

/// <summary>
/// 실제 manager·Create·getter와 명시 stream 16바이트·실제 버퍼를 연결한다(cpppj <c>o/RawFortBadgesSave.h</c>의 <c>FortBadgesSaveHooks</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-6에서 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class FortBadgesSaveHooks
{
    /// <summary>고정 this 슬롯의 현재 첫 DWORD를 매 원문 위치에서 읽는다.</summary>
    public Func<int, uint>? Manager { get; set; }

    /// <summary>진단 복귀 뒤 현재 data·pos·end·capacity 변경을 허용한다.</summary>
    public Action<FortBadgesSaveAssertion>? Assertion { get; set; }

    /// <summary>0044d1b0(manager,"Badges")가 반환한 같은 stream이다.</summary>
    public Func<uint, string, uint>? Create { get; set; }

    /// <summary>0047d7c0(type)의 실제 전체 count DWORD다. 새 getter에는 같은 live 장부 span을 준다.</summary>
    public Func<uint, uint>? Count { get; set; }

    /// <summary>같은 16바이트·버퍼의 동기 빌림이다. 최초 buffer 할당은 저장이 끝날 때까지 살아 있어야 한다.</summary>
    public Action<uint, Action<Span<byte>, MemStreamHooks>>? Access { get; set; }
}

/// <summary>
/// 004be7f0: 최초 count 포인터를 유지하고 type 0..254의 0이 아닌 DWORD만 두 바이트로 쓴다(cpppj <c>o/RawFortBadgesSave</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-6에서 추가했다. 최초 할당 Span을 C++처럼 보관하지 않고 토큰·조회로 다시 찾는다.</remarks>
public static class RawFortBadgesSave
{
    /// <summary>
    /// 선택되지 않은 장치는 요구하지 않으며 도달한 누락 경계는 앞선 효과 뒤에 실패한다.
    /// </summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식).</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <exception cref="InvalidOperationException">뱃지 저장의 실제 하위 경계가 연결되지 않았다.</exception>
    private static T NeedBadges<T>(T? function) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("Badges 저장의 실제 하위 경계 미연결");
        }
        return function;
    }

    /// <summary>
    /// 실제 비정렬 little endian DWORD만 읽는다.
    /// </summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int at)
    {
        if (at < 0 || at > bytes.Length || 4 > bytes.Length - at)
        {
            throw new ArgumentOutOfRangeException(nameof(at), at, "Badges DWORD 실제 물리 범위");
        }
        uint value = 0;
        // 원본 네 바이트 폭만 합친다
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)bytes[at + index] << (index * 8);
        }
        return value;
    }

    /// <summary>
    /// 해당 stream DWORD만 쓰고 다른 필드를 보존한다.
    /// </summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> bytes, int at, uint value)
    {
        if (at < 0 || at > bytes.Length || 4 > bytes.Length - at)
        {
            throw new ArgumentOutOfRangeException(nameof(at), at, "Badges DWORD 쓰기 실제 범위");
        }
        // 낮은 바이트부터 네 개만 쓴다
        for (int index = 0; index < 4; index++)
        {
            bytes[at + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>
    /// 원본 EDI가 처음 보관한 할당·위치다. 뒤의 stream data·pos 변경과 무관하게 같은 바이트를 쓴다.
    /// C++의 Span 보관과 달리 토큰·조회로 다시 찾는다. 최초 할당이 끝까지 살아 있다는 계약은 같다.
    /// </summary>
    private sealed class SavedCounter
    {
        /// <summary>최초 data 토큰으로 같은 버퍼를 다시 찾는 조회다.</summary>
        public RawBytesLookup? Lookup { get; set; }

        /// <summary>최초 data에 더한 원래 pos다. 뒤에 새 pos를 복사하지 않는다.</summary>
        public uint Data { get; set; }

        /// <summary>최초 data에 더한 원래 pos다.</summary>
        public uint Position { get; set; }
    }

    /// <summary>
    /// 실제 바이트 쓰기 직전에 물리 범위만 확인하고 원문 밖 메모리를 대신 쓰지 않는다.
    /// </summary>
    /// <param name="bytes">쓸 버퍼다.</param>
    /// <param name="position">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void WriteByte(Span<byte> bytes, uint position, byte value)
    {
        if (position >= (uint)bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(position), position, "Badges 출력 바이트 실제 물리 범위");
        }
        bytes[(int)position] = value;
    }

    /// <summary>
    /// 바이트를 쓴 뒤에 다시 읽은 현재 pos·end로 signed 비교·DWORD 감김 갱신을 수행한다.
    /// </summary>
    /// <param name="fields">현재 stream 필드다.</param>
    private static uint Advance(Span<byte> fields)
    {
        uint next = Read(fields, 4) + 1u;
        uint end = Read(fields, 8);
        Put(fields, 4, next);
        if (unchecked((int)next) > unchecked((int)end))
        {
            Put(fields, 8, next);
        }
        return next;
    }

    /// <summary>
    /// 타입·미정 padding을 변경하지 않으며 중복은 원본처럼 처음 기록에서 검색을 끝낸다.
    /// </summary>
    /// <param name="badges">28×220바이트 실제 장부다.</param>
    /// <param name="type">찾을 타입이다.</param>
    public static uint ReadFortBadgeCount(ReadOnlySpan<byte> badges, uint type)
    {
        if (badges.Length != FortBadgeSaveShape.RecordCount * FortBadgeSaveShape.RecordStride)
        {
            throw new ArgumentException("Badge getter의 실제28x220바이트 장부 누락", nameof(badges));
        }
        // type 255 이상도 DWORD로 비교하며 바이트로 절삭하지 않는다
        for (int index = 0; index < FortBadgeSaveShape.RecordCount; index++)
        {
            int at = index * FortBadgeSaveShape.RecordStride;
            if (Read(badges, at) == type)
            {
                return Read(badges, at + FortBadgeSaveShape.CountOffset);
            }
        }
        return 0;
    }

    /// <summary>
    /// 004be7f0: 최초 count 포인터를 유지하고 type 0..254의 0이 아닌 DWORD만 두 바이트로 쓴다.
    /// count getter 뒤에도 반환 DWORD는 보관하며 각 진단 뒤 주소만 현재 필드에서 다시 읽는다.
    /// </summary>
    /// <param name="slot">고정 this 슬롯이다.</param>
    /// <param name="hooks">실제 저장 경계다.</param>
    public static void SaveFortBadges(int slot, FortBadgesSaveHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (NeedBadges(hooks.Manager)(slot) == 0)
        {
            NeedBadges(hooks.Assertion)(FortBadgesSaveAssertion.MissingTemplate);
        }
        uint manager = NeedBadges(hooks.Manager)(slot);
        uint stream = NeedBadges(hooks.Create)(manager, "Badges");
        SavedCounter counter = new();
        NeedBadges(hooks.Access)(stream, (fields, memory) =>
        {
            uint position = Read(fields, 4);
            uint data = Read(fields, 0);
            uint end = Read(fields, 8);
            counter.Lookup = NeedBadges(memory.Buffer);
            counter.Data = data;
            counter.Position = position;
            uint next = position + 1u;
            Put(fields, 4, next);
            if (unchecked((int)next) > unchecked((int)end))
            {
                Put(fields, 8, next);
            }
            WriteByte(NeedBadges(memory.Buffer)(data), position, 0);
        });
        for (uint type = 0; type < 255; type++)
        {
            uint count = NeedBadges(hooks.Count)(type);
            if (count == 0)
            {
                continue;
            }
            NeedBadges(hooks.Access)(stream, (fields, memory) =>
            {
                if (unchecked((int)(Read(fields, 4) + 1u)) > unchecked((int)Read(fields, 12)))
                {
                    NeedBadges(hooks.Assertion)(FortBadgesSaveAssertion.WriteCapacity);
                }
                uint typePosition = Read(fields, 4);
                uint typeData = Read(fields, 0);
                WriteByte(NeedBadges(memory.Buffer)(typeData), typePosition, (byte)type);
                uint afterType = Advance(fields);
                if (unchecked((int)(afterType + 1u)) > unchecked((int)Read(fields, 12)))
                {
                    NeedBadges(hooks.Assertion)(FortBadgesSaveAssertion.WriteCapacity);
                }
                uint countData = Read(fields, 0);
                uint countPosition = Read(fields, 4);
                WriteByte(NeedBadges(memory.Buffer)(countData), countPosition, (byte)count);
                Advance(fields);
                Span<byte> allocation = NeedBadges(counter.Lookup)(counter.Data);
                WriteByte(allocation, counter.Position, (byte)(allocation[(int)counter.Position] + 1));
            });
        }
    }
}
