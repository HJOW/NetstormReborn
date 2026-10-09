using Netstorm.Assets;

namespace Netstorm.Core.Bridges;

/// <summary>수명 감소 한 번의 결과 (원본 00421c30). 실제 삭제·그림 전환·이동체 확인은 하지 않고 조건만 알린다.</summary>
/// <param name="Word">새 상태 단어. 제거 경로에서는 원본처럼 이전 단어 그대로다</param>
/// <param name="Previous">이전 수명</param>
/// <param name="Remaining">새 수명</param>
/// <param name="Remove">칸을 없애는지 (서버이고 새 수명이 0)</param>
/// <param name="RemovalFlags">제거에 넘기는 플래그. J·K 판자는 <see cref="BridgeDecayRules.PlankRemoval"/></param>
/// <param name="Crack">금 간 프레임으로 바꾸려 하는지 (제거가 아니고 이전 수명 ≥ 5, 새 수명 &lt; 5)</param>
/// <param name="Carriers">칸 위 이동체를 확인하는지 (제거가 아니고 새 수명 ≤ 5)</param>
public readonly record struct BridgeLifeChange(ushort Word, int Previous, int Remaining, bool Remove, uint RemovalFlags,
    bool Crack, bool Carriers);

/// <summary>프레임 전환 결과. Changed 가 거짓이면 프레임은 그대로다.</summary>
/// <param name="Word">새 상태 단어</param>
/// <param name="Frame">새 프레임 번호</param>
/// <param name="Changed">프레임을 찾아 바꿨는지</param>
public readonly record struct BridgeFrameChange(ushort Word, int Frame, bool Changed);

/// <summary>
/// 붕괴 처리가 읽는 원본 실행 상태 전역. 기본값은 싱글 플레이(로컬이 서버) 전투 화면이다.
/// </summary>
public sealed record BridgeDecayMode
{
    /// <summary>싱글 플레이 전투의 값 (서버·권한 있음, 편집기 아님, 디버그 꺼짐)</summary>
    public static BridgeDecayMode SinglePlayer { get; } = new();

    /// <summary>서버인지 (원본 00540bc0). 꺼져 있으면 수명 0 에서도 없애지 않고 비트만 쓴다</summary>
    public bool Server { get; init; } = true;

    /// <summary>권한이 있는지 (원본 00540bc4). 스캔 실행과 다리 destroy 재정의의 조건이다</summary>
    public bool Authority { get; init; } = true;

    /// <summary>편집기인지 (원본 005c85a4). 편집기에서는 스캔을 돌리지 않는다</summary>
    public bool Editor { get; init; }

    /// <summary>디버그: 금이 가지 않은 수명 변화에도 화면을 갱신한다 (원본 005453ac)</summary>
    public bool DebugRedraw { get; init; }

    /// <summary>디버그: 큰 그래프의 다리 destroy 를 막는다 (원본 0054db80)</summary>
    public bool DebugKeep { get; init; }
}

/// <summary>한 칸 처리가 읽고 바꾸는 다리 오브젝트의 상태</summary>
public sealed record BridgeDecayState
{
    /// <summary>오브젝트 번호</summary>
    public required int Id { get; init; }

    /// <summary>상태 단어 (원본 Squid +0xc). 수명 비트 밖의 비트는 보존한다</summary>
    public ushort Word { get; set; }

    /// <summary>다리 타입의 프레임 번호</summary>
    public int Frame { get; set; }

    /// <summary>이 오브젝트가 속한 그래프의 표면 수 (원본 00462c80 의 레코드)</summary>
    public short GraphSurfaces { get; init; }

    /// <summary>그래프 번호가 정상인지. 254(무효)면 원본은 NULL 레코드를 읽는다</summary>
    public bool GraphValid { get; init; } = true;

    /// <summary>추가 상태 바이트 (abstract 1, buried 8)</summary>
    public byte Extra { get; init; }

    /// <summary>죽음 비트</summary>
    public bool Dead { get; set; }
}

