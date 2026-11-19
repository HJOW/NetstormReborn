// 원본00416eb0의 현재 목표·지역·지갑·초기 월드 등록 준비를 같은 AI 본체에 적용한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// AI 준비 모듈을 담는 네임스페이스다. 2026-11-19 0.645에서 R4-5 AI 초기화용으로 추가했다.

// 원본이 각 위치에서 다시 읽는 현재 전역의 저장소를 빌린다.
public sealed class AiPreparationGlobals
{
    /// <summary>현재 로컬 소유자다. 효과마다 다시 읽는다. 0.645에서 추가했다.</summary>
    public Func<uint>? LocalOwner { get; set; }

    /// <summary>현재 로컬 소유자 확정 여부다. 0이면 0으로 취급한다. 0.645에서 추가했다.</summary>
    public Func<uint>? PlayerKnown { get; set; }

    /// <summary>현재 지역 없음 번호다. 0.645에서 추가했다.</summary>
    public Func<int>? NoRegion { get; set; }

    /// <summary>현재 무효 그래프를 읽는다. 0.645에서 추가했다.</summary>
    public Func<uint>? InvalidGraph { get; set; }

    /// <summary>현재 게임 시각이다. 데모 기한 계산에만 쓴다. 0.645에서 추가했다.</summary>
    public Func<double>? Now { get; set; }
}

// 기존 실제 월드 모듈과 장치 경계만 공급한다. 도달하지 않은 경계는 요구하지 않는다.
public sealed class AiPreparationHooks
{
    /// <summary>적 목표 없음 진단이다. 반환 뒤 원문처럼 계속한다. 0.645에서 추가했다.</summary>
    public Action? MissingEnemyAssertion { get; set; }

    /// <summary>현재 소유 사제 SID다. 0.645에서 추가했다.</summary>
    public Func<uint, uint>? Priest { get; set; }

    /// <summary>현재 지역 신전 SID다. 0.645에서 추가했다.</summary>
    public Func<int, uint>? Temple { get; set; }

    /// <summary>현재 최대 HP다. 현재 HP는 호출 전에 raw+1a에서 읽는다. 0.645에서 추가했다.</summary>
    public Func<int, int>? MaxHitPoints { get; set; }

    /// <summary>두 raw 좌표 사이 거리다. 발 중심 보정을 추가하지 않는다. 0.645에서 추가했다.</summary>
    public Func<float[], float[], double>? PointDistance { get; set; }

    /// <summary>현재 SID 그래프 번호다. 0.645에서 추가했다.</summary>
    public Func<uint, uint>? Graph { get; set; }

    /// <summary>청크 int를 16으로 곱한 float 사각형이다. 0.645에서 추가했다.</summary>
    public Func<int, float[]>? RegionBounds { get; set; }

    /// <summary>현재 분산 SP 읽기다. 0.645에서 추가했다.</summary>
    public Func<uint, float>? StoredSp { get; set; }

    /// <summary>현재 로컬 SP·같은 AI 지갑·표시 갱신이다. 0.645에서 추가했다.</summary>
    public Action<float>? AddLocalSp { get; set; }

    /// <summary>현재 미션+68이다. 0.645에서 추가했다.</summary>
    public Func<uint>? Demo { get; set; }

    /// <summary>현재 지식 창 열림 판정이다. 0.645에서 추가했다.</summary>
    public Func<bool>? KnowledgeOpen { get; set; }

    /// <summary>현재 루트 직계 차단 창 판정이다. 0.645에서 추가했다.</summary>
    public Func<bool>? BlockingGumpOpen { get; set; }

    /// <summary>전체 초기 월드 등록이다. 원래+25c 위치에서 실행한다. 0.645에서 추가했다.</summary>
    public Action<uint>? InitialScan { get; set; }
}

/// <summary>별도 값형 AI 상태 없이 실제0xb08 본체와 Player9·방향별 동맹을 공유한다.
/// 원본의 적 재선택·데모 기한·취약 사제 목표·SP·최초 등록·현재 수량 판정을 순서대로 실행한다. 게임 난수를 소비하지 않는다.</summary>
/// <remarks>2026-11-19 0.645에서 추가했다. 근거: cpppj <c>o/RawAiPreparation.h/.cpp</c>의 <c>RawAiPreparation</c>(원문 00416eb0)이다.</remarks>
public sealed class RawAiPreparation
{
    /// <summary>원본 Player0~8의 물리 기록 수다. 0.645에서 추가했다.</summary>
    private const int PlayerSlots = 9;

    /// <summary>패치판 Player 한 기록의 실제 바이트 수다. 0.645에서 추가했다.</summary>
    private const int PlayerBytes = 180;

    /// <summary>방향별 동맹 칸 수다. 0.645에서 추가했다.</summary>
    private const int AllianceCells = 81;

