using System.Buffers.Binary;

// 시설 연구의 원문 입력·수신·내용물 통지를 같은 슬롯과 현재 전역에 적용한다.
namespace Netstorm.Core.Original;

/// <summary>효과 전후에 다시 읽는 로딩 깊이·권한·시각·제외 종류와 공유 생산 알림이다.</summary>
public sealed class FactoryResearchState
{
    /// <summary>현재 로딩 깊이다. 비영이면 생산 통지를 억제한다.</summary>
    public Func<uint>? LoadingDepth { get; set; }
    /// <summary>다른 생산 경로와 공유하는 감김 DWORD다.</summary>
    public UIntBox ProductionDirty { get; set; } = new();
    /// <summary>현재 서버 권한이다. 진단 뒤에도 다시 조회한다.</summary>
    public Func<bool>? Boss { get; set; }
    /// <summary>입력 로그 뒤 패킷에 복사하는 현재 QWORD 시각이다.</summary>
    public Func<double>? Now { get; set; }
    /// <summary>연구 수신에서 제외할 현재 아웃포스트 TYPE다.</summary>
    public Func<uint>? OutpostType { get; set; }
}

/// <summary>검색·슬롯·실제 생성·송신·화면 효과의 선택된 하위 장치다.</summary>
public sealed class FactoryResearchHooks
{
    /// <summary>부모·mask·종류·순번으로 현재 내용물을 찾는다.</summary>
    public Func<int, uint, uint, uint, int>? Find { get; set; }
    /// <summary>현재 공장에 남은 서로 다른 연구 종류의 슬롯 수다.</summary>
    public Func<int, int>? Slots { get; set; }
    /// <summary>선택된 예측 내용물을 실제 가상 삭제한다.</summary>
    public Action<int, uint>? Destroy { get; set; }
    /// <summary>부모·종류·수량·목록 플래그·부착 플래그·품질로 실제 내용을 생성한다.</summary>
    public Func<int, uint, uint, uint, uint, uint, int>? Create { get; set; }
    /// <summary>현재 요청 원문을 채널9로 보낸다.</summary>
    public Action<uint, ReadOnlyMemory<byte>>? Send { get; set; }
    /// <summary>연구 도움말 절을 지정 모드로 연다.</summary>
    public Action<string, uint>? Help { get; set; }
    /// <summary>부모의 실제 플레어를 개수·사건 번호로 생성한다.</summary>
    public Action<int, int, uint>? Flare { get; set; }
    /// <summary>현재 소유자의 같은 생산 Deck을 재계산한다.</summary>
    public Action<uint>? Refresh { get; set; }
    /// <summary>복귀 후 현재 자료를 다시 읽을 수 있는 원문 진단이다.</summary>
    public Action<string>? Assertion { get; set; }
    /// <summary>같은58h의 kind0/mode1 업그레이드 수신 본체다.</summary>
    public Action<ReadOnlyMemory<byte>>? UpgradeReceive { get; set; }
    /// <summary>슬롯 조회 뒤 실행할 종류5 입력 로그다. 독립 관찰에서는 생략할 수 있다.</summary>
    public Action<int, int>? InputLog { get; set; }
}

/// <summary>453cc0·4547d0·453d70·453b50→475220의 연구 경로를 실행한다.</summary>
/// <remarks>근거: cpppj/src/o/RawFactoryResearch.cpp·h. 업그레이드 mode1은 기존 본체에 위임한다.</remarks>
public sealed class RawFactoryResearch
{
    /// <summary>수정키를 제거한 연구 메뉴 명령이다.</summary>
    private const uint ResearchCode = 0x03000080u;
    /// <summary>원문 부모·내용물이 들어 있는 같은 PATCH 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>현재 공장 genus를 읽는188개 TYPE 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>효과 전후 현재 전역을 빌리는 뷰다.</summary>
    private readonly FactoryResearchState _state;
    /// <summary>선택된 실제 하위 장치다.</summary>
    private readonly FactoryResearchHooks _hooks;