/// <summary>한 칸 처리가 밖으로 내는 효과의 종류. 실제 삭제·그리기·소리·낙하는 호출자가 한다.</summary>
public enum BridgeDecayEventKind
{
    /// <summary>기본 destroy 로 넘어갔다 (원본 004af780). Flags 는 넘긴 인자다</summary>
    Destroy,
    /// <summary>화면 갱신 (금 간 프레임 전환 또는 디버그 갱신)</summary>
    Redraw,
    /// <summary>그 칸 위의 이동체 확인</summary>
    Carriers,
    /// <summary>시작 칸 위치에서 bridgeCrack.wav</summary>
    CrackSound,
}

/// <summary>한 칸 처리의 효과 하나</summary>
/// <param name="Kind">종류</param>
/// <param name="Id">대상 오브젝트 번호</param>
/// <param name="Flags">destroy 인자 (그 밖에는 0)</param>
public readonly record struct BridgeDecayEvent(BridgeDecayEventKind Kind, int Id, uint Flags);

/// <summary>한 칸 처리가 어느 분기로 끝났는지</summary>
public enum BridgeDecayOutcome
{
    /// <summary>시작 칸의 그래프 번호가 254: 원본은 NULL 레코드를 읽어 예외가 나고 스캔이 삼킨다. 아무것도 하지 않는다</summary>
    InvalidGraph,
    /// <summary>그래프 표면 수가 5 미만: 그 칸을 destroy 한다</summary>
    SmallGraph,
    /// <summary>단단한 프레임: 아무것도 하지 않는다</summary>
    Hard,
    /// <summary>이웃 없음: 수명 1 감소</summary>
    Isolated,
    /// <summary>열린 방향 없음</summary>
    Closed,
    /// <summary>방문 뒤 붕괴 진행 플래그가 꺼져 있음</summary>
    Blocked,
    /// <summary>방문 목록의 수명을 목표 값으로 맞췄다</summary>
    Decayed,
    /// <summary>접합 고리 등으로 방문 목록이 불완전하다 (새 코드의 안전 중단). 상태를 바꾸지 않는다</summary>
    Incomplete,
    /// <summary>없앨 칸의 그래프가 254: 원본 예외 지점에서 멈춘다</summary>
    Faulted,
}

/// <summary>한 칸 처리의 결과</summary>
/// <param name="Outcome">끝난 분기</param>
/// <param name="Lowest">방문 목록의 0 이 아닌 최소 수명 (없으면 0). <see cref="BridgeDecayOutcome.Decayed"/> 에서만 뜻이 있다</param>
/// <param name="Target">맞춘 목표 수명</param>
/// <param name="Events">효과 (원본 호출 순서)</param>
/// <param name="States">입력 순서 그대로의 상태 사본 (바뀐 값 반영)</param>
public sealed record BridgeDecayCell(BridgeDecayOutcome Outcome, int Lowest, int Target, IReadOnlyList<BridgeDecayEvent> Events,
    IReadOnlyList<BridgeDecayState> States);

/// <summary>
/// 다리 수명 감소·프레임 전환·destroy 재정의·스캔의 한 칸 처리. 원본 10.78 의 00421c30·00421b60·00421bd0·00421db0·
/// 00421e60·004220f0·004227e0 을 옮겼다 (docs/exe/cpp-bridge-reconstruction.md, cpp-bridgedecay-reconstruction.md.
/// 기준 구현은 cpppj/src/o/Bridge.cpp). 표면 스냅샷과 상태 목록을 입력으로 받는 계산이며 월드에는 아직 연결하지 않았다.
/// 검증은 원본 기계어 기대값 bridge-x86.tsv(Life)와 bridgedecay-x86.tsv 로 한다.
/// </summary>
public static class BridgeDecayRules
{
    /// <summary>상태 단어에서 수명이 차지하는 비트 3~6</summary>
    public const ushort LifeMask = 0x78;

    /// <summary>수명 비트의 시작 위치</summary>
    public const int LifeShift = 3;

    /// <summary>정상 수명의 최댓값 (원본 brMAX_TIME_LEFT). 원본 디버그 검사도 이 범위를 요구한다</summary>
    public const int MaximumLife = 7;

    /// <summary>J·K 판자를 없앨 때 넘기는 플래그</summary>
    public const uint PlankRemoval = 0x02000000;

    /// <summary>수명이 이 값 아래로 처음 내려가면 금 간 프레임으로 바꾼다 (원본 0052f960)</summary>
    public const int CrackLife = 5;

