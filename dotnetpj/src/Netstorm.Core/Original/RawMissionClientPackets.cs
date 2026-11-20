using System.Text;
using static Netstorm.Core.Original.RawMissionPacketFields;

// 같은 실제 Player/참여/SP/전역에서 로그인 클라이언트의8종 수신을 실행한다.
namespace Netstorm.Core.Original;

/// <summary>원본 전역의 실제 저장소와 현재 getter를 빌리는 수신 뷰다.</summary>
public sealed record MissionClientPacketState
{
    /// <summary>00540bc0의 현재 서버 모드다.</summary>
    public Func<bool>? Server { get; init; }
    /// <summary>00540bc4의 현재 방장 모드다.</summary>
    public Func<bool>? Boss { get; init; }
    /// <summary>00594fa4의 현재 미션 후처리 관문이다.</summary>
    public Func<bool>? MissionPost { get; init; }
    /// <summary>005c85d4의 현재 저장 게임 관문이다.</summary>
    public Func<bool>? SavedGame { get; init; }
    /// <summary>00594fc8의 현재 메타맵 관문이다.</summary>
    public Func<bool>? Metamap { get; init; }
    /// <summary>00594fbc의 현재 전투 관문이다.</summary>
    public Func<bool>? InBattle { get; init; }
    /// <summary>005c85a4의 현재 편집 모드다.</summary>
    public Func<uint>? Editor { get; init; }
    /// <summary>005467d8의 현재 CS 존재 DWORD다.</summary>
    public Func<uint>? Outpost { get; init; }
    /// <summary>동일 PlayerWorld가 소유하는 두 owner와 known 값이다.</summary>
    public PlayerWorldState Player { get; init; } = new();
    /// <summary>005409c4의 실제 시작 시드다.</summary>
    public UIntBox BeginSeed { get; init; } = new();
    /// <summary>00595088의 실제 전투 시드이며 공유 signed 주소가 없을 때 사용한다.</summary>
    public UIntBox BattleSeed { get; init; } = new();
    /// <summary>존재하면 전투 자원과 같은 signed 시드 주소다.</summary>
    public IntBox? SharedBattleSeed { get; init; }
    /// <summary>005949d4의 Fort 수신 누적 번호다.</summary>
    public UIntBox Revision { get; init; } = new();
    /// <summary>005424bc의 실제 옵션 파생값이다.</summary>
    public UIntBox OptionValue { get; init; } = new();
    /// <summary>005c7a74의 실제 시작 전역이다.</summary>
    public UIntBox Begin { get; init; } = new();
    /// <summary>00594fb0의 실제 clock 저장값이다.</summary>
    public UIntBox ClockStamp { get; init; } = new();
    /// <summary>00594fb4의 실제 기준 clock이다.</summary>
    public UIntBox BaseClock { get; init; } = new();
    /// <summary>005c8758의 현재 State 종류다.</summary>
    public Func<uint>? Phase { get; init; }
    /// <summary>005c87e8의 실제 동기 State 요청이다.</summary>
    public UIntBox CurrentState { get; init; } = new();
    /// <summary>005c8820의 실제 버전 거부 보조값이다.</summary>
    public UIntBox CurrentAux { get; init; } = new();
    /// <summary>005c8878의 실제 다음 State 요청이다.</summary>
    public UIntBox PendingState { get; init; } = new();
    /// <summary>0055a420의 실제 잘못된 응답 초기화 대상이다.</summary>
    public UIntBox SourceTicks { get; init; } = new();
    /// <summary>00595348의 실제 owner0 활성 표시다.</summary>
    public UIntBox CoreDataActive { get; init; } = new();
    /// <summary>005c8618의 실제 벽시각 기반 종료시각이다.</summary>
    public DoubleBox CaptureEnd { get; init; } = new();
    /// <summary>005c85f0의 현재 로그인 타이머다.</summary>
    public Func<double>? LoginEnd { get; init; }
    /// <summary>00595350의 현재10바이트 선택 포인터 조회다.</summary>
    public Func<Memory<byte>>? ReadCoreData { get; init; }
    /// <summary>00595350의 같은 실제 선택 포인터 변경이다.</summary>
    public Action<Memory<byte>>? StoreCoreData { get; init; }
    /// <summary>005953f0의 실제10바이트 캐시다.</summary>
    public Memory<byte> CachedCoreData { get; init; }
    /// <summary>005c86c0의 실제6바이트다.</summary>
    public Memory<byte> ZoneAddress { get; init; }
    /// <summary>0052f7e0의 실제87바이트다.</summary>
    public Memory<byte> BattleOptions { get; init; }
    /// <summary>00540ca8의 실제 NUL 종료 현재 이름이다.</summary>
    public ReadOnlyMemory<byte> CurrentHandle { get; init; }
    /// <summary>BattleOptions+0x3e의 같은 실제9바이트다.</summary>
    public ReadOnlyMemory<byte> Hidden { get; init; }
}

