using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// Form 삭제 분배기(RawSquidFormDispatch)의 계약을 확인한다.
/// cpppj 에는 분배기만 따로 관찰한 x86 fixture 가 없으므로(원본 혼합 체인은 ContentLifetimeTests 가 함께 쓴다)
/// 원본 소스의 분기 순서·중첩 삭제·경계 교체를 합성 입력으로 검사한다.
/// 프로세스 Form(10~62) 등록은 Kernel 프로세스 호스트가 아직 이식되지 않아 장면에서 뺐다.
/// 기준 판본은 10.78 이다 (W03 삭제·수명 열다섯째 조각, 2026-10-23).
/// </summary>
public sealed class RawSquidFormDispatchTests
{
    /// <summary>장면 풀의 슬롯 수.</summary>
    private const int PoolCapacity = 32768;

    /// <summary>시험용 소유 UI 등록 ID. 원본 혼합 체인 시험과 같은 값이다.</summary>
    private const uint OwnerId = 0xabcdef12;

    /// <summary>삭제 뿌리로 쓰는 자산 타입 번호.</summary>
    private const uint RootType = 90;

    /// <summary>감시 쌍의 반대편 부모로 쓰는 자산 타입 번호.</summary>
    private const uint WatcherType = 91;

    /// <summary>슬롯 안 다음 종속 번호(16비트)의 위치.</summary>
    private const int NextOffset = 4;

    /// <summary>슬롯 안 종속 머리(16비트)의 위치.</summary>
    private const int HeadOffset = 6;

    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 상태 바이트의 위치.</summary>
    private const int StateOffset = 11;

    /// <summary>슬롯 안 x 좌표(float 4바이트)의 위치.</summary>
    private const int XOffset = 14;

    /// <summary>슬롯 안 y 좌표(float 4바이트)의 위치.</summary>
    private const int YOffset = 18;

    /// <summary>ContentForm 이 저장하는 내용물 종류(16비트)의 위치.</summary>
    private const int ContentKindOffset = 18;

    /// <summary>상태 바이트의 빈 슬롯 비트.</summary>
    private const byte FreeBit = 1;

    /// <summary>상태 바이트의 죽음(삭제 준비) 비트.</summary>
    private const byte DeadBit = 2;

    /// <summary>상태 바이트의 월드 밖(void) 비트.</summary>
    private const byte VoidBit = 4;

    /// <summary>16비트 값을 리틀 엔디언으로 쓴다.</summary>
    /// <param name="raw">대상 슬롯.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void Put(Span<byte> raw, int offset, ushort value)
    {
        // 지정 폭인 두 바이트만 고쳐 이웃 payload 를 보존한다
        raw[offset] = (byte)value;
        raw[offset + 1] = (byte)(value >> 8);
    }

