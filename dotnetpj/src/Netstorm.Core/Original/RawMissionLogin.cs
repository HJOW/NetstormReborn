using System.Text;
using static Netstorm.Core.Original.RawMissionPacketFields;

// 실제 연결은 형식 매개변수로 그대로 빌리고 로그인/Ready의 현재 저장소를 공유한다.
namespace Netstorm.Core.Original;

/// <summary>RawMissionLogin.cpp의 실제 서버 로그인/재접속 부모이며 TConnection은 같은 실제 노드다.</summary>
public sealed class RawMissionLogin<TConnection> where TConnection : class
{
    /// <summary>현재 실제 Player9 원문이다.</summary>
    private readonly Memory<byte> _players;
    /// <summary>같은 Player/방향 동맹/실제 연결 gate를 처리하는 참여 모듈이다.</summary>
    private readonly RawPlayerParticipation _participation;
    /// <summary>장기 실제 전역 참조와 현재 getter다.</summary>
    private readonly MissionLoginGlobals _globals;
    /// <summary>선택된 큰 실제 하위 장치다.</summary>
    private readonly MissionLoginHooks<TConnection> _hooks;
    /// <summary>타이머와 같은 실제 Player9 집계다.</summary>
    private readonly MissionLoginPlayerCounts _counts;
    /// <summary>동일 실제 Player/Zone/옵션/캐시를 빌리며 접속이나 수신은 실행하지 않는다.</summary>
    public RawMissionLogin(Memory<byte> players, RawPlayerParticipation participation, MissionLoginGlobals globals, MissionLoginHooks<TConnection> hooks)
    {
        ArgumentNullException.ThrowIfNull(participation); ArgumentNullException.ThrowIfNull(globals); ArgumentNullException.ThrowIfNull(hooks);
        if (players.Length != 1620 || !players.Equals(participation.Players) || globals.ZoneAddress.Length != 6 || globals.BattleOptions.Length != 87 || globals.CachedCoreData.Length != 10) throw new ArgumentException("미션 로그인 실제 Player/Zone/옵션 크기·소유 오류");
        _players = players; _participation = participation; _globals = globals; _hooks = hooks; _counts = new(OriginalEdition.Patch1078, players);
    }
    /// <summary>같은Session의실제Player물리인지연결부모가검증할수있게빌린다.</summary>
    public Memory<byte> Players => _players;
    /// <summary>현재장기옵션의같은주소를빌린다.</summary>
    public ReadOnlyMemory<byte> BattleOptions => _globals.BattleOptions;
    /// <summary>Player0을 포함한 실제 현재 필드를 원문 폭대로 읽는다.</summary>
    private uint Read(uint owner, int offset, int width = 4)
    {
        if (owner >= 9 || (uint)offset > 180 || width > 180 - offset) throw new ArgumentOutOfRangeException(nameof(owner), "미션 로그인 Player 읽기 범위"); return Get(_players.Span, checked((int)owner) * 180 + offset, width);
    }
    /// <summary>같은 현재 Player DWORD만 쓰고 인접 필드를 보존한다.</summary>
    private void Write(uint owner, int offset, uint value)
    {
        if (owner >= 9 || (uint)offset > 176) throw new ArgumentOutOfRangeException(nameof(owner), "미션 로그인 Player 쓰기 범위"); Put(_players.Span, checked((int)owner) * 180 + offset, value);
    }
    /// <summary>고정 생성자는0채움/실제 길이, 가변은ff/초기선언4를 각각 보존한다.</summary>
    private static byte[] Packet(byte type, int size, byte fill = 0, ushort initialLength = 0)
    {
        byte[] bytes = new byte[size]; Array.Fill(bytes, fill); bytes[0] = 1; bytes[1] = type; Put(bytes, 2, initialLength == 0 ? (uint)size : initialLength, 2); return bytes;
    }
    /// <summary>원본 _ftol의64비트 절삭 하위DWORD이며 비유한/범위 밖은0이다.</summary>
    private static uint Ftol(float value)
    {
        double wide = value; if (!(wide > -9223372036854775808.0 && wide < 9223372036854775808.0)) return 0; return unchecked((uint)(long)wide);
    }
    /// <summary>벽시각 callback 복귀 뒤 현재 captureEnd를 읽어 실제18바이트 응답을 작성한다.</summary>
    private byte[] Reply(int owner, byte accepted)
    {
        byte[] bytes = Packet(0x28, 18); Put(bytes, 4, 10, 2); Put(bytes, 6, 0x4e, 2); bytes[8] = unchecked((byte)owner); bytes[9] = accepted;
        double now = Need(_hooks.WallSeconds)(); ulong bits = BitConverter.DoubleToUInt64Bits(Need(_globals.CaptureEnd)() - now); Put(bytes, 10, (uint)bits); Put(bytes, 14, (uint)(bits >> 32)); return bytes;
    }
    /// <summary>같은 실제 응답 writer를 CheckLogin 거부 경계에서도 사용한다.</summary>
    public void SendReply(int owner, byte accepted) { byte[] bytes = Reply(owner, accepted); Need(_hooks.Send)(owner, bytes); }
    /// <summary>현재6바이트·두 시드의34바이트와 선택된 실제 디버그3예약을 보낸다.</summary>
    private void SendBegin(uint owner)
    {
        byte[] bytes = Packet(0x29, 34); Put(bytes, 4, owner); _globals.ZoneAddress.Span.CopyTo(bytes.AsSpan(8)); Put(bytes, 22, Need(_globals.BeginSeed)()); Put(bytes, 30, _globals.SharedBattleSeed is { } shared ? unchecked((uint)shared.Value) : Need(_globals.BattleSeed)()); Need(_hooks.Send)(unchecked((int)owner), bytes);
        if (Need(_globals.DebugBurst)() != 0)
        {
            // 각 실제 송신 뒤 다음 예약에서 현재 헤더/길이를 새로 읽는다.
            for (int repeat = 0; repeat < 3; repeat++)
            {
                if (_globals.DebugPacketHeader.Length != 2 || _globals.DebugPacketInitialLength is null) throw new InvalidOperationException("로그인 디버그의 현재0ch 생성 헤더 미연결");
                byte message = _globals.DebugPacketHeader.Span[0], protocol = _globals.DebugPacketHeader.Span[1]; ushort initial = _globals.DebugPacketInitialLength(); byte[] packet = new byte[63012]; Array.Fill(packet, (byte)255); packet[1] = message; packet[0] = (byte)(protocol & 0x7f); Put(packet, 2, Math.Max((uint)initial, 4), 2); Put(packet, 8, 63000);
                // 원본40a060의payload+12에 인덱스의 낮은BYTE만 쓴다.
                for (int index = 0; index < 63000; index++) packet[12 + index] = unchecked((byte)index);
                Need(_hooks.Send)(unchecked((int)owner), packet);
            }
        }
    }
    /// <summary>owner0은ff로 남기고 현재1..8 분산SP를 각각 절삭한 전체44바이트 예약을 보낸다.</summary>
    private void SendSp(uint owner)
    {
        byte[] bytes = Packet(0x81, 44, 255, 4);
        // 각 현재SP는 실제 하위를 한 번씩 호출해 같은 시점의 비트를 사용한다.
        for (uint player = 1; player < 9; player++) Put(bytes, 8 + (int)player * 4, Ftol(BitConverter.UInt32BitsToSingle(Need(_hooks.SpBits)(player))));
        Put(bytes, 4, 9); Need(_hooks.Send)(unchecked((int)owner), bytes);
    }
    /// <summary>가입 기록 갱신 뒤 새 Player 전체분배와 기존 참여자의 새 대상 송신 순회를 따로 실행한다.</summary>
    private void Joined(uint owner, uint subscriber)
    {
        Write(owner, 0x2c, subscriber);
        if (owner != _globals.Player.BattleOwner)
        {
            byte[] address = Encoding.Latin1.GetBytes(Need(_hooks.PeerAddress)(owner)); int at = checked((int)owner) * 180 + 0x30; if (address.Length + 1 > _players.Length - at) throw new ArgumentOutOfRangeException(nameof(owner), "미션 로그인 peer주소 범위"); address.CopyTo(_players.Span[at..]); _players.Span[at + address.Length] = 0;
        }
        Write(owner, 0x50, (Read(owner, 0x50) & ~0x100u) | 3);
        // callback 뒤 현재 각 대상의 접속 낮은BYTE를 다시 읽는다.
        for (uint target = 1; target < 9; target++) if ((Read(target, 0x50, 1) & 1) != 0) Need(_hooks.SendPlayer)(owner, target);
        // 두번째 원문 순회는 현재 사람 참여 Player 각각을 새 대상에게 보낸다.
        for (uint player = 1; player < 9; player++) if ((Read(player, 0x50, 1) & 2) != 0) Need(_hooks.SendPlayer)(player, owner);
    }
    /// <summary>0.695의 같은 원문 Ready writer와 현재 실제 Player/방장/송신을 사용한다.</summary>
    public void SendReady(uint target) => RawMissionReady.Send(target, _players, Need(_globals.Boss)(), _hooks.Assertion, _hooks.Broadcast, _hooks.Send);