    /// <summary>게임 효과 없이 PATCH 풀과188개 현재 TYPE의 계약을 확인한다.</summary>
    public RawFactoryResearch(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types,
        FactoryResearchState state, FactoryResearchHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count != 188)
            throw new ArgumentException("시설 연구 PATCH/TYPE 계약 오류");
        _pool = pool; _types = types; _state = state; _hooks = hooks;
    }

    /// <summary>세션 장치들이 같은 풀을 공유하는지 확인한다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>내용물 통지가453b50인 두 실제 가상 표만 선택한다.</summary>
    public bool HandlesNotice(int parent) => Read(parent, 0) is 0x509148u or 0x509210u;

    /// <summary>예측 삭제→슬롯 재조회→동일 종류 재검색→실제 생성 순서다.</summary>
    public void Learn(int parent, uint kind, uint attachFlags)
    {
        int found = Need(_hooks.Find)(parent, 16, kind, 0);
        if (Client(found)) Need(_hooks.Destroy)(found, 0);
        int remaining = Need(_hooks.Slots)(parent);
        if (found != 0 && remaining <= 0) Need(_hooks.Assertion)("slotsRemaining > 0");
        if (remaining < 1) return;
        if (Need(_hooks.Find)(parent, 16, kind, 0) == 0)
            _ = Need(_hooks.Create)(parent, kind, 0, 16, attachFlags, 0);
    }

    /// <summary>상위 signed WORD 선택값을 처리하고 Learn 전 현재 시각의16바이트 요청을 보낸다.</summary>
    public uint Input(int parent, uint code, uint packedArgument)
    {
        if ((code & 0x03ffffffu) != ResearchCode)
            throw new InvalidOperationException("시설 연구 외 입력 본체 미복원");
        int kind = unchecked((short)(ushort)(packedArgument >> 16));
        if (kind == 0) { Need(_hooks.Help)("researchHelp", 1); return 1; }
        int slots = Need(_hooks.Slots)(parent);
        _hooks.InputLog?.Invoke(parent, slots);
        if (slots > 0)
        {
            var packet = new byte[FactoryResearchCommand.PacketBytes];
            FactoryResearchCommand.Write(parent, unchecked((uint)kind), Need(_state.Now)(), packet);
            Learn(parent, unchecked((uint)kind), 16);
            Need(_hooks.Send)(9, packet);
        }
        return 1;
    }

    /// <summary>같은58h를 현재 상태·genus·소유자·제외 종류로 분배하고 연구 또는 업그레이드를 실행한다.</summary>
    public void Receive(uint sender, ReadOnlyMemory<byte> packet)
    {
        if (packet.Length < 8) throw new ArgumentOutOfRangeException(nameof(packet), "시설 연구 수신 본문 부족");
        int parent = BinaryPrimitives.ReadUInt16LittleEndian(packet.Span[4..]);
        if ((Read(parent, 11, 1) & 3u) != 0) return;
        if (packet.Span[6] == 0)
        {
            if (packet.Span[7] == 1) Need(_hooks.UpgradeReceive)(packet);
            return;
        }
        uint type = Read(parent, 10, 1);
        if (type >= _types.Count) throw new ArgumentOutOfRangeException(nameof(packet), "시설 연구 현재 TYPE 범위 오류");
        if ((_types[(int)type].Flags2 & 0x4000u) == 0 || Read(parent, 34, 1) != sender
            || Read(parent, 10, 1) == Need(_state.OutpostType)()) return;
        if (!Need(_state.Boss)())
        {
            Need(_hooks.Assertion)("boss");
            if (!Need(_state.Boss)()) return;
        }
        Learn(parent, packet.Span[6], 0);
    }

    /// <summary>부착 플레어 뒤 현재 종류로 중복 예측을 지우고 실제 생산 통지를 실행한다.</summary>
    public void Notice(int parent, int form, uint reason)
    {
        if (reason == 1 && form != 0 && (Read(form, 20, 1) & 16u) != 0)
        {
            Need(_hooks.Flare)(parent, 6, 2);
            int found = Need(_hooks.Find)(parent, 16, Read(form, 18, 2), 0);
            if (found != 0 && found != form && Client(found)) Need(_hooks.Destroy)(found, 0);
        }
        ManaNotice(parent, form, reason);
    }

    /// <summary>form0도 읽으며 flags 지역값과 각 위치의 현재 깊이·소유자 조회 순서를 보존한다.</summary>
    private void ManaNotice(int parent, int form, uint reason)
    {
        uint flags = Read(form, 20, 1);
        if (Need(_state.LoadingDepth)() == 0 && (flags & 16u) != 0)
            _state.ProductionDirty.Value = unchecked(_state.ProductionDirty.Value + 1);
        if (reason == 1 && Need(_state.LoadingDepth)() == 0 && (flags & 16u) != 0)
        {
            if ((Read(parent, 40, 1) & 9u) != 0) Need(_hooks.Assertion)("!isAstral() && !isAbstract()");
            Need(_hooks.Refresh)(Read(parent, 34, 1));
        }
    }

    /// <summary>물리 슬롯의 지정 BYTE·WORD·DWORD만 비정렬 little endian으로 읽는다.</summary>
    private uint Read(int sid, int offset, int width = 4)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid)[offset..];
        return width switch { 1 => raw[0], 2 => BinaryPrimitives.ReadUInt16LittleEndian(raw), 4 => BinaryPrimitives.ReadUInt32LittleEndian(raw), _ => throw new ArgumentOutOfRangeException(nameof(width)) };
    }
    /// <summary>원문41bce0의 예측 client SID 범위다.</summary>
    private static bool Client(int sid) => sid > 4 && sid < 15000;
    /// <summary>선택된 미연결 장치를 호출 위치에서 명시 오류로 보고한다.</summary>
    private static T Need<T>(T? function) where T : Delegate => function ?? throw new InvalidOperationException("시설 연구 실제 하위·전역 미연결");
}