    /// <summary>초기 사제 탐색 거리다. 0.645에서 추가했다.</summary>
    private const float NearestDistance = 99999.0f;

    /// <summary>적 청크 중심 배율이다. 0.645에서 추가했다.</summary>
    private const float Half = 0.5f;

    /// <summary>signed 지갑 초과 시 간헐천 부착 충족 기준이다. 0.645에서 추가했다.</summary>
    private const int RichEnough = 12000;

    /// <summary>AI 본체와 17개 목록의 실제 소유 메모리다. 0.645에서 추가했다.</summary>
    private readonly RawAiBrainMemory _memory;

    /// <summary>사제의 현재 HP·좌표를 공급하는 실제 풀이다. 0.645에서 추가했다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>같은9×180바이트의 현재 Player 기록이다. 0.645에서 추가했다.</summary>
    private readonly byte[] _players;

    /// <summary>현재 방향별9×9 동맹표다. 0.645에서 추가했다.</summary>
    private readonly uint[] _allies;

    /// <summary>복사본 대신 현재 전역 참조를 유지한다. 0.645에서 추가했다.</summary>
    private readonly AiPreparationGlobals _globals;

    /// <summary>선택된 기존 실제 하위의 수명을 빌린다. 0.645에서 추가했다.</summary>
    private readonly AiPreparationHooks _hooks;

    /// <summary>AI 준비 본체를 만든다. 같은 PATCH 풀·Player9 기록·동맹81 표를 확인한다.</summary>
    /// <param name="memory">AI 본체·목록의 실제 소유 메모리다.</param>
    /// <param name="pool">사제의 현재 HP·좌표를 읽는 실제 풀이다.</param>
    /// <param name="players">같은9×180바이트의 현재 Player 기록이다.</param>
    /// <param name="allies">현재 방향별9×9 동맹표다.</param>
    /// <param name="globals">현재 전역 묶음이다.</param>
    /// <param name="hooks">실제 월드 효과 묶음이다.</param>
    public RawAiPreparation(RawAiBrainMemory memory, RawSidPool pool, byte[] players, uint[] allies, AiPreparationGlobals globals, AiPreparationHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(players);
        ArgumentNullException.ThrowIfNull(allies);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || players.Length != PlayerSlots * PlayerBytes || allies.Length != AllianceCells)
        {
            throw new ArgumentException("AI 준비 같은 PATCH·Player9·동맹81 계약 누락", nameof(pool));
        }

