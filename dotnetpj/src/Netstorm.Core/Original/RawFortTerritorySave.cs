namespace Netstorm.Core.Original;

/// <summary>
/// 저장 부모가 실제 선택하는 두 진단 경계다. 복귀 뒤 현재 필드를 다시 읽는다
/// (cpppj <c>o/RawFortTerritorySave.h</c>의 <c>FortTerritorySaveAssertion</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-7에서 추가했다.</remarks>
public enum FortTerritorySaveAssertion
{
    /// <summary>원본 Template 521행: Territory 생성 전 같은 this의 manager가 0이다.</summary>
    MissingManager,

    /// <summary>원본 MemStream 36행: 120바이트 쓰기에 현재 용량이 모자란다.</summary>
    WriteCapacity,
}

/// <summary>
/// 기존 Territory 120바이트를 읽는 로더다. 부모 로컬 배열에 직접 쓴다.
/// </summary>
/// <param name="slot">고정 this의 Template 칸 번호다.</param>
/// <param name="records">읽은 120바이트를 받을 버퍼다. 호출자가 120바이트 배열을 준비한다.</param>
/// <remarks>2026-11-03 F2c-7에서 추가했다. 배열 참조를 그대로 전달하며 사본을 만들지 않는다.</remarks>
public delegate void FortTerritorySaveLoader(int slot, byte[] records);

/// <summary>
/// 같은 현재 ChunkMap의 첫 일치 record 6바이트를 값으로 받는 조회다.
/// </summary>
/// <param name="island">찾을 섬 번호다.</param>
/// <returns>첫 일치 record 6바이트의 복사본이다.</returns>
/// <remarks>2026-11-03 F2c-7에서 추가했다.</remarks>
public delegate byte[] FortTerritoryRecordLookup(int island);

/// <summary>
/// 기존 기록 일부만 누락 순번·공유 난수로 바꾸는 seed 효과다.
/// </summary>
/// <param name="record">바꿀 6바이트 기록이다. 호출자의 같은 배열을 직접 바꾼다.</param>
/// <param name="index">누락 순번이다.</param>
/// <remarks>2026-11-03 F2c-7에서 추가했다. <c>Span</c>을 담는 대리자는 전용 대리자로 둔다.</remarks>
public delegate void FortTerritorySeedEffect(Span<byte> record, uint index);

/// <summary>
/// 기존 자료 읽기와 현재 월드 검색만 외부에 맡기며 저장 분기·순서는 본체가 수행한다
/// (cpppj <c>o/RawFortTerritorySave.h</c>의 <c>FortTerritorySaveHooks</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-7에서 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class FortTerritorySaveHooks
{
    /// <summary>004bd660: 기존 Territory를 로컬 120바이트에 먼저 읽는다.</summary>
    public FortTerritorySaveLoader? Load { get; set; }

    /// <summary>005c85a4는 getter가 끝난 뒤 한 번 읽어 일반·Editor를 고른다.</summary>
    public Func<uint>? Special { get; set; }

    /// <summary>0046f380: clamp한 owner와 지역 0..19를 조회한다.</summary>
    public Func<int, int, int>? Island { get; set; }

    /// <summary>0046f3f0: Editor의 현재 지역을 owner 조건 없이 조회한다.</summary>
    public Func<int, int>? AnyIsland { get; set; }

    /// <summary>각 검색 결과 뒤 현재 0050c038을 다시 읽는다.</summary>
    public Func<int>? NoRegion { get; set; }

    /// <summary>00432430: 같은 현재 ChunkMap의 첫 일치 record를 값으로 받는다.</summary>
    public FortTerritoryRecordLookup? Record { get; set; }

    /// <summary>004b37c0: 기존 기록 일부만 누락 순번·공유 난수로 바꾼다.</summary>
    public FortTerritorySeedEffect? Seed { get; set; }

    /// <summary>같은 고정 Template 슬롯의 현재 manager다.</summary>
    public Func<int, uint>? Manager { get; set; }

    /// <summary>새 Territory를 만들고 반환 stream은 사용하지 않는다.</summary>
    public Action<uint>? Create { get; set; }

    /// <summary>생성 뒤 다시 읽은 manager+8의 현재 stream을 고정한다.</summary>
    public Func<uint, uint>? Current { get; set; }

    /// <summary>원본 Template 521·MemStream 36행 경계다.</summary>
    public Action<FortTerritorySaveAssertion>? Assertion { get; set; }

    /// <summary>고정 stream의 실제 16바이트·버퍼를 동기적으로 빌린다.</summary>
    public Action<uint, Action<Span<byte>, MemStreamHooks>>? Access { get; set; }
}

