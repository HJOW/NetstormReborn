using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Client.Display; // 팔레트 인덱스 화면의 장치 독립 출력 영역이다.

/// <summary>VFX 원색·현재 소유자 표·명시 변환 표를 구별하는 출력 모드다.</summary>
public enum SpriteShapeMode
{
    /// <summary>원본 색 번호를 유지한다.</summary>
    Plain,
    /// <summary>직전에 선택한 소유자 색 표를 쓴다.</summary>
    Owner,
    /// <summary>추상 표시 색 표를 쓴다.</summary>
    Abstract,
    /// <summary>건설·죽음 표시 색 표를 쓴다.</summary>
    Dead,
    /// <summary>소스 그림자 색 표를 쓴다.</summary>
    Shadow,
}

/// <summary>실제 타입의 표시 헤더·기본 프레임·상태 막대 속성이다. cpppj/src/client/SpriteDraw.h 대응.</summary>
public sealed class SpriteTypeView
{
    /// <summary>논리 프레임 수와 물리 SHP 헤더. alias는 물리 헤더만 치환한다.</summary>
    public SquidDisplayShape Shape { get; set; } = new();
    /// <summary>가상 프레임 -1일 때 쓰는 TYPE+118 기본 프레임.</summary>
    public uint GumpFrame { get; set; }
    /// <summary>자원 막대 용량(TYPE casttime).</summary>
    public int Capacity { get; set; }
    /// <summary>건설 막대 최대 시간(TYPE praytime).</summary>
    public int ConstructionTime { get; set; }
    /// <summary>상태 막대 지속 시간(TYPE effecttime).</summary>
    public int Duration { get; set; }
}

/// <summary>선택 막대가 읽는 현재 Kernel 프로세스 슬롯. 예약 슬롯 0은 방문하지 않는다.</summary>
public sealed class SpriteStatusProcess
{
    /// <summary>프로세스 타입 번호.</summary>
    public uint Type { get; set; }
    /// <summary>추상/상태 비트(1이 2보다 우선한다).</summary>
    public uint Flags { get; set; }
    /// <summary>프로세스에 등록한 SID 목록. 첫 일치에서 멈춘다.</summary>
    public IReadOnlyList<uint> Members { get; set; } = [];
    /// <summary>현재 시각과 차이를 절삭할 종료 시각.</summary>
    public double EndTime { get; set; }
}

/// <summary>내용물 조회가 제공한 자원 용량 타입과 남은 수량이다.</summary>
/// <param name="CapacityType">용량 속성을 읽을 타입 번호.</param>
/// <param name="Remaining">현재 남은 수량.</param>
public readonly record struct SpriteResourceStatus(uint CapacityType, int Remaining);