    /// <summary>32비트 값을 리틀 엔디언으로 쓴다.</summary>
    /// <param name="raw">대상 슬롯.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void PutDword(Span<byte> raw, int offset, uint value)
    {
        // 낮은 바이트부터 네 바이트를 쓴다
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

    /// <summary>
    /// 실제 풀·팩토리·삭제 본체·수명 호스트를 모두 공유하는 혼합 체인 장면(원본 시험과 같은 구성).
    /// 자산 뿌리의 공간 해시는 void 뿌리에서 쓰이지 않지만 원본 장면과 같은 등록 상태를 유지한다.
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

        /// <summary>공통 삭제 본체. 분배기·모든 호스트가 같은 것을 쓴다.</summary>
        public readonly RawSquidDestroy Destroy;

        /// <summary>내장 Form 생성기.</summary>
        public readonly RawSquidFactory Factory;

        /// <summary>자산 공통 효과·선택 경계. SetAssets 시험에서 교체 전 기준으로도 쓴다.</summary>
        public readonly SquidDestroyHooks Assets;

        /// <summary>자산 경계로 나간 효과 이름 기록.</summary>
        public readonly List<string> AssetEffects = [];

        /// <summary>자산 뿌리로 넘어간 삭제 횟수.</summary>
        public int AssetRoots;

        /// <summary>감시 쌍 통지 횟수.</summary>
        public int DependNotices;

        /// <summary>GumpForm 소유 UI 통지 횟수.</summary>
        public int GumpNotices;

        /// <summary>ContentForm 부모 제거 통지 횟수.</summary>
        public int ContentRemovals;

        /// <summary>현재 고른 번호. 자산 경계가 읽는다.</summary>
        public int Selected;

        /// <summary>감시 쌍 시험에서 기대하는 감시 대상 부모 번호.</summary>
        public int RootParent;

        /// <summary>감시 쌍 시험에서 기대하는 감시하는 부모 번호.</summary>
        public int WatcherParent;

        /// <summary>통지 안에서 함께 지울 GumpForm 번호(0 이면 없음).</summary>
        public int GumpForm;

        /// <summary>검증 대상 분배기.</summary>
        public readonly RawSquidFormDispatch Dispatch;

        /// <summary>내용물 부착 처리기.</summary>
        public readonly RawContentFormHost ContentForms;

        /// <summary>내용물 수명.</summary>
        public readonly RawContentLifetime Content;

        /// <summary>GumpForm 처리기.</summary>
        public readonly RawGumpFormHost Gumps;

        /// <summary>감시 쌍 처리기.</summary>
        public readonly RawDependFormHost Depends;

        /// <summary>세 Form 호스트와 내용물 수명을 같은 분배기에 등록한 장면을 만든다.</summary>
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
            // 자산 경계는 선택 해제와 사전·사후 깊이 반납을 이 장면이 직접 처리한다
            Assets = new SquidDestroyHooks
            {
                Selected = () => Selected,
                Emit = OnAssetEffect,
            };
            Dispatch = new RawSquidFormDispatch(Pool, Destroy, Assets, DestroyAsset);
            ContentForms = new RawContentFormHost(Pool, Types, OnContentNotice);
            // 내용물 수명의 파생 분배는 같은 분배기로 돌려 Form 종류별 효과를 섞는다
            Content = new RawContentLifetime(Pool, Types, ContentForms, Destroy, new ContentLifetimeState(),
                new ContentLifetimeHooks(), Dispatch.Hooks());
            Dispatch.Bind(Content);
            Gumps = new RawGumpFormHost(Pool, Factory, Types, Destroy, OnGumpNotice, Dispatch.Hooks());
            Dispatch.Bind(Gumps);
            Depends = new RawDependFormHost(Pool, Destroy, OnDependNotice, Dispatch.Hooks());
            Dispatch.Bind(Depends);
        }

        /// <summary>
        /// 자산 뿌리를 만들고 같은 좌표의 0단계 버킷 체인에 연결한다. 좌표·발자국은 원본 공간 규칙과 같은 값을 쓴다.
        /// </summary>
        /// <param name="type">자산 타입 번호(70 이상).</param>
        /// <param name="x">x 좌표.</param>
        /// <param name="y">y 좌표.</param>
        public int NewAsset(uint type, float x, float y)
        {
            int sid = Factory.Create(type, SidAllocation.Client);
            Span<byte> raw = Pool.AllocatedBytes(sid);
            PutDword(raw, XOffset, BitConverter.SingleToUInt32Bits(x));
            PutDword(raw, YOffset, BitConverter.SingleToUInt32Bits(y));
            Types[type].FootX = 1;
            Types[type].FootY = 1;
            // 같은 좌표의 0단계 버킷 체인 머리에 연결한다
            ref ushort head = ref Hash.Bucket(0, x, y);
            Put(raw, NextOffset, head);
            head = (ushort)sid;
            return sid;
        }

        /// <summary>자산 경계로 나가는 효과를 이름으로 기록하고 필요한 깊이를 닫는다.</summary>
        /// <param name="effect">나간 효과.</param>
        private void OnAssetEffect(SquidDestroyEvent effect)
        {
            switch (effect.Effect)
            {
                case SquidDestroyEffect.PreDestroy:
                    Destroy.CompletePreDestroy();
                    AssetEffects.Add("pre");
                    break;
                case SquidDestroyEffect.PostDestroy:
                    Destroy.CompletePostDestroy();
                    AssetEffects.Add("post");
                    break;
                case SquidDestroyEffect.ClearSelection:
                    Selected = 0;
                    AssetEffects.Add("clear");
                    break;
                case SquidDestroyEffect.Transmit:
                    AssetEffects.Add("transmit");
                    break;
                default:
                    // Form 종류별 효과는 각 호스트가 처리하므로 자산 경계에 오면 안 된다
                    throw new InvalidOperationException("자산 경계에 Form 효과가 도달했습니다");
            }
        }

