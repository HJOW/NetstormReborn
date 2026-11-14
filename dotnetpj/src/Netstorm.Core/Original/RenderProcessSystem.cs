// 파생 효과는 같은 표시 등록/토큰과 실제 Host를 빌리고 자기 버퍼/Kernel 수명은 소유한다.
namespace Netstorm.Core.Original;

/// <summary>종류별 목록을 합치지 않고 모든 효과가 공유하는 원문 표시 등록/수집 owner다.</summary>
public sealed class RenderProcessSystem
{
    /// <summary>모든 효과가 읽는 같은 raw 풀이다.</summary>
    public RawSidPool Pool { get; }
    /// <summary>실제 Form/Kernel 소유권의 공통 호스트다.</summary>
    public SquidProcessHost Host { get; }
    /// <summary>현재 좌표 배율 owner다.</summary>
    public ConstructionVisualState State { get; }
    /// <summary>생성/삭제가 공유하는 단일 원문 목록이다.</summary>
    public RawRenderRegistry Registry { get; }
    /// <summary>생성 중인 항목을 포함하는 비소유 조회 수다.</summary>
    public int Size => _bindings.Count;
    /// <summary>원문 빈 칸/40칸 확장이 보존된 현재 토큰 표다.</summary>
    public ReadOnlySpan<uint> Slots => Registry.Slots;
    /// <summary>파생 필드와 함수의 현재 수명을 조회한다.</summary>
    private readonly Dictionary<uint, Binding> _bindings = [];
    /// <summary>비영 토큰의 다음 번호다. 감긴0은 이후 호출에서 거부한다.</summary>
    private uint _nextToken = 1;

    /// <summary>파생이 소유한 같은 필드/함수와 완료 표식이다.</summary>
    private sealed class Binding
    {
        /// <summary>실제 등록 필드를 빌린다.</summary>
        public required RenderProcessFields Fields { get; init; }
        /// <summary>생성 중인 같은 파생을 실제 Host에 부착한다.</summary>
        public required Action<RenderProcessFields, uint, int, uint> Attach { get; init; }
        /// <summary>현재 파생을 출력하는 함수 사본이다.</summary>
        public required Action Draw { get; init; }
        /// <summary>초기화 끝난 뒤에만 수집/출력을 허용한다.</summary>
        public bool Ready { get; set; }
    }

    /// <summary>같은 Host/풀을 검증하고 원문 메모리 장치는 보존하며 실제 부착만 토큰별로 분배한다.</summary>
    public RenderProcessSystem(RawSidPool pool, SquidProcessHost host, ConstructionVisualState state, RenderRegistryHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(host); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, host.Pool())) throw new ArgumentException("공통 표시 Host/풀 불일치");
        Pool = pool; Host = host; State = state;
        RenderRegistryHooks copy = hooks.Copy(); copy.Attach = Attach;
        Registry = new(pool, copy);
    }

    /// <summary>Construct가 등록한 정확한 필드의 실제 pending 파생으로 부착을 넘긴다.</summary>
    private void Attach(RenderProcessFields fields, uint type, int parent, uint flags)
    {
        if (!_bindings.TryGetValue(fields.Token, out Binding? binding) || !ReferenceEquals(binding.Fields, fields))
            throw new InvalidOperationException("공통 표시 부착 토큰 불일치");
        Action<RenderProcessFields, uint, int, uint> attach = binding.Attach; attach(fields, type, parent, flags);
    }

    /// <summary>등록 전 조회를 확보한다. 실제 슬롯 등록/Host 부착은 파생 생성자가 이어서 수행한다.</summary>
    public uint Track(RenderProcessFields fields, Action<RenderProcessFields, uint, int, uint> attach, Action draw)
    {
        ArgumentNullException.ThrowIfNull(fields); ArgumentNullException.ThrowIfNull(attach); ArgumentNullException.ThrowIfNull(draw);
        if (fields.Slot >= 0 || _bindings.ContainsKey(fields.Token)) throw new ArgumentException("공통 표시 조회/출력 경계 오류");
        if (_nextToken == 0) throw new OverflowException("공통 표시 토큰 소진");
        uint token = _nextToken;
        _bindings.Add(token, new() { Fields = fields, Attach = attach, Draw = draw });
        fields.Token = token; _nextToken = unchecked(_nextToken + 1); return token;
    }

    /// <summary>원문 등록 완료 뒤 같은 파생을 수집/출력에 공개한다.</summary>
    public void Ready(uint token)
    {
        if (!_bindings.TryGetValue(token, out Binding? binding) || binding.Fields.Slot < 0) throw new InvalidOperationException("공통 표시 초기화 완료 토큰 오류");
        binding.Ready = true;
    }

    /// <summary>파생이 버퍼/등록을 정리한 뒤 조회만 제거한다. 없는 토큰은 무시한다.</summary>
    public void Forget(uint token) => _bindings.Remove(token);
    /// <summary>현재 살아 있고 완료된 필드만 빌린다. 삭제 뒤 이 참조를 계속 사용하지 않는다.</summary>
    public RenderProcessFields? Lookup(uint token) => _bindings.TryGetValue(token, out Binding? binding) && binding.Ready ? binding.Fields : null;
    /// <summary>출력 중 확장이 일어나도 매 차례 현재 슬롯을 조회한다.</summary>
    public void Draw()
    {
        // 원문 배열 한 개의 순서를 유지하며 확장/구멍을 다시 읽는다.
        for (int slot = 0; slot < Registry.Slots.Length; slot++) DrawToken(Registry.Slots[slot]);
    }
    /// <summary>함수 사본이 자기 삭제 뒤에도 호출 수명을 유지한다.</summary>
    public void DrawToken(uint token)
    {
        if (!_bindings.TryGetValue(token, out Binding? binding) || !binding.Ready) return;
        Action draw = binding.Draw; draw();
    }
    /// <summary>같은 목록의 완료된 항목을 원문 수집기에 직접 전달한다.</summary>
    public int Collect(RenderWorldRect view, Span<RenderCollectionEntry> output, int limit) => new RawRenderCollection(Pool, State, Registry, Lookup).Collect(view, output, limit);

    /// <summary>원문 CRT의 정상 signed 정수 범위만 0방향으로 절삭한다.</summary>
    private static int Ftol(double value)
    {
        if (!double.IsFinite(value) || value < -2147483648.0 || value >= 2147483648.0) throw new ArgumentOutOfRangeException(nameof(value), "표시 삭제 영역 정수화 범위 오류");
        return (int)value;
    }
    /// <summary>삭제 통지의 선행 정수화와 top 이외의 중간 float 저장을 원문대로 보존한다.</summary>
    public static int[] RemovalBounds(RenderProcessFields fields, float cameraX, float cameraY)
    {
        int left = Ftol(fields.Left), top = Ftol(fields.Top), right = Ftol(fields.Right), bottom = Ftol(fields.Bottom);
        float storedLeft = (float)((double)left - cameraX), storedRight = (float)((double)(float)right - cameraX), storedBottom = (float)((double)(float)bottom - cameraY);
        return [Ftol((double)storedLeft - 1), Ftol(((double)top - cameraY) - 1), Ftol((double)storedRight + 1), Ftol((double)storedBottom + 1)];
    }
}
