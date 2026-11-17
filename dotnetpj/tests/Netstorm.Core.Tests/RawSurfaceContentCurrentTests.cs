// Native 장기 Content 장치와 정렬(Form) 본체의 장기 수명 결선을 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>Content 장치 설치·상태 공유·정렬 본체·통지 분배의 선택 계약을 확인한다.</summary>
public sealed class RawSurfaceContentCurrentTests
{
    /// <summary>현재 TYPE/프레임/셰이프와 원문 생성자를 갖는 독립 장기 물리다.</summary>
    private static Netstorm.Core.Original.RawSurfaceStorage Storage()
    {
        var types = Enumerable.Range(0, 188).Select(index => new Netstorm.Core.Original.OriginalTypeRecord
        {
            ConstructorAddress = Netstorm.Core.Original.OriginalConstructors.Address(index),
            Flags1 = Netstorm.Core.Original.OriginalTypeRecord.HasHitPoints,
            MaxHitPoints = 1001,
            FootX = 1,
            FootY = 1,
        }).ToArray();
        var frames = Enumerable.Range(0, 188).Select(_ => new Netstorm.Core.Original.RiftTypeFrames([])).ToArray();
        var shapes = Enumerable.Range(0, 188).Select(_ => new Netstorm.Core.Original.SquidDisplayShape()).ToArray();
        return new Netstorm.Core.Original.RawSurfaceStorage(Netstorm.Core.Original.OriginalEdition.Patch1078, types, frames, shapes);
    }

    /// <summary>실제 방송 장치만 있는 Native 전송 묶음이다.</summary>
    private static void BindTransmission(Netstorm.Core.Original.RawSurfaceStorage storage)
    {
        storage.BindTransmissionDevices(new Netstorm.Core.Original.RawSurfaceTransmissionDevices
        {
            Broadcast = _ => { },
            Connected = _ => false,
            Send = (_, _) => { },
        });
    }

    /// <summary>실제 별칭 주소와 네 필수 효과를 묶은 Content 장치다.</summary>
    private static Netstorm.Core.Original.RawSurfaceContentDevices Devices(
        Netstorm.Core.Original.UIntBox? cursor = null,
        Netstorm.Core.Original.UIntBox? local = null,
        Netstorm.Core.Original.UIntBox? content = null) => new()
        {
            Cursor = cursor ?? new Netstorm.Core.Original.UIntBox() { Value = 0 },
            LocalOwner = local ?? new Netstorm.Core.Original.UIntBox() { Value = 1 },
            ContentType = content ?? new Netstorm.Core.Original.UIntBox() { Value = 6 },
            Effects = new Netstorm.Core.Original.ContentLifetimeHooks
            {
                CancelCursor = () => { },
                LocalNotice = _ => { },
                RemoteNotice = _ => { },
                Sound = (_, _) => { },
            },
        };

    /// <summary>실제 방송 장치 결합과 네 필수 효과 뒤에만 Content 설치를 허용한다.</summary>
    [Fact]
    public void BindRequiresNativeAndFourEffects()
    {
        var storage = Storage();
        // 전송 장치 없이 설치하면 거부한다.
        Assert.False(storage.UsesNativeContentDevices());
        Assert.Throws<ArgumentException>(() => storage.BindContentDevices(Devices()));
        BindTransmission(storage);
        storage.BindContentDevices(Devices());
        Assert.True(storage.UsesNativeContentDevices());
        // 두 번째 설치는 같은 장기 수명을 바꾸지 않는다.
        Assert.Throws<ArgumentException>(() => storage.BindContentDevices(Devices()));
        // 필수 항목이 하나라도 없으면 설치하지 않는다.
        var missing = Storage();
        BindTransmission(missing);
        Assert.Throws<ArgumentException>(() => missing.BindContentDevices(new Netstorm.Core.Original.RawSurfaceContentDevices()));
        // 이미 빌린 물리에서는 설치하지 않는다.
        var borrowed = Storage();
        BindTransmission(borrowed);
        using var lease = borrowed.AcquirePrepared();
        Assert.Throws<ArgumentException>(() => borrowed.BindContentDevices(Devices()));
    }

    /// <summary>장치 TYPE이 없으면 독립 내용물 번호를 읽고 있으면 장치 값을 읽는다.</summary>
    [Fact]
    public void ContentTypeFallsBackToContained()
    {
        var storage = Storage();
        // 장치 없이 독립 번호 6을 돌려준다.
        Assert.Equal(6u, storage.ContentType());
        BindTransmission(storage);
        var box = new Netstorm.Core.Original.UIntBox() { Value = 9 };
        storage.BindContentDevices(Devices(content: box));
        Assert.Equal(9u, storage.ContentType());
        // 장치 상자를 바꾸면 다음 조회에 반영한다.
        box.Value = 11;
        Assert.Equal(11u, storage.ContentType());
    }