/// <summary>Draw가 읽는 현재 전역. 공유 DWORD는 대리자로 빌려 가상 호출의 부작용 뒤에도 다시 읽는다.
/// 근거: cpppj/src/client/SpriteDraw.h. 세션의 전역 소유 연결은 R4·R5에서 수행한다.</summary>
public sealed class SpriteDrawState
{
    /// <summary>카메라의 화면 가로 원점.</summary>
    public int CameraX { get; set; }
    /// <summary>카메라의 화면 세로 원점.</summary>
    public int CameraY { get; set; }
    /// <summary>현재 로컬 플레이어.</summary>
    public uint LocalPlayer { get; set; }
    /// <summary>특수 추상 표시 모드.</summary>
    public uint SpecialMode { get; set; }
    /// <summary>특수 표시 대상 flags2 마스크.</summary>
    public uint FilteredFlags { get; set; }
    /// <summary>소유자를 +12 WORD에서 읽는 타입.</summary>
    public uint WideOwnerType { get; set; }
    /// <summary>특수 모드에서 항상 추상 출력하는 타입.</summary>
    public uint UnfilteredType { get; set; }
    /// <summary>그림자 SHP 포인터 alias 대상 타입.</summary>
    public uint ShadowAliasType { get; set; }
    /// <summary>독립 검사에서 alias 갱신을 막는 입력. 공유 진단 연결 시 읽지 않는다.</summary>
    public bool ShadowAliasPinned { get; set; }
    /// <summary>alias 타입의 최대 HP를 1/4로 표시한다.</summary>
    public bool QuarterHitPoints { get; set; }
    /// <summary>선택선과 막대 출력을 막는다.</summary>
    public bool SelectionDisabled { get; set; }
    /// <summary>독립 입력의 소유자 표식 옵션.</summary>
    public bool MarkerEnabled { get; set; }
    /// <summary>표식을 표시할 현재 상태.</summary>
    public bool MarkerVisible { get; set; }
    /// <summary>표식이 제외되는 표면 타입.</summary>
    public uint MarkerExcludeType { get; set; }
    /// <summary>소유자 표식을 그릴 타입.</summary>
    public uint MarkerType { get; set; }
    /// <summary>상태 그림·지속 시간을 읽을 타입.</summary>
    public uint StatusType { get; set; }
    /// <summary>추상 상태 지속 시간을 읽을 타입.</summary>
    public uint AbstractStatusType { get; set; }
    /// <summary>타이머 조회 프로세스 타입.</summary>
    public uint TimerProcessType { get; set; }
    /// <summary>멤버 목록을 조회할 상태 프로세스 타입.</summary>
    public uint StatusProcessType { get; set; }
    /// <summary>+12 WORD를 비용 막대로 표시하는 타입.</summary>
    public uint CostType { get; set; }
    /// <summary>독립 조회하는 3종 타이머 타입. 같은 값이 반복되어도 각각 조회한다.</summary>
    public uint[] TimerTypes { get; set; } = new uint[3];
    /// <summary>타입별 소유자 변환표 존재 여부의 독립 입력.</summary>
    public byte[] OwnerMaps { get; set; } = [];
    /// <summary>현재 공유557940 표. 비어 있으면 독립 OwnerMaps를 쓴다.</summary>
    public uint[] SharedOwnerMaps { get; set; } = [];
    /// <summary>현재5319c0 표식 옵션. 출력 순간 호출한다.</summary>
    public Func<uint>? SharedUseBanners { get; set; }
    /// <summary>현재005318ec 진단. 정확히 0일 때 alias를 갱신한다.</summary>
    public Func<uint>? SharedPatchDiagnostic { get; set; }
    /// <summary>소유자 번호별 팔레트 번호.</summary>
    public int[] OwnerPalettes { get; set; } = [];
    /// <summary>팔레트 번호별 선택선 색.</summary>
    public uint[] SelectionColors { get; set; } = [];
    /// <summary>정상/절반/1/4 이하 체력 색.</summary>
    public uint[] HitPointColors { get; set; } = new uint[3];
    /// <summary>상태 막대 배경색.</summary>
    public uint BarBackground { get; set; }
    /// <summary>마지막으로 선택선을 출력한 SID. Draw가 기록한다.</summary>
    public uint SelectedSid { get; set; }
    /// <summary>alias에 연결한 현재 실제 그림 타입. Draw가 기록한다.</summary>
    public uint? ShadowShapeType { get; set; }
    /// <summary>막대가 읽는 현재 초 단위 시각.</summary>
    public double Now { get; set; }
    /// <summary>독립 입력의 Kernel 순서 상태 프로세스 슬롯.</summary>
    public IReadOnlyList<SpriteStatusProcess> Processes { get; set; } = [];
}

/// <summary>색 문맥 설정과 별도로 전달하는 단일 그림 요청.</summary>
/// <param name="Type">그림 타입 번호.</param>
/// <param name="Frame">물리 프레임 번호.</param>
/// <param name="Position">화면 기준점.</param>
/// <param name="Mode">사용할 색 변환.</param>
public readonly record struct SpriteShapeRequest(uint Type, uint Frame, ScreenPoint Position, SpriteShapeMode Mode);