    /// <summary>수명이 이 값 이하가 되면 칸 위 이동체를 확인한다 (원본 0052f95c)</summary>
    public const int CarrierLife = 5;

    /// <summary>그래프 표면 수가 이 값보다 작은 다리는 곧바로 없앤다 (원본 004227e0)</summary>
    public const int MinimumGraphSurfaces = 5;

    /// <summary>금 간 번호는 보통 번호 + 10 이다</summary>
    public const int CrackedNumberOffset = 10;

    /// <summary>단단한 프레임의 번호는 20 이상이다</summary>
    public const int HardNumber = 20;

    /// <summary>추가 상태 바이트의 abstract(1)·buried(8) 비트</summary>
    public const byte AbstractOrBuried = 9;

    /// <summary>한 칸 처리가 열린 방향 검사에 넘기는 이웃 목록의 용량 (원본 지역 배열 10칸. 개수는 전체를 센다)</summary>
    public const int NeighborListCapacity = 10;

    /// <summary>
    /// 수명을 줄인다 (원본 00421c30 의 앞부분). 새 수명은 0 아래로 내려가지 않는다.
    /// 서버이고 새 수명이 0 이면 제거 경로이며 이때 상태 단어는 쓰지 않는다.
    /// 수명이 7 을 넘는 입력·결과는 원본 디버그 검사 대상이므로 예외로 거부한다.
    /// </summary>
    /// <param name="word">상태 단어</param>
    /// <param name="reduction">줄일 양 (음수면 늘어난다)</param>
    /// <param name="server">서버인지</param>
    /// <param name="side">현재 프레임의 방향 글자</param>
    public static BridgeLifeChange ReduceLife(ushort word, int reduction, bool server, char side)
    {
        int previous = (word & LifeMask) >> LifeShift;
        long remaining = Math.Max(0L, (long)previous - reduction);
        if (previous > MaximumLife || remaining > MaximumLife || !BridgeDirections.IsLetter(side))
        {
            throw new ArgumentOutOfRangeException(nameof(word), word, "다리 수명 또는 방향 글자가 범위를 벗어났습니다");
        }
        bool remove = server && remaining == 0;
        ushort next = remove ? word : (ushort)((word & ~LifeMask) | ((int)remaining << LifeShift));
        bool plank = side is 'J' or 'K';
        return new BridgeLifeChange(next, previous, (int)remaining, remove,
            remove && plank ? PlankRemoval : 0u,
            !remove && remaining < CrackLife && previous >= CrackLife,
            !remove && remaining <= CarrierLife);
    }

    /// <summary>
    /// 금 간 프레임을 찾는다 (원본 00421b60): 같은 방향·변형 'P'·번호 + 10·금 간 플래그의 첫 프레임. 없으면 −1.
    /// 이미 금 갔거나 단단한 프레임(번호 10 이상)은 −1 이다. force 가 참이면 단단한 프레임(번호 20 이상)을 번호 1 로 보고 찾는다.
    /// </summary>
    /// <param name="frames">다리 타입의 프레임 표</param>
    /// <param name="frame">현재 프레임 번호</param>
    /// <param name="force">약화(원본 00421db0)의 규칙을 쓸지</param>
    public static int CrackedFrame(TypeFrameTable frames, int frame, bool force)
    {
        int number = FrameNumber(frames, frame);
        if (force && number >= HardNumber)
        {
            number = 1;
        }
        else if (number >= CrackedNumberOffset)
        {
            return -1;
        }
        // 번호는 바이트 덧셈이라 하위 8비트만 검색에 쓰인다
        return frames.Find(frames.Codes[frame].Side, TypeFrameTable.DefaultVariant, (byte)(number + CrackedNumberOffset),
            (uint)FrameCodeFlags.Cracked);
    }

    /// <summary>보통 프레임을 찾는다 (원본 00421bd0): 단단한 번호는 1 로, 금 간 번호는 10 을 뺀 번호로 찾는다. 없으면 −1.</summary>
    /// <param name="frames">다리 타입의 프레임 표</param>
    /// <param name="frame">현재 프레임 번호</param>
    public static int NormalFrame(TypeFrameTable frames, int frame)
    {
        int number = FrameNumber(frames, frame);
        if (number >= HardNumber)
        {
            number = 1;
        }
        else if (number >= CrackedNumberOffset)
        {
            number -= CrackedNumberOffset;
        }
        return frames.Find(frames.Codes[frame].Side, TypeFrameTable.DefaultVariant, (byte)number);
    }