/// <summary>선택된 큰 실제 하위만 외부 장치로 받으며 누락을 성공으로 바꾸지 않는다.</summary>
public sealed record MissionClientPacketHooks
{
    /// <summary>원문 진단 뒤 반환하는 실제 장치다.</summary>
    public Action<string>? Assertion { get; init; }
    /// <summary>버전 불일치의 suppressVersionMessage 조회다.</summary>
    public Func<string, int>? ConfigInt { get; init; }
    /// <summary>버전 불일치 GameVerDiff의 실제 안내다.</summary>
    public Action<string>? Tell { get; init; }
    /// <summary>004b6dd0의 실제 동기 State 실행이다.</summary>
    public Action? RunState { get; init; }
    /// <summary>004340d0의 실제 카메라 초기화다.</summary>
    public Action? ResetCamera { get; init; }
    /// <summary>0043dad0의 실제 현재 SP 표시 갱신이다.</summary>
    public Action? RefreshSp { get; init; }
    /// <summary>00460d70의 현재 배정밀도 벽시각이다.</summary>
    public Func<double>? WallSeconds { get; init; }
    /// <summary>실제 CRT clock32비트 결과다.</summary>
    public Func<uint>? Clock { get; init; }
    /// <summary>현재 CS의 실제67h 전체 자식이다.</summary>
    public Action<uint, ReadOnlyMemory<byte>>? OutpostOptions { get; init; }
    /// <summary>현재 공급 반경/제곱 쓰기다.</summary>
    public Action? UpdateRange { get; init; }
    /// <summary>현재 signed 옵션 비용이다.</summary>
    public Func<int>? RequiredEnergy { get; init; }
    /// <summary>현재 TYPE+c4의 실제 float32 쓰기다.</summary>
    public Action<float>? SetEnergyType { get; init; }
    /// <summary>현재 옵션 파생값 조회다.</summary>
    public Func<uint>? ReadOptionValue { get; init; }
    /// <summary>실제 재접속 화면 재생성이다.</summary>
    public Action? ReconnectUi { get; init; }
    /// <summary>일반 모드의 자기 Fort 파일/Player/metadata 전체 부모다.</summary>
    public Action<uint>? LocalFort { get; init; }
    /// <summary>다른 소유자의 실제 Template 이미지 준비다.</summary>
    public Action<uint>? OpenFortImage { get; init; }
    /// <summary>같은 실제 Template의 이미지 읽기다.</summary>
    public Action<uint, ReadOnlyMemory<byte>>? ReadFortImage { get; init; }
    /// <summary>실제 전투섬 생성·배치 전체다.</summary>
    public Action<uint>? BuildFort { get; init; }
    /// <summary>현재 sender 연결의 실제 좁은 주소다.</summary>
    public Func<uint, string>? PeerAddress { get; init; }
    /// <summary>같은20채널의 실제 이름 추가다.</summary>
    public Action<uint, string, uint>? AddChat { get; init; }
    /// <summary>같은 subscriber 채널의 실제 제거다.</summary>
    public Action<uint, uint>? RemoveChat { get; init; }
    /// <summary>비미션의 실제0048fb30 전체다.</summary>
    public Action<uint>? RestoreCoreData { get; init; }
    /// <summary>00449200의 현재 전투 종료 관문이며 창 개수가 아니다.</summary>
    public Func<bool>? PlayerUiBusy { get; init; }
    /// <summary>비미션의 실제 재접속 정보 저장이다.</summary>
    public Action? SaveReconnect { get; init; }
    /// <summary>004d3090의 실제 kind/owner/argument 통지다.</summary>
    public Action<uint, uint, uint>? PlayerNotice { get; init; }
    /// <summary>현재 실제 연결/접속 gate를 지나는 송신이다.</summary>
    public Action<int, ReadOnlyMemory<byte>>? Send { get; init; }
    /// <summary>현재 전체 Ready mask 송신이다.</summary>
    public Action<uint>? SendReady { get; init; }
    /// <summary>송신 observer의 실제 식/파일/줄번호 진단이다.</summary>
    public Action<string, string, uint>? ObserverAssertion { get; init; }
}