/// <summary>실행한 분기의 출력·가상 처리·게임 조회 경계. 필수 경계가 빠지면 예외로 알린다.</summary>
public sealed class SpriteDrawHooks
{
    /// <summary>타입/소유자의 현재 색 문맥을 설정한다.</summary>
    public Action<uint, uint>? SetOwnerColors { get; set; }
    /// <summary>한 SHP 프레임을 그린다.</summary>
    public Action<SpriteShapeRequest>? Shape { get; set; }
    /// <summary>양 끝 포함 직교 선택선을 그린다.</summary>
    public Action<ScreenPoint, ScreenPoint, uint>? Line { get; set; }
    /// <summary>끝 제외 사각형을 채운다.</summary>
    public Action<ScreenRect, uint>? Fill { get; set; }
    /// <summary>파생 가상 프레임을 조회한다. -1이면 기본 프레임 분기다.</summary>
    public Func<int, int>? VirtualFrame { get; set; }
    /// <summary>파생 가상 Draw를 현재 기준점에서 호출한다.</summary>
    public Action<int, ScreenPoint>? VirtualDraw { get; set; }
    /// <summary>월드 좌표의 표면 SID를 읽는다.</summary>
    public Func<float, float, int>? Surface { get; set; }
    /// <summary>해당 SID의 건설 표시가 활성인지 조회한다.</summary>
    public Func<int, bool>? ConstructionActive { get; set; }
    /// <summary>현재 자원 막대 입력을 조회한다.</summary>
    public Func<int, SpriteResourceStatus?>? Resource { get; set; }
    /// <summary>SID/프로세스 타입으로 3종 종료 시각을 조회한다.</summary>
    public Func<int, uint, double[]?>? Timers { get; set; }
    /// <summary>가상 Draw 뒤의 현재 Kernel 프로세스 슬롯을 읽는다.</summary>
    public Func<IReadOnlyList<SpriteStatusProcess>>? StatusProcesses { get; set; }
}

/// <summary>일반 객체의 본체·그림자·표식·선택/상태 막대를 원본 순서대로 출력한다.
/// 근거: cpppj/src/client/SpriteDraw.cpp, 10.78 00498220·CD 00455f60.
/// 현재 RawSidPool의 저장 간격은 10.78의 50바이트다. CD 분기도 같은 저장소의 필드를 읽으며 CD 풀 자체의 이식은 별도 범위다.</summary>
public sealed class SpriteDraw
{
    /// <summary>현재 raw 슬롯 저장소. 가상 호출 뒤 같은 메모리를 다시 읽는다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>전체 타입 번호 순서의 TYPE 속성 표.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>같은 순서의 표시 자산 헤더와 막대 속성 표.</summary>
    private readonly IReadOnlyList<SpriteTypeView> _views;
    /// <summary>현재 읽기/쓰기 전역.</summary>
    private readonly SpriteDrawState _state;
    /// <summary>그림·조회 경계. 필요한 분기에서만 요구한다.</summary>
    private readonly SpriteDrawHooks _hooks;
    /// <summary>슬롯 0을 나타내는 수집기와 같은 DWORD 토큰 기준.</summary>
    private readonly uint _tokenBase;