    /// <summary>약화 (원본 00421db0): 수명을 4 로 쓴 뒤 금 간 프레임을 force 규칙으로 찾는다. 프레임을 못 찾아도 수명은 바뀐다.</summary>
    /// <param name="frames">다리 타입의 프레임 표</param>
    /// <param name="word">상태 단어</param>
    /// <param name="frame">현재 프레임 번호</param>
    public static BridgeFrameChange Weaken(TypeFrameTable frames, ushort word, int frame)
    {
        ushort next = (ushort)((word & ~LifeMask) | ((CrackLife - 1) << LifeShift));
        int found = CrackedFrame(frames, frame, true);
        return new BridgeFrameChange(next, found == -1 ? frame : found, found != -1);
    }

    /// <summary>복구 (원본 00421e60): 수명 비트를 지운 뒤 보통 프레임으로 되돌린다.</summary>
    /// <param name="frames">다리 타입의 프레임 표</param>
    /// <param name="word">상태 단어</param>
    /// <param name="frame">현재 프레임 번호</param>
    public static BridgeFrameChange Restore(TypeFrameTable frames, ushort word, int frame)
    {
        ushort next = (ushort)(word & ~LifeMask);
        int found = NormalFrame(frames, frame);
        return new BridgeFrameChange(next, found == -1 ? frame : found, found != -1);
    }

    /// <summary>
    /// 다리 destroy 재정의 (원본 004220f0): 기본 destroy 로 넘어가면 참.
    /// 편집기가 아니고 권한이 있고 abstract·buried 가 아니며 그래프 표면 수가 5 이상일 때,
    /// 단단한 프레임이나 디버그 유지는 destroy 를 막는다. 그래서 큰 그래프의 단단한 다리는 수명 0 에서도 남는다.
    /// </summary>
    /// <param name="mode">실행 상태</param>
    /// <param name="extra">추가 상태 바이트</param>
    /// <param name="graphSurfaces">그래프 표면 수</param>
    /// <param name="frameFlags">현재 프레임의 플래그</param>
    public static bool DestroyProceeds(BridgeDecayMode mode, byte extra, short graphSurfaces, FrameCodeFlags frameFlags)
    {
        if (!mode.Editor && mode.Authority && (extra & AbstractOrBuried) == 0 && graphSurfaces >= MinimumGraphSurfaces
            && ((frameFlags & FrameCodeFlags.Hard) != 0 || mode.DebugKeep))
        {
            return false;
        }
        return true;
    }

    /// <summary>
    /// 수명 감소 전체를 오브젝트 하나에 적용한다 (원본 00421c30): 제거(destroy 재정의 포함) 또는 수명 비트 갱신 →
    /// 금 간 프레임 전환/디버그 갱신 → 이동체 확인. 효과는 <paramref name="events"/> 끝에 원본 순서로 붙인다.
    /// 금 간 프레임을 찾지 못하면 화면 갱신도 하지 않는다.
    /// 반환값이 거짓이면 그래프 254 인 칸의 destroy 에서 원본 예외 지점에 닿은 것이다 (상태는 그대로다).
    /// </summary>
    /// <param name="frames">다리 타입의 프레임 표</param>
    /// <param name="state">바꿀 상태</param>
    /// <param name="reduction">줄일 양</param>
    /// <param name="mode">실행 상태</param>
    /// <param name="events">효과 목록</param>
    public static bool ApplyLife(TypeFrameTable frames, BridgeDecayState state, int reduction, BridgeDecayMode mode,
        List<BridgeDecayEvent> events)
    {
        FrameNumber(frames, state.Frame);
        BridgeLifeChange change = ReduceLife(state.Word, reduction, mode.Server, frames.Codes[state.Frame].Side);
        if (change.Remove)
        {
            return DestroyBridge(frames, state, change.RemovalFlags, mode, events);
        }
        state.Word = change.Word;
        if (change.Crack)
        {
            int cracked = CrackedFrame(frames, state.Frame, false);
            if (cracked != -1)
            {
                state.Frame = cracked;
                events.Add(new BridgeDecayEvent(BridgeDecayEventKind.Redraw, state.Id, 0));
            }
        }
        else if (mode.DebugRedraw)
        {
            events.Add(new BridgeDecayEvent(BridgeDecayEventKind.Redraw, state.Id, 0));
        }
        if (change.Carriers)
        {
            events.Add(new BridgeDecayEvent(BridgeDecayEventKind.Carriers, state.Id, 0));
        }
        return true;
    }

