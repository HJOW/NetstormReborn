using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;

// 취득·취소·미리보기 정리가 같은 원문 전역과 물리를 재조회한다.
namespace Netstorm.Client.Input;

/// <summary>수명 계산과 구분한 실제 가상/글자/자산/위치/음향 장치다. 선택된 호출에서만 요구한다.</summary>
public sealed class CursorLifecycleHooks
{
    /// <summary>같은 선택 owner의004d5bb0 강조 변경이다.</summary>
    public Action<uint>? Highlight { get; set; }
    /// <summary>실제004a4410 커서 종류 선택이다.</summary>
    public Action<uint>? SelectCursor { get; set; }
    /// <summary>현재4458e0 위치/dirty 준비다.</summary>
    public Action? PrepareCursor { get; set; }
    /// <summary>현재 원문 팝업 정리다.</summary>
    public Action? ClosePopups { get; set; }
    /// <summary>445b30의 실제 미리보기 생성이다.</summary>
    public Action? CreatePreviews { get; set; }
    /// <summary>445440의 현재 위치 판정이다.</summary>
    public Action? QueryCursor { get; set; }
    /// <summary>Form 가상+0의 현재 연관 객체 SID다.</summary>
    public Func<int, int>? Source { get; set; }
    /// <summary>연관 객체의 가상+24(preview,4)다.</summary>
    public Action<int, int, uint>? SourceNotice { get; set; }
    /// <summary>연관 객체의 가상+48(0) 부착 해제다.</summary>
    public Action<int, uint>? Detach { get; set; }
    /// <summary>연관 객체의 가상+4c(0) 월드 해제다.</summary>
    public Action<int, uint>? Unpop { get; set; }
    /// <summary>현재 미리보기의 가상+10(0) 파괴다.</summary>
    public Action<int, uint>? Destroy { get; set; }
    /// <summary>41bf10(process,0)의 실제 프로세스 제거다.</summary>
    public Action<uint, uint>? RemoveProcess { get; set; }
    /// <summary>4741d0(firstPreview,0)의 마나 프로세스 생성이다.</summary>
    public Func<int, uint, uint>? CreateMana { get; set; }
    /// <summary>474900(firstPreview,0)의 사정거리 프로세스 생성이다.</summary>
    public Func<int, uint, uint>? CreateRange { get; set; }
    /// <summary>프로세스 가상+1c의 실제 네 인자다.</summary>
    public Action<uint, uint, uint, uint>? ProcessEvent { get; set; }
    /// <summary>현재 TYPE+118의 기본 인자다.</summary>
    public Func<uint, uint>? DefaultArgument { get; set; }
    /// <summary>49adc0의 현재 TYPE 번호이며 SHP 토큰과 구분한다.</summary>
    public Func<uint, uint>? TypeNumber { get; set; }
    /// <summary>474240의 현재 타입별 사정거리 판정이다.</summary>
    public Func<uint, bool>? HasRange { get; set; }
    /// <summary>다섯 원문 경고의 현재 번역이다.</summary>
    public Func<string, string>? Translate { get; set; }
    /// <summary>4a3df0의 현재8DWORD 문맥/문자열 측정이며 제한값은 원문-1이다.</summary>
    public Func<string, ReadOnlyMemory<uint>, (int Width, int Height)>? Measure { get; set; }
    /// <summary>frameCheck 뒤 실제 SHP 프레임3의 추가 헤더다.</summary>
    public Func<uint, uint, CursorCostImage>? CostImage { get; set; }
    /// <summary>현재 마우스의446b50 꼬리 위치 갱신이다.</summary>
    public Action<int, int>? UpdatePosition { get; set; }
    /// <summary>teleporterActive.wav의 원문 여섯 인자 재생이다.</summary>
    public Action<string>? Sound { get; set; }
    /// <summary>40eec0의 실제 float 비용 차감이다.</summary>
    public Action<float>? AddLocalSp { get; set; }
    /// <summary>현재 원시 mouseX/Y를 꼬리 호출 직전에 읽는다.</summary>
    public Func<(int X, int Y)>? Mouse { get; set; }
    /// <summary>원문 false/파일/행 진단이며 복귀 뒤 현재 TYPE를 다시 소비한다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
    /// <summary>CPP 생성자의 함수 객체 값 보관처럼 builder의 후속 교체와 구분한다.</summary>
    internal CursorLifecycleHooks Copy() => (CursorLifecycleHooks)MemberwiseClone();
}