        /// <summary>자산 뿌리의 가상 삭제 재정의. 횟수를 세고 같은 공통 본체로 지운다.</summary>
        /// <param name="sid">지울 뿌리 번호.</param>
        /// <param name="flags">삭제 플래그.</param>
        /// <param name="hooks">분배기가 합성한 현재 훅.</param>
        private bool DestroyAsset(int sid, uint flags, SquidDestroyHooks hooks)
        {
            AssetRoots++;
            Destroy.Destroy(sid, flags, hooks);
            return true;
        }

        /// <summary>ContentForm 부모 제거 통지만 센다(부착 통지는 세지 않는다).</summary>
        /// <param name="parent">부모 번호.</param>
        /// <param name="form">부착·제거된 Form 번호.</param>
        /// <param name="reason">1 = 부착, 2 = 제거.</param>
        private void OnContentNotice(int parent, int form, uint reason)
        {
            if (reason == 2)
            {
                ContentRemovals++;
            }
        }

        /// <summary>GumpForm 소유 UI 통지를 세고, 통지 안에서 같은 분배기로 GumpForm 을 지운다.</summary>
        /// <param name="owner">Gump 등록 ID.</param>
        /// <param name="parent">통지 시점의 Form 부모 번호.</param>
        private void OnGumpNotice(uint owner, int parent)
        {
            Assert.Equal(OwnerId, owner);
            Assert.Equal(RootParent, parent);
            GumpNotices++;
            Dispatch.Destroy(GumpForm, 0x10);
        }