    /// <summary>
    /// 스캔이 다리 한 칸에 하는 처리 (원본 004227e0). 원본의 호출 순서를 그대로 따른다.
    /// 1. 그래프 표면 수가 5 미만이면 그 칸을 destroy 한다.
    /// 2. 단단한 프레임이면 아무것도 하지 않는다.
    /// 3. 이웃이 없으면 수명을 1 줄인다.
    /// 4. 열린 방향이 없으면 끝낸다 (이웃 목록의 앞 10개만 이어진 것으로 본다).
    /// 5. 방문 목록을 만들고 붕괴가 진행되면 목록의 수명을 "0 이 아닌 최솟값 − 1"(없으면 7)로 맞춘다.
    ///    그 시점에 죽지 않은 칸만 바꾸며 중복 항목은 두 번째부터 감소량이 0 이다.
    /// 6. 목표 수명이 5 미만이고 최솟값이 5 이상이었으면 금 가는 소리를 낸다.
    /// 실제 삭제는 하지 않는다. 기본 destroy 로 넘어간 칸은 죽음 비트를 켜고 효과로 알린다.
    /// </summary>
    /// <param name="surfaces">처리 시작 시점의 표면 스냅샷</param>
    /// <param name="frames">다리 타입의 프레임 표</param>
    /// <param name="states">방문될 수 있는 모든 다리의 상태 (시작 칸 포함). 사본에만 반영한다</param>
    /// <param name="root">시작 칸 번호</param>
    /// <param name="mode">실행 상태 (없으면 싱글 플레이)</param>
    public static BridgeDecayCell DecayCell(SurfaceFinder surfaces, TypeFrameTable frames, IEnumerable<BridgeDecayState> states,
        int root, BridgeDecayMode? mode = null)
    {
        mode ??= BridgeDecayMode.SinglePlayer;
        List<BridgeDecayState> copies = states.Select(state => state with { }).ToList();
        var events = new List<BridgeDecayEvent>();
        SurfaceObject item = surfaces.Object(root);
        if ((item.Flags2 & TypeFlagBits.Bridge) == 0)
        {
            throw new ArgumentException("붕괴 처리의 시작 칸이 다리가 아닙니다", nameof(root));
        }
        BridgeDecayState self = Find(copies, root);
        if (!self.GraphValid)
        {
            return new BridgeDecayCell(BridgeDecayOutcome.InvalidGraph, 0, 0, events, copies);
        }
        if (self.GraphSurfaces < MinimumGraphSurfaces)
        {
            bool destroyed = DestroyBridge(frames, self, 0, mode, events);
            return new BridgeDecayCell(destroyed ? BridgeDecayOutcome.SmallGraph : BridgeDecayOutcome.Faulted, 0, 0, events, copies);
        }
        FrameNumber(frames, self.Frame);
        if ((frames.Codes[self.Frame].Flags & FrameCodeFlags.Hard) != 0)
        {
            return new BridgeDecayCell(BridgeDecayOutcome.Hard, 0, 0, events, copies);
        }
        IReadOnlyList<int> neighbors = surfaces.Neighbors(root);
        if (neighbors.Count == 0)
        {
            bool applied = ApplyLife(frames, self, 1, mode, events);
            return new BridgeDecayCell(applied ? BridgeDecayOutcome.Isolated : BridgeDecayOutcome.Faulted, 0, 0, events, copies);
        }
        int[] listed = neighbors.Take(NeighborListCapacity).ToArray();
        if (BridgeSurfaceRules.OpenDirection(frames.Codes[self.Frame].Side, item.X, item.Y, surfaces.Map, listed) == -1)
        {
            return new BridgeDecayCell(BridgeDecayOutcome.Closed, 0, 0, events, copies);
        }
        BridgeDecayWalk walk = BridgeSurfaceRules.CollectDecay(surfaces, root);
        if (!walk.Complete)
        {
            return new BridgeDecayCell(BridgeDecayOutcome.Incomplete, 0, 0, events, copies);
        }
        if (!walk.CanDecay)
        {
            return new BridgeDecayCell(BridgeDecayOutcome.Blocked, 0, 0, events, copies);
        }
        int lowest = int.MaxValue;
        // 방문 목록에서 0 이 아닌 수명의 최솟값을 찾는다
        foreach (int id in walk.Visited)
        {
            int life = (Find(copies, id).Word & LifeMask) >> LifeShift;
            if (life != 0 && life <= lowest)
            {
                lowest = life;
            }
        }
        bool none = lowest == int.MaxValue;
        int clamped = none ? 0 : Math.Min(lowest, MaximumLife);
        int target = none ? MaximumLife : clamped - 1;
        // 방문 순서대로, 그 시점에 죽지 않은 칸만 목표 수명으로 맞춘다
        foreach (int id in walk.Visited)
        {
            BridgeDecayState state = Find(copies, id);
            if (state.Dead)
            {
                continue;
            }
            if (!ApplyLife(frames, state, ((state.Word & LifeMask) >> LifeShift) - target, mode, events))
            {
                return new BridgeDecayCell(BridgeDecayOutcome.Faulted, clamped, target, events, copies);
            }
        }
        if (!none && target < CrackLife && clamped >= CrackLife)
        {
            events.Add(new BridgeDecayEvent(BridgeDecayEventKind.CrackSound, root, 0));
        }
        return new BridgeDecayCell(BridgeDecayOutcome.Decayed, clamped, target, events, copies);
    }

