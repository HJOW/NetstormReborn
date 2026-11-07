// 섬 배치 후의 거주 구역·초기 객체 생성과 레코드 후처리를 같은 실제 월드에 연결한다.
using System;
using System.Buffers.Binary;
using Netstorm.Core.Simulation;

// 10.78 Islandbuilder/Addrandom의 실제 부모를 담는 네임스페이스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
namespace Netstorm.Core.Original;

// 원문 c2/139/c3/d1/118 진단 보고 뒤 현재 상태로 계속하기 위한 열거형이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
public enum IslandPopulationAssertion
{
    BoundsMissing,
    RecordMissing,
    ResidenceAuthority,
    PopulationAuthority,
    MissingPlacement,
}

// 원문 00540bc4/00594fb8 및 5411bc/541174/54126c/5412d0의 현재 장기 전역을 담는 클래스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
public sealed class IslandPopulationGlobals
{
    // 원문 00540bc4의 현재 boss 권한 불리언 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public bool Boss;

    // 원문 00594fb8의 현재 fort 모드 불리언 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public bool Fort;

    // 원문 005411bc의 현재 residence TYPE 번호 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public uint Residence;

    // 원문 00541174의 현재 primary 후보 TYPE 번호 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public uint Primary;

    // 원문 0054126c의 현재 fallback 후보 TYPE 번호 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public uint Fallback;

    // 원문 005412d0의 현재 replacement 생성 TYPE 번호 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public uint Replacement;
}

// 섬 인구/초기 객체 교체 시 필요한 외부 콜백을 묶는 훅 클래스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
public sealed class IslandPopulationHooks
{
    // 같은 실제 보고 장치이며 콜백이 바꾼 현재 권한도 다시 읽는 진단 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Action<IslandPopulationAssertion>? Assertion { get; set; }

    // Fort 모드 004bcab0의 실제 현재 시각을 읽는 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Func<double>? WallSeconds { get; set; }

    // 004adea0의 현재 TYPE 발자국/2를 뺀 중심을 삭제 전에 얻는 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Func<int, (float X, float Y)>? Position { get; set; }

    // 가상+10의 전체 삭제를 위치 스냅샷 뒤에 실행하는 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Action<int, uint>? Destroy { get; set; }

    // 004af530의 실제 생성/초기화를 수행하는 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Func<uint, uint, int>? Create { get; set; }

    // 현재 TYPE+118의 기본 프레임을 조회하는 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Func<uint, int>? DefaultFrame { get; set; }

    // 0049fce0의 타입 최대 체력을 조회하는 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Func<uint, int>? TypeHitPoints { get; set; }

    // 004adc60의 현재 실제 객체/TYPE 체력 유무 판정 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Func<int, bool>? HasHitPoints { get; set; }

    // 가상+74이며 실제 지역의 현재 owner를 전달하는 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Action<int, uint>? SetOwner { get; set; }

    // 가상+90의 실제 공간·표시·postPop을 실행하는 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Action<int, float, float, uint>? Pop { get; set; }

    // 004324e0의 모든 레코드 쓰기 뒤 0047d890 생산 갱신을 호출하는 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Action? RefreshProduction { get; set; }
}

// 원문의 스냅샷/재조회/반복·직접 DWORD/WORD 쓰기 위치를 보존하는 섬 인구 생성기 클래스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
public sealed class RawIslandPopulation
{
    // 실제 현재 메모리의 SID 풀 참조 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly RawSidPool _pool;

    // 경계 조회와 레코드 기록 대상이 되는 대상 청크 지도 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly ChunkMap _target;

    // 레코드 시드 조회 대상이 되는 월드 청크 지도 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly ChunkMap _world;

    // 섬 존재 여부와 소유자를 조회하는 섬 런타임 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly RawIslandRuntime _islands;

    // 기존 완료 단일 무작위 배치 몸체 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly RawFortFinishOwner _placement;

    // 현재 전역 참조를 담는 전역 객체 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly IslandPopulationGlobals _globals;

    // 선택된 실제 장치를 담는 훅 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly IslandPopulationHooks _hooks;