        /// <summary>감시 쌍 통지를 세고, 통지 안에서 같은 분배기로 감시 Form 을 지운다.</summary>
        /// <param name="watcher">감시하는 부모 번호.</param>
        /// <param name="watched">감시 대상 부모 번호.</param>
        /// <param name="form">알림을 보내는 감시 Form 번호.</param>
        private void OnDependNotice(int watcher, int watched, int form)
        {
            Assert.Equal(WatcherParent, watcher);
            Assert.Equal(RootParent, watched);
            DependNotices++;
            Dispatch.Destroy(form, 0x10);
        }
    }

    /// <summary>
    /// 한 부모의 감시·Gump·Content 가 자기 통지 안에서 삭제돼도 같은 풀·같은 공통 깊이·선저장 next 를 공유한다.
    /// 자산 뿌리는 가상 삭제 재정의를 한 번 거치고, 모든 Form 과 뿌리가 같은 삭제 기록으로 반납된다.
    /// </summary>
    [Fact]
    public void MixedChainDestroysEveryFormThroughDispatch()
    {
        var scene = new Scene();
        int root = scene.NewAsset(RootType, 20.75f, 21.9f);
        int watcher = scene.NewAsset(WatcherType, 30.75f, 31.9f);
        scene.RootParent = root;
        scene.WatcherParent = watcher;
        scene.Selected = root;
        int value = scene.Factory.CreateContained(2);
        Span<byte> valueRaw = scene.Pool.AllocatedBytes(value);
        Put(valueRaw, ContentKindOffset, 80);
        scene.ContentForms.Attach(value, root, 0x10);
        int gump = scene.Gumps.Create(2);
        scene.GumpForm = gump;
        scene.Gumps.SetOwner(gump, OwnerId);
        scene.Gumps.Attach(gump, root);
        DependFormPair pair = scene.Depends.Create(root, watcher);

        Assert.True(scene.Dispatch.Destroy(root, 0x10));
        Assert.Equal(1, scene.AssetRoots);
        // 선택 해제는 자산 경계로 나가고 현재 선택이 비워진다
        Assert.Equal(0, scene.Selected);
        Assert.Equal(1, scene.DependNotices);
        Assert.Equal(1, scene.GumpNotices);
        // 부모가 이미 dead 이므로 ContentForm 부모 제거 통지는 나가지 않는다
        Assert.Equal(0, scene.ContentRemovals);
        // 감시 대상이 사라진 watcher 부모의 체인은 비었고 watcher 자신은 살아 있다
        Assert.Equal(0, Word(scene.Pool, watcher, HeadOffset));
        Assert.Equal(0, scene.Pool.Slot(watcher)[StateOffset] & FreeBit);
        // 모든 Form 과 자산 뿌리가 같은 풀에서 반납됐다(죽음·빈 비트)
        foreach (int sid in new[] { root, value, gump, pair.WatchedForm, pair.WatcherForm })
        {
            Assert.Equal(FreeBit | DeadBit, scene.Pool.Slot(sid)[StateOffset] & (FreeBit | DeadBit));
        }
        Assert.Equal(0u, scene.Destroy.PreDepth);
        Assert.Equal(0u, scene.Destroy.PostDepth);
        // 자산 경계에는 뿌리의 사전·선택 해제·사후만 도달한다
        Assert.Equal(new[] { "pre", "clear", "post" }, scene.AssetEffects);
    }

    /// <summary>
    /// 분배기와 다른 풀의 본체·호스트는 만들거나 등록하지 않고, 같은 타입의 중복 등록도 슬롯을 바꾸기 전에 거부한다.
    /// </summary>
    [Fact]
    public void RejectsWrongPoolDuplicateAndIncompleteRegistration()
    {
        var scene = new Scene();
        var otherPool = new RawSidPool(PoolCapacity, true);
        var otherUnpop = new RawUnpop(otherPool, new RawSquidHash(),
            new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells]);
        var otherDestroy = new RawSquidDestroy(otherPool, otherUnpop, scene.Types);
        // 분배기와 다른 풀의 공통 삭제 본체는 만들지 않는다
        Assert.Throws<ArgumentException>(() => new RawSquidFormDispatch(scene.Pool, otherDestroy, scene.Assets));
        // 자산 공통 경계에는 효과·선택 콜백이 모두 있어야 한다
        Assert.Throws<ArgumentException>(() => new RawSquidFormDispatch(scene.Pool, scene.Destroy,
            new SquidDestroyHooks { Emit = _ => { } }));
        Assert.Throws<ArgumentException>(() => new RawSquidFormDispatch(scene.Pool, scene.Destroy,
            new SquidDestroyHooks { Selected = () => 0 }));
        // 다른 풀의 GumpForm 호스트는 등록하지 않는다
        var otherGumps = new RawGumpFormHost(otherPool, new RawSquidFactory(otherPool, scene.Types), scene.Types, otherDestroy);
        Assert.Throws<ArgumentException>(() => scene.Dispatch.Bind(otherGumps));
        // 같은 타입은 두 번 등록할 수 없다
        Assert.Throws<InvalidOperationException>(() => scene.Dispatch.Bind(scene.Gumps));
        Assert.Throws<InvalidOperationException>(() => scene.Dispatch.Bind(scene.Content));
        Assert.Throws<InvalidOperationException>(() => scene.Dispatch.Bind(scene.Depends));
    }

    /// <summary>
    /// SetAssets 는 등록된 Form 훅이 읽는 현재 자산 경계를 통째로 바꾸며, 불완전한 교체는 거부하고 기존 경계를 유지한다.
    /// </summary>
    [Fact]
    public void SetAssetsReplacesCurrentBoundaryForRegisteredHooks()
    {
        var scene = new Scene();
        int root = scene.NewAsset(RootType, 20.75f, 21.9f);
        scene.Selected = root;
        int gump = scene.Gumps.Create(2);
        scene.Gumps.Attach(gump, root);
        int swapped = 0;
        var effects = new List<string>();
        var replacement = new SquidDestroyHooks
        {
            // 교체 뒤에 만든 훅은 이 선택 조회를 쓴다
            Selected = () => gump,
            Emit = effect =>
            {
                switch (effect.Effect)
                {
                    case SquidDestroyEffect.PreDestroy:
                        scene.Destroy.CompletePreDestroy();
                        effects.Add("pre");
                        break;
                    case SquidDestroyEffect.PostDestroy:
                        scene.Destroy.CompletePostDestroy();
                        effects.Add("post");
                        break;
                    case SquidDestroyEffect.ClearSelection:
                        effects.Add("clear");
                        break;
                    default:
                        effects.Add(effect.Effect.ToString());
                        break;
                }
            },
        };
        scene.Dispatch.SetAssets(replacement, (sid, flags, hooks) =>
        {
            swapped++;
            scene.Destroy.Destroy(sid, flags, hooks);
            return true;
        });
        // Form 삭제는 새 자산 효과 경계와 새 선택 조회를 쓴다. 사전·사후는 GumpForm 훅이 직접 닫으므로
        // 자산 경계에는 선택 해제만 도달한다
        scene.Dispatch.Destroy(gump, 0x10);
        Assert.Equal(new[] { "clear" }, effects);
        Assert.Equal(0, swapped);
        Assert.Equal(FreeBit | DeadBit, scene.Pool.Slot(gump)[StateOffset] & (FreeBit | DeadBit));
        // 자산 뿌리는 새 가상 삭제 재정의로 보낸다
        Assert.True(scene.Dispatch.Destroy(root));
        Assert.Equal(1, swapped);
        Assert.Equal(0u, scene.Destroy.PreDepth);
        Assert.Equal(0u, scene.Destroy.PostDepth);
        // 불완전한 자산 경계 교체는 거부하고 기존 경계를 그대로 둔다
        Assert.Throws<ArgumentException>(() => scene.Dispatch.SetAssets(new SquidDestroyHooks { Selected = () => 0 }, null));
        int second = scene.NewAsset(WatcherType, 30.75f, 31.9f);
        Assert.True(scene.Dispatch.Destroy(second));
        Assert.Equal(2, swapped);
        Assert.Equal(0u, scene.Destroy.PreDepth);
        Assert.Equal(0u, scene.Destroy.PostDepth);
    }

    /// <summary>
    /// 자산 뿌리의 가상 삭제 재정의가 거짓을 돌려주면 분배기도 그 거부를 보존하며, 슬롯·효과·깊이를 바꾸지 않는다.
    /// Form 뿌리는 자산 재정의를 거치지 않으므로 같은 거부 상태에서도 정상 삭제된다.
    /// </summary>
    [Fact]
    public void AssetDestroyRejectionIsPreserved()
    {
        var scene = new Scene();
        int root = scene.NewAsset(RootType, 20.75f, 21.9f);
        int attempts = 0;
        scene.Dispatch.SetAssets(scene.Assets, (sid, flags, hooks) =>
        {
            attempts++;
            return false;
        });
        Assert.False(scene.Dispatch.Destroy(root));
        Assert.Equal(1, attempts);
        // 거부된 삭제는 죽음·빈 비트와 깊이·효과를 바꾸지 않는다
        Assert.Equal(0, scene.Pool.Slot(root)[StateOffset] & (FreeBit | DeadBit));
        Assert.Equal(VoidBit, scene.Pool.Slot(root)[StateOffset] & VoidBit);
        Assert.Empty(scene.AssetEffects);
        Assert.Equal(0u, scene.Destroy.PreDepth);
        Assert.Equal(0u, scene.Destroy.PostDepth);
        // Form 뿌리는 자산 재정의를 거치지 않고 공통 본체로 지운다
        int parent = scene.NewAsset(WatcherType, 30.75f, 31.9f);
        int gump = scene.Gumps.Create(2);
        scene.Gumps.Attach(gump, parent);
        Assert.True(scene.Dispatch.Destroy(gump, 0x10));
        Assert.Equal(1, attempts);
        Assert.Equal(FreeBit | DeadBit, scene.Pool.Slot(gump)[StateOffset] & (FreeBit | DeadBit));
        // 재정의가 승인하면 같은 뿌리를 정상 삭제한다
        scene.Dispatch.SetAssets(scene.Assets, (sid, flags, hooks) =>
        {
            scene.Destroy.Destroy(sid, flags, hooks);
            return true;
        });
        Assert.True(scene.Dispatch.Destroy(root));
        Assert.Equal(FreeBit | DeadBit, scene.Pool.Slot(root)[StateOffset] & (FreeBit | DeadBit));
        Assert.Equal(0u, scene.Destroy.PreDepth);
        Assert.Equal(0u, scene.Destroy.PostDepth);
    }
}
