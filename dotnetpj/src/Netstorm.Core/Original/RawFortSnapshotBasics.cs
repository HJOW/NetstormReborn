using Netstorm.Core.Simulation;

namespace Netstorm.Core.Original;

/// <summary>
/// 같은 부모의 원본 assert 위치를 구별하며 진단 복귀 뒤 원문 처리를 계속한다(cpppj <c>o/RawFortSnapshotBasics.h</c>의 <c>FortSnapshotAssertion</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-4에서 추가했다.</remarks>
public enum FortSnapshotAssertion
{
    /// <summary>이름 Template의 manager가 0이다.</summary>
    MissingNamesTemplate,

    /// <summary>고정 this와 현재 선택 주소가 다르다.</summary>
    NamesNotMine,

    /// <summary>덱 Template의 manager가 0이다.</summary>
    MissingDeckTemplate,

    /// <summary>일반 쓰기 용량 진단이다.</summary>
    WriteCapacity,
}

/// <summary>
/// 같은 명시 stream에 최초 6바이트·물리 버퍼를 쓰기용으로 빌린다(cpppj <c>o/RawFortSnapshotBasics.h</c>의 <c>FortSnapshotBasicsHooks</c>).
/// 실제 쓰기 장치는 span 필드를 빌려주며 호출 뒤 버퍼를 바꾸지 않는다.
/// </summary>
/// <param name="stream">쓸 stream 토큰이다.</param>
/// <param name="bytes">쓸 바이트 구간이다.</param>
/// <remarks>2026-11-03 F2c-4에서 추가했다. <c>Span</c>을 담는 대리자는 전용 대리자로 둔다.</remarks>
public delegate void FortSnapshotWriter(uint stream, ReadOnlySpan<byte> bytes);

/// <summary>
/// 파일·현재 필드 효과만 공급하고 선택·루프·필드 쓰기는 부모 본체가 실행한다(cpppj <c>o/RawFortSnapshotBasics.h</c>의 <c>FortSnapshotBasicsHooks</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-4에서 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class FortSnapshotBasicsHooks
{
    /// <summary>같은 this Template의 현재 manager DWORD를 읽는다.</summary>
    public Func<int, uint>? Manager { get; set; }

    /// <summary>선택된 원본 진단이다. 무시는 진단 해당 callback을 공급한다.</summary>
    public Action<FortSnapshotAssertion>? Assertion { get; set; }

    /// <summary>0044d1b0: 현재 manager로 섹션을 만들고 명시 stream 토큰을 반환한다.</summary>
    public Func<uint, string, uint>? Create { get; set; }

    /// <summary>0044cce0: 최초 count 바이트의 실제 일반 쓰기다.</summary>
    public FortSnapshotWriter? Write { get; set; }

    /// <summary>명시 stream의 16바이트·물리 버퍼를 쓰기용으로 빌린다. 실제 원본 효과는 없다.</summary>
    public Action<uint, Action<Span<byte>, MemStreamHooks>>? Access { get; set; }

    /// <summary>004bd550: 현재 this의 State를 조회한다. manager.current가 바뀐다.</summary>
    public RawReadOnlyBytesLookup? State { get; set; }

    /// <summary>0049a840(index)+4의 현재 NUL 문자열이다.</summary>
    public Func<int, string>? TypeName { get; set; }

    /// <summary>0044f7c0: 같은 실제 생산 료료를 받을 객체로 갱신한다.</summary>
    public Action<uint>? RefreshDeck { get; set; }

    /// <summary>0044f700: 객체별 현재 Player 토큰이다.</summary>
    public Func<uint, uint>? GetDeck { get; set; }

    /// <summary>같은 덱의 0x160바이트를 조회만 빌린다.</summary>
    public RawReadOnlyBytesLookup? Deck { get; set; }
}

