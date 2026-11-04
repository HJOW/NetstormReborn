namespace Netstorm.Core.Original;

/// <summary>
/// 실제 100개 32바이트 목록의 현재 owner 변경을 같은 pool·해시·목록·Player 상태에서 실행한다
/// (cpppj <c>o/RawIslandRuntime.h</c>·<c>o/RawIslandRuntime.cpp</c>의 상수·저장소·진단·훅·클래스).
/// </summary>
/// <remarks>2026-11-04 F2c-11에서 추가했다. 경로 탐색기 전체가 아니라 거주 구역 owner 전파에 쓰는 표면 조회만 <see cref="RawPathSurface"/>로 빌린다.</remarks>
public static class IslandRuntimeShape
{
    /// <summary>기록 하나의 바이트 수다.</summary>
    public const int Stride = 32;

    /// <summary>Reset이 대입하는 -10000 float 비트다.</summary>
    public const uint UnsetBounds = 0xc61c4000u;

    /// <summary>owner 변경 몸체가 Player 첫 지역과 직접 비교하는 고정 DWORD다.</summary>
    public const int UnassignedFirstRegion = 127;
}

/// <summary>
/// 실행기가 빌리는 같은 물리의 count DWORD·100개 32바이트 기록이다
/// (cpppj <c>o/RawIslandRuntime.h</c>의 <c>RawIslandStorage</c>).
/// </summary>
/// <remarks>2026-11-04 F2c-11에서 추가했다. 호출자가 소유하며 이 모듈보다 오래 살아야 한다.</remarks>
public sealed class RawIslandStorage
{
    /// <summary>원문 00568b60이 쓰는 실제 3204바이트 범위다.</summary>
    public const int Bytes = 4 + 100 * 32;

    /// <summary>최초 테스트 입력이며 실제 할당 용도로 주장하지 않는다.</summary>
    public byte[] Records { get; } = new byte[Bytes];
}

/// <summary>
/// 실제 섬 목록이 보고하는 진단이며 보고 뒤 원본처럼 조건부 작업을 이어간다
/// (cpppj <c>o/RawIslandRuntime.h</c>의 <c>IslandRuntimeAssertion</c>).
/// </summary>
/// <remarks>2026-11-04 F2c-11에서 추가했다.</remarks>
public enum IslandRuntimeAssertion
{
    /// <summary>존재·범위 검사에 실패한 기록의 getter·setter가 요청됐다.</summary>
    MissingIsland,

    /// <summary>owner 변경의 원본 경계 검사에 실패한 뒤 현재 거주 구역 파괴 전에 보고한다.</summary>
    InvalidBounds,
}

/// <summary>
/// 지원·유효·현재 거주 구역 가상 owner를 전달하고 원본 진단을 대비에게 보고한다
/// (cpppj <c>o/RawIslandRuntime.h</c>의 <c>IslandRuntimeHooks</c>).
/// </summary>
/// <remarks>2026-11-04 F2c-11에서 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class IslandRuntimeHooks
{
    /// <summary>현재 residences의 실제 SID에 가상 +74를 실행한다. 같은 raw pool도 갱신해야 한다.</summary>
    public Action<int, uint>? SetResidenceOwner { get; set; }

    /// <summary>원본 진단 뒤 계속 진행되는 경계를 보존한다. 빈 효과로 복원 완료를 주장하지 않는다.</summary>
    public Action<IslandRuntimeAssertion>? Assertion { get; set; }
}