    /// <summary>프레임 번호를 부호 있는 8비트로 읽는다 (원본 movsx). 표 밖의 프레임은 예외다.</summary>
    private static int FrameNumber(TypeFrameTable frames, int frame)
    {
        if (frame < 0 || frame >= frames.Codes.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(frame), frame, "다리 프레임 번호가 표를 벗어났습니다");
        }
        return (sbyte)(byte)frames.Codes[frame].Number;
    }

    /// <summary>번호로 상태를 찾는다. 없는 번호는 호출자의 입력 누락이다.</summary>
    private static BridgeDecayState Find(List<BridgeDecayState> states, int id) =>
        states.Find(state => state.Id == id) ?? throw new KeyNotFoundException($"다리 붕괴 상태가 없습니다: {id}");

    /// <summary>
    /// 다리의 destroy (원본 다리 vtable +0x10, 004220f0): 막히지 않으면 기본 destroy 가 죽음 비트를 켠다.
    /// 그래프 254 인 칸은 재정의가 NULL 레코드를 읽는 조건에서 거짓을 돌려준다 (원본 예외 지점).
    /// </summary>
    private static bool DestroyBridge(TypeFrameTable frames, BridgeDecayState state, uint flags, BridgeDecayMode mode,
        List<BridgeDecayEvent> events)
    {
        FrameNumber(frames, state.Frame);
        if (!mode.Editor && mode.Authority && (state.Extra & AbstractOrBuried) == 0 && !state.GraphValid)
        {
            return false;
        }
        if (!DestroyProceeds(mode, state.Extra, state.GraphValid ? state.GraphSurfaces : (short)0, frames.Codes[state.Frame].Flags))
        {
            return true;
        }
        state.Dead = true;
        events.Add(new BridgeDecayEvent(BridgeDecayEventKind.Destroy, state.Id, flags));
        return true;
    }
}

/// <summary>붕괴 스캔의 커서와 주기 (원본 00422490·00422bc0 의 상태). First~Last 는 양 끝을 포함한다.</summary>
public sealed class BridgeDecayScanState
{
    /// <summary>훑는 번호 범위의 첫 번호</summary>
    public int First { get; init; }

    /// <summary>훑는 번호 범위의 마지막 번호</summary>
    public int Last { get; init; }

    /// <summary>다음에 훑을 번호</summary>
    public int Cursor { get; set; }

    /// <summary>지금 주기가 끝나는 게임 시각(초)</summary>
    public double Next { get; set; }
}

