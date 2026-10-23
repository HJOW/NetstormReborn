using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// GumpForm 호스트(RawGumpFormHost)의 계약을 확인한다.
/// cpppj 에는 이 몸체만 따로 관찰한 x86 fixture 가 없으므로(캠페인 세션이 함께 쓴다)
/// 원본 소스의 쓰기 순서·상태 비트·경계를 합성 입력으로 검사하고, 같은 풀의 실제 생성·삭제 연결까지 확인한다.
/// 기준 판본은 10.78 이다 (W03 삭제·수명 열다섯째 조각, 2026-10-23).
/// </summary>
public sealed class RawGumpFormHostTests
{
    /// <summary>장면 풀의 슬롯 수.</summary>
    private const int PoolCapacity = 32768;

    /// <summary>10.78 GumpForm(타입 3)의 가상 표 값.</summary>
    private const uint GumpVtable = 0x00512188;

    /// <summary>시험용 소유 UI 등록 ID.</summary>
    private const uint OwnerId = 0xabcdef12;

    /// <summary>슬롯의 16비트 값을 리틀 엔디언으로 쓴다.</summary>
    /// <param name="raw">대상 슬롯.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void Put(Span<byte> raw, int offset, ushort value)
    {
        // 지정 폭만 쓰도록 두 바이트만 고친다
        raw[offset] = (byte)value;
        raw[offset + 1] = (byte)(value >> 8);
    }

