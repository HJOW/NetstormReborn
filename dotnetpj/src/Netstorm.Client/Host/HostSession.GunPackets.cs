using System.Buffers.Binary;
using Netstorm.Core.Original;

// 같은 실제 Host의 Gun owner/원문 장치/상태 패킷 수신과 전송 수명을 연결한다.
namespace Netstorm.Client.Host;

/// <summary>RawCampaignSession.cpp의0046a0c0과 RawCampaignAssetBindings의 Gun 부분이다.</summary>
public sealed partial class HostSession
{
    /// <summary>같은 실제 Pool/TYPE/현재 상태를 읽는 Gun 본체다.</summary>
    private RawGunProcess? _gunProcess;
    /// <summary>Source Impl처럼 같은 세션 동안 주소를 유지하는 현재 Gun 상태다.</summary>
    private GunProcessState? _gunProcessState;
    /// <summary>현재 writer 종류를 공유하는 같은 Host 전송 분배다.</summary>
    private ProcessFormTransmission? _gunPacketTransmission;
    /// <summary>이후 실제 PostPop/전투 연결도 같은 Gun 본체를 빌린다. 반납 뒤는 실패한다.</summary>
    public RawGunProcess GunBody => FixedPacketNeed(_gunProcess);
    /// <summary>현재 실제 권한/역할/checking/방향별 동맹을 빌리는 Gun owner다.</summary>
    public GunProcessState GunState => FixedPacketNeed(_gunProcessState);

    /// <summary>현재 프레임/수신 직전 시각과 Player 모드만 준비한다. 권한과 역할은 복사하지 않는다.</summary>
    private void PrepareGunFrame()
    {
        double now = PlayerProcessClock.Now;
        if (!double.IsFinite(now) || Gameplay.Player.BattleOwner > 8) throw new ArgumentOutOfRangeException(nameof(now), "Gun 현재 clock/Player owner 오류");
        GunState.Now = now; GunState.FortMode = Gameplay.Player.FortMode;
    }

    /// <summary>원문 현재 부모 체인에서 발사/회전 역할과 처음 일치하는 AnimationForm만 반환한다.</summary>
    private ushort GunAnimationForm(ushort parent)
    {
        ushort sid = BinaryPrimitives.ReadUInt16LittleEndian(Pool.Slot(parent)[6..]); int visited = 0;
        // 현재 다음 링크/역할을 매번 읽고 손상 체인의 무한 반복만 실제 풀 범위에서 거부한다.
        while (sid != 0)
        {
            if (sid >= Pool.Capacity || ++visited >= Pool.Capacity) throw new InvalidOperationException("Gun 현재 Animation Form 체인 오류");
            ReadOnlySpan<byte> raw = Pool.Slot(sid);
            if (raw[10] == PlayerProcessClock.FireAnimationType() || raw[10] == PlayerProcessClock.RotateAnimationType()) return sid;
            sid = BinaryPrimitives.ReadUInt16LittleEndian(raw[4..]);
        }
        return 0;
    }

    /// <summary>현재 Host가 소유한 같은 객체 이미지인지 확인하고 감시/재전송에 쓴다.</summary>
    private GunProcess CurrentGun(GunProcessFields fields)
    {
        if (PlayerProcesses.FromForm(fields.Form()) is not GunProcess process || !ReferenceEquals(process.Fields, fields))
            throw new InvalidOperationException("Gun 현재 Host/raw 이미지 불일치");
        return process;
    }

    /// <summary>큰 효과 함수는 값 복사하고 Core의 현재 감시/Host/체인/난수를 원문 장치로 덮는다.</summary>
    private void BindGunProcessDevices()
    {
        if (_gunProcess is not null) return;
        RawSurfaceStorage storage = FixedPacketNeed(_squidPacketStorage);
        _ = _worldDependencies ?? BuildDependencyWorldDevices();
        GunProcessHooks input = _squidPacketEffects?.Gun?.Copy() ?? new();
        var state = new GunProcessState
        {
            SharedAuthority = Gameplay.Authority, SharedWindArcherType = GameAux.WindArcherTypeBox,
            SharedThunderCannonType = GameAux.ThunderCannonTypeBox, SharedRainCannonType = GameAux.RainCannonTypeBox,
            AllianceRead = (owner, target) => Alliances[owner * 9 + target],
            Checking = new(() => AiChecking.Read()), FriendlyFire = new(() => GameAux.MissionSettings[3]),
        };
        var hooks = new GunProcessHooks
        {
            RemoveDependency = (watcher, watched) => WorldDependencies.Remove(watcher, watched, 0),
            AddDependency = (watched, fields) => { _ = WorldDependencies.Create(watched, CurrentGun(fields).Form()); },
            RepopForm = fields => PlayerProcesses.RefreshForm(CurrentGun(fields)), AnimationForm = GunAnimationForm,
            KillAnimation = (form, flags) =>
            {
                SquidProcess? process = PlayerProcesses.FromForm(form); byte type = Pool.Slot(form)[10];
                if (process is null || (type != PlayerProcessClock.FireAnimationType() && type != PlayerProcessClock.RotateAnimationType())) throw new InvalidOperationException("Gun Animation 현재 Host 오류");
                PlayerProcesses.Kill(process, flags);
            },
            ParentBlocked = parent => (Pool.Slot(parent)[Pool.Edition == OriginalEdition.Patch1078 ? 40 : 35] & 0x20) != 0,
            RandomBetween = (low, high) => (double)Game.Next(65536) * (1.0 / 65536.0) * Math.Abs((double)high - low) + low,
            RefreshRange = sid => FixedPacketNeed(input.RefreshRange)(sid), TargetingMode = (sid, owner) => FixedPacketNeed(input.TargetingMode)(sid, owner),
            FindTarget = (x, y, type, mode, owner) => FixedPacketNeed(input.FindTarget)(x, y, type, mode, owner),
            StartAnimation = (stage, form, target) => FixedPacketNeed(input.StartAnimation)(stage, form, target),
            ValidateIslandOwners = parent => FixedPacketNeed(input.ValidateIslandOwners)(parent), CheckForms = parent => FixedPacketNeed(input.CheckForms)(parent),
            ShotContext = parent => FixedPacketNeed(input.ShotContext)(parent), Shoot = (context, target) => FixedPacketNeed(input.Shoot)(context, target), FiringInterval = parent => FixedPacketNeed(input.FiringInterval)(parent),
            Attach = (_, _) => throw new InvalidOperationException("Gun 단독 Construct 대신 실제 CreateGunProcess 필요"), Assertion = storage.AssertDisplay,
        };
        _gunProcessState = state; _gunProcess = new(Pool, FixedPacketNeed(_terrainPopTypes), state, hooks);
        PrepareGunFrame(); BindGunPacketTransmission(storage);
    }