    /// <summary>실제 연결을 그대로 빌리며 NULL은 선택된 거부 응답까지 유지한다.</summary>
    public void Receive(int pending, TConnection connection, ReadOnlyMemory<byte> packet) => ReceiveDirect(pending, connection, packet);
    /// <summary>004a7800의 실제 CheckLogin 뒤에도 원문 후반을 계속하며 임의 Ready/Begin 성공을 만들지 않는다.</summary>
    public void ReceiveDirect(int pending, TConnection? connection, ReadOnlyMemory<byte> packet)
    {
        if (!Need(_globals.Server)()) { Need(_hooks.Disconnect)(pending); return; }
        uint subscriber = Get(packet.Span, 8), owner = 1;
        // 같은 subscriber의 실제 첫 Player만 선택하고 미발견은0으로 보존한다.
        for (; owner < 9; owner++) if (Read(owner, 0x2c) == subscriber) break; if (owner == 9) owner = 0;
        if (_globals.Player.PlayerKnown == 0)
        {
            // 최초 known 조건을 한 번만 고르며 각 비교의 자기 구독자는 현재값이다.
            for (uint player = 1; player < 9; player++) if (Read(player, 0x2c) == Need(_globals.MySubscriber)())
            {
                _globals.Player.TemplateOwner = player; _globals.Player.BattleOwner = player; _globals.Player.PlayerKnown = 1; Need(_globals.StoreCoreData)(Need(_globals.Metamap)() ? _globals.CachedCoreData : _players.Slice((int)player * 180 + 0x6c, 10));
            }
        }
        if (owner == 0)
        {
            byte[] bytes = Reply(0, 0); if (connection is null) throw new InvalidOperationException("선택된 Login 거부 응답의 실제 연결 미연결"); Need(_hooks.PendingReply)(connection, bytes); return;
        }
        if (Need(_globals.Simulconnect)())
        {
            if (pending < 0) Need(_hooks.Disconnect)((int)owner); else if (pending < 1 || pending > 8) Need(_hooks.Assertion)("FIRST_REAL_PLAYER <= pendingId && pendingId < END_REAL_PLAYERS");
        }
        uint previous = (Read(owner, 0x50) >> 1) & 1;
        if (previous == 0 && (_hooks.RestoreCoreData is not null || !Need(_globals.MissionPost)())) Need(_hooks.RestoreCoreData)(owner);
        _participation.CheckLogin(pending, (int)owner); Need(_hooks.Flush)(); SendBegin(owner); Need(_hooks.Flush)();
        byte[] options = Packet(0x67, 94, 255, 4); options[4] = 0; Put(options, 5, 87, 2); _globals.BattleOptions.Span.CopyTo(options.AsSpan(7)); Need(_hooks.Send)((int)owner, options); if (_counts.Participating() < 2) Need(_hooks.ApplyOptions)(options); Need(_hooks.Flush)();
        if (_globals.FortCount.Value == 0) Need(_hooks.ClearTypes)(); Need(_hooks.Flush)();
        // 같은 현재 구독자 기록마다 실제 Player writer를 호출한다.
        for (uint player = 1; player < 9; player++) if (Read(player, 0x2c) != 0) Need(_hooks.SendPlayer)(player, owner);
        Need(_hooks.Flush)(); Need(_hooks.SendFortMetadata)(owner); Need(_hooks.Flush)();
        if (previous == 0)
        {
            uint size = Get(packet.Span, 13), start = 17 + Get(packet.Span, 12, 1); if (start > packet.Length || size > packet.Length - start) throw new ArgumentOutOfRangeException(nameof(packet), "미션 로그인 Fort image 물리 범위");
            Write(owner, 0x5c, _globals.FortCount.Value); Write(owner, 0x60, _globals.PredictableCount.Value); _globals.FortCount.Value = unchecked(_globals.FortCount.Value + 1); _globals.LastLoginPlayer.Value = owner;
            if (Need(_globals.Server)() && owner == _globals.Player.BattleOwner) Need(_hooks.LoadCurrentFort)(owner); else Need(_hooks.LoadFortImage)(owner, packet.Slice((int)start, (int)size));
        }
        Joined(owner, Get(packet.Span, 8)); SendReply((int)owner, 1);
        if (!Need(_globals.MissionPost)() && !Need(_globals.SavedGame)() && Need(_globals.InBattle)())
        {
            Write(owner, 0x58, _globals.BattleOptions.Span[0x3e + (int)owner] == 1 ? 1u : 0);
            if (Read(owner, 0x64) == 0)
            {
                // 각 실제 지정 뒤 현재 watcher/시작SP 옵션을 다시 읽는다.
                for (uint player = 1; player < 9; player++)
                {
                    int amount = 0;
                    if (_globals.BattleOptions.Span[0x3e + (int)player] != 1)
                    {
                        ReadOnlySpan<int> startingSp = [1000, 2500, 3000]; byte index = _globals.BattleOptions.Span[0x0e]; if (index >= startingSp.Length) throw new ArgumentOutOfRangeException(nameof(packet), "일반 로그인 시작SP 옵션 색인"); amount = startingSp[index] + 4000;
                    }
                    Need(_hooks.SetSp)(player, amount);
                }
            }
        }
        Need(_hooks.Flush)(); if (Need(_globals.Metamap)()) Need(_hooks.Assertion)("!metamap"); SendSp(owner); Need(_hooks.Flush)();
        _globals.ClockStamp.Value = Need(_hooks.Clock)(); int difference = unchecked((int)(_globals.ClockStamp.Value - Need(_globals.BaseClock)())) / 1000;
        // 실제1..8 전체를 요청하며 현재 접속/연결 gate는 송신 장치가 처리한다.
        for (int target = 1; target < 9; target++) { byte[] bytes = Packet(0x84, 8); Put(bytes, 4, unchecked((uint)difference)); Need(_hooks.Send)(target, bytes); }
        Need(_hooks.Flush)(); Write(owner, 0x64, 1);
        // 바깥 fortCount와 안쪽 flags/순서/예측수를 매 원문 위치에서 다시 읽는다.
        for (uint order = 0; unchecked((int)order) < unchecked((int)_globals.FortCount.Value); order++)
        {
            // 새 자기 Fort만 첫 순회에서 제외하며 기존 전송 순서를 합치지 않는다.
            for (uint player = 1; player < 9; player++) if ((Read(player, 0x50, 1) & 2) != 0 && Read(player, 0x5c) == order && (previous != 0 || player != owner)) { Need(_hooks.SendFort)(player, owner, Read(player, 0x60)); Need(_hooks.Flush)(); }
        }
        if (owner != _globals.Player.BattleOwner) Need(_hooks.SynchronizePeer)(owner); Need(_hooks.Flush)();
        if (previous == 0)
        {
            // 두번째 실제 순회는 자기 대상도 포함한 현재 접속자다.
            for (uint target = 1; target < 9; target++) if ((Read(target, 0x50, 1) & 1) != 0)
            {
                if (Read(_globals.LastLoginPlayer.Value, 0x60) != _globals.PredictableCount.Value) Need(_hooks.Assertion)("predictableCount"); Need(_hooks.SendFort)(owner, target, _globals.PredictableCount.Value);
            }
            if (Need(_globals.MissionWorld)()) Need(_hooks.LoadWorld)(); else if (Need(_globals.MissionPost)() || Read(owner, 0x58) == 0) Need(_hooks.MakeBattleIsland)(owner);
        }
        Need(_hooks.SendSettings)(owner); Need(_hooks.Flush)(); Need(_hooks.Flush)(); uint current = _globals.Player.BattleOwner; Write(current, 0x50, Read(current, 0x50) | 0x40); SendReady(owner); Need(_hooks.Flush)();
        if (_counts.Subscribers() != 1) { uint disconnected = _counts.Disconnected(); if (unchecked(_counts.Subscribers() - disconnected) != 1) return; } _globals.Begin.Value = 1;
    }
    /// <summary>63h는 실제 구독자를 찾아 CheckLogin 거부 뒤에도 원문 Joined/정리/Ready 후반을 수행한다.</summary>
    public void ReceiveReconnect(int pending, ReadOnlyMemory<byte> packet)
    {
        if (packet.Length != 8 || packet.Span[1] != 0x63 || Get(packet.Span, 2, 2) != 8) throw new ArgumentException("재접속63h 실제8바이트 입력 오류", nameof(packet));
        Need(_hooks.Log)("Reconnecting " + pending + "\n"); uint owner = 1;
        // flags/서버 권한 관문을 새로 추가하지 않고 subscriber0도 비교한다.
        for (; owner < 9; owner++) if (Get(packet.Span, 4) == Read(owner, 0x2c)) break;
        if (owner == 9) { if (Need(_hooks.CheckingAssertions)()) Need(_hooks.Assertion)("0"); return; }
        if (_participation.CheckLogin(pending, (int)owner)) SendReply((int)owner, 1);
        if (Get(packet.Span, 4) == Need(_globals.MySubscriber)()) Write(owner, 0x50, Read(owner, 0x50) | 0x18);
        Joined(owner, Get(packet.Span, 4)); _globals.FortCount.Value = 0; _globals.LastLoginPlayer.Value = 0; int marker = 0;
        // 최대 counter 대신 앞서 고른 owner번호와 다음+5c를 비교하는 실제 원문 선택이다.
        for (uint player = 1; player < 9; player++) if ((Read(player, 0x50, 1) & 2) != 0)
        {
            _globals.FortCount.Value = unchecked(_globals.FortCount.Value + 1); if (unchecked((int)Read(player, 0x5c)) > marker) { marker = (int)player; _globals.LastLoginPlayer.Value = player; }
        }
        Need(_hooks.CleanupReconnectClients)(); uint local = _globals.Player.BattleOwner; Write(local, 0x50, Read(local, 0x50) | 0x40); SendReady(owner);
        if (_counts.Subscribers() != 1) { uint disconnected = _counts.Disconnected(); if (unchecked(_counts.Subscribers() - disconnected) != 1) return; } _globals.Begin.Value = 1;
    }
}