    /// <summary>같은 수명의 풀·타입·그림·전역과 수집기 토큰 기준을 빌린다. 빈 표나 DWORD를 넘는 토큰 범위는 거부한다.</summary>
    public SpriteDraw(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<SpriteTypeView> views,
        SpriteDrawState state, SpriteDrawHooks hooks, uint tokenBase)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(views);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (types.Count == 0 || types.Count != views.Count || (ulong)tokenBase + (ulong)pool.Capacity * RawSidPool.Stride > 0x100000000UL)
            throw new ArgumentException("Sprite Draw 표/토큰 범위 오류");
        _pool = pool; _types = types; _views = views; _state = state; _hooks = hooks; _tokenBase = tokenBase;
    }

    /// <summary>실행한 분기의 필수 경계가 빠지면 명시 오류를 낸다.</summary>
    private static T Required<T>(T? hook) where T : Delegate => hook ?? throw new ArgumentException("Sprite Draw 필수 경계 누락");

    /// <summary>원본 CRT signed64 절삭의 하위 DWORD를 반환한다. 유한 범위 밖은 거부한다.</summary>
    internal static int Truncate(double value)
    {
        if (!double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0)
            throw new ArgumentOutOfRangeException(nameof(value), "Sprite Draw 좌표/시각 범위 오류");
        return unchecked((int)(long)value);
    }

    /// <summary>alias의 물리 SHP만 현재 타입에서 읽고 논리 프레임 수는 원래 타입에 남긴다.</summary>
    private SquidDisplayShape Shape(uint type)
    {
        if (type == _state.ShadowAliasType && _state.ShadowShapeType is uint alias) type = alias;
        if (type >= _views.Count) throw new ArgumentOutOfRangeException(nameof(type), "Sprite Draw SHP alias 오류");
        return _views[(int)type].Shape;
    }

    /// <summary>해당 항목의 일반 Draw를 호출한다. 화면 WORD와 월드 투영이 다르면 원본 assert에 해당하는 오류다.</summary>
    public void Draw(RenderCollectionEntry entry)
    {
        if (entry.Token < _tokenBase) throw new ArgumentOutOfRangeException(nameof(entry), "Sprite Draw 토큰 오류");
        uint offset = entry.Token - _tokenBase;
        if (offset == 0 || offset % RawSidPool.Stride != 0 || offset / RawSidPool.Stride >= _pool.Capacity)
            throw new ArgumentOutOfRangeException(nameof(entry), "Sprite Draw SID 오류");
        int sid = unchecked((ushort)(offset / RawSidPool.Stride));
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        uint type = raw[10];
        if (type >= _types.Count) throw new ArgumentOutOfRangeException(nameof(entry), "Sprite Draw 타입 오류");
        OriginalTypeRecord record = _types[(int)type];
        SpriteTypeView view = _views[(int)type];
        SpriteDrawState s = _state;
        if (!Shape(type).Loaded) return;
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        uint frame = patch ? BinaryPrimitives.ReadUInt32LittleEndian(raw[36..]) : raw[34];
        int extraOffset = patch ? 40 : 35;
        byte extra = raw[extraOffset];
        int x = unchecked(BinaryPrimitives.ReadInt16LittleEndian(raw[22..]) - s.CameraX);
        int y = unchecked(BinaryPrimitives.ReadInt16LittleEndian(raw[24..]) - s.CameraY);
        float wx = BinaryPrimitives.ReadSingleLittleEndian(raw[14..]);
        float wy = BinaryPrimitives.ReadSingleLittleEndian(raw[18..]);
        if (unchecked(Truncate((double)wx * 16 + 0.5) - s.CameraX) != x || unchecked(Truncate((double)wy * 11 + 0.5) - s.CameraY) != y)
            throw new ArgumentException("Sprite Draw 월드/화면 좌표 불일치");
        ScreenPoint point = new(unchecked(x - record.HotspotX), unchecked(y - record.HotspotY));
        if ((entry.Tail & 4) != 0)
        {
            if ((extra & 0x10) != 0) return;
            if (patch && (s.SharedPatchDiagnostic is not null ? s.SharedPatchDiagnostic() == 0 : !s.ShadowAliasPinned)) s.ShadowShapeType = type;
            uint index = frame;
            if (view.Shape.FrameCount <= unchecked((int)index)) index = unchecked(index - (uint)view.Shape.FrameCount);
            if (s.SpecialMode != 0 || (extra & 1) != 0) return;
            Required(_hooks.Shape)(new(type, unchecked(index + (uint)view.Shape.FrameCount), point, SpriteShapeMode.Shadow));
            return;
        }
        uint owner = type == s.WideOwnerType ? BinaryPrimitives.ReadUInt16LittleEndian(raw[12..]) : raw[patch ? 34 : 32];
        bool abstractShape = (extra & 0x10) != 0;
        bool dead = (extra & 1) != 0;
        uint flags2 = record.Flags2;
        // 가상 호출에서 바뀐 현재 전역을 호출 순간 다시 조회한다.
        bool Filtered() => (s.FilteredFlags & flags2) != 0 || type == s.WideOwnerType || type == s.UnfilteredType;
        // 양수 소유자와 존재하는 표에서만 색 문맥을 먼저 바꾼다.
        void Normal(uint index)
        {
            bool shared = s.SharedOwnerMaps.Length != 0;
            if (type >= (shared ? s.SharedOwnerMaps.Length : s.OwnerMaps.Length)) throw new ArgumentOutOfRangeException(nameof(type), "Sprite Draw 소유자 표 오류");
            uint map = shared ? s.SharedOwnerMaps[type] : s.OwnerMaps[type];
            if (map != 0 && owner != 0)
            {
                Required(_hooks.SetOwnerColors)(type, owner);
                Required(_hooks.Shape)(new(type, index, point, SpriteShapeMode.Owner));
            }
            else Required(_hooks.Shape)(new(type, index, point, SpriteShapeMode.Plain));
        }
        if ((extra & 2) == 0)
        {
            if (dead) Required(_hooks.Shape)(new(type, frame, point, SpriteShapeMode.Dead));
            else if (abstractShape)
            {
                if (owner == s.LocalPlayer) Required(_hooks.Shape)(new(type, frame, point, SpriteShapeMode.Abstract));
            }
            else if (s.SpecialMode != 0 && Filtered()) Required(_hooks.Shape)(new(type, frame, point, SpriteShapeMode.Abstract));
            else Normal(frame);
        }
        else
        {
            int custom = Required(_hooks.VirtualFrame)(sid);
            uint fallback = custom == -1 ? view.GumpFrame : unchecked((uint)custom);
            if (dead) Required(_hooks.Shape)(new(type, fallback, point, SpriteShapeMode.Dead));
            else if (s.SpecialMode == 0)
            {
                if (custom != -1)
                {
                    if (!abstractShape) Normal(unchecked((uint)custom));
                    else if (owner == s.LocalPlayer) Required(_hooks.Shape)(new(type, unchecked((uint)custom), point, SpriteShapeMode.Abstract));
                }
                Required(_hooks.VirtualDraw)(sid, point);
            }
            else if (abstractShape)
            {
                if (owner == s.LocalPlayer) Required(_hooks.Shape)(new(type, unchecked((uint)custom), point, SpriteShapeMode.Abstract));
            }
            else if (Filtered()) Required(_hooks.Shape)(new(type, fallback, point, SpriteShapeMode.Abstract));
            else
            {
                if (custom != -1) Normal(unchecked((uint)custom));
                Required(_hooks.VirtualDraw)(sid, point);
            }
        }
        if (!abstractShape)
        {
            if ((s.SharedUseBanners is not null ? s.SharedUseBanners() != 0 : s.MarkerEnabled) && (record.Flags1 & 0x400) != 0 && s.MarkerVisible && owner != 0)
            {
                int surface = Required(_hooks.Surface)(entry.X, entry.Y);
                if (surface != 0 && _pool.Slot(surface)[10] != s.MarkerExcludeType)
                {
                    if (owner >= s.OwnerPalettes.Length) throw new ArgumentOutOfRangeException(nameof(owner), "Sprite Draw 소유자 팔레트 오류");
                    Required(_hooks.Shape)(new(s.MarkerType, unchecked((uint)(s.OwnerPalettes[owner] - 1)), point, SpriteShapeMode.Plain));
                }
            }
            if ((raw[extraOffset] & 0x20) != 0)
            {
                if (s.StatusType >= _views.Count) throw new ArgumentOutOfRangeException(nameof(type), "Sprite Draw 상태 타입 오류");
                Required(_hooks.Shape)(new(s.StatusType, _views[(int)s.StatusType].GumpFrame,
                    new(unchecked(point.X - record.FootX / 2), unchecked(point.Y - record.FootY / 2)), SpriteShapeMode.Plain));
            }
        }
        if ((raw[extraOffset] & 4) != 0 && !s.SelectionDisabled) Selection(sid, type, owner, frame, new(x, y));
    }

    /// <summary>현재 선택 헤더와 raw HP로 바깥/안쪽 선택선·판본별 체력·각 상태 막대를 순서대로 출력한다.</summary>
    private void Selection(int sid, uint type, uint owner, uint frame, ScreenPoint screen)
    {
        SpriteDrawState s = _state;
        SpriteTypeView view = _views[(int)type];
        OriginalTypeRecord record = _types[(int)type];
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        SquidDisplayShape physical = Shape(type);
        if (frame >= physical.Frames.Count || (patch && (unchecked((int)frame) < 0 ||
            (long)frame >= (long)view.Shape.FrameCount * ((record.Flags1 & 0x440000) != 0 ? 2 : 1) || view.Shape.FrameCount <= 0 ||
            record.MaxHitPoints == unchecked((int)0xdddddddd) || record.MaxHitPoints == unchecked((int)0xcdcdcdcd))))
            throw new ArgumentOutOfRangeException(nameof(frame), "Sprite Draw 선택 프레임 오류");
        SquidDisplayFrame header = physical.Frames[(int)frame];
        s.SelectedSid = (uint)sid;
        if (owner >= s.OwnerPalettes.Length || s.OwnerPalettes[owner] < 0 || s.OwnerPalettes[owner] >= s.SelectionColors.Length)
            throw new ArgumentOutOfRangeException(nameof(owner), "Sprite Draw 선택 색 오류");
        uint color = s.SelectionColors[s.OwnerPalettes[owner]];
        int left = unchecked(screen.X - header.HotspotX);
        int right = unchecked(left + header.Width);
        int bottom = unchecked(screen.Y - header.HotspotY + header.Height);
        int horizontal = header.Width / 4;
        int vertical = (header.Height - 1) / 4;
        // 바깥 네 선 다음 안쪽 네 선을 원본 순서로 호출한다.
        for (int layer = 0; layer < 2; layer++)
        {
            Required(_hooks.Line)(new(left, unchecked(bottom - vertical)), new(left, bottom), color);
            Required(_hooks.Line)(new(left, bottom), new(unchecked(left + horizontal), bottom), color);
            Required(_hooks.Line)(new(right, unchecked(bottom - vertical)), new(right, bottom), color);
            Required(_hooks.Line)(new(unchecked(right - horizontal), bottom), new(right, bottom), color);
            left = unchecked(left + 1); right = unchecked(right - 1); bottom = unchecked(bottom - 1);
        }
        int row = 0;
        // 패치 막대는 폭 전체·3픽셀 행 간격·half-open fill을 쓴다.
        void Bar(uint barColor, int current, int maximum)
        {
            int divisor = Math.Max(maximum, 1);
            int amount = Math.Max(0, Math.Min(current, maximum));
            int bx = unchecked(screen.X - header.HotspotX);
            int by = unchecked(screen.Y - row * 3 - header.HotspotY);
            Required(_hooks.Fill)(new(unchecked(bx - 1), unchecked(by - 5), unchecked(bx + 1 + header.Width), unchecked(by - 1)), s.BarBackground);
            Required(_hooks.Fill)(new(bx, unchecked(by - 4), unchecked(bx + unchecked(amount * header.Width) / divisor), unchecked(by - 2)), barColor);
            row++;
        }
        bool hp = (record.Flags2 & 0x400000) != 0 ? (raw[patch ? 40 : 35] & 1) != 0 : (record.Flags1 & 0x10) != 0;
        int maximum = s.QuarterHitPoints && type == s.ShadowAliasType ? record.MaxHitPoints / 4 : record.MaxHitPoints;
        int current = patch ? BinaryPrimitives.ReadInt32LittleEndian(raw[26..]) : BinaryPrimitives.ReadInt16LittleEndian(raw[26..]);
        if (hp)
        {
            if (!patch) { maximum = Math.Max(maximum, 1); current = Math.Clamp(current, 0, maximum); }
            uint hpColor = current <= maximum / 4 ? s.HitPointColors[2] : current <= maximum / 2 ? s.HitPointColors[1] : s.HitPointColors[0];
            if (patch) Bar(hpColor, current, maximum);
            else
            {
                int bx = unchecked(header.Width / 2 - header.HotspotX + screen.X - 10);
                int by = unchecked(screen.Y - header.HotspotY);
                Required(_hooks.Fill)(new(unchecked(bx - 1), unchecked(by - 9), unchecked(bx + 20), unchecked(by - 2)), s.BarBackground);
                Required(_hooks.Fill)(new(bx, unchecked(by - 8), unchecked(bx + unchecked(current * 19) / maximum), unchecked(by - 3)), hpColor);
            }
        }
        if (!patch) return;
        if (raw[10] == s.ShadowAliasType)
        {
            uint target = BinaryPrimitives.ReadUInt32LittleEndian(raw[42..]);
            if (target >= _views.Count) throw new ArgumentOutOfRangeException(nameof(type), "Sprite Draw 건설 타입 오류");
            int duration = _views[(int)target].ConstructionTime;
            if (Required(_hooks.ConstructionActive)(sid)) Bar(0x7b, unchecked(duration - BinaryPrimitives.ReadInt32LittleEndian(raw[46..])), duration);
        }
        if (Required(_hooks.Resource)(sid) is SpriteResourceStatus resource)
        {
            if (resource.CapacityType >= _views.Count) throw new ArgumentOutOfRangeException(nameof(type), "Sprite Draw 자원 타입 오류");
            int capacity = unchecked(_views[(int)resource.CapacityType].Capacity * 10);
            Bar(0xbe, unchecked(capacity - resource.Remaining), capacity);
        }
        // 같은 타입이 반복되어도 3개의 독립 타이머 조회/막대를 유지한다.
        for (int i = 0; i < s.TimerTypes.Length; i++)
        {
            if (raw[10] == s.TimerTypes[i] && Required(_hooks.Timers)(sid, s.TimerProcessType) is double[] times)
                Bar(i == 0 ? 0xd9U : i == 1 ? 9U : 0x6bU, Truncate(times[i] - s.Now), i == 0 ? 20 : 60);
        }
        if ((raw[40] & 0x30) != 0)
        {
            int abstractTime = 0;
            int statusTime = 0;
            IReadOnlyList<SpriteStatusProcess> processes = _hooks.StatusProcesses?.Invoke() ?? s.Processes;
            // 예약 슬롯 0은 제외하고 실제 Kernel 순서의 끝까지 방문한다.
            for (int index = 1; index < processes.Count; index++)
            {
                SpriteStatusProcess process = processes[index];
                if (process.Type != s.StatusProcessType) continue;
                // 멤버 첫 일치에서 멈추며 flag 1을 flag 2보다 먼저 판정한다.
                foreach (uint member in process.Members)
                {
                    if (member != sid) continue;
                    if ((process.Flags & 1) != 0) abstractTime = Math.Max(abstractTime, Truncate(process.EndTime - s.Now));
                    else if ((process.Flags & 2) != 0) statusTime = Math.Max(statusTime, Truncate(process.EndTime - s.Now));
                    break;
                }
            }
            if ((raw[40] & 0x10) != 0)
            {
                if (s.AbstractStatusType >= _views.Count) throw new ArgumentOutOfRangeException(nameof(type), "Sprite Draw 추상 상태 타입 오류");
                Bar(0xc3, abstractTime, _views[(int)s.AbstractStatusType].Duration);
            }
            if ((raw[40] & 0x20) != 0)
            {
                if (s.StatusType >= _views.Count) throw new ArgumentOutOfRangeException(nameof(type), "Sprite Draw 상태 시간 타입 오류");
                Bar(0xfe, statusTime, _views[(int)s.StatusType].Duration);
            }
        }
        if (raw[10] == s.CostType) Bar(0xf0, BinaryPrimitives.ReadUInt16LittleEndian(raw[12..]), Truncate(record.Cost));
    }

    /// <summary>표식의 +0.9999·음수 절삭·0단계 해시 조회를 연결한다. 선택 단계는 바꾸지 않는다.</summary>
    public static Func<float, float, int> MakeSpriteSurfaceLookup(RawSquidHash hash)
    {
        ArgumentNullException.ThrowIfNull(hash);
        return (x, y) =>
        {
            int column = Truncate((double)x + (double)0.9999F);
            int row = Truncate((double)y + (double)0.9999F);
            return column < 0 || row < 0 || column >= 256 || row >= 256 ? 0 : hash.Cell(0, column, row);
        };
    }
}