/// <summary>00447000 취득·00446e70 취소·00445aa0 임시 정리의10.78 본체다.</summary>
public sealed class RawCursorLifecycle
{
    /// <summary>005325cc의 원문 경고이며 번역 뒤 실제 일반 측정기로 전달한다.</summary>
    private static readonly string[] Warnings = ["Priest can't get here.", "Must be placed over constructing artifact", "Cannot place here", "Energy not satisfied", "Only on Controlled Island"];
    /// <summary>Form/부모/임시 객체의 동일 물리 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>callback 뒤 현재 비용/flags를 다시 읽는188TYPE다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>내려놓기와 같은 전체 전역이다.</summary>
    private readonly CursorDropState _drop;
    /// <summary>목록/측정/추상 TYPE의 같은 장기 전역이다.</summary>
    private readonly CursorLifecycleState _state;
    /// <summary>도달한 실제 효과만 요구하는 고정 함수 객체 사본이다.</summary>
    private readonly CursorLifecycleHooks _hooks;
    /// <summary>같은 판본/188TYPE를 검증하고 상태 참조와 함수 값만 보관한다.</summary>
    public RawCursorLifecycle(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, CursorDropState drop, CursorLifecycleState state, CursorLifecycleHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(drop); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count != 188) throw new ArgumentException("커서수명10.78 Pool/TYPE계약오류");
        _pool = pool; _types = types; _drop = drop; _state = state; _hooks = hooks.Copy();
    }
    /// <summary>실제 호출자의 단일 Pool 정체성이다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>미선택 하위는 요구하지 않고 도달한 누락은 그 위치에서 실패한다.</summary>
    private static T Need<T>(T? hook) where T : Delegate => hook ?? throw new InvalidOperationException("실제커서수명경계미연결");
    /// <summary>원문 DWORD를 잃지 않고 호스트 SID16 경계에서만 검사한다.</summary>
    public static int PreviewSid(uint number) => number <= ushort.MaxValue ? (int)number : throw new ArgumentOutOfRangeException(nameof(number), "커서DWORD의호스트SID범위");
    /// <summary>현재 실제 소비의5이상/Pool범위를 확인하며 죽음 상태를 추가 제외하지 않는다.</summary>
    private ReadOnlySpan<byte> Raw(int sid) => sid >= 5 && sid < _pool.Capacity ? _pool.Slot(sid) : throw new ArgumentOutOfRangeException(nameof(sid), "커서수명현재SID범위");
    /// <summary>선택 TYPE와 별개인 현재 abstractType 항목이다.</summary>
    private OriginalTypeRecord Type() => _state.AbstractType < _types.Count ? _types[(int)_state.AbstractType] : throw new ArgumentOutOfRangeException(nameof(_state.AbstractType));
    /// <summary>비정렬 원문 WORD만 읽는다.</summary>
    private static uint Word(ReadOnlySpan<byte> raw, int offset) => BinaryPrimitives.ReadUInt16LittleEndian(raw[offset..]);
    /// <summary>CRT _ftol의 signed64 중간 하위DWORD이며 비유한/범위 밖의 하위DWORD는0이다.</summary>
    public static int TruncateCost(float value) => !float.IsFinite(value) || Math.Abs((double)value) >= 9223372036854775808.0 ? 0 : unchecked((int)(uint)(long)value);
    /// <summary>준비 뒤 현재 flags/개수/목록을 읽고 선택된 경우만 실제 임시 객체를 삭제한다.</summary>
    public void Activate(uint force)
    {
        Need(_hooks.PrepareCursor)();
        if ((_drop.CostFlags & 0x200) != 0 || force != 0)
        {
            // 가상 삭제 뒤에도 현재 signed 개수와 다음 DWORD를 다시 읽는다.
            for (int index = 0; index < _state.PreviewCount; index++)
            {
                if (index >= _state.Previews.Length) throw new ArgumentOutOfRangeException(nameof(index), "커서임시목록원문30칸범위");
                int sid = PreviewSid(_state.Previews[index]); _ = Raw(sid); Need(_hooks.Destroy)(sid, 0);
            }
            _state.PreviewCount = 0; Array.Clear(_state.Previews);
        }
    }
    /// <summary>현재 preview 캡처와 callback 뒤 전역 재조회를 구분하며 잠겨도 실제 꼬리 정리를 실행한다.</summary>
    public void Cancel()
    {
        if (_drop.LockDepth == 0 && _drop.Preview != 0)
        {
            Need(_hooks.Highlight)(0);
            if (_drop.LockDepth == 0) { Activate(0); Need(_hooks.SelectCursor)(1); }
            _drop.LockDepth = unchecked(_drop.LockDepth + 1); int preview = PreviewSid(_drop.Preview); ReadOnlySpan<byte> raw = Raw(preview);
            if ((raw[11] & 1) == 0)
            {
                Span<byte> writable = _pool.AllocatedBytes(preview); writable[20] = unchecked((byte)_drop.InputFlags); writable[21] = unchecked((byte)(_drop.InputFlags >> 8));
                if ((_drop.InputFlags & 0x80) != 0)
                {
                    int source = Need(_hooks.Source)(preview); ReadOnlySpan<byte> sourceRaw = Raw(source);
                    if ((sourceRaw[11] & 4) == 0) Need(_hooks.Detach)(source, 0); Need(_hooks.Unpop)(source, 0);
                }
                if ((_drop.CostFlags & 0x100) != 0) Need(_hooks.Destroy)(preview, 0);
                else
                {
                    int source = Need(_hooks.Source)(preview); _ = Raw(source); Need(_hooks.SourceNotice)(source, PreviewSid(_drop.Preview), 4);
                    if ((_drop.CostFlags & 0x80) != 0)
                    {
                        Need(_hooks.Sound)("teleporterActive.wav");
                        if (_state.ReadChecking() != 0) Need(_hooks.Assertion)("false", ".\\Cursor.cpp", 0x3f1);
                        Need(_hooks.AddLocalSp)(-Type().Cost);
                    }
                }
            }
        }
        Activate(1);
        if (_drop.CursorGump != 0) { Need(_hooks.RemoveProcess)(_drop.CursorGump, 0); _drop.CursorGump = 0; }
        Array.Clear(_state.Previews); _drop.Preview = 0; _drop.LockDepth = 1; _state.PreviewCount = 0;
    }
    /// <summary>두 cdecl 인자로 원문 취득을 수행하며 팝업부터 현재 마우스 꼬리까지 순서를 유지한다.</summary>
    public void Take(int preview, uint flags)
    {
        Cancel(); if (_drop.CursorGump != 0) throw new InvalidOperationException("cursorManaProcess == NULL 원문진단"); Need(_hooks.ClosePopups)();
        _drop.Preview = unchecked((uint)preview); ReadOnlySpan<byte> raw = Raw(preview); _drop.Type = _state.AbstractType = Word(raw, 18); _ = Type();
        _drop.Argument = Word(raw, 12); if (_drop.Argument == 0) _drop.Argument = Need(_hooks.DefaultArgument)(_state.AbstractType);
        _drop.Direction = 0; _state.DirectionArgument = 0; _drop.InputFlags = Word(raw, 20); _pool.AllocatedBytes(preview)[21] |= 1;
        int source = Need(_hooks.Source)(preview); _ = Raw(source); Need(_hooks.SourceNotice)(source, PreviewSid(_drop.Preview), 4);
        _drop.CostFlags = flags; if ((Type().Flags2 & 4) != 0) _drop.CostFlags = flags | 0x200;
        _drop.LockDepth = 0; Need(_hooks.CreatePreviews)(); Need(_hooks.SelectCursor)(_drop.LegalGump != 0 ? 3u : 2u);
        _drop.LegalGump = 0; _state.CurrentGump = 0;
        if ((Type().Flags2 & 0xa00) != 0)
        {
            _drop.CursorGump = Need(_hooks.CreateMana)(PreviewSid(_state.Previews[0]), 0);
            if (_state.CurrentGump != 0 && _drop.CursorGump != 0) Need(_hooks.ProcessEvent)(_drop.CursorGump, 1, 1, 0);
        }
        uint typeNumber = Need(_hooks.TypeNumber)(_state.AbstractType);
        if (Need(_hooks.HasRange)(typeNumber)) _drop.CursorGump = Need(_hooks.CreateRange)(PreviewSid(_state.Previews[0]), 0);
        Need(_hooks.QueryCursor)(); _state.WarningContext[2] = _state.WarningFont; _state.WarningWidth = _state.WarningHeight = 0;
        // 각 문구의 현재 측정 결과를 개별 저장하고 signed 최대값을 갱신한다.
        for (int index = 0; index < Warnings.Length; index++)
        {
            string text = Need(_hooks.Translate)(Warnings[index]); (int width, int height) = Need(_hooks.Measure)(text, _state.WarningContext);
            _state.WarningWidths[index] = width; _state.WarningHeights[index] = height;
            if (_state.WarningWidth <= _state.WarningWidths[index]) _state.WarningWidth = _state.WarningWidths[index];
            if (_state.WarningHeight <= _state.WarningHeights[index]) _state.WarningHeight = _state.WarningHeights[index];
        }
        _state.CostContext[2] = _state.CostFont; _state.CostText = TruncateCost(Type().Cost).ToString(CultureInfo.InvariantCulture);
        (int costWidth, int costHeight) = Need(_hooks.Measure)(_state.CostText, _state.CostContext); _state.CostTextWidth = costWidth; _state.CostHeight = costHeight;
        CursorCostImage image = Need(_hooks.CostImage)(_state.CostType, 3); _state.CostImage = image.Token; _state.CostHotspotX = image.HotspotX; _state.CostHotspotY = image.HotspotY;
        _drop.CursorWidth = unchecked((int)((uint)image.Width + (uint)_state.CostTextWidth));
        if (_state.CostHeight <= image.Height) _state.CostHeight = image.Height; _state.ImageHeight = image.Height;
        (int x, int y) = Need(_hooks.Mouse)(); Need(_hooks.UpdatePosition)(x, y);
    }
    /// <summary>현재 TYPE 번호의 사정거리 종류를 같은 실제 표에서 읽는다.</summary>
    public static bool HasCursorRange(IReadOnlyList<OriginalTypeRecord> types, uint type)
    {
        ArgumentNullException.ThrowIfNull(types); if (type >= types.Count) throw new ArgumentOutOfRangeException(nameof(type)); return RawSquidSelection.UsesRange(types[(int)type]);
    }
}
