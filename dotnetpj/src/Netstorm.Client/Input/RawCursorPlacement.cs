using System.Buffers.Binary;
using Netstorm.Client.Display;
using Netstorm.Core.Original;

// 취득/취소/내려놓기가 같은 실제 커서의 허용과 표시 영역을 소비한다.
namespace Netstorm.Client.Input;

/// <summary>원문 string 참조를 직접 갱신하는 위치 설명 장치다.</summary>
public delegate uint CursorDescribe(ref string text, float x, float y, uint type, uint owner);

/// <summary>실제 CanonDecoder가 단정도로 저장한 한 칸이다.</summary>
/// <param name="Frame">현재 물리 프레임이다.</param>
/// <param name="X">현재 단정도 X다.</param>
/// <param name="Y">현재 단정도 Y다.</param>
public readonly record struct CursorCanonCell(int Frame, float X, float Y);

/// <summary>공간/GUI/자산/출력 경계다. 도달한 동작만 요구하며 성공 기본값을 만들지 않는다.</summary>
public sealed class CursorPlacementHooks
{
    /// <summary>4734d0의 현재 설명과 DWORD 허용값이다.</summary>
    public CursorDescribe? Describe { get; set; }
    /// <summary>425c20/425860의 실제 칸 순회이며 false에서 Advance 없이 끝낸다.</summary>
    public Action<CanonTypeQuery, Func<CursorCanonCell, bool>>? Walk { get; set; }
    /// <summary>419850(TYPE,0,frame)의 signed 추가 헤더다.</summary>
    public Func<uint, int, CursorCostImage>? Image { get; set; }
    /// <summary>465ed0의 현재 화면 Gump다.</summary>
    public Func<int, int, uint>? GumpAt { get; set; }
    /// <summary>4651b0(this,event,0,0,0)의 실제 수신이다.</summary>
    public Func<uint, uint, uint, uint, uint, uint>? Accept { get; set; }
    /// <summary>수신 Gump+24의 현재 부모다.</summary>
    public Func<uint, uint>? Parent { get; set; }
    /// <summary>49b510의 현재 타입/인자/반올림 좌표/방향/owner BYTE/1이다.</summary>
    public Func<CanonPlacementQuery, uint>? MayPlace { get; set; }
    /// <summary>Form 가상+0의 현재 연관 객체다.</summary>
    public Func<int, int>? Source { get; set; }
    /// <summary>462c80의 현재 연결 지역 번호다.</summary>
    public Func<int, int>? Region { get; set; }
    /// <summary>마나/범위 프로세스 가상+1c다.</summary>
    public Action<uint, uint, uint, uint>? ProcessEvent { get; set; }
    /// <summary>497220의 첫 출력 점이다.</summary>
    public Func<float, float, ScreenPoint>? Project { get; set; }
    /// <summary>497580의 실제 사각형과 flags2/0이다.</summary>
    public Action<ScreenRect, uint>? Invalidate { get; set; }
    /// <summary>CPP 함수 객체의 값 사본을 보존한다.</summary>
    internal CursorPlacementHooks Copy() => (CursorPlacementHooks)MemberwiseClone();
}