/// <summary>
/// 원본 10.78의 TypeNames·Deck snapshot writer와 현재 필드 명시 쓰기 stream을 연결한다(cpppj <c>o/RawFortSnapshotBasics</c>).
/// 같은 부모의 별도 stream을 보존하며 원본 일반 MemStream 쓰기 규칙대로 복원한다.
/// </summary>
/// <remarks>2026-11-03 F2c-4에서 추가했다.</remarks>
public static class RawFortSnapshotBasics
{
    /// <summary>
    /// 전달할 장치가 빠지면 먼저 원본 효과를 막고 선택 실패로 처리한다.
    /// </summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식).</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <exception cref="InvalidOperationException">Fort snapshot 실제 경계 미연결이다.</exception>
    private static T Need<T>(T? function) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("Fort snapshot 실제 경계 미연결");
        }
        return function;
    }

    /// <summary>
    /// 비정렬 원본 DWORD를 현재 물리 범위로 읽는다.
    /// </summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset)
    {
        if (offset > bytes.Length || bytes.Length - offset < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), offset, "Fort snapshot DWORD 물리 범위");
        }
        uint value = 0;
        // 원본 little endian 네 바이트만 조합한다
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)bytes[offset + index] << (index * 8);
        }
        return value;
    }

    /// <summary>
    /// 같은 stream의 지정 DWORD만 바꾸고 다른 필드를 보존한다.
    /// </summary>
    /// <param name="bytes">쓸 16바이트 필드다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        if (offset > bytes.Length || bytes.Length - offset < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), offset, "Fort snapshot DWORD 쓰기 범위");
        }
        // 이웃 바이트만 쓰고 원본 값을 더한다
        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>
    /// 원본 일반 바이트·DWORD 쓰기로 진단 뒤 현재 data·pos를 읽고 pos·end를 진행한다.
    /// </summary>
    /// <param name="stream">쓸 stream 토큰이다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓸 너비(BYTE 수)다.</param>
    /// <param name="hooks">실제 쓰기 경계다.</param>
    private static void Append(uint stream, uint value, uint width, FortSnapshotBasicsHooks hooks)
    {
        Need(hooks.Access)(stream, (fields, memory) =>
        {
            if (unchecked((int)Read(fields, 12)) < unchecked((int)(Read(fields, 4) + width)))
            {
                Need(hooks.Assertion)(FortSnapshotAssertion.WriteCapacity);
            }
            Span<byte> buffer = Need(memory.Buffer)(Read(fields, 0));
            uint position = Read(fields, 4);
            // 용량 진단이 바꾼 현재 필드를 실제 쓰기 주소로 결정한다
            if (position > (uint)buffer.Length || width > (uint)buffer.Length - position)
            {
                throw new ArgumentOutOfRangeException(nameof(stream), position, "Fort snapshot 실제 일반 버퍼 범위");
            }
            for (uint index = 0; index < width; index++)
            {
                buffer[(int)(position + index)] = (byte)(value >> ((int)index * 8));
            }
            uint next = position + width;
            Put(fields, 4, next);
            if (unchecked((int)Read(fields, 8)) < unchecked((int)next))
            {
                Put(fields, 8, next);
            }
        });
    }

    /// <summary>
    /// 같은 캡처 안의 현재 signed count를 반복 조건마다 조회한다.
    /// </summary>
    /// <param name="token">볼 Deck 토큰이다.</param>
    /// <param name="hooks">실제 쓰기 경계다.</param>
    private static int DeckCount(uint token, FortSnapshotBasicsHooks hooks)
    {
        return unchecked((int)Read(Need(hooks.Deck)(token), 0x14c));
    }

    /// <summary>
    /// 이름 조회를 작동 stream으로 하고 State 조회가 current를 바꿔도 교체하지 않는다.
    /// </summary>
    /// <param name="slot">고정 this의 Template 칸 번호다.</param>
    /// <param name="templateOwner">같은 Template 선택 DWORD다. 호출자가 소유한다. 원본 <c>const uint32_t&amp;</c>라서 <c>in</c>으로 받아 callback 중 바뀐 값을 그대로 본다.</param>
    /// <param name="typeCount">타입 수다. 원본 <c>const int32_t&amp;</c>라서 <c>in</c>으로 받아 Create 뒤 바뀐 현재값을 count 바이트와 반복 조건에 반영한다.</param>
    /// <param name="hooks">실제 저장소·쓰기 경계다.</param>
    public static void SnapshotFortTypeNames(int slot, in uint templateOwner, in int typeCount, FortSnapshotBasicsHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (Need(hooks.Manager)(slot) == 0)
        {
            Need(hooks.Assertion)(FortSnapshotAssertion.MissingNamesTemplate);
        }
        if (unchecked((uint)slot) != templateOwner)
        {
            Need(hooks.Assertion)(FortSnapshotAssertion.NamesNotMine);
        }
        uint stream = Need(hooks.Create)(Need(hooks.Manager)(slot), "TypeNames");
        ReadOnlySpan<byte> state = Need(hooks.State)(unchecked((uint)slot));
        if (state.Length < 18)
        {
            throw new ArgumentOutOfRangeException(nameof(slot), slot, "Fort snapshot 현재 State+17 누락");
        }
        if (state[17] == 0)
        {
            return;
        }
        byte[] count = [(byte)typeCount];
        Need(hooks.Write)(stream, count);
        // 이름 조회·진단 뒤 일반 쓰기를 실제 이름 해시로 다음 반복에 반영한다
        for (int index = 0; index < typeCount; index++)
        {
            string name = Need(hooks.TypeName)(index);
            int nul = name.IndexOf('\0');
            string key = nul < 0 ? name : name.Substring(0, nul);
            Append(stream, RiftTypeTable.NameHash(key), 4, hooks);
        }
    }

    /// <summary>
    /// 슬롯을 owner 0..8로 변환해 갱신·조회하고 매 바이트 뒤에는 다음 바이트 쓰기를 다음 조회와 진단한다.
    /// </summary>
    /// <param name="slot">고정 this의 Template 칸 번호다.</param>
    /// <param name="hooks">실제 저장소·쓰기 경계다.</param>
    public static void SnapshotFortDeck(int slot, FortSnapshotBasicsHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (Need(hooks.Manager)(slot) == 0)
        {
            Need(hooks.Assertion)(FortSnapshotAssertion.MissingDeckTemplate);
        }
        uint owner = slot >= 1 && slot <= 8 ? (uint)slot : 0u;
        Need(hooks.RefreshDeck)(owner);
        uint token = Need(hooks.GetDeck)(owner);
        uint stream = Need(hooks.Create)(Need(hooks.Manager)(slot), "Deck");
        byte[] count = [(byte)DeckCount(token, hooks)];
        Need(hooks.Write)(stream, count);
        // 원본은 매 바이트 뒤에 다음 바이트 쓰기와 용량 진단을 수행한다
        for (int index = 0; index < DeckCount(token, hooks); index++)
        {
            // 원문은 매 바이트마다 다음 바이트 쓰기와 용량 진단을 수행한다
            for (int offset = 0; offset < 4; offset++)
            {
                ReadOnlySpan<byte> source = Need(hooks.Deck)(token);
                int at = index * 5 + offset;
                if (at >= source.Length)
                {
                    throw new ArgumentOutOfRangeException(nameof(slot), at, "Fort snapshot Deck 바이트 범위 누락");
                }
                Append(stream, source[at], 1, hooks);
            }
        }
    }
}
