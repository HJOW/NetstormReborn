using System.Buffers.Binary;

// AI 생산 소비와 건설 요청을 원본 순서로 처리하는 네임스페이스다.
namespace Netstorm.Core.Original;

/// <summary>서버 건설 확정에 전달하는 원본 10인자다. 직렬화할 때 각 필드의 원본 폭을 사용한다.</summary>
/// <param name="Type">배치할 TYPE 번호다.</param>
/// <param name="X">첫 칸 X 좌표다.</param>
/// <param name="Y">첫 칸 Y 좌표다.</param>
/// <param name="Player">소유 플레이어다.</param>
/// <param name="Argument">패턴 또는 프레임 인자다.</param>
/// <param name="Direction">방향 값이다.</param>
/// <param name="Flags">기준점 검사 등의 제어 비트다.</param>
/// <param name="TimeLow">도착 시각의 하위 DWORD다.</param>
/// <param name="TimeHigh">도착 시각의 상위 DWORD다.</param>
/// <param name="Quality">다리 품질이다.</param>
public readonly record struct ConstructionConfirmRequest(uint Type, float X, float Y, uint Player,
    uint Argument, uint Direction, uint Flags, uint TimeLow, uint TimeHigh, uint Quality);

/// <summary>로컬 예측 배치의 원본 다섯 인자다.</summary>
/// <param name="Preview">같은 풀의 미리보기 SID다.</param>
/// <param name="X">X 좌표다.</param>
/// <param name="Y">Y 좌표다.</param>
/// <param name="Argument">패턴 인자다.</param>
/// <param name="Direction">방향 값이다.</param>
public readonly record struct ConstructionLocalRequest(int Preview, float X, float Y, uint Argument, uint Direction);

/// <summary>배치가 매 분기에서 다시 읽고 쓰는 실제 전역이다.</summary>
public sealed class AiDeploymentGlobals
{
    /// <summary>현재 서버 권한이다.</summary>
    public Func<bool>? Authority { get; set; }
    /// <summary>건설 사제 이동 플래그를 실제 저장소에 쓴다.</summary>
    public Action<bool>? MoveLocalBuilder { get; set; }
    /// <summary>현재 QWORD 게임 시각이다.</summary>
    public Func<double>? Now { get; set; }
}

/// <summary>기존 생산·타이머·건설 본체와 장치 효과를 연결한다. 선택된 누락은 명시 오류다.</summary>
public sealed class AiDeploymentHooks
{
    /// <summary>현재 기술 부모에서 TYPE에 맞는 생산 조각을 찾는다.</summary>
    public Func<uint, uint, int>? Product { get; set; }
    /// <summary>현재 소유자의 실제 기술 부모를 조회한다.</summary>
    public Func<uint, uint>? Technology { get; set; }
    /// <summary>같은 월드의 생산 조각을 삭제한다.</summary>
    public Action<int, uint>? Destroy { get; set; }
    /// <summary>Regular 메모리 예약 성공 여부다. 실패해도 배치를 계속한다.</summary>
    public Func<bool>? ReserveRegular { get; set; }
    /// <summary>예약된 Regular를 새 생산 조각에 사건 0·현재 주기로 부착한다.</summary>
    public Action<int, float>? AddRegular { get; set; }
    /// <summary>실제 서버 건설을 요청한다. 원본처럼 반환값은 사용하지 않는다.</summary>
    public Action<ConstructionConfirmRequest>? Request { get; set; }
    /// <summary>실제 로컬 예측·서버 송신 본체다.</summary>
    public Action<ConstructionLocalRequest>? LocalPlace { get; set; }
    /// <summary>현재 데모 여부다.</summary>
    public Func<bool>? Demo { get; set; }
    /// <summary>반환 뒤 계속 진행하는 원본 진단이다.</summary>
    public Action<string>? Assertion { get; set; }
}

/// <summary>004106e0 생산 교체·대기·비용·배치와 00410280/004102b0 비용 처리를 실행한다.</summary>
/// <remarks>근거: cpppj/src/o/RawAiDeployment.cpp·h. 2026-11-20 0.661에서 수집자 배치 선행 본체로 이식했다.</remarks>
public sealed class RawAiDeployment
{
    /// <summary>데모 지갑의 signed 하한이다.</summary>
    private const uint DemoMinimum = 1190;
    /// <summary>시설 재판단 기한에 더하는 초다.</summary>
    private const double DeploymentDelay = 20.0;
    /// <summary>기존 AI와 같은 본체 저장소다.</summary>
    private readonly RawAiBrainMemory _memory;
    /// <summary>생산·내용물·건설이 공유하는 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>현재 비용과 genus를 읽는 TYPE 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>원본 내용물 생성 본체다.</summary>
    private readonly RawContainedCreate _create;
    /// <summary>현재 권한·이동·시각 장치다.</summary>
    private readonly AiDeploymentGlobals _globals;
    /// <summary>선택된 실제 하위 효과다.</summary>
    private readonly AiDeploymentHooks _hooks;