    /// <summary>Native는 같은 장기2048BYTE/현재 헤더, 독립 typed는 원문의64BYTE 예약과 최초 헤더를 쓴다.</summary>
    private void BindGunPacketTransmission(RawSurfaceStorage storage)
    {
        ProcessFormTransmission other = PlayerProcesses.Transmission()?.Clone() ?? new(); bool writingGun = false;
        byte[] buffer = storage.UsesNativeTransmissionDevices() ? GameAux.SquidTransmission.Buffer : new byte[64];
        // 실제 Native 분배와 독립 typed writer의 서로 다른 기존 헤더 계약을 유지한다.
        int Write(GunProcess process, bool changed, uint force) => storage.UsesNativeTransmissionDevices()
            ? storage.SerializeProcessForm(process, buffer, changed, force) : process.WritePacket(buffer, changed, force != 0);
        _gunPacketTransmission = new()
        {
            WriteData = (process, changed) => { writingGun = process is GunProcess; return process is GunProcess gun ? Write(gun, changed, 0) : FixedPacketNeed(other.WriteData)(process, changed); },
            WriteFullData = (process, force) => { writingGun = process is GunProcess; return process is GunProcess gun ? Write(gun, false, force) : FixedPacketNeed(other.WriteFullData)(process, force); },
            Packet = length =>
            {
                if (!writingGun) return FixedPacketNeed(other.Packet)(length);
                if (length < 0 || length > buffer.Length) throw new ArgumentOutOfRangeException(nameof(length), "Gun 실제 작성 버퍼 범위");
                return buffer.AsSpan(0, length).ToArray();
            },
            Transmit = length =>
            {
                if (!writingGun) { FixedPacketNeed(other.Transmit)(length); return; }
                if (storage.UsesNativeTransmissionDevices()) { storage.BroadcastProcessForm(length); return; }
                if (length < 0 || length > buffer.Length) throw new ArgumentOutOfRangeException(nameof(length), "Gun 실제 packet 송신 길이");
                FixedPacketNeed(_squidPacketEffects?.FormPacket)(buffer.AsMemory(0, length));
            },
        };
        PlayerProcesses.SetFormTransmit(_gunPacketTransmission);
    }

    /// <summary>0046a0c0은 최초 Form/free를 보존하고 진단 뒤 현재 패킷/TYPE/PID를 사용한다.</summary>
    private void ReceiveGunPacket(ReadOnlyMemory<byte> packet)
    {
        if (packet.Length < 9) throw new ArgumentOutOfRangeException(nameof(packet), "Gun 수신의 실제9BYTE 머리 부족");
        uint number = BinaryPrimitives.ReadUInt32LittleEndian(packet.Span[4..]);
        if (number == 0 || number > ushort.MaxValue || number >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(packet), "Gun 수신의 명시 Form DWORD 물리 범위");
        int form = (int)number; bool free = (Pool.Slot(form)[11] & 1) != 0;
        // 원문 파일/줄의 진단이 정상 복귀한 뒤에도 최초 Form만 유지한다.
        void Diagnose(string expression, uint line) => FixedPacketNeed(_squidPacketStorage).AssertDisplay(expression, @"c:\ns\o\_p_gunprocessdata.cpp", line);
        if (free)
        {
            if (packet.Span[8] != 0xff) Diagnose("*(( char *)((char *)zacket+8) ) == (char)0xFFFFFFFF", 0x4e);
            PrepareGunFrame(); RawGunProcess logic = GunBody; var pending = new GunProcess(logic, new()); bool transferred = false;
            try
            {
                logic.InitializeReceived(pending.Fields); logic.ReadData(pending.Fields, packet, out uint parent);
                if (parent > ushort.MaxValue || parent >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(packet), "Gun 수신의 DWORD 부모 SID 물리 범위");
                transferred = true; _ = PlayerProcesses.Attach(pending, GameAux.GunProcessTypeBox.Value, (int)parent, 0x10, form); return;
            }
            finally { if (!transferred) pending.Dispose(); }
        }
        if ((packet.Span[8] & 1) != 0) Diagnose("(*(char *)(( (char *)zacket+8) ) & 1) == 0", 0x56);
        GunProcess current = PlayerProcesses.FromForm(form) as GunProcess ?? throw new InvalidOperationException("Gun 수신의 현재 PID/outer 객체 미발견");
        PrepareGunFrame(); GunBody.ReadData(current.Fields, packet, out _);
    }

    /// <summary>Kernel/Host 정리가 끝난 뒤 현재 Gun owner/전송 참조만 반납한다.</summary>
    private void ReleaseGunProcessDevices() { _gunProcess = null; _gunProcessState = null; _gunPacketTransmission = null; }
}