/// <summary>
/// 원본 Territory snapshot과 현재 섬 검색·빈 지역 seed를 같은 월드에 연결한다
/// (cpppj <c>o/RawFortTerritorySave</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-7에서 추가했다.</remarks>
public static class RawFortTerritorySave
{
    /// <summary>
    /// 선택한 경계에서만 실제 장치가 필요한지 검사한다.
    /// </summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식).</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <exception cref="InvalidOperationException">Territory 저장의 실제 장치가 연결되지 않았다.</exception>
    private static T NeedTerritory<T>(T? function) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("Territory 저장의 실제 장치 누락");
        }
        return function;
    }

    /// <summary>
    /// 비정렬 필드의 지정 폭을 little endian으로 읽고 물리 범위 밖은 거부한다.
    /// </summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <param name="width">읽을 너비다(기본 4).</param>
    /// <exception cref="ArgumentOutOfRangeException">Territory 현재 필드의 물리 범위 밖이다.</exception>
    private static uint Read(ReadOnlySpan<byte> bytes, int at, int width = 4)
    {
        if (at < 0 || at > bytes.Length || width < 0 || width > bytes.Length - at)
        {
            throw new ArgumentOutOfRangeException(nameof(at), at, "Territory 현재 필드의 물리 범위");
        }
        uint value = 0;
        // 지정 폭의 바이트만 합친다
        for (int index = 0; index < width; index++)
        {
            value |= (uint)bytes[at + index] << (index * 8);
        }
        return value;
    }

    /// <summary>
    /// 원본 stream의 지정 DWORD만 써 주변 필드를 보존한다.
    /// </summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <exception cref="ArgumentOutOfRangeException">Territory stream DWORD 물리 범위 밖이다.</exception>
    private static void Put(Span<byte> bytes, int at, uint value)
    {
        if (at < 0 || at > bytes.Length || 4 > bytes.Length - at)
        {
            throw new ArgumentOutOfRangeException(nameof(at), at, "Territory stream DWORD 물리 범위");
        }
        // 낮은 바이트부터 기록한다
        for (int index = 0; index < 4; index++)
        {
            bytes[at + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>
    /// 원본 MSVC char를 signed DWORD로 확장한다.
    /// </summary>
    /// <param name="value">바꿀 바이트 값이다.</param>
    /// <returns>부호 확장한 정수다.</returns>
    private static int SignedByte(uint value)
    {
        return (sbyte)(byte)value;
    }

    /// <summary>
    /// 004bf6d0: 기존 기록→현재 지도·빈 seed→현재 stream 재조회→120바이트 쓰기 순서다.
    /// 일반·Editor 분기는 최초 special만 보관하며 noRegion과 manager는 실제 소비 위치에서 다시 읽는다.
    /// </summary>
    /// <param name="slot">고정 this의 Template 칸 번호다.</param>
    /// <param name="hooks">실제 저장 경계다.</param>
    public static void SnapshotFortTerritory(int slot, FortTerritorySaveHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        byte[] records = new byte[120];
        NeedTerritory(hooks.Load)(slot, records);
        bool special = NeedTerritory(hooks.Special)() != 0;
        uint missing = 0;
        // 기존 record와 현재 지도 record를 같은 0..19 순서로 처리한다
        for (int region = 0; region < 20; region++)
        {
            int at = region * 6;
            if (!special)
            {
                int island = NeedTerritory(hooks.Island)(slot >= 1 && slot <= 8 ? slot : 0, region);
                if (island != NeedTerritory(hooks.NoRegion)() && (records[at + 1] & 1u) != 0)
                {
                    byte[] current = NeedTerritory(hooks.Record)(island);
                    Array.Copy(current, 0, records, at, 6);
                }
            }
            else
            {
                int island = NeedTerritory(hooks.AnyIsland)(region);
                if (island == NeedTerritory(hooks.NoRegion)())
                {
                    NeedTerritory(hooks.Seed)(records.AsSpan(at, 6), missing++);
                }
                else
                {
                    byte[] current = NeedTerritory(hooks.Record)(island);
                    Array.Copy(current, 0, records, at, 6);
                }
            }
        }
        if (NeedTerritory(hooks.Manager)(slot) == 0)
        {
            NeedTerritory(hooks.Assertion)(FortTerritorySaveAssertion.MissingManager);
        }
        NeedTerritory(hooks.Create)(NeedTerritory(hooks.Manager)(slot));
        uint stream = NeedTerritory(hooks.Current)(NeedTerritory(hooks.Manager)(slot));
        NeedTerritory(hooks.Access)(stream, (fields, memory) =>
        {
            if (unchecked((int)Read(fields, 12)) < unchecked((int)(Read(fields, 4) + 120u)))
            {
                NeedTerritory(hooks.Assertion)(FortTerritorySaveAssertion.WriteCapacity);
            }
            uint position = Read(fields, 4);
            Span<byte> buffer = NeedTerritory(memory.Buffer)(Read(fields, 0));
            if (position > (uint)buffer.Length || (uint)buffer.Length - position < (uint)records.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(position), position, "Territory 현재 출력 120바이트 물리 범위");
            }
            records.CopyTo(buffer.Slice((int)position));
            uint next = Read(fields, 4) + 120u;
            Put(fields, 4, next);
            if (unchecked((int)Read(fields, 8)) < unchecked((int)next))
            {
                Put(fields, 8, next);
            }
        });
    }

    /// <summary>
    /// 0046f380: 일반은 originalOwner와 currentOwner, Fort는 currentOwner 0 또는 요청 owner를 허용한다.
    /// count는 진입 때 보관하며 최초 일치에서 반환한다. 잘못된 큰 count는 실제 접근할 때만 거부한다.
    /// </summary>
    /// <param name="islands">현재 3204바이트 섬 정보다.</param>
    /// <param name="fort">Fort 모드면 참, 일반이면 거짓이다.</param>
    /// <param name="owner">비교할 소유자다.</param>
    /// <param name="region">비교할 지역이다.</param>
    /// <returns>첫 일치 번호, 없으면 127이다.</returns>
    public static int FindFortTerritoryIsland(ReadOnlySpan<byte> islands, bool fort, int owner, int region)
    {
        int count = unchecked((int)Read(islands, 0));
        // 현재 3204바이트의 존재·소유·지역 필드만 읽는다
        for (int index = 0; index < count; index++)
        {
            int at = 4 + index * 32;
            if (Read(islands, at + 4) == 0)
            {
                continue;
            }
            uint current = Read(islands, at + 8);
            if (fort ? (current != 0 && current != unchecked((uint)owner)) : (SignedByte(Read(islands, at + 13, 1)) != owner || current != unchecked((uint)owner)))
            {
                continue;
            }
            if (SignedByte(Read(islands, at + 12, 1)) == region)
            {
                return index;
            }
        }
        return 127;
    }

    /// <summary>
    /// 0046f3f0: 존재하는 첫 signed region 바이트 일치 항목을 반환한다.
    /// Editor 검색은 원래·현재 owner를 읽거나 별도의 유효성 조건을 추가하지 않는다.
    /// </summary>
    /// <param name="islands">현재 3204바이트 섬 정보다.</param>
    /// <param name="region">비교할 지역이다.</param>
    /// <returns>첫 일치 번호, 없으면 127이다.</returns>
    public static int FindAnyFortTerritoryIsland(ReadOnlySpan<byte> islands, int region)
    {
        int count = unchecked((int)Read(islands, 0));
        // 첫 존재 일치만 반환한다
        for (int index = 0; index < count; index++)
        {
            int at = 4 + index * 32;
            if (Read(islands, at + 4) != 0 && SignedByte(Read(islands, at + 12, 1)) == region)
            {
                return index;
            }
        }
        return 127;
    }

    /// <summary>
    /// 00432430: y 바깥·x 안쪽 순서로 같은 섬의 첫 record 6바이트를 복사하고 없으면 0을 돌려준다.
    /// 동일 ChunkMap의 실제 조회 순서이며 축약 IslandList나 Fort 파일의 과거 record를 사용하지 않는다.
    /// </summary>
    /// <param name="chunks">조회할 청크 지도다.</param>
    /// <param name="island">찾을 섬 번호다.</param>
    /// <returns>첫 일치 record 6바이트, 없으면 0이다.</returns>
    public static byte[] FortTerritoryIslandRecord(ChunkMap chunks, int island)
    {
        ArgumentNullException.ThrowIfNull(chunks);
        // 현재 y 행을 먼저 고정한다
        for (int y = 0; y < chunks.YLength(); y++)
        {
            // 같은 행의 x를 먼저 진행한다
            for (int x = 0; x < chunks.XLength(); x++)
            {
                ref ChunkEntry entry = ref chunks.At(x, y);
                if ((sbyte)entry.Island == island)
                {
                    byte[] found = new byte[6];
                    Array.Copy(entry.Record, found, 6);
                    return found;
                }
            }
        }
        return new byte[6];
    }

    /// <summary>
    /// 004b37c0: 첫 두 바이트를 먼저 바꾸고 Next(254)가 복귀한 뒤 byte3만 쓴다.
    /// 순번은 원본 인자의 하위 바이트에서 6비트만 쓰고 콜백 전에 기록한 필드도 되돌리지 않는다.
    /// </summary>
    /// <param name="record">바꿀 6바이트 기록이다.</param>
    /// <param name="index">누락 순번이다.</param>
    /// <param name="random">공유 난수 조회다(254 상한).</param>
    public static void SeedFortTerritoryRecord(Span<byte> record, uint index, Func<uint, uint> random)
    {
        if (record.Length < 6)
        {
            throw new ArgumentOutOfRangeException(nameof(record), record.Length, "Territory seed 기록 6바이트 물리 범위");
        }
        record[0] = (byte)((record[0] & 0xc0u) | (index & 0x3fu));
        record[1] = (byte)((record[1] & 0xf7u) | 3u);
        uint value = NeedTerritory(random)(254u);
        record[3] = (byte)(value + 1u);
    }
}