        _memory = memory;
        _pool = pool;
        _players = players;
        _allies = allies;
        _globals = globals;
        _hooks = hooks;
    }

    /// <summary>강제 대상 또는 첫 비동맹 사제 보유자를 선택한다. 0.645에서 추가했다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    public void SelectEnemy(uint token)
    {
        Field(token, 0x264, 0);
        uint forced = Field(token, 0x240);
        if (forced != 0)
        {
            Field(token, 0x264, forced);
            return;
        }

        // 첫 살아 있는 사제 보유 비동맹 플레이어에서 즉시 끝낸다.
        for (uint owner = 1; owner < PlayerSlots; owner++)
        {
            if (Need(_hooks.Priest)(owner) != 0 && !Allied(Field(token, 0x260), owner))
            {
                Field(token, 0x264, owner);
                return;
            }
        }
    }

    /// <summary>원문은+324 목록을+338 개수만큼 읽는다. 이 불일치를+32c로 교정하지 않는다. 0.645에서 추가했다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    public int CountAttachedGeysers(uint token)
    {
        int attached = 0;
        // graph 하위가 개수·포인터·기준 그래프를 바꿀 수 있으므로 각 원문 위치에서 다시 읽는다.
        for (int index = 0; index < Signed(Field(token, 0x338)); index++)
        {
            byte[] list = _memory.Bytes(Field(token, 0x324));
            uint sid = BinaryPrimitives.ReadUInt32LittleEndian(list.AsSpan(index * 4, 4));
            uint graph = Need(_hooks.Graph)(sid);
            if (graph == Field(token, 0x298))
            {
                attached++;
            }
        }

        return attached;
    }

    /// <summary>원문00416eb0 전체이며 원래 초기 스캔 시점을 보존한다. 0.645에서 추가했다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    public void Update(uint token)
    {
        if (Field(token, 0x264) == 0 || Allied(Field(token, 0x260), Field(token, 0x264)))
        {
            SelectEnemy(token);
        }

        if (Field(token, 0x264) == 0)
        {
            Need(_hooks.MissingEnemyAssertion)();
        }

        if (Need(_hooks.Demo)() != 0 && Field(token, 0x260) == Need(_globals.LocalOwner)()
            && (Need(_hooks.KnowledgeOpen)() || Need(_hooks.BlockingGumpOpen)()))
        {
            double due = (float)Field(token, 0x228) + Need(_globals.Now)();
            ulong bits = BitConverter.DoubleToUInt64Bits(due);
            Field(token, 0x5a8, unchecked((uint)bits));
            Field(token, 0x5ac, unchecked((uint)(bits >> 32)));
        }

        int ownRegion = Signed(Player(Field(token, 0x260), 0x78));
        Field(token, 0x294, ownRegion == Need(_globals.NoRegion)() ? 0u : Need(_hooks.Temple)(ownRegion));
        if ((Field(token, 0x248) & 0x100u) != 0)
        {
            // 참여 DWORD가 비제로인 적 사제 중 절반 HP 미만인 대상을 원문 순서로 살핀다.
            for (uint enemy = 1; enemy < PlayerSlots; enemy++)
            {
                if (Player(enemy, 0x2c) == 0 || Allied(Field(token, 0x260), enemy))
                {
                    continue;
                }

                uint priest = Need(_hooks.Priest)(enemy);
                if (priest == 0)
                {
                    continue;
                }

                int sid = PhysicalSid(priest);
                int hitPoints = Signed(BinaryPrimitives.ReadUInt32LittleEndian(_pool.Slot(sid).Slice(0x1a, 4)));
                int maxHitPoints = Need(_hooks.MaxHitPoints)(sid);
                if (hitPoints >= maxHitPoints / 2)
                {
                    continue;
                }

                if (Field(token, 0x264) == enemy)
                {
                    break;
                }

                float nearest = NearestDistance, nearestOwner = 0;
                // 같은 방향 동맹의 모든 참여 사제를 방문한다. 먼저 가까운 자신을 발견해도 중단하지 않는다.
                for (uint ally = 1; ally < PlayerSlots; ally++)
                {
                    if (Player(ally, 0x2c) == 0 || !Allied(Field(token, 0x260), ally))
                    {
                        continue;
                    }

                    uint friendPriest = Need(_hooks.Priest)(ally);
                    if (friendPriest == 0)
                    {
                        continue;
                    }

                    float[] source = Point(_pool.Slot(PhysicalSid(friendPriest)));
                    float[] target = Point(_pool.Slot(sid));
                    double distance = Need(_hooks.PointDistance)(source, target);
                    if (distance < nearest)
                    {
                        nearest = (float)distance;
                        nearestOwner = ally;
                    }
                }

                if (nearestOwner == Field(token, 0x260))
                {
                    Field(token, 0x264, enemy);
                    break;
                }
            }
        }

        int enemyRegion = Signed(Player(Field(token, 0x264), 0x78));
        Field(token, 0x2a8, enemyRegion == Need(_globals.NoRegion)() ? 0u : Need(_hooks.Temple)(enemyRegion));
        uint ownTemple = Field(token, 0x294);
        uint ownGraph = ownTemple != 0 ? Need(_hooks.Graph)(ownTemple) : Need(_globals.InvalidGraph)();
        Field(token, 0x298, ownGraph);
        Field(token, 0x5f8, ownGraph);
        Field(token, 0x604, Field(token, 0x238));
        Field(token, 0x5fc, Field(token, 0x260));
        Field(token, 0x600, Field(token, 0x220));
        uint enemyTemple = Field(token, 0x2a8);
        Field(token, 0x2ac, enemyTemple != 0 ? Need(_hooks.Graph)(enemyTemple) : Need(_globals.InvalidGraph)());
        int currentEnemyRegion = Signed(Player(Field(token, 0x264), 0x78));
        if (currentEnemyRegion != Need(_globals.NoRegion)())
        {
            float[] bounds = Need(_hooks.RegionBounds)(currentEnemyRegion);
            // 하위가 돌려준 네 float를 원래 연속 필드에 복사한다.
            for (int index = 0; index < bounds.Length; index++)
            {
                Field(token, 0x2b8 + unchecked((uint)index) * 4u, BitConverter.SingleToUInt32Bits(bounds[index]));
            }

            Field(token, 0x2b0, Field(token, 0x2b8));
            Field(token, 0x2b4, Field(token, 0x2bc));
            float left = BitConverter.UInt32BitsToSingle(Field(token, 0x2b8));
            float right = BitConverter.UInt32BitsToSingle(Field(token, 0x2c0));
            float x = (float)((right - left) * Half + BitConverter.UInt32BitsToSingle(Field(token, 0x2b0)));
            Field(token, 0x2b0, BitConverter.SingleToUInt32Bits(x));
            float top = BitConverter.UInt32BitsToSingle(Field(token, 0x2bc));
            float bottom = BitConverter.UInt32BitsToSingle(Field(token, 0x2c4));
            float y = (float)((bottom - top) * Half + BitConverter.UInt32BitsToSingle(Field(token, 0x2b4)));
            Field(token, 0x2b4, BitConverter.SingleToUInt32Bits(y));
        }

        Field(token, 0x29c, Need(_hooks.Priest)(Field(token, 0x260)));
        Field(token, 0x2a0, Need(_hooks.Priest)(Field(token, 0x264)));
        if (Field(token, 0x260) == Need(_globals.LocalOwner)())
        {
            uint known = Need(_globals.PlayerKnown)();
            float current = known != 0 ? Need(_hooks.StoredSp)(Need(_globals.LocalOwner)()) : 0.0f;
            // 원문처럼 double에서 비교한 뒤 float로 절삭한 차이를 가산한다.
            if ((double)current != Signed(Field(token, 0x2d8)))
            {
                float reread = known != 0 ? Need(_hooks.StoredSp)(Need(_globals.LocalOwner)()) : 0.0f;
                float delta = (float)(Signed(Field(token, 0x2d8)) - (double)reread);
                Need(_hooks.AddLocalSp)(delta);
            }
        }

        if (Field(token, 0x25c) == 0)
        {
            Need(_hooks.InitialScan)(token);
            Field(token, 0x25c, 1);
        }

        int attached = CountAttachedGeysers(token);
        uint collectors = attached < 1 ? (Field(token, 0x234) != 0 ? 1u : 0u) : Field(token, 0x234);
        Field(token, 0x2cc, Signed(collectors) <= Signed(Field(token, 0x2e4)) ? 1u : 0u);
        if (Signed(Field(token, 0x32c)) < 2)
        {
            Field(token, 0x2d0, 1);
        }
        else
        {
            int desired = Signed(Field(token, 0x230));
            int minimum = desired > 0 ? desired : 1;
            Field(token, 0x2d0, minimum <= CountAttachedGeysers(token) ? 1u : 0u);
        }

        if (Signed(Field(token, 0x2d8)) > RichEnough)
        {
            Field(token, 0x2d0, 1);
        }
    }

    /// <summary>후속 실제 월드 조립 확인용 동일 풀이다. 0.645에서 추가했다.</summary>
    public RawSidPool Pool() => _pool;

    /// <summary>실제 소유 블록을 매번 다시 확인하며 값형 사본을 만들지 않는다. 0.645에서 추가했다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    private byte[] Body(uint token)
    {
        byte[] raw = _memory.Bytes(token);
        if (raw.Length != RawAiBrainConstructor.BytesCount)
        {
            throw new ArgumentException("AI 준비 실제0xb08 본체 누락", nameof(token));
        }

        return raw;
    }

    /// <summary>콜백 뒤에도 같은 token의 현재 DWORD를 다시 읽는다. 0.645에서 추가했다.</summary>
    private uint Field(uint token, uint offset) => BinaryPrimitives.ReadUInt32LittleEndian(Body(token).AsSpan((int)offset, 4));

    /// <summary>현재 본체의 해당 원문 DWORD에만 쓴다. 0.645에서 추가했다.</summary>
    private void Field(uint token, uint offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(Body(token).AsSpan((int)offset, 4), value);

    /// <summary>손상된 owner만 물리 범위 밖에서 거부하고 0번 기록은 허용한다. 0.645에서 추가했다.</summary>
    private uint Player(uint owner, uint offset)
    {
        if (owner >= PlayerSlots || offset > PlayerBytes - 4)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), "AI 준비 현재 Player 물리 범위");
        }

        return BinaryPrimitives.ReadUInt32LittleEndian(_players.AsSpan((int)owner * PlayerBytes + (int)offset, 4));
    }

    /// <summary>대칭화하지 않고 원본 owner 행의 현재 동맹 칸만 읽는다. 0.645에서 추가했다.</summary>
    private bool Allied(uint owner, uint target)
    {
        if (owner >= PlayerSlots || target >= PlayerSlots)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), "AI 준비 현재 방향별 동맹 범위");
        }

        return _allies[owner * PlayerSlots + target] != 0;
    }

    /// <summary>현재 풀에 실제로 존재하는 물리 번호만 좁힌다. 0.645에서 추가했다.</summary>
    private int PhysicalSid(uint value)
    {
        if (value >= (uint)_pool.Capacity || value > ushort.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "AI 준비 현재 사제 SID 물리 범위");
        }

        return unchecked((int)value);
    }

    /// <summary>현재 사제 raw 좌표를 float 저장값 그대로 읽는다. 0.645에서 추가했다.</summary>
    private static float[] Point(ReadOnlySpan<byte> raw) => [BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(14, 4))), BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(18, 4)))];

    /// <summary>DWORD의 signed 분기에서 구현 의존 변환을 피한다. 0.645에서 추가했다.</summary>
    private static int Signed(uint value) => unchecked((int)value);

    /// <summary>선택된 원본 하위가 없으면 그 지점에서 명시 오류를 낸다. 0.645에서 추가했다.</summary>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("AI 준비의 선택된 실제 하위 미연결");
}