    // 모든 자료가 같은 실제 Pool인지 검증하고 참조를 연결하는 생성자다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public RawIslandPopulation(
        RawSidPool pool,
        ChunkMap target,
        ChunkMap world,
        RawIslandRuntime islands,
        RawFortFinishOwner randomPlacement,
        IslandPopulationGlobals globals,
        IslandPopulationHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(target);
        ArgumentNullException.ThrowIfNull(world);
        ArgumentNullException.ThrowIfNull(islands);
        ArgumentNullException.ThrowIfNull(randomPlacement);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 ||
            !ReferenceEquals(islands.Pool(), pool) ||
            !ReferenceEquals(randomPlacement.Pool(), pool))
        {
            throw new ArgumentException("섬 population의 실제 PATCH/Pool 불일치");
        }

        _pool = pool;
        _target = target;
        _world = world;
        _islands = islands;
        _placement = randomPlacement;
        _globals = globals;
        _hooks = hooks;
    }

    // 상위 Builder/World와 같은 실제 저장소만 조립하기 위한 SID 풀 조회 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public RawSidPool Pool() => _pool;

    // 0046df80 → 현재 world record seed → 0040e130 → 기존 실제 0040ded0 순서를 실행하는 거주 구역 배치 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public void Residence(int region)
    {
        FortRandomRectangle area = Bounds(region);
        if (!_globals.Boss)
        {
            Need(_hooks.Assertion)(IslandPopulationAssertion.ResidenceAuthority);
            if (!_globals.Boss)
            {
                return;
            }
        }

        ResidenceIn(area, region, 1u);
    }

    // 0046e000의 사각형/레코드 스냅샷 → Residence → 교체 객체 → bit 0x800 / 모든 청크 / 생산 통지를 수행하는 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public void Populate(int region)
    {
        FortRandomRectangle area = Bounds(region);
        byte[] record = RawFortTerritorySave.FortTerritoryIslandRecord(_target, region);
        if (!_globals.Boss)
        {
            Need(_hooks.Assertion)(IslandPopulationAssertion.PopulationAuthority);
            if (!_globals.Boss)
            {
                return;
            }
        }

        Residence(region);
        ReplaceInitial(area, region);
        record[1] |= 8;
        WriteRecord(region, record);
    }

    // 0040e2b0의 중앙 10×10 두 종류 최대 10회 / 선택 객체 교체를 수행하는 메서드다. 짧은 변의 원문 음수 대체식도 유지하며, 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public void ReplaceInitial(FortRandomRectangle area, int region)
    {
        FortRandomRectangle rectangle = Center(area);
        byte[] record = RawFortTerritorySave.FortTerritoryIslandRecord(_world, region);
        NetstormRandom random = new((uint)record[3] * 103u);
        if (_globals.Fort)
        {
            random.SetState(RawFortFinishOwner.TimeSeed(Need(_hooks.WallSeconds)()));
        }

        uint result = 0u;
        // 기본 타입으로 최대 10회 무작위 배치를 시도하며 각 시도에 현재 값을 재조회하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (uint attempt = 0u; attempt < 10u && result == 0u; ++attempt)
        {
            result = _placement.PlaceRandom(random, rectangle, region, 1u, _globals.Primary, 0u);
        }

        if (result == 0u)
        {
            // 기본 10회가 모두 0일 때만 대체 TYPE를 읽어 최대 10회 시도하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            for (uint attempt = 0u; attempt < 10u && result == 0u; ++attempt)
            {
                result = _placement.PlaceRandom(random, rectangle, region, 1u, _globals.Fallback, 0u);
            }
        }

        if (result == 0u)
        {
            Need(_hooks.Assertion)(IslandPopulationAssertion.MissingPlacement);
        }

        (float posX, float posY) = Need(_hooks.Position)(Candidate(result));
        Need(_hooks.Destroy)(Candidate(result), 0u);
        int created = Need(_hooks.Create)(_globals.Replacement, 0u);
        Span<byte> raw = _pool.AllocatedBytes(created);
        byte type = raw[10];
        BinaryPrimitives.WriteInt32LittleEndian(raw.Slice(36, 4), Need(_hooks.DefaultFrame)(type));
        if (Need(_hooks.HasHitPoints)(created))
        {
            RawSquidHitPoints.Set(_pool, created, Need(_hooks.TypeHitPoints)(_pool.Slot(created)[10]));
        }

        Need(_hooks.SetOwner)(created, _islands.Owner(region));
        Need(_hooks.Pop)(created, posX, posY, 0u);
    }

    // 선택된 콜백의 부재만 그 위치에서 거부하는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static T Need<T>(T? callback)
        where T : class
    {
        if (callback is null)
        {
            throw new InvalidOperationException("섬 population의 실제 하위 미연결");
        }

        return callback;
    }

    // 0040d900의 DWORD 감김 16배를 float32로 저장하는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static float Cell(int value)
    {
        return (float)unchecked((int)((uint)value << 4));
    }

    // 원문 _ftol64 절삭의 하위 DWORD를 구하는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static int Truncate(double value)
    {
        if (!double.IsFinite(value) || Math.Abs(value) >= 9223372036854775808.0)
        {
            return 0;
        }

        return unchecked((int)(uint)(long)value);
    }

    // 0040e2b0의 중앙 10×10 사각형과 네 _ftol 출력을 계산하는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static FortRandomRectangle Center(FortRandomRectangle area)
    {
        float horizontal = (float)((double)area.Right - area.Left - 10.0);
        double vertical = (double)area.Bottom - area.Top - 10.0;
        double x = horizontal >= 0f ? (double)horizontal : -(double)area.Right - area.Left - 10.0;
        double y = vertical >= 0.0 ? vertical : -(double)area.Bottom - area.Top - 10.0;
        return new FortRandomRectangle(
            (float)Truncate((double)area.Left + (x * 0.5)),
            (float)Truncate((double)area.Top + (y * 0.5)),
            (float)Truncate((double)area.Right - (x * 0.5)),
            (float)Truncate((double)area.Bottom - (y * 0.5)));
    }

    // 현재 target의 실제 bounds와 원문 16배 DWORD 감김을 반환하는 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private FortRandomRectangle Bounds(int region)
    {
        if (_islands.Exists(region) == 0u)
        {
            Need(_hooks.Assertion)(IslandPopulationAssertion.BoundsMissing);
        }

        ChunkRect rect = _target.Bounds(region);
        return new FortRandomRectangle(Cell(rect.Left), Cell(rect.Top), Cell(rect.Right), Cell(rect.Bottom));
    }

    // 0040e130의 world record 조회/현재 TYPE 시점과 실제 로컬 난수를 기존 완료 배치에 전달하는 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private void ResidenceIn(FortRandomRectangle area, int region, uint quantity)
    {
        byte[] record = RawFortTerritorySave.FortTerritoryIslandRecord(_world, region);
        NetstormRandom random = new((uint)record[3] * 103u);
        _ = _placement.PlaceRandom(random, area, region, quantity, _globals.Residence, 0u);
    }

    // 004324e0의 y/x 전체 순회와 마지막 생산 통지를 실행하는 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private void WriteRecord(int region, ReadOnlySpan<byte> record)
    {
        if (record.Length < 6)
        {
            throw new ArgumentException("섬 레코드는 최소 6바이트여야 합니다.", nameof(record));
        }

        if (_islands.Exists(region) == 0u)
        {
            Need(_hooks.Assertion)(IslandPopulationAssertion.RecordMissing);
        }

        // 현재 실제 높이를 y 순서로 먼저 읽는 외부 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (int y = 0; y < _target.YLength(); ++y)
        {
            // 현재 실제 폭을 x 순서로 읽고 일치한 6바이트만 쓰는 내부 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            for (int x = 0; x < _target.XLength(); ++x)
            {
                ref ChunkEntry entry = ref _target.At(x, y);
                if (unchecked((sbyte)entry.Island) == region)
                {
                    record[..6].CopyTo(entry.Record.AsSpan(0, 6));
                }
            }
        }

        Need(_hooks.RefreshProduction)();
    }

    // 원문 반환값을 조용히 성공 SID로 바꾸지 않고 실제 WORD/Pool 물리 경계만 확인하는 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private int Candidate(uint value)
    {
        if (value > ushort.MaxValue || value >= (uint)_pool.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "섬 population의 실제 배치 반환 SID 범위");
        }

        return (ushort)value;
    }
}
