using System.Buffers.Binary;
using Netstorm.Core.Original;

// 원문 부모 종류별 검증과 실제 Start·현재 경로 조작을 연결한다.
namespace Netstorm.Client.Host;

/// <summary>RawCampaignPathBindings의 실제 명령/조회 입구다.</summary>
public sealed partial class HostSession
{
    /// <summary>원문 할당 슬롯의 물리 범위와 살아 있는 자산을 고른다.</summary>
    private ReadOnlySpan<byte> PathParentBytes(int parent)
    {
        if (parent < 5 || parent >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(parent)); return Pool.AllocatedBytes(parent);
    }
    /// <summary>실제 사제TYPE/가상표/54 역할만 명령으로 허용한다.</summary>
    private void ValidatePathPriest(int parent)
    {
        ReadOnlySpan<byte> raw = PathParentBytes(parent); uint expected = Pool.Edition == OriginalEdition.Patch1078 ? 0x50f210u : 0x5003e0u;
        if (raw[10] != PathStartState.PriestType || BinaryPrimitives.ReadUInt32LittleEndian(raw) != expected || (raw[11] & 9) != 0) throw new ArgumentException("사제 명령 실제 TYPE/가상표/살아 있는 자산 오류");
        if (PriestWorld.MovementProcessType(raw[10]) != 54 || PathProcessState.PriestProcess != 54) throw new InvalidOperationException("사제의 다른 mutable 이동 역할 미복원");
    }
    /// <summary>실제134/105 생성자의 가상표와 현재 이동 역할을 확인한다.</summary>
    private void ValidatePathConstruction(int parent)
    {
        if (Pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("건설 운반 경로의 PATCH 판본 필요");
        ReadOnlySpan<byte> raw = PathParentBytes(parent); uint table = BinaryPrimitives.ReadUInt32LittleEndian(raw); RawPathStartState start = PathStartState; RawPathProcessState process = PathProcessState;
        if ((raw[11] & 9) != 0 || !((raw[10] == start.TeleportType && table == 0x5070c0 && PriestWorld.MovementProcessType(raw[10]) == process.TeleportProcess) || (raw[10] == start.ManaBoltType && table == 0x509810 && PriestWorld.MovementProcessType(raw[10]) == process.FlyProcess))) throw new InvalidOperationException("건설 운반 실제 TYPE/가상표/이동 종류 오류");
    }
    /// <summary>복원한 웜·벌룬·벌프·워커 부모만 현재 역할과 정확한 가상표로 허용한다.</summary>
    private void ValidatePathCarrier(int parent)
    {
        if (Pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("Carrier 경로의 PATCH 판본 필요");
        ReadOnlySpan<byte> raw = PathParentBytes(parent); uint table = BinaryPrimitives.ReadUInt32LittleEndian(raw), movement = PriestWorld.MovementProcessType(raw[10]); RawPathProcessState s = PathProcessState;
        bool worm = raw[10] == s.SunWalkerType && table == 0x50e568 && movement == s.WormProcess;
        bool balloon = (((raw[10] == 111 || raw[10] == 141) && table == 0x5040c0) || (raw[10] == 146 && table == 0x504198)) && movement == s.BalloonProcess;
        bool bulf = raw[10] == 135 && table == 0x503e50 && movement == s.BulfProcess;
        bool walker = ((raw[10] == s.WindWalkerType && table == 0x503d80) || (raw[10] == s.RainWalkerType && table == 0x503f20)) && movement == s.WalkerProcess;
        if ((raw[11] & 9) != 0 || (!worm && !balloon && !bulf && !walker)) throw new InvalidOperationException("Carrier 실제 부모/이동 종류 오류:187 전용 부모 미연결");
    }
    /// <summary>현재70/508cc8·실제24 종류의 살아 있는 Dude 부모를 확인한다.</summary>
    private void ValidatePathDude(int parent)
    {
        if (Pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("Dude 경로의 PATCH 판본 필요");
        ReadOnlySpan<byte> raw = PathParentBytes(parent); RawPathProcessState p = PathProcessState;
        if (raw[10] != p.DudeType || raw[10] != PathStartState.DudeType || BinaryPrimitives.ReadUInt32LittleEndian(raw) != 0x508cc8 || (raw[11] & 9) != 0 || p.DudeProcess != 24 || PriestWorld.MovementProcessType(raw[10]) != p.DudeProcess) throw new InvalidOperationException("Dude24 실제 부모/가상표/이동 종류 오류");
    }
    /// <summary>실제 사제 명령을 같은 Start/BFS/Prepare/Host 부착으로 실행한다.</summary>
    public RawPathProcess? StartPriestPath(RawPathStartRequest request) { _ = PathStarter; ValidatePathPriest(request.Parent); return PathStarter.Start(request); }
    /// <summary>실제 건설 부모 도착을 요구한 뒤 같은 Start를 실행한다.</summary>
    public RawPathProcess? StartConstructionPath(RawPathStartRequest request) { _ = PathStarter; ValidatePathConstruction(request.Parent); _ = FixedPacketNeed(_pathEffects?.ConstructionArrival); return PathStarter.Start(request); }
    /// <summary>복원한 Carrier의 실제 생성/통지/부착을 같은 Start로 실행한다.</summary>
    public RawPathProcess? StartCarrierPath(RawPathStartRequest request) { _ = PathStarter; ValidatePathCarrier(request.Parent); return PathStarter.Start(request); }
    /// <summary>Dude의 두 실제 부모 가상 효과를 요구한 뒤 같은 Start를 실행한다.</summary>
    public RawPathProcess? StartDudePath(RawPathStartRequest request) { _ = PathStarter; ValidatePathDude(request.Parent); _ = FixedPacketNeed(_pathEffects?.DudeHalt); _ = FixedPacketNeed(_pathEffects?.DudeArrival); return PathStarter.Start(request); }
    /// <summary>Source의 현재 부모 분배 뒤 종속 체인과 같은 Host의 실제 Form/PID를 매번 확인한다.</summary>
    private RawPathProcess? CurrentBoundPath(int parent)
    {
        if (parent < 5 || parent >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(parent));
        ReadOnlySpan<byte> raw = Pool.Slot(parent); RawPathStartState start = PathStartState; RawPathProcessState p = PathProcessState; uint movement = 54;
        if (raw[10] != start.PriestType)
        {
            if (raw[10] == start.DudeType) ValidatePathDude(parent);
            else if (raw[10] == p.SunWalkerType || raw[10] == p.EmptyType || raw[10] is 111 or 141 or 146 or 135 || raw[10] == p.WindWalkerType || raw[10] == p.RainWalkerType) ValidatePathCarrier(parent);
            else ValidatePathConstruction(parent);
            movement = PriestWorld.MovementProcessType(raw[10]);
        }
        int form = FixedPacketNeed(_pathParentLookup).Find(parent, movement); if (form == 0) return null;
        if (PlayerProcesses.FromForm(form) is not RawPathProcess current || !ReferenceEquals(current.Pool, Pool) || current.Parent() != parent || current.Form() != form) throw new InvalidOperationException("현재 경로의 실제 Host/Form/부모 계약 오류"); return current;
    }
    /// <summary>현재 같은 경로가 있을 때만 대상 두 DWORD를 쓴다.</summary>
    public bool SetCurrentPathTargets(int parent, uint first, uint second) { _ = PathStarter; RawPathRuntime? current = CurrentPathRuntime(parent); if (current is null) return false; current.SetTargets(first, second); return true; }
    /// <summary>실제 재탐색이 경로를 삭제할 수 있으므로 호출 뒤 포인터를 재사용하지 않는다.</summary>
    public bool RetargetCurrentPath(int parent) { _ = PathStarter; RawPathRuntime? current = CurrentPathRuntime(parent); if (current is null) return false; FixedPacketNeed(_pathEffects?.Retarget)(current, parent); return true; }
    /// <summary>기존 경로를 교체하지 않고 실제 BFS의 임시 후보만 채운다.</summary>
    public bool SearchPathCandidate(RawPathRuntime temporary, int parent, RawPathPoint start, RawPathPoint destination, uint flags) { _ = PathStarter; return PathSearch.Search(temporary, parent, start, destination, flags); }
}