/// <summary>
/// 다리 붕괴 스캔의 커서 진행 (원본 00422bc0 의 바깥 루프). 번호 범위를 10초에 걸쳐 나누어 훑는다.
/// 한 칸 처리는 호출자가 구간의 번호마다 <see cref="Eligible"/> 을 확인해 <see cref="BridgeDecayRules.DecayCell"/> 로 한다.
/// 오브젝트 번호 할당기(계획 4-2)가 월드에 연결되기 전에는 게임에서 쓰지 않는다.
/// </summary>
public static class BridgeDecayScan
{
    /// <summary>스캔 주기(초) (원본 0052f968)</summary>
    public const float Period = 10.0f;

    /// <summary>10.78 의 첫 번호: 서버 번호 영역의 시작 (원본 00422490)</summary>
    public const int FirstId = 15000;

    /// <summary>10.78 의 마지막 번호: 예측 머리 0x59d9</summary>
    public const int LastId = 23001;

    /// <summary>추가 상태 바이트의 abstract 비트</summary>
    private const byte Abstract = 1;

    /// <summary>상태 바이트의 free(1)·dead(2)·void(4) 비트</summary>
    private const byte FreeDeadOrVoid = 7;

    /// <summary>스캔을 초기화한다 (원본 00422490): 범위와 커서를 정하고 첫 주기의 끝 시각을 잡는다.</summary>
    /// <param name="now">게임 시각(초)</param>
    public static BridgeDecayScanState Reset(double now) =>
        new() { First = FirstId, Last = LastId, Cursor = FirstId, Next = (double)Period + now };

    /// <summary>
    /// 이번 프레임에 훑을 번호 구간 [Begin, End) 을 정하고 커서를 옮긴다.
    /// 정지·편집기·권한 없음이면 아무것도 하지 않는다. 주기의 남은 시간이 양수면 trunc(delta ÷ 주기 × 범위)개,
    /// 아니면 범위 전체만큼 (끝 번호까지) 전진한다. 주기가 지났으면 커서를 처음으로 되돌리고 다음 시각을
    /// "지금 + 주기"로 잡는다 (이전 예정 시각 기준이 아니다).
    /// </summary>
    /// <param name="state">스캔 상태</param>
    /// <param name="now">게임 시각(초)</param>
    /// <param name="delta">이번 프레임의 경과 시간(초)</param>
    /// <param name="paused">게임이 정지 중인지</param>
    /// <param name="mode">실행 상태 (없으면 싱글 플레이)</param>
    public static (int Begin, int End) Advance(BridgeDecayScanState state, double now, double delta, bool paused,
        BridgeDecayMode? mode = null)
    {
        mode ??= BridgeDecayMode.SinglePlayer;
        if (paused || mode.Editor || !mode.Authority)
        {
            return (state.Cursor, state.Cursor);
        }
        int range = state.Last - state.First;
        // 남은 시간이 0 보다 클 때만 비례 개수를 쓴다. 0 이하이거나 NaN 이면 범위 전체다
        int count = state.Next - now > 0.0 ? Ftol(delta / (double)Period * range) : range;
        int begin = state.Cursor;
        int end = begin;
        if (count > 0 && begin <= state.Last)
        {
            long available = (long)state.Last - begin + 1;
            end = begin + (int)Math.Min(count, available);
        }
        state.Cursor = end;
        if (now >= state.Next)
        {
            state.Cursor = state.First;
            state.Next = (double)Period + now;
        }
        return (begin, end);
    }

    /// <summary>스캔이 한 칸 처리를 부르는 조건: free·dead·void 가 아니고 다리이며 abstract 가 아니다.</summary>
    /// <param name="state">상태 바이트</param>
    /// <param name="flags2">타입 플래그 2</param>
    /// <param name="extra">추가 상태 바이트</param>
    public static bool Eligible(byte state, uint flags2, byte extra) =>
        (state & FreeDeadOrVoid) == 0 && (flags2 & TypeFlagBits.Bridge) != 0 && (extra & Abstract) == 0;

    /// <summary>원본 CRT _ftol: 0 쪽으로 자른 64비트 정수의 하위 32비트. 범위 밖과 NaN 은 0 이다.</summary>
    private static int Ftol(double value)
    {
        if (!(value > -9223372036854775808.0 && value < 9223372036854775808.0))
        {
            return 0;
        }
        return unchecked((int)(long)value);
    }
}