    /// <summary>상태는 같은 boss/너겟/커서 주소를 별칭으로 빌린다.</summary>
    [Fact]
    public void ContentStateSharesLongTermAddresses()
    {
        var storage = Storage();
        BindTransmission(storage);
        var cursor = new Netstorm.Core.Original.UIntBox() { Value = 77 };
        var local = new Netstorm.Core.Original.UIntBox() { Value = 2 };
        storage.BindContentDevices(Devices(cursor: cursor, local: local));
        Netstorm.Core.Original.ContentLifetimeState state = storage.ContentState();
        // 수입 명령 초기값은 원문 0x020000b4다.
        Assert.Equal(0x020000b4u, state.Notice);
        // 로컬 소유자는 같은 상자를 빌린다.
        Assert.Same(local, state.SharedLocalOwner);
        Assert.Equal(2u, state.ReadLocalOwner());
        // 너겟은 같은 GameAux 주소를 빌린다.
        Assert.Same(storage.GameAux().NuggetTypeBox, state.SharedNuggetType);
        Assert.Equal(154u, state.ReadNuggetType());
        // 커서는 같은 장치 DWORD를 읽는다.
        Assert.Equal(77u, state.CurrentCursor!());
        cursor.Value = 78;
        Assert.Equal(78u, state.CurrentCursor!());
        // 권한은 같은 장기 서버 주소를 읽는다.
        storage.Authority().Boss = true;
        Assert.True(state.ReadBoss());
    }

    /// <summary>정렬 본체는 PATCH에서만 같은 풀/TYPE로 한 번 구성하고 재사용한다.</summary>
    [Fact]
    public void ContentFormsBindsOnceAndReuses()
    {
        var storage = Storage();
        // 장치 없이 정렬 본체를 요구하면 거부한다.
        Assert.Throws<InvalidOperationException>(() => storage.ContentForms());
        BindTransmission(storage);
        storage.BindContentDevices(Devices());
        Netstorm.Core.Original.RawContentFormHost first = storage.ContentForms();
        Assert.Same(first, storage.ContentForms());
        Assert.Same(storage.Pool, first.Pool);
        // CD 판본은 정렬 본체를 만들지 않는다.
        var types = Enumerable.Range(0, 171).Select(index => new Netstorm.Core.Original.OriginalTypeRecord
        {
            ConstructorAddress = 0,
            Flags1 = 0,
            MaxHitPoints = 1,
            FootX = 1,
            FootY = 1,
        }).ToArray();
        var frames = Enumerable.Range(0, 171).Select(_ => new Netstorm.Core.Original.RiftTypeFrames([])).ToArray();
        var shapes = Enumerable.Range(0, 171).Select(_ => new Netstorm.Core.Original.SquidDisplayShape()).ToArray();
        var cd = new Netstorm.Core.Original.RawSurfaceStorage(Netstorm.Core.Original.OriginalEdition.Cd1072, types, frames, shapes);
        Assert.Throws<InvalidOperationException>(() => cd.ContentForms());
    }

    /// <summary>빈 부모 통지는 조용히 넘기고 파생 통지는 현재 대여에 전달한다.</summary>
    [Fact]
    public void NoticeContentRoutesEmptyAndDerived()
    {
        var storage = Storage();
        BindTransmission(storage);
        storage.BindContentDevices(Devices());
        // 빈 부모(Form VT 6)는 인자를 읽지 않고 반환한다.
        int form = storage.FactoryExecutor().Create(6);
        // 생성된 Form 슬롯에 실제 Form VT를 기록한다.
        uint table = Netstorm.Core.Original.RawSquidForms.VtableFor(6);
        var raw = storage.Pool.RawSlot(form);
        raw[0] = (byte)table;
        raw[1] = (byte)(table >> 8);
        raw[2] = (byte)(table >> 16);
        raw[3] = (byte)(table >> 24);
        storage.NoticeContent(form, 0, 1);
        // 파생 대여 없이 파생 부모를 통지하면 명시 실패한다.
        int asset = storage.FactoryExecutor().Create(74);
        uint assetTable = 0x509148u;
        var assetRaw = storage.Pool.RawSlot(asset);
        assetRaw[0] = (byte)assetTable;
        assetRaw[1] = (byte)(assetTable >> 8);
        assetRaw[2] = (byte)(assetTable >> 16);
        assetRaw[3] = (byte)(assetTable >> 24);
        Assert.Throws<InvalidOperationException>(() => storage.NoticeContent(asset, form, 2));
        // 현재 대여를 연결하면 같은 인자를 전달한다.
        int seenParent = -1;
        int seenForm = -1;
        uint seenReason = 0;
        var pinOwner = new object();
        storage.BindContentRuntime(new Netstorm.Core.Original.RawSurfaceContentBindings
        {
            Notice = (parent, child, reason) => { seenParent = parent; seenForm = child; seenReason = reason; },
            Captured = (_, _) => { },
            Pin = () => pinOwner,
        });
        storage.NoticeContent(asset, form, 2);
        Assert.Equal(asset, seenParent);
        Assert.Equal(form, seenForm);
        Assert.Equal(2u, seenReason);
        // 중복 대여는 거부한다.
        Assert.Throws<ArgumentException>(() => storage.BindContentRuntime(new Netstorm.Core.Original.RawSurfaceContentBindings
        {
            Notice = (_, _, _) => { },
            Pin = () => pinOwner,
        }));
    }

    /// <summary>정렬 본체의 Pop/Unpop/Reattach 경계가 같은 장기 공장을 쓴다.</summary>
    [Fact]
    public void ContentFormsPopUnpopShareFactory()
    {
        var storage = Storage();
        BindTransmission(storage);
        storage.BindContentDevices(Devices());
        Netstorm.Core.Original.RawContentFormHost forms = storage.ContentForms();
        // Pop/Unpop 경계가 연결됐는지 확인한다.
        Assert.NotNull(forms);
        Assert.Same(storage.Pool, forms.Pool);
        // 같은 정렬 본문을 두 번 요구해도 새로 만들지 않는다.
        Assert.Same(forms, storage.ContentForms());
    }
}
