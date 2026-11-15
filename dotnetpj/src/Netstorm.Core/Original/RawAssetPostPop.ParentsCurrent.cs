// 거주 구역/간헐천의 현재 장치·목록·물리 쓰기와 실제 순회 순서를 보존한다.
namespace Netstorm.Core.Original;

/// <summary>자산 후처리가 실제 장기 권한/목록/TYPE를 소비 위치에서 빌리는 상태다.</summary>
public sealed partial class AssetPostPopState
{
    /// <summary>실제 Client의 같은 서버/보스 owner다.</summary>
    public RawAuthorityState? SharedAuthority { get; set; }
    /// <summary>거주 구역의 실제 장기50칸 목록이다.</summary>
    public RawPostPopList? SharedResidences { get; set; }
    /// <summary>신전의 실제 장기20칸 목록이다.</summary>
    public RawPostPopList? SharedTemples { get; set; }
    /// <summary>간헐천의 실제 장기100칸 목록이다.</summary>
    public RawPostPopList? SharedGeysers { get; set; }
    /// <summary>아웃포스트의 별도 실제100칸 목록이다.</summary>
    public RawPostPopList? SharedOutposts { get; set; }
    /// <summary>WORD 통지가 읽는 독립 다리 TYPE DWORD다.</summary>
    public uint BridgeType { get; set; } = 82;
    /// <summary>같은 실제 BridgeState의 현재 다리 TYPE 조회다.</summary>
    public Func<uint>? SharedBridgeType { get; set; }
    /// <summary>같은 실제 BridgeState의 현재 섬 받침 TYPE 조회다.</summary>
    public Func<uint>? SharedIslandType { get; set; }
    /// <summary>같은 실제 BridgeState의 현재 종유석 TYPE 조회다.</summary>
    public Func<uint>? SharedStalagType { get; set; }
    /// <summary>실제 서버 주소가 있으면 독립 fixture의 과거 battle 입력과 분리한다.</summary>
    public bool CurrentServer => SharedAuthority?.Server ?? InBattle;
    /// <summary>실제 보스 주소가 있으면 같은 현재 권한을 읽는다.</summary>
    public bool CurrentBoss => SharedAuthority?.Boss ?? Boss;
    /// <summary>같은 현재 거주 구역 목록을 반환한다.</summary>
    public RawPostPopList CurrentResidences => SharedResidences ?? Residences;
    /// <summary>같은 현재 신전 목록을 반환한다.</summary>
    public RawPostPopList CurrentTemples => SharedTemples ?? Temples;
    /// <summary>같은 현재 간헐천 목록을 반환한다.</summary>
    public RawPostPopList CurrentGeysers => SharedGeysers ?? Geysers;
    /// <summary>같은 현재 별도 아웃포스트 목록을 반환한다.</summary>
    public RawPostPopList CurrentOutposts => SharedOutposts ?? Outposts;
    /// <summary>WORD 저장 뒤 현재 전체 다리 TYPE를 읽는다.</summary>
    public uint ReadBridgeType() => SharedBridgeType?.Invoke() ?? BridgeType;
    /// <summary>각 후보 TYPE 비교 때 현재 전체 섬 받침 TYPE를 읽는다.</summary>
    public uint ReadIslandType() => SharedIslandType?.Invoke() ?? IslandType;
    /// <summary>섬 받침이 아닌 후보에서 현재 전체 종유석 TYPE를 읽는다.</summary>
    public uint ReadStalagType() => SharedStalagType?.Invoke() ?? StalagType;
}