/// <summary>
/// 위치 0046edf0..0046f320과 CD 004bd3f0..004bda70의 mutable IslandList 몸체다
/// (cpppj <c>o/RawIslandRuntime</c>).
/// </summary>
/// <remarks>2026-11-04 F2c-11에서 추가했다. 기록·모드·목록·전역은 이 객체보다 오래 살아야 한다.</remarks>
public sealed class RawIslandRuntime
{
    /// <summary>실제 3204바이트 저장소와 필수 하위 효과·기존 모드·Player·목록을 확인한다.</summary>
    /// <param name="pool">가상 owner와 현재 타입·좌표·표면 owner 조회에 쓰는 같은 raw 풀이다.</param>
    /// <param name="hash">기존 표면 조회가 읽는 현재 hash0 단어 지도다.</param>
    /// <param name="records">호출자가 소유하는 같은 물리를 빌린다.</param>
    /// <param name="modes">기존 FortFinishOwner의 fort·special·chaff·Player 첫 지역과 거주 구역 타입을 공유한다.</param>
    /// <param name="residences">거주 건물이다. AssetPostPop의 실제 등록 목록이며 매 반복 현재 count·항목을 다시 읽는다.</param>
    /// <param name="worldRedraw">위치 0059a8b0의 지도·확장 공유 값이다.</param>
    /// <param name="setWorldRedraw">실제 owner 변경이 증가시키는 같은 값의 쓰기다.</param>
    /// <param name="mapRevision">위치 005949d4의 증가 값이다. CD에는 별도 참조가 없다.</param>
    /// <param name="setMapRevision">패치판 실제 owner 변경이 1로 두는 같은 값의 쓰기다.</param>
    /// <param name="hooks">필수 실제 거주 구역 owner와 원본 진단 경계다.</param>
    /// <param name="worldExtent">실제 isValid가 float로 읽는 원본 월드 한 DWORD다.</param>
    /// <exception cref="ArgumentException">섬 목록의 실제 기록·필수 효과 누락이다.</exception>
    public RawIslandRuntime(
        RawSidPool pool,
        RawSquidHash hash,
        byte[] records,
        FortFinishOwnerState modes,
        RawPostPopList residences,
        Func<uint> worldRedraw,
        Action<uint> setWorldRedraw,
        Func<uint> mapRevision,
        Action<uint> setMapRevision,
        IslandRuntimeHooks hooks,
        int worldExtent = 256)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hash);
        ArgumentNullException.ThrowIfNull(records);
        ArgumentNullException.ThrowIfNull(modes);
        ArgumentNullException.ThrowIfNull(residences);
        ArgumentNullException.ThrowIfNull(worldRedraw);
        ArgumentNullException.ThrowIfNull(setWorldRedraw);
        ArgumentNullException.ThrowIfNull(mapRevision);
        ArgumentNullException.ThrowIfNull(setMapRevision);
        ArgumentNullException.ThrowIfNull(hooks);
        if (records.Length != RawIslandStorage.Bytes || hooks.SetResidenceOwner is null || hooks.Assertion is null)
        {
            throw new ArgumentException("IslandRuntime 실제 기록·필수 효과 누락");
        }
        _pool = pool;
        _hash = hash;
        _records = records;
        _modes = modes;
        _residences = residences;
        _worldRedraw = worldRedraw;
        _setWorldRedraw = setWorldRedraw;
        _mapRevision = mapRevision;
        _setMapRevision = setMapRevision;
        _hooks = hooks;
        _worldExtent = worldExtent;
    }

    /// <summary>가상 owner의 현재 타입·좌표·표면 owner 조회에 쓰는 같은 raw 풀이다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>기존 표면 조회가 읽는 현재 hash0 단어 지도다.</summary>
    private readonly RawSquidHash _hash;

    /// <summary>호출자가 소유하는 명시 독립 호출자가 소유하는 실제 count·100개 mutable 32바이트 기록이다.</summary>
    private readonly byte[] _records;

    /// <summary>기존 FortFinishOwner의 fort·special·chaff·Player 첫 지역과 거주 구역 타입을 공유한다.</summary>
    private readonly FortFinishOwnerState _modes;

    /// <summary>거주 건물이다. AssetPostPop의 실제 등록 목록이다.</summary>
    private readonly RawPostPopList _residences;

    /// <summary>위치 0059a8b0의 지도·확장 공유 값의 읽기다.</summary>
    private readonly Func<uint> _worldRedraw;

    /// <summary>실제 owner 변경이 증가시키는 같은 값의 쓰기다.</summary>
    private readonly Action<uint> _setWorldRedraw;

    /// <summary>위치 005949d4의 증가 값의 읽기다.</summary>
    private readonly Func<uint> _mapRevision;

    /// <summary>패치판 실제 owner 변경이 1로 두는 같은 값의 쓰기다.</summary>
    private readonly Action<uint> _setMapRevision;

    /// <summary>필수 실제 거주 구역 owner와 원본 진단 경계다.</summary>
    private readonly IslandRuntimeHooks _hooks;

    /// <summary>실제 isValid가 float로 읽는 원본 월드 한 DWORD다.</summary>
    private readonly int _worldExtent;

    /// <summary>잘못된 물리 크기를 쓰기 전에 거부하고 같은 Player 주소만 빌린다.</summary>
    private byte[] _players = [];

    /// <summary>MSVC 부호 있는 char를 같은 비트의 부호 있는 DWORD로 확장한다.</summary>
    /// <param name="value">바이트 값이다.</param>
    /// <returns>부호 확장한 값이다.</returns>
    private static int SignedByte(uint value)
    {
        return unchecked((int)unchecked((sbyte)unchecked((byte)value)));
    }

    /// <summary>CD isValid의 초기 검사는 float 변환이 아닌 원본 DWORD의 부호 있는 양수 검사다.</summary>
    /// <param name="value">검사할 float 값이다.</param>
    /// <returns>원본 비트가 양수이면 참이다.</returns>
    private static bool PositiveBits(float value)
    {
        return BitConverter.SingleToInt32Bits(value) > 0;
    }

    /// <summary>실제 필드의 낮은 바이트부터 쓰며 정렬·구조체 채움을 가정하지 않는다.</summary>
    /// <param name="records">쓸 기록이다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓰는 바이트 수다.</param>
    private static void WriteIslandField(byte[] records, int offset, uint value, int width = 4)
    {
        // 해당 원문 필드의 폭만 쓴다
        for (int index = 0; index < width; index++)
        {
            records[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>Delete와 전체 Reset은 같은 한 기록 몸체를 사용하며 물리 밖 입력은 쓰기 전에 거부한다.</summary>
    /// <param name="records">쓸 기록이다.</param>
    /// <param name="island">초기화할 기록 번호다.</param>
    /// <exception cref="ArgumentOutOfRangeException">섬 Reset의 실제 3204바이트·기록 범위 오류다.</exception>
    private static void ResetIslandRecord(byte[] records, int island)
    {
        if (records.Length != RawIslandStorage.Bytes || island < 0 || island >= 100)
        {
            throw new ArgumentOutOfRangeException(nameof(island), island, "Island Reset 실제 3204바이트·기록 범위 오류");
        }
        // count 다음의 현재 32바이트 기록 시작이다
        int record = 4 + island * IslandRuntimeShape.Stride;
        WriteIslandField(records, record, 0);
        WriteIslandField(records, record + 4, 0);
        WriteIslandField(records, record + 8, 0);
        WriteIslandField(records, record + 12, 255, 1);
        WriteIslandField(records, record + 13, 0, 1);
        WriteIslandField(records, record + 20, IslandRuntimeShape.UnsetBounds);
        WriteIslandField(records, record + 16, IslandRuntimeShape.UnsetBounds);
        WriteIslandField(records, record + 28, IslandRuntimeShape.UnsetBounds);
        WriteIslandField(records, record + 24, IslandRuntimeShape.UnsetBounds);
    }

    /// <summary>세션 없는 정리도 같은 물리의 100행 지정 필드와 마지막 count를 직접 초기화한다.</summary>
    /// <param name="records">초기화할 3204바이트 기록이다.</param>
    /// <exception cref="ArgumentException">섬 Reset의 실제 3204바이트 범위 오류다.</exception>
    public static void ResetRawIslandRecords(byte[] records)
    {
        ArgumentNullException.ThrowIfNull(records);
        if (records.Length != RawIslandStorage.Bytes)
        {
            throw new ArgumentException("Island Reset 실제 3204바이트 범위 오류", nameof(records));
        }
        // 실제 100행을 모두 쓰며 채움 2바이트는 유지한다
        for (int island = 0; island < 100; island++)
        {
            ResetIslandRecord(records, island);
        }
        WriteIslandField(records, 0, 0);
    }

    /// <summary>원본 바이트 필드를 낮은 바이트부터 결합한다.</summary>
    /// <param name="offset">읽을 위치다.</param>
    /// <param name="width">읽을 바이트 수다.</param>
    /// <returns>결합한 값이다.</returns>
    private uint Read(int offset, int width = 4)
    {
        uint value = 0;
        // 원본 지정 폭을 넘어 채움이나 이웃을 읽지 않는다
        for (int index = 0; index < width; index++)
        {
            value |= (uint)_records[offset + index] << (index * 8);
        }
        return value;
    }

    /// <summary>원본 지정 폭의 낮은 바이트부터 대입한다.</summary>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓰는 바이트 수다.</param>
    private void Write(int offset, uint value, int width = 4)
    {
        // 필드 이외의 바이트는 그대로 남긴다
        for (int index = 0; index < width; index++)
        {
            _records[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>범위 밖 원본 포인터 산술을 호스트 배열 아래쪽 넘침으로 수행하지 않는다.</summary>
    /// <param name="island">기록 번호다.</param>
    /// <returns>기록 시작 위치다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">IslandRuntime 물리 기록 범위 오류다.</exception>
    private static int Record(int island)
    {
        if (island < 0 || island >= 100)
        {
            throw new ArgumentOutOfRangeException(nameof(island), island, "IslandRuntime 물리 기록 범위 오류");
        }
        return 4 + island * IslandRuntimeShape.Stride;
    }

    /// <summary>원본은 DWORD 3개·바이트 2개·float 4개만 초기화한다.</summary>
    /// <param name="island">초기화할 기록 번호다.</param>
    private void ResetRecord(int island)
    {
        ResetIslandRecord(_records, island);
    }

    /// <summary>count와 100개 기록의 실제 Reset이며 배치 축약 IslandList를 새로 만들지 않는다.</summary>
    public void Reset()
    {
        ResetRawIslandRecords(_records);
    }

    /// <summary>실제 부호 없는 DWORD count를 진단·일반 소비자에게 전달한다.</summary>
    /// <returns>현재 count다.</returns>
    public uint Count()
    {
        return Read(0);
    }

    /// <summary>원본 부호 있는 범위 검사와 존재 DWORD 반환을 그대로 보존한다.</summary>
    /// <param name="island">조회할 기록 번호다.</param>
    /// <returns>존재 DWORD다. 범위 밖은 0이다.</returns>
    public uint Exists(int island)
    {
        if (island < 0 || island >= unchecked((int)Count()))
        {
            return 0;
        }
        return Read(Record(island) + 4);
    }

    /// <summary>없는 기록 진단 뒤 현재 Exists를 다시 읽어 장치의 원본 상태 변경도 보존한다.</summary>
    /// <param name="island">검사할 기록 번호다.</param>
    /// <param name="bothEditions">두 판본 모두 진단할지다.</param>
    /// <returns>기록이 있으면 참이다.</returns>
    private bool Require(int island, bool bothEditions = false)
    {
        if (Exists(island) == 0 && (bothEditions || _pool.Edition == OriginalEdition.Patch1078))
        {
            Need(_hooks.Assertion)(IslandRuntimeAssertion.MissingIsland);
        }
        return Exists(island) != 0;
    }

    /// <summary>원본 Add의 EAX는 양판본 모두 이전 count이며 나머지 신전·범위·채움은 유지한다.</summary>
    /// <param name="owner">현재 owner DWORD다.</param>
    /// <param name="territory">원래 owner 바이트와 함께 쓰는 지역 바이트다.</param>
    /// <returns>추가 전 count(새 기록 번호)다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">IslandRuntime Add의 원본 100기록 경계다.</exception>
    public uint Add(uint owner, byte territory)
    {
        uint index = Count();
        if (index >= 100)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), owner, "IslandRuntime Add 원본 100기록 경계");
        }
        int record = Record((int)index);
        Write(0, index + 1);
        Write(record + 8, owner);
        Write(record + 13, owner, 1);
        Write(record + 4, 1);
        Write(record + 12, territory, 1);
        return index;
    }

    /// <summary>삭제는 기록만 초기화하고 count·원래 번호 재사용 정책을 변경하지 않는다.</summary>
    /// <param name="island">삭제할 기록 번호다.</param>
    public void Delete(int island)
    {
        if (Require(island))
        {
            ResetRecord(island);
        }
    }

    /// <summary>현재 owner가 바뀌어도 일반 모드의 원래 owner 바이트는 유지된다.</summary>
    /// <param name="island">검사할 기록 번호다.</param>
    /// <param name="owner">비교할 부호 있는 원래 owner다.</param>
    /// <param name="territory">비교할 부호 있는 지역 바이트다.</param>
    /// <returns>일치하면 참이다.</returns>
    public bool Matches(int island, int owner, int territory)
    {
        if (Exists(island) == 0)
        {
            return false;
        }
        int record = Record(island);
        if (!_modes.Special && !_modes.ChaffMode)
        {
            if (SignedByte(Read(record + 13, 1)) != owner)
            {
                return false;
            }
        }
        else if (Read(record + 8) == 0)
        {
            return false;
        }
        return SignedByte(Read(record + 12, 1)) == territory;
    }

    /// <summary>신전 getter는 패치 진단 뒤 기록이 여전히 없으면 0이다.</summary>
    /// <param name="island">조회할 기록 번호다.</param>
    /// <returns>현재 record 신전 DWORD다.</returns>
    public uint Temple(int island)
    {
        return Require(island) ? Read(Record(island)) : 0;
    }

    /// <summary>현재 record 신전 DWORD를 원본 그대로 대입한다.</summary>
    /// <param name="island">쓸 기록 번호다.</param>
    /// <param name="temple">쓸 신전 DWORD다.</param>
    public void Temple(int island, uint temple)
    {
        if (Require(island))
        {
            Write(Record(island), temple);
        }
    }

    /// <summary>owner getter는 원래 owner 바이트를 반환하지 않는다.</summary>
    /// <param name="island">조회할 기록 번호다.</param>
    /// <returns>현재 owner DWORD다.</returns>
    public uint Owner(int island)
    {
        return Require(island) ? Read(Record(island) + 8) : 0;
    }

    /// <summary>논리 부재 진단 뒤에도 물리 100기록 안의 원본 float 4개는 읽는다.</summary>
    /// <param name="island">조회할 기록 번호다.</param>
    /// <returns>현재 범위 사각형이다.</returns>
    public FortRandomRectangle Bounds(int island)
    {
        Require(island, true);
        int record = Record(island);
        return new FortRandomRectangle(
            BitConverter.UInt32BitsToSingle(Read(record + 16)),
            BitConverter.UInt32BitsToSingle(Read(record + 20)),
            BitConverter.UInt32BitsToSingle(Read(record + 24)),
            BitConverter.UInt32BitsToSingle(Read(record + 28)));
    }

    /// <summary>실제 float 비트만 대입하며 경계 유효성 검사는 owner 변경 때 별도 실행한다.</summary>
    /// <param name="island">쓸 기록 번호다.</param>
    /// <param name="bounds">쓸 범위 사각형이다.</param>
    public void Bounds(int island, FortRandomRectangle bounds)
    {
        if (!Require(island))
        {
            return;
        }
        int record = Record(island);
        float[] values = [bounds.Left, bounds.Top, bounds.Right, bounds.Bottom];
        // 전달받은 네 float를 그대로 대입한다
        for (int index = 0; index < values.Length; index++)
        {
            Write(record + 16 + index * 4, BitConverter.SingleToUInt32Bits(values[index]));
        }
    }

    /// <summary>패치는 두 유효 점·포함 사각형, CD는 양수 DWORD 비트와 부분 순서 없음 수용 조건을 사용한다.</summary>
    /// <param name="bounds">검사할 범위 사각형이다.</param>
    /// <returns>유효하면 참이다.</returns>
    private bool ValidBounds(FortRandomRectangle bounds)
    {
        float extent = _worldExtent;
        if (_pool.Edition == OriginalEdition.Patch1078)
        {
            return bounds.Left > 0 && bounds.Left < extent && bounds.Top > 0 && bounds.Top < extent
                && bounds.Right > 0 && bounds.Right < extent && bounds.Bottom > 0 && bounds.Bottom < extent
                && bounds.Left <= bounds.Right && bounds.Top <= bounds.Bottom;
        }
        return PositiveBits(bounds.Left) && PositiveBits(bounds.Top) && PositiveBits(bounds.Right) && PositiveBits(bounds.Bottom)
            && bounds.Left < extent && !(bounds.Top >= extent) && !(bounds.Right >= extent) && !(bounds.Bottom >= extent)
            && bounds.Right >= bounds.Left && !(bounds.Top > bounds.Bottom);
    }

    /// <summary>실제 후보 검사는 범위의 양끝을 포함하며 CD 왼쪽·위쪽의 순서 없음만 추가로 허용한다.</summary>
    /// <param name="bounds">검사할 범위 사각형이다.</param>
    /// <param name="x">후보 X 좌표다.</param>
    /// <param name="y">후보 Y 좌표다.</param>
    /// <returns>범위 안이면 참이다.</returns>
    private bool Contains(FortRandomRectangle bounds, float x, float y)
    {
        if (_pool.Edition == OriginalEdition.Patch1078)
        {
            return bounds.Left <= x && x <= bounds.Right && bounds.Top <= y && y <= bounds.Bottom;
        }
        return !(bounds.Left > x) && x <= bounds.Right && !(bounds.Top > y) && y <= bounds.Bottom;
    }

    /// <summary>owner 변경의 각 전역 쓰기·현재 목록·가상 효과·Player 첫 지역을 원본 순서로 처리한다.</summary>
    /// <param name="island">바꿀 기록 번호다.</param>
    /// <param name="owner">새 현재 owner DWORD다.</param>
    /// <exception cref="ArgumentOutOfRangeException">섬 목록의 현재 목록·Player 첫 지역 범위 오류다.</exception>
    public void Owner(int island, uint owner)
    {
        if (!Require(island) || (_modes.Fort && !_modes.Special))
        {
            return;
        }
        int record = Record(island);
        Write(record + 8, owner);
        _setWorldRedraw(unchecked(_worldRedraw() + 1));
        if (_pool.Edition == OriginalEdition.Patch1078)
        {
            _setMapRevision(1);
        }
        FortRandomRectangle bounds = new(
            BitConverter.UInt32BitsToSingle(Read(record + 16)),
            BitConverter.UInt32BitsToSingle(Read(record + 20)),
            BitConverter.UInt32BitsToSingle(Read(record + 24)),
            BitConverter.UInt32BitsToSingle(Read(record + 28)));
        if (!ValidBounds(bounds))
        {
            Need(_hooks.Assertion)(IslandRuntimeAssertion.InvalidBounds);
        }
        int ownerOffset = _pool.Edition == OriginalEdition.Patch1078
            ? FortFinishOwnerShape.PatchOwnerOffset
            : FortFinishOwnerShape.CdOwnerOffset;
        // 원본은 목록 count·SID·타입·좌표와 표면 단어를 매 반복 다시 읽고 빈·죽음 선택을 추가하지 않는다
        for (uint index = 0; unchecked((int)index) < unchecked((int)_residences.Count); index++)
        {
            if (index >= (uint)_residences.Entries.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(island), island, "IslandRuntime 현재 목록 저장소 부족");
            }
            uint number = _residences.Entries[(int)index];
            if (number >= (uint)_pool.Capacity || number > 65535)
            {
                throw new ArgumentOutOfRangeException(nameof(island), island, "IslandRuntime 거주 구역 SID 범위 오류");
            }
            int sid = (int)number;
            ReadOnlySpan<byte> raw = _pool.Slot(sid);
            if (raw[FortFinishOwnerShape.TypeOffset] != _modes.ResidenceType)
            {
                continue;
            }
            float x = BitConverter.UInt32BitsToSingle(Get(raw, 14));
            float y = BitConverter.UInt32BitsToSingle(Get(raw, 18));
            if (!Contains(bounds, x, y))
            {
                continue;
            }
            int support = RawPathSurface.SurfaceAt(_hash, new RawPathPoint { X = x, Y = y });
            Need(_hooks.SetResidenceOwner)(sid, _pool.Slot(support)[ownerOffset]);
        }
        if (owner != 0)
        {
            if (owner >= (uint)_modes.PlayerRegions.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(island), island, "IslandRuntime Player 첫 지역 범위 오류");
            }
            if (FirstRegion(owner) == IslandRuntimeShape.UnassignedFirstRegion)
            {
                FirstRegion(owner, island);
            }
        }
    }

    /// <summary>같은 mutable 100개 기록을 외부 소비자에게 제공한다.</summary>
    /// <returns>현재 기록의 읽기 전용 보기다.</returns>
    public ReadOnlySpan<byte> Bytes()
    {
        return _records;
    }

    /// <summary>실제 pool 참조를 월드 묶음 검사에 전달한다.</summary>
    /// <returns>이 모듈이 쓰는 풀이다.</returns>
    public RawSidPool Pool()
    {
        return _pool;
    }

    /// <summary>잘못된 물리 크기를 쓰기 전에 거부하고 같은 Player 주소만 빌린다.</summary>
    /// <param name="players">원본 9개 Player 기록이다. 이 객체보다 오래 살아야 한다.</param>
    /// <exception cref="ArgumentException">IslandRuntime 실제 9개 Player 크기 오류다.</exception>
    public void BindPlayerRecords(byte[] players)
    {
        ArgumentNullException.ThrowIfNull(players);
        // 실제 판본별 한 기록의 크기다
        uint stride = _pool.Edition == OriginalEdition.Patch1078 ? 0xb4u : 0xacu;
        if ((uint)players.Length != 9 * stride)
        {
            throw new ArgumentException("IslandRuntime 실제 9개 Player 크기 오류", nameof(players));
        }
        _players = players;
    }

    /// <summary>외부 거주 구역 장치 뒤의 현재 Player DWORD를 다시 읽는다.</summary>
    /// <param name="owner">조회할 소유자다.</param>
    /// <returns>첫 지역이다.</returns>
    private int FirstRegion(uint owner)
    {
        if (_players.Length == 0)
        {
            return _modes.PlayerRegions[(int)owner];
        }
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        // 첫 지역의 판본별 위치를 선택한다
        uint field = owner * (patch ? 0xb4u : 0xacu) + (patch ? 0x78u : 0x70u);
        // 현재 기록에서 읽은 원본 비트다
        uint value = 0;
        // 정확한 네 바이트만 조합한다
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)_players[(int)(field + (uint)index)] << (index * 8);
        }
        return unchecked((int)value);
    }

    /// <summary>공유 기록 모드에서는 첫 지역 필드만 저장한다. 다른 Player 메모리와 독립 상태는 보존한다.</summary>
    /// <param name="owner">쓸 소유자다.</param>
    /// <param name="region">쓸 첫 지역이다.</param>
    private void FirstRegion(uint owner, int region)
    {
        if (_players.Length == 0)
        {
            _modes.PlayerRegions[(int)owner] = region;
            return;
        }
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        // 실제 저장 폭은 두 판본 모두 DWORD다
        uint field = owner * (patch ? 0xb4u : 0xacu) + (patch ? 0x78u : 0x70u);
        // 부호와 무관하게 원본 비트를 저장한다
        uint value = unchecked((uint)region);
        // 인접 신전·거주 구역 필드를 바꾸지 않는다
        for (int index = 0; index < 4; index++)
        {
            _players[(int)(field + (uint)index)] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>원본 little endian 바이트 필드를 DWORD로 읽는다.</summary>
    /// <param name="raw">읽을 슬롯이다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>결합한 값이다.</returns>
    private static uint Get(ReadOnlySpan<byte> raw, int offset)
    {
        uint value = 0;
        // 비정렬 좌표 DWORD의 지정 4바이트만 결합한다
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)raw[offset + index] << (index * 8);
        }
        return value;
    }

    /// <summary>도달한 미연결 장치만 거부하여 앞선 조회·생성의 부분 효과를 보존한다.</summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식).</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    /// <exception cref="InvalidOperationException">섬 목록의 실제 장치가 연결되지 않았다.</exception>
    private static T Need<T>(T? function) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("IslandRuntime 실제 장치 누락");
        }
        return function;
    }
}