/// <summary>10.78의445440 위치 판정·445340 연관 지역·4458e0 dirty 준비 본체다.</summary>
public sealed class RawCursorPlacement
{
    /// <summary>동일 Form/첫 미리보기의 물리 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>callback 뒤 현재 추상 타입을 조회하는 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>source flags20 분기에서만 요구하는256×256 SID 지도다.</summary>
    private readonly ReadOnlyMemory<ushort> _surfaceMap;
    /// <summary>내려놓기/수명과 같은 좌표와 허용 전역이다.</summary>
    private readonly CursorDropState _drop;
    /// <summary>현재 추상 타입·목록·치수·Gump 전역이다.</summary>
    private readonly CursorLifecycleState _life;
    /// <summary>나머지 허용·설명·범위 전역이다.</summary>
    private readonly CursorPlacementState _state;
    /// <summary>생성 시 고정한 실제 함수 사본이다.</summary>
    private readonly CursorPlacementHooks _hooks;
    /// <summary>10.78만 허용하며 지도와 TYPE 길이는 실제 소비 위치에서 확인한다.</summary>
    public RawCursorPlacement(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, ReadOnlyMemory<ushort> surfaceMap, CursorDropState drop, CursorLifecycleState lifecycle, CursorPlacementState state, CursorPlacementHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(drop); ArgumentNullException.ThrowIfNull(lifecycle); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("커서위치10.78판본계약오류", nameof(pool));
        _pool = pool; _types = types; _surfaceMap = surfaceMap; _drop = drop; _life = lifecycle; _state = state; _hooks = hooks.Copy();
    }
    /// <summary>호출자 연결의 동일 풀 정체성이다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>도달한 실제 외부 경계의 누락만 그 위치에서 실패한다.</summary>
    private static T Need<T>(T? hook) where T : Delegate => hook ?? throw new InvalidOperationException("실제커서위치경계미연결");
    /// <summary>별도 drop.type으로 대체하지 않고 현재 abstract TYPE를 읽는다.</summary>
    private OriginalTypeRecord Type() => _life.AbstractType < _types.Count ? _types[(int)_life.AbstractType] : throw new ArgumentOutOfRangeException(nameof(_life.AbstractType));
    /// <summary>CRT signed64 절삭의 하위 DWORD다. indefinite 결과의 하위0도 유지한다.</summary>
    private static int Truncate(double value) => !double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0 ? 0 : unchecked((int)(uint)(long)value);
    /// <summary>flags20일 때 현재 패턴 중 한 칸이라도 연관 객체와 같은 지역인지 찾는다.</summary>
    public uint SourceAllowed()
    {
        if ((_drop.CostFlags & 0x20) == 0) return 1;
        int source = Need(_hooks.Source)(RawCursorLifecycle.PreviewSid(_drop.Preview)); int region = Need(_hooks.Region)(source); uint allowed = 0;
        Need(_hooks.Walk)(new(_drop.Type, unchecked((int)_drop.Argument), unchecked((int)_drop.Direction), _drop.X, _drop.Y), cell =>
        {
            int x = Truncate((double)cell.X + (double)0.9999f), y = Truncate((double)cell.Y + (double)0.9999f);
            if (x >= 0 && y >= 0 && x < 256 && y < 256)
            {
                if (_surfaceMap.Length != 65536) throw new InvalidOperationException("커서연관지역표면지도누락");
                int sid = _surfaceMap.Span[y * 256 + x];
                if (sid != 0 && Need(_hooks.Region)(sid) == region) { allowed = 1; return false; }
            }
            return true;
        });
        return allowed;
    }
    /// <summary>설명→SHP 범위→현재 Gump/배치→연관 지역→프로세스 전이를 원문 순서로 실행한다.</summary>
    public void Query()
    {
        _drop.Legal = Need(_hooks.Describe)(ref _state.DescriptionReference, _drop.X, _drop.Y, _drop.Type, _drop.LocalPlayer);
        _state.DescriptionAllowed = _drop.Legal;
        int nul = _state.Description.IndexOf('\0'); uint textLength = (uint)(nul < 0 ? _state.Description.Length : nul);
        float left = unchecked((int)(0u - ((uint)Type().FootX << 4))), top = unchecked((int)((uint)Type().FootY * 0xfffffff5u));
        float right = 0, bottom = 0; int index = 0; _state.HotspotX = _state.HotspotY = 0;
        Need(_hooks.Walk)(new(_drop.Type, unchecked((int)_drop.Argument), unchecked((int)_drop.Direction), 0, 0), cell =>
        {
            CursorCostImage image = Need(_hooks.Image)(_life.AbstractType, cell.Frame);
            if (index == 0) { _state.HotspotX = image.HotspotX; _state.HotspotY = image.HotspotY; _state.ImageWidth = image.Width; _state.ImageHeight = image.Height; }
            float px = (float)((double)cell.X * 16.0 - image.HotspotX); float yProduct = cell.Y * 11.0f; double py = (double)yProduct - image.HotspotY;
            if (!(px > left)) left = px; if (!(py > top)) top = (float)py;
            double farX = (double)px + image.Width, farY = py + image.Height;
            if (!(right > farX)) right = (float)farX; if (!(bottom > farY)) bottom = (float)farY;
            index++; return true;
        });
        double finalLeft;
        if ((Type().Flags2 & 4) != 0) { finalLeft = (double)left - 16.0; top -= 11.0f; right += 16.0f; bottom += 11.0f; }
        else
        {
            finalLeft = (double)left - _state.LabelSpacing;
            int extent = unchecked(_state.ImageHeight - _state.HotspotY), textExtent = unchecked((int)((uint)_state.LabelSpacing * textLength));
            double labelTop = (double)bottom - (extent < 0 ? 0 : extent) - textExtent;
            if (!(labelTop > top)) top = (float)labelTop;
        }
        if ((Type().Flags1 & 0x400) != 0 && !(bottom > 30.0f)) bottom = 30.0f;
        _state.Bounds[0] = Truncate(-finalLeft); _state.Bounds[1] = Truncate(-(double)top); _state.Bounds[2] = Truncate((double)right - finalLeft); _state.Bounds[3] = Truncate((double)bottom - top);
        uint oldGump = _life.CurrentGump; _drop.LegalGump = 0; _life.CurrentGump = 0;
        _life.CurrentGump = Need(_hooks.GumpAt)(_drop.ScreenX, _drop.ScreenY);
        if (_life.CurrentGump == 0)
        {
            float x = (float)Math.Floor((double)_drop.X + 0.5), y = (float)Math.Floor((double)_drop.Y + 0.5);
            _state.PlacementAllowed = Need(_hooks.MayPlace)(new(_life.AbstractType, _drop.Argument, x, y, _drop.Direction, _drop.LocalPlayer & 255u, 1));
            _state.SpaceAllowed = _state.Obstruction == 0 ? 1u : 0u;
        }
        else
        {
            _state.PlacementAllowed = 0; _drop.LegalGump = Need(_hooks.Accept)(_life.CurrentGump, _state.AcceptEvent, 0, 0, 0);
            if (_drop.LegalGump != 0)
            {
                _state.PlacementAllowed = _state.DescriptionAllowed = _drop.Legal = 1;
                if (Need(_hooks.Parent)(_drop.LegalGump) == 0) throw new InvalidOperationException("cursorLegalGump->parentSid원문진단");
            }
        }
        _drop.Legal = _drop.Legal != 0 && _state.PlacementAllowed != 0 ? 1u : 0u;
        _state.SourceAllowed = SourceAllowed(); _drop.Legal = _drop.Legal != 0 && _state.SourceAllowed != 0 ? 1u : 0u; _state.PlacementAllowed = _state.PlacementAllowed != 0 && _state.SourceAllowed != 0 ? 1u : 0u;
        if (_drop.CursorGump != 0 && (oldGump == 0) != (_life.CurrentGump == 0)) Need(_hooks.ProcessEvent)(_drop.CursorGump, 1, _life.CurrentGump != 0 ? 1u : 0u, 0);
    }
    /// <summary>빈 커서도 투영/주영역을 갱신하고 경고·비용 영역을 현재 값으로 이어서 계산한다.</summary>
    public void Prepare()
    {
        ScreenPoint point = Need(_hooks.Project)(_drop.X, _drop.Y); int[] b = _state.Bounds;
        ScreenRect rect = new(unchecked(point.X - b[0] - 2), unchecked(point.Y - b[1] - 2), unchecked(b[2] - b[0] + 6 + point.X), unchecked(b[3] - b[1] + 14 + point.Y));
        Need(_hooks.Invalidate)(rect, 2);
        if (((_drop.CostFlags & 0x20) != 0 && _state.SourceAllowed == 0) || _state.DescriptionAllowed == 0 || (_state.PlacementAllowed == 0 && _state.PlacementFlags != 0) || _state.SpaceAllowed == 0)
        {
            int x = unchecked(b[2] / 2 - _life.WarningWidth / 2 + rect.Left), y = unchecked(b[3] / 2 - _life.WarningHeight / 2 + rect.Top);
            Need(_hooks.Invalidate)(new(x, y, unchecked(_life.WarningWidth + 2 + x), unchecked(_life.WarningHeight + 2 + y)), 0);
        }
        if ((_drop.CostFlags & 0x40) != 0)
        {
            ReadOnlySpan<byte> raw = _pool.Slot(RawCursorLifecycle.PreviewSid(_life.Previews[0]));
            int cellX = BinaryPrimitives.ReadInt16LittleEndian(raw[22..]), cellY = BinaryPrimitives.ReadInt16LittleEndian(raw[24..]), foot = unchecked((int)((uint)Type().FootX << 4));
            int x = unchecked(cellX - foot / 2 - _drop.CursorWidth / 2 - _state.CurrentOriginX()), y = unchecked(cellY - _state.CurrentOriginY());
            Need(_hooks.Invalidate)(new(x, y, unchecked(_drop.CursorWidth + 2 + x), unchecked(_life.CostHeight + 2 + y)), 0);
        }
    }
    /// <summary>수명 취득/취소의 판정과 준비를 같은 현재 위치 본체에 묶는다.</summary>
    public void BindLifecycle(CursorLifecycleHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks); hooks.QueryCursor = Query; hooks.PrepareCursor = Prepare;
    }
}