    /// <summary>게임 계산 없이 같은 PATCH 풀·내용물·TYPE 계약을 확인한다.</summary>
    public RawAiDeployment(RawAiBrainMemory memory, RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types,
        RawContainedCreate create, AiDeploymentGlobals globals, AiDeploymentHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(create);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || !ReferenceEquals(pool, create.Pool) || types.Count == 0)
        {
            throw new ArgumentException("AI 배치 같은 PATCH Pool/TYPE/내용물 생성 계약 누락");
        }
        _memory = memory;
        _pool = pool;
        _types = types;
        _create = create;
        _globals = globals;
        _hooks = hooks;
    }

    /// <summary>비용을 원본 signed64로 절삭한 하위 DWORD와 현재 지갑을 비교한다.</summary>
    public bool CanAfford(uint token, uint type) => Signed(Truncate(Type(type).Cost)) <= Signed(Field(token, 0x2d8));

    /// <summary>먼저 음수로 만든 비용을 절삭해 감김 가산한다. 실제 SP는 경제 정리에서 쓴다.</summary>
    public void Charge(uint token, uint type)
    {
        uint delta = Truncate(-(double)Type(type).Cost);
        Field(token, 0x2d8, unchecked(Field(token, 0x2d8) + delta));
        if (Need(_hooks.Demo)())
        {
            uint wallet = Field(token, 0x2d8);
            Field(token, 0x2d8, Signed(wallet) > (int)DemoMinimum ? wallet : DemoMinimum);
        }
    }

    /// <summary>생산 소비→대기 조각·Regular→비용→권한별 배치→이동 해제→시설 기한 순서다.</summary>
    public void Place(uint token, uint type, float x, float y, uint argument, uint direction)
    {
        int product = Need(_hooks.Product)(token, type);
        bool charge;
        if (product != 0)
        {
            Need(_hooks.Destroy)(product, 0);
            int replacement = _create.Create(Technology(token), type, 0, 0x108, 0x10, 0);
            if (Need(_hooks.ReserveRegular)())
            {
                Need(_hooks.AddRegular)(replacement, BitConverter.UInt32BitsToSingle(Field(token, 0x224)));
            }
            charge = true;
        }
        else
        {
            if ((Type(type).Flags2 & 0x70000u) != 0)
            {
                Need(_hooks.Assertion)("!( ::getType(type)->genus & (gEMPLACEMENT|gCARRIER) )");
            }
            charge = (Type(type).Flags2 & 0x4200u) != 0;
        }
        if (charge)
        {
            Charge(token, type);
        }
        bool authority = Need(_globals.Authority)();
        Need(_globals.MoveLocalBuilder)(true);
        if (authority)
        {
            Need(_hooks.Request)(new(type, x, y, Field(token, 0x260), argument, direction, 0, 0, 0, 0));
        }
        else
        {
            int preview = _create.Create(Technology(token), type, 0, 0x108, 0x10, 0);
            Need(_hooks.LocalPlace)(new(preview, x, y, argument, direction));
        }
        Need(_globals.MoveLocalBuilder)(false);
        if ((Type(type).Flags2 & 0x800u) != 0)
        {
            Deadline(token, 0x5b8);
        }
        if ((Type(type).Flags2 & 0x4200u) != 0)
        {
            Deadline(token, 0x5c0);
        }
    }

    /// <summary>월드 조립 시 같은 풀 연결을 확인한다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>기술 부모를 미리 WORD로 자르지 않고 실제 물리 범위를 확인한다.</summary>
    private int Technology(uint token)
    {
        uint parent = Need(_hooks.Technology)(Field(token, 0x260));
        if (parent > ushort.MaxValue || parent >= (uint)_pool.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(token), "AI 배치 현재 기술부모 SID 범위");
        }
        return (int)parent;
    }

    /// <summary>실제 배치 하위가 반환한 뒤의 현재 시각으로 기한 두 DWORD를 쓴다.</summary>
    private void Deadline(uint token, int offset)
    {
        ulong bits = BitConverter.DoubleToUInt64Bits(Need(_globals.Now)() + DeploymentDelay);
        Field(token, offset, unchecked((uint)bits));
        Field(token, offset + 4, unchecked((uint)(bits >> 32)));
    }

    /// <summary>현재 살아 있는 0xb08 본체만 허용한다.</summary>
    private byte[] Body(uint token)
    {
        byte[] bytes = _memory.Bytes(token);
        return bytes.Length == RawAiBrainConstructor.BytesCount ? bytes
            : throw new ArgumentException("AI 배치 실제0xb08 본체 누락", nameof(token));
    }
    /// <summary>외부 효과 뒤에도 현재 본체의 DWORD를 다시 읽는다.</summary>
    private uint Field(uint token, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(Body(token).AsSpan(offset, 4));
    /// <summary>현재 본체의 선택한 DWORD만 쓴다.</summary>
    private void Field(uint token, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(Body(token).AsSpan(offset, 4), value);
    /// <summary>TYPE 0을 허용하고 표 밖 접근은 거부한다.</summary>
    private OriginalTypeRecord Type(uint type) => type < (uint)_types.Count ? _types[(int)type]
        : throw new ArgumentOutOfRangeException(nameof(type), "AI 배치 현재 TYPE 범위");
    /// <summary>signed DWORD의 비트 표현을 보존한다.</summary>
    private static int Signed(uint value) => unchecked((int)value);
    /// <summary>CRT ftol의 signed64 절삭과 무효 FISTP의 하위 DWORD 0을 보존한다.</summary>
    private static uint Truncate(double value) => !double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0
        ? 0u : unchecked((uint)(long)Math.Truncate(value));
    /// <summary>선택된 미연결을 빈 성공으로 대신하지 않는다.</summary>
    private static T Need<T>(T? function) where T : Delegate => function ?? throw new InvalidOperationException("AI 배치 실제 하위·전역 미연결");
}
