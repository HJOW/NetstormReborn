// Player의 9×16바이트 Core 장부와 별도 Core 10바이트 초기화를 복원한다.
namespace Netstorm.Core.Original;

// Player Core 장부 모듈을 담는 네임스페이스다. 2026-11-07 F2d Player 이식에서 추가됐다.

// Player Core 장부 상수를 담는 정적 클래스다. 2026-11-07 F2d Player 이식에서 추가됐다.
public static class PlayerCoreRecordValues
{
    /// <summary>실제 기록 수다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public const uint RecordCount = 9u;

    /// <summary>기록 간 바이트 간격이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public const uint RecordStride = 16u;

    /// <summary>장부 전체 바이트 수다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public const uint RecordBytes = 144u;
}

// 실제 물리 안의 장부 시작을 빌리는 뷰 클래스다. 2026-11-07 F2d Player 이식에서 추가됐다.
public sealed class PlayerCoreRecordsView
{
    /// <summary>장부 바이트다. 정상 Client는 144바이트다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public byte[] Bytes { get; set; } = [];

    /// <summary>장부 첫 주소까지의 32비트 상대 위치다. 정상 Client는 0이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public uint Origin { get; set; }
}

// Player Core 장부 초기화를 복원한 정적 클래스다. 2026-11-07 F2d Player 이식에서 추가됐다.
public static class RawPlayerCoreRecords
{
    /// <summary>
    /// 같은 Core에 DWORD 0→DWORD 4→WORD 8을 0으로 쓴다. 짧으면 앞선 쓰기만 남긴다.
    /// </summary>
    /// <param name="core">쓸 Core 10바이트 뷰 다.</param>
    /// <exception cref="ArgumentOutOfRangeException">Core 장부의 실제 쓰기 물리 범위다.</exception>
    public static void InitializePlayerCore(Span<byte> core)
    {
        Store(core, 0u, 4u, 0u);
        Store(core, 4u, 4u, 0u);
        Store(core, 8u, 2u, 0u);
    }

    /// <summary>
    /// 9회 각 기록+4의 Core 초기화 뒤 기록 ID에 0을 쓴다. 끝 2바이트 패딩은 보존한다.
    /// </summary>
    /// <param name="records">장부 뷰다.</param>
    /// <exception cref="ArgumentOutOfRangeException">Core 장부의 실제 물리 범위다.</exception>
    public static void InitializePlayerCoreRecords(PlayerCoreRecordsView records)
    {
        ArgumentNullException.ThrowIfNull(records);
        uint offset = records.Origin;
        // 정상 9회만 수행하며 매 기록 끝 패딩 2바이트는 건드리지 않는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (uint record = 0u; record < PlayerCoreRecordValues.RecordCount; ++record)
        {
            InitializePlayerCore(At(records.Bytes, offset + 4u));
            Store(records.Bytes, offset, 4u, 0u);
            offset += PlayerCoreRecordValues.RecordStride;
        }
    }

    /// <summary>
    /// 소유자 주소의 ID DWORD→복제된 Core DWORD/DWORD/WORD를 쓴다.
    /// </summary>
    /// <param name="records">장부 뷰다.</param>
    /// <param name="owner">소유자 번호다.</param>
    /// <param name="subscriber">쓸 구독자 번호다.</param>
    /// <param name="core">복제된 Core 10바이트다.</param>
    /// <exception cref="ArgumentOutOfRangeException">Core 장부의 실제 쓰기 물리 범위다.</exception>
    public static void WritePlayerCoreMetadata(PlayerCoreRecordsView records, uint owner, uint subscriber, byte[] core)
    {
        ArgumentNullException.ThrowIfNull(records);
        ArgumentNullException.ThrowIfNull(core);
        uint offset = unchecked(records.Origin + owner * PlayerCoreRecordValues.RecordStride);
        Store(records.Bytes, offset, 4u, subscriber);
        Store(records.Bytes, offset + 4u, 4u, Value(core, 0u, 4u));
        Store(records.Bytes, offset + 8u, 4u, Value(core, 4u, 4u));
        Store(records.Bytes, offset + 12u, 2u, Value(core, 8u, 2u));
    }

    /// <summary>단일 원문 쓰기의 전체 물리 폭을 확인한 뒤 같은 폭만 쓴다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="width">쓰는 폭이다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <exception cref="ArgumentOutOfRangeException">Core 장부의 실제 쓰기 물리 범위다.</exception>
    private static void Store(Span<byte> bytes, uint offset, uint width, uint value)
    {
        if (offset > (uint)bytes.Length || width > (uint)bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "Core 장부의 실제 쓰기 물리 범위");
        }

        // 이웃 필드와 패딩은 보존하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (uint index = 0u; index < width; ++index)
        {
            bytes[(int)(offset + index)] = (byte)(value >> (int)(index * 8u));
        }
    }

    /// <summary>호출 인자로 이미 복제된 10바이트의 값을 읽는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="core">읽을 10바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <param name="width">읽을 폭이다.</param>
    /// <returns>조합한 값이다.</returns>
    private static uint Value(byte[] core, uint offset, uint width)
    {
        uint value = 0u;
        // 실제 인자 폭만 읽는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (uint index = 0u; index < width; ++index)
        {
            value |= (uint)core[offset + index] << (int)(index * 8u);
        }

        return value;
    }

    /// <summary>최초 호출의 주소만 고정한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="bytes">자를 바이트다.</param>
    /// <param name="offset">시작 위치다.</param>
    /// <returns>시작 위치 뒤의 뷰다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">Core 장부 초기화의 실제 시작 물리다.</exception>
    private static Span<byte> At(byte[] bytes, uint offset)
    {
        if (offset > (uint)bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "Core 장부 초기화의 실제 시작 물리");
        }

        return bytes.AsSpan((int)offset);
    }
}
