// 연소 생성자는 같은 원문 표시 목록과150입자/공유 팔레트를 초기화한다.
namespace Netstorm.Core.Original;

/// <summary>원문24바이트 연소 입자의 의미 필드다.</summary>
/// <param name="X">현재 가로 좌표다.</param>
/// <param name="Y">현재 세로 좌표다.</param>
/// <param name="VelocityX">현재 가로 속도다.</param>
/// <param name="VelocityY">현재 세로 속도다.</param>
/// <param name="Life">현재 수명이다.</param>
/// <param name="Source">고정 연소점 포인터의 비영 표식이다.</param>
public record struct BurningParticle(float X, float Y, float VelocityX, float VelocityY, float Life, bool Source);

/// <summary>연소 파생이 단독 소유하는 원문 필드다. timer 두float는 별도 프레임 몸체가 쓴다.</summary>
public sealed class BurningVisualFields
{
    /// <summary>공통 원문 등록 필드다.</summary>
    public RenderProcessFields Registration { get; } = new();
    /// <summary>확보한 입자 수다.</summary>
    public uint Capacity { get; set; }
    /// <summary>현재 사용한 입자 수다.</summary>
    public uint Total { get; set; }
    /// <summary>실제 입자 저장소다.</summary>
    public BurningParticle[] Particles { get; set; } = [];
    /// <summary>공통 시각 helper의 이전 시각이다.</summary>
    public double LastTime { get; set; }
    /// <summary>공통 시각 helper의 누적 경과다.</summary>
    public double Elapsed { get; set; }
    /// <summary>원문 생성자가0으로 두는 팔레트 필드다.</summary>
    public uint Palette { get; set; }
    /// <summary>후속 실제 소리 핸들이다.</summary>
    public uint Sound { get; set; }
    /// <summary>첫 시각/캡처/좌표 단계다.</summary>
    public uint Stage { get; set; }
    /// <summary>현재 프레임의 캡처/기준점이다.</summary>
    public ConstructionVisualImage Image { get; set; } = new();
    /// <summary>부착이 바꾼 Parent와 별도로 생성 진입 피해SID를 유지한다.</summary>
    public int Victim { get; set; }
    /// <summary>모드0/1과 그 밖의 원문DWORD를 그대로 보존한다.</summary>
    public uint Mode { get; set; }
    /// <summary>현재 대각선 연소점 행 표식이다.</summary>
    public byte[] Rows { get; set; } = [];
    /// <summary>선택한 연소점 개수이며 입자 추가 성공 수와 다르다.</summary>
    public int Selected { get; set; }
    /// <summary>후속 visible/시각 갱신 표식이다.</summary>
    public bool Off { get; set; }
}
/// <summary>생성 객체 수명과 별개인 월드 공통 역할/팔레트 owner다.</summary>
public sealed class BurningVisualState
{
    /// <summary>현재 burning 프로세스TYPE다.</summary>
    public uint ProcessType { get; set; } = 14;
    /// <summary>한 번 만든 공유 색 표 토큰이다.</summary>
    public uint Palette { get; set; }
}
/// <summary>선택한 메모리 확보와 첫 색 표의 실제 장치다.</summary>
public sealed class BurningVisualHooks
{
    /// <summary>150*24 또는 색표8바이트를 확보한다.</summary>
    public Func<uint, bool>? Reserve { get; set; }
    /// <summary>실제8바이트 색 표 토큰을 만든다.</summary>
    public Func<uint>? CreatePalette { get; set; }
    /// <summary>시작/끝RGB와색수8/보정2.4로 실제 색 띠를 만든다.</summary>
    public Action<double[], double[], uint, double>? Gradient { get; set; }
    /// <summary>같은 공유 토큰에 만든 색 띠를 넣는다.</summary>
    public Action<uint>? BuildPalette { get; set; }
    /// <summary>함수 값은 복사하고 실제 장치/월드 owner는 빌린다.</summary>
    internal BurningVisualHooks Copy() => new() { Reserve = Reserve, CreatePalette = CreatePalette, Gradient = Gradient, BuildPalette = BuildPalette };
}
/// <summary>원문 Burning 생성자와00423e80/CD004a60a0의 입자 추가다.</summary>
public sealed class RawBurningVisual
{
    /// <summary>원문 고정 입자 확보 수다.</summary>
    private const uint ParticleCount = 150;
    /// <summary>같은 피해/부착/표시 raw 풀이다.</summary>
    public RawSidPool Pool { get; }
    /// <summary>모든 효과가 공유하는 실제 등록기다.</summary>
    private readonly RawRenderRegistry _registry;
    /// <summary>같은 공유 색 표/역할 owner다.</summary>
    private readonly BurningVisualState _state;
    /// <summary>구성 때 복사한 실제 장치 함수다.</summary>
    private readonly BurningVisualHooks _hooks;
    /// <summary>같은 풀/등록과 필수 메모리 장치만 확인하며 cold팔레트 장치는 선택 때 요구한다.</summary>
    public RawBurningVisual(RawSidPool pool, RawRenderRegistry registry, BurningVisualState state, BurningVisualHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(registry); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, registry.Pool) || hooks.Reserve is null) throw new ArgumentException("Burning 표시 등록/메모리 실제월드 오류");
        Pool = pool; _registry = registry; _state = state; _hooks = hooks.Copy();
    }
    /// <summary>등록→의미필드 초기화→150입자 zero→필요한 첫 색 표 순서를 유지한다.</summary>
    public void Initialize(BurningVisualFields fields, uint token, int victim, uint mode)
    {
        _registry.Construct(fields.Registration, token, victim, _state.ProcessType);
        fields.Capacity = fields.Total = 0; fields.Particles = []; fields.LastTime = fields.Elapsed = 0; fields.Palette = 0;
        fields.Victim = victim; fields.Stage = 0; fields.Image = new(); fields.Rows = []; fields.Selected = 0; fields.Off = false; fields.Sound = 0;
        if (!_hooks.Reserve!(ParticleCount * 24)) throw new OutOfMemoryException("Burning 입자 확보 실패");
        fields.Capacity = ParticleCount; fields.Particles = new BurningParticle[ParticleCount]; fields.Total = 0; fields.Mode = mode;
        if (_state.Palette == 0)
        {
            if (_hooks.CreatePalette is null || _hooks.Gradient is null || _hooks.BuildPalette is null) throw new InvalidOperationException("Burning 실제 첫 팔레트 생성 미연결");
            if (!_hooks.Reserve(8)) throw new OutOfMemoryException("Burning 팔레트 확보 실패");
            _state.Palette = _hooks.CreatePalette();
            if (_state.Palette == 0) throw new OutOfMemoryException("Burning 팔레트 생성 실패");
            _hooks.Gradient([1, 115.0 / 255, 7.0 / 255], [1, 245.0 / 255, 117.0 / 255], 8, 2.4); _hooks.BuildPalette(_state.Palette);
        }
    }
    /// <summary>앞 여유칸 또는 첫 만료칸을 쓰며 PATCH float ordered비교/CD signedDWORD비교를 구별한다.</summary>
    public bool Append(BurningVisualFields fields, BurningParticle particle)
    {
        if (fields.Particles.Length != fields.Capacity || fields.Total > fields.Capacity) throw new ArgumentException("Burning 현재 입자 저장소/개수 오류");
        uint index = fields.Total;
        if (index < fields.Capacity) fields.Total++;
        else
        {
            index = 0; bool patch = Pool.Edition == OriginalEdition.Patch1078;
            // PATCH의NaN은 재사용하지 않고 CD는 같은float비트의 signed부호를 검사한다.
            while (index < fields.Total && (patch ? !(fields.Particles[(int)index].Life <= 0) : BitConverter.SingleToInt32Bits(fields.Particles[(int)index].Life) > 0)) index++;
            if (index == fields.Total) return false;
        }
        fields.Particles[(int)index] = particle; return true;
    }
}
