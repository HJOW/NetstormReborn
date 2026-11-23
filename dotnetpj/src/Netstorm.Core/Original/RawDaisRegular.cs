using System.Buffers.Binary;

// Rune304와 Dais305/306/307, 단계 진행/다음 목표 명령/희생 시작·종료의 원문 순서를 복원한다.
namespace Netstorm.Core.Original;

/// <summary>RawDaisRegular.cpp의 Dais 실제 상위 몸체다.</summary>
public sealed class RawDaisRegular
{
    /// <summary>PATCH 실제165 생성자의 최종 가상표다.</summary>
    private const uint PatchDaisVtable = 0x00507950;
    /// <summary>CD 실제165 생성자의 최종 가상표다.</summary>
    private const uint CdDaisVtable = 0x00504090;
    /// <summary>PATCH 실제166 생성자의 최종 가상표다.</summary>
    private const uint PatchRuneVtable = 0x005075d8;
    /// <summary>CD 실제166 생성자의 최종 가상표다.</summary>
    private const uint CdRuneVtable = 0x00504148;
    /// <summary>원본 DAT532624/CD52e2b8[count]의 다음 첫 프레임이다.</summary>
    private static readonly int[] UpgradeFirst = [3, 64, 120, 0];
    /// <summary>최초예약의 실제 float 비트다.</summary>
    private const float InitialDelay = 0.01F;
    /// <summary>altar24의 실제 float 비트다.</summary>
    private const float SacrificeDelay = 0.11999999731779099F;
    /// <summary>원본 double DAT507918/CD504048의 지연이다.</summary>
    private const double ThunderDelay = 2.799999952316284;
    /// <summary>지연 전역소리의 flags다.</summary>
    private const uint SoundFlags = 0xc;
    /// <summary>포획 내용물의 실제 가상삭제flags다.</summary>
    private const uint ConsumedFlags = 0x400048;
    /// <summary>중립0과 플레이어1..8 배열만 접근한다.</summary>
    private const uint PlayerCount = 8;
    /// <summary>유일한 현재 raw 저장소다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>기존 typed finder와 같은 네 해시다.</summary>
    private readonly RawSquidHash _hash;
    /// <summary>현재 사제 genus와 typed 발자국이다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>현재 direction/first와 전역169 희생 구간이다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;
    /// <summary>기존 count/Stage/목표점/사제·룬/의식 몸체다.</summary>
    private readonly RawTempleRitual _ritual;
    /// <summary>기존 같은ownerDais와 공유AltarAt 몸체다.</summary>
    private readonly RawRunePostPop _rune;
    /// <summary>기존 contained 조건/순서 몸체다.</summary>
    private readonly RawCarrierPreDestroy _carrier;
    /// <summary>현재604 조회/실제삭제/PathFromForm의같은Host다.</summary>
    private readonly SquidProcessHost _host;
    /// <summary>기존현재Path form을조회하는같은종속저장소다.</summary>
    private readonly RawContainedLookup _lookup;
    /// <summary>실제검색/flags0 Init/Prepare/서버부착의같은시작몸체다.</summary>
    private readonly RawPathStart _path;
    /// <summary>공유 원본 전역의 참조와 현재 player 필드다.</summary>
    private readonly DaisRegularState _state;
    /// <summary>실제 프레임/삭제/음향/예약/장치/UI 효과다.</summary>
    private readonly DaisRegularHooks _hooks;
    /// <summary>선택한 callable을 복사해 실제 호출 중 연결 교체에도 수명을 유지한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("다이스의 선택된 실제 장치 미연결");
    /// <summary>비정렬 현재 raw 필드를 원본 폭만큼 읽는다. WORD/BYTE의 옆 필드를 읽거나 바꾸지 않는다.</summary>
    private static uint Read(ReadOnlySpan<byte> raw, int offset, int width = 4)
    {
        uint value = 0;
        for (int index = 0; index < width; index++) value |= (uint)raw[offset + index] << (8 * index);
        return value;
    }
    /// <summary>현재 float 좌표/SP를 원본 __ftol의 정상 signed32 범위에서 절삭한다.</summary>
    private static int Integer(float value)
    {
        if (!float.IsFinite(value) || value < -2147483648.0 || value >= 2147483648.0) throw new ArgumentOutOfRangeException(nameof(value), "Dais 정상 절삭 범위");
        return (int)value;
    }
    /// <summary>같은 풀/TYPE 물리를 확인하고 PATCH 효과는 선택한 원문 위치에서 요구한다.</summary>
    public RawDaisRegular(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<RiftTypeFrames> frames,
        RawTempleRitual ritual, RawRunePostPop rune, RawCarrierPreDestroy carrier, SquidProcessHost host,
        RawContainedLookup lookup, RawPathStart path, DaisRegularState state, DaisRegularHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(hash); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(ritual); ArgumentNullException.ThrowIfNull(rune); ArgumentNullException.ThrowIfNull(carrier); ArgumentNullException.ThrowIfNull(host);
        ArgumentNullException.ThrowIfNull(lookup); ArgumentNullException.ThrowIfNull(path); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, ritual.Pool) || !ReferenceEquals(pool, rune.Pool) || !ReferenceEquals(pool, carrier.Pool) || !ReferenceEquals(pool, host.Pool())
            || !ReferenceEquals(pool, lookup.Pool) || !ReferenceEquals(pool, path.Pool)
            || types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171) || types.Count != frames.Count
            || (pool.Edition == OriginalEdition.Cd1072 && (hooks.SetFrame is null || hooks.Destroy is null || hooks.ReserveShared is null || hooks.AddShared is null
            || hooks.PlayAt is null || hooks.ReserveSound is null || hooks.LoadSound is null || hooks.AttachSound is null || hooks.WallSeconds is null
            || hooks.SetSoundStart is null || hooks.StopVoice is null || hooks.ProcessType is null || hooks.PathMetadata is null || hooks.Damageable is null
            || hooks.AddLocalSp is null || hooks.NormalizeForeignSp is null || hooks.GrantForeignType is null || hooks.LocalSpecial is null
            || hooks.LocalType is null || hooks.RefreshOwner is null || hooks.RefreshLocal is null || hooks.PriestExists is null
            || hooks.MultiplayerStatus is null || hooks.Chat is null)))
            throw new ArgumentException("Dais 실제 몸체/풀/필수 효과 연결 누락", nameof(ritual));
        _pool = pool; _hash = hash; _types = types; _frames = frames; _ritual = ritual; _rune = rune; _carrier = carrier;
        _host = host; _lookup = lookup; _path = path; _state = state; _hooks = hooks.Copy();
    }
    /// <summary>선택된 진단만 실제 장치를 요구하며 정상 복귀 뒤 현재 상태 조회를 계속한다.</summary>
    private void Assert(string expression, string file, uint line)
    {
        if (_hooks.Assertion is null) throw new InvalidOperationException("Dais의 선택된 실제 원문 진단 장치 미연결");
        Need(_hooks.Assertion)(expression, file, line);
    }
    /// <summary>실제 Client getter와 독립 입력의 기존 권한 계약을 구별한다.</summary>
    private bool Boss() => _hooks.Boss is not null ? Need(_hooks.Boss)() : _state.Ritual.BossCurrent();
    /// <summary>원본 방향별9×9 행/열을 바꾸지 않는다.</summary>
    private int Alliance(uint row, uint column)
    {
        Player(row); Player(column);
        return _hooks.Alliance is not null ? Need(_hooks.Alliance)(row, column) : _state.Alliances[(int)(row * 9 + column)];
    }
    /// <summary>원문 자기 가상80 지점만 이 경계에 위임한다. 실제 자기 가상80과 독립 입력의 기존 Temple Count를 같은 호출 위치에 연결한다.</summary>
    private uint Count(int dais) => _hooks.Count is not null ? Need(_hooks.Count)(dais) : _ritual.Count(dais);
    /// <summary>진단 뒤에도 원본 현재 사제 종류를 실제 소유자에서 조회한다. 실제 Session의 현재 전역과 독립 입력의 기존 상태를 구별한다.</summary>
    private uint PriestType() => _hooks.PriestType is not null ? Need(_hooks.PriestType)() : _state.Ritual.PriestTypeCurrent();
    /// <summary>원본 프레임의 판본별 폭과 부호를 유지한다. 패치signedDWORD/CD BYTE의 현재 프레임이다.</summary>
    private int CurrentFrame(int sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        return _pool.Edition == OriginalEdition.Patch1078 ? BinaryPrimitives.ReadInt32LittleEndian(raw[36..]) : raw[34];
    }
    /// <summary>실제 현재 좌표를 하위효과뒤 다시 읽는다. 원본 저장 폭의 현재 float 좌표다.</summary>
    private TempleRitualPoint Position(int sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        return new(BinaryPrimitives.ReadSingleLittleEndian(raw[14..]), BinaryPrimitives.ReadSingleLittleEndian(raw[18..]));
    }
    /// <summary>현재 owner BYTE를 효과 뒤 다시 읽는다.</summary>
    private uint Owner(int sid) => _pool.Slot(sid)[_pool.Edition == OriginalEdition.Patch1078 ? 34 : 32];
    /// <summary>side별 첫 프레임은 현재 raw 타입을 사용한다.</summary>
    private int First(int sid, byte letter)
    {
        uint type = _pool.Slot(sid)[10];
        if (type >= (uint)_frames.Count) throw new ArgumentOutOfRangeException(nameof(sid), "다이스 현재 frame TYPE 물리 범위");
        return _frames[(int)type].Run(letter).First;
    }
    /// <summary>PATCH는 실제 Rune/Dais VT만 사용하며 CD는 기존 TYPE/VT 계약을 유지한다. 실제 Rune/Dais 생성자의 가상표만 받아 일반 모형의 base vtable로 대체하지 않는다.</summary>
    public bool Handles(int sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid); bool patch = _pool.Edition == OriginalEdition.Patch1078; uint vtable = Read(raw, 0);
        if (patch) return vtable == PatchRuneVtable || vtable == PatchDaisVtable;
        return (raw[10] == _state.Rune.CurrentRuneType() && vtable == CdRuneVtable) || (raw[10] == _state.Rune.CurrentDaisType() && vtable == CdDaisVtable);
    }
    /// <summary>Dais wrapper의allowDead1과CanBridge의0을 기존 공통 선택몸체에 전달한다.</summary>
    private int Contained(int dais, uint mask, uint kind, bool allowDead = true) => _carrier.FindContained(dais, mask, kind, 0, allowDead);
    /// <summary>포획한 내용물의kind는 현재+18 WORD다.</summary>
    private uint CapturedWord(int dais)
    {
        int captured = Contained(dais, 4, 0);
        return captured != 0 ? Read(_pool.Slot(captured), 18, 2) : 0;
    }
    /// <summary>중립0과 플레이어1..8 저장 범위를 손상 입력 전에 확인한다.</summary>
    private void Player(uint owner)
    {
        if (owner > PlayerCount) throw new ArgumentOutOfRangeException(nameof(owner), "Dais 현재 플레이어 범위");
    }
    /// <summary>altar 이탈이 결과를 버리는 Dais typed lookup도 동일한 일반 finder에서 수행한다. buried 제외/버킷순서는기존일반몸체이며추가free/dead/owner필터를넣지않는다.</summary>
    private int FindDaisAt(int sid)
    {
        TempleRitualPoint point = Position(sid); uint type = _state.Rune.CurrentDaisType();
        if (type >= (uint)_types.Count) throw new ArgumentOutOfRangeException(nameof(sid), "Dais 현재 타입 범위");
        int x = Integer(point.X), y = Integer(point.Y);
        var finder = new RawSquidFinder(_pool, _hash, _types);
        for (int current = finder.Begin(new(x, y, x, y), (_types[(int)type].Flags2 & 6) == 0 ? 1u : 0u); current != 0; current = finder.Next())
            if (_pool.Slot(current)[10] == type) return current;
        return 0;
    }
    /// <summary>실제 Regular 분배의 반환 지연을 원본 float 비트대로 유지한다. Rune304/Dais305·306·307 및 기존 fallback의 전체 분배다.</summary>
    public float Handle(int sid, uint @event, uint count, float payload)
    {
        if (!Handles(sid)) throw new ArgumentException("Dais/Rune Handle 타입/가상표 오류", nameof(sid));
        if (_pool.Edition == OriginalEdition.Patch1078 ? Read(_pool.Slot(sid), 0) == PatchRuneVtable : _pool.Slot(sid)[10] == _state.Rune.CurrentRuneType())
        {
            return RawRitualRegular.RunRuneRegular(_pool, sid, @event, payload, new RuneRegularHooks
            {
                FindDais = rune => _rune.FindOwnedDais((ushort)rune),
                Progress = dais => Progress(dais),
                Destroy = (rune, flags) => Need(_hooks.Destroy)(rune, flags),
            });
        }
        var current = new DaisHandleHooks
        {
            Stage = (dais, frame) => _ritual.Stage(dais, frame),
            Frame = (dais, frame, flags) => Need(_hooks.SetFrame)(dais, frame, flags),
            SacrificeAdvance = dais => SacrificeAdvance(dais),
            Ritual = dais => { int priest = _ritual.FindStrongOwnedPriest(dais); _ritual.Ritual(dais, priest, 1); },
            Damageable = _hooks.Damageable,
        };
        return RawRitualRegular.RunDaisHandle(_pool, sid, @event, count, payload, current);
    }
    /// <summary>현재K..O에서만604한개를실제로지우고현재타입E첫프레임으로복귀한다.</summary>
    public void StopPriestRitual(int priest) => StopPriestRitualIn(_pool, _frames, _host, priest, Need(_hooks.SetFrame));
    /// <summary>현재 방향 조회→첫604 실제 삭제→다시 현재 TYPE의 E 프레임 조회/지정 순서를 보존한다. 장기 삭제도 같은 현재 Host/Frame의604 중지 본문을 공유한다.</summary>
    public static void StopPriestRitualIn(RawSidPool pool, IReadOnlyList<RiftTypeFrames> frames, SquidProcessHost host, int priest, Action<int, int, uint> setFrame)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(frames); ArgumentNullException.ThrowIfNull(host); ArgumentNullException.ThrowIfNull(setFrame);
        ReadOnlySpan<byte> raw = pool.Slot(priest);
        int frame = pool.Edition == OriginalEdition.Patch1078 ? BinaryPrimitives.ReadInt32LittleEndian(raw[36..]) : raw[34];
        int direction = RawPathAnimation.Direction(frames[raw[10]], frame);
        if (direction <= 9 || direction >= 15) return;
        RegularProcess? process = host.FindEvent(priest, 604);
        if (process is not null) host.Kill(process, 0);
        setFrame(priest, frames[pool.Slot(priest)[10]].Run((byte)'E').First, 0);
    }
    /// <summary>new/Shared효과뒤현재단계와좌표를다시읽는다.</summary>
    public void Collapse(int dais)
    {
        if (Need(_hooks.ReserveShared)()) Need(_hooks.AddShared)(dais, 0x305, InitialDelay);
        TempleRitualPoint point = _ritual.StagePoint(dais, _ritual.Stage(dais));
        Need(_hooks.PlayAt)(point.X, point.Y, "altarBurnCollapse.wav", 0);
    }
    /// <summary>실제 코드의extra2/altarM/0또는45/Shared306순서다.</summary>
    public void StartSacrifice(int dais)
    {
        Span<byte> raw = _pool.RawSlot(dais);
        raw[_pool.Edition == OriginalEdition.Patch1078 ? 40 : 35] |= 2;
        int altar = _rune.AltarAt((ushort)dais);
        if (altar != 0 && altar < _pool.Capacity) Need(_hooks.SetFrame)(altar, First(altar, (byte)'M'), 0);
        uint count = Count(dais);
        if (count == 1) Need(_hooks.SetFrame)(dais, 0, 0);
        else if (count == 2) Need(_hooks.SetFrame)(dais, 45, 0);
        if (Need(_hooks.ReserveShared)()) Need(_hooks.AddShared)(dais, 0x306, InitialDelay);
    }
    /// <summary>전역169코드구간을사용하며현재Dais타입의범용한번감김Advance로대신하지않는다.</summary>
    public float SacrificeAdvance(int dais)
    {
        int current = CurrentFrame(dais); uint type = _state.SacrificeType;
        if (type >= (uint)_frames.Count || current < 0 || current >= _frames[(int)type].Codes.Count) throw new ArgumentOutOfRangeException(nameof(dais), "Dais 희생 코드 범위");
        RiftTypeFrames.LetterRun run = _frames[(int)type].Run(_frames[(int)type].Codes[current].Side);
        int next = run.First;
        if (run.First <= current && (long)current < (long)run.First + run.Count)
        {
            if (run.Count <= 0) throw new InvalidOperationException("Dais 희생 구간 길이 오류");
            next = run.First + (current - run.First + 1) % run.Count;
        }
        if (current <= next) { Need(_hooks.SetFrame)(dais, next, 0); return SacrificeDelay; }
        _pool.RawSlot(dais)[_pool.Edition == OriginalEdition.Patch1078 ? 40 : 35] &= 0xfd;
        FinishSacrifice(dais); return 0;
    }
    /// <summary>지연 시각은실제부착성공뒤에만읽으며두음향의new실패는독립이다.</summary>
    private void DelaySound(int dais, string name, double delay)
    {
        if (!Need(_hooks.ReserveSound)()) return;
        uint sound = Need(_hooks.LoadSound)(name, 0, SoundFlags);
        ulong process = Need(_hooks.AttachSound)(dais, sound, 0, SoundFlags);
        if (process != 0) Need(_hooks.SetSoundStart)(process, Need(_hooks.WallSeconds)() + delay);
    }
    /// <summary>최초stage를유지하되하위효과뒤사제/권한/포획조건은현재raw에서다시읽는다.</summary>
    public void Progress(int dais)
    {
        uint stage = _ritual.Stage(dais); int priest = _ritual.FindStrongOwnedPriest(dais);
        if (priest != 0 && priest < _pool.Capacity) StopPriestRitual(priest);
        if (stage < 4)
        {
            if (Boss()) BridgeCommand(dais, 0, 0);
            Collapse(dais); return;
        }
        TempleRitualPoint point = Position(dais);
        Need(_hooks.PlayAt)(point.X, point.Y, "itIsDone2.wav", 0);
        bool upgrade = Contained(dais, 4, _pool.Slot(dais)[10]) != 0;
        DelaySound(dais, upgrade ? "priestSacrifice4.wav" : "priestSacrifice2.wav", upgrade ? 1.5 : 4.0);
        DelaySound(dais, "thunderCrack.wav", ThunderDelay);
        if (upgrade) { StartSacrifice(dais); return; }
        if (CapturedWord(dais) != 0 || _state.Rune.CurrentSuppressDuration())
        {
            int altar = _rune.AltarAt((ushort)dais);
            if (altar != 0 && altar < _pool.Capacity) AltarEscape(altar);
        }
    }
    /// <summary>이탈전에원본의Dais조회도수행하고현재altarL프레임및24예약을연결한다.</summary>
    public void AltarEscape(int altar)
    {
        _ = FindDaisAt(altar);
        Need(_hooks.SetFrame)(altar, First(altar, (byte)'L'), 0);
        if (Need(_hooks.ReserveShared)()) Need(_hooks.AddShared)(altar, 24, SacrificeDelay);
    }
    /// <summary>내용물 첫 후보/next는genus 진단 전에 저장하고 복귀 뒤 현재 TYPE/owner/후보 조건/동맹을 읽는다.</summary>
    public bool CanBridge(int dais, int priest)
    {
        if (_ritual.Stage(dais) == 4 && _ritual.HasAbstractRune(dais)) return false;
        int altar = _rune.AltarAt((ushort)dais);
        if (altar == 0 || altar >= _pool.Capacity) return false;
        uint altarType = _pool.Slot(altar)[10];
        if (altarType >= (uint)_frames.Count) throw new ArgumentOutOfRangeException(nameof(dais), "Dais 자격의 제단 TYPE 범위");
        int direction = RawPathAnimation.Direction(_frames[(int)altarType], CurrentFrame(altar));
        if (direction == 12 || direction == 11) return false;
        var finder = new RawContainedFinder(_pool, _carrier.FinderState);
        int captured = finder.Begin(dais, false);
        uint priestType = _pool.Slot(priest)[10];
        if (priestType >= (uint)_types.Count) throw new ArgumentOutOfRangeException(nameof(priest), "Dais 자격의 사제 TYPE 범위");
        uint genus = _types[(int)priestType].Flags2;
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        if (patch && _state.Debug && (genus & 0x200000) == 0) Assert("priest.getGenus() & gPRIEST", ".\\Dais.cpp", 0x140);
        if ((genus & 0x200000) == 0 || Owner(priest) != Owner(dais)) return false;
        while (captured != 0)
        {
            ReadOnlySpan<byte> raw = _pool.Slot(captured);
            if ((raw[20] & 0x20) != 0 && Read(raw, 18, 2) == PriestType())
            {
                if (!patch) return true;
                uint originalOwner = (uint)(raw[12] & 127);
                return Alliance(Owner(altar), originalOwner) == 0;
            }
            captured = finder.Next();
        }
        return false;
    }
    /// <summary>현재첫Path form을같은mapper/종속/Host에서조회한다.</summary>
    private int MoveAwayDepth(int priest)
    {
        uint type = Need(_hooks.ProcessType)(_pool.Slot(priest)[10]);
        int form = _lookup.Find(priest, type);
        if (form == 0) return 0;
        RawPathProcess? process = _host.FromForm(form) as RawPathProcess;
        if (process is null) throw new InvalidOperationException("Dais 현재 Path form 실제 프로세스 누락");
        uint pid = Read(_pool.Slot(form), 18);
        if (!_host.IsRegistered(pid, process)) throw new InvalidOperationException("Dais 이동 깊이 진단 중 실제 경로 수명 종료");
        int current()
        {
            if (!_host.IsRegistered(pid, process)) throw new InvalidOperationException("Dais 이동 깊이 진단 중 실제 경로 수명 종료");
            return process.Runtime.MoveAwayDepth;
        }
        if (_pool.Edition == OriginalEdition.Patch1078 && _state.Debug)
        {
            int saved = current();
            if (saved < 0) Assert("0 <= p->moveAwayDepth", "\\Ns\\O\\Carrier.cpp", 0x87b);
            if (_state.Debug && saved > 3) Assert("p->moveAwayDepth <= MAX_MOVE_AWAY_DEPTH", "\\Ns\\O\\Carrier.cpp", 0x87c);
        }
        return current();
    }
    /// <summary>실제명령꼬리와마지막꼭짓점의위치를원본과같은순서로지정한다.</summary>
    public void BridgeCommand(int dais, int priest, uint currentStage)
    {
        if (priest == 0) priest = _ritual.FindStrongOwnedPriest(dais);
        if (priest == 0 || !CanBridge(dais, priest) || _ritual.Stage(dais) >= 5) return;
        if (priest >= _pool.Capacity) return;
        uint stage = _ritual.Stage(dais) + (currentStage != 0 ? 0u : 1u);
        if (_ritual.Stage(dais) >= 5) return;
        if (stage > 4) throw new InvalidOperationException("Dais 손상 목표5의 원본 미초기화 좌표 경계");
        TempleRitualPoint target = _ritual.StagePoint(dais, stage); TempleRitualPoint point = Position(priest);
        if (Integer(point.X) == Integer(target.X) && Integer(point.Y) == Integer(target.Y)) { _ritual.Ritual(dais, priest, 0); return; }
        if (!Boss()) return;
        int depth = MoveAwayDepth(priest); int next = unchecked(depth + 1);
        if (next >= 4) return;
        RawPathProcess? process = _path.Start(new RawPathStartRequest(priest, new RawPathPoint(target.X, target.Y), 1, 0, (uint)dais, stage, 0));
        if (process is null)
        {
            if (_pool.Edition == OriginalEdition.Patch1078 && _state.Debug) Assert("p", ".\\Dais.cpp", 0x18e);
            return;
        }
        process.Runtime.ReplacePoint(process.Runtime.Count - 1, new RawPathPoint(target.X, target.Y));
        RawPathStartType meta = Need(_hooks.PathMetadata)(priest);
        process.Prepare(0, meta.Speed, meta.TurningSpeed);
        process.Runtime.MoveAwayDepth = next;
    }
    /// <summary>patch의비로컬획득과CD의생략분기를추가추정없이보존한다.</summary>
    public void GrantCapturedType(uint type, uint owner)
    {
        Player(owner); bool local = owner == _state.LocalOwner;
        _state.SetGrantFlag(owner, 0); bool patch = _pool.Edition == OriginalEdition.Patch1078;
        if (type == _state.NuggetTypeCurrent())
        {
            if (local) { Need(_hooks.AddLocalSp)(5000); Need(_hooks.Chat)(new DaisChat { Kind = DaisChatKind.Stormpower }); }
            else if (patch) Need(_hooks.NormalizeForeignSp)(owner);
            return;
        }
        if (type == _state.NoIslandType)
        {
            if (local) Need(_hooks.LocalSpecial)(0);
            return;
        }
        if (local) { Need(_hooks.LocalType)(type); _state.SelectedType = type; }
        else if (patch) Need(_hooks.GrantForeignType)(owner, type);
    }
    /// <summary>실제cue/altar/현재count/owner/포획삭제/획득/UI/사제존재순서를유지한다.</summary>
    public void FinishSacrifice(int dais)
    {
        Need(_hooks.StopVoice)(_state.VoiceCue, 0, 0);
        int altar = _rune.AltarAt((ushort)dais);
        if (altar == 0)
        {
            if (_pool.Edition == OriginalEdition.Patch1078) Assert("altarSid", ".\\Dais.cpp", 0x2ba);
            else throw new InvalidOperationException("Dais altarSid assert");
        }
        Need(_hooks.SetFrame)(altar, First(altar, (byte)'P'), 0);
        uint originalOwner = Read(_pool.Slot(altar), 12, 2);
        if (!_state.Rune.CurrentSuppressDuration() && Contained(dais, 4, _pool.Slot(dais)[10]) != 0)
        {
            uint count = Count(dais);
            if (count >= (uint)UpgradeFirst.Length) throw new ArgumentOutOfRangeException(nameof(dais), "Dais 다음 첫 프레임 범위");
            Need(_hooks.SetFrame)(dais, UpgradeFirst[count], 0);
            uint owner = Owner(dais); Player(owner);
            if (_pool.Edition == OriginalEdition.Patch1078)
            {
                Span<byte> destination = _state.PlayerResultSlot(owner); uint currentCount = Count(dais);
                for (int index = 0; index < 4; index++) destination[index] = (byte)(currentCount >> (8 * index));
            }
            else { uint currentCount = Count(dais); _state.SetPlayerResult(owner, currentCount); }
            int captured = Contained(dais, 0, _pool.Slot(dais)[10]);
            if (captured != 0 && captured < _pool.Capacity) Need(_hooks.Destroy)(captured, ConsumedFlags);
            captured = Contained(dais, 0, PriestType());
            if (captured != 0 && captured < _pool.Capacity)
            {
                Need(_hooks.Destroy)(captured, ConsumedFlags);
                RawSquidWord.Set(_pool, altar, 0, () => _state.BridgeTypeCurrent(), _hooks.NotifySurface);
            }
            if (Owner(dais) == _state.LocalOwner) Need(_hooks.Chat)(new DaisChat { Kind = DaisChatKind.Upgrade });
        }
        else
        {
            uint type = CapturedWord(dais);
            if (!_state.Rune.CurrentSuppressDuration()) GrantCapturedType(type, Owner(dais));
            Player(Owner(dais)); _state.SetPlayerResult(Owner(dais), type);
        }
        for (uint owner = 1; owner <= PlayerCount; owner++) Need(_hooks.RefreshOwner)(owner);
        if (!_state.Rune.CurrentSuppressDuration())
        {
            if (Owner(dais) == _state.LocalOwner) Need(_hooks.RefreshLocal)();
            if (originalOwner == _state.LocalOwner) Need(_hooks.Chat)(new DaisChat { Kind = DaisChatKind.LocalPriestDead, OriginalOwner = originalOwner, CurrentOwner = Owner(dais), Status = Need(_hooks.MultiplayerStatus)() });
            else if (Owner(dais) != _state.LocalOwner) Need(_hooks.Chat)(new DaisChat { Kind = DaisChatKind.OtherPriestDead, OriginalOwner = originalOwner, CurrentOwner = Owner(dais) });
            if (!_state.Rune.CurrentSuppressDuration()) return;
        }
        if (Owner(dais) == _state.LocalOwner)
        {
            Need(_hooks.AddLocalSp)(5000); bool deadOpponent = false; Player(_state.LocalOwner);
            for (uint owner = 1; owner <= PlayerCount; owner++)
                if (!Need(_hooks.PriestExists)(owner) && Alliance(_state.LocalOwner, owner) != 0) deadOpponent = true;
            if (deadOpponent) Need(_hooks.Chat)(new DaisChat { Kind = DaisChatKind.Stormpower });
        }
    }
    /// <summary>다른월드의소비자등록을거부할때사용한다. 후속 GameWorld/등록의 동일 풀 검증이다.</summary>
    public RawSidPool Pool => _pool;
}