/// <summary>RawMissionClientPackets.cpp의 실제8종 수신과 고정 원문F 선택을 보존한다.</summary>
public sealed class RawMissionClientPackets
{
    /// <summary>실제 Player 한 기록의 바이트 폭이다.</summary>
    private const int Stride = 180;
    /// <summary>모든 수신기가 공유하는 같은 Player9 원문이다.</summary>
    private readonly Memory<byte> _players;
    /// <summary>같은 Player/방향 동맹/채널을 갱신할 실제 참여 모듈이다.</summary>
    private readonly RawPlayerParticipation _participation;
    /// <summary>같은 게임 난수를 소비하는 실제 분산 SP 저장소다.</summary>
    private readonly RawSpStore _sp;
    /// <summary>원본 현재 전역의 실제 뷰다.</summary>
    private readonly MissionClientPacketState _state;
    /// <summary>실제 도달 위치에서만 요구하는 외부 소비자다.</summary>
    private readonly MissionClientPacketHooks _hooks;

    /// <summary>같은 Player9·옵션87/숨김9 별칭과 실제 주소 폭을 확인하며 수신은 실행하지 않는다.</summary>
    public RawMissionClientPackets(Memory<byte> players, RawPlayerParticipation participation, RawSpStore sp, MissionClientPacketState state, MissionClientPacketHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(participation); ArgumentNullException.ThrowIfNull(sp); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (players.Length != 1620 || !players.Equals(participation.Players) || state.CachedCoreData.Length != 10 || state.ZoneAddress.Length != 6 || state.BattleOptions.Length != 87 || state.Hidden.Length != 9
            || !state.Hidden.Equals((ReadOnlyMemory<byte>)state.BattleOptions.Slice(0x3e, 9))) throw new ArgumentException("미션 수신의 같은 실제 Player/현재 전역 크기·별칭 오류");
        _players = players; _participation = participation; _sp = sp; _state = state; _hooks = hooks;
    }
    /// <summary>현재 Player 필드의 실제 폭만 읽는다.</summary>
    private uint Read(uint owner, int offset, int width = 4)
    {
        if (owner >= 9 || (uint)offset > Stride || width > Stride - offset) throw new ArgumentOutOfRangeException(nameof(owner), "미션 수신 Player 읽기 범위");
        return Get(_players.Span, checked((int)owner * Stride + offset), width);
    }
    /// <summary>현재 Player의 지정 폭만 변경한다.</summary>
    private void Write(uint owner, int offset, uint value, int width = 4)
    {
        if (owner >= 9 || (uint)offset > Stride || width > Stride - offset) throw new ArgumentOutOfRangeException(nameof(owner), "미션 수신 Player 쓰기 범위");
        Put(_players.Span, checked((int)owner * Stride + offset), value, width);
    }
    /// <summary>현재 NUL까지 복사하며 필드 폭을 넘더라도 같은 Player9 물리 범위 안의 쓰기는 보존한다.</summary>
    private void Copy(uint owner, int offset, ReadOnlySpan<byte> source)
    {
        int end = source.IndexOf((byte)0);
        if (end < 0 || owner >= 9) throw new ArgumentOutOfRangeException(nameof(owner), "미션 수신 Player 문자열 범위/NUL 오류");
        int at = checked((int)owner * Stride + offset);
        if (at < 0 || at > _players.Length || end + 1 > _players.Length - at) throw new ArgumentOutOfRangeException(nameof(offset), "미션 수신 Player 문자열 물리 범위");
        source[..(end + 1)].CopyTo(_players.Span[at..]);
    }
    /// <summary>현재 정제된 이름을 원문 단일바이트 그대로 채널 장치에 전달한다.</summary>
    private string Name(uint owner)
    {
        if (owner >= 9) throw new ArgumentOutOfRangeException(nameof(owner)); ReadOnlySpan<byte> name = _players.Span[(checked((int)owner) * Stride + 12)..];
        int end = name.IndexOf((byte)0); if (end < 0) throw new ArgumentOutOfRangeException(nameof(owner), "미션 수신 이름 NUL 누락"); return Encoding.Latin1.GetString(name[..end]);
    }
    /// <summary>현재 두 owner/CoreData 주소·여섯바이트 주소·두 시드를 원문 순서로 변경한다.</summary>
    private void LoginBegin(ReadOnlyMemory<byte> packet)
    {
        if (Need(_state.Server)() && Get(packet.Span, 4) != _state.Player.BattleOwner) Need(_hooks.Assertion)("z->yourPlayerId == myPlayerId()");
        uint owner = Get(packet.Span, 4); if (owner >= 9) throw new ArgumentOutOfRangeException(nameof(packet), "LoginBegin Player 범위"); _state.Player.TemplateOwner = owner; _state.Player.BattleOwner = owner;
        if (owner == 0)
        {
            Memory<byte> core = Need(_state.ReadCoreData)();
            if (!core.IsEmpty) { if (core.Length < 10) throw new ArgumentOutOfRangeException(nameof(packet), "LoginBegin 이전 CoreData 범위"); core.Span[..10].CopyTo(_state.CachedCoreData.Span); }
            Need(_state.StoreCoreData)(Memory<byte>.Empty); _state.CoreDataActive.Value = 0;
        }
        else { _state.Player.PlayerKnown = 1; Need(_state.StoreCoreData)(Need(_state.Metamap)() ? _state.CachedCoreData : _players.Slice(checked((int)owner) * Stride + 0x6c, 10)); }
        _state.BeginSeed.Value = Get(packet.Span, 22); packet.Span.Slice(8, 6).CopyTo(_state.ZoneAddress.Span);
        uint seed = Get(packet.Span, 30); if (_state.SharedBattleSeed is { } shared) shared.Value = unchecked((int)seed); else _state.BattleSeed.Value = seed;
    }
    /// <summary>일반/메타가 같은67h 전체 부모와 현재 전역/효과를 사용한다.</summary>
    public void Options(uint sender, ReadOnlyMemory<byte> packet) => RawStateBattleOptions.Receive(sender, packet,
        new() { Options = _state.BattleOptions, Challenge = _state.Outpost, Value = _state.OptionValue, Battle = _state.InBattle, Mission = _state.MissionPost, Saved = _state.SavedGame },
        new() { Assertion = _hooks.Assertion, ChallengeOptions = _hooks.OutpostOptions, Range = _hooks.UpdateRange, Cost = _hooks.RequiredEnergy, StoreCost = _hooks.SetEnergyType, CurrentValue = _hooks.ReadOptionValue, Reconnect = _hooks.ReconnectUi });
    /// <summary>로컬 미션 서버의 재설치 생략과 선택된 실제 이미지/전투섬 부모 및 revision 증가를 보존한다.</summary>
    private void Fort(ReadOnlyMemory<byte> packet)
    {
        uint owner = Get(packet.Span, 4); if (owner >= 9) throw new ArgumentOutOfRangeException(nameof(packet), "미션 Fort Player 범위");
        if (owner == _state.Player.BattleOwner) { if (!Need(_state.MissionPost)()) Need(_hooks.LocalFort)(owner); }
        else { if (Need(_state.Server)()) { _state.Revision.Value = unchecked(_state.Revision.Value + 1); return; } Need(_hooks.OpenFortImage)(owner); }
        if (!Need(_state.Server)())
        {
            if (Need(_state.MissionPost)() || _state.Hidden.Span[checked((int)Get(packet.Span, 4))] == 0)
            {
                uint size = Get(packet.Span, 12); if (packet.Length < 17 || size > packet.Length - 17) throw new ArgumentOutOfRangeException(nameof(packet), "미션 Fort image 실제 범위");
                Need(_hooks.ReadFortImage)(owner, packet.Slice(17, checked((int)size))); Need(_hooks.BuildFort)(Get(packet.Span, 4));
            }
        }
        _state.Revision.Value = unchecked(_state.Revision.Value + 1);
    }
    /// <summary>이름·현재 flags/채널·방향별 동맹·metadata·UI 통지를 원문 위치에서 재조회한다.</summary>
    private void Player(uint sender, ReadOnlyMemory<byte> packet)
    {
        uint owner = Get(packet.Span, 0x32, 1); if (owner >= 9 || packet.Length < 0x3b) throw new ArgumentOutOfRangeException(nameof(packet), "미션 Player packet 범위"); Copy(owner, 12, packet.Span[0x3b..]);
        if (Need(_state.MissionPost)() && Get(packet.Span, 0x32, 1) == _state.Player.BattleOwner) Copy(owner, 12, _state.CurrentHandle.Span);
        SanitizePlayerName(Get(packet.Span, 0x26), _players.Span[(checked((int)owner) * Stride + 12)..]); Write(owner, 0x2c, Get(packet.Span, 0x26)); Write(owner, 0x71, Get(packet.Span, 0x3a, 1), 1);
        if (!Need(_state.Server)())
        {
            Copy(owner, 0x30, packet.Span[4..]);
            if (_players.Span[checked((int)owner) * Stride + 0x30] == (byte)'*') Copy(owner, 0x30, Encoding.Latin1.GetBytes(Need(_hooks.PeerAddress)(sender) + "\0"));
            Write(owner, 0x50, (Read(owner, 0x50) & 0x40) | Get(packet.Span, 0x2a)); Write(owner, 0x54, Get(packet.Span, 0x22));
        }
        if ((Read(owner, 0x50, 1) & 1) == 0) Need(_hooks.RemoveChat)(Read(owner, 0x2c), 0x20); else Need(_hooks.AddChat)(Read(owner, 0x2c), Name(owner), 0x20);
        _participation.SetAlliance(Get(packet.Span, 0x32, 1), Get(packet.Span, 0x22));
        if (!Need(_state.Server)())
        {
            Write(owner, 0x5c, Get(packet.Span, 0x34, 1)); Write(owner, 0x60, Get(packet.Span, 0x35, 2)); Write(owner, 0xa0, Get(packet.Span, 0x37, 1)); Write(owner, 0xa4, Get(packet.Span, 0x38, 1), 1); Write(owner, 0xa5, Get(packet.Span, 0x39, 1), 1);
        }
        if (!Need(_state.MissionPost)() && Read(owner, 0x2c) != 0) Need(_hooks.RestoreCoreData)(Get(packet.Span, 0x32, 1));
        if (!Need(_hooks.PlayerUiBusy)())
        {
            if (!Need(_state.MissionPost)()) Need(_hooks.SaveReconnect)();
            if ((Read(owner, 0x50, 1) & 1) != 0) { Need(_hooks.PlayerNotice)(5, Get(packet.Span, 0x32, 1), 0); Need(_hooks.PlayerNotice)(3, Get(packet.Span, 0x32, 1), 1); return; }
            Need(_hooks.PlayerNotice)(3, Get(packet.Span, 0x32, 1), 0); Need(_hooks.PlayerNotice)(2, Get(packet.Span, 0x32, 1), 0);
            if ((Read(owner, 0x50, 1) & 2) != 0) Need(_hooks.PlayerNotice)(5, Get(packet.Span, 0x32, 1), 1);
        }
    }
    /// <summary>현재 버전/설정·벽시각·phase 뒤 동기 State 요청을 원문 순서로 수행한다.</summary>
    private void Reply(ReadOnlyMemory<byte> packet)
    {
        uint major = Get(packet.Span, 4, 2), minor = Get(packet.Span, 6, 2);
        if ((major != 10 || minor < 0x4e) && Need(_hooks.ConfigInt)("suppressVersionMessage") == 0)
        {
            Need(_hooks.Tell)($"GameVerDiff({major}.{minor})"); _state.CurrentState.Value = 12; _state.CurrentAux.Value = 0; Need(_hooks.RunState)(); return;
        }
        ulong bits = Get(packet.Span, 10) | ((ulong)Get(packet.Span, 14) << 32); double remaining = BitConverter.UInt64BitsToDouble(bits); double now = Need(_hooks.WallSeconds)(); _state.CaptureEnd.Value = remaining + now;
        if (Need(_state.Phase)() == 16) { _state.CurrentState.Value = 18u - (Get(packet.Span, 9, 1) != 0 ? 1u : 0); Need(_hooks.RunState)(); return; }
        if (Need(_state.Phase)() == 10) { _state.CurrentState.Value = (Get(packet.Span, 9, 1) != 0 ? 2u : 0) + 11; Need(_hooks.ResetCamera)(); Need(_hooks.RunState)(); return; }
        _state.SourceTicks.Value = 0; _state.PendingState.Value = 3;
    }
    /// <summary>현재 count를 매 반복마다 읽고 음수 입력을 건너뛰며 같은 분산 SP를 직접 지정한다.</summary>
    private void Sp(ReadOnlyMemory<byte> packet)
    {
        // 표시 callback이 현재 count/owner/편집 모드를 바꾸면 다음 원문 읽기에서 반영한다.
        for (uint owner = 0; unchecked((int)owner) < unchecked((int)Get(packet.Span, 4)); owner++)
        {
            uint value = Get(packet.Span, checked(8 + (int)owner * 4)); if (unchecked((int)value) < 0) continue;
            uint amount = Get(packet.Span, checked(8 + (int)owner * 4)); bool local = _state.Player.PlayerKnown != 0 && owner == _state.Player.BattleOwner;
            if (local || (owner > 0 && owner < 9))
            {
                float number = Need(_state.Editor)() != 0 ? 194162f : (float)unchecked((int)amount); _sp.Set(local ? _state.Player.BattleOwner : owner, BitConverter.SingleToUInt32Bits(number)); if (local) Need(_hooks.RefreshSp)();
            }
        }
    }
    /// <summary>방장의 서버9 자연 반환과 client mask/현재 자기40응답을 각각 실행한다.</summary>
    private void Ready(uint sender, ReadOnlyMemory<byte> packet)
    {
        if (!Need(_state.Boss)())
        {
            if (unchecked((int)Get(packet.Span, 4)) < 0) _state.Begin.Value = 1;
            // 각 현재 mask 읽기와 Player flags의 원문DWORD OR를 보존한다.
            for (uint owner = 1; owner < 9; owner++) if ((Get(packet.Span, 4) & (1u << (int)owner)) != 0) Write(owner, 0x50, Read(owner, 0x50) | 0x40);
            uint local = _state.Player.BattleOwner; if ((Read(local, 0x50) & 0x40) == 0) { Write(local, 0x50, Read(local, 0x50) | 0x40); Need(_hooks.Send)(9, packet[..8]); }
        }
        else if (sender != 9)
        {
            Write(sender, 0x50, Read(sender, 0x50) | 0x40); Need(_hooks.SendReady)(0);
            if (Need(_state.LoginEnd)() == 0) Need(_hooks.Send)(unchecked((int)sender), new byte[] { 1, 0x6e, 8, 0, 0, 0, 0, 0x80 });
        }
    }
    /// <summary>현재 패킷 종류에 해당하는 실제 원문F를 선택하며 미등록 종류를 거부한다.</summary>
    public void Receive(uint sender, ReadOnlyMemory<byte> packet) => ReceiveFixed(Get(packet.Span, 1, 1) switch
    {
        0x29 => 0x004b3b40u, 0x67 => 0x004b4900u, 0x2b => 0x004912d0u, 0x2a => 0x004b3d30u, 0x28 => 0x004b8940u, 0x81 => 0x004b3e90u, 0x84 => 0x004a6f30u, 0x6e => 0x004a7050u,
        _ => throw new ArgumentException("미등록 미션 클라이언트 packet 종류", nameof(packet)),
    }, sender, packet);
    /// <summary>저장된 원문F만 선택하며 현재 packet[1]과 관계없는 연결을 읽지 않는다.</summary>
    public void ReceiveFixed(uint entry, uint sender, ReadOnlyMemory<byte> packet)
    {
        switch (entry)
        {
            case 0x004b3b40: LoginBegin(packet); return;
            case 0x004b4900: Options(sender, packet); return;
            case 0x004912d0: Player(sender, packet); return;
            case 0x004b3d30: Fort(packet); return;
            case 0x004b8940: Reply(packet); return;
            case 0x004b3e90: Sp(packet); return;
            case 0x004a6f30:
                int ticks = unchecked((int)Get(packet.Span, 4)); _state.ClockStamp.Value = Need(_hooks.Clock)(); long difference = (long)unchecked((int)_state.ClockStamp.Value) - (long)ticks * 1000; _state.BaseClock.Value = unchecked((uint)difference); return;
            case 0x004a7050: Ready(sender, packet); return;
            default: throw new ArgumentException("미등록 미션 클라이언트 원문F", nameof(entry));
        }
    }
    /// <summary>0.693의 같은 공유 observer를 실제 진단 장치와 호출한다.</summary>
    public void Observe(uint type, Memory<byte> packet, uint sending) => ObserveClient(type, packet, sending, _hooks.ObserverAssertion);
}