    /// <summary>슬롯의 32비트 값을 리틀 엔디언으로 쓴다.</summary>
    /// <param name="raw">대상 슬롯.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void PutDword(Span<byte> raw, int offset, uint value)
    {
        // 좌표 원본 바이트 순서를 그대로 보존한다
        for (int index = 0; index < 4; index++)
        {
            raw[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>슬롯의 16비트 값을 리틀 엔디언으로 읽는다.</summary>
    /// <param name="pool">읽을 풀.</param>
    /// <param name="sid">슬롯 번호.</param>
    /// <param name="offset">슬롯 안 시작 위치.</param>
    private static ushort Word(RawSidPool pool, int sid, int offset)
    {
        ReadOnlySpan<byte> raw = pool.Slot(sid);
        return (ushort)(raw[offset] | (raw[offset + 1] << 8));
    }

    /// <summary>슬롯의 32비트 값을 리틀 엔디언으로 읽는다.</summary>
    /// <param name="pool">읽을 풀.</param>
    /// <param name="sid">슬롯 번호.</param>
    /// <param name="offset">슬롯 안 시작 위치.</param>
    private static uint Dword(RawSidPool pool, int sid, int offset)
    {
        ReadOnlySpan<byte> raw = pool.Slot(sid);
        return (uint)(raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24));
    }

    /// <summary>
    /// 실제 풀·팩토리·삭제 본체를 공유하는 장면. 공간 해시에 넣은 자산 뿌리도 만들 수 있다.
    /// </summary>
    private sealed class Scene
    {
        /// <summary>삭제 대상 슬롯 풀(32,768칸, 서버).</summary>
        public readonly RawSidPool Pool = new(PoolCapacity, true);

        /// <summary>공간 해시.</summary>
        public readonly RawSquidHash Hash = new();

        /// <summary>점유 지도(65,536칸, 항상 0).</summary>
        public readonly byte[] Spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];

        /// <summary>타입 표(10.78 188개).</summary>
        public readonly OriginalTypeRecord[] Types = new OriginalTypeRecord[OriginalConstructors.TypeCount];

        /// <summary>자산 뿌리의 공간 해제기.</summary>
        public readonly RawUnpop Unpop;

        /// <summary>삭제 본체.</summary>
        public readonly RawSquidDestroy Destroy;

        /// <summary>내장 Form 생성기.</summary>
        public readonly RawSquidFactory Factory;

        /// <summary>소유 UI 알림 기록.</summary>
        public readonly List<(uint Owner, int Parent)> Notices = [];

        /// <summary>GumpForm 밖 효과 기록.</summary>
        public readonly List<SquidDestroyEvent> FallbackEffects = [];

        /// <summary>현재 고른 번호. 이 장면은 선택을 바꾸지 않으므로 항상 0 이다.</summary>
        public int Selected { get; set; }

        /// <summary>알림 콜백이 함께 지울 GumpForm 번호(0 이면 없음).</summary>
        public int GumpToDestroy;

        /// <summary>검증 대상 호스트.</summary>
        public readonly RawGumpFormHost Gumps;

        /// <summary>알림 경계를 연결한 장면을 만든다.</summary>
        public Scene()
        {
            // 타입 번호 전체에 빈 타입 레코드를 채운다
            for (int index = 0; index < Types.Length; index++)
            {
                Types[index] = new OriginalTypeRecord();
            }
            Unpop = new RawUnpop(Pool, Hash, Spots);
            Destroy = new RawSquidDestroy(Pool, Unpop, Types);
            Factory = new RawSquidFactory(Pool, Types, false, Unpop);
            var fallback = new SquidDestroyHooks
            {
                Selected = () => Selected,
                Emit = effect =>
                {
                    // 자산 경계처럼 사전·사후 깊이를 이 장면이 직접 닫는다
                    if (effect.Effect == SquidDestroyEffect.PreDestroy)
                    {
                        Destroy.CompletePreDestroy();
                    }
                    else if (effect.Effect == SquidDestroyEffect.PostDestroy)
                    {
                        Destroy.CompletePostDestroy();
                    }
                    FallbackEffects.Add(effect);
                },
            };
            Gumps = new RawGumpFormHost(Pool, Factory, Types, Destroy, (owner, parent) =>
            {
                Notices.Add((owner, parent));
                // 원본 세션처럼 알림 안에서 같은 분배기로 Form 을 지운다
                if (GumpToDestroy != 0)
                {
                    // 생성자 안에서 대입 뒤에만 호출되는 경로이므로 현재 인스턴스를 쓴다
                    Gumps!.Destroy(GumpToDestroy, 0x10);
                }
            }, fallback);
        }

        /// <summary>
        /// 공간 해시에 등록한 자산 뿌리를 만든다. 가상 표는 공통 Unpop 표에 있는 기본 값을 쓴다.
        /// </summary>
        /// <param name="x">x 좌표.</param>
        /// <param name="y">y 좌표.</param>
        /// <param name="type">자산 타입 번호(70 이상).</param>
        public int NewAsset(float x, float y, uint type = 90)
        {
            int sid = Pool.Allocate();
            Span<byte> raw = Pool.AllocatedBytes(sid);
            raw.Clear();
            PutDword(raw, 0, OriginalConstructors.BaseVtable);
            raw[10] = (byte)type;
            PutDword(raw, 14, BitConverter.SingleToUInt32Bits(x));
            PutDword(raw, 18, BitConverter.SingleToUInt32Bits(y));
            Types[type].FootX = 1;
            Types[type].FootY = 1;
            // 같은 좌표의 0단계 버킷 체인 머리에 연결한다
            ref ushort head = ref Hash.Bucket(0, x, y);
            Put(raw, 4, head);
            head = (ushort)sid;
            return sid;
        }
    }

    /// <summary>
    /// 기본 생성은 클라이언트 영역의 GumpForm 을 만들고 내장 Form 가상 표·타입·void 비트를 쓴다.
    /// 서버 영역 생성은 flags 0 으로 요구한다.
    /// </summary>
    [Fact]
    public void CreatesClientGumpFormWithInternalVtable()
    {
        var scene = new Scene();
        int form = scene.Gumps.Create();
        Assert.InRange(form, RawSidPool.FirstOrdinary, RawSidPool.ServerFirst - 1);
        Assert.Equal(GumpVtable, Dword(scene.Pool, form, 0));
        Assert.Equal(3, scene.Pool.Slot(form)[10]);
        // 생성 직후에는 void 이고 부모 종속이 아니다
        Assert.Equal(4, scene.Pool.Slot(form)[11] & 0x0c);
        Assert.True(scene.Gumps.IsGumpForm(form));
        Assert.Equal(0u, scene.Gumps.Owner(form));
        int server = scene.Gumps.Create(0);
        Assert.True(server >= RawSidPool.ServerFirst);
        Assert.True(scene.Gumps.IsGumpForm(server));
    }

    /// <summary>
    /// 부착은 부모의 종속 머리 앞에 넣고 void 를 끄며, 두 번째 부착은 첫 항목의 앞에 들어간다.
    /// 10.78 은 부모 0 부착을 조용히 넘긴다.
    /// </summary>
    [Fact]
    public void AttachLinksHeadAndClearsVoid()
    {
        var scene = new Scene();
        int parent = scene.NewAsset(10.5f, 20.5f);
        int first = scene.Gumps.Create();
        int second = scene.Gumps.Create();
        scene.Gumps.Attach(first, parent);
        Assert.Equal(first, Word(scene.Pool, parent, 6));
        Assert.Equal(0, Word(scene.Pool, first, 4));
        Assert.Equal(0, Word(scene.Pool, first, 14));
        Assert.Equal(parent, scene.Gumps.Parent(first));
        Assert.Equal(8, scene.Pool.Slot(first)[11] & 8);
        Assert.Equal(0, scene.Pool.Slot(first)[11] & 4);
        scene.Gumps.Attach(second, parent);
        Assert.Equal(second, Word(scene.Pool, parent, 6));
        Assert.Equal(first, Word(scene.Pool, second, 4));
        Assert.Equal(second, Word(scene.Pool, first, 14));
        Assert.Equal(0, Word(scene.Pool, second, 14));
        // 부모 0 은 10.78 GumpForm 에서 무동작이다
        scene.Gumps.Attach(first, 0);
        Assert.Equal(second, Word(scene.Pool, parent, 6));
    }

    /// <summary>소유 ID 는 DWORD 전체이고, 알림은 호출 인자 대신 현재 부모를 보낸다. 소유 ID 0 은 알리지 않는다.</summary>
    [Fact]
    public void NotifyReportsOwnerAndCurrentParent()
    {
        var scene = new Scene();
        int parent = scene.NewAsset(10.5f, 20.5f);
        int form = scene.Gumps.Create();
        scene.Gumps.SetOwner(form, OwnerId);
        Assert.Equal(OwnerId, scene.Gumps.Owner(form));
        // 부착 전에도 부모 필드를 읽을 수 있다
        scene.Gumps.Notify(form);
        Assert.Equal(new List<(uint, int)> { (OwnerId, 0) }, scene.Notices);
        scene.Gumps.Attach(form, parent);
        scene.Gumps.Notify(form);
        Assert.Equal(new List<(uint, int)> { (OwnerId, 0), (OwnerId, parent) }, scene.Notices);
        int quiet = scene.Gumps.Create();
        scene.Gumps.Notify(quiet);
        Assert.Equal(2, scene.Notices.Count);
    }

    /// <summary>
    /// 부모 자산 삭제는 종속 GumpForm 을 해제·알림·삭제하고, 알림 안에서 같은 Form 을 먼저 지워도
    /// 이미 dead 인 중첩 삭제는 본체가 바로 돌아온다. 두 깊이는 모두 0 으로 돌아와야 한다.
    /// </summary>
    [Fact]
    public void HooksDestroyDependentGumpFormAndNotify()
    {
        var scene = new Scene();
        int parent = scene.NewAsset(10.5f, 20.5f);
        int form = scene.Gumps.Create();
        scene.Gumps.SetOwner(form, OwnerId);
        scene.Gumps.Attach(form, parent);
        scene.GumpToDestroy = form;
        scene.Destroy.Destroy(parent, 0, scene.Gumps.Hooks());
        Assert.Equal(new List<(uint, int)> { (OwnerId, parent) }, scene.Notices);
        // 부모와 GumpForm 모두 실제로 반납됐다
        Assert.Equal(1, scene.Pool.Slot(parent)[11] & 1);
        Assert.Equal(1, scene.Pool.Slot(form)[11] & 1);
        // GumpForm 밖 효과는 서버 뿌리의 사전·전송·사후이며 선택 해제는 없었다
        // (GumpForm 자신은 client 번호라 전송 효과를 내지 않는다)
        Assert.Equal([SquidDestroyEffect.PreDestroy, SquidDestroyEffect.Transmit, SquidDestroyEffect.PostDestroy],
            scene.FallbackEffects.Select(effect => effect.Effect));
        Assert.Equal(0u, scene.Destroy.PreDepth);
        Assert.Equal(0u, scene.Destroy.PostDepth);
    }

    /// <summary>
    /// GumpForm 자신의 삭제에는 소유 UI 알림이 없다(부모 삭제의 종속 알림만 낸다).
    /// </summary>
    [Fact]
    public void DestroyDoesNotNotifyOwner()
    {
        var scene = new Scene();
        int parent = scene.NewAsset(10.5f, 20.5f);
        int form = scene.Gumps.Create();
        scene.Gumps.SetOwner(form, OwnerId);
        scene.Gumps.Attach(form, parent);
        scene.Gumps.Destroy(form, 0x10);
        Assert.Empty(scene.Notices);
        Assert.Equal(1, scene.Pool.Slot(form)[11] & 1);
        // 부모 체인의 머리는 비었고 부모는 그대로 살아 있다
        Assert.Equal(0, Word(scene.Pool, parent, 6));
        Assert.Equal(0, scene.Pool.Slot(parent)[11] & 1);
    }

    /// <summary>
    /// 풀·타입 표·생성/삭제 연결이 어긋나면 만들지 않고, 잘못된 부착·삭제·알림은 슬롯을 바꾸기 전에 거부한다.
    /// 10.78 GumpForm 의 서버 Pop 전송은 미복원이므로 서버 영역 Form 부착은 거부된다.
    /// </summary>
    [Fact]
    public void RejectsInvalidContracts()
    {
        var scene = new Scene();
        int parent = scene.NewAsset(10.5f, 20.5f);
        var otherPool = new RawSidPool(PoolCapacity, true);
        var otherUnpop = new RawUnpop(otherPool, new RawSquidHash(), new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells]);
        var otherDestroy = new RawSquidDestroy(otherPool, otherUnpop, scene.Types);
        Assert.Throws<ArgumentException>(() => new RawGumpFormHost(otherPool, scene.Factory, scene.Types, otherDestroy));
        Assert.Throws<ArgumentException>(() => new RawGumpFormHost(scene.Pool, scene.Factory,
            new OriginalTypeRecord[OriginalConstructors.TypeCount - 1], scene.Destroy));
        int form = scene.Gumps.Create();
        int free = scene.Pool.Allocate();
        scene.Pool.Release(free);
        Assert.Throws<InvalidOperationException>(() => scene.Gumps.Attach(form, free));
        Assert.Throws<InvalidOperationException>(() => scene.Gumps.Attach(form, form));
        // 서버 영역 GumpForm 의 실제 Pop 직렬화·전송은 아직 미복원이므로 부착을 거부한다
        int serverSlot = scene.Pool.Allocate();
        scene.Pool.Release(serverSlot);
        int serverForm = scene.Gumps.Take(serverSlot);
        Assert.Throws<InvalidOperationException>(() => scene.Gumps.Attach(serverForm, parent));
        Assert.Throws<InvalidOperationException>(() => scene.Gumps.Destroy(parent));
        // 알림 경계가 없는 호스트는 소유 ID 가 있는 Form 에서 거부한다
        var silent = new RawGumpFormHost(scene.Pool, scene.Factory, scene.Types, scene.Destroy);
        int owned = silent.Create();
        silent.SetOwner(owned, OwnerId);
        Assert.Throws<InvalidOperationException>(() => silent.Notify(owned));
    }
}