/// <summary>현재 원문 부모의 부분 효과와 호출 함수 사본을 같은 물리에서 유지한다.</summary>
public sealed partial class RawAssetPostPop
{
    /// <summary>도달한 실제 장치만 선택하며 함수 연결을 자기 호출 중 바꿔도 현재 호출은 유지한다.</summary>
    private static T AssetNeed<T>(T? callback) where T : Delegate => callback ?? throw new InvalidOperationException("자산 후처리의 선택된 실제 장치 미연결");
    /// <summary>원문 signed count/capacity 관문 뒤 store→현재 count 증가만 수행한다.</summary>
    private static void AppendAssetList(RawPostPopList list, int sid)
    {
        uint used = list.Count, capacity = list.Capacity;
        if (unchecked((int)used) >= unchecked((int)capacity)) return;
        if (unchecked((int)used) < 0) throw new ArgumentOutOfRangeException(nameof(list), "자산 목록의 음수 저장 위치");
        uint[] storage = list.Entries; if (used >= storage.Length) throw new ArgumentOutOfRangeException(nameof(list), "자산 목록의 실제 저장 배열 범위 오류");
        storage[used] = (uint)sid; list.Count = unchecked(list.Count + 1);
    }
    /// <summary>WORD만 저장하고 그 뒤 현재 다리 TYPE가 선택한 통지를 실행한다.</summary>
    private void SetAssetWord(int sid, uint value)
    {
        Write(_pool.RawSlot(sid), 12, value, 2);
        if (_pool.Slot(sid)[10] == _state.ReadBridgeType()) AssetNeed(_hooks.NotifySurface)(sid);
    }
    /// <summary>CRT signed64 범위 밖/비유한의 indefinite low DWORD0를 Native 경로에서만 보존한다.</summary>
    private static uint NativeTruncate(float value)
    {
        double wide = value;
        return !double.IsFinite(wide) || wide < -9223372036854775808.0 || wide >= 9223372036854775808.0 ? 0 : unchecked((uint)(long)wide);
    }
    /// <summary>거주 구역 최초 지면/소유자→목록→독립 예약→BYTE 통지→현재 잔여 프레임 순서다.</summary>
    private void ResidenceCurrent(int sid, uint flags)
    {
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        if ((flags & 1) != 0)
        {
            ReadOnlySpan<byte> raw = _pool.Slot(sid); int surface = AssetNeed(_hooks.Surface)(X(raw, 14), X(raw, 18));
            if (surface != 0) { uint owner = _pool.Slot(surface)[patch ? 34 : 32]; if (owner != 0) AssetNeed(_hooks.SetOwner)(sid, owner); }
            AppendAssetList(_state.CurrentResidences, sid); Regular(sid, 0, 0); Regular(sid, 1, 0); if (patch) Regular(sid, 3, 0);
            _pool.RawSlot(sid)[12] &= 0xf0; AssetNeed(_hooks.NotifySurface)(sid);
            if (patch)
            {
                int value = unchecked((int)Read(_pool.Slot(sid), 36));
                if (_state.ResidenceAnimation)
                {
                    value = unchecked(value / 2 * 2 + (_pool.Slot(sid)[12] & 15)); int group = value < 6 ? 0 : value < 12 ? 1 : value < 18 ? 2 : 3;
                    value = unchecked(value - group * 6);
                }
                Write(_pool.RawSlot(sid), 42, unchecked((uint)value));
            }
        }
        _base.PostPopBase(sid, flags);
    }
    /// <summary>가상 선택이 실제 간헐천 body를 정한 Native 장기 호출에 동일 전체 본문을 제공한다.</summary>
    public void GeyserPostPop(int sid, uint flags) => GeyserCurrent(sid, flags, true);
    /// <summary>최초 목록/등록→현재 비용 WORD→예약→현재 후보 효과→직접 Damageable를 실행한다.</summary>
    private void GeyserCurrent(int sid, uint flags, bool native)
    {
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        if ((flags & 1) != 0 && (_pool.Slot(sid)[patch ? 40 : 35] & 9) == 0)
        {
            AppendAssetList(_state.CurrentGeysers, sid); AssetNeed(_hooks.GeyserRegistration)(sid, 1);
            SetAssetWord(sid, native ? NativeTruncate(Type(sid).Cost) : Truncate(Type(sid).Cost)); Regular(sid, 0, 0.1f);
            ReadOnlySpan<byte> raw = _pool.Slot(sid);
            // 각 효과 뒤 다시 선택된 후보 TYPE와 현재 지면 TYPE를 비교한다.
            void Visit(int neighbor)
            {
                byte type = _pool.Slot(neighbor)[10];
                if (type == _state.ReadIslandType() || type == _state.ReadStalagType())
                {
                    AssetNeed(_hooks.Unpop)(neighbor, 0); Write(_pool.RawSlot(neighbor), patch ? 36 : 34, 8, patch ? 4 : 1); AssetNeed(_hooks.Repop)(neighbor, 0x40);
                }
            }
            if (_hooks.VisitAtPosition is not null) AssetNeed(_hooks.VisitAtPosition)(X(raw, 14), X(raw, 18), Visit);
            else
            {
                int[] neighbors = AssetNeed(_hooks.AtPosition)(X(raw, 14), X(raw, 18));
                // 독립 벡터 경계에서도 각 후보의 현재 TYPE는 앞선 효과 뒤 읽는다.
                foreach (int neighbor in neighbors) Visit(neighbor);
            }
        }
        _damageable.DamageablePostPop(sid, flags);
    }
    /// <summary>좌표 정수화와 실제 일반 finder의 Next를 각 후보 효과와 번갈아 실행한다.</summary>
    public static Action<float, float, Action<int>> MakeAssetPostPopFinder(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types, bool native = false)
    {
        return (x, y, visitor) =>
        {
            Action<int> action = AssetNeed(visitor); int cellX = unchecked((int)(native ? NativeTruncate(x) : Truncate(x))), cellY = unchecked((int)(native ? NativeTruncate(y) : Truncate(y)));
            var finder = new RawSquidFinder(pool, hash, types);
            // 최초 후보의 저장된 next와 효과 뒤 Next의 현재 링크/버킷 조회를 보존한다.
            for (int sid = finder.Begin(new(cellX, cellY, cellX, cellY)); sid != 0; sid = finder.Next()) action(sid);
        };
    }
}
